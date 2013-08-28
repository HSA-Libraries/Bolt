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
#if !defined( BOLT_CL_SORT_BY_KEY_INL )
#define BOLT_CL_SORT_BY_KEY_INL

#include <algorithm>
#include <type_traits>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>

#include "bolt/cl/bolt.h"
#include "bolt/cl/stablesort_by_key.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"

#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/sort_by_key.h"
#endif

#define BITONIC_SORT_WGSIZE 64
#define DEBUG 1
namespace bolt {
    namespace cl {

namespace detail {

enum sortByKeyTypes {sort_by_key_keyValueType, sort_by_key_keyIterType,
                     sort_by_key_valueValueType, sort_by_key_valueIterType,
                     sort_by_key_StrictWeakOrdering, sort_by_key_end };

class BitonicSortByKey_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    BitonicSortByKey_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("BitonicSortByKeyTemplate");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =

            "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "kernel void BitonicSortByKeyTemplate(\n"
            "global " + typeNames[sort_by_key_keyValueType] + "* A,\n"
            ""        + typeNames[sort_by_key_keyIterType]  + " input_iter,\n"
            "global " + typeNames[sort_by_key_valueValueType] + "* values_ptr,\n"
            ""        + typeNames[sort_by_key_valueIterType] + " values_iter,\n"
            "const uint stage,\n"
            "const uint passOfStage,\n"
            "global " + typeNames[sort_by_key_StrictWeakOrdering] + " * userComp\n"
            ");\n\n";
            return templateSpecializationString;
        }
};

    //Serial CPU code path implementation.
    //Class to hold the key value pair. This will be used to zip th ekey and value together in a vector.
    template <typename keyType, typename valueType>
    class std_sort
    {
    public:
        keyType   key;
        valueType value;
    };
    //This is the functor which will sort the std_sort vector.
    template <typename keyType, typename valueType, typename StrictWeakOrdering>
    class std_sort_comp
    {
    public:
        typedef std_sort<keyType, valueType> KeyValueType;
        std_sort_comp(const StrictWeakOrdering &_swo):swo(_swo)
        {}
        StrictWeakOrdering swo;
        bool operator() (const KeyValueType &lhs, const KeyValueType &rhs) const
        {
            return swo(lhs.key, rhs.key);
        }
    };

    //The serial CPU implementation of sort_by_key routine. This routines zips the key value pair and then sorts
    //using the std::sort routine.
    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void serialCPU_sort_by_key(const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                      const RandomAccessIterator2 values_first,
                                      const StrictWeakOrdering& comp)
    {
        typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keyType;
        typedef typename std::iterator_traits< RandomAccessIterator2 >::value_type valType;
        typedef std_sort<keyType, valType> KeyValuePair;
        typedef std_sort_comp<keyType, valType, StrictWeakOrdering> KeyValuePairFunctor;

        size_t vecSize = std::distance( keys_first, keys_last );
        std::vector<KeyValuePair> KeyValuePairVector(vecSize);
        KeyValuePairFunctor functor(comp);
        //Zip the key and values iterators into a std_sort vector.
        for (size_t i=0; i< vecSize; i++)
        {
            KeyValuePairVector[i].key   = *(keys_first + i);
            KeyValuePairVector[i].value = *(values_first + i);
        }
        //Sort the std_sort vector using std::sort
        std::sort(KeyValuePairVector.begin(), KeyValuePairVector.end(), functor);
        //Extract the keys and values from the KeyValuePair and fill the respective iterators.
        for (size_t i=0; i< vecSize; i++)
        {
            *(keys_first + i)   = KeyValuePairVector[i].key;
            *(values_first + i) = KeyValuePairVector[i].value;
        }
    }



    template<typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_enqueue(control &ctl, const DVRandomAccessIterator1& keys_first,
                             const DVRandomAccessIterator1& keys_last, const DVRandomAccessIterator2& values_first,
                             const StrictWeakOrdering& comp, const std::string& cl_code)
    {
        stablesort_by_key_enqueue(ctl, keys_first, keys_last, values_first, comp, cl_code);
        return;
#if 0
            typedef typename std::iterator_traits< DVRandomAccessIterator1 >::value_type T_keys;
            typedef typename std::iterator_traits< DVRandomAccessIterator2 >::value_type T_values;
            size_t szElements = (size_t)(keys_last - keys_first);
            if(((szElements-1) & (szElements)) != 0)
            {
                // sort_by_key_enqueue_non_powerOf2(ctl,keys_first,keys_last,values_first,comp,cl_code);
                //std::cout << "There is no use supporting selection sort for the sort_by_key routine.\n";
                //std::cout << " Hence only power of 2 buffer sizes work.\n";
                //std::cout << "non power of 2 buffer sizes will be supported once radix-sort is working\n";
                throw ::cl::Error( CL_INVALID_BUFFER_SIZE,
                    "Currently the sort_by_key routine supports only power of 2 buffer size" );
                return;
            }

            std::vector<std::string> typeNames( sort_by_key_end );
            typeNames[sort_by_key_keyValueType] = TypeName< T_keys >::get( );
            typeNames[sort_by_key_valueValueType] = TypeName< T_values >::get( );
            typeNames[sort_by_key_keyIterType] = TypeName< DVRandomAccessIterator1 >::get( );
            typeNames[sort_by_key_valueIterType] = TypeName< DVRandomAccessIterator2 >::get( );
            typeNames[sort_by_key_StrictWeakOrdering] = TypeName< StrictWeakOrdering >::get();

            std::vector<std::string> typeDefinitions;
            PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T_keys >::get() )
            PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T_values >::get() )
            PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator1 >::get() )
            PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator2 >::get() )
            PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get() )

            bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
            /*\TODO - Do CPU specific kernel work group size selection here*/
            //const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
            std::string compileOptions;
            //std::ostringstream oss;
            //oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;

            BitonicSortByKey_KernelTemplateSpecializer ts_kts;
            std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
                ctl,
                typeNames,
                &ts_kts,
                typeDefinitions,
                sort_by_key_kernels,
                compileOptions);

            size_t temp;

            // Set up shape of launch grid and buffers:
            int computeUnits     = ctl.getDevice().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
            int wgPerComputeUnit =  ctl.getWGPerComputeUnit();
            int resultCnt = computeUnits * wgPerComputeUnit;
            cl_int l_Error = CL_SUCCESS;

            size_t wgSize  = kernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >
                                                                                ( ctl.getDevice( ), &l_Error );
            V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
            if((szElements/2) < wgSize)
            {
                wgSize = (int)szElements/2;
            }
            unsigned int numStages,stage,passOfStage;

            ::cl::Buffer Keys = keys_first.getContainer().getBuffer();
            ::cl::Buffer Values = values_first.getContainer().getBuffer();
            ::cl::Buffer userFunctor(ctl.getContext(), CL_MEM_USE_HOST_PTR, sizeof(comp), (void*)&comp );

            numStages = 0;
            for(temp = szElements; temp > 1; temp >>= 1)
                ++numStages;
            V_OPENCL( kernels[0].setArg(0, Keys), "Error setting a kernel argument" );
            V_OPENCL( kernels[0].setArg(1, keys_first.gpuPayloadSize( ), &keys_first.gpuPayload( ) ),
                                                  "Error setting a kernel argument" );
            V_OPENCL( kernels[0].setArg(2, Values), "Error setting a kernel argument" );
            V_OPENCL( kernels[0].setArg(3, values_first.gpuPayloadSize( ), &values_first.gpuPayload( ) ),
                                                  "Error setting a kernel argument" );
            V_OPENCL( kernels[0].setArg(6, userFunctor), "Error setting a kernel argument" );
            for(stage = 0; stage < numStages; ++stage)
            {
                // stage of the algorithm
                V_OPENCL( kernels[0].setArg(4, stage), "Error setting a kernel argument" );
                // Every stage has stage + 1 passes
                for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage)
                {
                    // pass of the current stage
                    V_OPENCL( kernels[0].setArg(5, passOfStage), "Error setting a kernel argument" );
                    /*
                        * Enqueue a kernel run call.
                        * Each thread writes a sorted pair.
                        * So, the number of  threads (global) should be half the length of the input buffer.
                        */
                    l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                            kernels[0],
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/2),
                            ::cl::NDRange(wgSize),
                            NULL,
                            NULL);
                    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
                    V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
                }//end of for passStage = 0:stage-1
            }//end of for stage = 0:numStage-1
            //Map the buffer back to the host
            //ctl.commandQueue().enqueueMapBuffer(Keys, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/,
            //    sizeof(T_keys) * szElements, NULL, NULL, &l_Error );
            //ctl.commandQueue().enqueueMapBuffer(Values, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/,
            //   sizeof(T_values) * szElements, NULL, NULL, &l_Error );
            V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
            V_OPENCL( l_Error, "Error calling map on the result buffer" );
            return;
#endif
    }// END of sort_by_key_enqueue



    //Fancy iterator specialization
    template<typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, DVRandomAccessIterator1 keys_first,
                                   DVRandomAccessIterator1 keys_last, DVRandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp, const std::string& cl_code, bolt::cl::fancy_iterator_tag )
    {
        static_assert( std::is_same<DVRandomAccessIterator1, bolt::cl::fancy_iterator_tag  >::value, "It is not possible to output to fancy iterators; they are not mutable! " );
    }

    //Device Vector specialization
    template<typename DVRandomAccessIterator1, typename DVRandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, DVRandomAccessIterator1 keys_first,
                                   DVRandomAccessIterator1 keys_last, DVRandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp, const std::string& cl_code,
                                   bolt::cl::device_vector_tag, bolt::cl::device_vector_tag )
    {
        typedef typename std::iterator_traits< DVRandomAccessIterator1 >::value_type keyType;
        typedef typename std::iterator_traits< DVRandomAccessIterator2 >::value_type valueType;
        // User defined Data types are not supported with device_vector. Hence we have a static assert here.
        // The code here should be in compliant with the routine following this routine.
        size_t szElements = (size_t)(keys_last - keys_first);
        if (szElements == 0 )
                return;
        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );
        if( runMode == bolt::cl::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }
        if (runMode == bolt::cl::control::SerialCpu) {
            typename bolt::cl::device_vector< keyType >::pointer   keysPtr   =  keys_first.getContainer( ).data( );
            typename bolt::cl::device_vector< valueType >::pointer valuesPtr =  values_first.getContainer( ).data( );
            serialCPU_sort_by_key(&keysPtr[keys_first.m_Index], &keysPtr[keys_last.m_Index],
                                            &valuesPtr[values_first.m_Index], comp);
            return;
        } else if (runMode == bolt::cl::control::MultiCoreCpu) {

            #ifdef ENABLE_TBB
                typename bolt::cl::device_vector< keyType >::pointer   keysPtr   =  keys_first.getContainer( ).data( );
                typename bolt::cl::device_vector< valueType >::pointer valuesPtr =  values_first.getContainer( ).data( );
                bolt::btbb::sort_by_key(&keysPtr[keys_first.m_Index], &keysPtr[keys_last.m_Index],
                                                &valuesPtr[values_first.m_Index], comp);
                return;
            #else
               throw std::runtime_error( "The MultiCoreCpu version of Sort_by_key is not enabled to be built with TBB!\n");
            #endif
        }

        else {

            sort_by_key_enqueue(ctl, keys_first, keys_last, values_first, comp, cl_code);
        }
        return;
    }

    //Non Device Vector specialization.
    //This implementation creates a cl::Buffer and passes the cl buffer to the
    //sort specialization whichtakes the cl buffer as a parameter.
    //In the future, Each input buffer should be mapped to the device_vector and the
    //specialization specific to device_vector should be called.
    template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering>
    void sort_by_key_pick_iterator(control &ctl, RandomAccessIterator1 keys_first,
                                   RandomAccessIterator1 keys_last, RandomAccessIterator2 values_first,
                                   StrictWeakOrdering comp, const std::string& cl_code,
                                   std::random_access_iterator_tag, std::random_access_iterator_tag )
    {

        typedef typename std::iterator_traits<RandomAccessIterator1>::value_type T_keys;
        typedef typename std::iterator_traits<RandomAccessIterator2>::value_type T_values;
        size_t szElements = (size_t)(keys_last - keys_first);
        if (szElements == 0)
            return;

        bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );
        if( runMode == bolt::cl::control::Automatic )
        {
            runMode = ctl.getDefaultPathToRun( );
        }
        if ((runMode == bolt::cl::control::SerialCpu) /*|| (szElements < WGSIZE) */) {
            serialCPU_sort_by_key(keys_first, keys_last, values_first, comp);
            return;
        } else if (runMode == bolt::cl::control::MultiCoreCpu) {

            #ifdef ENABLE_TBB
                serialCPU_sort_by_key(keys_first, keys_last, values_first, comp);
                return;
            #else
                throw std::runtime_error("The MultiCoreCpu Version of Sort_by_key is not enabled to be built with TBB!\n");
            #endif
        } else {

            device_vector< T_values > dvInputValues( values_first, szElements,
                                                     CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, true, ctl );
            device_vector< T_keys > dvInputKeys( keys_first, keys_last,
                                                 CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
            //Now call the actual cl algorithm
            sort_by_key_enqueue(ctl,dvInputKeys.begin(),dvInputKeys.end(), dvInputValues.begin(), comp, cl_code);
            //Map the buffer back to the host
            dvInputValues.data( );
            dvInputKeys.data( );
            return;
        }
    }


    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code,
                                    std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        return sort_by_key_pick_iterator( ctl, keys_first, keys_last, values_first,
                                    comp, cl_code,
                                    typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                    typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code,
                                    std::input_iterator_tag, std::input_iterator_tag )
    {
        //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
        //  to a temporary buffer.  Should we?
        static_assert(std::is_same< RandomAccessIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
        static_assert(std::is_same< RandomAccessIterator2, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
    };

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code,
                                    bolt::cl::fancy_iterator_tag, std::input_iterator_tag )
    {
        static_assert( std::is_same< RandomAccessIterator1, bolt::cl::fancy_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
        static_assert( std::is_same< RandomAccessIterator2, std::input_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
    }

    template< typename RandomAccessIterator1, typename RandomAccessIterator2, typename StrictWeakOrdering >
    void sort_by_key_detect_random_access( control &ctl,
                                    const RandomAccessIterator1 keys_first, const RandomAccessIterator1 keys_last,
                                    const RandomAccessIterator2 values_first,
                                    const StrictWeakOrdering& comp, const std::string& cl_code,
                                    std::input_iterator_tag, bolt::cl::fancy_iterator_tag )
    {


        static_assert( std::is_same< RandomAccessIterator2, bolt::cl::fancy_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );
        static_assert( std::is_same< RandomAccessIterator1, std::input_iterator_tag >::value , "It is not possible to sort fancy iterators. They are not mutable" );

    }


}//namespace bolt::cl::detail


        template<typename RandomAccessIterator1 , typename RandomAccessIterator2>
        void sort_by_key(RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         const std::string& cl_code)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( control::getDefault( ),
                                       keys_first, keys_last,
                                       values_first,
                                       less< keys_T >( ),
                                       cl_code,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2, typename StrictWeakOrdering>
        void sort_by_key(RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         StrictWeakOrdering comp,
                         const std::string& cl_code)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( control::getDefault( ),
                                       keys_first, keys_last,
                                       values_first,
                                       comp,
                                       cl_code,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2>
        void sort_by_key(control &ctl,
                         RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         const std::string& cl_code)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( ctl,
                                       keys_first, keys_last,
                                       values_first,
                                       less< keys_T >( ),
                                       cl_code,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator1 , typename RandomAccessIterator2, typename StrictWeakOrdering>
        void sort_by_key(control &ctl,
                         RandomAccessIterator1 keys_first,
                         RandomAccessIterator1 keys_last,
                         RandomAccessIterator2 values_first,
                         StrictWeakOrdering comp,
                         const std::string& cl_code)
        {
            typedef typename std::iterator_traits< RandomAccessIterator1 >::value_type keys_T;

            detail::sort_by_key_detect_random_access( ctl,
                                       keys_first, keys_last,
                                       values_first,
                                       comp,
                                       cl_code,
                                       typename std::iterator_traits< RandomAccessIterator1 >::iterator_category( ),
                                       typename std::iterator_traits< RandomAccessIterator2 >::iterator_category( ) );
            return;
        }

    }
};



#endif
