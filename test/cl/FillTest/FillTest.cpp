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
#include <bolt/cl/fill.h>

#define STRUCT 1


/******************************************************************************
 * checkResults
 *      compare std:: and bolt::cl:: results
 *      returns number of errors
 *		For testing struct types checkResults is embedded in the test function.
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
		printf(" PASSED %20s Correct for all %6i elements.\n", msg.c_str(), sz);
	} else {
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
    std::cin.ignore(1, '\n');
}



/******************************************************************************
 * Tests
 *****************************************************************************/
int testFill1DevVec( int length );
int testFill2DevVec( int length );
int testFillN1DevVec( int length );
int testFillN2DevVec( int length );
int testFill1HostVec( int length );
int testFill2HostVec( int length );
int testFillN1HostVec( int length );
int testFillN2HostVec( int length );

#if STRUCT
int teststruct( int length );
int teststruct_n( int length );
struct samp
{
	int x,y;

};

#endif


/******************************************************************************
 * Main
 *****************************************************************************/
int main(int argc, char* argv[])
{
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
        int errorCount = 0;

        /***********************************************************
         * Test Device Vectors
         **********************************************************/

		//Fill 1
        errorCount = testFill1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;


        /***********************************************************
         * Test Host Vectors
         **********************************************************/

        // Fill 1
        errorCount = 0;
        errorCount = testFill1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

#if STRUCT
		// Fill struct
		errorCount = teststruct(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

		errorCount = teststruct_n(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;
    
#endif
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
 * Device Vector Functions
 **********************************************************/

// Fill 1
int testFill1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill
    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::cl::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill
    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::cl::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

 // Fill_N 1
int testFillN1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill_N
    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::cl::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 2
int testFillN2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill_N
    std::fill_n(gold.begin(), length, 0.f);
    bolt::cl::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}



/***********************************************************
 * Host Vector Functions
 **********************************************************/

// Fill 1
int testFill1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::cl::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::cl::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::cl::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 0.f);
    bolt::cl::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

#if STRUCT
/*	
*
*Fill and Fill_N Host/Device Vector Tests
*
*/

 // Fill 
int teststruct( int length )
{
	int errCnt=0;
	static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
	bolt::cl::device_vector<samp>dv(length);
	struct samp s1,temp;			/*temp is used to map back the device_vector to host_vector*/
	s1.x=10;
	s1.y=20;
    std::fill(gold.begin(), gold.end(),s1);
    bolt::cl::fill(hv.begin(), hv.end(),s1);
	bolt::cl::fill(dv.begin(), dv.end(),s1);
    //check results

    for (int i=0; i<length ; i++) {
		temp = dv[i];
		if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
				errCnt++;
				if (errCnt < maxErrCnt) 
				{
				std::cout<<"\tMISMATCH"<<std::endl;
				}
				else if (errCnt == maxErrCnt) 
				{
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
				}
		}
	}
	 if ( errCnt == 0 ) {
		printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
	} else {
		printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", fName.c_str(), length);
	};
    fflush(stdout);
	return errCnt;
}

// Fill_n 
int teststruct_n( int length )
{
	int errCnt=0;
	static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
	bolt::cl::device_vector<samp>dv(length);
	struct samp s1,temp;			/*temp is used to map back the device_vector to host_vector*/
	s1.x=10;
	s1.y=20;
    std::fill_n(gold.begin(), length,s1);
    bolt::cl::fill_n(hv.begin(), length,s1);
	bolt::cl::fill_n(dv.begin(), length,s1);
    //check results

    for (int i=0; i<length ; i++) {
		temp = dv[i];
		if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
				errCnt++;
				if (errCnt < maxErrCnt) 
				{
				std::cout<<"\tMISMATCH"<<std::endl;
				}
				else if (errCnt == maxErrCnt) 
				{
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
				}
		}
	}
	 if ( errCnt == 0 ) {
		printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
	} else {
		printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", fName.c_str(), length);
	};
    fflush(stdout);
	return errCnt;
}


#endif