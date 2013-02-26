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

#pragma once
#if !defined( BOLT_CL_STABLESORT_INL )
#define BOLT_CL_STABLESORT_INL

#include <algorithm>
#include <type_traits>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>
#include <boost/shared_array.hpp>

#if defined( ENABLE_TBB )
    #include "tbb/parallel_for.h"
#endif

#define CL_VERSION_1_2 1
#include "bolt/cl/bolt.h"
#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"

#define BOLT_UINT_MAX 0xFFFFFFFFU
#define BOLT_UINT_MIN 0x0U
#define BOLT_INT_MAX 0x7FFFFFFFU
#define BOLT_INT_MIN 0x80000000U

#define WGSIZE 64

namespace bolt {
namespace cl {
template<typename RandomAccessIterator> 
void stable_sort(RandomAccessIterator first, 
          RandomAccessIterator last, 
          const std::string& cl_code)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;

    detail::stablesort_detect_random_access( control::getDefault( ), 
                                       first, last, 
                                       less< T >( ), cl_code, 
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering> 
void stable_sort(RandomAccessIterator first, 
          RandomAccessIterator last,  
          StrictWeakOrdering comp, 
          const std::string& cl_code)  
{
    detail::stablesort_detect_random_access( control::getDefault( ), 
                                       first, last, 
                                       comp, cl_code, 
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator> 
void stable_sort(control &ctl,
          RandomAccessIterator first, 
          RandomAccessIterator last, 
          const std::string& cl_code)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;

    detail::stablesort_detect_random_access(ctl, 
                                      first, last, 
                                      less< T >( ), cl_code, 
                                      std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering> 
void stable_sort(control &ctl,
          RandomAccessIterator first, 
          RandomAccessIterator last,  
          StrictWeakOrdering comp, 
          const std::string& cl_code)  
{
    detail::stablesort_detect_random_access(ctl, 
                                      first, last, 
                                      comp, cl_code, 
                                      std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}
}
};


namespace bolt {
namespace cl {
namespace detail {

struct CallCompiler_StableSort {
    static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code_dataType, 
                                std::string valueTypeName,  std::string compareTypeName, const control *ctl) 
    {

        const std::string instantiationString = 
            "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(stableSortInstantiated)))\n"
            "kernel void stableSortTemplate(\n"
            "global " + valueTypeName + "* A,\n"
            "const uint stage,\n"
            "const uint passOfStage,\n"
            "global " + compareTypeName + " * userComp\n"
            ");\n\n";

        bolt::cl::constructAndCompileString( masterKernel, "stableSort", 
                                            stablesort_kernels, instantiationString, 
                                            cl_code_dataType, valueTypeName, "", *ctl);
    }

    static void constructAndCompileSelectionStableSort(std::vector< ::cl::Kernel >* sortKernels,  
                                                 std::string cl_code_dataType, std::string valueTypeName,  
                                                 std::string compareTypeName, const control *ctl)
    {
        std::vector< const std::string > kernelNames;
        kernelNames.push_back( "selectionSortLocal" );
        kernelNames.push_back( "selectionSortFinal" );

        const std::string instantiationString = 
            "\n// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + kernelNames[0] + "Instantiated)))\n"
            "kernel void selectionSortLocalTemplate(\n"
            "global const " + valueTypeName + " * in,\n"
            "global " + valueTypeName + " * out,\n"
            "global " + compareTypeName + " * userComp,\n"
            "local  " + valueTypeName + " * scratch,\n"
            "const int buffSize\n"
            ");\n\n"
            "\n// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + kernelNames[1] + "Instantiated)))\n"
            "kernel void selectionSortFinalTemplate(\n"
            "global const " + valueTypeName + " * in,\n"
            "global " + valueTypeName + " * out,\n"
            "global " + compareTypeName + " * userComp,\n"
            "local  " + valueTypeName + " * scratch,\n"
            "const int buffSize\n"
            ");\n\n";

        bolt::cl::compileKernelsString( *sortKernels, kernelNames, 
                                        stablesort_kernels, instantiationString, 
                                        cl_code_dataType, valueTypeName, "", *ctl );
    }
}; //End of struct CallCompiler_Sort  

// Wrapper that uses default control class, iterator interface
template<typename RandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_detect_random_access( control &ctl, 
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, const std::string& cl_code, 
                                std::input_iterator_tag )
{
    //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template<typename RandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_detect_random_access( control &ctl, 
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, const std::string& cl_code, 
                                std::random_access_iterator_tag )
{
    return stablesort_pick_iterator(ctl, first, last, 
                              comp, cl_code, 
                              std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
};

//Device Vector specialization
template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_pick_iterator( control &ctl, 
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, const std::string& cl_code, 
                         bolt::cl::fancy_iterator_tag ) 
{
    static_assert( false, "It is not possible to sort fancy iterators. They are not mutable" );
}

#if defined( ENABLE_TBB )

    //  This is a parallel implementation of a merge with tbb acceleration.  It takes two input ranges as input,
    //  which it assumes are sorted.  The two input ranges can be from the same container, but must not alias.
    //  The output range must be out-of-place, or this merge will fail.  The size of the output range is infered 
    //  as the total size of the input ranges.
    //  Good references for how parallel merging works can be found at:
    //  http://software.intel.com/sites/products/documentation/doclib/tbb_sa/help/reference/algorithms/parallel_for_func.htm
    //  http://www.drdobbs.com/article/print?articleId=229204454&siteSectionName=parallel
    template< class tbbInputIterator1, class tbbInputIterator2, class tbbOutputIterator, class tbbCompare > 
    struct outOfPlaceMergeRange
    {
        tbbInputIterator1 first1, last1;
        tbbInputIterator2 first2, last2;
        tbbOutputIterator result;
        tbbCompare comp;
        static const size_t divSize = 256;
        //static const size_t divSize = 4;

        bool empty( ) const
        {
            return (std::distance( first1, last1 ) == 0) && (std::distance( first2, last2 ) == 0);
        }

        bool is_divisible( ) const
        {
            bolt::cl::minimum< size_t > myMin;
            return myMin( std::distance( first1, last1 ), std::distance( first2, last2 )) > divSize;
        }

        outOfPlaceMergeRange( tbbInputIterator1 begin1, tbbInputIterator1 end1, tbbInputIterator2 begin2, tbbInputIterator2 end2,
            tbbOutputIterator out, tbbCompare func ):
            first1( begin1 ), last1( end1 ), first2( begin2 ), last2( end2 ), result( out ), comp( func )
        {
        }

        outOfPlaceMergeRange( outOfPlaceMergeRange& r, tbb::split ): first1( r.first1 ), last1( r.last1 ), first2( r.first2 ), last2( r.last2 ),
             result( r.result ), comp( r.comp )
        {
            size_t length1 = std::distance( r.first1, r.last1 );

            tbbInputIterator1 mid1 = r.first1 + (length1 >> 1);
            tbbInputIterator2 mid2 = std::lower_bound( r.first2, r.last2, *mid1, comp );

            first1 = mid1;
            first2 = mid2;
            last1 = r.last1;
            last2 = r.last2;
            r.last1 = mid1;
            r.last2 = mid2;

            result = r.result + std::distance( r.first1, mid1 ) + std::distance( r.first2, mid2 );
        }
    };

    template< class tbbInputIterator1, class tbbInputIterator2, class tbbOutputIterator, class tbbCompare >
    struct outOfPlaceMergeRangeBody
    {
        void operator( )( outOfPlaceMergeRange< tbbInputIterator1, tbbInputIterator2, tbbOutputIterator, tbbCompare >& r ) const
        {
#if defined( _WIN32 )
            std::merge( r.first1, r.last1, r.first2, r.last2, stdext::make_unchecked_array_iterator( r.result ), r.comp );
#else
            std::merge( r.first1, r.last1, r.first2, r.last2, r.result, r.comp );
#endif
        }
    };

    //  This is the parallel implementation of a stable_sort implemented with tbb acceleration.  The stable sort 
    //  is recursive, first sorting the left half of the input array, then sorting the right half.  Once two
    //  halves are sorted, the recursion stack pops once and the two halves are merged together.  This recursive
    //  routine has no parallelism, but relies on a parallel merge implemented in TBB to use multiple cores.
    //  A good reference for this algorithm can be found at
    //  http://www.drdobbs.com/article/print?articleId=229400239&siteSectionName=parallel
    template< typename RandomAccessIterator, typename StrictWeakOrdering > 
    void tbbParallelStableSort( RandomAccessIterator srcLeft, RandomAccessIterator srcRight, RandomAccessIterator dstLeft, RandomAccessIterator dstRight, const StrictWeakOrdering& comp, bool dstWrite = true )
    {
        if( std::distance( srcLeft, srcRight ) < outOfPlaceMergeRange< RandomAccessIterator, RandomAccessIterator, RandomAccessIterator, StrictWeakOrdering >::divSize )
        {
            std::stable_sort( srcLeft, srcRight, comp );
            if( dstWrite )
            {
                std::copy( srcLeft, srcRight, dstLeft );
            }
            return;
        }

        size_t srcLength = std::distance( srcLeft, srcRight );
        size_t dstLength = std::distance( dstLeft, dstRight );
        RandomAccessIterator midSrc = srcLeft + (srcLength >> 1);
        RandomAccessIterator midDst = dstLeft + (dstLength >> 1);

        //  TODO: Should the left and right recursive calls themselves be parallelized with tbb::parallel_invoke?
        //  This might oversubscribe threads, as the merge operation is already multi-threaded
        // recurse left half
        tbbParallelStableSort( srcLeft, midSrc, dstLeft, midDst, comp, !dstWrite );

        // recurse right half
        tbbParallelStableSort( midSrc, srcRight, midDst, dstRight, comp, !dstWrite );

        if( dstWrite )
        {
            tbb::parallel_for( 
                outOfPlaceMergeRange< RandomAccessIterator, RandomAccessIterator, RandomAccessIterator, StrictWeakOrdering >( srcLeft, midSrc, midSrc, srcRight, dstLeft, comp ),
                outOfPlaceMergeRangeBody< RandomAccessIterator, RandomAccessIterator, RandomAccessIterator, StrictWeakOrdering >( ),
                tbb::simple_partitioner( )
            );
        }
        else
        {
            tbb::parallel_for( 
                outOfPlaceMergeRange< RandomAccessIterator, RandomAccessIterator, RandomAccessIterator, StrictWeakOrdering >( dstLeft, midDst, midDst, dstRight, srcLeft, comp ),
                outOfPlaceMergeRangeBody< RandomAccessIterator, RandomAccessIterator, RandomAccessIterator, StrictWeakOrdering >( ),
                tbb::simple_partitioner( )
            );
        }
    }

#endif

//Non Device Vector specialization.
//This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
//In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
template< typename RandomAccessIterator, typename StrictWeakOrdering > 
void stablesort_pick_iterator( control &ctl, const RandomAccessIterator& first, const RandomAccessIterator& last,
                            const StrictWeakOrdering& comp, const std::string& cl_code, 
                            std::random_access_iterator_tag )
{
    typedef typename std::iterator_traits< RandomAccessIterator >::value_type Type;

    size_t vecSize = std::distance( first, last ); 
    if( vecSize == 0 )
        return;

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();

    if ((runMode == bolt::cl::control::SerialCpu) )
    {
        std::stable_sort( first, last, comp );
        return;
    }
    else if (runMode == bolt::cl::control::MultiCoreCpu)
    {
#if defined( ENABLE_TBB )
        std::vector< Type > scratchBuffer( vecSize );

        tbbParallelStableSort( first, last, scratchBuffer.begin( ), scratchBuffer.end( ), comp, false );
#else
        std::stable_sort( first, last, comp );
#endif
        return;
    } 
    else 
    {
        device_vector< Type > dvInputOutput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        //Now call the actual cl algorithm
        stablesort_enqueue(ctl,dvInputOutput.begin(),dvInputOutput.end(),comp,cl_code);
        //Map the buffer back to the host
        dvInputOutput.data( );
        return;
    }
}

//Device Vector specialization
template< typename DVRandomAccessIterator, typename StrictWeakOrdering > 
void stablesort_pick_iterator( control &ctl, 
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, const std::string& cl_code, 
                         bolt::cl::device_vector_tag ) 
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type Type;

    size_t vecSize = std::distance( first, last ); 
    if( vecSize == 0 )
        return;

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();

    if (runMode == bolt::cl::control::SerialCpu)
    {
        bolt::cl::device_vector< Type >::pointer firstPtr =  first.getContainer( ).data( );

        std::stable_sort( &firstPtr[ first.m_Index ], &firstPtr[ vecSize-1 ], comp );
        return;
    }
    else if (runMode == bolt::cl::control::MultiCoreCpu)
    {
#if defined( ENABLE_TBB )
        bolt::cl::device_vector< Type >::pointer firstPtr =  first.getContainer( ).data( );
        std::vector< Type > scratchBuffer( vecSize );

        tbbParallelStableSort( &firstPtr[ first.m_Index ], &firstPtr[ vecSize ], 
            &*scratchBuffer.begin( ), &*scratchBuffer.begin( )+vecSize, comp, false );
#else
        std::stable_sort( &firstPtr[ first.m_Index ], &firstPtr[ vecSize ], comp );
#endif
        return;
    } 
    else 
    {
        stablesort_enqueue(ctl,first,last,comp,cl_code);
    }

    return;
}

template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_enqueue(control &ctl, 
             const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
             const StrictWeakOrdering& comp, const std::string& cl_code)  
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    cl_int l_Error;
    size_t szElements = (size_t)(last - first);
    if(((szElements-1) & (szElements)) != 0)
    {
        stablesort_enqueue_non_powerOf2(ctl,first,last,comp,cl_code);
        return;
    }
    static  boost::once_flag initOnlyOnce;
    static  ::cl::Kernel masterKernel;

    size_t temp;

    //Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class - 
    // Note use of TypeName<>::get to retreive the name here.
    if (boost::is_same<T, StrictWeakOrdering>::value) 
        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_StableSort::constructAndCompile, &masterKernel, 
                      "\n//--User Code\n" + cl_code + 
                      "\n//--typedef T Code\n" + ClCode<T>::get(),
                      TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );
    else
        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_StableSort::constructAndCompile, &masterKernel, 
                      "\n//--User Code\n" + cl_code + 
                      "\n//--typedef T Code\n" + ClCode<T>::get() + 
                      "\n//--typedef StrictWeakOrdering Code\n" + ClCode<StrictWeakOrdering>::get(), 
                      TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );

    //std::cout << "I am executing a bitonic sort" << std::endl;

    size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
    if((szElements/2) < wgSize)
    {
        wgSize = (int)szElements/2;
    }
    unsigned int numStages,stage,passOfStage;

    ::cl::Buffer A = first.getBuffer( );
    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
    // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof( aligned_comp ), &aligned_comp );   // Create buffer wrapper so we can access host parameters.
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

    ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.
    numStages = 0;
    for(temp = szElements; temp > 1; temp >>= 1)
        ++numStages;
    V_OPENCL( k.setArg(0, A), "Error setting a kernel argument" );
    V_OPENCL( k.setArg(3, *userFunctor), "Error setting a kernel argument" );
    for(stage = 0; stage < numStages; ++stage) 
    {
        // stage of the algorithm
        V_OPENCL( k.setArg(1, stage), "Error setting a kernel argument" );
        // Every stage has stage + 1 passes
        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
            // pass of the current stage
            V_OPENCL( k.setArg(2, passOfStage), "Error setting a kernel argument" );
            /* 
                * Enqueue a kernel run call.
                * Each thread writes a sorted pair.
                * So, the number of  threads (global) should be half the length of the input buffer.
                */
            l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    k, 
                    ::cl::NullRange,
                    ::cl::NDRange(szElements/2),
                    ::cl::NDRange(wgSize),
                    NULL,
                    NULL);
            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
            V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
        }//end of for passStage = 0:stage-1
    }//end of for stage = 0:numStage-1
    //Map the buffer back to the host
    ctl.commandQueue().enqueueMapBuffer(A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements, NULL, NULL, &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
    return;
}// END of sort_enqueue

template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
void stablesort_enqueue_non_powerOf2(control &ctl, 
                               const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                               const StrictWeakOrdering& comp, const std::string& cl_code)  
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    static boost::once_flag initOnlyOnce;
    cl_int l_Error;
    size_t szElements = (size_t)(last - first);

    // Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class - 
    // Note use of TypeName<>::get to retreive the name here.
    static std::vector< ::cl::Kernel > sortKernels;

    if (boost::is_same<T, StrictWeakOrdering>::value) 
        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_StableSort::constructAndCompileSelectionStableSort, &sortKernels, 
                          "\n//--User Code\n" + cl_code + 
                          "\n//--typedef T Code\n" + ClCode<T>::get(), 
                          TypeName<T>::get(), 
                          TypeName<StrictWeakOrdering>::get(), &ctl) );
        else
        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_StableSort::constructAndCompileSelectionStableSort, &sortKernels, 
                          "\n//--User Code\n" + cl_code + 
                          "\n//--typedef T Code\n" + ClCode<T>::get() + 
                          "\n//--typedef StrictWeakOrdering Code\n" + ClCode<StrictWeakOrdering>::get(), 
                          TypeName<T>::get(), 
                          TypeName<StrictWeakOrdering>::get(), &ctl) );

    size_t wgSize  = sortKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    
    size_t totalWorkGroups = (szElements + wgSize)/wgSize;
    size_t globalSize = totalWorkGroups * wgSize;
    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    
    const ::cl::Buffer& in = first.getBuffer( );
    // ::cl::Buffer out(ctl.context(), CL_MEM_READ_WRITE, sizeof(T)*szElements);
    control::buffPointer out = ctl.acquireBuffer( sizeof(T)*szElements );

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
    // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof( aligned_comp ), &aligned_comp );
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), 
                                                          CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

    ::cl::LocalSpaceArg loc;
    loc.size_ = wgSize*sizeof(T);
    
    V_OPENCL( sortKernels[0].setArg(0, in), "Error setting a kernel argument in" );
    V_OPENCL( sortKernels[0].setArg(1, *out), "Error setting a kernel argument out" );
    V_OPENCL( sortKernels[0].setArg(2, *userFunctor), "Error setting a kernel argument userFunctor" );
    V_OPENCL( sortKernels[0].setArg(3, loc), "Error setting kernel argument loc" );
    V_OPENCL( sortKernels[0].setArg(4, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                        sortKernels[0], 
                                        ::cl::NullRange,
                                        ::cl::NDRange(globalSize),
                                        ::cl::NDRange(wgSize),
                                        NULL,
                                        NULL);
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
        V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
    }

    wgSize  = sortKernels[1].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );

    V_OPENCL( sortKernels[1].setArg(0, *out), "Error setting a kernel argument in" );
    V_OPENCL( sortKernels[1].setArg(1, in), "Error setting a kernel argument out" );
    V_OPENCL( sortKernels[1].setArg(2, *userFunctor), "Error setting a kernel argument userFunctor" );
    V_OPENCL( sortKernels[1].setArg(3, loc), "Error setting kernel argument loc" );
    V_OPENCL( sortKernels[1].setArg(4, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                        sortKernels[1],
                                        ::cl::NullRange,
                                        ::cl::NDRange(globalSize),
                                        ::cl::NDRange(wgSize),
                                        NULL,
                                        NULL);
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
        V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
    }
    // Map the buffer back to the host
    ctl.commandQueue().enqueueMapBuffer(in, true, 
                                        CL_MAP_READ | CL_MAP_WRITE, 
                                        0/*offset*/, sizeof(T) * szElements, 
                                        NULL, NULL, &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );

    return;
}// END of sort_enqueue_non_powerOf2

}//namespace bolt::cl::detail
}//namespace bolt::cl
}//namespace bolt

#endif
