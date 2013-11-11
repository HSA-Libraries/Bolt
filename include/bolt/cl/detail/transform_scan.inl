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

#if !defined( BOLT_CL_TRANSFORM_SCAN_INL )
#define BOLT_CL_TRANSFORM_SCAN_INL
#pragma once

#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define WAVESIZE 64

//#define BOLT_ENABLE_PROFILING

#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform_scan");
#endif

#include <algorithm>
#include <type_traits>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#include "bolt/cl/transform.h"
#include "bolt/cl/bolt.h"

#ifdef ENABLE_TBB
#include "bolt/btbb/transform.h"
#include "bolt/btbb/scan.h"
#endif
namespace bolt
{
namespace cl
{
namespace detail
{
/*!
*   \internal
*   \addtogroup detail
*   \ingroup scan
*   \{
*/
    template<
    typename oType,
    typename BinaryFunction,
    typename T>
oType*
Serial_Scan(
    oType *values,
    oType *result,
    unsigned int  num,
    const BinaryFunction binary_op,
    const bool Incl,
    const T &init)
{
    oType  sum, temp;
    if(Incl){
      *result = *values; // assign value
      sum = *values;
    }
    else {
        temp = *values;
       *result = (oType)init;
       sum = binary_op( *result, temp);
    }
    for ( unsigned int i= 1; i<num; i++)
    {
        oType currentValue = *(values + i); // convertible
        if (Incl)
        {
            oType r = binary_op( sum, currentValue);
            *(result + i) = r;
            sum = r;
        }
        else // new segment
        {
            *(result + i) = sum;
            sum = binary_op( sum, currentValue);

        }
    }
    return result;
}


    enum transformScanTypes{ transformScan_iValueType, transformScan_iIterType, transformScan_oValueType,
                             transformScan_oIterType, transformScan_initType, transformScan_UnaryFunction,
                             transformScan_BinaryFunction, transformScan_end };


class TransformScan_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    TransformScan_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("perBlockTransformScan");
        addKernelName("intraBlockInclusiveScan");
        addKernelName("perBlockAddition");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[transformScan_iValueType] + "* input_ptr,\n"
            ""        + typeNames[transformScan_iIterType] + " input_iter,\n"
            ""        + typeNames[transformScan_initType] + " identity,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[transformScan_oValueType] + "* lds,\n"
            "global " + typeNames[transformScan_UnaryFunction] + "* unaryOp,\n"
            "global " + typeNames[transformScan_BinaryFunction] + "* binaryOp,\n"
            "global " + typeNames[transformScan_oValueType] + "* scanBuffer,\n"
            "global " + typeNames[transformScan_oValueType] + "* scanBuffer1,\n"
            "int exclusive\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global " + typeNames[transformScan_oValueType] + "* postSumArray,\n"
            "global " + typeNames[transformScan_oValueType] + "* preSumArray,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[transformScan_oValueType] + "* lds,\n"
            "const uint workPerThread,\n"
            "global " + typeNames[transformScan_BinaryFunction] + "* binaryOp\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(2) + "(\n"
            "global " + typeNames[transformScan_oValueType] + "* output_ptr,\n"
            ""        + typeNames[transformScan_oIterType] + " output_iter,\n"
            "global " + typeNames[transformScan_iValueType] + "* input_ptr,\n"
            ""        + typeNames[transformScan_iIterType] + " input_iter,\n"
            "global " + typeNames[transformScan_oValueType] + "* postSumArray,\n"
            "global " + typeNames[transformScan_oValueType] + "* postSumArray1,\n"
            "local "  + typeNames[transformScan_oValueType] + "* lds,\n"
            "const uint vecSize,\n"
            "global " + typeNames[transformScan_UnaryFunction] + "* unaryOp,\n"
            "global " + typeNames[transformScan_BinaryFunction] + "* binaryOp,\n"
            "int exclusive,\n"
            ""        + typeNames[transformScan_initType] + " identity\n"
            ");\n\n";

        return templateSpecializationString;
    }
};

//  All calls to transform_scan end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template<
    typename DVInputIterator,
    typename DVOutputIterator,
    typename UnaryFunction,
    typename T,
    typename BinaryFunction >
void
transform_scan_enqueue(
    control &ctl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const UnaryFunction& unary_op,
    const T& init_T,
    const BinaryFunction& binary_op,
    const bool& inclusive = true )
{
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("transform_scan");
aProfiler.startTrial();
aProfiler.setStepName("Setup");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

size_t k0_stepNum, k1_stepNum, k2_stepNum;
#endif
    cl_int l_Error;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef typename std::iterator_traits< DVInputIterator  >::value_type iType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    std::vector<std::string> typeNames( transformScan_end );
    typeNames[ transformScan_iValueType ] = TypeName< iType >::get( );
    typeNames[ transformScan_iIterType ] = TypeName< DVInputIterator >::get( );
    typeNames[ transformScan_oValueType ] = TypeName< oType >::get( );
    typeNames[ transformScan_oIterType ] = TypeName< DVOutputIterator >::get( );
    typeNames[ transformScan_initType ] = TypeName< T >::get( );
    typeNames[ transformScan_UnaryFunction ] = TypeName< UnaryFunction >::get();
    typeNames[ transformScan_BinaryFunction ] = TypeName< BinaryFunction >::get();

    /**********************************************************************************
     * Type Definitions - directly concatenated into kernel string
     *********************************************************************************/
    std::vector< std::string > typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVOutputIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< UnaryFunction >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction >::get() )

    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
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
    TransformScan_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        transform_scan_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor

    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent;
    cl_uint doExclusiveScan = inclusive ? 0 : 1;
    // Set up shape of launch grid and buffers:
    int computeUnits     = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    int wgPerComputeUnit =  ctl.getWGPerComputeUnit( );
    int resultCnt = computeUnits * wgPerComputeUnit;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );
    typename device_vector< iType >::size_type sizeInputBuff = numElements;
    size_t modWgSize = (sizeInputBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += (kernel0_WgSize*2);
    }
    cl_uint numWorkGroupsK0 = static_cast< cl_uint >( sizeInputBuff / (kernel0_WgSize*2) );


    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    typename device_vector< oType >::size_type sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & ((kernel0_WgSize*2)-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += (kernel0_WgSize*2);
    }

    // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
    ALIGNED( 256 ) UnaryFunction aligned_unary_op( unary_op );
    control::buffPointer unaryBuffer = ctl.acquireBuffer( sizeof( aligned_unary_op ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_unary_op);
    ALIGNED( 256 ) BinaryFunction aligned_binary_op( binary_op );
    control::buffPointer binaryBuffer = ctl.acquireBuffer( sizeof( aligned_binary_op ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary_op );


    control::buffPointer preSumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( iType ) );
    control::buffPointer preSumArray1 = ctl.acquireBuffer( (sizeScanBuff)*sizeof( iType ) );
    control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( iType ) );
    cl_uint ldsSize;


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
    try
    {

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 0");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    ldsSize  = static_cast< cl_uint >( (kernel0_WgSize*2) * sizeof( iType ) );

   typename DVInputIterator::Payload firs_payload = first.gpuPayload( );

    V_OPENCL( kernels[0].setArg( 0, first.getContainer().getBuffer() ),  "Error setArg kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[0].setArg( 1, first.gpuPayloadSize( ),&firs_payload  ),"Error setting a kernel argument");
    V_OPENCL( kernels[0].setArg( 2, init_T ),               "Error setArg kernels[ 0 ]" ); // Initial value exclusive
    V_OPENCL( kernels[0].setArg( 3, numElements ),          "Error setArg kernels[ 0 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[0].setArg( 4, ldsSize, NULL ),        "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[0].setArg( 5, *unaryBuffer ),         "Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 6, *binaryBuffer ),        "Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 7, *preSumArray ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[0].setArg( 8, *preSumArray1 ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[0].setArg( 9, doExclusiveScan ),     "Error setArg kernels[ 0 ]" ); // Exclusive scan?


#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k0_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 0");
aProfiler.set(AsyncProfiler::device, ctl.getForceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(iType) + 1*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[0],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff/2 ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel0Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[0]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel() in bolt::cl::transform_scan_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 1");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    ldsSize  = static_cast< cl_uint >( ( kernel0_WgSize ) * sizeof( iType ) );
    cl_int workPerThread = static_cast< cl_uint >( (sizeScanBuff) / kernel1_WgSize  );
    workPerThread = workPerThread ? workPerThread : 1;


    V_OPENCL( kernels[1].setArg( 0, *postSumArray ),        "Error setArg kernels[ 1 ]" ); // Output buffer
    V_OPENCL( kernels[1].setArg( 1, *preSumArray ),         "Error setArg kernels[ 1 ]" ); // Input buffer
    V_OPENCL( kernels[1].setArg( 2, numWorkGroupsK0 ),      "Error setArg kernels[ 1 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[1].setArg( 3, ldsSize, NULL ),        "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 4, workPerThread ),        "Error setArg kernels[ 1 ]" ); // User provided functor
    V_OPENCL( kernels[1].setArg( 5, *binaryBuffer ),        "Error setArg kernels[ 1 ]" ); // User provided functor

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k1_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 1");
aProfiler.set(AsyncProfiler::device, ctl.getForceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*sizeScanBuff);
aProfiler.set(AsyncProfiler::memory, 4*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[1],
        ::cl::NullRange,
        ::cl::NDRange( kernel1_WgSize ), // only 1 work-group
        ::cl::NDRange( kernel1_WgSize ),
        NULL,
        &kernel1Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[1]" );


    /**********************************************************************************
     *  Kernel 2
     *********************************************************************************/

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 2");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    typename DVOutputIterator::Payload result_payload = result.gpuPayload();
    typename DVInputIterator::Payload   first_payload = first.gpuPayload();

    V_OPENCL( kernels[2].setArg( 0, result.getContainer().getBuffer()),   "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 1, result.gpuPayloadSize( ),&result_payload),"Error setting a kernel argument");
    V_OPENCL( kernels[2].setArg( 2, first.getContainer().getBuffer() ),  "Error setArg kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 3, first.gpuPayloadSize( ),&first_payload  ),"Error setting a kernel argument");
    V_OPENCL( kernels[2].setArg( 4, *postSumArray ),        "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 5, *preSumArray1 ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[2].setArg( 6, ldsSize, NULL ),        "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[2].setArg( 7, numElements ),          "Error setArg kernels[ 2 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[2].setArg( 8, *unaryBuffer ),         "Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[2].setArg( 9, *binaryBuffer ),        "Error setArg kernels[ 2 ]" ); // User provided functor
    V_OPENCL( kernels[2].setArg( 10, doExclusiveScan ),     "Error setArg kernels[ 0 ]" ); // Exclusive scan?
    V_OPENCL( kernels[2].setArg( 11, init_T ),               "Error setArg kernels[ 0 ]" ); // Initial value exclusive

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k2_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 2");
aProfiler.set(AsyncProfiler::device, ctl.getForceRunMode());
aProfiler.set(AsyncProfiler::flops, numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*sizeof(oType) + 1*sizeScanBuff*sizeof(oType));
#endif

    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[2],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel2_WgSize ),
        NULL,
        &kernel2Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[2]" );

    // wait for results
    l_Error = kernel2Event.wait( );
    V_OPENCL( l_Error, "post-kernel[2] failed wait" );

    /**********************************************************************************
     *  Print Kernel times
     *********************************************************************************/

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Querying Kernel Times");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

aProfiler.setDataSize(numElements*sizeof(iType));
std::string strDeviceName = ctl.getDevice().getInfo< CL_DEVICE_NAME >( &l_Error );
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
        size_t shift = (size_t)k0_start - k0_start_cpu;
        //size_t shift = k0_start_cpu - k0_start;

        //std::cout << "setting step " << k0_stepNum << " attribute " << AsyncProfiler::stopTime << " to " ;
        //std::cout << k0_stop-shift << std::endl;
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

}   //end of transform_scan_enqueue( )

/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template<
    typename InputIterator,
    typename OutputIterator,
    typename UnaryFunction,
    typename T,
    typename BinaryFunction >
OutputIterator
transform_scan_pick_iterator(
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const UnaryFunction& unary_op,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::random_access_iterator_tag )
{
    typedef typename std::iterator_traits< InputIterator >::value_type iType;
    typedef typename std::iterator_traits< OutputIterator >::value_type oType;
    //static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements == 0 )
        return result;

    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::cl::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun();
    }
    #if defined(BOLT_DEBUG_LOG)
    BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
    #endif
	
    if( runMode == bolt::cl::control::SerialCpu )
    {
	    #if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_SERIAL_CPU,"::Transform_Scan::SERIAL_CPU");
        #endif
        std::transform(first, last, result, unary_op);
        Serial_Scan<oType, BinaryFunction, T>(&(*result), &(*result), numElements, binary_op,inclusive,init);
        return result + numElements;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        #ifdef ENABLE_TBB
		     #if defined(BOLT_DEBUG_LOG)
             dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform_Scan::MULTICORE_CPU");
             #endif
             if(inclusive)
               {
                  bolt::btbb::transform(first, last, result, unary_op);
                  return bolt::btbb::inclusive_scan(result, result+numElements, result, binary_op);
               }
               else
               {
                  bolt::btbb::transform(first, last, result, unary_op);
                  return bolt::btbb::exclusive_scan(result, result+numElements, result, init, binary_op);
               }

        #else
                throw std::runtime_error("The MultiCoreCpu version of Transform_scan is not enabled to be built! \n");
        #endif

        return result + numElements;

    }
    else
    {
	    #if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_OPENCL_GPU,"::Transform_Scan::OPENCL_GPU");
        #endif
		
        // Map the input iterator to a device_vector
        device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        device_vector< oType > dvOutput( result, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

        //Now call the actual cl algorithm
        transform_scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ),
            unary_op, init, binary_op, inclusive );

        // This should immediately map/unmap the buffer
        dvInput.data();
        dvOutput.data( );

    }
    return result + numElements;
}

/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template<
    typename DVInputIterator,
    typename DVOutputIterator,
    typename UnaryFunction,
    typename T,
    typename BinaryFunction >
DVOutputIterator
transform_scan_pick_iterator(
    control &ctl,
    const DVInputIterator& first,
    const DVInputIterator& last,
    const DVOutputIterator& result,
    const UnaryFunction& unary_op,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    bolt::cl::device_vector_tag )
{
    typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    //static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
    if( numElements < 1 )
        return result;
    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::cl::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun();
    }
    #if defined(BOLT_DEBUG_LOG)
    BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
    #endif
	
    if( runMode == bolt::cl::control::SerialCpu )
    {
	    #if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_SERIAL_CPU,"::Transform_Scan::SERIAL_CPU");
        #endif
		
        typename bolt::cl::device_vector< iType >::pointer InputBuffer =  first.getContainer( ).data( );
        typename bolt::cl::device_vector< oType >::pointer ResultBuffer =  result.getContainer( ).data( );

#if defined(_WIn32)
        std::transform(&InputBuffer[ first.m_Index ], &InputBuffer[first.m_Index] + numElements, stdext::make_checked_array_iterator(&ResultBuffer[ result.m_Index], numElements), unary_op);
#else

        std::transform(&InputBuffer[ first.m_Index ], &InputBuffer[first.m_Index] + numElements, &ResultBuffer[ result.m_Index], unary_op);
#endif
        Serial_Scan<oType, BinaryFunction, T>(&ResultBuffer[ result.m_Index  ], &ResultBuffer[ result.m_Index ], numElements, binary_op, inclusive, init);
        return result + numElements;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        #ifdef ENABLE_TBB
		     #if defined(BOLT_DEBUG_LOG)
             dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform_Scan::MULTICORE_CPU");
             #endif
			 
            typename bolt::cl::device_vector< iType >::pointer InputBuffer =  first.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer ResultBuffer =  result.getContainer( ).data( );

            if(inclusive)
               {
                 bolt::btbb::transform( &InputBuffer[ first.m_Index ], &InputBuffer[ first.m_Index ]+numElements ,  &ResultBuffer[ first.m_Index ], unary_op);
                 bolt::btbb::inclusive_scan( &ResultBuffer[ first.m_Index ],  &ResultBuffer[ first.m_Index ] + numElements, &ResultBuffer[ result.m_Index], binary_op);
               }
               else
               {
                 bolt::btbb::transform( &InputBuffer[ first.m_Index ], &InputBuffer[ first.m_Index ]+numElements ,  &ResultBuffer[ first.m_Index ], unary_op);
                 bolt::btbb::exclusive_scan(  &ResultBuffer[ first.m_Index ],  &ResultBuffer[ first.m_Index ] + numElements, &ResultBuffer[ result.m_Index], init, binary_op);
               }


        #else
                throw std::runtime_error("The MultiCoreCpu version of Transform_scan is not enabled to be built!\n");
        #endif

        return result + numElements;

    }

    else
    {
	    #if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORMSCAN,BOLTLOG::BOLT_OPENCL_GPU,"::Transform_Scan::OPENCL_GPU");
        #endif
		
        //Now call the actual cl algorithm
        transform_scan_enqueue( ctl, first, last, result, unary_op, init, binary_op, inclusive );

    }
    return result + numElements;
}

template<
    typename InputIterator,
    typename OutputIterator,
    typename UnaryFunction,
    typename T,
    typename BinaryFunction >
OutputIterator
transform_scan_detect_random_access(
    control &ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const UnaryFunction& unary_op,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::random_access_iterator_tag )
{
    return detail::transform_scan_pick_iterator( ctl, first, last, result, unary_op, init, inclusive, binary_op,
        typename std::iterator_traits< InputIterator >::iterator_category( ) );
};
template<
    typename InputIterator,
    typename OutputIterator,
    typename UnaryFunction,
    typename T,
    typename BinaryFunction >
OutputIterator
transform_scan_detect_random_access(
    control& ctl,
    const InputIterator& first,
    const InputIterator& last,
    const OutputIterator& result,
    const UnaryFunction& unary_op,
    const T& init,
    const bool& inclusive,
    const BinaryFunction& binary_op,
    std::input_iterator_tag )
{
    //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( std::is_same< InputIterator, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
};




    /*!   \}  */
} //namespace detail


    //////////////////////////////////////////
    //  Inclusive scan overloads
    //////////////////////////////////////////
    template<
        typename InputIterator,
        typename OutputIterator,
        typename UnaryFunction,
        typename BinaryFunction>
    OutputIterator
    transform_inclusive_scan(
        InputIterator first,
        InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op,
        BinaryFunction binary_op,
        const std::string& user_code )
    {
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        oType init; memset(&init, 0, sizeof(oType) );
        return detail::transform_scan_detect_random_access(
            control::getDefault( ),
            first,
            last,
            result,
            unary_op,
            init,
            true, // inclusive
            binary_op,
            typename std::iterator_traits< InputIterator >::iterator_category( ) );
    }

    template<
        typename InputIterator,
        typename OutputIterator,
        typename UnaryFunction,
        typename BinaryFunction>
    OutputIterator
    transform_inclusive_scan(
        bolt::cl::control &ctl,
        InputIterator first,
        InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op,
        BinaryFunction binary_op,
        const std::string& user_code )
    {
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        oType init; memset(&init, 0, sizeof(oType) );
        return detail::transform_scan_detect_random_access(
            ctl,
            first,
            last,
            result,
            unary_op,
            init,
            true, // inclusive
            binary_op,
            typename std::iterator_traits< InputIterator >::iterator_category( ) );
    }

    //////////////////////////////////////////
    //  Exclusive scan overloads
    //////////////////////////////////////////
    template<
        typename InputIterator,
        typename OutputIterator,
        typename UnaryFunction,
        typename T,
        typename BinaryFunction>
    OutputIterator
    transform_exclusive_scan(
        InputIterator first,
        InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op,
        T init,
        BinaryFunction binary_op,
        const std::string& user_code )
    {
        return detail::transform_scan_detect_random_access(
            control::getDefault( ),
            first,
            last,
            result,
            unary_op,
            init,
            false, // exclusive
            binary_op,
            typename std::iterator_traits< InputIterator >::iterator_category( ) );
    }

    template<
        typename InputIterator,
        typename OutputIterator,
        typename UnaryFunction,
        typename T,
        typename BinaryFunction>
    OutputIterator
    transform_exclusive_scan(
        bolt::cl::control &ctl,
        InputIterator first,
        InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op,
        T init,
        BinaryFunction binary_op,
        const std::string& user_code )
    {
        return detail::transform_scan_detect_random_access(
            ctl,
            first,
            last,
            result,
            unary_op,
            init,
            false, // exclusive
            binary_op,
            typename std::iterator_traits< InputIterator >::iterator_category( ) );
    }

////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace cl
} //namespace bolt

#endif
