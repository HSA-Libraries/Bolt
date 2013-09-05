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
#if !defined( BOLT_CL_GATHER_INL )
#define BOLT_CL_GATHER_INL
#define WAVEFRONT_SIZE 64

#include <type_traits>

#ifdef ENABLE_TBB
    #include "bolt/btbb/gather.h"
#endif

#include "bolt/cl/bolt.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/iterator/iterator_traits.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/clcode.h"

namespace bolt {
namespace cl {

namespace detail {

/* Begin-- Serial Implementation of the gather and gather_if routines */



template< typename InputIterator1,
          typename InputIterator2,
          typename OutputIterator >

void serial_gather(InputIterator1 mapfirst,
                   InputIterator1 maplast,
                   InputIterator2 input,
                   OutputIterator result)
{
    //std::cout<<"Serial code path ... \n";
   size_t numElements = static_cast< size_t >( std::distance( mapfirst, maplast ) );
   typedef typename  std::iterator_traits<InputIterator1>::value_type iType1;
   iType1 temp;
   for(size_t iter = 0; iter < numElements; iter++)
   {
                   temp = *(mapfirst + (int)iter);
                  *(result + (int)iter) = *(input + (int)temp);
   }
}


template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator >

void serial_gather_if(InputIterator1 mapfirst,
                      InputIterator1 maplast,
                      InputIterator2 stencil,
                      InputIterator3 input,
                      OutputIterator result)
{
    //std::cout<<"Serial code path ... \n";
   size_t numElements = static_cast< size_t >( std::distance( mapfirst, maplast ) );
   for(size_t iter = 0; iter < numElements; iter++)
   {
       if(stencil[(int)iter]== 1)
            result[(int)iter] = *(input + mapfirst[(int)iter]);
   }
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator,
          typename BinaryPredicate >

void serial_gather_if(InputIterator1 mapfirst,
                      InputIterator1 maplast,
                      InputIterator2 stencil,
                      InputIterator3 input,
                      OutputIterator result,
                      BinaryPredicate pred)
{
   //std::cout<<"Serial code path ... \n";
   unsigned int numElements = static_cast< unsigned int >( std::distance( mapfirst, maplast ) );
  // for (InputIterator1 iter = mapfirst; iter != maplast; iter++)
  // {
  //      if(pred(*(stencil + ( iter - mapfirst))))
        //{
  //          // result[(int)iter] = input[mapfirst[(int)iter]];
        //	 *(result + (iter - mapfirst) )= input[*iter];
        //}
  // }
      for(unsigned int iter = 0; iter < numElements; iter++)
   {
        if(pred(*(stencil + (int)iter)))
             result[(int)iter] = input[mapfirst[(int)iter]];
   }
}

 /* End-- Serial Implementation of the gather and gather_if routines */

////////////////////////////////////////////////////////////////////
// GatherIf KTS
////////////////////////////////////////////////////////////////////
  enum GatherIfTypes { gather_if_mapType, gather_if_DVMapType,
                       gather_if_stencilType, gather_if_DVStencilType,
                       gather_if_iType, gather_if_DVInputIterator,
                       gather_if_resultType, gather_if_DVResultType,
                       gather_if_Predicate, gather_if_endB };

class GatherIf_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    GatherIf_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
       addKernelName("gatherIfTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& gatherIfKernels ) const
    {
      const std::string templateSpecializationString =
        "// Host generates this instantiation string with user-specified value type and functor\n"
        "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
        "kernel void "+name(0)+"(\n"
        "global " + gatherIfKernels[gather_if_mapType] + "* map, \n"
        + gatherIfKernels[gather_if_DVMapType] + " mapIter, \n"
        "global " + gatherIfKernels[gather_if_stencilType] + "* stencil, \n"
        + gatherIfKernels[gather_if_DVStencilType] + " stencilIter, \n"
        "global " + gatherIfKernels[gather_if_iType] + "* input, \n"
        + gatherIfKernels[gather_if_DVInputIterator] + " inputIter, \n"
        "global " + gatherIfKernels[gather_if_resultType] + "* result, \n"
        + gatherIfKernels[gather_if_DVResultType] + " resultIter, \n"
        "const uint length, \n"
        "global " + gatherIfKernels[gather_if_Predicate] + "* functor);\n\n";

        return templateSpecializationString;
    }
};

////////////////////////////////////////////////////////////////////
// Gather KTS
////////////////////////////////////////////////////////////////////

  enum GatherTypes { gather_mapType, gather_DVMapType,
                     gather_iType, gather_DVInputIterator,
                     gather_resultType, gather_DVResultType,
                     gather_endB };

class GatherKernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    GatherKernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
       addKernelName("gatherTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& gatherKernels ) const
    {
      const std::string templateSpecializationString =
        "// Host generates this instantiation string with user-specified value type and functor\n"
        "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
        "kernel void "+name(0)+"(\n"
        "global " + gatherKernels[gather_mapType] + "* map, \n"
        + gatherKernels[gather_DVMapType] + " mapIter, \n"
        "global " + gatherKernels[gather_iType] + "* input, \n"
        + gatherKernels[gather_DVInputIterator] + " inputIter, \n"
        "global " + gatherKernels[gather_resultType] + "* result, \n"
        + gatherKernels[gather_DVResultType] + " resultIter, \n"
        "const uint length ); \n";

        return templateSpecializationString;
    }
};


////////////////////////////////////////////////////////////////////
// GatherIf enqueue
////////////////////////////////////////////////////////////////////

    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVInputIterator3,
              typename DVOutputIterator,
              typename Predicate >
    void gather_if_enqueue( bolt::cl::control &ctl,
                            const DVInputIterator1& map_first,
                            const DVInputIterator1& map_last,
                            const DVInputIterator2& stencil,
                            const DVInputIterator3& input,
                            const DVOutputIterator& result,
                            const Predicate& pred,
                            const std::string& cl_code )
    {
        typedef typename std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<DVInputIterator3>::value_type iType3;
        typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >(  map_first.distance_to(map_last) );
        if( distVec == 0 )
            return;

        const size_t numComputeUnits = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
        const size_t numWorkGroupsPerComputeUnit = ctl.getWGPerComputeUnit( );
        size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;

        /**********************************************************************************
         * Type Names - used in KernelTemplateSpecializer
         *********************************************************************************/

        std::vector<std::string> gatherIfKernels(gather_if_endB);
        gatherIfKernels[gather_if_mapType] = TypeName< iType1 >::get( );
        gatherIfKernels[gather_if_stencilType] = TypeName< iType2 >::get( );
        gatherIfKernels[gather_if_iType] = TypeName< iType3 >::get( );
        gatherIfKernels[gather_if_DVMapType] = TypeName< DVInputIterator1 >::get( );
        gatherIfKernels[gather_if_DVStencilType] = TypeName< DVInputIterator2 >::get( );
        gatherIfKernels[gather_if_DVInputIterator] = TypeName< DVInputIterator3 >::get( );
        gatherIfKernels[gather_if_resultType] = TypeName< oType >::get( );
        gatherIfKernels[gather_if_DVResultType] = TypeName< DVOutputIterator >::get( );
        gatherIfKernels[gather_if_Predicate] = TypeName< Predicate >::get();

       /**********************************************************************************
        * Type Definitions - directrly concatenated into kernel string
        *********************************************************************************/

        // For user-defined types, the user must create a TypeName trait which returns the name of the
        //class - note use of TypeName<>::get to retrieve the name here.
        std::vector<std::string> typeDefinitions;
        PUSH_BACK_UNIQUE( typeDefinitions, cl_code)
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType2 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType3 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator2 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator3 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVOutputIterator >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< Predicate >::get() )
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

        //if (wgMultiple/wgSize < numWorkGroups)
        //    numWorkGroups = wgMultiple/wgSize;

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
         GatherIf_KernelTemplateSpecializer s_if_kts;
         std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
             ctl,
             gatherIfKernels,
             &s_if_kts,
             typeDefinitions,
             gather_kernels,
             compileOptions);
         // kernels returned in same order as added in KernelTemplaceSpecializer constructor

        ALIGNED( 256 ) Predicate aligned_binary( pred );
        control::buffPointer userPredicate = ctl.acquireBuffer( sizeof( aligned_binary ),
                                                                CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY,
                                                                &aligned_binary );
       typename DVInputIterator1::Payload   map_payload = map_first.gpuPayload( );
       typename DVInputIterator2::Payload   stencil_payload = stencil.gpuPayload( );
       typename DVInputIterator3::Payload   input_payload = input.gpuPayload( );
       typename DVOutputIterator::Payload   result_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg( 0, map_first.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, map_first.gpuPayloadSize( ), &map_payload);
        kernels[boundsCheck].setArg( 2, stencil.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, stencil.gpuPayloadSize( ),&stencil_payload );
        kernels[boundsCheck].setArg( 4, input.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, input.gpuPayloadSize( ), &input_payload );
        kernels[boundsCheck].setArg( 6, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 7, result.gpuPayloadSize( ),&result_payload );
        kernels[boundsCheck].setArg( 8, distVec );
        kernels[boundsCheck].setArg( 9, *userPredicate );

        ::cl::Event gatherIfEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
            kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange(wgMultiple), // numWorkGroups*wgSize
            ::cl::NDRange(wgSize),
            NULL,
            &gatherIfEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for gather_if() kernel" );

        ::bolt::cl::wait(ctl, gatherIfEvent);

    };

////////////////////////////////////////////////////////////////////
// Gather enqueue
////////////////////////////////////////////////////////////////////
    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVOutputIterator >
    void gather_enqueue( bolt::cl::control &ctl,
                         const DVInputIterator1& map_first,
                         const DVInputIterator1& map_last,
                         const DVInputIterator2& input,
                         const DVOutputIterator& result,
                         const std::string& cl_code )
    {
        typedef typename std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >(  map_first.distance_to(map_last) );
        if( distVec == 0 )
            return;

        const size_t numComputeUnits = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
        const size_t numWorkGroupsPerComputeUnit = ctl.getWGPerComputeUnit( );
        size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;

        /**********************************************************************************
         * Type Names - used in KernelTemplateSpecializer
         *********************************************************************************/
        std::vector<std::string> gatherKernels(gather_endB);
        gatherKernels[gather_mapType] = TypeName< iType1 >::get( );
        gatherKernels[gather_iType] = TypeName< iType2 >::get( );
        gatherKernels[gather_DVMapType] = TypeName< DVInputIterator1 >::get( );
        gatherKernels[gather_DVInputIterator] = TypeName< DVInputIterator2 >::get( );
        gatherKernels[gather_resultType] = TypeName< oType >::get( );
        gatherKernels[gather_DVResultType] = TypeName< DVOutputIterator >::get( );

       /**********************************************************************************
        * Type Definitions - directrly concatenated into kernel string
        *********************************************************************************/

        // For user-defined types, the user must create a TypeName trait which returns the name of the
        //class - note use of TypeName<>::get to retrieve the name here.
        std::vector<std::string> typeDefinitions;
        PUSH_BACK_UNIQUE( typeDefinitions, cl_code)
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< iType2 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator1 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVInputIterator2 >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< oType >::get() )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVOutputIterator >::get() )
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

        //if (wgMultiple/wgSize < numWorkGroups)
        //    numWorkGroups = wgMultiple/wgSize;

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
         GatherKernelTemplateSpecializer s_kts;
         std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
             ctl,
             gatherKernels,
             &s_kts,
             typeDefinitions,
             gather_kernels,
             compileOptions);
         // kernels returned in same order as added in KernelTemplaceSpecializer constructor

       typename DVInputIterator1::Payload   map_payload = map_first.gpuPayload( );
       typename DVInputIterator2::Payload   input_payload = input.gpuPayload( );
       typename DVOutputIterator::Payload   result_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg( 0, map_first.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, map_first.gpuPayloadSize( ),&map_payload );
        kernels[boundsCheck].setArg( 2, input.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, input.gpuPayloadSize( ),&input_payload );
        kernels[boundsCheck].setArg( 4, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, result.gpuPayloadSize( ), &result_payload );
        kernels[boundsCheck].setArg( 6, distVec );

        ::cl::Event gatherEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
            kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange(wgMultiple), // numWorkGroups*wgSize
            ::cl::NDRange(wgSize),
            NULL,
            &gatherEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for gather_if() kernel" );

        ::bolt::cl::wait(ctl, gatherEvent);

    };

////////////////////////////////////////////////////////////////////
// Enqueue ends
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// GatherIf pick iterator
////////////////////////////////////////////////////////////////////

// Host vectors

    /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
        \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
        *  iterators.  This overload is called strictly for non-device_vector iterators
    */
    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename BinaryPredicate >
    void gather_if_pick_iterator( bolt::cl::control &ctl,
                                  const InputIterator1& map_first,
                                  const InputIterator1& map_last,
                                  const InputIterator2& stencil,
                                  const InputIterator3& input,
                                  const OutputIterator& result,
                                  const BinaryPredicate& pred,
                                  const std::string& user_code,
                                  std::random_access_iterator_tag,
                                  std::random_access_iterator_tag,
                                  std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;

        size_t sz = std::distance( map_first, map_last );

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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather_If::SERIAL_CPU");
            #endif
            serial_gather_if(map_first, map_last, stencil, input, result, pred );
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
           #if defined(BOLT_DEBUG_LOG)
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather_If::MULTICORE_CPU");
           #endif
           bolt::btbb::gather_if(map_first, map_last, stencil, input, result, pred);
#else
          throw std::runtime_error( "The MultiCoreCpu version of gather_if is not enabled to be built! \n" );
#endif
        }
        else
        {
		  #if defined(BOLT_DEBUG_LOG)
          dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather_If::OPENCL_GPU");
          #endif
						
          // Map the input iterator to a device_vector
          device_vector< iType1 > dvMap( map_first, map_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
          device_vector< iType2 > dvStencil( stencil, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );
          device_vector< iType3 > dvInput( input, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

          // Map the output iterator to a device_vector
          device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );
          gather_if_enqueue( ctl,
                              dvMap.begin( ),
                              dvMap.end( ),
                              dvStencil.begin( ),
                              dvInput.begin( ),
                              dvResult.begin( ),
                              pred,
                              user_code );

          // This should immediately map/unmap the buffer
          dvResult.data( );
        }
    }


// Stencil is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename BinaryPredicate >
    void gather_if_pick_iterator( bolt::cl::control &ctl,
                                  const InputIterator1& map_first,
                                  const InputIterator1& map_last,
                                  const InputIterator2& stencilFancyIter,
                                  const InputIterator3& input,
                                  const OutputIterator& result,
                                  const BinaryPredicate& pred,
                                  const std::string& user_code,
                                  std::random_access_iterator_tag,
                                  bolt::cl::fancy_iterator_tag,
                                  std::random_access_iterator_tag
                                   )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( map_first, map_last );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather_If::SERIAL_CPU");
            #endif
            serial_gather_if(map_first, map_last, stencilFancyIter, input, result, pred);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather_If::MULTICORE_CPU");
            #endif
            bolt::btbb::gather_if(map_first, map_last, stencilFancyIter, input, result, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather_If::OPENCL_GPU");
            #endif
		  
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvMap( map_first, map_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            device_vector< iType3 > dvInput( input, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,true, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_if_enqueue( ctl,
                               dvMap.begin( ),
                               dvMap.end( ),
                               stencilFancyIter,
                               dvInput.begin(),
                               dvResult.begin( ),
                               pred,
                               user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// Input is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename BinaryPredicate >
    void gather_if_pick_iterator( bolt::cl::control &ctl,
                                  const InputIterator1& fancymapFirst,
                                  const InputIterator1& fancymapLast,
                                  const InputIterator2& stencil,
                                  const InputIterator3& input,
                                  const OutputIterator& result,
                                  const BinaryPredicate& pred,
                                  const std::string& user_code,
                                  bolt::cl::fancy_iterator_tag,
                                  std::random_access_iterator_tag,
                                  std::random_access_iterator_tag )
    {

        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( fancymapFirst, fancymapLast );
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
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather_If::SERIAL_CPU");
           #endif
           serial_gather_if (fancymapFirst, fancymapLast, stencil, input, result, pred );
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather_If::MULTICORE_CPU");
            #endif
            bolt::btbb::gather_if( fancymapFirst, fancymapLast, stencil, input, result, pred );
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather_If::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvStencil( stencil, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );
            device_vector< iType3 > dvInput( input, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_if_enqueue( ctl,
                               fancymapFirst,
                               fancymapLast,
                               dvStencil.begin( ),
                               dvInput.begin( ),
                               dvResult.begin( ),
                               pred,
                               user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// Device Vectors

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVInputIterator3,
              typename DVOutputIterator,
              typename BinaryPredicate >
    void gather_if_pick_iterator( bolt::cl::control &ctl,
                                  const DVInputIterator1& map_first,
                                  const DVInputIterator1& map_last,
                                  const DVInputIterator2& stencil,
                                  const DVInputIterator3& input,
                                  const DVOutputIterator& result,
                                  const BinaryPredicate& pred,
                                  const std::string& user_code,
                                  bolt::cl::device_vector_tag,
                                  bolt::cl::device_vector_tag,
                                  bolt::cl::device_vector_tag )
    {

        typedef typename std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef typename std::iterator_traits< DVInputIterator3 >::value_type iType3;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( map_first, map_last );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather_If::SERIAL_CPU");
            #endif
		   
            typename bolt::cl::device_vector< iType1 >::pointer mapPtr =  map_first.getContainer( ).data( );
            typename bolt::cl::device_vector< iType2 >::pointer stenPtr =  stencil.getContainer( ).data( );
            typename bolt::cl::device_vector< iType3 >::pointer inputPtr =  input.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

          #if defined( _WIN32 )
            serial_gather_if(&mapPtr[ map_first.m_Index ], &mapPtr[ map_last.m_Index ], &stenPtr[ stencil.m_Index ],
                 &inputPtr[ input.m_Index ], stdext::make_checked_array_iterator( &resPtr[ result.m_Index ], sz ), pred );

         #else
            serial_gather_if( &mapPtr[ map_first.m_Index ], &mapPtr[ map_last.m_Index ], &stenPtr[ stencil.m_Index ],
                 &inputPtr[ input.m_Index ], &resPtr[ result.m_Index ], pred );
         #endif
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
          {
		   #if defined(BOLT_DEBUG_LOG)
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather_If::MULTICORE_CPU");
           #endif
           typename bolt::cl::device_vector< iType1 >::pointer mapPtr =  map_first.getContainer( ).data( );
           typename bolt::cl::device_vector< iType2 >::pointer stenPtr =  stencil.getContainer( ).data( );
           typename bolt::cl::device_vector< iType3 >::pointer inputPtr =  input.getContainer( ).data( );
           typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

           bolt::btbb::gather_if( &mapPtr[ map_first.m_Index ], &mapPtr[ map_last.m_Index ], &stenPtr[ stencil.m_Index ],
                 &inputPtr[ input.m_Index ], &resPtr[ result.m_Index ], pred );
          }
#else
             throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather_If::OPENCL_GPU");
            #endif	
            gather_if_enqueue( ctl, map_first, map_last, stencil, input, result, pred, user_code );
        }
    }

////////////////////////////////////////////////////////////////////
// Gather pick iterator
////////////////////////////////////////////////////////////////////

// Host vectors

    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                               const InputIterator1& map_first,
                               const InputIterator1& map_last,
                               const InputIterator2& input,
                               const OutputIterator& result,
                               const std::string& user_code,
                               std::random_access_iterator_tag,
                               std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;

        size_t sz = std::distance( map_first, map_last );

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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
            #endif
            serial_gather(map_first, map_last, input, result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
           #if defined(BOLT_DEBUG_LOG)
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
           #endif
           bolt::btbb::gather(map_first, map_last, input, result);
#else
          throw std::runtime_error( "The MultiCoreCpu version of gather_if is not enabled to be built! \n" );

#endif
        }
        else
        {
		  #if defined(BOLT_DEBUG_LOG)
          dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
          #endif
		  
          // Map the input iterator to a device_vector
          device_vector< iType1 > dvMap( map_first, map_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
          device_vector< iType2 > dvInput( input, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );
          // Map the output iterator to a device_vector
          device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );
          gather_enqueue( ctl,
                          dvMap.begin( ),
                          dvMap.end( ),
                          dvInput.begin( ),
                          dvResult.begin( ),
                          user_code );

          // This should immediately map/unmap the buffer
          dvResult.data( );
        }
    }

// Map is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                               const InputIterator1& firstFancy,
                               const InputIterator1& lastFancy,
                               const InputIterator2& input,
                               const OutputIterator& result,
                               const std::string& user_code,
                               bolt::cl::fancy_iterator_tag,
                               std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( firstFancy, lastFancy );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
            #endif
			
            serial_gather( firstFancy, lastFancy, input, result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
            #endif
            bolt::btbb::gather( firstFancy, lastFancy, input, result);
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
		  
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvInput( input, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_enqueue( ctl,
                            firstFancy,
                            lastFancy,
                            dvInput.begin(),
                            dvResult.begin( ),
                            user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }


// Input is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator>
    void gather_pick_iterator( bolt::cl::control &ctl,
                               const InputIterator1& map_first,
                               const InputIterator1& map_last,
                               const InputIterator2& inputFancy,
                               const OutputIterator& result,
                               const std::string& user_code,
                               std::random_access_iterator_tag,
                               bolt::cl::fancy_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( map_first, map_last );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
            #endif
            serial_gather(map_first, map_last, inputFancy, result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
             #if defined(BOLT_DEBUG_LOG)
             dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
             #endif
             bolt::btbb::gather(map_first, map_last, inputFancy, result);
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvMap( map_first, map_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_enqueue( ctl,
                            dvMap.begin( ),
                            dvMap.end( ),
                            inputFancy,
                            dvResult.begin( ),
                            user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// Device Vectors
    template< typename DVMapIterator,
              typename DVInputIterator,
              typename DVOutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                               const DVMapIterator& map_first,
                               const DVMapIterator& map_last,
                               const DVInputIterator& input,
                               const DVOutputIterator& result,
                               const std::string& user_code,
                               bolt::cl::device_vector_tag,
                               bolt::cl::device_vector_tag )
    {

        typedef typename std::iterator_traits< DVMapIterator >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator >::value_type iType2;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( map_first, map_last );
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
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
           #endif
           typename bolt::cl::device_vector< iType1 >::pointer  MapBuffer  =  map_first.getContainer( ).data( );
           typename bolt::cl::device_vector< iType2 >::pointer InputBuffer    =  input.getContainer( ).data( );
           typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
           serial_gather(&MapBuffer[ map_first.m_Index ], &MapBuffer[ map_last.m_Index ], &InputBuffer[ input.m_Index ],
           &ResultBuffer[ result.m_Index ]);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
         #if defined( ENABLE_TBB )
            {
			   #if defined(BOLT_DEBUG_LOG)
               dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
               #endif
               typename bolt::cl::device_vector< iType1 >::pointer  MapBuffer  =  map_first.getContainer( ).data( );
               typename bolt::cl::device_vector< iType2 >::pointer InputBuffer    =  input.getContainer( ).data( );
               typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
               bolt::btbb::gather(&MapBuffer[ map_first.m_Index ], &MapBuffer[ map_last.m_Index ], &InputBuffer[ input.m_Index ],
               &ResultBuffer[ result.m_Index ]);
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
            gather_enqueue( ctl,
                            map_first,
                            map_last,
                            input,
                            result,
                            user_code );
        }
    }

// Map fancy ; Input DV
    template< typename FancyIterator,
              typename DVInputIterator,
              typename DVOutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                                const FancyIterator& firstFancy,
                                const FancyIterator& lastFancy,
                                const DVInputIterator& input,
                                const DVOutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::fancy_iterator_tag,
                                bolt::cl::device_vector_tag )
    {

        typedef typename std::iterator_traits< FancyIterator >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator >::value_type iType2;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( firstFancy, lastFancy );
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
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
           #endif
           typename bolt::cl::device_vector< iType2 >::pointer InputBuffer    =  input.getContainer( ).data( );
           typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
            serial_gather(firstFancy, lastFancy, &InputBuffer[ input.m_Index ], &ResultBuffer[ result.m_Index ]);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			   #if defined(BOLT_DEBUG_LOG)
               dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
               #endif
               typename bolt::cl::device_vector< iType2 >::pointer InputBuffer    =  input.getContainer( ).data( );
               typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
                bolt::btbb::gather(firstFancy, lastFancy, &InputBuffer[ input.m_Index ], &ResultBuffer[ result.m_Index ]);
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
            gather_enqueue( ctl,
                            firstFancy,
                            lastFancy,
                            input,
                            result,
                            user_code );
        }
    }

// Input fancy ; Map DV
    template< typename DVMapIterator,
              typename FancyInput,
              typename DVOutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                                const DVMapIterator& mapfirst,
                                const DVMapIterator& maplast,
                                const FancyInput& fancyInpt,
                                const DVOutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::device_vector_tag,
                                bolt::cl::fancy_iterator_tag )
    {

        typedef typename std::iterator_traits< DVMapIterator >::value_type iType1;
        typedef typename std::iterator_traits< FancyInput >::value_type iType2;
        typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( mapfirst, maplast );
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
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
           #endif
		   
           typename bolt::cl::device_vector< iType1 >::pointer mapBuffer    =  mapfirst.getContainer( ).data( );
           typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
            serial_gather( &mapBuffer[ mapfirst.m_Index ], &mapBuffer[ maplast.m_Index ], fancyInpt,
                                                                         &ResultBuffer[ result.m_Index ]);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			   #if defined(BOLT_DEBUG_LOG)
               dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
               #endif
               typename bolt::cl::device_vector< iType1 >::pointer mapBuffer    =  mapfirst.getContainer( ).data( );
               typename  bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
               bolt::btbb::gather(  &mapBuffer[ mapfirst.m_Index ], &mapBuffer[ maplast.m_Index ], fancyInpt,
                                                                         &ResultBuffer[ result.m_Index ]);
            }

#else
             throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
            gather_enqueue( ctl,
                            mapfirst,
                            maplast,
                            fancyInpt,
                            result,
                            user_code );
        }
    }

// Map DV ; Input random access
    template< typename DVInputIterator,
              typename InputIterator,
              typename OutputIterator >
    void gather_pick_iterator( bolt::cl::control &ctl,
                               const DVInputIterator& map_first,
                               const DVInputIterator& map_last,
                               const InputIterator& input,
                               const OutputIterator& result,
                               const std::string& user_code,
                               bolt::cl::device_vector_tag,
                               std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<DVInputIterator>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( map_first, map_last );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
            #endif
            serial_gather(map_first, map_last, input, result );
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
             {
			   #if defined(BOLT_DEBUG_LOG)
               dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
               #endif
               typename bolt::cl::device_vector< iType1 >::pointer mapBuffer    =  map_first.getContainer( ).data( );
                bolt::btbb::gather( &mapBuffer[ map_first.m_Index ], &mapBuffer[ map_last.m_Index ], input, result);
            }
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvInput( input, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_enqueue( ctl,
                            map_first,
                            map_last,
                            dvInput.begin(),
                            dvResult.begin( ),
                            user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// RA Map ; DV Input
    template< typename InputIterator,
              typename DVInputIterator,
              typename OutputIterator>
    void gather_pick_iterator( bolt::cl::control &ctl,
                                const InputIterator& map_first,
                                const InputIterator& map_last,
                                const DVInputIterator& input,
                                const OutputIterator& result,
                                const std::string& user_code,
                                std::random_access_iterator_tag,
                                bolt::cl::device_vector_tag )
    {
        typedef typename std::iterator_traits<InputIterator>::value_type iType1;
        typedef typename std::iterator_traits<DVInputIterator>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( map_first, map_last );
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
          dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_SERIAL_CPU,"::Gather::SERIAL_CPU");
          #endif
			
          typename bolt::cl::device_vector< iType2 >::pointer inputBuffer    =  input.getContainer( ).data( );
           serial_gather(map_first, map_last, &inputBuffer[ input.m_Index ], result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
           #if defined(BOLT_DEBUG_LOG)
           dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_MULTICORE_CPU,"::Gather::MULTICORE_CPU");
           #endif
           bolt::btbb::gather(map_first, map_last , input, result);
            {
               typename bolt::cl::device_vector< iType2 >::pointer inputBuffer    =  input.getContainer( ).data( );
                bolt::btbb::gather(map_first, map_last, &inputBuffer[ input.m_Index ], result);
            }
#else
            throw std::runtime_error( "The MultiCoreCpu version of gather is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_GATHER,BOLTLOG::BOLT_OPENCL_GPU,"::Gather::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvMap( map_first, map_last, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            gather_enqueue( ctl,
                            dvMap.begin(),
                            dvMap.end(),
                            input,
                            dvResult.begin( ),
                            user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }



////////////////////////////////////////////////////////////////////
// GatherIf detect random access
////////////////////////////////////////////////////////////////////



    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename BinaryPredicate >
    void gather_if_detect_random_access( bolt::cl::control& ctl,
                                         const InputIterator1& map_first,
                                         const InputIterator1& map_last,
                                         const InputIterator2& stencil,
                                         const InputIterator3& input,
                                         const OutputIterator& result,
                                         const BinaryPredicate& pred,
                                         const std::string& user_code,
                                         std::random_access_iterator_tag,
                                         std::random_access_iterator_tag,
                                         std::random_access_iterator_tag )
    {
       gather_if_pick_iterator( ctl,
                                map_first,
                                map_last,
                                stencil,
                                input,
                                result,
                                pred,
                                user_code,
                                typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                                typename std::iterator_traits< InputIterator2 >::iterator_category( ),
                                typename std::iterator_traits< InputIterator3 >::iterator_category( ) );
    };


    // Wrapper that uses default ::bolt::cl::control class, iterator interface
    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename BinaryPredicate >
    void gather_if_detect_random_access( bolt::cl::control& ctl,
                                         const InputIterator1& map_first,
                                         const InputIterator1& map_last,
                                         const InputIterator2& stencil,
                                         const InputIterator3& input,
                                         const OutputIterator& result,
                                         const BinaryPredicate& pred,
                                         const std::string& user_code,
                                         std::input_iterator_tag,
                                         std::input_iterator_tag,
                                         std::input_iterator_tag )
    {
            // TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
            // to a temporary buffer.  Should we?

            static_assert( std::is_same< InputIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };

    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator>
    void gather_detect_random_access( bolt::cl::control& ctl,
                                      const InputIterator1& map_first,
                                      const InputIterator1& map_last,
                                      const InputIterator2& input,
                                      const OutputIterator& result,
                                      const std::string& user_code,
                                      std::input_iterator_tag,
                                      std::input_iterator_tag )
    {
            static_assert( std::is_same< InputIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };


////////////////////////////////////////////////////////////////////
// Gather detect random access
////////////////////////////////////////////////////////////////////



    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void gather_detect_random_access( bolt::cl::control& ctl,
                                      const InputIterator1& map_first,
                                      const InputIterator1& map_last,
                                      const InputIterator2& input,
                                      const OutputIterator& result,
                                      const std::string& user_code,
                                      std::random_access_iterator_tag,
                                      std::random_access_iterator_tag )
    {
       gather_pick_iterator( ctl,
                             map_first,
                             map_last,
                             input,
                             result,
                             user_code,
                             typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                             typename std::iterator_traits< InputIterator2 >::iterator_category( ) );
    };


} //End of detail namespace

////////////////////////////////////////////////////////////////////
// Gather APIs
////////////////////////////////////////////////////////////////////
template< typename InputIterator1,
          typename InputIterator2,
          typename OutputIterator >
void gather( bolt::cl::control& ctl,
             InputIterator1 map_first,
             InputIterator1 map_last,
             InputIterator2 input,
             OutputIterator result,
             const std::string& user_code )
{
    detail::gather_detect_random_access( ctl,
                                         map_first,
                                         map_last,
                                         input,
                                         result,
                                         user_code,
                                         typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                                         typename std::iterator_traits< InputIterator2 >::iterator_category( ) );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename OutputIterator >
void gather( InputIterator1 map_first,
             InputIterator1 map_last,
             InputIterator2 input,
             OutputIterator result,
             const std::string& user_code )
{
    detail::gather_detect_random_access( control::getDefault( ),
                                         map_first,
                                         map_last,
                                         input,
                                         result,
                                         user_code,
                                         typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                                         typename std::iterator_traits< InputIterator2 >::iterator_category( ) );
}


////////////////////////////////////////////////////////////////////
// GatherIf APIs
////////////////////////////////////////////////////////////////////
template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator >
void gather_if( bolt::cl::control& ctl,
                InputIterator1 map_first,
                InputIterator1 map_last,
                InputIterator2 stencil,
                InputIterator3 input,
                OutputIterator result,
                const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator2>::value_type stencilType;
    gather_if( ctl,
               map_first,
               map_last,
               stencil,
               input,
               result,
               bolt::cl::identity <stencilType> ( ),
               user_code );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator >
void gather_if( InputIterator1 map_first,
                InputIterator1 map_last,
                InputIterator2 stencil,
                InputIterator3 input,
                OutputIterator result,
                const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator2>::value_type stencilType;
    gather_if( map_first,
               map_last,
               stencil,
               input,
               result,
               bolt::cl::identity <stencilType> ( ),
               user_code );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator,
          typename BinaryPredicate >
void gather_if( bolt::cl::control& ctl,
                InputIterator1 map_first,
                InputIterator1 map_last,
                InputIterator2 stencil,
                InputIterator3 input,
                OutputIterator result,
                BinaryPredicate pred,
                const std::string& user_code )
{
    detail::gather_if_detect_random_access( ctl,
                                            map_first,
                                            map_last,
                                            stencil,
                                            input,
                                            result,
                                            pred,
                                            user_code,
                                            typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                                            typename std::iterator_traits< InputIterator2 >::iterator_category( ),
                                            typename std::iterator_traits< InputIterator3 >::iterator_category( ) );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator,
          typename BinaryPredicate >
void gather_if(  InputIterator1 map_first,
                 InputIterator1 map_last,
                 InputIterator2 stencil,
                 InputIterator3 input,
                 OutputIterator result,
                 BinaryPredicate pred,
                 const std::string& user_code )
{
    detail::gather_if_detect_random_access( control::getDefault( ),
                                            map_first,
                                            map_last,
                                            stencil,
                                            input,
                                            result,
                                            pred,
                                            user_code,
                                            typename std::iterator_traits< InputIterator1 >::iterator_category( ),
                                            typename std::iterator_traits< InputIterator2 >::iterator_category( ),
                                            typename std::iterator_traits< InputIterator3 >::iterator_category( ));
}


} //End of cl namespace
} //End of bolt namespace

#endif
