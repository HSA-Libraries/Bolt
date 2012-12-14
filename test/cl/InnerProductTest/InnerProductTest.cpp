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

// InnerProductTest.cpp : Defines the entry point for the console application.
//
#define OCL_CONTEXT_BUG_WORKAROUND 1

#include "stdafx.h"
#include <bolt/cl/inner_product.h>
#include <bolt/cl/functional.h>
#include <bolt/cl/control.h>

#include <iostream>
#include <algorithm>  // for testing against STL functions.
#include <numeric>

#include "common/myocl.h"
#include <gtest/gtest.h>

#include <bolt/cl/functional.h>
#include <bolt/cl/device_vector.h>
#include <bolt/unicode.h>
#include <bolt/miniDump.h>


extern void testDeviceVector();

// Super-easy windows profiling interface.
// Move to timing infrastructure when that becomes available.
template<typename T>
void printCheckMessage(bool err, std::string msg, T  stlResult, T boltResult)
{
    if (err) {
        std::cout << "*ERROR ";
    } else {
        std::cout << "PASSED ";
    }

    std::cout << msg << "  STL=" << stlResult << " BOLT=" << boltResult << std::endl;
};

template<typename T>
bool checkResult(std::string msg, T  stlResult, T boltResult)
{
    bool err =  (stlResult != boltResult);
    printCheckMessage(err, msg, stlResult, boltResult);

    return err;
};


// For comparing floating point values:
template<typename T>
bool checkResult(std::string msg, T  stlResult, T boltResult, double errorThresh)
{
    bool err;
    if ((errorThresh != 0.0) && stlResult) {
        double ratio = (double)(boltResult) / (double)(stlResult) - 1.0;
        err = abs(ratio) > errorThresh;
    } else {
        // Avoid div-by-zero, check for exact match.
        err = (stlResult != boltResult);
    }

    printCheckMessage(err, msg, stlResult, boltResult);
    return err;
};



// Simple test case for bolt::inner_product
// Sum together specified numbers, compare against STL::inner_product function.
// Demonstrates:
//    * use of bolt with STL::vector iterators
//    * use of bolt with default plus 
//    * use of bolt with explicit plus argument
void simpleInProd1(int aSize)
{
    std::vector<int> A(aSize), B(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = B[i] = i;
    };

    int stlInProd = std::inner_product(A.begin(), A.end(), B.begin(), 0);
	int boltInProd = bolt::cl::inner_product(A.begin(), A.end(), B.begin(),0, bolt::cl::plus<int>(), bolt::cl::multiplies<int>());

    checkResult("simpleInProd1", stlInProd, boltInProd);
 
};



void floatInProd1(int aSize)
{
    std::vector<float> A(aSize), B(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = B[i] = (float) 3.5;
    };

    float stlInProd = std::inner_product(A.begin(), A.end(), B.begin(), 0.0f);
	float boltInProd = bolt::cl::inner_product(A.begin(), A.end(), B.begin(),0.0f);

	double error_threshold = 0.0;
    checkResult("floatInProd1", stlInProd, boltInProd,0);
 
};

void ctlInprod(){
	int aSize=64;
	int numIters = 100;
	 std::vector<int> A(aSize),B(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = i;
		B[i] = i;
    };

	::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    bolt::cl::control c( getQueueFromContext(myContext, CL_DEVICE_TYPE_GPU, 0 )); 



    //printContext(c.context());

    c.debug(bolt::cl::control::debug::Compile + bolt::cl::control::debug::SaveCompilerTemps);

    int stlReduce = std::inner_product(A.begin(), A.end(), B.begin(),0);
    int boltReduce = 0;

    


    boltReduce = bolt::cl::inner_product(c, A.begin(), A.end(), B.begin(),0);
   
    checkResult("ctlInProduct", stlReduce, boltReduce);
}

void InProdDV()
{
    const int aSize = 64;
    std::vector<int> hA(aSize), hB(aSize);
    bolt::cl::device_vector<int> dA(aSize), dB(aSize);

    for(int i=0; i<aSize; i++) {
        hA[i] = hB[i] = dB[i] = dA[i] = i;
    };

    int hSum = std::inner_product(hA.begin(), hA.end(), hB.begin(), 1);

	int sum = bolt::cl::inner_product(dA.begin(), dA.end(), dB.begin(), 1,bolt::cl::plus<int>(),bolt::cl::multiplies<int>());
	 checkResult("InProductDeviceVector", hSum, sum);
};

int _tmain(int argc, _TCHAR* argv[])
{
    testDeviceVector();

    int numIters = 100;

	simpleInProd1(64);
    simpleInProd1(256);
    simpleInProd1(1024);
	simpleInProd1(2048);
	simpleInProd1(4096);

	floatInProd1(64);
	floatInProd1(256);
	floatInProd1(1024);
	floatInProd1(2048);
	floatInProd1(4096);
	
	ctlInprod();
	InProdDV();

	return 0;
}

