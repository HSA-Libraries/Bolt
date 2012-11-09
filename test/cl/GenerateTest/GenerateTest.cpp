/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
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


#include "common/stdafx.h"
#include "common/myocl.h"

#include <bolt/cl/generate.h>
//#include <bolt/miniDump.h>

//#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
//#include <array>

/******************************************************************************
 * checkResults
 *      compare std:: and bolt::cl:: results
 *      returns number of errors
 *****************************************************************************/
template<typename InputIterator1, typename InputIterator2>
int checkResults(std::string &msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 10;
	size_t sz = end1-first1 ;
	for (int i=0; i<sz ; i++) {
		if (first1 [i] != *(first2 + i) ) {
			errCnt++;
			if (errCnt < maxErrCnt) {
				std::cout << "\tMISMATCH[" << i << "] " << msg << " STL= " << first1[i] << "  BOLT=" << *(first2 + i) << std::endl;
			} else if (errCnt == maxErrCnt) {
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
			}
		};
	};

    if ( errCnt == 0 ) {
		//std::cout << " PASSED  " << msg << " Correct on all " << sz << " elements." << std::endl;
        printf(" PASSED %20s Correct for all %6i elements.\n", msg.c_str(), sz);
	} else {
		//std::cout << "*FAILED  " << msg << " Mismatch on " << errCnt << " / " << sz << " elements." << std::endl;
        printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", msg.c_str(), sz);
	};
    fflush(stdout);

	return errCnt;
};

/******************************************************************************
 * Pause Program to see Results
 *****************************************************************************/
void waitForEnter()
{
    std::cout << "Press <ENTER> to continue." << std::endl;
    std::cin.clear();
    std::cin.ignore(/*std::numeric_limits<std::streamsize>::max()*/1, '\n');
}

/******************************************************************************
 * Generator Gen1: return constant float
 *****************************************************************************/
BOLT_FUNCTOR(Gen1,
struct Gen1
{
    const float _a;
    Gen1( float a ) : _a(a) {};

	float operator() ()
	{
		return _a;
	};

};
);  // end BOLT_FUNCTOR

/******************************************************************************
 * Generator Gen2: return incrementing int, begining at base value
 *****************************************************************************/
BOLT_FUNCTOR(Gen2,
struct Gen2
{
	const int _a;
	Gen2( int a ) : _a(a) {};

	int operator() ()
	{
        return _a;
	};
};
);  // end BOLT_FUNCTOR

/******************************************************************************
 * Tests
 *****************************************************************************/
int testGen1DevVec( int length );
int testGen2DevVec( int length );
int testGenN1DevVec( int length );
int testGenN2DevVec( int length );
int testGenN12DevVec( int length, int cycle );
int testGen1HostVec( int length );
int testGen2HostVec( int length );
int testGenN1HostVec( int length );
int testGenN2HostVec( int length );
int testGenN12HostVec( int length, int cycle );
                                 
/******************************************************************************
 * Main
 *****************************************************************************/                                                 
int main(int argc, char* argv[])
{
#if 0
    // the below code demonstrates the short-comming of the device_vector::iterator at pointing to the middle of a buffer
    int length = 10;
    std::vector<int> gold(length);
    bolt::cl::device_vector<int> dv(length);
    // generator
    Gen2 genA(1);
    Gen2 genB(2);
    
    std::vector<int>::iterator goldIter = gold.begin();
    bolt::cl::device_vector<int>::iterator boltIter = dv.begin();
    printf("Bolt Iter Index %i\n", boltIter.getIndex());

    ////////////////////////////////////////////////////////////////////////////////
    printf("Vectors Before Generating\n");
    std::vector<int>::iterator tmpGoldIter = gold.begin();
    bolt::cl::device_vector<int>::iterator tmpBoltIter = dv.begin();
    
    for ( int i = 0; tmpGoldIter != gold.end(); tmpGoldIter++, tmpBoltIter++, i++ )
    {
        printf("Elem[%02i]: Std=%i, Bolt=%i\n", i, (int)(*tmpGoldIter), (int)(*tmpBoltIter));
    }

    goldIter += 3;
    boltIter += 3;
    printf("Bolt Iter Index %i\n", boltIter.getIndex());

    goldIter = std::generate_n(goldIter, 3, genA);
    boltIter = bolt::cl::generate_n(boltIter, 3, genA);
    printf("Bolt Iter Index %i\n", boltIter.getIndex());

    ////////////////////////////////////////////////////////////////////////////////////
    printf("Vectors After Generating 3,4,5\n");
    tmpGoldIter = gold.begin();
    tmpBoltIter = dv.begin();
    for ( int i = 0; tmpGoldIter != gold.end(); tmpGoldIter++, tmpBoltIter++, i++ )
    {
        printf("Elem[%02i]: Std=%i, Bolt=%i\n", i, (int)(*tmpGoldIter), (int)(*tmpBoltIter));
    }

    goldIter = std::generate_n(goldIter, 3, genB);
    boltIter = bolt::cl::generate_n(boltIter, 3, genB);
    printf("Bolt Iter Index %i\n", boltIter.getIndex());

    ////////////////////////////////////////////////////////////////////////////////////////
    printf("Vectors After Generating 6,7,8\n");
    tmpGoldIter = gold.begin();
    tmpBoltIter = dv.begin();
    for ( int i = 0; tmpGoldIter != gold.end(); tmpGoldIter++, tmpBoltIter++, i++ )
    {
        printf("Elem[%02i]: Std=%i, Bolt=%i\n", i, (int)(*tmpGoldIter), (int)(*tmpBoltIter));
    }
#endif
   

    // Test several vector lengths
    std::vector<int> lengths;
    lengths.push_back(0);
    lengths.push_back(1);
    lengths.push_back(63);
    lengths.push_back(64);
    lengths.push_back(65);
    lengths.push_back(1023);
    lengths.push_back(1024);
    lengths.push_back(1025);
    lengths.push_back(16384+1);

    int testsPassed = 0;
    int testsFailed = 0;

    // for each test length
    for (int iter = 0; iter < lengths.size(); iter++)
    {
        int i = iter % lengths.size();
        // Generator 1
        int errorCount = 0;

        /***********************************************************
         * Test Device Vectors
         **********************************************************/

        errorCount = testGen1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator 2
        errorCount = testGen2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator N 1
        errorCount = testGenN1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator N 2
        errorCount = testGenN2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

#if 0
        // Generator N 1,2
        // This test doesn't pass because of short-commings
        // in the device_vector::iterator at returning a cl::Buffer
        // which points to the middle of the raw buffer
        // the generate_n call is most likely correct, but can only be tested
        // after the device_vector::iterator is fixed. -DT
        errorCount = testGenN12DevVec(lengths[i], 3);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;
#endif

        /***********************************************************
         * Test Host Vectors
         **********************************************************/

        // Generator 1
        errorCount = 0;
        errorCount = testGen1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator 2
        errorCount = testGen2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator N 1
        errorCount = testGenN1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator N 2
        errorCount = testGenN2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Generator N 1,2
        errorCount = testGenN12HostVec(lengths[i], 11);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;
    }

    // Print final results
    printf("Final Results:\n");
    printf("%9i Tests Passed\n", testsPassed);
    printf("%9i Tests Failed\n", testsFailed);

    // Wait to exit
    waitForEnter();
    return 1;
}

/***********************************************************
 * Device Vector Tests
 **********************************************************/

// Generator Gen1, device_vector
int testGen1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
    // generator
    Gen1 genGold(3.14159f);
    Gen1 genBolt(3.14159f);
    // generate
    std::generate(gold.begin(), gold.end(), genGold);
    bolt::cl::generate(dv.begin(), dv.end(), genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator Gen2, device_vector
int testGen2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
    // generator
    Gen2 genGold(0);
    Gen2 genBolt(0);
    // generate
    std::generate(gold.begin(), gold.end(), genGold);
    bolt::cl::generate(dv.begin(), dv.end(), genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator N Gen1, device_vector
int testGenN1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
    // generator
    Gen1 genGold(3.14159f);
    Gen1 genBolt(3.14159f);
    // generate
    std::generate_n(gold.begin(), length, genGold);
    bolt::cl::generate_n(dv.begin(), length, genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator N Gen2, device_vector
int testGenN2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
    // generator
    Gen2 genGold(0);
    Gen2 genBolt(0);
    // generate
    std::generate_n(gold.begin(), length, genGold);
    bolt::cl::generate_n(dv.begin(), length, genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator Gen2, device_vector
int testGenN12DevVec( int length , int cycle)
{
    // function name for reporting
    std::string fName = __FUNCTION__;

    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
    // generator
    Gen1 genAGold(1.23f);
    Gen1 genABolt(1.23f);
    Gen1 genBGold(2.34f);
    Gen1 genBBolt(2.34f);
    
    // generate
    int numCycles = length / cycle;
    if (numCycles*cycle < length) numCycles++;

    std::vector<float>::iterator goldIter = gold.begin();
    bolt::cl::device_vector<float>::iterator boltIter = dv.begin();
    //OutputIterator boltIter = dv.begin();

    for (int c = 0; c < numCycles; c++)
    {
        if (c == numCycles-1)
            cycle = length%cycle;
        goldIter =      std::generate_n(goldIter, cycle, (c%2) ? genAGold : genBGold);
        boltIter = bolt::cl::generate_n(boltIter, cycle, (c%2) ? genABolt : genBBolt);
    }
    
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

/***********************************************************
 * Host Vector Tests
 **********************************************************/

// Generator Gen1, host_vector
int testGen1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);
    // generator
    Gen1 genGold(3.14159f);
    Gen1 genBolt(3.14159f);
    // generate
    std::generate(gold.begin(), gold.end(), genGold);
    bolt::cl::generate(dv.begin(), dv.end(), genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator Gen2, host_vector
int testGen2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);
    // generator
    Gen2 genGold(0);
    Gen2 genBolt(0);
    // generate
    std::generate(gold.begin(), gold.end(), genGold);
    bolt::cl::generate(dv.begin(), dv.end(), genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator N Gen1, host_vector
int testGenN1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);
    // generator
    Gen1 genGold(3.14159f);
    Gen1 genBolt(3.14159f);
    // generate
    std::generate_n(gold.begin(), length, genGold);
    bolt::cl::generate_n(dv.begin(), length, genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator N Gen2, host_vector
int testGenN2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);
    // generator
    Gen2 genGold(0);
    Gen2 genBolt(0);
    // generate
    std::generate_n(gold.begin(), length, genGold);
    bolt::cl::generate_n(dv.begin(), length, genBolt);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Generator Gen2, host_vector
int testGenN12HostVec( int length , int cycle)
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);
    // generator
    Gen1 genAGold(1.23f);
    Gen1 genABolt(1.23f);
    Gen1 genBGold(2.34f);
    Gen1 genBBolt(2.34f);
    
    // generate
    int numCycles = length / cycle;
    if (numCycles*cycle < length) numCycles++;

    std::vector<float>::iterator goldIter = gold.begin();
    std::vector<float>::iterator boltIter = dv.begin();

    for (int c = 0; c < numCycles; c++)
    {
        if (c == numCycles-1)
            cycle = length%cycle;
        goldIter =      std::generate_n(goldIter, cycle, (c%2) ? genAGold : genBGold);
        boltIter = bolt::cl::generate_n(boltIter, cycle, (c%2) ? genABolt : genBBolt);
    }
    
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

