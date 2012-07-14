#include "stdafx.h"
#include <stdio.h>

#include <numeric>
#include <limits>
#include <list>	// For debugging purposes, to prove that we can reject lists

#include <bolt/AMP/functional.h>
#include <bolt/AMP/scan.h>
#include <bolt/unicode.h>

#include <gtest/gtest.h>

template<typename InputIterator1, typename InputIterator2>
int checkResults( bolt::tstring msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 20;
	size_t sz = end1-first1 ;
	for (int i=0; i<sz ; i++) {
		if (first1 [i] != first2 [i]) {
			errCnt++;
			if (errCnt < maxErrCnt) {
				bolt::tout << "MISMATCH " << msg << " STL= " << first1[i] << "  BOLT=" << first2[i] << std::endl;
			} else if (errCnt == maxErrCnt) {
				bolt::tout << "Max error count reached; no more mismatches will be printed...\n";
			}
		};
	};

	if (errCnt==0) {
		bolt::tout << " PASSED  " << msg << " Correct on all " << sz << " elements." << std::endl;
	} else {
		bolt::tout << "*FAILED  " << msg << "mismatch on " << errCnt << " / " << sz << " elements." << std::endl;
	};

	return errCnt;
};


// Simple test case for bolt::inclusive_scan:
// Sum together specified numbers, compare against STL::partial_sum function.
// Demonstrates:
//    * use of bolt with STL::array iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
template< size_t arraySize >
void simpleScanArray( )
{
	bolt::tstring fName( _T( __FUNCTION__ ) );
	fName += _T( ":" );

	std::array< int, arraySize > stdA, boltA;
	std::array< int, arraySize > stdB, boltB;

	for (int i=0; i < arraySize; i++) {
//		stdA[i] = i;
//		boltA[i] = 1;
		stdA[i] = 1;
		boltA[i] = 1;
	};

	//Out-of-place
	std::partial_sum( stdA.begin( ), stdA.end( ), stdB.begin( ) );
	bolt::inclusive_scan( boltA.begin( ), boltA.end( ), boltB.begin( ) );
	checkResults( fName + _T( "Out-of-place" ), stdB.begin(), stdB.end(), boltB.begin() );

	//In-place
	std::partial_sum( stdB.begin( ), stdB.end( ), stdB.begin( ) );
	bolt::inclusive_scan( boltB.begin( ), boltB.end( ), boltB.begin( ) );
	checkResults( fName + _T( "In-place" ), stdB.begin(), stdB.end(), boltB.begin() );

	// Binary operator
	//bolt::inclusive_scan( boltA.begin( ), boltA.end(), boltA.begin( ), bolt::plus<int>( ) );

	// Invalid calls
	//bolt::inclusive_scan( boltA.rbegin( ), boltA.rend( ) );  // reverse iterators should not be supported

	//printf ("Sum: stl=%d,  bolt=%d %d %d\n", stlScan, boltScan, boltScan2, boltScan3 );
};

// Test driver function
void simpleScan()
{
	//	Tests within 1 wavefront
	simpleScanArray< 64 >( );
	simpleScanArray< 63 >( );
	simpleScanArray< 32 >( );
	simpleScanArray< 31 >( );
	simpleScanArray< bolt::scanGpuThreshold - 1 >( );
	simpleScanArray< bolt::scanMultiCpuThreshold - 1 >( );
	simpleScanArray< 1 >( );

	//	Tests with multi-wavefronts
	simpleScanArray< 127 >( );
	simpleScanArray< 128 >( );
	simpleScanArray< 129 >( );
	simpleScanArray< 1000 >( );
	simpleScanArray< 1053 >( );

	//	Huge Arrays
	simpleScanArray< 4096 >( );
	simpleScanArray< 4097 >( );
	simpleScanArray< 65535 >( );
	simpleScanArray< 65536 >( );
	simpleScanArray< 131032 >( );
	simpleScanArray< 262154 >( );

	////	Stack overflows
	//simpleScanArray< 1048576 >( );
	//simpleScanArray< 52428800 >( );

	//	This results in a compile error; is this OK?
//	simpleScanArray< 0 >( );

	//simpleScan3<float> ("sum", bolt::plus<float>(), 1000000, .0001/*errorThresh*/);
	//simpleScan3<float> ("min", bolt::minimum<float>(), 1000000);
	//simpleScan3<float> ("max", bolt::maximum<float>(), 1000000);

	//simpleScan4();
};

TEST( ScanSingleWavefront, ArraySize64 )
{
    simpleScanArray< 64 >( );
}

TEST( ScanSingleWavefront, ArraySize63 )
{
    simpleScanArray< 63 >( );
}

TEST( ScanSingleWavefront, ArraySize32 )
{
    simpleScanArray< 32 >( );
}

TEST( ScanSingleWavefront, ArraySize31 )
{
    simpleScanArray< 31 >( );
}

TEST( ScanSingleWavefront, ArraySize1 )
{
    simpleScanArray< 1 >( );
}

TEST( ScanMultiWavefront, ArraySize127 )
{
    simpleScanArray< 127 >( );
}

TEST( ScanMultiWavefront, ArraySize128 )
{
    simpleScanArray< 128 >( );
}

TEST( ScanMultiWavefront, ArraySize129 )
{
    simpleScanArray< 129 >( );
}

TEST( ScanMultiWavefront, ArraySize1000 )
{
    simpleScanArray< 1000 >( );
}

TEST( ScanMultiWavefront, ArraySize1053 )
{
    simpleScanArray< 1053 >( );
}

TEST( ScanHugeWavefront, ArraySize4096 )
{
    simpleScanArray< 4096 >( );
}

TEST( ScanHugeWavefront, ArraySize4097 )
{
    simpleScanArray< 4097 >( );
}

TEST( ScanHugeWavefront, ArraySize65535 )
{
    simpleScanArray< 65535 >( );
}

TEST( ScanHugeWavefront, ArraySize65536 )
{
    simpleScanArray< 65536 >( );
}

TEST( ScanHugeWavefront, ArraySize131032 )
{
    simpleScanArray< 131032 >( );
}

TEST( ScanHugeWavefront, ArraySize262154 )
{
    simpleScanArray< 262154 >( );
}

int _tmain(int argc, _TCHAR* argv[])
{
	::testing::InitGoogleTest( &argc, &argv[ 0 ] );

	return RUN_ALL_TESTS();
}