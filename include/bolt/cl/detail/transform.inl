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

#pragma once
#if !defined( BOLT_CL_TRANSFORM_INL )
#define BOLT_CL_TRANSFORM_INL
#define WAVEFRONT_SIZE 64
#define TRANSFORM_ENABLE_PROFILING 0

#include <type_traits>

#ifdef ENABLE_TBB
    #include "bolt/btbb/transform.h"
#endif

#include "bolt/cl/bolt.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/iterator/iterator_traits.h"

namespace bolt {
namespace cl {

namespace detail {

  enum TransformTypes {transform_iType1, transform_DVInputIterator1, transform_iType2, transform_DVInputIterator2,
                       transform_oTypeB,transform_DVOutputIteratorB, transform_BinaryFunction, transform_endB };
  enum TransformUnaryTypes {transform_iType, transform_DVInputIterator, transform_oTypeU,
                            transform_DVOutputIteratorU, transform_UnaryFunction, transform_endU };

class Transform_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    Transform_KernelTemplateSpecializer() : KernelTemplateSpecializer()
        {
        addKernelName("transformTemplate");
        addKernelName("transformNoBoundsCheckTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& binaryTransformKernels ) const
            {
        const std::string templateSpecializationString =
                "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
            "kernel void "+name(0)+"(\n"
            "global " + binaryTransformKernels[transform_iType1] + "* A_ptr,\n"
            + binaryTransformKernels[transform_DVInputIterator1] + " A_iter,\n"
            "global " + binaryTransformKernels[transform_iType2] + "* B_ptr,\n"
            + binaryTransformKernels[transform_DVInputIterator2] + " B_iter,\n"
            "global " + binaryTransformKernels[transform_oTypeB] + "* Z_ptr,\n"
            + binaryTransformKernels[transform_DVOutputIteratorB] + " Z_iter,\n"
                "const uint length,\n"
            "global " + binaryTransformKernels[transform_BinaryFunction] + "* userFunctor);\n\n"

                "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name("+name(1)+"Instantiated)))\n"
            "kernel void "+name(1)+"(\n"
            "global " + binaryTransformKernels[transform_iType1] + "* A_ptr,\n"
            + binaryTransformKernels[transform_DVInputIterator1] + " A_iter,\n"
            "global " + binaryTransformKernels[transform_iType2] + "* B_ptr,\n"
            + binaryTransformKernels[transform_DVInputIterator2] + " B_iter,\n"
            "global " + binaryTransformKernels[transform_oTypeB] + "* Z_ptr,\n"
            + binaryTransformKernels[transform_DVOutputIteratorB] + " Z_iter,\n"
                "const uint length,\n"
            "global " + binaryTransformKernels[transform_BinaryFunction] + "* userFunctor);\n\n";

            return templateSpecializationString;
            }
    };

class TransformUnary_KernelTemplateSpecializer : public KernelTemplateSpecializer
    {
public:
    TransformUnary_KernelTemplateSpecializer() : KernelTemplateSpecializer()
        {
        addKernelName("unaryTransformTemplate");
        addKernelName("unaryTransformNoBoundsCheckTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& unaryTransformKernels ) const
            {
        const std::string templateSpecializationString =
            "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name("+name( 0 )+"Instantiated)))\n"
            "kernel void unaryTransformTemplate(\n"
            "global " + unaryTransformKernels[transform_iType] + "* A,\n"
            + unaryTransformKernels[transform_DVInputIterator] + " A_iter,\n"
            "global " + unaryTransformKernels[transform_oTypeU] + "* Z,\n"
            + unaryTransformKernels[transform_DVOutputIteratorU] + " Z_iter,\n"
            "const uint length,\n"
            "global " + unaryTransformKernels[transform_UnaryFunction] + "* userFunctor);\n\n"

            "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name("+name(1)+"Instantiated)))\n"
            "kernel void unaryTransformNoBoundsCheckTemplate(\n"
            "global " + unaryTransformKernels[transform_iType] + "* A,\n"
            + unaryTransformKernels[transform_DVInputIterator] + " A_iter,\n"
            "global " + unaryTransformKernels[transform_oTypeU] + "* Z,\n"
            + unaryTransformKernels[transform_DVOutputIteratorU] + " Z_iter,\n"
            "const uint length,\n"
            "global " +unaryTransformKernels[transform_UnaryFunction] + "* userFunctor);\n\n";

            return templateSpecializationString;
            }
        };


    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction>
    void transform_enqueue( bolt::cl::control &ctl, const DVInputIterator1& first1, const DVInputIterator1& last1,
        const DVInputIterator2& first2, const DVOutputIterator& result,
        const BinaryFunction& f, const std::string& cl_code)
    {
        typedef typename std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >(  first1.distance_to(last1) );
        if( distVec == 0 )
            return;

        const size_t numComputeUnits = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
        const size_t numWorkGroupsPerComputeUnit = ctl.getWGPerComputeUnit( );
        size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;

        /**********************************************************************************
         * Type Names - used in KernelTemplateSpecializer
         *********************************************************************************/

        std::vector<std::string> binaryTransformKernels(transform_endB);
        binaryTransformKernels[transform_iType1] = TypeName< iType1 >::get( );
        binaryTransformKernels[transform_iType2] = TypeName< iType2 >::get( );
        binaryTransformKernels[transform_DVInputIterator1] = TypeName< DVInputIterator1 >::get( );
        binaryTransformKernels[transform_DVInputIterator2] = TypeName< DVInputIterator2 >::get( );
        binaryTransformKernels[transform_oTypeB] = TypeName< oType >::get( );
        binaryTransformKernels[transform_DVOutputIteratorB] = TypeName< DVOutputIterator >::get( );
        binaryTransformKernels[transform_BinaryFunction] = TypeName< BinaryFunction >::get();

       /**********************************************************************************
        * Type Definitions - directrly concatenated into kernel string
        *********************************************************************************/

        // For user-defined types, the user must create a TypeName trait which returns the name of the
        //class - note use of TypeName<>::get to retrieve the name here.
        std::vector<std::string> typeDefinitions;
        PUSH_BACK_UNIQUE( typeDefinitions, cl_code)
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator2 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVOutputIterator >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< BinaryFunction  >::get() )
        /**********************************************************************************
         * Calculate WG Size
         *********************************************************************************/

        cl_int l_Error = CL_SUCCESS;
        const size_t wgSize  = WAVEFRONT_SIZE;
        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
        assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2

        int boundsCheck = 0;
        size_t wgMultiple = distVec;
        size_t lowerBits = ( distVec & (wgSize-1) );
        if( lowerBits )
        {
            //  Bump the workitem count to the next multiple of wgSize
            wgMultiple &= ~lowerBits;
            wgMultiple += wgSize;
        }
        else
        {
            boundsCheck = 1;
        }
        if (wgMultiple/wgSize < numWorkGroups)
            numWorkGroups = wgMultiple/wgSize;

        /**********************************************************************************
         * Compile Options
         *********************************************************************************/
        bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
        //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
        const size_t kernel_WgSize = (cpuDevice) ? 1 : wgSize;
        std::string compileOptions;
        std::ostringstream oss;
        oss << " -DKERNELWORKGROUPSIZE=" << kernel_WgSize;
        compileOptions = oss.str();

        /**********************************************************************************
          * Request Compiled Kernels
          *********************************************************************************/
         Transform_KernelTemplateSpecializer ts_kts;
         std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
             ctl,
             binaryTransformKernels,
             &ts_kts,
             typeDefinitions,
             transform_kernels,
             compileOptions);
         // kernels returned in same order as added in KernelTemplaceSpecializer constructor


        ALIGNED( 256 ) BinaryFunction aligned_binary( f );
        control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_binary ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );

        typename DVInputIterator1::Payload first1_payload = first1.gpuPayload( );
        typename DVInputIterator2::Payload first2_payload = first2.gpuPayload( );
        typename DVOutputIterator::Payload result_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg( 0, first1.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, first1.gpuPayloadSize( ),&first1_payload);
        kernels[boundsCheck].setArg( 2, first2.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, first2.gpuPayloadSize( ),&first2_payload);
        kernels[boundsCheck].setArg( 4, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, result.gpuPayloadSize( ),&result_payload);
        kernels[boundsCheck].setArg( 6, distVec );
        kernels[boundsCheck].setArg( 7, *userFunctor);

        ::cl::Event transformEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
          kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange(wgMultiple), // numWorkGroups*wgSize
            ::cl::NDRange(wgSize),
            NULL,
            &transformEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );

        ::bolt::cl::wait(ctl, transformEvent);

#if TRANSFORM_ENABLE_PROFILING
        if( 0 )
        {
          cl_ulong start_time, stop_time;

          l_Error = transformEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &start_time);
          V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
          l_Error = transformEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &stop_time);
          V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
          size_t time = stop_time - start_time;
          std::cout << "Global Memory Bandwidth: "<<((distVec*(2.0*sizeof(iType1)+sizeof(oType)))/time)<<std::endl;
        }
#endif // BOLT_ENABLE_PROFILING

    };

    template< typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction >
    void transform_unary_enqueue( ::bolt::cl::control &ctl, const DVInputIterator& first, const DVInputIterator& last,
        const DVOutputIterator& result, const UnaryFunction& f, const std::string& cl_code)
    {
        typedef typename std::iterator_traits<DVInputIterator>::value_type iType;
        typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >( std::distance( first, last ) );
        if( distVec == 0 )
            return;

        const size_t numComputeUnits = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
        const size_t numWorkGroupsPerComputeUnit = ctl.getWGPerComputeUnit( );
        const size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;

        /**********************************************************************************
         * Type Names - used in KernelTemplateSpecializer
         *********************************************************************************/

        std::vector<std::string> unaryTransformKernels( transform_endU );
        unaryTransformKernels[transform_iType] = TypeName< iType >::get( );
        unaryTransformKernels[transform_DVInputIterator] = TypeName< DVInputIterator >::get( );
        unaryTransformKernels[transform_oTypeU] = TypeName< oType >::get( );
        unaryTransformKernels[transform_DVOutputIteratorU] = TypeName< DVOutputIterator >::get( );
        unaryTransformKernels[transform_UnaryFunction] = TypeName< UnaryFunction >::get();

        /**********************************************************************************
         * Type Definitions - directrly concatenated into kernel string
         *********************************************************************************/
        std::vector<std::string> typeDefinitions;
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVOutputIterator >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< UnaryFunction  >::get() )


        /**********************************************************************************
         * Calculate WG Size
         *********************************************************************************/
        cl_int l_Error = CL_SUCCESS;
        const size_t wgSize  = WAVEFRONT_SIZE;
        int boundsCheck = 0;

        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
        assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2

        size_t wgMultiple = distVec;
        size_t lowerBits = ( distVec & (wgSize-1) );
        if( lowerBits )
        {
            //  Bump the workitem count to the next multiple of wgSize
            wgMultiple &= ~lowerBits;
            wgMultiple += wgSize;
        }
        else
        {
            boundsCheck = 1;
        }

        /**********************************************************************************
         * Compile Options
         *********************************************************************************/
        bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
        //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
        const size_t kernel_WgSize = (cpuDevice) ? 1 : wgSize;
        std::string compileOptions;
        std::ostringstream oss;
        oss << " -DKERNELWORKGROUPSIZE=" << kernel_WgSize;
        compileOptions = oss.str();

        /**********************************************************************************
         * Request Compiled Kernels
         *********************************************************************************/
        TransformUnary_KernelTemplateSpecializer ts_kts;
        std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
            ctl,
            unaryTransformKernels,
            &ts_kts,
            typeDefinitions,
            transform_kernels,
            compileOptions);
        // kernels returned in same order as added in KernelTemplaceSpecializer constructor

        // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
        ALIGNED( 256 ) UnaryFunction aligned_binary( f );
        control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_binary ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );

        typename DVInputIterator::Payload first_payload = first.gpuPayload( );
        typename DVOutputIterator::Payload result_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg(0, first.getContainer().getBuffer() );
        kernels[boundsCheck].setArg(1, first.gpuPayloadSize( ),&first_payload);
        kernels[boundsCheck].setArg(2, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg(3, result.gpuPayloadSize( ),&result_payload);
        kernels[boundsCheck].setArg(4, distVec );
        kernels[boundsCheck].setArg(5, *userFunctor);
        //k.setArg(3, numElementsPerThread );

        ::cl::Event transformEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
            kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange( wgMultiple ), // numThreads
            ::cl::NDRange( wgSize ),
            NULL,
            &transformEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );

        ::bolt::cl::wait(ctl, transformEvent);

#if TRANSFORM_ENABLE_PROFILING
        if( 0 )
        {
          cl_ulong start_time, stop_time;

          l_Error = transformEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &start_time);
          V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
          l_Error = transformEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &stop_time);
          V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
          size_t time = stop_time - start_time;
          //std::cout << "Global Memory Bandwidth: "<<((distVec*(1.0*sizeof(iType)+sizeof(oType)))/time)<< std::endl;

        }
#endif // BOLT_ENABLE_PROFILING

    };

    /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
        \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
        *  iterators.  This overload is called strictly for non-device_vector iterators
    */
    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
    void transform_pick_iterator( bolt::cl::control &ctl,  const InputIterator1& first1, const InputIterator1& last1,
        const InputIterator2& first2, const OutputIterator& result, const BinaryFunction& f,
        const std::string& user_code, std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( first1, last1 );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		#if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
		
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
            std::transform( first1, last1, first2, result, f );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
                #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
                #endif
                bolt::btbb::transform(first1,last1,first2,result,f);

#else
                //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
                throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
			
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            device_vector< iType2 > dvInput2( first2, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            transform_enqueue(ctl,dvInput.begin( ),dvInput.end( ),dvInput2.begin( ),dvOutput.begin( ),f,user_code );

            // This should immediately map/unmap the buffer
            dvOutput.data( );
        }
    }

    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
    void transform_pick_iterator( bolt::cl::control &ctl,  const InputIterator1& first1, const InputIterator1& last1,
        const InputIterator2& fancyIter, const OutputIterator& result, const BinaryFunction& f,
        const std::string& user_code, std::random_access_iterator_tag, bolt::cl::fancy_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( first1, last1 );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		#if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
		
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
            std::transform( first1, last1, fancyIter, result, f );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
                #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
                #endif
                bolt::btbb::transform(first1,last1,fancyIter,result,f);

#else
                //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
                throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            transform_enqueue( ctl, dvInput.begin( ), dvInput.end( ), fancyIter, dvOutput.begin( ), f, user_code );

            // This should immediately map/unmap the buffer
            dvOutput.data( );
        }
    }

    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
    void transform_pick_iterator( bolt::cl::control &ctl,  const InputIterator1& fancyIterfirst,
        const InputIterator1& fancyIterlast, const InputIterator2& first2, const OutputIterator& result,

        const BinaryFunction& f, const std::string& user_code, bolt::cl::fancy_iterator_tag,
        std::random_access_iterator_tag)
    {

        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( fancyIterfirst, fancyIterlast );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
		#if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
		
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
            std::transform( fancyIterfirst, fancyIterlast, first2, result, f );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
                #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
                #endif
                bolt::btbb::transform(fancyIterfirst,fancyIterlast,first2,result,f);

#else
                //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
                throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvInput( first2, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            transform_enqueue( ctl, fancyIterfirst, fancyIterlast, dvInput.begin( ), dvOutput.begin( ),f,user_code );

            // This should immediately map/unmap the buffer
            dvOutput.data( );
        }
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction>

    void transform_pick_iterator( bolt::cl::control &ctl,  const DVInputIterator1& first1,
        const DVInputIterator1& last1, const DVInputIterator2& first2, const DVOutputIterator& result,
        const BinaryFunction& f,const std::string& user_code, bolt::cl::device_vector_tag, bolt::cl::device_vector_tag)
    {

        typedef typename std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( first1, last1 );
        if( sz == 0 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
             runMode = ctl.getDefaultPathToRun();
        }
        #if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< iType2 >::pointer secPtr =  first2.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

#if defined( _WIN32 )
            std::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ], &secPtr[ first2.m_Index ],
                stdext::make_checked_array_iterator( &resPtr[ result.m_Index ], sz ), f );
#else
            std::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                &secPtr[ first2.m_Index ], &resPtr[ result.m_Index ], f );
#endif
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {

#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
            #endif
				
            typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< iType2 >::pointer secPtr =  first2.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

            bolt::btbb::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                                   &secPtr[ first2.m_Index ],&resPtr[ result.m_Index ],f );


#else
             //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
             throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );
#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
            transform_enqueue( ctl, first1, last1, first2, result, f, user_code );
        }
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction>
    void transform_pick_iterator( bolt::cl::control &ctl,const DVInputIterator1& first1,const DVInputIterator1& last1,
        const DVInputIterator2& fancyIter, const DVOutputIterator& result, const BinaryFunction& f,
        const std::string& user_code, bolt::cl::device_vector_tag, bolt::cl::fancy_iterator_tag )
    {
        typedef typename std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( first1, last1 );
        if( sz == 0 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
        #if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

#if defined( _WIN32 )
            std::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ], fancyIter,
                stdext::make_checked_array_iterator( &resPtr[ result.m_Index ], sz ), f );
#else
            std::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                            fancyIter, &resPtr[ result.m_Index ], f );
#endif
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {

#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

            bolt::btbb::transform(  &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                                    fancyIter, &resPtr[ result.m_Index ], f );


#else
             //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
             throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
            transform_enqueue( ctl, first1, last1, fancyIter, result, f, user_code );
        }
    }

    /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
        \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
        *  iterators.  This overload is called strictly for non-device_vector iterators
    */
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    void
    transform_unary_pick_iterator( ::bolt::cl::control &ctl, const InputIterator& first, const InputIterator& last,
    const OutputIterator& result, const UnaryFunction& f, const std::string& user_code,
        std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator>::value_type iType;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = (last - first);
        if (sz == 0)
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
	    #if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
        if( runMode == bolt::cl::control::SerialCpu )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
            std::transform( first, last, result, f );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )

                #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
                #endif
                bolt::btbb::transform(first, last, result, f);

#else
             //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
             throw std::runtime_error( "The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.

            // Map the input iterator to a device_vector
            device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

            transform_unary_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), f, user_code );

            // This should immediately map/unmap the buffer
            dvOutput.data( );
        }
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction>
    void
    transform_unary_pick_iterator( ::bolt::cl::control &ctl, const DVInputIterator& first, const DVInputIterator& last,
    const DVOutputIterator& result, const UnaryFunction& f, const std::string& user_code,
        bolt::cl::device_vector_tag )
    {
        typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( first, last );
        if( sz == 0 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
             runMode = ctl.getDefaultPathToRun();
        }
        #if defined(BOLT_DEBUG_LOG)
        BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
        #endif
		
        //  TBB does not have an equivalent for two input iterator std::transform
        if( (runMode == bolt::cl::control::SerialCpu) )
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_SERIAL_CPU,"::Transform::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType >::pointer firstPtr = first.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr = result.getContainer( ).data( );

#if defined( _WIN32 )
            std::transform( &firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ],
                stdext::make_checked_array_iterator( &resPtr[ result.m_Index ], sz ), f );
#else
            std::transform( &firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], &resPtr[ result.m_Index ], f );
#endif
            return;
        }
        else if( (runMode == bolt::cl::control::MultiCoreCpu) )
        {

#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_MULTICORE_CPU,"::Transform::MULTICORE_CPU");
            #endif
				
            typename bolt::cl::device_vector< iType >::pointer firstPtr = first.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr = result.getContainer( ).data( );

            bolt::btbb::transform(&firstPtr[ first.m_Index ], &firstPtr[ last.m_Index ], &resPtr[ result.m_Index ], f );


#else
             //std::cout << "The MultiCoreCpu version of Transform is not enabled. " << std ::endl;
             throw std::runtime_error("The MultiCoreCpu version of transform is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_TRANSFORM,BOLTLOG::BOLT_OPENCL_GPU,"::Transform::OPENCL_GPU");
            #endif
            transform_unary_enqueue( ctl, first, last, result, f, user_code );
        }
    }


    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
    void transform_detect_random_access( bolt::cl::control& ctl, const InputIterator1& first1,
            const InputIterator1& last1, const InputIterator2& first2, const OutputIterator& result,
            const BinaryFunction& f, const std::string& user_code, std::random_access_iterator_tag,
            std::random_access_iterator_tag )
    {
            transform_pick_iterator( ctl, first1, last1, first2, result, f, user_code,
            typename std::iterator_traits< InputIterator1 >::iterator_category( ),
            typename std::iterator_traits< InputIterator2 >::iterator_category( ) );
    };

    // Wrapper that uses default ::bolt::cl::control class, iterator interface
    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
    void transform_detect_random_access( bolt::cl::control& ctl, const InputIterator1& first1,
            const InputIterator1& last1, const InputIterator2& first2, const OutputIterator& result,
            const BinaryFunction& f,const std::string& user_code, std::input_iterator_tag, std::input_iterator_tag )
        {
            // TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
            // to a temporary buffer.  Should we?

            static_assert( std::is_same< InputIterator1, std::input_iterator_tag >::value, "Bolt only supports random access iterator types" );
        };



        template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    void transform_unary_detect_random_access( ::bolt::cl::control& ctl, const InputIterator& first1,
        const InputIterator& last1,const OutputIterator& result, const UnaryFunction& f,
        const std::string& user_code, std::random_access_iterator_tag )

    {
        transform_unary_pick_iterator( ctl, first1, last1, result, f, user_code,
            typename  std::iterator_traits< InputIterator >::iterator_category( ) );
    };

// Wrapper that uses default ::bolt::cl::control class, iterator interface
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    void transform_unary_detect_random_access( ::bolt::cl::control& ctl, const InputIterator& first1,
        const InputIterator& last1, const OutputIterator& result, const UnaryFunction& f,
        const std::string& user_code, std::input_iterator_tag )

    {
        //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
        //  to a temporary buffer.  Should we?
        static_assert( std::is_same< InputIterator, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };



} //End of detail namespace

// two-input transform, std:: iterator
template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >

void transform( bolt::cl::control& ctl, InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                OutputIterator result, BinaryFunction f, const std::string& user_code )
{
    detail::transform_detect_random_access( ctl, first1, last1, first2, result, f, user_code,
      typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
       typename  std::iterator_traits< InputIterator2 >::iterator_category( ) );
}

    // default ::bolt::cl::control, two-input transform, std:: iterator
template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction >
void transform( InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result,
                BinaryFunction f, const std::string& user_code )
{
    detail::transform_detect_random_access( control::getDefault(), first1, last1, first2, result, f, user_code,
       typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
       typename  std::iterator_traits< InputIterator2 >::iterator_category( ) );
}

// one-input transform, std:: iterator
template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
void transform( ::bolt::cl::control& ctl, InputIterator first1, InputIterator last1, OutputIterator result,
                UnaryFunction f, const std::string& user_code )
{
    detail::transform_unary_detect_random_access( ctl, first1, last1, result, f, user_code,
       typename  std::iterator_traits< InputIterator >::iterator_category( ) );
}

// default control, one-input transform, std:: iterator
template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
void transform( InputIterator first1, InputIterator last1, OutputIterator result,
                UnaryFunction f, const std::string& user_code )
{
    detail::transform_unary_detect_random_access( control::getDefault(), first1, last1, result, f, user_code,
       typename  std::iterator_traits< InputIterator >::iterator_category( ) );
}

} //End of cl namespace
} //End of bolt namespace

#endif
