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

#if !defined( OCL_SORT_INL )
#define OCL_SORT_INL
#pragma once

#include <algorithm>
#include <type_traits>

#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"
#ifdef ENABLE_TBB
#include "tbb/parallel_sort.h"
#include "tbb/task_scheduler_init.h"
#endif

#define BOLT_UINT_MAX 0xFFFFFFFFU
#define BOLT_UINT_MIN 0x0U
#define BOLT_INT_MAX 0x7FFFFFFF
#define BOLT_INT_MIN 0x80000000

#define BITONIC_SORT_WGSIZE 64
/* \brief - SORT_CPU_THRESHOLD should be atleast 2 times the BITONIC_SORT_WGSIZE*/
#define SORT_CPU_THRESHOLD 128

namespace bolt {
namespace cl {
template<typename RandomAccessIterator>
void sort(RandomAccessIterator first,
          RandomAccessIterator last,
          const std::string& cl_code)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;

    detail::sort_detect_random_access( control::getDefault( ),
                                       first, last,
                                       less< T >( ), cl_code,
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort(RandomAccessIterator first,
          RandomAccessIterator last,
          StrictWeakOrdering comp,
          const std::string& cl_code)
{
    detail::sort_detect_random_access( control::getDefault( ),
                                       first, last,
                                       comp, cl_code,
                                       std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator>
void sort(control &ctl,
          RandomAccessIterator first,
          RandomAccessIterator last,
          const std::string& cl_code)
{
    typedef std::iterator_traits< RandomAccessIterator >::value_type T;

    detail::sort_detect_random_access(ctl,
                                      first, last,
                                      less< T >( ), cl_code,
                                      std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
    return;
}

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort(control &ctl,
          RandomAccessIterator first,
          RandomAccessIterator last,
          StrictWeakOrdering comp,
          const std::string& cl_code)
{
    detail::sort_detect_random_access(ctl,
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

enum sortTypes {sort_iValueType, sort_iIterType, sort_StrictWeakOrdering, sort_end };

class BitonicSort_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    BitonicSort_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("BitonicSortTemplate");
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =

            "// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "kernel void BitonicSortTemplate(\n"
            "global " + typeNames[sort_iValueType] + "* A,\n"
            ""        + typeNames[sort_iIterType]  + " input_iter,\n"
            "const uint stage,\n"
            "const uint passOfStage,\n"
            "global " + typeNames[sort_StrictWeakOrdering] + " * userComp\n"
            ");\n\n";
            return templateSpecializationString;
        }
};

class SelectionSort_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
public:
    SelectionSort_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("selectionSortLocalTemplate");
        addKernelName("selectionSortFinalTemplate");
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =

            "\n// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "kernel void selectionSortLocalTemplate(\n"
            "global const " + typeNames[sort_iValueType] + " * in,\n"
            "global " + typeNames[sort_iValueType] + " * out,\n"
            "global " + typeNames[sort_StrictWeakOrdering] + " * userComp,\n"
            "local  " + typeNames[sort_iValueType] + " * scratch,\n"
            "const int buffSize\n"
            ");\n\n"
            "\n// Host generates this instantiation string with user-specified value type and functor\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "kernel void selectionSortFinalTemplate(\n"
            "global const " + typeNames[sort_iValueType] + " * in,\n"
            "global " + typeNames[sort_iValueType] + " * out,\n"
            "global " + typeNames[sort_StrictWeakOrdering] + " * userComp,\n"
            "local  " + typeNames[sort_iValueType] + " * scratch,\n"
            "const int buffSize\n"
            ");\n\n";
        return templateSpecializationString;
    }
};

class RadixSort_Int_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
private:
    int _radix;
public:
    RadixSort_Int_KernelTemplateSpecializer(int radix) : KernelTemplateSpecializer()
    {
        _radix = radix;
        addKernelName("histogramAscendingRadixNTemplate");
        addKernelName("histogramDescendingRadixNTemplate");
        addKernelName("permuteAscendingRadixNTemplate");
        addKernelName("permuteDescendingRadixNTemplate");
        addKernelName("histogramSignedDescendingRadixNTemplate");
        addKernelName("histogramSignedAscendingRadixNTemplate");
        addKernelName("permuteSignedDescendingRadixNTemplate");
        addKernelName("permuteSignedAscendingRadixNTemplate");
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        std::stringstream radixStream;
        radixStream << _radix;
        const std::string templateSpecializationString =

            "// Host generates this instantiation string with user-specified value type and functor\n"
            "\ntemplate __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "void histogramAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "void permuteAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n"
            "\n"
            "\n"
            "\ntemplate __attribute__((mangled_name(" + name(4) + "Instantiated)))\n"
            "void histogramSignedDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(6) + "Instantiated)))\n"
            "void permuteSignedDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n\n"
            "// Host generates this instantiation string with user-specified value type and functor\n"
            "\ntemplate __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "void histogramDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "void permuteDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n\n"
            "\n"
            "\n"
            "\ntemplate __attribute__((mangled_name(" + name(5) + "Instantiated)))\n"
            "void histogramSignedAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(7) + "Instantiated)))\n"
            "void permuteSignedAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n\n";
        return templateSpecializationString;
    }
};

class RadixSort_Uint_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
private:
    int _radix;
public:
    RadixSort_Uint_KernelTemplateSpecializer(int radix) : KernelTemplateSpecializer()
    {
        _radix = radix;
        addKernelName("histogramAscendingRadixNTemplate");
        addKernelName("histogramDescendingRadixNTemplate");
        addKernelName("permuteAscendingRadixNTemplate");
        addKernelName("permuteDescendingRadixNTemplate");
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        std::stringstream radixStream;
        radixStream << _radix;
        const std::string templateSpecializationString =

            "// Host generates this instantiation string with user-specified value type and functor\n"
            "\ntemplate __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "void histogramAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "void permuteAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n"
            "\n"
            "\n"
            "// Host generates this instantiation string with user-specified value type and functor\n"
            "\ntemplate __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "void histogramDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* buckets,\n"
            "uint shiftCount\n"
            ");\n"
            "\n"
            "\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "void permuteDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
            "global uint* scanedBuckets,\n"
            "uint shiftCount,\n"
            "global uint* sortedData\n"
            ");\n\n"
            "\n"
            "\n";
        return templateSpecializationString;
    }
};

// Wrapper that uses default control class, iterator interface
template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_detect_random_access( control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, const std::string& cl_code,
                                std::input_iterator_tag )
{
    //  \TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_detect_random_access( control &ctl,
                                const RandomAccessIterator& first, const RandomAccessIterator& last,
                                const StrictWeakOrdering& comp, const std::string& cl_code,
                                std::random_access_iterator_tag )
{
    return sort_pick_iterator(ctl, first, last,
                              comp, cl_code,
                              std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
};


//Device Vector specialization
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, const std::string& cl_code,
                         bolt::cl::device_vector_tag )
{
    // User defined Data types are not supported with device_vector. Hence we have a static assert here.
    // The code here should be in compliant with the routine following this routine.
    typedef typename std::iterator_traits<DVRandomAccessIterator>::value_type T;
    size_t szElements = static_cast< size_t >( std::distance( first, last ) );
    if (szElements == 0 )
            return;
    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
    if(runMode == bolt::cl::control::Automatic)
    {
        runMode = ctl.getDefaultPathToRun();
    }
    if ((runMode == bolt::cl::control::SerialCpu) || (szElements < SORT_CPU_THRESHOLD)) {
        ::cl::Event serialCPUEvent;
        cl_int l_Error = CL_SUCCESS;
        /*Map the device buffer to CPU*/
        T *sortInputBuffer = (T*)ctl.getCommandQueue().enqueueMapBuffer(first.getBuffer(), false,
                                                                     CL_MAP_READ|CL_MAP_WRITE,
                                                                     0, sizeof(T) * szElements,
                                                                     NULL, &serialCPUEvent, &l_Error );
        serialCPUEvent.wait();
        //Compute sort using STL
        std::sort(sortInputBuffer, sortInputBuffer + szElements, comp);
        /*Unmap the device buffer back to device memory. This will copy the host modified buffer back to the device*/
        ctl.getCommandQueue().enqueueUnmapMemObject(first.getBuffer(), sortInputBuffer);
        return;
    } else if (runMode == bolt::cl::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        //std::cout << "The MultiCoreCpu version of sort is enabled with TBB. " << std ::endl;
        ::cl::Event multiCoreCPUEvent;
        cl_int l_Error = CL_SUCCESS;
        /*Map the device buffer to CPU*/
        T *sortInputBuffer = (T*)ctl.getCommandQueue().enqueueMapBuffer(first.getBuffer(), false,
                                                                     CL_MAP_READ|CL_MAP_WRITE,
                                                                     0, sizeof(T) * szElements,
                                                                     NULL, &multiCoreCPUEvent, &l_Error );
        multiCoreCPUEvent.wait();
        //Compute parallel sort using TBB
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        tbb::parallel_sort(sortInputBuffer,sortInputBuffer+szElements, comp);
        /*Unmap the device buffer back to device memory. This will copy the host modified buffer back to the device*/
        ctl.getCommandQueue().enqueueUnmapMemObject(first.getBuffer(), sortInputBuffer);
        return;
#else
        //std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
        throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of sort is not enabled to be built." );
#endif

    } else {
        sort_enqueue(ctl,first,last,comp,cl_code);
    }
    return;
}

//Fancy Iterator specialization
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( control &ctl,
                         const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                         const StrictWeakOrdering& comp, const std::string& cl_code,
                         bolt::cl::fancy_iterator_tag )
{
    static_assert( false, "It is not possible to sort fancy iterators. They are not mutable" );
}

//Non Device Vector specialization.
//This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization
//whichtakes the cl buffer as a parameter. In the future, Each input buffer should be mapped to the device_vector
//and the specialization specific to device_vector should be called.
template<typename RandomAccessIterator, typename StrictWeakOrdering>
void sort_pick_iterator( control &ctl,
                         const RandomAccessIterator& first, const RandomAccessIterator& last,
                         const StrictWeakOrdering& comp, const std::string& cl_code,
                         std::random_access_iterator_tag )
{
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    size_t szElements = (size_t)(last - first);
    if (szElements == 0)
        return;

    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
    if(runMode == bolt::cl::control::Automatic)
    {
        runMode = ctl.getDefaultPathToRun();
    }
    if ((runMode == bolt::cl::control::SerialCpu) || (szElements < BITONIC_SORT_WGSIZE)) {
        std::sort(first, last, comp);
        return;
    } else if (runMode == bolt::cl::control::MultiCoreCpu) {
#ifdef ENABLE_TBB
        //std::cout << "The MultiCoreCpu version of sort is enabled with TBB. " << std ::endl;
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        tbb::parallel_sort(first,last, comp);
#else
        //std::cout << "The MultiCoreCpu version of sort is not enabled. " << std ::endl;
        throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of sort is not enabled to be built." );
#endif
    } else {
        device_vector< T > dvInputOutput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        //Now call the actual cl algorithm
        sort_enqueue(ctl,dvInputOutput.begin(),dvInputOutput.end(),comp,cl_code);
        //Map the buffer back to the host
        dvInputOutput.data( );
        return;
    }
}

/****** sort_enqueue specailization for unsigned int data types. ******
 * THE FOLLOWING CODE IMPLEMENTS THE RADIX SORT ALGORITHM FOR unsigned integers
 *********************************************************************/

template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,
                                       unsigned int
                                     >::value
                       >::type  /*If enabled then this typename will be evaluated to void*/
sort_enqueue(control &ctl,
             DVRandomAccessIterator first, DVRandomAccessIterator last,
             StrictWeakOrdering comp, const std::string& cl_code)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 4; //Now you cannot replace this with Radix 8 since there is a
                         //local array of 16 elements in the histogram kernel.
    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
    size_t orig_szElements = static_cast<size_t>(std::distance(first, last));
    size_t szElements = orig_szElements;

    bool  newBuffer = false;
    ::cl::Buffer *pLocalBuffer;
    int computeUnits     = ctl.getDevice().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    cl_int l_Error = CL_SUCCESS;

    static std::vector< ::cl::Kernel > radixSortUintKernels;
    std::vector<std::string> typeNames( sort_end );
    typeNames[sort_iValueType]         = TypeName< T >::get( );
    typeNames[sort_iIterType]          = TypeName< DVRandomAccessIterator >::get( );
    typeNames[sort_StrictWeakOrdering] = TypeName< StrictWeakOrdering >::get();

    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get() )

    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    /*\TODO - Do CPU specific kernel work group size selection here*/

    std::string compileOptions;
    //std::ostringstream oss;
    RadixSort_Uint_KernelTemplateSpecializer ts_kts(RADIX);
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        sort_uint_kernels,
        compileOptions);

    size_t groupSize  = RADICES;

    int i = 0;
    size_t mulFactor = groupSize * RADICES;
    size_t numGroups;

    if(orig_szElements%mulFactor != 0)
    {
        ::cl::Event copyEvent;
        szElements  = ((szElements + mulFactor) /mulFactor) * mulFactor;
        pLocalBuffer = new ::cl::Buffer(ctl.getContext(),CL_MEM_READ_WRITE| CL_MEM_ALLOC_HOST_PTR, sizeof(T) * szElements);

        ctl.getCommandQueue().enqueueCopyBuffer( first.getBuffer( ),
                                              *pLocalBuffer, 0, 0,
                                              orig_szElements*sizeof(T), NULL, &copyEvent );
        copyEvent.wait();
        newBuffer = true;
    }
    else
    {
        pLocalBuffer = new ::cl::Buffer(first.getBuffer( ) );
        newBuffer = false;
    }
    numGroups = szElements / mulFactor;
    device_vector< T > dvSwapInputData( szElements, 0, CL_MEM_READ_WRITE, false, ctl);
    device_vector< T > dvHistogramBins( (numGroups* groupSize * RADICES), 0, CL_MEM_READ_WRITE, false, ctl);
    //This can be avoided if we do a inplace scan.
    device_vector< T > dvHistogramBinsDest( (numGroups* groupSize * RADICES), 0, CL_MEM_READ_WRITE, false, ctl);

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );

    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ),
                                                          CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY,
                                                          &aligned_comp );
    ::cl::Buffer clInputData = *pLocalBuffer;
    ::cl::Buffer clSwapData = dvSwapInputData.begin( ).getBuffer( );
    ::cl::Buffer clHistData = dvHistogramBins.begin( ).getBuffer( );
    ::cl::Buffer clHistDataDest = dvHistogramBinsDest.begin( ).getBuffer( );

    ::cl::Kernel histKernel;
    ::cl::Kernel permuteKernel;

    if(comp(2,3))
    {
        /*Ascending Sort*/
        histKernel = kernels[0];
        permuteKernel = kernels[2];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            ::cl::Event fillEvent;
            clBR.origin = orig_szElements* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - orig_szElements* sizeof(T);
            ctl.getCommandQueue().enqueueFillBuffer(clInputData, (unsigned int)BOLT_UINT_MAX, clBR.origin, clBR.size, NULL, &fillEvent);
            fillEvent.wait();
        }
    }
    else
    {
        /*Descending Sort*/
        histKernel = kernels[1];
        permuteKernel = kernels[3];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            ::cl::Event fillEvent;
            clBR.origin = orig_szElements* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - orig_szElements* sizeof(T);
            ctl.getCommandQueue().enqueueFillBuffer(clInputData, (unsigned int)BOLT_UINT_MIN, clBR.origin, clBR.size, NULL, &fillEvent);
            fillEvent.wait();
        }
    }

    int swap = 0;
    for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
    {
        //Do a histogram pass locally
        if (swap == 0)
            V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
        else
            V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );

        V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(2, bits), "Error setting a kernel argument" );

        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                            histKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );

        //Perform a global scan
        detail::scan_enqueue(ctl, dvHistogramBins.begin(), dvHistogramBins.end(), dvHistogramBinsDest.begin(), 0, plus< T >( ), false);

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
        V_OPENCL( permuteKernel.setArg(1, clHistDataDest), "Error setting a kernel argument" );
        V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(3, clSwapData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(3, clInputData), "Error setting kernel argument" );
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                            permuteKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
        /*For swapping the buffers*/
        swap = swap? 0: 1;
    }
    ::cl::Event uintRadixSortEvent;
    V_OPENCL( ctl.getCommandQueue().clEnqueueBarrierWithWaitList(NULL, &uintRadixSortEvent) , "Error calling clEnqueueBarrierWithWaitList on the command queue" );
    bolt::cl::wait(ctl, uintRadixSortEvent);

    if(newBuffer == true)
    {
        ::cl::Event copyBackEvent;
        ctl.getCommandQueue().enqueueCopyBuffer( clInputData, first.getBuffer( ), 0, 0, sizeof(T)*orig_szElements, NULL, &copyBackEvent );
        copyBackEvent.wait();
    }
    delete pLocalBuffer;
    return;
}


/****** sort_enqueue specailization for signed int data types. ******
 * THE FOLLOWING CODE IMPLEMENTS THE RADIX SORT ALGORITHM FOR signed integers
 *********************************************************************/
template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,
                                       int
                                     >::value
                       >::type   /*If enabled then this typename will be evaluated to void*/
sort_enqueue(control &ctl,
             DVRandomAccessIterator first, DVRandomAccessIterator last,
             StrictWeakOrdering comp, const std::string& cl_code)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    const int RADIX = 4; //Now you cannot replace this with Radix 8 since there is a
                         //local array of 16 elements in the histogram kernel.
    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
    cl_int l_Error = CL_SUCCESS;
    size_t orig_szElements = static_cast<size_t>(std::distance(first, last));
    size_t szElements = orig_szElements;
    bool  newBuffer = false;
    ::cl::Buffer *pLocalBuffer;

    int computeUnits     = ctl.getDevice().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

    std::vector<std::string> typeNames( sort_end );
    typeNames[sort_iValueType] = TypeName< T >::get( );
    typeNames[sort_iIterType] = TypeName< DVRandomAccessIterator >::get( );
    typeNames[sort_StrictWeakOrdering] = TypeName< StrictWeakOrdering >::get();

    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get() )

    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    /*\TODO - Do CPU specific kernel work group size selection here*/
    //const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;

    std::string compileOptions;
    //std::ostringstream oss;

    RadixSort_Int_KernelTemplateSpecializer ts_kts(RADIX);
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        sort_uint_kernels,
        compileOptions);

    unsigned int groupSize  = RADICES;

    int i = 0;
    size_t mulFactor = groupSize * RADICES;

    if(orig_szElements%mulFactor != 0)
    {
        ::cl::Event copyEvent;
        szElements  = ((szElements + mulFactor) /mulFactor) * mulFactor;
        pLocalBuffer = new ::cl::Buffer(ctl.getContext(),CL_MEM_READ_WRITE| CL_MEM_ALLOC_HOST_PTR, sizeof(T) * szElements);

        ctl.getCommandQueue().enqueueCopyBuffer( first.getBuffer( ),
                                              *pLocalBuffer, 0, 0,
                                              orig_szElements*sizeof(T), NULL, &copyEvent );
        copyEvent.wait();
        newBuffer = true;
    }
    else
    {
        pLocalBuffer = new ::cl::Buffer(first.getBuffer( ) );
        newBuffer = false;
    }
    size_t numGroups = szElements / mulFactor;
    device_vector< T > dvSwapInputData( szElements, 0, CL_MEM_READ_WRITE, true, ctl);
    device_vector< T > dvHistogramBins( (numGroups* groupSize * RADICES), 0, CL_MEM_READ_WRITE, true, ctl);
    //This can be avoided if we do an inplace scan and probaly will get better performance
    device_vector< T > dvHistogramBinsDest( (numGroups* groupSize * RADICES), 0, CL_MEM_READ_WRITE, false, ctl);

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );

    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );
    ::cl::Buffer clInputData = *pLocalBuffer;
    ::cl::Buffer clSwapData = dvSwapInputData.begin( ).getBuffer( );
    ::cl::Buffer clHistData = dvHistogramBins.begin( ).getBuffer( );
    ::cl::Buffer clHistDataDest = dvHistogramBinsDest.begin( ).getBuffer( );

    ::cl::Kernel histKernel, histSignedKernel;
    ::cl::Kernel permuteKernel, permuteSignedKernel;
    if(comp(2,3))
    {
        /*Ascending Sort*/
        histKernel = kernels[0];
        //scanLocalKernel = kernels[2];
        permuteKernel = kernels[2];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            clBR.origin = orig_szElements* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - orig_szElements* sizeof(T);
            ctl.getCommandQueue().enqueueFillBuffer(clInputData, BOLT_INT_MAX, clBR.origin, clBR.size, NULL, NULL);
        }
    }
    else
    {
        /*Descending Sort*/
        histKernel = kernels[1];
        //scanLocalKernel = kernels[2];
        permuteKernel = kernels[3];
        if(newBuffer == true)
        {
            cl_buffer_region clBR;
            clBR.origin = orig_szElements* sizeof(T);
            clBR.size  = (szElements * sizeof(T)) - orig_szElements* sizeof(T);
            ctl.getCommandQueue().enqueueFillBuffer(clInputData, BOLT_INT_MIN, clBR.origin, clBR.size, NULL, NULL);
        }
    }

    ::cl::LocalSpaceArg localScanArray;
    localScanArray.size_ = 2*RADICES* sizeof(cl_uint);
    int swap = 0;
    int bits;
    for(bits = 0; bits < (sizeof(T) * 8 - RADIX)/*Bits per Byte*/; bits += RADIX)
    {
        //Do a histogram pass locally
        if (swap == 0)
            V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
        else
            V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );

        V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
        //V_OPENCL( histKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
        V_OPENCL( histKernel.setArg(2, bits), "Error setting a kernel argument" );
        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                            histKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );

        //Perform a global scan

        detail::scan_enqueue(ctl, dvHistogramBins.begin(), dvHistogramBins.end(),
                                  dvHistogramBinsDest.begin(), 0, plus< T >( ), false);

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
        V_OPENCL( permuteKernel.setArg(1, clHistDataDest), "Error setting a kernel argument" );
        V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );
        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );

        if (swap == 0)
            V_OPENCL( permuteKernel.setArg(3, clSwapData), "Error setting kernel argument" );
        else
            V_OPENCL( permuteKernel.setArg(3, clInputData), "Error setting kernel argument" );
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                            permuteKernel,
                            ::cl::NullRange,
                            ::cl::NDRange(szElements/RADICES),
                            ::cl::NDRange(groupSize),
                            NULL,
                            NULL);
        //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );

            /*For swapping the buffers*/
            swap = swap? 0: 1;
    }

    if(comp(2,3))
    {
        /*Ascending Sort*/
        histSignedKernel = kernels[4];
        permuteSignedKernel = kernels[6];
    }
    else
    {
        histSignedKernel = kernels[5];
        permuteSignedKernel = kernels[7];
    }
        //Do a histogram pass locally
            V_OPENCL( histSignedKernel.setArg(0, clSwapData), "Error setting a kernel argument" );
            V_OPENCL( histSignedKernel.setArg(1, clHistData), "Error setting a kernel argument" );
            //V_OPENCL( histSignedKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
            V_OPENCL( histSignedKernel.setArg(2, bits), "Error setting a kernel argument" );
            //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

            l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                                histSignedKernel,
                                ::cl::NullRange,
                                ::cl::NDRange(szElements/RADICES),
                                ::cl::NDRange(groupSize),
                                NULL,
                                NULL);
            //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );

            //Perform a global scan
            detail::scan_enqueue(ctl, dvHistogramBins.begin(), dvHistogramBins.end(), dvHistogramBinsDest.begin(), 0, plus< T >( ), false);

            V_OPENCL( permuteSignedKernel.setArg(0, clSwapData), "Error setting kernel argument" );
            V_OPENCL( permuteSignedKernel.setArg(1, clHistDataDest), "Error setting a kernel argument" );
            V_OPENCL( permuteSignedKernel.setArg(2, bits), "Error setting a kernel argument" );
            V_OPENCL( permuteSignedKernel.setArg(3, clInputData), "Error setting kernel argument" );
            l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                                permuteSignedKernel,
                                ::cl::NullRange,
                                ::cl::NDRange(szElements/RADICES),
                                ::cl::NDRange(groupSize),
                                NULL,
                                NULL);

    ::cl::Event intRadixSortEvent;
    V_OPENCL( ctl.getCommandQueue().clEnqueueBarrierWithWaitList(NULL, &intRadixSortEvent) , "Error calling clEnqueueBarrierWithWaitList on the command queue" );
    bolt::cl::wait(ctl, intRadixSortEvent);

    if(newBuffer == true)
    {
        ::cl::Event copyBackEvent;
        ctl.getCommandQueue().enqueueCopyBuffer( clInputData, first.getBuffer( ), 0, 0, sizeof(T)*orig_szElements, NULL, &copyBackEvent );
        copyBackEvent.wait();
    }
    delete pLocalBuffer;
    return;
}


template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
typename std::enable_if<
    !(std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value
   || std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,          int >::value
    )
                       >::type
sort_enqueue(control &ctl,
             const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
             const StrictWeakOrdering& comp, const std::string& cl_code)
{
    cl_int l_Error = CL_SUCCESS;
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
    size_t szElements = static_cast< size_t >( std::distance( first, last ) );
    if(((szElements-1) & (szElements)) != 0)
    {
        sort_enqueue_non_powerOf2(ctl,first,last,comp,cl_code);
        return;
    }

    std::vector<std::string> typeNames( sort_end );
    typeNames[sort_iValueType] = TypeName< T >::get( );
    typeNames[sort_iIterType] = TypeName< DVRandomAccessIterator >::get( );
    typeNames[sort_StrictWeakOrdering] = TypeName< StrictWeakOrdering >::get();

    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get() )

    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    /*\TODO - Do CPU specific kernel work group size selection here*/
    //const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    std::string compileOptions;
    //std::ostringstream oss;
    //oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;

    size_t temp;

    BitonicSort_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        sort_kernels,
        compileOptions);
    //Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class -
    // Note use of TypeName<>::get to retreive the name here.


    size_t wgSize  = BITONIC_SORT_WGSIZE;

    if((szElements/2) < BITONIC_SORT_WGSIZE)
    {
        wgSize = (int)szElements/2;
    }
    unsigned int stage,passOfStage;
    unsigned int numStages = 0;
    for(temp = szElements; temp > 1; temp >>= 1)
        ++numStages;

    //::cl::Buffer A = first.getBuffer( );
    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ),
                                                          CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

    V_OPENCL( kernels[0].setArg(0, first.getBuffer( )), "Error setting 0th kernel argument" );
    V_OPENCL( kernels[0].setArg(1, first.gpuPayloadSize( ), &first.gpuPayload( )), "Error setting 1st kernel argument" );
    V_OPENCL( kernels[0].setArg(4, *userFunctor), "Error setting 4th kernel argument" );
    for(stage = 0; stage < numStages; ++stage)
    {
        // stage of the algorithm
        V_OPENCL( kernels[0].setArg(2, stage), "Error setting 2nd kernel argument" );
        // Every stage has stage + 1 passes
        for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
            // pass of the current stage
            V_OPENCL( kernels[0].setArg(3, passOfStage), "Error setting 3rd kernel argument" );
            /*
             * Enqueue a kernel run call.
             * Each thread writes a sorted pair.
             * So, the number of  threads (global) should be half the length of the input buffer.
             */
            l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                                            kernels[0],
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/2),
                                            ::cl::NDRange(BITONIC_SORT_WGSIZE),
                                            NULL,
                                            NULL);

            V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
            //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
        }//end of for passStage = 0:stage-1
    }//end of for stage = 0:numStage-1
    ::cl::Event bitonicSortEvent;
    V_OPENCL( ctl.getCommandQueue().clEnqueueBarrierWithWaitList(NULL, &bitonicSortEvent) ,
                        "Error calling clEnqueueBarrierWithWaitList on the command queue" );
    l_Error = bitonicSortEvent.wait( );
    V_OPENCL( l_Error, "bitonicSortEvent failed to wait" );
    //V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
    return;
}// END of sort_enqueue


template<typename DVRandomAccessIterator, typename StrictWeakOrdering>
void sort_enqueue_non_powerOf2(control &ctl,
                               const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                               const StrictWeakOrdering& comp, const std::string& cl_code)
{
    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;

    cl_int l_Error;
    size_t szElements = (size_t)(last - first);

    std::vector<std::string> typeNames( sort_end );
    typeNames[sort_iValueType] = TypeName< T >::get( );
    typeNames[sort_iIterType] = TypeName< DVRandomAccessIterator >::get( );
    typeNames[sort_StrictWeakOrdering] = TypeName< StrictWeakOrdering >::get();
    // Power of 2 buffer size
    // For user-defined types, the user must create a TypeName trait which returns the name of the class -
    // Note use of TypeName<>::get to retreive the name here.

    std::vector<std::string> typeDefinitions;
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< T >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< DVRandomAccessIterator >::get() )
    PUSH_BACK_UNIQUE( typeDefinitions, ClCode< StrictWeakOrdering  >::get() )

    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    /*\TODO - Do CPU specific kernel work group size selection here*/
    //const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    std::string compileOptions;
    //std::ostringstream oss;
    //oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;

    SelectionSort_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefinitions,
        sort_kernels,
        compileOptions);

    size_t wgSize  = kernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.getDevice( ), &l_Error );

    size_t totalWorkGroups = (szElements + wgSize)/wgSize;
    size_t globalSize = totalWorkGroups * wgSize;
    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

    const ::cl::Buffer& in = first.getBuffer( );
    control::buffPointer out = ctl.acquireBuffer( sizeof(T)*szElements );

    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ),
                                                          CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

    ::cl::LocalSpaceArg loc;
    loc.size_ = wgSize*sizeof(T);

    V_OPENCL( kernels[0].setArg(0, in), "Error setting a kernel argument in" );
    V_OPENCL( kernels[0].setArg(1, *out), "Error setting a kernel argument out" );
    V_OPENCL( kernels[0].setArg(2, *userFunctor), "Error setting a kernel argument userFunctor" );
    V_OPENCL( kernels[0].setArg(3, loc), "Error setting kernel argument loc" );
    V_OPENCL( kernels[0].setArg(4, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                                        kernels[0],
                                        ::cl::NullRange,
                                        ::cl::NDRange(globalSize),
                                        ::cl::NDRange(wgSize),
                                        NULL,
                                        NULL);
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
        V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
    }

    wgSize  = kernels[1].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.getDevice( ), &l_Error );

    V_OPENCL( kernels[1].setArg(0, *out), "Error setting a kernel argument in" );
    V_OPENCL( kernels[1].setArg(1, in), "Error setting a kernel argument out" );
    V_OPENCL( kernels[1].setArg(2, *userFunctor), "Error setting a kernel argument userFunctor" );
    V_OPENCL( kernels[1].setArg(3, loc), "Error setting kernel argument loc" );
    V_OPENCL( kernels[1].setArg(4, static_cast<cl_uint> (szElements)), "Error setting kernel argument szElements" );
    {
        l_Error = ctl.getCommandQueue().enqueueNDRangeKernel(
                                        kernels[1],
                                        ::cl::NullRange,
                                        ::cl::NDRange(globalSize),
                                        ::cl::NDRange(wgSize),
                                        NULL,
                                        NULL);
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for sort() kernel" );
        V_OPENCL( ctl.getCommandQueue().finish(), "Error calling finish on the command queue" );
    }
    return;
}// END of sort_enqueue_non_powerOf2

}//namespace bolt::cl::detail
}//namespace bolt::cl
}//namespace bolt

#endif
