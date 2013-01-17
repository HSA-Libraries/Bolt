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

//#ifndef USE_AMD_HSA
#define USE_AMD_HSA 0
//#endif

#if USE_AMD_HSA
#define HSA_STAT_INIT 0 // device hasn't done pre-scan
#define HSA_STAT_DEVP1COMPLETE 1 // device has done pre-scan
#define HSA_STAT_CPUP2COMPLETE 2 // cpu has done intermediate scan
#define HSA_STAT_DEVP3COMPLETE 3 // gpu has done post scan
#endif

/******************************************************************************
 * OpenCL Scan
 *****************************************************************************/

#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define HSAWAVES 4
#define WAVESIZE 64

#if !defined( OCL_SCAN_INL )
#define OCL_SCAN_INL
#pragma once

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform_scan");
#endif

#include <algorithm>
#include <type_traits>
#include "bolt/cl/bolt.h"

namespace bolt
{
namespace cl
{
//////////////////////////////////////////
//  Inclusive scan overloads
//////////////////////////////////////////
template< typename InputIterator, typename OutputIterator >
OutputIterator inclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, true, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
OutputIterator inclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    BinaryFunction binary_op, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, true, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator >
OutputIterator inclusive_scan(
    control &ctrl,
    InputIterator first,
    InputIterator last,
    OutputIterator result, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctrl, first, last, result, init, true, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
OutputIterator inclusive_scan(
    control &ctrl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    BinaryFunction binary_op, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctrl, first, last, result, init, true, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

//////////////////////////////////////////
//  Exclusive scan overloads
//////////////////////////////////////////
template< typename InputIterator, typename OutputIterator >
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T >
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction > 
OutputIterator exclusive_scan(
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    BinaryFunction binary_op,
    const std::string& user_code )
{
    return detail::scan_detect_random_access(
        control::getDefault( ), first, last, result, init, false, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator >
OutputIterator exclusive_scan(
    const control &ctrl,
    InputIterator first,
    InputIterator last,
    OutputIterator result, 
    const std::string& user_code ) // assumes addition of numbers
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init = static_cast< iType >( 0 );
    return detail::scan_detect_random_access(
        ctrl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T >
OutputIterator exclusive_scan(
    control &ctrl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    return detail::scan_detect_random_access(
        ctrl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction > 
OutputIterator exclusive_scan(
    control &ctrl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    BinaryFunction binary_op,
    const std::string& user_code )
{
    return detail::scan_detect_random_access(
        ctrl, first, last, result, init, false, binary_op,
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail
{

enum scanTypes {scan_iType, scan_oType, scan_initType, scan_BinaryFunction};

class Scan_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    Scan_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
#if USE_AMD_HSA
        addKernelName("HSA_Scan");
#else
        addKernelName("perBlockInclusiveScan");
        addKernelName("intraBlockInclusiveScan");
        addKernelName("perBlockAddition");
#endif
    }
    
    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
#if USE_AMD_HSA
        const std::string templateSpecializationString = 
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "kernel void " + name(0) + "(\n"
            "global " + typeNames[scan_oType] + " *output,\n"
            "global " + typeNames[scan_iType] + " *input,\n"
            ""        + typeNames[scan_initType] + " init,\n"
            "const uint numElements,\n"
            "const uint numIterations,\n"
            "local "  + typeNames[scan_oType] + " *lds,\n"
            "global " + typeNames[scan_BinaryFunction] + " *binaryOp,\n"
            "global " + typeNames[scan_oType] + " *intermediateScanArray,\n"
            "global int *intermediateScanStatus,\n"
            "int exclusive\n"
            ");\n\n";
#else
        const std::string templateSpecializationString = 
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "kernel void " + name(0) + "(\n"
            "global " + typeNames[scan_oType] + "* output,\n"
            "global " + typeNames[scan_iType] + "* input,\n"
            ""        + typeNames[scan_initType] + " identity,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[scan_oType] + "* lds,\n"
            "global " + typeNames[scan_BinaryFunction] + "* binaryOp,\n"
            "global " + typeNames[scan_oType] + "* scanBuffer,\n"
            "int exclusive\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))\n"
            "kernel void " + name(1) + "(\n"
            "global " + typeNames[scan_oType] + "* postSumArray,\n"
            "global " + typeNames[scan_oType] + "* preSumArray,\n"
            "" + typeNames[scan_oType]+" identity,\n"
            "const uint vecSize,\n"
            "local " + typeNames[scan_oType] + "* lds,\n"
            "const uint workPerThread,\n"
            "global " + typeNames[scan_BinaryFunction] + "* binaryOp\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "kernel void " + name(2) + "(\n"
            "global " + typeNames[scan_oType] + "* output,\n"
            "global " + typeNames[scan_oType] + "* postSumArray,\n"
            "const uint vecSize,\n"
            "global " + typeNames[scan_BinaryFunction] + "* binaryOp\n"
            ");\n\n";
#endif
        return templateSpecializationString;
    }
};


template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator scan_detect_random_access(
    control &ctrl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    BinaryFunction binary_op,
    std::input_iterator_tag )
{
    //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator scan_detect_random_access(
    control &ctrl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::random_access_iterator_tag )
{
    return detail::scan_pick_iterator( ctrl, first, last, result, init, inclusive, binary_op );
};

/*! 
* \brief This overload is called strictrly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
typename std::enable_if< 
    !(std::is_base_of<typename device_vector<typename
           std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
      std::is_base_of<typename device_vector<typename
           std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
OutputIterator >::type
scan_pick_iterator(
    control &ctrl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op )
{
    typedef typename std::iterator_traits< InputIterator >::value_type iType;
    typedef typename std::iterator_traits< OutputIterator >::value_type oType;
    static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements == 0 )
        return result;

    const bolt::cl::control::e_RunMode runMode = ctrl.forceRunMode( );  // could be dynamic choice some day.
    if( runMode == bolt::cl::control::SerialCpu )
    {
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("scan");
aProfiler.startTrial();
aProfiler.setStepName("serial");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

        std::partial_sum( first, last, result, binary_op );

#ifdef BOLT_ENABLE_PROFILING
aProfiler.setDataSize(numElements*sizeof(iType));
aProfiler.stopTrial();
#endif

        return result;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        std::cout << "The MultiCoreCpu version of inclusive_scan is not implemented yet." << std ::endl;
    }
    else
    {

        // Map the input iterator to a device_vector
        device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctrl );
        device_vector< oType > dvOutput( result, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctrl );

        //Now call the actual cl algorithm
        scan_enqueue( ctrl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), init, binary_op, inclusive );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    return result + numElements;
}

/*! 
* \brief This overload is called strictrly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
typename std::enable_if< 
    (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
     std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
DVOutputIterator >::type
scan_pick_iterator(
    control &ctrl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op )
{
    typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements == 0 )
        return result;

    const bolt::cl::control::e_RunMode runMode = ctrl.forceRunMode( );  // could be dynamic choice some day.
    if( runMode == bolt::cl::control::SerialCpu )
    {
        //  TODO:  Need access to the device_vector .data method to get a host pointer
        throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
        return result;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        //  TODO:  Need access to the device_vector .data method to get a host pointer
        throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
        return result;
    }

    //Now call the actual cl algorithm
    scan_enqueue( ctrl, first, last, result, init, binary_op, inclusive );

    return result + numElements;
}


//  All calls to inclusive_scan end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
void scan_enqueue(
    control &ctrl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const T& init_T,
    const BinaryFunction& binary_op,
    const bool& inclusive = true )
{
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("scan");
aProfiler.startTrial();
aProfiler.setStepName("Setup");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif
    cl_int l_Error = CL_SUCCESS;
    cl_uint doExclusiveScan = inclusive ? 0 : 1; 
    const size_t numComputeUnits = ctrl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    const size_t numWorkGroupsPerComputeUnit = ctrl.wgPerComputeUnit( );
    const size_t workGroupSize = HSAWAVES*WAVESIZE;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef std::iterator_traits< DVInputIterator >::value_type iType;
    typedef std::iterator_traits< DVOutputIterator >::value_type oType;
    std::vector<std::string> typeNames(4);
    typeNames[scan_iType] = TypeName< iType >::get( );
    typeNames[scan_oType] = TypeName< oType >::get( );
    typeNames[scan_initType] = TypeName< T >::get( );
    typeNames[scan_BinaryFunction] = TypeName< BinaryFunction >::get();

    /**********************************************************************************
     * Type Definitions - directrly concatenated into kernel string
     *********************************************************************************/
    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction  >::get() )


    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    bool cpuDevice = ctrl.device().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
    const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    const size_t kernel1_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL1WAVES;
    const size_t kernel2_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    std::string compileOptions;
    std::ostringstream oss;
    oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;
    oss << " -DKERNEL1WORKGROUPSIZE=" << kernel1_WgSize;
    oss << " -DKERNEL2WORKGROUPSIZE=" << kernel2_WgSize;

    oss << " -DUSE_AMD_HSA=" << USE_AMD_HSA;
    compileOptions = oss.str();

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    Scan_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctrl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        scan_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor

    /**********************************************************************************
     * Round Up Number of Elements
     *********************************************************************************/
    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );

    size_t numElementsRUP = numElements;
    size_t modWgSize = (numElementsRUP & (kernel0_WgSize-1));
    if( modWgSize )
    {
        numElementsRUP &= ~modWgSize;
        numElementsRUP += kernel0_WgSize;
    }

    cl_uint numWorkGroupsK0 = static_cast< cl_uint >( numElementsRUP / kernel0_WgSize );


    // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
    ALIGNED( 256 ) BinaryFunction aligned_binary( binary_op );
    control::buffPointer userFunctor = ctrl.acquireBuffer( sizeof( aligned_binary ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );
    cl_uint ldsSize;


    /**********************************************************************************
     *
     *  HSA Implementation
     *
     *********************************************************************************/
#if USE_AMD_HSA
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup HSA Kernel");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    ::cl::Event kernel0Event;
    size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;
    if (numWorkGroupsK0 < numWorkGroups)
        numWorkGroups = numWorkGroupsK0; // nWG is lesser of elements vs compute units
    ldsSize = static_cast< cl_uint >( ( kernel0_WgSize ) * sizeof( oType ) );
    
    // allocate and initialize gpu -> cpu array
    control::buffPointer dev2hostD = ctrl.acquireBuffer( numWorkGroups*sizeof( int ),
        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR );
    ctrl.commandQueue().enqueueFillBuffer( *dev2hostD, HSA_STAT_INIT, 0, numWorkGroups*sizeof(int) );
    int *dev2hostH = (int *) ctrl.commandQueue().enqueueMapBuffer( *dev2hostD, CL_TRUE, CL_MAP_READ, 0,
        numWorkGroups*sizeof(int), NULL, NULL, &l_Error);
    V_OPENCL( l_Error, "Error: Mapping Device->Host Buffer." );

    // allocate and initialize cpu -> gpu array
    control::buffPointer dev2hostD = ctrl.acquireBuffer( numWorkGroups*sizeof( int ),
        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR );

    /*int *dev2hostH = clEnqueueMapBuffer(
        ctrl.commandQueue(),
        dev2hostD,
        CL_TRUE,
        CL_MAP_READ,
        0,
        numWorkGroups*sizeof(int),
        0, NULL, NULL, &l_Error);*/

    int *intermediateScanStatusHost = new int[ numWorkGroups ];
    memset( intermediateScanStatusHost, HSA_STAT_INIT, numWorkGroups*sizeof( int ) );
    oType *intermediateScanArrayHost = new oType[ numWorkGroups ];
    memset( intermediateScanArrayHost, init_T, numWorkGroups*sizeof( oType ) ); // TODO remove me b/c wrong and superfluous
    control::buffPointer intermediateScanArray  = ctrl.acquireBuffer( numWorkGroups*sizeof( oType ),
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, intermediateScanArrayHost );
    control::buffPointer intermediateScanStatus = ctrl.acquireBuffer( numWorkGroups*sizeof( int ),
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, intermediateScanStatusHost );
    // how many iterations
    cl_uint numIterations = static_cast< cl_uint >( numElementsRUP / (numWorkGroups*workGroupSize) );
    if (numWorkGroups*workGroupSize*numIterations < numElementsRUP) numIterations++;

    for (size_t i = 0; i < numWorkGroups; i++ )
    {
        std::cout << "preScanStat[" << i << "]=" << intermediateScanArrayHost[i] << " ( " << dev2hostH[i] << " )"<< std::endl;
    }

    /**********************************************************************************
     * Set Kernel Arguments
     *********************************************************************************/
    V_OPENCL( kernels[ 0 ].setArg( 0, result->getBuffer( ) ),   "Error: Output Buffer" );
    V_OPENCL( kernels[ 0 ].setArg( 1, first->getBuffer( ) ),    "Error: Input Buffer" );
    V_OPENCL( kernels[ 0 ].setArg( 2, init_T ),                 "Error: Initial Value" );
    V_OPENCL( kernels[ 0 ].setArg( 3, numElements ),            "Error: Number of Elements" );
    V_OPENCL( kernels[ 0 ].setArg( 4, numIterations ),          "Error: Number of Iterations" );
    V_OPENCL( kernels[ 0 ].setArg( 5, ldsSize, NULL ),          "Error: Local Memory" );
    V_OPENCL( kernels[ 0 ].setArg( 6, *userFunctor ),           "Error: Binary Function" );
    V_OPENCL( kernels[ 0 ].setArg( 7, *intermediateScanArray ), "Error: Intermediate Scan Array" );
    V_OPENCL( kernels[ 0 ].setArg( 8, *dev2hostD ),             "Error: Intermediate Scan Status" );
    V_OPENCL( kernels[ 0 ].setArg( 9, doExclusiveScan ),        "Error: Do Exclusive Scan" );

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("HSA Kernel");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory,
    1*numElements*sizeof(iType) + // read input
    3*numElements*sizeof(oType) + // write,read,write output
    1*numWorkGroups*sizeof(binary_op) + // in case the functor has state
    2*numWorkGroups*sizeof(oType)+ // write,read intermediate array
    2*numWorkGroups*sizeof(int)); // write,read intermediate array status (perhaps multiple times)
std::string strDeviceName = ctrl.device().getInfo< CL_DEVICE_NAME >( &l_Error );
bolt::cl::V_OPENCL( l_Error, "Device::getInfo< CL_DEVICE_NAME > failed" );
aProfiler.setArchitecture(strDeviceName);
#endif
    /**********************************************************************************
     * Launch Kernel
     *********************************************************************************/
    l_Error = ctrl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 0 ],
        ::cl::NullRange,
        ::cl::NDRange( numElementsRUP ),
        ::cl::NDRange( workGroupSize ),
        NULL,
        &kernel0Event);
    ctrl.commandQueue().flush(); // needed

    bool printAgain = true;
    while (printAgain)
    {
        printAgain = false;
        for (size_t i = 0; i < numWorkGroups; i++ )
        {
            std::cout << "interScan[" << i << "]=" << intermediateScanArrayHost[i] << " ( " << dev2hostH[i] << " )"<< std::endl;
            if (dev2hostH[i] != HSA_STAT_DEVP1COMPLETE)
                printAgain = true;
        }
        std::cout << std::endl;
    }

    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for HSA Kernel." );
    l_Error = kernel0Event.wait( );
    V_OPENCL( l_Error, "HSA Kernel failed to wait" );


    for (size_t i = 0; i < numWorkGroups; i++ )
    {
        std::cout << "inter2can[" << i << "]=" << intermediateScanArrayHost[i] << " ( " << dev2hostH[i] << " )"<< std::endl;
    }








#ifdef BOLT_ENABLE_PROFILING
aProfiler.stopTrial();
#endif








    /**********************************************************************************
     *
     *  Discrete GPU implementation
     *
     *********************************************************************************/
#else // regular
    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent;
    
    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    device_vector< oType >::size_type sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += kernel0_WgSize;
    }
    
    control::buffPointer preSumArray = ctrl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    control::buffPointer postSumArray = ctrl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    //::cl::Buffer userFunctor( ctrl.context( ), CL_MEM_USE_HOST_PTR, sizeof( binary_op ), &binary_op );
    //::cl::Buffer preSumArray( ctrl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );
    //::cl::Buffer postSumArray( ctrl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
    
size_t k0_stepNum, k1_stepNum, k2_stepNum;
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 0");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    ldsSize  = static_cast< cl_uint >( ( kernel0_WgSize /*+ ( kernel0_WgSize / 2 )*/ ) * sizeof( iType ) );
    V_OPENCL( kernels[ 0 ].setArg( 0, result->getBuffer( ) ),   "Error setting argument for kernels[ 0 ]" ); // Output buffer
    V_OPENCL( kernels[ 0 ].setArg( 1, first->getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[ 0 ].setArg( 2, init_T ),                 "Error setting argument for kernels[ 0 ]" ); // Initial value used for exclusive scan
    V_OPENCL( kernels[ 0 ].setArg( 3, numElements ),            "Error setting argument for kernels[ 0 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[ 0 ].setArg( 4, ldsSize, NULL ),          "Error setting argument for kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[ 0 ].setArg( 5, *userFunctor ),           "Error setting argument for kernels[ 0 ]" ); // User provided functor class
    V_OPENCL( kernels[ 0 ].setArg( 6, *preSumArray ),           "Error setting argument for kernels[ 0 ]" ); // Output per block sum buffer
    V_OPENCL( kernels[ 0 ].setArg( 7, doExclusiveScan ),        "Error setting argument for scanKernels[ 0 ]" ); // Exclusive scan?

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k0_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 0");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(iType) + 1*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctrl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 0 ],
        ::cl::NullRange,
        ::cl::NDRange( numElementsRUP ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel0Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 1");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / kernel1_WgSize );
    V_OPENCL( kernels[ 1 ].setArg( 0, *postSumArray ),  "Error setting 0th argument for kernels[ 1 ]" );          // Output buffer
    V_OPENCL( kernels[ 1 ].setArg( 1, *preSumArray ),   "Error setting 1st argument for kernels[ 1 ]" );            // Input buffer
    V_OPENCL( kernels[ 1 ].setArg( 2, init_T ),         "Error setting     argument for kernels[ 1 ]" );   // Initial value used for exclusive scan
    V_OPENCL( kernels[ 1 ].setArg( 3, numWorkGroupsK0 ),"Error setting 2nd argument for kernels[ 1 ]" );            // Size of scratch buffer
    V_OPENCL( kernels[ 1 ].setArg( 4, ldsSize, NULL ),  "Error setting 3rd argument for kernels[ 1 ]" );  // Scratch buffer
    V_OPENCL( kernels[ 1 ].setArg( 5, workPerThread ),  "Error setting 4th argument for kernels[ 1 ]" );           // User provided functor class
    V_OPENCL( kernels[ 1 ].setArg( 6, *userFunctor ),   "Error setting 5th argument for kernels[ 1 ]" );           // User provided functor class

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k1_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 1");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*sizeScanBuff);
aProfiler.set(AsyncProfiler::memory, 4*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctrl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 1 ],
        ::cl::NullRange,
        ::cl::NDRange( kernel1_WgSize ),
        ::cl::NDRange( kernel1_WgSize ),
        NULL,
        &kernel1Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );


    /**********************************************************************************
     *  Kernel 2
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 2");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif


    V_OPENCL( kernels[ 2 ].setArg( 0, result->getBuffer( ) ), "Error setting 0th argument for scanKernels[ 2 ]" );          // Output buffer
    V_OPENCL( kernels[ 2 ].setArg( 1, *postSumArray ), "Error setting 1st argument for scanKernels[ 2 ]" );            // Input buffer
    V_OPENCL( kernels[ 2 ].setArg( 2, numElements ), "Error setting 2nd argument for scanKernels[ 2 ]" );   // Size of scratch buffer
    V_OPENCL( kernels[ 2 ].setArg( 3, *userFunctor ), "Error setting 3rd argument for scanKernels[ 2 ]" );           // User provided functor class

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k2_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 2");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(oType) + 1*sizeScanBuff*sizeof(oType));
#endif
    try
    {
    l_Error = ctrl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 2 ],
        ::cl::NullRange,
        ::cl::NDRange( numElementsRUP/1 ), // remove /2 to return to 1 element per thread
        ::cl::NDRange( kernel2_WgSize ),
        NULL,
        &kernel2Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );
    }
    catch ( ::cl::Error& e )
    {
        std::cout << ( "Kernel 3 enqueueNDRangeKernel error condition reported:" ) << std::endl << e.what() << std::endl;
        return;
    }
    l_Error = kernel2Event.wait( );
    V_OPENCL( l_Error, "perBlockInclusiveScan failed to wait" );

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Querying Kernel Times");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

aProfiler.setDataSize(numElements*sizeof(iType));
std::string strDeviceName = ctrl.device().getInfo< CL_DEVICE_NAME >( &l_Error );
bolt::cl::V_OPENCL( l_Error, "Device::getInfo< CL_DEVICE_NAME > failed" );
aProfiler.setArchitecture(strDeviceName);

    try
    {
        cl_ulong k0_start, k0_stop, k1_stop, k2_stop;
        cl_ulong k1_start, k2_start;
        
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k2_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");

        size_t k0_start_cpu = aProfiler.get(k0_stepNum, AsyncProfiler::startTime);
        size_t shift = k0_start - k0_start_cpu;
        //size_t shift = k0_start_cpu - k0_start;

        //std::cout << "setting step " << k0_stepNum << " attribute " << AsyncProfiler::stopTime << " to " << k0_stop-shift << std::endl;
        aProfiler.set(k0_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k0_stop-shift) );

        aProfiler.set(k1_stepNum, AsyncProfiler::startTime, static_cast<size_t>(k0_stop-shift) );
        aProfiler.set(k1_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k1_stop-shift) );

        aProfiler.set(k2_stepNum, AsyncProfiler::startTime, static_cast<size_t>(k1_stop-shift) );
        aProfiler.set(k2_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k2_stop-shift) );

    }
    catch( ::cl::Error& e )
    {
        std::cout << ( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return;
    }

aProfiler.stopTrial();
#endif // ENABLE_PROFILING

#endif

}   //end of inclusive_scan_enqueue( )

}   //namespace detail
}   //namespace cl
}//namespace bolt

#endif // OCL_SCAN_INL