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

#include <bolt/cl/count.h>
#include <bolt/cl/device_vector.h>
#include <bolt/cl/scan.h>
#include <bolt/cl/scatter.h>
#include <bolt/cl/transform.h>
#include <bolt/statisticalTimer.h>

#include <iostream>
#include <vector>
#include <numeric>

#define BITS_IN_UNSIGEND (8*sizeof(unsigned))

template <typename T>
void CheckAscending(T &input, size_t length)
{
    size_t index;
    for( index = 0; index < input.size( ) -1; ++index )
    {
        if(input[index] <= input[index+1])
            continue;
        else
            break;
    }
    if(index == (length-1))
    {
        std::cout << "PASSED....\n";
    }
    else
    {
        std::cout << "FAILED....\n";
    }
}

// Serial version of Radix Sort
void SerialRadixSort (const std::vector<unsigned> &input, size_t length, std::vector<unsigned> &answerSerial)
{
    std::vector<unsigned> vInput(input.begin(), input.end());
    std::vector<unsigned> vBuffer0(length);
    std::vector<unsigned> vBuffer1(length);

    for (int iter = 0; iter < BITS_IN_UNSIGEND; iter++)
    {
        int iOffsetB0 = 0;
        int iOffsetB1 = 0;
        int i = 0;

        // Separate 0s and 1s to separate buffers
        for (i = 0; i < length; i++)
        {
            if ((vInput[i] & (1<<iter)) == 0) // 0s
                vBuffer0[iOffsetB0++] = vInput[i];
            else // 1s
                vBuffer1[iOffsetB1++] = vInput[i];
        }
        // Copy 0s to the beginning of the vInput
        std::vector<unsigned>::iterator vInputNext = std::copy(vBuffer0.begin(), vBuffer0.begin() + iOffsetB0, vInput.begin());
        // Copy 1s to after 0s we just copied
        std::copy(vBuffer1.begin(), vBuffer1.begin() + iOffsetB1, vInputNext);
    }

    // Copy the answer 
    std::copy(vInput.begin(), vInput.end(), back_inserter(answerSerial));
}

// Functor for checking whether bit is 0 or 1
BOLT_FUNCTOR(find_zeros,
struct find_zeros
{
    unsigned iMask;

    find_zeros (unsigned iter) { iMask = 1 << iter;};

    bool operator()(const unsigned &x) const {return (x & iMask) == 0;}
};
);

BOLT_FUNCTOR(find_ones,
struct find_ones
{
    unsigned iMask;

    find_ones (unsigned iter) { iMask = 1 << iter;};

    bool operator()(const unsigned &x) const {return (x & iMask) != 0;}
};
);

// Bolt version of Radix Sort
void BoltRadixSort (const std::vector<unsigned> &input, size_t length, std::vector<unsigned> &answerBolt)
{
    bolt::cl::device_vector<unsigned> dvInput( input.begin(), input.end());
    bolt::cl::device_vector<unsigned> dvBuffer(length);
    bolt::cl::device_vector<unsigned> dvMaskedVals(length);
    bolt::cl::device_vector<unsigned> dvOffsetVals(length);

    for (int iter = 0; iter < BITS_IN_UNSIGEND; iter += 2)
    {
        // iter
        // Separate 0s first
        bolt::cl::transform(dvInput.begin(), dvInput.end(), dvMaskedVals.begin(), find_zeros(iter)); // Find elements with 0's, in parallel
        bolt::cl::exclusive_scan(dvMaskedVals.begin(), dvMaskedVals.end(), dvOffsetVals.begin()); // Figure out where in the buffer to copy to, in parallel
        bolt::cl::scatter_if(dvInput.begin(), dvInput.end(), dvOffsetVals.begin(), dvMaskedVals.begin(), dvBuffer.begin()); // Copy all the values to the buffer in parallel

        // And then, separate 1s
        int count = bolt::cl::count(dvMaskedVals.begin(), dvMaskedVals.end(), 1); // Count how many elements with 0's we already processed
        bolt::cl::transform(dvInput.begin(), dvInput.end(), dvMaskedVals.begin(), find_ones(iter)); // Find elements with 1's, in parallel
        bolt::cl::exclusive_scan(dvMaskedVals.begin(), dvMaskedVals.end(), dvOffsetVals.begin(), count); // Figure out where in the buffer to copy to, in parallel
        bolt::cl::scatter_if(dvInput.begin(), dvInput.end(), dvOffsetVals.begin(), dvMaskedVals.begin(), dvBuffer.begin()); // Copy all the values to the buffer in parallel

        // iter + 1
        // In order to avoid unnecessary copy operation, perfrom (iter+1)'th iteration in the same loop
        // Separate 0s first
        bolt::cl::transform(dvBuffer.begin(), dvBuffer.end(), dvMaskedVals.begin(), find_zeros(iter+1)); // Find elements with 0's first, in parallel
        bolt::cl::exclusive_scan(dvMaskedVals.begin(), dvMaskedVals.end(), dvOffsetVals.begin()); // Figure out where in the buffer to copy to, in parallel
        bolt::cl::scatter_if(dvBuffer.begin(), dvBuffer.end(), dvOffsetVals.begin(), dvMaskedVals.begin(), dvInput.begin()); // Copy all the values to the buffer in parallel

        // And then, separate 1s
        count = bolt::cl::count(dvMaskedVals.begin(), dvMaskedVals.end(), 1); // Count how many elements with 0's we already processed
        bolt::cl::transform(dvBuffer.begin(), dvBuffer.end(), dvMaskedVals.begin(), find_ones(iter+1)); // Find elements with 1's, in parallel
        bolt::cl::exclusive_scan(dvMaskedVals.begin(), dvMaskedVals.end(), dvOffsetVals.begin(), count); // Figure out where in the buffer to copy to, in parallel
        bolt::cl::scatter_if(dvBuffer.begin(), dvBuffer.end(), dvOffsetVals.begin(), dvMaskedVals.begin(), dvInput.begin()); // Copy all the values to the buffer in parallel
    }

    // Copy the answer 
    bolt::cl::device_vector<unsigned>::pointer pData = dvInput.data();
    std::copy(&pData[0], &pData[length], back_inserter(answerBolt));
}

int main()
{
    std::cout << "\nRadix Sort EXAMPLE \n";

    srand (time(NULL));

    // Prepare 2^10 elements of random unsigned numbers to be sorted
    size_t length = 1024*1024;
    std::vector<unsigned> input(length);
    std::generate(input.begin(), input.end(), rand);

    std::vector<unsigned> answerBolt;
    std::vector<unsigned> answerSerial;

    // Serial version of Radix Sort
    std::cout << "\nSorting STL vector of " << length << " unsigned integer elements using Serial Radix sort.\n";
    SerialRadixSort (input, length, answerSerial);
    CheckAscending (answerSerial, length);

    // Bolt version of Radix Sort
    std::cout << "\nSorting STL vector of " << length << " unsigned integer elements using Bolt Radix sort.\n";
    BoltRadixSort (input, length, answerBolt);
    CheckAscending (answerBolt, length);

    // Verify that answerSerial matches answerBolt
    std::cout << "\nComparing output of Serial Radix sort and Bolt Radix sort.\n";
    size_t i=0;
    for (i=0; i<answerSerial.size(); i++)
    {
        if (answerSerial[i] != answerBolt[i])
        {
            std::cout << "Mismatch!!!" << std::endl;
            break;
        }
    }
    if (i == answerSerial.size())
        std::cout << "Serial Radix sort and Bolt Radix sort matched!" << std::endl;

    std::cout << "COMPLETED. ...\n";
    return 0;
}