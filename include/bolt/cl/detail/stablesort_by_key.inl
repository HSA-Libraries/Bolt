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
#if !defined( BOLT_CL_STABLESORT_BY_KEY_INL )
#define BOLT_CL_STABLESORT_BY_KEY_INL

#include <algorithm>
#include <type_traits>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>
#include <boost/shared_array.hpp>

#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"

#define BOLT_CL_STABLESORT_BY_KEY_CPU_THRESHOLD 64

namespace bolt {
namespace cl {
    template< typename RandomAccessIterator1, typename RandomAccessIterator2 > 
    void stable_sort_by_key( RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, const std::string& cl_code )
    {
        typedef std::iterator_traits< RandomAccessIterator1 >::value_type T;

        detail::stablesort_by_key_detect_random_access( control::getDefault( ), 
                                           keys_first, keys_last, values_first,
                                           less< T >( ), cl_code, 
                                           std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                           std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
        return;
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stable_sort_by_key( RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, StrictWeakOrdering comp, const std::string& cl_code )
    {
        detail::stablesort_by_key_detect_random_access( control::getDefault( ), 
                                           keys_first, keys_last, values_first,
                                           comp, cl_code, 
                                           std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                           std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
        return;
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2 > 
    void stable_sort_by_key( control &ctl, RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, const std::string& cl_code)
    {
        typedef std::iterator_traits< RandomAccessIterator1 >::value_type T;

        detail::stablesort_by_key_detect_random_access(ctl, 
                                           keys_first, keys_last, values_first,
                                          less< T >( ), cl_code, 
                                           std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                           std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
        return;
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stable_sort_by_key( control &ctl, RandomAccessIterator1 keys_first, RandomAccessIterator1 keys_last, 
        RandomAccessIterator2 values_first, StrictWeakOrdering comp, const std::string& cl_code )
    {
        detail::stablesort_by_key_detect_random_access(ctl, 
                                           keys_first, keys_last, values_first,
                                          comp, cl_code, 
                                           std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                           std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
        return;
    }

namespace detail
{

    enum stableSortTypes { stableSort_by_key_KeyType, stableSort_by_key_KeyIterType, stableSort_by_key_ValueType, 
        stableSort_by_key_ValueIterType, stableSort_by_key_lessFunction, stableSort_by_key_end };

    class StableSort_by_key_KernelTemplateSpecializer : public KernelTemplateSpecializer
    {
    public:
        StableSort_by_key_KernelTemplateSpecializer() : KernelTemplateSpecializer( )
        {
            addKernelName( "blockInsertionSort" );
            addKernelName( "merge" );
        }

        const ::std::string operator( ) ( const ::std::vector< ::std::string >& typeNames ) const
        {
            const std::string templateSpecializationString = 
                "template __attribute__((mangled_name(" + name( 0 ) + "Instantiated)))\n"
                "kernel void " + name( 0 ) + "Template(\n"
                "global " + typeNames[stableSort_by_key_KeyType] + "* data_ptr,\n"
                ""        + typeNames[stableSort_by_key_KeyIterType] + " data_iter,\n"
                "global " + typeNames[stableSort_by_key_ValueType] + "* value_ptr,\n"
                ""        + typeNames[stableSort_by_key_ValueIterType] + " value_iter,\n"
                "const uint vecSize,\n"
                "local "  + typeNames[stableSort_by_key_KeyType] + "* key_lds,\n"
                "local "  + typeNames[stableSort_by_key_ValueType] + "* val_lds,\n"
                "global " + typeNames[stableSort_by_key_lessFunction] + " * lessOp\n"
                ");\n\n"

                "template __attribute__((mangled_name(" + name( 1 ) + "Instantiated)))\n"
                "kernel void " + name( 1 ) + "Template(\n"
                "global " + typeNames[stableSort_by_key_KeyType] + "* iKey_ptr,\n"
                ""        + typeNames[stableSort_by_key_KeyIterType] + " iKey_iter,\n"
                "global " + typeNames[stableSort_by_key_ValueType] + "* iValue_ptr,\n"
                ""        + typeNames[stableSort_by_key_ValueIterType] + " iValue_iter,\n"
                "global " + typeNames[stableSort_by_key_KeyType] + "* oKey_ptr,\n"
                ""        + typeNames[stableSort_by_key_KeyIterType] + " oKey_iter,\n"
                "global " + typeNames[stableSort_by_key_ValueType] + "* oValue_ptr,\n"
                ""        + typeNames[stableSort_by_key_ValueIterType] + " oValue_iter,\n"
                "const uint srcVecSize,\n"
                "const uint srcBlockSize,\n"
                "local "  + typeNames[stableSort_by_key_KeyType] + "* key_lds,\n"
                "local "  + typeNames[stableSort_by_key_ValueType] + "* val_lds,\n"
                "global " + typeNames[stableSort_by_key_lessFunction] + " * lessOp\n"
                ");\n\n";

            return templateSpecializationString;
        }
    };

// Wrapper that uses default control class, iterator interface
    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_detect_random_access( control &ctl, 
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last, 
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code, 
                                    std::input_iterator_tag, std::input_iterator_tag )
    {
        //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
        //  to a temporary buffer.  Should we?
        static_assert( false, "Bolt only supports random access iterator types" );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_detect_random_access( control &ctl, 
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last, 
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code, 
                                    std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        return stablesort_by_key_pick_iterator( ctl, keys_first, keys_last, values_first,
                                    comp, cl_code, 
                                    std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                    std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_detect_random_access( control &ctl, 
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last, 
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code, 
                                    bolt::cl::fancy_iterator_tag, std::input_iterator_tag ) 
    {
        static_assert( false, "It is not possible to sort fancy iterators. They are not mutable" );
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_detect_random_access( control &ctl, 
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last, 
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code, 
                                    std::input_iterator_tag, bolt::cl::fancy_iterator_tag ) 
    {
        static_assert( false, "It is not possible to sort fancy iterators. They are not mutable" );
    }

    //Non Device Vector specialization.
    //This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
    //In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_pick_iterator( control &ctl, 
                                const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last, 
                                const RandomAccessIterator2 values_first,
                                const StrictWeakOrdering& comp, const std::string& cl_code, 
                                std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keyType;
        typedef typename std::iterator_traits< RandomAccessIterator2 >::value_type valType;

        size_t vecSize = std::distance( keys_first, keys_last ); 
        if( vecSize < 2 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();

        if( runMode == bolt::cl::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }

        if( runMode == bolt::cl::control::SerialCpu )
        {
            throw ::cl::Error( CL_INVALID_OPERATION, "stable_sort_by_key not implemented yet for SerialCPU" );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
            throw ::cl::Error( CL_INVALID_OPERATION, "stable_sort_by_key not implemented yet for MultiCoreCpu" );
            return;
        } 
        else 
        {
            device_vector< keyType > dvKeys( keys_first, keys_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
            device_vector< valType > dvValues( values_first, vecSize, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, false, ctl );

            //Now call the actual cl algorithm
            stablesort_by_key_enqueue( ctl, dvKeys.begin(), dvKeys.end(), dvValues.begin( ), comp, cl_code );

            //Map the buffer back to the host
            dvKeys.data( );
            dvValues.data( );
            return;
        }
    }

    //Device Vector specialization
    template< typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_pick_iterator( control &ctl, 
                                    const DVRandomAccessIterator1 keys_first, const DVRandomAccessIterator1 keys_last, 
                                    const DVRandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code, 
                                    bolt::cl::device_vector_tag, bolt::cl::device_vector_tag )
    {
        typedef typename std::iterator_traits< DVRandomAccessIterator1 >::value_type keyType;

        size_t vecSize = std::distance( first, last ); 
        if( vecSize < 2 )
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();

        if( runMode == bolt::cl::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }

        if( runMode == bolt::cl::control::SerialCpu )
        {
            throw ::cl::Error( CL_INVALID_OPERATION, "stable_sort_by_key not implemented yet for SerialCPU" );
            return;
        }
        else if( runMode == bolt::cl::control::MultiCoreCpu )
        {
            throw ::cl::Error( CL_INVALID_OPERATION, "stable_sort_by_key not implemented yet for MultiCoreCpu" );
            return;
        } 
        else
        {
            stablesort_by_key_enqueue( ctl, keys_first, keys_last, values_first, comp, cl_code );
        }

        return;
    }

    template< typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering >
    void stablesort_by_key_enqueue( control& ctrl, 
                                    const DVRandomAccessIterator1 keys_first, const DVRandomAccessIterator1 keys_last, 
                                    const DVRandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code )
    {
        cl_int l_Error;
        cl_uint vecSize = static_cast< cl_uint >( std::distance( keys_first, keys_last ) );

        /**********************************************************************************
         * Type Names - used in KernelTemplateSpecializer
         *********************************************************************************/
        typedef std::iterator_traits< DVRandomAccessIterator1 >::value_type keyType;
        typedef std::iterator_traits< DVRandomAccessIterator2 >::value_type valueType;

        std::vector<std::string> typeNames( stableSort_by_key_end );
        typeNames[stableSort_by_key_KeyType] = TypeName< keyType >::get( );
        typeNames[stableSort_by_key_ValueType] = TypeName< valueType >::get( );
        typeNames[stableSort_by_key_KeyIterType] = TypeName< DVRandomAccessIterator1 >::get( );
        typeNames[stableSort_by_key_ValueIterType] = TypeName< DVRandomAccessIterator2 >::get( );
        typeNames[stableSort_by_key_lessFunction] = TypeName< StrictWeakOrdering >::get( );

        /**********************************************************************************
         * Type Definitions - directrly concatenated into kernel string
         *********************************************************************************/
        std::vector< std::string > typeDefinitions;
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< keyType >::get( ) )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< valueType >::get( ) )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator1 >::get( ) )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator2 >::get( ) )
        PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get( ) )

        /**********************************************************************************
         * Compile Options
         *********************************************************************************/
        bool cpuDevice = ctrl.getDevice( ).getInfo< CL_DEVICE_TYPE >( ) == CL_DEVICE_TYPE_CPU;
        //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
        //const size_t kernel0_localRange = ( cpuDevice ) ? 1 : localRange*4;
        //std::ostringstream oss;
        //oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_localRange;

        //oss << " -DUSE_AMD_HSA=" << USE_AMD_HSA;
        //compileOptions = oss.str();

        /**********************************************************************************
         * Request Compiled Kernels
         *********************************************************************************/
        std::string compileOptions;

        StableSort_by_key_KernelTemplateSpecializer ss_kts;
        std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
            ctrl,
            typeNames,
            &ss_kts,
            typeDefinitions,
            stablesort_by_key_kernels,
            compileOptions );
        // kernels returned in same order as added in KernelTemplaceSpecializer constructor

        size_t localRange  = kernels[ 0 ].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctrl.getDevice( ), &l_Error );
        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );


        //  Make sure that globalRange is a multiple of localRange
        size_t globalRange = vecSize;
        size_t modlocalRange = ( globalRange & ( localRange-1 ) );
        if( modlocalRange )
        {
            globalRange &= ~modlocalRange;
            globalRange += localRange;
        }

        ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
        control::buffPointer userFunctor = ctrl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, &aligned_comp );

        //  kernels[ 0 ] sorts values within a workgroup, in parallel across the entire vector
        //  kernels[ 0 ] reads and writes to the same vector
        cl_uint keyLdsSize  = static_cast< cl_uint >( localRange * sizeof( keyType ) );
        cl_uint valueLdsSize  = static_cast< cl_uint >( localRange * sizeof( valueType ) );
        V_OPENCL( kernels[ 0 ].setArg( 0, keys_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
        V_OPENCL( kernels[ 0 ].setArg( 1, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ), "Error setting a kernel argument" );
        V_OPENCL( kernels[ 0 ].setArg( 2, values_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
        V_OPENCL( kernels[ 0 ].setArg( 3, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ), "Error setting a kernel argument" );
        V_OPENCL( kernels[ 0 ].setArg( 4, vecSize ),            "Error setting argument for kernels[ 0 ]" ); // Size of scratch buffer
        V_OPENCL( kernels[ 0 ].setArg( 5, keyLdsSize, NULL ),          "Error setting argument for kernels[ 0 ]" ); // Scratch buffer
        V_OPENCL( kernels[ 0 ].setArg( 6, valueLdsSize, NULL ),          "Error setting argument for kernels[ 0 ]" ); // Scratch buffer
        V_OPENCL( kernels[ 0 ].setArg( 7, *userFunctor ),           "Error setting argument for kernels[ 0 ]" ); // User provided functor class

        ::cl::CommandQueue& myCQ = ctrl.getCommandQueue( );

        ::cl::Event blockSortEvent;
        l_Error = myCQ.enqueueNDRangeKernel( kernels[ 0 ], ::cl::NullRange,
                ::cl::NDRange( globalRange ), ::cl::NDRange( localRange ), NULL, &blockSortEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

        //  Early exit for the case of no merge passes, values are already in destination vector
        if( vecSize <= localRange )
        {
            wait( ctrl, blockSortEvent );
            return;
        };

        //  An odd number of elements requires an extra merge pass to sort
        size_t numMerges = 0;

        //  Calculate the log2 of vecSize, taking into account our block size from kernel 1 is 64
        //  this is how many merge passes we want
        size_t log2BlockSize = vecSize >> 6;
        for( ; log2BlockSize > 1; log2BlockSize >>= 1 )
        {
            ++numMerges;
        }

        //  Check to see if the input vector size is a power of 2, if not we will need last merge pass
        size_t vecPow2 = (vecSize & (vecSize-1));
        numMerges += vecPow2? 1: 0;

        //{
        //    device_vector< keyType >::pointer myKeys = keys_first.getContainer( ).data( );
        //    device_vector< valueType >::pointer myValues = values_first.getContainer( ).data( );
        //}

        //  Allocate a flipflop buffer because the merge passes are out of place
        control::buffPointer tmpKeyBuffer = ctrl.acquireBuffer( globalRange * sizeof( keyType ) );
        control::buffPointer tmpValueBuffer = ctrl.acquireBuffer( globalRange * sizeof( valueType ) );
        V_OPENCL( kernels[ 1 ].setArg( 8, vecSize ),            "Error setting argument for kernels[ 0 ]" ); // Size of scratch buffer
        V_OPENCL( kernels[ 1 ].setArg( 10, keyLdsSize, NULL ),          "Error setting argument for kernels[ 0 ]" ); // Scratch buffer
        V_OPENCL( kernels[ 1 ].setArg( 11, valueLdsSize, NULL ),          "Error setting argument for kernels[ 0 ]" ); // Scratch buffer
        V_OPENCL( kernels[ 1 ].setArg( 12, *userFunctor ),           "Error setting argument for kernels[ 0 ]" ); // User provided functor class

        ::cl::Event kernelEvent;
        for( size_t pass = 1; pass <= numMerges; ++pass )
        {
            //  For each pass, flip the input-output buffers 
            if( pass & 0x1 )
            {
                V_OPENCL( kernels[ 1 ].setArg( 0, keys_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 1, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 2, values_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 3, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 4, *tmpKeyBuffer ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 5, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 6, *tmpValueBuffer ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 7, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ), "Error setting a kernel argument" );
            }
            else
            {
                V_OPENCL( kernels[ 1 ].setArg( 0, *tmpKeyBuffer ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 1, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 2, *tmpValueBuffer ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 3, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 4, keys_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 5, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ), "Error setting a kernel argument" );
                V_OPENCL( kernels[ 1 ].setArg( 6, values_first.getBuffer( ) ),    "Error setting argument for kernels[ 0 ]" ); // Input buffer
                V_OPENCL( kernels[ 1 ].setArg( 7, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ), "Error setting a kernel argument" );
            }
            //  For each pass, the merge window doubles
            unsigned srcLogicalBlockSize = static_cast< unsigned >( localRange << (pass-1) );
            V_OPENCL( kernels[ 1 ].setArg( 9, static_cast< unsigned >( srcLogicalBlockSize ) ),            "Error setting argument for kernels[ 0 ]" ); // Size of scratch buffer

            if( pass == numMerges )
            {
                //  Grab the event to wait on from the last enqueue call
                l_Error = myCQ.enqueueNDRangeKernel( kernels[ 1 ], ::cl::NullRange, ::cl::NDRange( globalRange ),
                        ::cl::NDRange( localRange ), NULL, &kernelEvent );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for mergeTemplate kernel" );
            }
            else
            {
                l_Error = myCQ.enqueueNDRangeKernel( kernels[ 1 ], ::cl::NullRange, ::cl::NDRange( globalRange ),
                        ::cl::NDRange( localRange ), NULL, NULL );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for mergeTemplate kernel" );
            }

            ///  Debug code to examine input/output buffers on every pass
            //myCQ.finish( );
            //std::cout << "BlockSize[ " << srcLogicalBlockSize << " ] " << std::endl;
            //if( pass & 0x1 )
            //{
            //    //  Debug mapping, to look at result vector in memory
            //    keyType* dev2Host = static_cast< keyType* >( myCQ.enqueueMapBuffer( *tmpBuffer, CL_TRUE, CL_MAP_READ, 0,
            //        globalRange * sizeof( keyType ), NULL, NULL, &l_Error) );
            //    V_OPENCL( l_Error, "Error: Mapping Device->Host Buffer." );

            //    for( unsigned i = 0; i < globalRange/srcLogicalBlockSize; ++i )
            //    {
            //        unsigned blockIndex = i * srcLogicalBlockSize;
            //        unsigned endIndex = bolt::cl::minimum< unsigned >()( blockIndex+(srcLogicalBlockSize-1), vecSize ) - 1;
            //        for( unsigned j = blockIndex; j < endIndex; ++j )
            //        {
            //            if( !(comp)( dev2Host[ j ], dev2Host[ j + 1 ] ) && (dev2Host[ j ] != dev2Host[ j + 1 ]) )
            //            {
            //                std::cout << " Element[ " << j+1 << " ] out of sequence Block[ " << i << " ] Index[ " << j+1-blockIndex << " ]" << std::endl;
            //            }
            //        }
            //    }

            //    myCQ.enqueueUnmapMemObject( *tmpBuffer, dev2Host );
            //}
            //else
            //{
            //    //  Debug mapping, to look at result vector in memory
            //    keyType* dev2Host = static_cast< keyType* >( myCQ.enqueueMapBuffer( first.getBuffer( ), CL_TRUE, CL_MAP_READ, 0,
            //        bolt::cl::minimum< size_t >()( globalRange, vecSize ) * sizeof( keyType ), NULL, NULL, &l_Error) );
            //    V_OPENCL( l_Error, "Error: Mapping Device->Host Buffer." );

            //    for( unsigned i = 0; i < globalRange/srcLogicalBlockSize; ++i )
            //    {
            //        unsigned blockIndex = i * srcLogicalBlockSize;
            //        unsigned endIndex = bolt::cl::minimum< unsigned >()( blockIndex+(srcLogicalBlockSize-1), vecSize ) - 1;
            //        for( unsigned j = blockIndex; j < endIndex; ++j )
            //        {
            //            if( !(comp)( dev2Host[ j ], dev2Host[ j + 1 ] ) && (dev2Host[ j ] != dev2Host[ j + 1 ]) )
            //            {
            //                std::cout << "Block[ " << i << " ] Element[ " << j << " ] out of sequence" << std::endl;
            //            }
            //        }
            //    }

            //    myCQ.enqueueUnmapMemObject( first.getBuffer( ), dev2Host );
            //}
            //std::cout << std::endl;
        }

        //  If there are an odd number of merges, then the output data is sitting in the temp buffer.  We need to copy
        //  the results back into the input array
        if( numMerges & 1 )
        {
            ::cl::Event copyEvent;

            l_Error = myCQ.enqueueCopyBuffer( *tmpKeyBuffer, keys_first.getBuffer( ), 0, keys_first.m_Index * sizeof( keyType ), 
                vecSize * sizeof( keyType ), NULL, NULL );
            V_OPENCL( l_Error, "device_vector failed to copy data inside of operator=()" );

            l_Error = myCQ.enqueueCopyBuffer( *tmpValueBuffer, values_first.getBuffer( ), 0, values_first.m_Index * sizeof( keyType ), 
                vecSize * sizeof( keyType ), NULL, &copyEvent );
            V_OPENCL( l_Error, "device_vector failed to copy data inside of operator=()" );

            wait( ctrl, copyEvent );
        }
        else
        {
            wait( ctrl, kernelEvent );
        }

        return;
    }// END of sort_enqueue

}//namespace bolt::cl::detail
}//namespace bolt::cl
}//namespace bolt

#endif
