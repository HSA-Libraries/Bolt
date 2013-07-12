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

//template < >
//struct TypeName<bolt::cl::plus<int>>
//{
//    static std::string get()
//    {
//        return "hi";
//    }
//};

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
                                          std::iterator_traits< InputIterator1 >::iterator_category( ),
                                          std::iterator_traits< InputIterator2 >::iterator_category( ) );
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
                                          std::iterator_traits< InputIterator1 >::iterator_category( ),
                                          std::iterator_traits< InputIterator2 >::iterator_category( ) );
}


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
    typedef typename std::iterator_traits<InputIterator3>::value_type stencilType;
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
    typedef typename std::iterator_traits<InputIterator3>::value_type stencilType;
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
                                             std::iterator_traits< InputIterator1 >::iterator_category( ),
                                             std::iterator_traits< InputIterator2 >::iterator_category( ),
                                             std::iterator_traits< InputIterator3 >::iterator_category( ) );
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
                                             std::iterator_traits< InputIterator1 >::iterator_category( ),
                                             std::iterator_traits< InputIterator2 >::iterator_category( ),
                                             std::iterator_traits< InputIterator3 >::iterator_category( ));
}



namespace detail {

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

    const ::std::string operator() ( const ::std::vector<::std::string>& scatterIfKernels ) const
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
                                          const InputIterator2& stencil,
                                          const OutputIterator& result,
                                          const Predicate& pred,
                                          const std::string& user_code,
                                          std::input_iterator_tag,
                                          std::input_iterator_tag,
                                          std::input_iterator_tag )
    {
            // TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
            // to a temporary buffer.  Should we?

            static_assert( false, "Bolt only supports random access iterator types" );
    };

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
                                 std::iterator_traits< InputIterator1 >::iterator_category( ),
                                 std::iterator_traits< InputIterator2 >::iterator_category( ),
                                 std::iterator_traits< InputIterator3 >::iterator_category( ) );
    };

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
        typedef std::iterator_traits<InputIterator1>::value_type iType1;
        typedef std::iterator_traits<InputIterator2>::value_type iType2;
        typedef std::iterator_traits<InputIterator3>::value_type iType3;
        typedef std::iterator_traits<OutputIterator>::value_type oType;

        size_t sz = std::distance( first1, last1 );

        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
        if( runMode == bolt::cl::control::SerialCpu )
        {
            // Call serial code
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )
            // Call MultiCore CPU code
#else
          throw std::exception( "The MultiCoreCpu version of scatter_if is not enabled to be built! \n" );

#endif
          return;
        }
        else
        {
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
        typedef std::iterator_traits<InputIterator1>::value_type iType1;
        typedef std::iterator_traits<InputIterator2>::value_type iType2;
        typedef std::iterator_traits<InputIterator3>::value_type iType3;
        typedef std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( first1, last1 );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
          runMode = ctl.getDefaultPathToRun();
        }
        if( runMode == bolt::cl::control::SerialCpu )
        {
            // Call serial scatter_if
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )

            // Call MC scatter_if

#else
            throw std::exception( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
            device_vector< iType2 > dvMap( map, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );

            // Map the output iterator to a device_vector
            device_vector< oType > dvResult( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

            scatter_if_enqueue( ctl,
                                dvInput.begin( ),
                                dvInput.end( ),
                                dvMap,
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
                                  const InputIterator2& stencil,
                                  const OutputIterator& result,
                                  const Predicate& pred,
                                  const std::string& user_code,
                                  bolt::cl::fancy_iterator_tag,
                                  std::random_access_iterator_tag,
                                  std::random_access_iterator_tag )
    {

        typedef std::iterator_traits<InputIterator1>::value_type iType1;
        typedef std::iterator_traits<InputIterator2>::value_type iType2;
        typedef std::iterator_traits<InputIterator2>::value_type iType3;
        typedef std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( fancyIterfirst, fancyIterlast );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
           runMode = ctl.getDefaultPathToRun();
        }
        if( runMode == bolt::cl::control::SerialCpu )
        {
            // Call serial
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
#if defined( ENABLE_TBB )

            // Call MC

#else
            throw std::exception( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );

#endif
            return;
        }
        else
        {
            // Use host pointers memory since these arrays are only read once - no benefit to copying.
            // Map the input iterator to a device_vector
            device_vector< iType2 > dvMap( first2, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );
            device_vector< iType3 > dvStencil( first2, sz, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

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

        typedef std::iterator_traits< DVInputIterator1 >::value_type iType1;
        typedef std::iterator_traits< DVInputIterator2 >::value_type iType2;
        typedef std::iterator_traits< DVInputIterator3 >::value_type iType3;
        typedef std::iterator_traits< DVOutputIterator >::value_type oType;

        size_t sz = std::distance( first1, last1 );
        if( sz == 0 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
        if(runMode == bolt::cl::control::Automatic)
        {
             runMode = ctl.getDefaultPathToRun();
        }

        if( runMode == bolt::cl::control::SerialCpu )
        {
            // Call Serial
            bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            bolt::cl::device_vector< iType2 >::pointer secPtr =  first2.getContainer( ).data( );
            bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

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
            // Call MC 
#if defined( ENABLE_TBB )

            bolt::cl::device_vector< iType1 >::pointer firstPtr =  first1.getContainer( ).data( );
            bolt::cl::device_vector< iType2 >::pointer secPtr =  first2.getContainer( ).data( );
            bolt::cl::device_vector< oType >::pointer resPtr =  result.getContainer( ).data( );

            bolt::btbb::transform( &firstPtr[ first1.m_Index ], &firstPtr[ last1.m_Index ],
                                   &secPtr[ first2.m_Index ],&resPtr[ result.m_Index ],f );


#else
             throw std::exception( "The MultiCoreCpu version of scatter is not enabled to be built! \n" );
#endif
            return;
        }
        else
        {
            scatter_if_enqueue( ctl, first1, last1, map, stencil, result, pred, user_code );
        }
    }


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
        typedef std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef std::iterator_traits<DVInputIterator3>::value_type iType3;
        typedef std::iterator_traits<DVOutputIterator>::value_type oType;

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

        kernels[boundsCheck].setArg( 0, first1.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 1, first1.gpuPayloadSize( ), &first1.gpuPayload( ) );
        kernels[boundsCheck].setArg( 2, map.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 3, map.gpuPayloadSize( ), &map.gpuPayload( ) );
        kernels[boundsCheck].setArg( 4, stencil.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 5, stencil.gpuPayloadSize( ), &stencil.gpuPayload( ) );
        kernels[boundsCheck].setArg( 6, result.getContainer().getBuffer() );
        kernels[boundsCheck].setArg( 7, result.gpuPayloadSize( ), &result.gpuPayload( ) );
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


} //End of detail namespace
} //End of cl namespace
} //End of bolt namespace

#endif
