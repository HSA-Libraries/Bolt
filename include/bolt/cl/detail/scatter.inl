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
#if !defined( BOLT_CL_SCATTER_INL )
#define BOLT_CL_SCATTER_INL
#define WAVEFRONT_SIZE 64

#include <type_traits>

#ifdef ENABLE_TBB
    #include "bolt/btbb/scatter.h"
#endif

#include "bolt/cl/bolt.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/iterator/iterator_traits.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/clcode.h"

namespace bolt {
namespace cl {

namespace detail {


/* Begin-- Serial Implementation of the scatter and scatter_if routines */

template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator>

void gold_scatter_enqueue (InputIterator1 first1,
                           InputIterator1 last1,
                           InputIterator2 map,
                           OutputIterator result)
    {
       size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );

       for(int iter = 0; iter<numElements; iter++)
                *(result+*(map + iter)) = *(first1 + iter);
    }

 template< typename InputIterator1,
           typename InputIterator2,
           typename InputIterator3,
           typename OutputIterator >
void gold_scatter_if_enqueue (InputIterator1 first1,
                              InputIterator1 last1,
                              InputIterator2 map,
                              InputIterator3 stencil,
                              OutputIterator result)
   {
       size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
       for(int iter = 0; iter<numElements; iter++)
        {
             if(stencil[iter] == 1)
                  result[*(map+(iter - 0))] = first1[iter];
             }
   }

 template< typename InputIterator1,
           typename InputIterator2,
           typename InputIterator3,
           typename OutputIterator,
           typename Predicate>
void gold_scatter_if_enqueue (InputIterator1 first1,
                              InputIterator1 last1,
                              InputIterator2 map,
                              InputIterator3 stencil,
                              OutputIterator result,
                              Predicate pred)
   {
       size_t numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
       for(int iter = 0; iter<numElements; iter++)
        {
             if(pred(stencil[iter]) != 0)
                  result[*(map+(iter))] = first1[iter];
             }
   }

 /* End-- Serial Implementation of the scatter and scatter_if routines */


////////////////////////////////////////////////////////////////////
// ScatterIf KTS
////////////////////////////////////////////////////////////////////

  enum ScatterIfTypes { scatter_if_iType, scatter_if_DVInputIterator,
                        scatter_if_mapType, scatter_if_DVMapType,
                        scatter_if_stencilType, scatter_if_DVStencilType,
                        scatter_if_resultType, scatter_if_DVResultType,
                        scatter_if_Predicate, scatter_if_endB };

class ScatterIf_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    ScatterIf_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
       addKernelName("scatterIfTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& scatterIfKernels ) const
    {
      const std::string templateSpecializationString =
        "// Host generates this instantiation string with user-specified value type and functor\n"
        "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
        "kernel void "+name(0)+"(\n"
        "global " + scatterIfKernels[scatter_if_iType] + "* input, \n"
        + scatterIfKernels[scatter_if_DVInputIterator] + " inputIter, \n"
        "global " + scatterIfKernels[scatter_if_mapType] + "* map, \n"
        + scatterIfKernels[scatter_if_DVMapType] + " mapIter, \n"
        "global " + scatterIfKernels[scatter_if_stencilType] + "* stencil, \n"
        + scatterIfKernels[scatter_if_DVStencilType] + " stencilIter, \n"
        "global " + scatterIfKernels[scatter_if_resultType] + "* result, \n"
        + scatterIfKernels[scatter_if_DVResultType] + " resultIter, \n"
        "const uint length, \n"
        "global " + scatterIfKernels[scatter_if_Predicate] + "* functor);\n\n";

        return templateSpecializationString;
    }
};

////////////////////////////////////////////////////////////////////
// Scatter KTS
////////////////////////////////////////////////////////////////////

  enum ScatterTypes { scatter_iType, scatter_DVInputIterator,
                      scatter_mapType, scatter_DVMapType,
                      scatter_resultType, scatter_DVResultType,
                      scatter_endB };

class ScatterKernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    ScatterKernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
       addKernelName("scatterTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& scatterKernels ) const
    {
      const std::string templateSpecializationString =
        "// Host generates this instantiation string with user-specified value type and functor\n"
        "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
        "kernel void "+name(0)+"(\n"
        "global " + scatterKernels[scatter_iType] + "* input, \n"
        + scatterKernels[scatter_DVInputIterator] + " inputIter, \n"
        "global " + scatterKernels[scatter_mapType] + "* map, \n"
        + scatterKernels[scatter_DVMapType] + " mapIter, \n"
        "global " + scatterKernels[scatter_resultType] + "* result, \n"
        + scatterKernels[scatter_DVResultType] + " resultIter, \n"
        "const uint length ); \n";

        return templateSpecializationString;
    }
};

////////////////////////////////////////////////////////////////////
// ScatterIf enqueue
////////////////////////////////////////////////////////////////////

    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVInputIterator3,
              typename DVOutputIterator,
              typename Predicate >
    void scatter_if_enqueue( bolt::cl::control &ctl,
                             const DVInputIterator1& first1,
                             const DVInputIterator1& last1,
                             const DVInputIterator2& map,
                             const DVInputIterator3& stencil,
                             const DVOutputIterator& result,
                             const Predicate& pred,
                             const std::string& cl_code )
    {
        typedef typename std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<DVInputIterator3>::value_type iType3;
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
        std::vector<std::string> scatterIfKernels(scatter_if_endB);
        scatterIfKernels[scatter_if_iType] = TypeName< iType1 >::get( );
        scatterIfKernels[scatter_if_mapType] = TypeName< iType2 >::get( );
        scatterIfKernels[scatter_if_stencilType] = TypeName< iType3 >::get( );
        scatterIfKernels[scatter_if_DVInputIterator] = TypeName< DVInputIterator1 >::get( );
        scatterIfKernels[scatter_if_DVMapType] = TypeName< DVInputIterator2 >::get( );
        scatterIfKernels[scatter_if_DVStencilType] = TypeName< DVInputIterator3 >::get( );
        scatterIfKernels[scatter_if_resultType] = TypeName< oType >::get( );
        scatterIfKernels[scatter_if_DVResultType] = TypeName< DVOutputIterator >::get( );
        scatterIfKernels[scatter_if_Predicate] = TypeName< Predicate >::get();

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
       /* else
        {
            boundsCheck = 1;
        }
        if (wgMultiple/wgSize < numWorkGroups)
            numWorkGroups = wgMultiple/wgSize;*/

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
         ScatterIf_KernelTemplateSpecializer s_if_kts;
         std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
             ctl,
             scatterIfKernels,
             &s_if_kts,
             typeDefinitions,
             scatter_kernels,
             compileOptions);
         // kernels returned in same order as added in KernelTemplaceSpecializer constructor

        ALIGNED( 256 ) Predicate aligned_binary( pred );
        control::buffPointer userPredicate = ctl.acquireBuffer( sizeof( aligned_binary ),
                                                                CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY,
                                                                &aligned_binary );

        typename DVInputIterator1::Payload first1_payload = first1.gpuPayload( );
        typename DVInputIterator2::Payload map_payload = map.gpuPayload( );
        typename DVInputIterator3::Payload stencil_payload = stencil.gpuPayload( );
        typename DVOutputIterator::Payload result_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg( 0, first1.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, first1.gpuPayloadSize( ),&first1_payload);
        kernels[boundsCheck].setArg( 2, map.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, map.gpuPayloadSize( ),&map_payload );
        kernels[boundsCheck].setArg( 4, stencil.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, stencil.gpuPayloadSize( ),&stencil_payload  );
        kernels[boundsCheck].setArg( 6, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 7, result.gpuPayloadSize( ),&result_payload );
        kernels[boundsCheck].setArg( 8, distVec );
        kernels[boundsCheck].setArg( 9, *userPredicate );

        ::cl::Event scatterIfEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
            kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange(wgMultiple), // numWorkGroups*wgSize
            ::cl::NDRange(wgSize),
            NULL,
            &scatterIfEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for scatter_if() kernel" );

        ::bolt::cl::wait(ctl, scatterIfEvent);

    };

////////////////////////////////////////////////////////////////////
// Scatter enqueue
////////////////////////////////////////////////////////////////////
    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVOutputIterator >
    void scatter_enqueue( bolt::cl::control &ctl,
                          const DVInputIterator1& first1,
                          const DVInputIterator1& last1,
                          const DVInputIterator2& map,
                          const DVOutputIterator& result,
                          const std::string& cl_code )
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
        std::vector<std::string> scatterKernels(scatter_endB);
        scatterKernels[scatter_iType] = TypeName< iType1 >::get( );
        scatterKernels[scatter_mapType] = TypeName< iType2 >::get( );
        scatterKernels[scatter_DVInputIterator] = TypeName< DVInputIterator1 >::get( );
        scatterKernels[scatter_DVMapType] = TypeName< DVInputIterator2 >::get( );
        scatterKernels[scatter_resultType] = TypeName< oType >::get( );
        scatterKernels[scatter_DVResultType] = TypeName< DVOutputIterator >::get( );

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
        /*else
        {
            boundsCheck = 1;
        }
        if (wgMultiple/wgSize < numWorkGroups)
            numWorkGroups = wgMultiple/wgSize;*/

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
         ScatterKernelTemplateSpecializer s_kts;
         std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
             ctl,
             scatterKernels,
             &s_kts,
             typeDefinitions,
             scatter_kernels,
             compileOptions);
         // kernels returned in same order as added in KernelTemplaceSpecializer constructor
        typename DVInputIterator1::Payload first11_payload = first1.gpuPayload( );
        typename DVInputIterator2::Payload map1_payload = map.gpuPayload( ) ;
        typename DVOutputIterator::Payload result1_payload = result.gpuPayload( );

        kernels[boundsCheck].setArg( 0, first1.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, first1.gpuPayloadSize( ),&first11_payload );
        kernels[boundsCheck].setArg( 2, map.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, map.gpuPayloadSize( ), &map1_payload);
        kernels[boundsCheck].setArg( 4, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, result.gpuPayloadSize( ),&result1_payload );
        kernels[boundsCheck].setArg( 6, distVec );

        ::cl::Event scatterEvent;
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
            kernels[boundsCheck],
            ::cl::NullRange,
            ::cl::NDRange(wgMultiple), // numWorkGroups*wgSize
            ::cl::NDRange(wgSize),
            NULL,
            &scatterEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for scatter_if() kernel" );

        ::bolt::cl::wait(ctl, scatterEvent);

    };

////////////////////////////////////////////////////////////////////
// Enqueue ends
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// ScatterIf pick iterator
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
              typename Predicate >
    void scatter_if_pick_iterator( bolt::cl::control &ctl,
                                   const InputIterator1& first1,
                                   const InputIterator1& last1,
                                   const InputIterator2& map,
                                   const InputIterator3& stencil,
                                   const OutputIterator& result,
                                   const Predicate& pred,
                                   const std::string& user_code,
                                   std::random_access_iterator_tag,
                                   std::random_access_iterator_tag,
                                   std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter_If::SERIAL_CPU");
            #endif
            gold_scatter_if_enqueue(first1, last1, map, stencil, result, pred);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
           #if defined(BOLT_DEBUG_LOG)
           dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter_If::MULTICORE_CPU");
           #endif
           bolt::btbb::scatter_if(first1, last1, map, stencil, result, pred);
#else
          throw std::runtime_error( "The MultiCoreCpu version of scatter_if is not enabled to be built! \n" );

#endif

        }
        else
        {
		  #if defined(BOLT_DEBUG_LOG)
          dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter_If::OPENCL_GPU");
          #endif
          // Map the input iterator to a device_vector
          device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
          device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );
          device_vector< iType3 > dvStencil( stencil, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

          // Map the output iterator to a device_vector
          device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );
          scatter_if_enqueue( ctl,
                              dvInput.begin( ),
                              dvInput.end( ),
                              dvMap.begin( ),
                              dvStencil.begin( ),
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
              typename Predicate >
    void scatter_if_pick_iterator( bolt::cl::control &ctl,
                                   const InputIterator1& first1,
                                   const InputIterator1& last1,
                                   const InputIterator2& map,
                                   const InputIterator3& stencilFancyIter,
                                   const OutputIterator& result,
                                   const Predicate& pred,
                                   const std::string& user_code,
                                   std::random_access_iterator_tag,
                                   std::random_access_iterator_tag,
                                   bolt::cl::fancy_iterator_tag )
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter_If::SERIAL_CPU");
            #endif
            gold_scatter_if_enqueue(first1, last1, map, stencilFancyIter, result, pred);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter_If::MULTICORE_CPU");
            #endif
            bolt::btbb::scatter_if(first1, last1, map, stencilFancyIter, result, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter_If::OPENCL_GPU");
            #endif
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true,ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_if_enqueue( ctl,
                                dvInput.begin( ),
                                dvInput.end( ),
                                dvMap.begin(),
                                stencilFancyIter,
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
              typename Predicate >
    void scatter_if_pick_iterator( bolt::cl::control &ctl,
                                  const InputIterator1& fancyIterfirst,
                                  const InputIterator1& fancyIterlast,
                                  const InputIterator2& map,
                                  const InputIterator3& stencil,
                                  const OutputIterator& result,
                                  const Predicate& pred,
                                  const std::string& user_code,
                                  bolt::cl::fancy_iterator_tag,
                                  std::random_access_iterator_tag,
                                  std::random_access_iterator_tag )
    {

        typedef typename std::iterator_traits<InputIterator1>::value_type iType1;
        typedef typename std::iterator_traits<InputIterator2>::value_type iType2;
        typedef typename std::iterator_traits<InputIterator3>::value_type iType3;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter_If::SERIAL_CPU");
            #endif
            gold_scatter_if_enqueue(fancyIterfirst, fancyIterlast, map, stencil, result, pred);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter_If::MULTICORE_CPU");
            #endif
            bolt::btbb::scatter_if(fancyIterfirst, fancyIterlast, map, stencil, result, pred);
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter_If::OPENCL_GPU");
            #endif
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );
            device_vector< iType3 > dvStencil( stencil, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_if_enqueue( ctl,
                               fancyIterfirst,
                               fancyIterlast,
                               dvMap.begin( ),
                               dvStencil.begin( ),
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
              typename Predicate >
    void scatter_if_pick_iterator( bolt::cl::control &ctl,
                                   const DVInputIterator1& first1,
                                   const DVInputIterator1& last1,
                                   const DVInputIterator2& map,
                                   const DVInputIterator3& stencil,
                                   const DVOutputIterator& result,
                                   const Predicate& pred,
                                   const std::string& user_code,
                                   bolt::cl::device_vector_tag,
                                   bolt::cl::device_vector_tag,
                                   bolt::cl::device_vector_tag )
    {

        typedef typename std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef typename std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef typename std::iterator_traits< DVInputIterator3 >::value_type iType3;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter_If::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< iType2 >::pointer mapPtr =  map.getContainer( ).data( );
            typename bolt::cl::device_vector< iType3 >::pointer stenPtr =  stencil.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

#if defined( _WIN32 )
            gold_scatter_if_enqueue(&firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ], &mapPtr[ map.m_Index ],
                 &stenPtr[ stencil.m_Index ], stdext::make_checked_array_iterator( &resPtr[ result.m_Index ], sz ), pred );

#else
            gold_scatter_if_enqueue( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                &mapPtr[ map.m_Index ], &stenPtr[ stencil.m_Index ], &resPtr[ result.m_Index ], pred );
#endif
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
		   	
            // Call MC
#if defined( ENABLE_TBB )
            {
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter_If::MULTICORE_CPU");
                #endif
			
                typename bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
                typename bolt::cl::device_vector< iType2 >::pointer mapPtr =  map.getContainer( ).data( );
                typename bolt::cl::device_vector< iType3 >::pointer stenPtr =  stencil.getContainer( ).data( );
                typename bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

                bolt::btbb::scatter_if( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                &mapPtr[ map.m_Index ], &stenPtr[ stencil.m_Index ], &resPtr[ result.m_Index ], pred );
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter_If::OPENCL_GPU");
            #endif
			
            scatter_if_enqueue( ctl, first1, last1, map, stencil, result, pred, user_code );
        }
    }

////////////////////////////////////////////////////////////////////
// Scatter pick iterator
////////////////////////////////////////////////////////////////////

// Host vectors

    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const InputIterator1& first1,
                                const InputIterator1& last1,
                                const InputIterator2& map,
                                const OutputIterator& result,
                                const std::string& user_code,
                                std::random_access_iterator_tag,
                                std::random_access_iterator_tag )
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
            gold_scatter_enqueue(first1, last1, map, result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
            #if defined( ENABLE_TBB )
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
                #endif
                bolt::btbb::scatter(first1, last1, map, result);
            #else
                 throw std::runtime_error( "The MultiCoreCpu version of scatter_if is not enabled to be built! \n" );

            #endif
        }

        else
        {
		  #if defined(BOLT_DEBUG_LOG)
          dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
          #endif
			
          // Map the input iterator to a device_vector
          device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
          device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

          // Map the output iterator to a device_vector
          device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );
          scatter_enqueue( ctl,
                           dvInput.begin( ),
                           dvInput.end( ),
                           dvMap.begin( ),
                           dvResult.begin( ),
                           user_code );

          // This should immediately map/unmap the buffer
          dvResult.data( );
        }
    }

// Input is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const InputIterator1& firstFancy,
                                const InputIterator1& lastFancy,
                                const InputIterator2& map,
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
             dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
             #endif
             gold_scatter_enqueue(firstFancy, lastFancy, map, result);

        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
            #endif
            bolt::btbb::scatter(firstFancy, lastFancy, map, result);
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
		  
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,true, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_enqueue( ctl,
                                firstFancy,
                                lastFancy,
                                dvMap.begin(),
                                dvResult.begin( ),
                                user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }


// Map is a fancy iterator
    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator>
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const InputIterator1& first1,
                                const InputIterator1& last1,
                                const InputIterator2& mapFancy,
                                const OutputIterator& result,
                                const std::string& user_code,
                                std::random_access_iterator_tag,
                                 bolt::cl::fancy_iterator_tag )
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
            gold_scatter_enqueue(first1, last1, mapFancy, result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
            #endif
            bolt::btbb::scatter(first1, last1, mapFancy, result);
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_enqueue( ctl,
                                dvInput.begin( ),
                                dvInput.end( ),
                                mapFancy,
                                dvResult.begin( ),
                                user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// Device Vectors
    template< typename DVInputIterator1,
              typename DVInputIterator2,
              typename DVOutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const DVInputIterator1& first1,
                                const DVInputIterator1& last1,
                                const DVInputIterator2& map,
                                const DVOutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::device_vector_tag,
                                bolt::cl::device_vector_tag )
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer InputBuffer  =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< iType2 >::pointer MapBuffer    =  map.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
            gold_scatter_enqueue(&InputBuffer[ first1.m_Index ], &InputBuffer[ last1.m_Index ], &MapBuffer[ map.m_Index ],
            &ResultBuffer[ result.m_Index ]);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {

#if defined( ENABLE_TBB )
            {
			  #if defined(BOLT_DEBUG_LOG)
              dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
              #endif
              typename bolt::cl::device_vector< iType1 >::pointer InputBuffer   =  first1.getContainer( ).data( );
              typename bolt::cl::device_vector< iType2 >::pointer MapBuffer     =  map.getContainer( ).data( );
              typename bolt::cl::device_vector< oType >::pointer ResultBuffer   =  result.getContainer( ).data( );
                bolt::btbb::scatter(&InputBuffer[ first1.m_Index ], &InputBuffer[ last1.m_Index ], &MapBuffer[ map.m_Index ],
                &ResultBuffer[ result.m_Index ]);
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );
#endif
         }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
			
            scatter_enqueue( ctl,
                             first1,
                             last1,
                             map,
                             result,
                             user_code );
        }
    }

// Input fancy ; Map DV
    template< typename FancyIterator,
              typename DVMapIterator,
              typename DVOutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const FancyIterator& firstFancy,
                                const FancyIterator& lastFancy,
                                const DVMapIterator& map,
                                const DVOutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::fancy_iterator_tag,
                                bolt::cl::device_vector_tag )
    {

        typedef typename std::iterator_traits< FancyIterator >::value_type iType1;
        typedef typename std::iterator_traits< DVMapIterator >::value_type iType2;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType2 >::pointer MapBuffer    =  map.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
            gold_scatter_enqueue(firstFancy, lastFancy, &MapBuffer[ map.m_Index ], &ResultBuffer[ result.m_Index ]);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
                #endif
			  
                typename bolt::cl::device_vector< iType2 >::pointer MapBuffer =  map.getContainer( ).data( );
                typename bolt::cl::device_vector< oType >::pointer ResultBuffer =  result.getContainer( ).data( );

                bolt::btbb::scatter(firstFancy, lastFancy, &MapBuffer[ map.m_Index ],&ResultBuffer[ result.m_Index ]);
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
			
            scatter_enqueue( ctl,
                             firstFancy,
                             lastFancy,
                             map,
                             result,
                             user_code );
        }
    }

// Map fancy ; Input DV
    template< typename DVInputIterator,
              typename FancyMapIterator,
              typename DVOutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const DVInputIterator& first1,
                                const DVInputIterator& last1,
                                const FancyMapIterator& mapFancy,
                                const DVOutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::device_vector_tag,
                                bolt::cl::fancy_iterator_tag )
    {
        typedef typename std::iterator_traits< DVInputIterator >::value_type iType1;
        typedef typename std::iterator_traits< FancyMapIterator >::value_type iType2;
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
			
            typename bolt::cl::device_vector< iType1 >::pointer InputBuffer    =  first1.getContainer( ).data( );
            typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
            gold_scatter_enqueue( &InputBuffer[ first1.m_Index ], &InputBuffer[ last1.m_Index ], mapFancy,
                                                                         &ResultBuffer[ result.m_Index ]);

        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
                #endif
				
                typename bolt::cl::device_vector< iType1 >::pointer InputBuffer    =  first1.getContainer( ).data( );
                typename bolt::cl::device_vector< oType >::pointer  ResultBuffer =  result.getContainer( ).data( );
                bolt::btbb::scatter( &InputBuffer[ first1.m_Index ], &InputBuffer[ last1.m_Index ], mapFancy,
                                                                            &ResultBuffer[ result.m_Index ]);
            }
#else
             throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );
#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
			
            scatter_enqueue( ctl,
                             first1,
                             last1,
                             mapFancy,
                             result,
                             user_code );
        }
    }

// Input DV ; Map random access
    template< typename DVInputIterator,
              typename MapIterator,
              typename OutputIterator >
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const DVInputIterator& first,
                                const DVInputIterator& last,
                                const MapIterator& map,
                                const OutputIterator& result,
                                const std::string& user_code,
                                bolt::cl::device_vector_tag,
                                std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits<DVInputIterator>::value_type iType1;
        typedef typename std::iterator_traits<MapIterator>::value_type iType2;
        typedef typename std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( first, last );
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
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
            #endif
            typename bolt::cl::device_vector< iType1 >::pointer InputBuffer    =  first.getContainer( ).data( );
            gold_scatter_enqueue( &InputBuffer[ first.m_Index ], &InputBuffer[ last.m_Index ], map,result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
                #endif
                typename bolt::cl::device_vector< iType1 >::pointer InputBuffer    =  first.getContainer( ).data( );
                bolt::btbb::scatter( &InputBuffer[ first.m_Index ], &InputBuffer[ last.m_Index ],map, result);
            }
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
        }
        else
        {
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
			
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the map iterator to a device_vector
            device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );
            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_enqueue( ctl,
                             first,
                             last,
                             dvMap.begin(),
                             dvResult.begin( ),
                             user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }

// DV Map ; RA Input
    template< typename InputIterator,
              typename DVMapIterator,
              typename OutputIterator>
    void scatter_pick_iterator( bolt::cl::control &ctl,
                                const InputIterator& first1,
                                const InputIterator& last1,
                                const DVMapIterator& map,
                                const OutputIterator& result,
                                const std::string& user_code,
                                std::random_access_iterator_tag,
                                bolt::cl::device_vector_tag )
    {
        typedef typename std::iterator_traits<InputIterator>::value_type iType1;
        typedef typename std::iterator_traits<DVMapIterator>::value_type iType2;
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
           dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_SERIAL_CPU,"::Scatter::SERIAL_CPU");
           #endif
           typename bolt::cl::device_vector< iType2 >::pointer mapBuffer    =  map.getContainer( ).data( );
           gold_scatter_enqueue(first1, last1, &mapBuffer[ map.m_Index ], result);
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            {
			    #if defined(BOLT_DEBUG_LOG)
                dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_MULTICORE_CPU,"::Scatter::MULTICORE_CPU");
                #endif
                typename bolt::cl::device_vector< iType2 >::pointer mapBuffer    =  map.getContainer( ).data( );
                bolt::btbb::scatter(first1, last1, &mapBuffer[ map.m_Index ], result);
            }
#else
            throw std::runtime_error( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
        }
        else
        {
		     #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_SCATTER,BOLTLOG::BOLT_OPENCL_GPU,"::Scatter::OPENCL_GPU");
            #endif
				
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            // Map the result iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_enqueue( ctl,
                             dvInput.begin(),
                             dvInput.end(),
                             map,
                             dvResult.begin( ),
                             user_code );

            // This should immediately map/unmap the buffer
            dvResult.data( );
        }
    }






////////////////////////////////////////////////////////////////////
// ScatterIf detect random access
////////////////////////////////////////////////////////////////////


    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename Predicate >
    void scatter_if_detect_random_access( bolt::cl::control& ctl,
                                          const InputIterator1& first1,
                                          const InputIterator1& last1,
                                          const InputIterator2& map,
                                          const InputIterator3& stencil,
                                          const OutputIterator& result,
                                          const Predicate& pred,
                                          const std::string& user_code,
                                          std::random_access_iterator_tag,
                                          std::random_access_iterator_tag,
                                          std::random_access_iterator_tag )
    {
       scatter_if_pick_iterator( ctl,
                                 first1,
                                 last1,
                                 map,
                                 stencil,
                                 result,
                                 pred,
                                 user_code,
                                 typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                                 typename  std::iterator_traits< InputIterator2 >::iterator_category( ),
                                 typename  std::iterator_traits< InputIterator3 >::iterator_category( ));
    };






////////////////////////////////////////////////////////////////////
// Scatter detect random access
////////////////////////////////////////////////////////////////////



    template< typename InputIterator1,
              typename InputIterator2,
              typename OutputIterator >
    void scatter_detect_random_access( bolt::cl::control& ctl,
                                       const InputIterator1& first1,
                                       const InputIterator1& last1,
                                       const InputIterator2& map,
                                       const OutputIterator& result,
                                       const std::string& user_code,
                                       std::random_access_iterator_tag,
                                       std::random_access_iterator_tag )
    {
       scatter_pick_iterator( ctl,
                              first1,
                              last1,
                              map,
                              result,
                              user_code,
                              typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                              typename  std::iterator_traits< InputIterator2 >::iterator_category( ) );
    };


    // Wrapper that uses default ::bolt::cl::control class, iterator interface
    template< typename InputIterator1,
              typename InputIterator2,
              typename InputIterator3,
              typename OutputIterator,
              typename Predicate >
    void scatter_if_detect_random_access( bolt::cl::control& ctl,
                                          const InputIterator1& first1,
                                          const InputIterator1& last1,
                                          const InputIterator2& map,
                                          const InputIterator3& stencil,
                                          const OutputIterator& result,
                                          const Predicate& pred,
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
    void scatter_detect_random_access( bolt::cl::control& ctl,
                                       const InputIterator1& first1,
                                       const InputIterator1& last1,
                                       const InputIterator2& map,
                                       const OutputIterator& result,
                                       const std::string& user_code,
                                       std::input_iterator_tag,
                                       std::input_iterator_tag )
    {
            static_assert( std::is_same< InputIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };

} //End of detail namespace

////////////////////////////////////////////////////////////////////
// Scatter APIs
////////////////////////////////////////////////////////////////////
template< typename InputIterator1,
          typename InputIterator2,
          typename OutputIterator >
void scatter( bolt::cl::control& ctl,
              InputIterator1 first1,
              InputIterator1 last1,
              InputIterator2 map,
              OutputIterator result,
              const std::string& user_code )
{
    detail::scatter_detect_random_access( ctl,
                                          first1,
                                          last1,
                                          map,
                                          result,
                                          user_code,
                                          typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                                          typename  std::iterator_traits< InputIterator2 >::iterator_category( ) );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename OutputIterator >
void scatter( InputIterator1 first1,
              InputIterator1 last1,
              InputIterator2 map,
              OutputIterator result,
              const std::string& user_code )
{
    detail::scatter_detect_random_access( control::getDefault( ),
                                          first1,
                                          last1,
                                          map,
                                          result,
                                          user_code,
                                          typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                                          typename  std::iterator_traits< InputIterator2 >::iterator_category( ) );
}


////////////////////////////////////////////////////////////////////
// ScatterIf APIs
////////////////////////////////////////////////////////////////////
template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator >
void scatter_if( bolt::cl::control& ctl,
                 InputIterator1 first1,
                 InputIterator1 last1,
                 InputIterator2 map,
                 InputIterator3 stencil,
                 OutputIterator result,
                 const std::string& user_code )
{
    typedef typename  std::iterator_traits<InputIterator3>::value_type stencilType;
    scatter_if( ctl,
                first1,
                last1,
                map,
                stencil,
                result,
                bolt::cl::identity <stencilType> ( ),
                user_code );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator >
void scatter_if( InputIterator1 first1,
                 InputIterator1 last1,
                 InputIterator2 map,
                 InputIterator3 stencil,
                 OutputIterator result,
                 const std::string& user_code )
{
    typedef typename  std::iterator_traits<InputIterator3>::value_type stencilType;
    scatter_if( first1,
                last1,
                map,
                stencil,
                result,
                bolt::cl::identity <stencilType> ( ),
                user_code );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator,
          typename Predicate >
void scatter_if( bolt::cl::control& ctl,
                 InputIterator1 first1,
                 InputIterator1 last1,
                 InputIterator2 map,
                 InputIterator3 stencil,
                 OutputIterator result,
                 Predicate pred,
                 const std::string& user_code )
{
    detail::scatter_if_detect_random_access( ctl,
                                             first1,
                                             last1,
                                             map,
                                             stencil,
                                             result,
                                             pred,
                                             user_code,
                                             typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                                             typename  std::iterator_traits< InputIterator2 >::iterator_category( ),
                                             typename  std::iterator_traits< InputIterator3 >::iterator_category( ) );
}

template< typename InputIterator1,
          typename InputIterator2,
          typename InputIterator3,
          typename OutputIterator,
          typename Predicate >
void scatter_if( InputIterator1 first1,
                 InputIterator1 last1,
                 InputIterator2 map,
                 InputIterator3 stencil,
                 OutputIterator result,
                 Predicate pred,
                 const std::string& user_code )
{
    detail::scatter_if_detect_random_access( control::getDefault( ),
                                             first1,
                                             last1,
                                             map,
                                             stencil,
                                             result,
                                             pred,
                                             user_code,
                                             typename  std::iterator_traits< InputIterator1 >::iterator_category( ),
                                             typename  std::iterator_traits< InputIterator2 >::iterator_category( ),
                                             typename  std::iterator_traits< InputIterator3 >::iterator_category( ));
}



} //End of cl namespace
} //End of bolt namespace

#endif
