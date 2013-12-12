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
/******************************************************************************
 *  Benchmark Bolt Functions
 *****************************************************************************/
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <vector>
#include <algorithm>
#include <boost/program_options.hpp>
#define Bolt_Benchmark 1
#include <bolt/unicode.h>
#include "bolt/unicode.h"
#include <CL/cl.h>
#include "statisticalTimer.h"

#if (Bolt_Benchmark == 1)
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/generate.h"
#include "bolt/cl/binary_search.h"
#include "bolt/cl/copy.h"
#include "bolt/cl/count.h"
#include "bolt/cl/fill.h"
#include "bolt/cl/max_element.h"
#include "bolt/cl/min_element.h"
#include "bolt/cl/merge.h"
#include "bolt/cl/transform.h"
#include "bolt/cl/scan.h"
#include "bolt/cl/sort.h"
#include "bolt/cl/reduce.h"
#include "bolt/cl/sort_by_key.h"
#include "bolt/cl/stablesort.h"
#include "bolt/cl/reduce_by_key.h"
#include "bolt/cl/stablesort_by_key.h"
#include "bolt/cl/transform_scan.h"
#include "bolt/cl/scan_by_key.h"
#include "bolt/cl/gather.h"
#include "bolt/cl/scatter.h"
#include <random>
//#define BOLT_PROFILER_ENABLED
#define BOLT_BENCH_DEVICE_VECTOR_FLAGS CL_MEM_READ_WRITE
#include "bolt/AsyncProfiler.h"
#include "bolt/countof.h"
#else
#include <thrust/device_vector.h>
#include <thrust/transform.h>
#include <thrust/merge.h>
#include <thrust/sort.h>
#include <thrust/binary_search.h>
#include <thrust/transform_reduce.h>
#include <thrust/reduce.h>
#include <thrust/generate.h>
#include <thrust/extrema.h>
#include <thrust/fill.h>
#include <thrust/count.h>
#include <thrust/scan.h>
#include <thrust/transform_scan.h>
#include <thrust/gather.h>
#include <thrust/random.h>
#include <iomanip>
#include "PerformanceCounter.h"
#endif
#if defined(_WIN32)
AsyncProfiler aProfiler("default");
#endif
const std::streamsize colWidth = 26;

//#define DATA_TYPE float
//BOLT_CREATE_DEFINE(Bolt_DATA_TYPE,DATA_TYPE,float);
#ifndef DATA_TYPE  
#define DATA_TYPE unsigned int
#if (Bolt_Benchmark == 1)
BOLT_CREATE_DEFINE(Bolt_DATA_TYPE,DATA_TYPE,unsigned int);
#endif
#endif 
//user defined data types and functions and predicates are dedined
#include "data_type.hpp"

// function generator:
DATA_TYPE RandomNumber() 
{
    return rand()*rand()*rand();
}

/******************************************************************************
 *  Functions Enumerated
 *****************************************************************************/
static const size_t FList = 23;
enum functionType {
    f_binarytransform,
    f_binarysearch,
    f_copy,
    f_count,
    f_fill,
    f_generate,
    f_innerproduct,
    f_maxelement,
    f_minelement,
    f_merge,
    f_reduce,
    f_reducebykey,
    f_scan,
    f_scanbykey,
    f_sort,
    f_sortbykey,
    f_stablesort,
    f_stablesortbykey,
    f_transformreduce,
    f_transformscan,
    f_unarytransform,
    f_gather,
    f_scatter
};
static char *functionNames[] = {
"binarytransform",
"binarysearch",
"copy",
"count",
"fill",
"generate",
"innerproduct",
"maxelement",
"minelement",
"merge",
"reduce",
"reducebykey",
"scan",
"scanbykey",
"sort",
"sortbykey",
"stablesort",
"stablesortbykey",
"transformreduce",
"transformscan",
"unarytransform",
"gather",
"scatter"
};

/******************************************************************************
 *  Data Types Enumerated
 *****************************************************************************/
enum dataType {
    t_int,
    t_vec2,
    t_vec4,
    t_vec8
};
static char *dataTypeNames[] = {
    "int1",
    "vec2",
    "vec4",
    "vec8"
};
namespace po = boost::program_options;
using namespace std;

/******************************************************************************
 *  Initializers
 *****************************************************************************/
vec2 v2init = { 1, 1 };
vec2 v2iden = { 0, 0 };
vec4 v4init = { 1, 1, 1, 1 };
vec4 v4iden = { 0, 0, 0, 0 };
vec8 v8init = { 1, 1, 1, 1, 1, 1, 1, 1 };
vec8 v8iden = { 0, 0, 0, 0, 0, 0, 0, 0 };

/******************************************************************************
 *
 *  Execute Function 
 *
 *****************************************************************************/
functionType get_functionindex(std::string &fun)
{
    for(int i =0 ; i < FList; i++)
    {
        if(fun.compare(functionNames[i]) == 0)
            return (functionType)i;
    }
    std::cout<< "Specified Function not listed for Benchmark. exiting";
    exit(0);
}

/******************************************************************************
 *
 *  Execute Function Type
 *
 *****************************************************************************/
template<
    typename VectorType,
    typename Generator,
    typename UnaryFunction,
    typename BinaryFunction,
    typename BinaryPredEq,
    typename BinaryPredLt >
#if (Bolt_Benchmark == 1)
void executeFunctionType(
    bolt::cl::control& ctrl,
    VectorType &input1,
    VectorType &input2,
    VectorType &input3,
    VectorType &output,
    VectorType &output_merge,
    Generator generator,
    UnaryFunction unaryFunct,
    BinaryFunction binaryFunct,
    BinaryPredEq binaryPredEq,
    BinaryPredLt binaryPredLt,
    size_t function,
    size_t iterations,
    size_t siz
    )
#else
void executeFunctionType(
    VectorType &input1,
    VectorType &input2,
    VectorType &input3,
    VectorType &output,
    VectorType &output_merge,
    Generator generator,
    UnaryFunction unaryFunct,
    BinaryFunction binaryFunct,
    BinaryPredEq binaryPredEq,
    BinaryPredLt binaryPredLt,
    size_t function,
    size_t iterations,
    size_t siz
    )
#endif
{

    bolt::statTimer& myTimer = bolt::statTimer::getInstance( );
    myTimer.Reserve( 1, iterations );
    size_t testId	= myTimer.getUniqueID( _T( "test" ), 0 );
    switch(function)
    {
    case f_merge: 
        {
            std::cout <<  functionNames[f_merge] << std::endl;
#if (Bolt_Benchmark == 1)
            bolt::cl::sort( ctrl, input1.begin( ), input1.end( ), binaryPredLt);
            bolt::cl::sort( ctrl, input2.begin( ), input2.end( ), binaryPredLt);
#else
            thrust::sort( input1.begin( ), input1.end( ), binaryPredLt);
            thrust::sort( input2.begin( ), input2.end( ), binaryPredLt);
#endif
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::merge( ctrl,input1.begin( ),input1.end( ),input2.begin( ),input2.end( ),output_merge.begin( ),binaryPredLt); 
#else
                thrust::merge( input1.begin( ),input1.end( ),input2.begin( ),input2.end( ),output_merge.begin( ),binaryPredLt); 
#endif
                myTimer.Stop( testId );
            }
        } 
        break; 
    case f_binarysearch: 
        {
            bool tmp;
            typename VectorType::value_type val;
            std::cout <<  functionNames[f_binarysearch] << std::endl;
#if (Bolt_Benchmark == 1)
            bolt::cl::sort( ctrl, input1.begin( ), input1.end( ), binaryPredLt);
#else
            thrust::sort( input1.begin( ), input1.end( ), binaryPredLt);
#endif
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                int index = 0;
                if(iter!=0)
                    index = rand()%iter;
                val = input1[index];
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                tmp = bolt::cl::binary_search( ctrl,input1.begin( ),input1.end( ),val,binaryPredLt); 
#else
                tmp = thrust::binary_search( input1.begin( ),input1.end( ),val,binaryPredLt); 
#endif
                myTimer.Stop( testId );
            }
        } 
        break;	    
    case f_transformreduce: 
        {
            typename VectorType::value_type tmp;
            tmp=0;
            std::cout <<  functionNames[f_transformreduce] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                tmp = bolt::cl::transform_reduce( ctrl,input1.begin( ), input1.end( ),
                    unaryFunct, tmp,
                    binaryFunct);
#else
                tmp = thrust::transform_reduce( input1.begin( ), input1.end( ),
                    unaryFunct, tmp,
                    binaryFunct);
#endif
                myTimer.Stop( testId );
            }
        }
        break;
    case f_stablesort:
        {
            std::cout <<  functionNames[f_stablesort] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::stable_sort(ctrl, input1.begin(), input1.end(),binaryPredLt); 
#else
                thrust::stable_sort( input1.begin(), input1.end(),binaryPredLt); 
#endif
                myTimer.Stop( testId );
            }
        }
        break;
    case f_stablesortbykey:
        {
            std::cout <<  functionNames[f_stablesortbykey] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::stable_sort_by_key(ctrl, input1.begin(), input1.end(),input2.begin(),binaryPredLt); 
#else
                thrust::stable_sort_by_key( input1.begin(), input1.end(),input2.begin(),binaryPredLt); 
#endif
                myTimer.Stop( testId );
            }
        }
        break;
    case f_reducebykey: 
        {
            std::cout <<  functionNames[f_reducebykey] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::reduce_by_key(ctrl, input1.begin(), input1.end(),input2.begin(),input3.begin(),
                    output.begin(),binaryPredEq, binaryFunct);
#else
                thrust::reduce_by_key( input1.begin(), input1.end(),input2.begin(),input3.begin(),
                    output.begin(),binaryPredEq, binaryFunct);
#endif
                myTimer.Stop( testId );
            }
        }
        break;

    case f_sort: 
        {
            std::cout <<  functionNames[f_sort] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                VectorType inputBackup = input1;
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::sort(ctrl, inputBackup.begin(), inputBackup.end(),binaryPredLt);                    
#else
                thrust::sort( inputBackup.begin(), inputBackup.end(),binaryPredLt); 
                //thrust::sort( inputBackup.begin(), inputBackup.end());                    
#endif
                myTimer.Stop( testId );
            }
        }
        break;      
    case f_sortbykey: 
        {
            std::cout <<  functionNames[f_sortbykey] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::sort_by_key(ctrl, input1.begin(), input1.end(), input2.begin( ),binaryPredLt );
#else
                thrust::sort_by_key( input1.begin(), input1.end(), input2.begin( ),binaryPredLt );
#endif
                myTimer.Stop( testId ); 
            }
        }
        break;
    case f_reduce:
        {
            typename VectorType::value_type tmp;
            tmp=0;
            std::cout <<  functionNames[f_reduce] << std::endl;

            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                tmp = bolt::cl::reduce(ctrl, input1.begin(), input1.end(),tmp,binaryFunct);                    
#else
                tmp = thrust::reduce( input1.begin(), input1.end(),tmp,binaryFunct);                    
#endif
                myTimer.Stop( testId );
            }
        }
        break;
    case f_maxelement: 
        {
            std::cout <<  functionNames[f_maxelement] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                typename VectorType::iterator itr = bolt::cl::max_element(ctrl, input1.begin(), input1.end(),binaryPredLt);                    
#else
                typename VectorType::iterator itr = thrust::max_element( input1.begin(), input1.end(),binaryPredLt);                    
#endif
                myTimer.Stop( testId ); 
            }
        }
        break;
    case f_minelement: 
        {
            std::cout <<  functionNames[f_minelement] << std::endl;
            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                typename VectorType::iterator itr = bolt::cl::min_element(ctrl, input1.begin(), input1.end(),binaryPredLt);                    
#else
                typename VectorType::iterator itr = thrust::min_element( input1.begin(), input1.end(),binaryPredLt);                    
#endif
                myTimer.Stop( testId ); 
            }
        }
        break;

    case f_fill: 
        {
            typename VectorType::value_type tmp;
            std::cout <<  functionNames[f_fill] << std::endl;

            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::fill(ctrl, input1.begin(), input1.end(),tmp);
#else
                thrust::fill( input1.begin(), input1.end(),tmp);
#endif
                myTimer.Stop( testId );
            }
        }
        break;
    case f_count: 
        {
            typename VectorType::value_type tmp;
            std::cout <<  functionNames[f_count] << std::endl;

            for (size_t iter = 0; iter < iterations+1; iter++)
            {
                myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
                bolt::cl::count(ctrl, input1.begin(), input1.end(),tmp);
#else
                thrust::count( input1.begin(), input1.end(),tmp);
#endif
                myTimer.Stop( testId );
            }
        }
        break;

    case f_generate: 
        std::cout <<  functionNames[f_generate] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::generate(ctrl, input1.begin(), input1.end(), generator );
#else
            thrust::generate( input1.begin(), input1.end(), generator );
#endif
            myTimer.Stop( testId ); 
        }
        break;
    case f_copy:
        std::cout <<  functionNames[f_copy] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::copy(ctrl, input1.begin(), input1.end(), output.begin() );
#else
            thrust::copy( input1.begin(), input1.end(), output.begin() );
#endif
            myTimer.Stop( testId ); 
        }
        break;
    case f_unarytransform:
        std::cout <<  functionNames[f_unarytransform] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::transform(ctrl, input1.begin(), input1.end(), output.begin(), unaryFunct );
#else
            thrust::transform( input1.begin(), input1.end(), output.begin(), unaryFunct );
#endif
            myTimer.Stop( testId ); 
        }
        break;
    case f_binarytransform: 
        std::cout <<  functionNames[f_binarytransform] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::transform(ctrl, input1.begin(), input1.end(), input2.begin(), output.begin(), binaryFunct );
#else
            thrust::transform( input1.begin(), input1.end(), input2.begin(), output.begin(), binaryFunct );
#endif
            myTimer.Stop( testId ); 
        }
        break;
    case f_scan: 
        std::cout <<  functionNames[f_scan] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::inclusive_scan(ctrl, input1.begin(), input1.end(), output.begin(), binaryFunct );
#else
             thrust::inclusive_scan( input1.begin(), input1.end(), output.begin(), binaryFunct );
             //thrust::inclusive_scan( input1.begin(), input1.end(), output.begin() );
#endif
            myTimer.Stop( testId ); 
        }
        break;
    case f_transformscan: 
        std::cout <<  functionNames[f_transformscan] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::transform_inclusive_scan(ctrl, input1.begin(), input1.end(), output.begin(),
                unaryFunct, binaryFunct );
#else
            thrust::transform_inclusive_scan( input1.begin(), input1.end(), output.begin(), 
                unaryFunct, binaryFunct );
#endif
            myTimer.Stop( testId );
        }
        break;
    case f_scanbykey: 
        std::cout <<  functionNames[f_scanbykey] << std::endl;
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
            myTimer.Start( testId );
#if (Bolt_Benchmark == 1)
            bolt::cl::inclusive_scan_by_key(ctrl, input1.begin(), input1.end(), input2.begin(), 
                output.begin(), binaryPredEq, binaryFunct );
#else
            thrust::inclusive_scan_by_key( input1.begin(), input1.end(), input2.begin(),
                output.begin(), binaryPredEq, binaryFunct );
#endif
            myTimer.Stop( testId );
        }
        break;
        
        case f_gather: 
        {           
        std::cout <<  functionNames[f_gather] << std::endl; 
        myTimer.Start( testId );
        #if (Bolt_Benchmark == 1)
        bolt::cl::device_vector<unsigned int> Map(input1.size());
        #else
        thrust::device_vector<DATA_TYPE> Map(input1.size());
        #endif
        myTimer.Stop( testId );
        for( int i=0; i < input1.size() ; i++ )
        {
        Map[i] = i;
        }
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
        myTimer.Start( testId );
        #if (Bolt_Benchmark == 1)
        bolt::cl::gather( ctrl,Map.begin( ), Map.end( ),input1.begin( ),output.begin()); 
        #else
        thrust::gather( Map.begin( ), Map.end( ),input1.begin( ),output.begin()); 
        #endif
        myTimer.Stop( testId );
        }
        } 
        break;
        case f_scatter: 
        {            
        std::cout <<  functionNames[f_scatter] << std::endl;
        bolt::cl::device_vector<unsigned int> Map(input1.size());
        for( int i=0; i < input1.size() ; i++ )
        {
        Map[i] = i;
        }
        for (size_t iter = 0; iter < iterations+1; iter++)
        {
        bolt::cl::scatter( ctrl, input1.begin( ),input1.end( ), Map.begin(), output.begin()); 
        }
        } 
        break;
        
    default:
        std::cout << "\nUnsupported function = " << function <<"\n"<< std::endl;
        break;
    } // switch

    size_t length = input1.size();
    double MKeys = length / ( 1024.0 * 1024.0 );
    //#if (Bolt_Benchmark == 1)
    size_t pruned = myTimer.pruneOutliers( 1.0 );
    double sortTime = myTimer.getAverageTime( testId );
    //#else    
    //    double sortTime = (timer.TimeInMs()/1000)/(iterations+1);
    //#endif
    double testMB = MKeys*siz;
    double testGB = testMB/ 1024.0;
    bolt::tout << std::left;
    //#if (Bolt_Benchmark == 1)
    bolt::tout << std::setw( colWidth ) << _T( "Test profile: " ) << _T( "[" ) << iterations-pruned << _T( "] samples" ) << std::endl;
    //#else
    //    bolt::tout << std::setw( colWidth ) << _T( "Test profile: " ) << _T( "[" ) << iterations << _T( "] samples" ) << std::endl;
    //#endif
    bolt::tout << std::setw( colWidth ) << _T( "    Size (MKeys): " ) << MKeys << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Size (GB): " ) << testGB << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Time (s): " ) << sortTime << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Speed (GB/s): " ) << testGB / sortTime << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Speed (MKeys/s): " ) << MKeys / sortTime << std::endl;
    bolt::tout << std::endl;

}

/******************************************************************************
 *
 *  Determine types
 *
 *****************************************************************************/
#if (Bolt_Benchmark == 1)
void executeFunction(
    bolt::cl::control& ctrl,
    size_t vecType,
    bool hostMemory,
    size_t length,
    size_t routine,
    size_t iterations )
#else
void executeFunction(
    size_t vecType,
    bool hostMemory,
    size_t length,
    size_t routine,
    size_t iterations )
#endif
{
    size_t siz;
    if (vecType == t_int)
    {
#if (Bolt_Benchmark == 1)
        intgen                        generator;       
        bolt::cl::square<DATA_TYPE>   unaryFunct;
        bolt::cl::plus<DATA_TYPE>     binaryFunct;
        bolt::cl::equal_to<DATA_TYPE> binaryPredEq;
        bolt::cl::less<DATA_TYPE>     binaryPredLt;
#else
        intgen     generator; 
        vec1square unaryFunct;
        vec1plus   binaryFunct;
        vec1equal  binaryPredEq;;
        vec1less   binaryPredLt;
#endif  
        siz = sizeof(DATA_TYPE);
        
        std::vector<DATA_TYPE> input1(length);
        std::vector<DATA_TYPE> input2(length);
        std::vector<DATA_TYPE> input3(length);
        std::vector<DATA_TYPE> output(length);
        std::vector<DATA_TYPE> output_merge(length*2) ;
        std::generate(input1.begin(), input1.end(), RandomNumber);
        std::generate(input2.begin(), input2.end(), RandomNumber);
        std::generate(input3.begin(), input3.end(), RandomNumber);
        std::generate(output.begin(), output.end(), RandomNumber);
        std::generate(output_merge.begin(), output_merge.end(), RandomNumber);
        if (hostMemory) {
#if (Bolt_Benchmark == 1)
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge, 
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
#else
            executeFunctionType( input1, input2, input3, output, output_merge, 
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
#endif
        }
        else
        {
#if (Bolt_Benchmark == 1)
            bolt::cl::device_vector<DATA_TYPE> binput1(input1.begin(), input1.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<DATA_TYPE> binput2(input2.begin(), input2.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<DATA_TYPE> binput3(input3.begin(), input3.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<DATA_TYPE> boutput(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl); 
            bolt::cl::device_vector<DATA_TYPE> boutput_merge(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl); 
   
            executeFunctionType( ctrl, binput1, binput2, binput3, boutput, boutput_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
#else
            thrust::device_vector<DATA_TYPE> binput1(input1.begin(), input1.end());
            thrust::device_vector<DATA_TYPE> binput2(input2.begin(), input2.end());
            thrust::device_vector<DATA_TYPE> binput3(input3.begin(), input3.end());
            thrust::device_vector<DATA_TYPE> boutput(output.begin(), output.end()); 
            thrust::device_vector<DATA_TYPE> boutput_merge(output.begin(), output.end()); 
   
            executeFunctionType( binput1, binput2, binput3, boutput, boutput_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
#endif
        }
    }
#if 1
    else if (vecType == t_vec2)
    {
        vec2gen     generator;
        vec2square  unaryFunct;
        vec2plus    binaryFunct;
        vec2equal   binaryPredEq;
        vec2less    binaryPredLt;
        siz = sizeof(vec2);
        
    #if (Bolt_Benchmark == 1)
        BOLT_ADD_DEPENDENCY(vec2, Bolt_DATA_TYPE);
        #endif
        std::vector<vec2> input1(length);
        std::vector<vec2> input2(length);
        std::vector<vec2> input3(length);
        std::vector<vec2> output(length);
        std::vector<vec2> output_merge(length*2) ;

        std::generate(input1.begin(), input1.end(),RandomNumber);
        std::generate(input2.begin(), input2.end(),RandomNumber);
        std::generate(input3.begin(), input3.end(),RandomNumber);
        std::generate(output.begin(), output.end(),RandomNumber);
        std::generate(output_merge.begin(), output_merge.end(), RandomNumber);

        if (hostMemory) {
    #if (Bolt_Benchmark == 1)
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            executeFunctionType( input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #endif
        }
        else
        {
    #if (Bolt_Benchmark == 1)
            bolt::cl::device_vector<vec2> binput1(input1.begin(), input1.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec2> binput2(input2.begin(), input2.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec2> binput3(input3.begin(), input3.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec2> boutput(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec2> boutput_merge(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl); 
            executeFunctionType( ctrl, binput1, binput2,binput3, boutput, boutput_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            thrust::device_vector<vec2> binput1(input1.begin(), input1.end() );
            thrust::device_vector<vec2> binput2(input2.begin(), input2.end() );
            thrust::device_vector<vec2> binput3(input3.begin(), input3.end() );
            thrust::device_vector<vec2> boutput(output.begin(), output.end() );
            thrust::device_vector<vec2> boutput_merge(output.begin(), output.end() ); 
            executeFunctionType( binput1, binput2,binput3, boutput, boutput_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #endif
        }
    }
    else if (vecType == t_vec4)
    {
        siz = sizeof(vec4);
        vec4gen     generator;
        vec4square  unaryFunct;
        vec4plus    binaryFunct;
        vec4equal   binaryPredEq;
        vec4less    binaryPredLt;
        std::vector<vec4> input1(length, v4init);
        std::vector<vec4> input2(length, v4init);
        std::vector<vec4> input3(length, v4init);
        std::vector<vec4> output(length, v4iden);
        std::vector<vec4> output_merge(length * 2, v4iden);
    #if (Bolt_Benchmark == 1)
        BOLT_ADD_DEPENDENCY(vec4, Bolt_DATA_TYPE);
    #endif
        std::generate(input1.begin(), input1.end(),RandomNumber);
        std::generate(input2.begin(), input2.end(),RandomNumber);
        std::generate(input3.begin(), input3.end(),RandomNumber);
        std::generate(output.begin(), output.end(),RandomNumber);
        std::generate(output_merge.begin(), output_merge.end(), RandomNumber);
        if (hostMemory) {
    #if (Bolt_Benchmark == 1)
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            executeFunctionType( input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #endif
        }
        else
        {
    #if (Bolt_Benchmark == 1)
            bolt::cl::device_vector<vec4> binput1(input1.begin(), input1.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec4> binput2(input2.begin(), input2.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec4> binput3(input3.begin(), input3.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec4> boutput(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec4> boutput_merge(output_merge.begin(), output_merge.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            thrust::device_vector<vec4> binput1(input1.begin(), input1.end() );
            thrust::device_vector<vec4> binput2(input2.begin(), input2.end() );
            thrust::device_vector<vec4> binput3(input3.begin(), input3.end() );
            thrust::device_vector<vec4> boutput(output.begin(), output.end() );
            thrust::device_vector<vec4> boutput_merge(output_merge.begin(), output_merge.end() );
            executeFunctionType( input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #endif
        }
    }
    else if (vecType == t_vec8)
    {
        vec8gen     generator;
        vec8square  unaryFunct;
        vec8plus    binaryFunct;
        vec8equal   binaryPredEq;
        vec8less    binaryPredLt;
        siz = sizeof(vec8);
        std::vector<vec8> input1(length, v8init);
        std::vector<vec8> input2(length, v8init);
        std::vector<vec8> input3(length, v8init);
        std::vector<vec8> output(length, v8iden);
        std::vector<vec8> output_merge(length*2, v8iden);
    #if (Bolt_Benchmark == 1)
        BOLT_ADD_DEPENDENCY(vec8, Bolt_DATA_TYPE);
    #endif
        std::generate(input1.begin(), input1.end(),RandomNumber);
        std::generate(input2.begin(), input2.end(),RandomNumber);
        std::generate(input3.begin(), input3.end(),RandomNumber);
        std::generate(output.begin(), output.end(),RandomNumber);
        std::generate(output_merge.begin(), output_merge.end(),RandomNumber);
        if (hostMemory) {
    #if (Bolt_Benchmark == 1)
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            executeFunctionType( input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
        #endif
        }
        else
        {
    #if (Bolt_Benchmark == 1)
            bolt::cl::device_vector<vec8> binput1(input1.begin(), input1.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec8> binput2(input2.begin(), input2.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec8> binput3(input3.begin(), input3.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec8> boutput(output.begin(), output.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            bolt::cl::device_vector<vec8> boutput_merge(output_merge.begin(), output_merge.end(), BOLT_BENCH_DEVICE_VECTOR_FLAGS,  ctrl);
            executeFunctionType( ctrl, input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #else
            thrust::device_vector<vec8> binput1(input1.begin(), input1.end() );
            thrust::device_vector<vec8> binput2(input2.begin(), input2.end() );
            thrust::device_vector<vec8> binput3(input3.begin(), input3.end() );
            thrust::device_vector<vec8> boutput(output.begin(), output.end() );
            thrust::device_vector<vec8> boutput_merge(output_merge.begin(), output_merge.end() );
            executeFunctionType( input1, input2, input3, output, output_merge,
                generator, unaryFunct, binaryFunct, binaryPredEq, binaryPredLt, routine, iterations,siz);
    #endif
        }
    }
#endif
    else
    {
        std::cerr << "Unsupported vecType=" << vecType << std::endl;
    }

}

/******************************************************************************
 *
 *  Main
 *
 *****************************************************************************/
int _tmain( int argc, _TCHAR* argv[] )
{
    cl_int err = CL_SUCCESS;
    cl_uint userPlatform    = 0;
    cl_uint userDevice      = 0;
    size_t iterations       = 100;
    size_t length           = 1024;
    size_t vecType          = 0;
    size_t runMode          = 0;
    size_t routine          = f_binarytransform;
    size_t numThrowAway     = 0;
    std::string function_called=functionNames[routine] ;
    std::string filename    = "bench.xml";
    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;
    bool defaultDevice      = true;
    bool print_clInfo       = false;
    bool hostMemory         = true;
    /******************************************************************************
     * Parse Command-line Parameters
     ******************************************************************************/
    try
    {
        // Declare the supported options.
        po::options_description desc( "OpenCL Scan command line options" );
        desc.add_options()
            ( "help,h",			"produces this help message" )
        #if (Bolt_Benchmark == 1)
            ( "version,v",		"Print queryable version information from the Bolt CL library" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "gpu,g",          "Report only OpenCL GPU devices" )
            ( "cpu,c",          "Report only OpenCL CPU devices" )
            ( "all,a",          "Report all OpenCL devices" )
        #endif
            ( "queryCuda,q",    "Print queryable platform and device info and return" )
            
            ( "deviceMemory,D",   "Allocate vectors in device memory; default is host memory" )
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( userPlatform ),
                "Specify the platform under test using the index reported by -q flag" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( userDevice ),
                "Specify the device under test using the index reported by the -q flag.  "
                "Index is relative with respect to -g, -c or -a flags" )
            ( "length,l",       po::value< size_t >( &length )->default_value( length ),
                "Length of scan array" )
            ( "iterations,i",   po::value< size_t >( &iterations )->default_value( iterations ),
                "Number of samples in timing loop" )
            ( "vecType,t",      po::value< size_t >( &vecType )->default_value( vecType ),
                "Data Type to use: 0-(1 value), 1-(2 values), 2-(4 values), 3-(8 values)" )
            ( "runMode,m",      po::value< size_t >( &runMode )->default_value( runMode ),
                "Run Mode: 0-Auto, 1-SerialCPU, 2-MultiCoreCPU, 3-GPU" )
            ( "function,f",      po::value< std::string >( &function_called )->default_value( function_called ),
                "Number of samples in timing loop" )
            ( "filename",     po::value< std::string >( &filename )->default_value( filename ),
                "Name of output file" )
            ( "throw-away",   po::value< size_t >( &numThrowAway )->default_value( numThrowAway ),
                "Number of trials to skip averaging" )
            ;

        po::variables_map vm;
        if(argc <= 1)
        {
            std::cout << desc << std::endl;
            return 0;
        }
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );
        routine = get_functionindex(function_called);

    #if (Bolt_Benchmark == 1)
        if( vm.count( "version" ) )
        {
            cl_uint libMajor, libMinor, libPatch;
            bolt::cl::getVersion( libMajor, libMinor, libPatch );
            const int indent = countOf( "Bolt version: " );
            bolt::tout << std::left << std::setw( indent ) << _T( "Bolt version: " )
                << libMajor << _T( "." )
                << libMinor << _T( "." )
                << libPatch << std::endl;
        }
        if( vm.count( "queryOpenCL" ) )
        {
            print_clInfo = true;
        }
        if( vm.count( "gpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_GPU;
        }
        
        if( vm.count( "cpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_CPU;
        }
        if( vm.count( "all" ) )
        {
            deviceType	= CL_DEVICE_TYPE_ALL;
        }
      #endif
        if( vm.count( "help" ) )
        {
            //	This needs to be 'cout' as program-options does not support wcout yet
            std::cout << desc << std::endl;
            return 0;
        }     
        
        if( vm.count( "deviceMemory" ) )
        {
            hostMemory = false;
        }
    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }
    /******************************************************************************
     * Initialize platforms and devices
     ******************************************************************************/
#if defined(_WIN32)
    aProfiler.throwAway( numThrowAway );
#endif
#if (Bolt_Benchmark == 1)
    bolt::cl::control ctrl = bolt::cl::control::getDefault();
    
    std::string strDeviceName;
    if (runMode == 1) // serial cpu
    {
        ctrl.setForceRunMode( bolt::cl::control::SerialCpu );
        strDeviceName = "Serial CPU";
    }
    else if (runMode == 2) // multicore cpu
    {
        ctrl.setForceRunMode( bolt::cl::control::MultiCoreCpu );
        strDeviceName = "MultiCore CPU";
    }
    else // gpu || automatic
    {
        // Platform vector contains all available platforms on system
        std::vector< ::cl::Platform > platforms;
        bolt::cl::V_OPENCL( ::cl::Platform::get( &platforms ), "Platform::get() failed" );
        if( print_clInfo )
        {
            bolt::cl::control::printPlatforms(true,deviceType);
           //std::for_each( platforms.begin( ), platforms.end( ), printPlatformFunctor( 0 ) );
            return 0;
        }
        // Device info
        ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
        std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
        //::cl::CommandQueue myQueue( myContext, devices.at( userDevice ) , CL_QUEUE_PROFILING_ENABLE);
        ::cl::CommandQueue myQueue( myContext, devices.at( userDevice ));
        //  Now that the device we want is selected and we have created our own cl::CommandQueue, set it as the
        //  default cl::CommandQueue for the Bolt API
        ctrl.setCommandQueue( myQueue );
        strDeviceName = ctrl.getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
        bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );
    }
#endif
    //std::cout << "Device: " << strDeviceName << std::endl;
    /******************************************************************************
     * Select then Execute Function
     ******************************************************************************/
#if (Bolt_Benchmark == 1)
        executeFunction(
        ctrl,
        vecType,
        hostMemory,
        length,
        routine,
        iterations + numThrowAway
        );
#else
    executeFunction(
        vecType,
        hostMemory,
        length,
        routine,
        iterations + numThrowAway
        );
#endif
    /******************************************************************************
     * Print Results
     ******************************************************************************/
#if defined(_WIN32)
    aProfiler.end();
#endif
    std::ofstream outFile( filename.c_str() );
#if defined(_WIN32)
    aProfiler.writeSum( outFile );
#endif
    outFile.close();
    return 0;
}
