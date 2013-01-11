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

/******************************************************************************
 * OpenCL Scan
 *****************************************************************************/

#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
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
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, true, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
OutputIterator inclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    BinaryFunction binary_op, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init; memset(&init, 0, sizeof(iType) );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, true, binary_op,
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
    const control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result, 
    const std::string& user_code ) // assumes addition of numbers
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    iType init = static_cast< iType >( 0 );
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T >
OutputIterator exclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init, 
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, plus< iType >( ),
        std::iterator_traits< InputIterator >::iterator_category( ) );
};

template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction > 
OutputIterator exclusive_scan(
    control &ctl,
    InputIterator first,
    InputIterator last,
    OutputIterator result,
    T init,
    BinaryFunction binary_op,
    const std::string& user_code )
{
    return detail::scan_detect_random_access(
        ctl, first, last, result, init, false, binary_op,
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
        addKernelName("perBlockInclusiveScan");
        addKernelName("intraBlockInclusiveScan");
        addKernelName("perBlockAddition");
    }
    
    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
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

        return templateSpecializationString;
    }
};


template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
OutputIterator scan_detect_random_access(
    control &ctl,
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
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::random_access_iterator_tag )
{
    return detail::scan_pick_iterator( ctl, first, last, result, init, inclusive, binary_op );
};

/*! 
* \brief This overload is called strictly for non-device_vector iterators
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
    control &ctl,
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

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.
    if( runMode == bolt::cl::control::SerialCpu )
    {
        std::partial_sum( first, last, result, binary_op );
        return result;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        std::cout << "The MultiCoreCpu version of inclusive_scan is not implemented yet." << std ::endl;
    }
    else
    {

        // Map the input iterator to a device_vector
        device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        device_vector< oType > dvOutput( result, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

        //Now call the actual cl algorithm
        scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), init, binary_op, inclusive );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    return result + numElements;
}

/*! 
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
typename std::enable_if< 
    (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
     std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
DVOutputIterator >::type
scan_pick_iterator(
    control &ctl,
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

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.
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
    scan_enqueue( ctl, first, last, result, init, binary_op, inclusive );

    return result + numElements;
}


//  All calls to inclusive_scan end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
void scan_enqueue(
    control &ctl,
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

size_t k0_stepNum, k1_stepNum, k2_stepNum;
#endif
    cl_int l_Error = CL_SUCCESS;


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
     * Type Definitions - directly concatenated into kernel string
     *********************************************************************************/
    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction  >::get() )


    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    bool cpuDevice = ctl.device().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
    const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    const size_t kernel1_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL1WAVES;
    const size_t kernel2_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    std::string compileOptions;
    std::ostringstream oss;
    oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;
    oss << " -DKERNEL1WORKGROUPSIZE=" << kernel1_WgSize;
    oss << " -DKERNEL2WORKGROUPSIZE=" << kernel2_WgSize;
    compileOptions = oss.str();

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    Scan_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        scan_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor


    cl_uint doExclusiveScan = inclusive ? 0 : 1;    
    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent;

    // Set up shape of launch grid and buffers:
    int computeUnits     = ctl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    int wgPerComputeUnit =  ctl.wgPerComputeUnit( );
    int resultCnt = computeUnits * wgPerComputeUnit;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );

    device_vector< iType >::size_type sizeInputBuff = numElements;
    size_t modWgSize = (sizeInputBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += kernel0_WgSize;
    }

    cl_uint numWorkGroupsK0 = static_cast< cl_uint >( sizeInputBuff / kernel0_WgSize );

    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    device_vector< oType >::size_type sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += kernel0_WgSize;
    }

    // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
    ALIGNED( 256 ) BinaryFunction aligned_binary( binary_op );
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_binary ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );
    control::buffPointer preSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    //::cl::Buffer userFunctor( ctl.context( ), CL_MEM_USE_HOST_PTR, sizeof( binary_op ), &binary_op );
    //::cl::Buffer preSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );
    //::cl::Buffer postSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );
    cl_uint ldsSize;


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
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
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(iType) + 1*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 0 ],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
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
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*sizeScanBuff);
aProfiler.set(AsyncProfiler::memory, 4*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
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
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(oType) + 1*sizeScanBuff*sizeof(oType));
#endif
    try
    {
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[ 2 ],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
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
std::string strDeviceName = ctl.device().getInfo< CL_DEVICE_NAME >( &l_Error );
bolt::cl::V_OPENCL( l_Error, "Device::getInfo< CL_DEVICE_NAME > failed" );
aProfiler.setArchitecture(strDeviceName);

    try
    {
        cl_ulong k0_start, k0_stop, k1_stop, k2_stop;
        
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        //l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
        //V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        //l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
        //V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
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

#endif

}   //end of inclusive_scan_enqueue( )

}   //namespace detail
}   //namespace cl
}//namespace bolt

#endif // OCL_SCAN_INL