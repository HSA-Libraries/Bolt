/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   

***************************************************************************/                                                                                     
#if (_MSC_VER == 1700)
#define NOMINMAX

//#include "Common/Options.hpp"
//#include "Common/Output.hpp"

#include <amp.h>
#include <amp_short_vectors.h>
#include <iostream>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <vector>

// Number of threads per AMP tile in reduction and bottom scan.
// (Top scan must have cNTiles threads per tile.)
#define cTileSize 256

// Number of AMP tiles to launch for reduction and bottom scan.
// (Top scan launch consists of a single tile only.)
#define cNTiles 64

using namespace concurrency;
using namespace concurrency::graphics;

// Computes an unmerged set of histograms of the low 4-bit values
// of a given set of integers, following a parameterized bitwise
// right shift of each element.
void BuildHistograms(array<unsigned int> &integers,
                     array<unsigned int> &histograms,
                     int shift)
{
  parallel_for_each(tiled_extent<cTileSize>(extent<1>(cNTiles * cTileSize)),
    [&integers, &histograms, shift](tiled_index<cTileSize> idx) restrict(amp)
  {
    // Shared memory for in-tile histogram reduction.
    tile_static unsigned int reduceIn[cTileSize];

    // Private histogram (counts of 0-15) for this thread.
    unsigned int histogram[16] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0 };

    // Compute our tile's assigned region of the integer set.
    const int regionSize  = integers.extent[0] / cNTiles;
    const int regionStart = idx.tile[0] * regionSize;
    const int regionEnd   = regionStart + regionSize;

    // Stride threads across the tile's region.
    for(int i = regionStart + idx.local[0];
            i < regionEnd;
            i += cTileSize)
    {
      // Build a histogram of the low 4 bytes (0-15) after a right shift.
      // Function is parameterized by shift [0,4,8,12,16,20,24,28].
      ++ histogram[(integers[i] >> shift) & 0xF];
    }

    // Reduce histograms across the tile.
    for(int digit = 0; digit < 16; ++ digit) {
      // Write our value to shared memory.
      reduceIn[idx.local[0]] = histogram[digit];
      idx.barrier.wait_with_tile_static_memory_fence();

      // Subsets of threads reduce histogram entry to a single value.
      for(int i = cTileSize / 2; i > 0; i >>= 1) {
        if(idx.local[0] < i) {
          reduceIn[idx.local[0]] += reduceIn[idx.local[0] + i];
        }

        idx.barrier.wait_with_tile_static_memory_fence();
      }

      // One thread writes reduced value to intermediate sum array.
      if(idx.local[0] == 0) {
        histograms[digit * cNTiles + idx.tile[0]] = reduceIn[0];
      }
    }
  });
}

// Performs an exclusive scan across integer values provided by each thread.
// The corresponding scanned values are returned back to each thread.
template <int TILESIZE>
unsigned int Scan(unsigned int value,
                  tiled_index<TILESIZE> &idx) restrict(amp)
{
  // Shared memory for in-tile scan.
  tile_static unsigned int scanIn[2 * TILESIZE];

  // Set the lower half of shared memory to zero.
  scanIn[idx.local[0]] = 0;

  // Copy each thread's value into the upper half.
  unsigned int *scanInUpper = &scanIn[TILESIZE];
  scanInUpper[idx.local[0]] = value;
  idx.barrier.wait_with_tile_static_memory_fence();

  // Perform a Kogge-Stone scan across values.
  for(int i = 1; i < TILESIZE; i <<= 1) {
    unsigned int tmpVal = scanInUpper[idx.local[0] - i];
    idx.barrier.wait_with_tile_static_memory_fence();

    scanInUpper[idx.local[0]] += tmpVal;
    idx.barrier.wait_with_tile_static_memory_fence();
  }

  return scanInUpper[idx.local[0] - 1];
}

// Performs a scan of each digit in a given set of histograms (0-15 counts).
// Scanned histograms are not merged, but structured the same as the source array.
void ScanHistograms(array<unsigned int> &histograms) {
  parallel_for_each(tiled_extent<cNTiles>(extent<1>(cNTiles)),
    [&histograms](tiled_index<cNTiles> idx) restrict(amp)
  {
    // Shared value recording the start of the current histogram's scan.
    // To break a scan into multiple small scans, need to carry across the
    // last value from one to the first value of the next.
    tile_static unsigned int scanBaseValue;
    scanBaseValue = 0;

    // Scan each digit across the histogram.
    for(int digit = 0; digit < 16; ++ digit) {
      // Perform a scan across the histogram value computed by each tile in BuildHistograms.
      // There is exactly one thread per histogram (cNTiles threads total).
      unsigned int digitCount = histograms[digit * cNTiles + idx.local[0]];
      unsigned int scannedDigitCount = Scan(digitCount, idx);
      histograms[digit * cNTiles + idx.local[0]] = scannedDigitCount + scanBaseValue;

      // Carry the highest scanned value across to the next histogram.
      // Scan is exclusive, so include digit count from the last thread.
      if(idx.local[0] == cNTiles - 1) {
        scanBaseValue += scannedDigitCount + digitCount;
      }

      idx.barrier.wait_with_tile_static_memory_fence();
    }
  });
}

void SortIntegerKeys(array<unsigned int> &integers,
                     array<unsigned int> &sortedIntegers,
                     array<unsigned int> &scannedHistograms,
                     int shift)
{
  parallel_for_each(tiled_extent<cTileSize>(extent<1>(cNTiles * cTileSize)),
    [&integers, &sortedIntegers, &scannedHistograms, shift](tiled_index<cTileSize> idx) restrict(amp)
  {
    // Shared cache of scanned digit (0-15) counts for this tile's assigned histogram.
    tile_static unsigned int scannedDigitCounts[16];

    // Cross-tile/region accumulation of histogram values (see next declaration).
    tile_static unsigned int tileHistogram[16];

    // Private histogram to track the next available
    // positions for integers sorted in this pass.
    unsigned int histogram[16];

    // Zero accumulated histogram and read scanned digit counts into cache.
    if(idx.local[0] < 16) {
      tileHistogram[idx.local[0]] = 0;
      scannedDigitCounts[idx.local[0]] = scannedHistograms[idx.local[0] * cNTiles + idx.tile[0]];
    }

    // Work on four integer elements per iteration per thread.
    uint_4 *integers4 = (uint_4 *)&integers[0];
    int nIntegers4 = integers.extent[0] >> 2;

    // Compute our tile's region of the integer set.
    const int blockSize  = nIntegers4 / cNTiles;
    const int blockStart = idx.tile[0] * blockSize;
    const int blockEnd   = blockStart + blockSize;

    // Stride threads across the tile's region.
    // Note: Cannot initialize with (blockStart + idx.local[0]) because
    // HLSL cannot statically determine that control flow through barriers
    // inside the loop is not divergent.
    for(int i = blockStart; i < blockEnd; i += cTileSize) {
      // Reset private histogram to zero for accumulation to tile histogram.
      for(int j = 0; j < 16; ++ j) {
        histogram[j] = 0;
      }

      // Read four integers and shift/mask to extract the keys being sorted.
      uint_4 integer4 = integers4[i + idx.local[0]];
      uint_4 key4 = (integer4 >> shift) & 0xF;

      // Track digit counts locally prior to tile-wide scan.
      ++ histogram[key4.x];
      ++ histogram[key4.y];
      ++ histogram[key4.z];
      ++ histogram[key4.w];

      // Perform a tile-wide exclusive scan across new histogram values for each digit.
      // This assigns regions in the output for each set of digits in each thread.
      for(int j = 0; j < 16; ++ j) {
        histogram[j] = Scan(histogram[j], idx);
      }

      // Write four integers to key-sorted regions of the output set.
      // Increment histogram counts further to make exclusive scan inclusive.
      unsigned int sortedIdx;

      sortedIdx = histogram[key4.x] + scannedDigitCounts[key4.x] + tileHistogram[key4.x];
      sortedIntegers[sortedIdx] = integer4.x;
      ++ histogram[key4.x];

      sortedIdx = histogram[key4.y] + scannedDigitCounts[key4.y] + tileHistogram[key4.y];
      sortedIntegers[sortedIdx] = integer4.y;
      ++ histogram[key4.y];

      sortedIdx = histogram[key4.z] + scannedDigitCounts[key4.z] + tileHistogram[key4.z];
      sortedIntegers[sortedIdx] = integer4.z;
      ++ histogram[key4.z];

      sortedIdx = histogram[key4.w] + scannedDigitCounts[key4.w] + tileHistogram[key4.w];
      sortedIntegers[sortedIdx] = integer4.w;
      ++ histogram[key4.w];

      // Record the final, inclusive-scanned locations for the next sorting pass.
      idx.barrier.wait_with_tile_static_memory_fence();

      if(idx.local[0] == (cTileSize - 1)) {
        for(int j = 0; j < 16; ++ j) {
          tileHistogram[j] += histogram[j];
        }
      }

      idx.barrier.wait_with_tile_static_memory_fence();
    }
  });
}

void Sort(array<unsigned int> &integers,
          array<unsigned int> &tmpIntegers,
          array<unsigned int> &tmpHistograms)
{
  for(int shift = 0; shift < 32; shift += 4) {
    // Swap input and output vectors on each iteration.
    bool swapVectors = (shift % 8 != 0);
    array<unsigned int> &srcIntegers = (swapVectors ? tmpIntegers : integers);
    array<unsigned int> &dstIntegers = (swapVectors ? integers    : tmpIntegers);

    BuildHistograms(srcIntegers, tmpHistograms, shift);
    ScanHistograms(tmpHistograms);
    SortIntegerKeys(srcIntegers, dstIntegers, tmpHistograms, shift);
  }
}
/*
void TestSort(Options &options) {
  // Randomize host-side integer set for validation.
  int nIntegerElems = options.problemSize() * 1024 * 1024;
  std::vector<unsigned int> integers(nIntegerElems);

  std::uniform_int_distribution<unsigned int> rngDist;
  std::mt19937 rng;

  for(int i = 0; i < nIntegerElems; ++ i) {
    integers[i] = rngDist(rng);
  }

  // Copy vector to the device.
  array<unsigned int> dIntegers(nIntegerElems, integers.begin());

  // Allocate space for temporary integers and histograms.
  array<unsigned int> dTmpIntegers(nIntegerElems);
  array<unsigned int> dTmpHistograms(cNTiles * 16);

  // Repeat the benchmark and compute a mean kernel execution time.
  AMPSerialTimer execTimer;

  for(int run = 0; run < options.nBenchmarkRuns(); ++ run) {
    // Execute and time the kernel.
    execTimer.startRun();
    Sort(dIntegers, dTmpIntegers, dTmpHistograms);
    execTimer.stopRun();
  }

  // Report mean/SD/throughput to the user.
  std::stringstream modeStr;
  modeStr << "Radix sort " << options.problemSize() << "M (2^20) integer keys";

  double mKeys = double(nIntegerElems) / 1000000.0;
  OutputData(options, execTimer, modeStr.str(), mKeys, "Mkeys/s");

  if(options.doValidation()) {
    // Check that the output vector is fully sorted.
    // As a further approximate inclusion test, compare the sums of both vectors.
    std::cout << "  [Validating... ";
    std::cout.flush();

    std::vector<unsigned int> sortedIntegers(nIntegerElems);
    copy(dIntegers, sortedIntegers.begin());

    uint64_t integerSum       = integers[0];
    uint64_t sortedIntegerSum = sortedIntegers[0];

    for(int i = 1; i < nIntegerElems; ++ i) {
      if(sortedIntegers[i] < sortedIntegers[i - 1]) {
        std::cerr << "failed at " << i << ": ";
        std::cerr << sortedIntegers[i] << " < " << sortedIntegers[i - 1] << "]\n\n";
        std::cerr << "Benchmark aborted due to error\n";
        exit(1);
      }

      integerSum       += integers[i];
      sortedIntegerSum += sortedIntegers[i];
    }

    if(integerSum != sortedIntegerSum) {
      std::cerr << "failed: input/output vector sum mismatch]\n\n";
      std::cerr << "Benchmark aborted due to error\n";
      exit(1);
    }

    std::cout << "passed]\n";
  }
  else {
    std::cout << "  [Skipped validation]\n";
  }

  std::cout << "\n";
  std::cout.flush();
}

int main(int argc,
         char *argv[])
{
  // Parse command-line options.
  Options options(argc, argv,
    "Sort",
    "Performs a radix sort on an array of randomized integer data.",
    "Array size in M (2^20) integer elements", 8
  );

  // Print benchmarking information.
  OutputSummary(options);

  // Run the radix sort test.
  TestSort(options);
}

*/

#endif