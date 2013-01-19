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

#if !defined( SORT_INL )
#define SORT_INL
#pragma once

#include <algorithm>
#include <type_traits>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>
#define CL_VERSION_1_2 1
#include "bolt/cl/bolt.h"
#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"

#define BOLT_UINT_MAX 0xFFFFFFFFU
#define BOLT_UINT_MIN 0x0U
#define WGSIZE 64

namespace bolt {
    namespace cl {
        template<typename RandomAccessIterator> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last, 
            const std::string& cl_code)
        {
            typedef std::iterator_traits< RandomAccessIterator >::value_type T;

            detail::sort_detect_random_access( control::getDefault( ), first, last, less< T >( ), cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

        template<typename RandomAccessIterator, typename StrictWeakOrdering> 
        void sort(RandomAccessIterator first, 
            RandomAccessIterator last,  
            StrictWeakOrdering comp, 
            const std::string& cl_code)  
        {
            detail::sort_detect_random_access( control::getDefault( ), first, last, comp, cl_code, 
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

            detail::sort_detect_random_access(ctl, first, last, less< T >( ), cl_code, 
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
            detail::sort_detect_random_access(ctl, first, last, comp, cl_code, 
                std::iterator_traits< RandomAccessIterator >::iterator_category( ) );
            return;
        }

    }
};


namespace bolt {
    namespace cl {
        namespace detail {

            struct CallCompiler_Sort {

                static void constructAndCompileRadixSortInt(std::vector< ::cl::Kernel >* radixSortKernels,  std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control *ctl) {

                    std::vector< const std::string > kernelNames;
                    kernelNames.push_back( "histogramAscendingRadixN" );
                    kernelNames.push_back( "histogramDescendingRadixN" );
                    kernelNames.push_back( "scanLocalRadixN" );
                    kernelNames.push_back( "permuteAscendingRadixN" );
                    kernelNames.push_back( "permuteDescendingRadixN" );

                    const std::string instantiationString = "";

                    bolt::cl::compileKernelsString( *radixSortKernels, kernelNames, sort_uint_kernels, instantiationString, cl_code_dataType, valueTypeName, "", *ctl );
                }
                static void constructAndCompileRadixSortUintTemplate(std::vector< ::cl::Kernel >* radixSortKernels,  int radix, std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control *ctl) {
                    
                    std::stringstream radixStream;
                    std::vector< const std::string > kernelNames;
                    radixStream << radix;

                    std::string temp = "histogramAscendingRadix" + radixStream.str();
                    kernelNames.push_back( temp );
                    temp = "histogramDescendingRadix" + radixStream.str();
                    kernelNames.push_back( temp );

                    kernelNames.push_back( "scanLocal" );
                    
                    temp = "permuteAscendingRadix" + radixStream.str();
                    kernelNames.push_back( temp );
                    temp = "permuteDescendingRadix" + radixStream.str();
                    kernelNames.push_back( temp );

                    std::string tempHistAscending;
                    std::string tempPermAscending; 
                    std::string tempHistDescending;
                    std::string tempPermDescending;

                    tempHistAscending  = "histogramAscendingRadix"+ radixStream.str() +"Instantiated";
                    tempPermAscending  = "permuteAscendingRadix"+ radixStream.str() +"Instantiated";
                    tempHistDescending = "histogramDescendingRadix"+ radixStream.str() +"Instantiated";
                    tempPermDescending = "permuteDescendingRadix"+ radixStream.str() +"Instantiated";

                    std::string ScanInstantiationString = 
                        "\ntemplate __attribute__((mangled_name(scanLocalInstantiated)))\n"
                         "void scanLocalTemplate< " +radixStream.str()+ " >(__global uint* buckets,\n"
                         "global uint* histScanBuckets,\n"
                         "local uint* localScanArray);\n\n";

                    std::string AscendingInstantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "\ntemplate __attribute__((mangled_name(" + tempHistAscending + ")))\n"
                        "void histogramAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
                        "global uint* buckets,\n"
                        "global uint* histScanBuckets,\n"
                        "uint shiftCount\n"
                        ");\n\n"
                        "template __attribute__((mangled_name(" + tempPermAscending + ")))\n"
                        "void permuteAscendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
                        "global uint* scanedBuckets,\n"
                        "uint shiftCount,\n"
                        "global uint* sortedData\n"
                        ");\n\n";

                    std::string DescendingInstantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "\ntemplate __attribute__((mangled_name(" + tempHistDescending + ")))\n"
                        "void histogramDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
                        "global uint* buckets,\n"
                        "global uint* histScanBuckets,\n"
                        "uint shiftCount\n"
                        ");\n\n"
                        "template __attribute__((mangled_name(" + tempPermDescending + ")))\n"
                        "void permuteDescendingRadixNTemplate< " +radixStream.str()+ " >(__global uint* unsortedData,\n"
                        "global uint* scanedBuckets,\n"
                        "uint shiftCount,\n"
                        "global uint* sortedData\n"
                        ");\n\n";

                    bolt::cl::compileKernelsString( *radixSortKernels, kernelNames, sort_uint_kernels, AscendingInstantiationString + ScanInstantiationString + DescendingInstantiationString, cl_code_dataType, valueTypeName, "", *ctl );
                }
                static void constructAndCompile(::cl::Kernel *masterKernel,  std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control *ctl) {

                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(sortInstantiated)))\n"
                        "kernel void sortTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const uint stage,\n"
                        "const uint passOfStage,\n"
                        "global " + compareTypeName + " * userComp\n"
                        ");\n\n";

                    bolt::cl::constructAndCompileString(masterKernel, "sort", sort_kernels, instantiationString, cl_code_dataType, valueTypeName, "", *ctl);
                }
                static void constructAndCompileSelectionSort(std::vector< ::cl::Kernel >* sortKernels,  std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control *ctl) {

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

                    bolt::cl::compileKernelsString( *sortKernels, kernelNames, sort_kernels, instantiationString, cl_code_dataType, valueTypeName, "", *ctl );
                }

            }; //End of struct CallCompiler_Sort  

            // Wrapper that uses default control class, iterator interface
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( control &ctl, const RandomAccessIterator& first, const RandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( control &ctl, const RandomAccessIterator& first, const RandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code, std::random_access_iterator_tag )
            {
                return sort_pick_iterator(ctl, first, last, comp, cl_code);
            };

            //Device Vector specialization
            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVRandomAccessIterator>::value_type>::iterator,DVRandomAccessIterator>::value >::type
            sort_pick_iterator(control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code) 
            {
                // User defined Data types are not supported with device_vector. Hence we have a static assert here.
                // The code here should be in compliant with the routine following this routine.
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0 )
                        return;
                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
                if (runMode == bolt::cl::control::SerialCpu) {
                    //  TODO:  Need access to the device_vector .data method to get a host pointer
                    throw ::cl::Error( CL_INVALID_DEVICE, "Sort of device_vector CPU device not implemented" );
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of device_vector sort is not implemented yet." << std ::endl;
                    throw ::cl::Error( CL_INVALID_DEVICE, "The BOLT sort routine device_vector does not support non power of 2 buffer size." );
                    return;
                } else {
                    sort_enqueue(ctl,first,last,comp,cl_code);
                }
                return;
            }
            
            //Non Device Vector specialization.
            //This implementation creates a cl::Buffer and passes the cl buffer to the sort specialization whichtakes the cl buffer as a parameter. 
            //In the future, Each input buffer should be mapped to the device_vector and the specialization specific to device_vector should be called. 
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<RandomAccessIterator>::value_type>::iterator,RandomAccessIterator>::value >::type
            sort_pick_iterator(control &ctl, const RandomAccessIterator& first, const RandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code)  
            {
                typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
                size_t szElements = (size_t)(last - first); 
                if (szElements == 0)
                    return;

                const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode();  // could be dynamic choice some day.
                if ((runMode == bolt::cl::control::SerialCpu) || (szElements < WGSIZE)) {
                    std::sort(first, last, comp);
                    return;
                } else if (runMode == bolt::cl::control::MultiCoreCpu) {
                    std::cout << "The MultiCoreCpu version of sort is not implemented yet." << std ::endl;
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
 * THE FOLLOWING CODE IMPLEMENTS THE RADIX SORT ALGORITHM FOR sort()
 *********************************************************************/
#define DEBUG 0
            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
            sort_enqueue(control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    const int RADIX = 2;
                    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
                    unsigned int szElements = (last - first);
                    device_vector< T > dvInputData;//(sizeof(T), 0);
                    bool  newBuffer = false;
                    //std::cout << "Calling unsigned int sort_enqueue sizeof T = "<< sizeof(T) << "\n";
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    //std::cout << "CU = " << computeUnits << "wgPerComputeUnit = "<< wgPerComputeUnit << "\n";
                    cl_int l_Error = CL_SUCCESS;

                    static  boost::once_flag initOnlyOnce;
                    static std::vector< ::cl::Kernel > radixSortUintKernels;
                    
                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileRadixSortUintTemplate, &radixSortUintKernels, RADIX, cl_code +ClCode<T>::get(), "", TypeName<StrictWeakOrdering>::get(), &ctl) );
                    unsigned int groupSize  = (unsigned int)radixSortUintKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    groupSize = RADICES;
                    const int NUM_OF_ELEMENTS_PER_WORK_ITEM = RADICES;
                    unsigned int num_of_elems_per_group = RADICES  * groupSize;

                    int i = 0;
                    unsigned int mulFactor = groupSize * RADICES;
                    unsigned int numGroups = szElements / mulFactor;
                    
                    if(szElements%mulFactor != 0)
                    {
                        szElements  = ((szElements + mulFactor) /mulFactor) * mulFactor;
                        dvInputData.resize(sizeof(T)*szElements);
                        ctl.commandQueue().enqueueCopyBuffer( first->getBuffer( ), dvInputData.begin( )->getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
                        newBuffer = true;
                    }
                    else
                    {
                        dvInputData = device_vector< T >(first->getBuffer( ), ctl);
                        newBuffer = false;
                    }

                    device_vector< T > dvSwapInputData( sizeof(T)*szElements, 0);
                    device_vector< T > dvHistogramBins( sizeof(T)*(numGroups* groupSize * RADICES), 0);
                    device_vector< T > dvHistogramScanBuffer( sizeof(T)*(numGroups* RADICES + 10), 0);

                    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
                    
                    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );
                    ::cl::Buffer clInputData = dvInputData.begin( )->getBuffer( );
                    ::cl::Buffer clSwapData = dvSwapInputData.begin( )->getBuffer( );
                    ::cl::Buffer clHistData = dvHistogramBins.begin( )->getBuffer( );
                    ::cl::Buffer clHistScanData = dvHistogramScanBuffer.begin( )->getBuffer( );
                    
                    ::cl::Kernel histKernel;
                    ::cl::Kernel permuteKernel;
                    ::cl::Kernel scanLocalKernel;
                    if(comp(2,3))
                    {
                        /*Ascending Sort*/
                        histKernel = radixSortUintKernels[0];
                        scanLocalKernel = radixSortUintKernels[2];
                        permuteKernel = radixSortUintKernels[3];
                        if(newBuffer == true)
                        {
                            cl_buffer_region clBR;
                            clBR.origin = (last - first)* sizeof(T);
                            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);

                            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MAX, clBR.origin, clBR.size, NULL, NULL);
                        }
                    }
                    else
                    {
                        /*Descending Sort*/
                        histKernel = radixSortUintKernels[1];
                        scanLocalKernel = radixSortUintKernels[2];
                        permuteKernel = radixSortUintKernels[4];
                        if(newBuffer == true)
                        {
                            cl_buffer_region clBR;
                            clBR.origin = (last - first)* sizeof(T);
                            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
                            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MIN, clBR.origin, clBR.size, NULL, NULL);
                        }
                    }
                    std::cout << "szElements " << szElements << "\n";
                    ::cl::LocalSpaceArg localScanArray;
                    localScanArray.size_ = 2*RADICES* sizeof(cl_uint);
                    int swap = 0;
                    for(int bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
                    {
                        //Do a histogram pass locally
                        if (swap == 0)
                            V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
                        else
                            V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );

                        V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(3, bits), "Error setting a kernel argument" );
                        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            histKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        //Create local device_vector's 
        T *histBuffer;// = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
        T *histScanBuffer;// = (T*)calloc(1, numGroups* RADICES * sizeof(T));
        ::cl::Event l_histEvent;
        histBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
        bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer);

        ::cl::Event l_histScanBufferEvent;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        int temp = 0;
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                temp += histScanBuffer[i*numGroups + ng];
                printf ("%x, ",temp);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);
#endif

                        //Perform a global scan 
                        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));
#if (DEBUG==1)
        ::cl::Event l_histScanBufferEvent1;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);

#endif 

                        //Add the results of the global scan to the local scan buffers
                        V_OPENCL( scanLocalKernel.setArg(0, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(1, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(2, localScanArray), "Error setting a kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            scanLocalKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        ::cl::Event l_histEvent1;
        T *histBuffer1;
        histBuffer1 = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );

        printf("\n\nAfter Scan bits = %d", bits);
        for (int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer1[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer1);
#endif 
                        if (swap == 0)
                            V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
                        else
                            V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
                        V_OPENCL( permuteKernel.setArg(1, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );
                        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );

                        if (swap == 0)
                            V_OPENCL( permuteKernel.setArg(3, clSwapData), "Error setting kernel argument" );
                        else
                            V_OPENCL( permuteKernel.setArg(3, clInputData), "Error setting kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            permuteKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
#if (DEBUG==1)
            T *swapBuffer;// = (T*)malloc(szElements * sizeof(T));
            ::cl::Event l_swapEvent;
            if(bits==0 || bits==16)
            //if(bits==0 || bits==8 || bits==16 || bits==24)
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clSwapData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clSwapData, swapBuffer);
            }
            else
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clInputData, swapBuffer);
            }


#endif
                        if(swap==0)
                            swap = 1;
                        else
                            swap = 0;
                    }
                    if(newBuffer == true)
                    {
                        //::cl::copy(clInputData, first, last);
                        ctl.commandQueue().enqueueCopyBuffer( dvInputData.begin( )->getBuffer( ), first->getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
                    }
                    return;
            }

            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, int >::value >::type
            sort_enqueue(control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    const int RADIX = 4;
                    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
                    unsigned int szElements = (last - first);
                    device_vector< T > dvInputData;//(sizeof(T), 0);
                    bool  newBuffer = false;
                    //std::cout << "Calling unsigned int sort_enqueue sizeof T = "<< sizeof(T) << "\n";
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    //std::cout << "CU = " << computeUnits << "wgPerComputeUnit = "<< wgPerComputeUnit << "\n";
                    cl_int l_Error = CL_SUCCESS;

                    static  boost::once_flag initOnlyOnce;
                    static std::vector< ::cl::Kernel > radixSortIntKernels;
                    
                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileRadixSortInt, &radixSortIntKernels, cl_code +ClCode<T>::get(), "", TypeName<StrictWeakOrdering>::get(), &ctl) );
                    unsigned int groupSize  = (unsigned int)radixSortIntKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    groupSize = RADICES;
                    const int NUM_OF_ELEMENTS_PER_WORK_ITEM = RADICES;
                    unsigned int num_of_elems_per_group = RADICES  * groupSize;

                    int i = 0;
                    unsigned int mulFactor = groupSize * RADICES;
                    unsigned int numGroups = szElements / mulFactor;
                    
                    if(szElements%mulFactor != 0)
                    {
                        szElements  = ((szElements + mulFactor) /mulFactor) * mulFactor;
                        dvInputData.resize(sizeof(T)*szElements);
                        ctl.commandQueue().enqueueCopyBuffer( first->getBuffer( ), dvInputData.begin( )->getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
                        newBuffer = true;
                    }
                    else
                    {
                        dvInputData = device_vector< T >(first->getBuffer( ), ctl);
                        newBuffer = false;
                    }

                    device_vector< T > dvSwapInputData( sizeof(T)*szElements, 0);
                    device_vector< T > dvHistogramBins( sizeof(T)*(numGroups* groupSize * RADICES), 0);
                    device_vector< T > dvHistogramScanBuffer( sizeof(T)*(numGroups* RADICES + 10), 0);

                    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
                    
                    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );
                    ::cl::Buffer clInputData = dvInputData.begin( )->getBuffer( );
                    ::cl::Buffer clSwapData = dvSwapInputData.begin( )->getBuffer( );
                    ::cl::Buffer clHistData = dvHistogramBins.begin( )->getBuffer( );
                    ::cl::Buffer clHistScanData = dvHistogramScanBuffer.begin( )->getBuffer( );
                    
                    ::cl::Kernel histKernel;
                    ::cl::Kernel permuteKernel;
                    ::cl::Kernel scanLocalKernel;
                    if(comp(2,3))
                    {
                        /*Ascending Sort*/
                        histKernel = radixSortIntKernels[0];
                        scanLocalKernel = radixSortIntKernels[2];
                        permuteKernel = radixSortIntKernels[3];
                        if(newBuffer == true)
                        {
                            cl_buffer_region clBR;
                            clBR.origin = (last - first)* sizeof(T);
                            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
                            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MAX, clBR.origin, clBR.size, NULL, NULL);
                        }
                    }
                    else
                    {
                        /*Descending Sort*/
                        histKernel = radixSortIntKernels[1];
                        scanLocalKernel = radixSortIntKernels[2];
                        permuteKernel = radixSortIntKernels[4];
                        if(newBuffer == true)
                        {
                            cl_buffer_region clBR;
                            clBR.origin = (last - first)* sizeof(T);
                            clBR.size  = (szElements * sizeof(T)) - (last - first)* sizeof(T);
                            ctl.commandQueue().enqueueFillBuffer(clInputData, BOLT_UINT_MIN, clBR.origin, clBR.size, NULL, NULL);
                        }
                    }
                    std::cout << "szElements " << szElements << "\n";
                    ::cl::LocalSpaceArg localScanArray;
                    localScanArray.size_ = 2*RADICES* sizeof(cl_uint);
                    int swap = 0;
                    int mask = (1<<RADIX) - 1;
                    int bits = 0;
                    for(bits = 0; bits < (sizeof(T) * 8)/*Bits per Byte*/; bits += RADIX)
                    {
                        //Do a histogram pass locally
                        V_OPENCL( histKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(1, mask), "Error setting a kernel argument" );
                        if (swap == 0)
                            V_OPENCL( histKernel.setArg(2, clInputData), "Error setting a kernel argument" );
                        else
                            V_OPENCL( histKernel.setArg(2, clSwapData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(4, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(5, bits), "Error setting a kernel argument" );
                        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            histKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        //Create local device_vector's 
        T *histBuffer;// = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
        T *histScanBuffer;// = (T*)calloc(1, numGroups* RADICES * sizeof(T));
        ::cl::Event l_histEvent;
        histBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
        bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
        for (unsigned int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(unsigned int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer);

        ::cl::Event l_histScanBufferEvent;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        int temp = 0;
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
            for (unsigned int ng=0; ng<numGroups; ng++)
            {
                temp += histScanBuffer[i*numGroups + ng];
                printf ("%x, ",temp);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);
#endif

                        //Perform a global scan 
                        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));
#if (DEBUG==1)
        ::cl::Event l_histScanBufferEvent1;
        histScanBuffer = (T*) ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histScanBufferEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
        for(int i=0; i<RADICES;i++)
        { 
            printf ("\nRadix = %d\n",i);
            for (int ng=0; ng<numGroups; ng++)
            {
                printf ("%x, ",histScanBuffer[i*numGroups + ng]);
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistScanData, histScanBuffer);

#endif 

                        //Add the results of the global scan to the local scan buffers
                        V_OPENCL( scanLocalKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(1, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(3, localScanArray), "Error setting a kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            scanLocalKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
        ::cl::Event l_histEvent1;
        T *histBuffer1;
        histBuffer1 = (T*)ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent1, &l_Error );
        bolt::cl::wait(ctl, l_histEvent1);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );

        printf("\n\nAfter Scan bits = %d", bits);
        for (int ng=0; ng<numGroups; ng++)
        { printf ("\nGroup-Block =%d",ng);
            for(int gS=0;gS<groupSize; gS++)
            { printf ("\nGroup =%d\n",gS);
                for(int i=0; i<RADICES;i++)
                {
                    size_t index = ng * groupSize * RADICES + gS * RADICES + i;
                    int value = histBuffer1[ index ];
                    printf("%x %x, ",index, value);
                }
            }
        }
        ctl.commandQueue().enqueueUnmapMemObject(clHistData, histBuffer1);
#endif 
                        V_OPENCL( permuteKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( permuteKernel.setArg(1, mask),  "Error setting a kernel argument" );
                        if (swap == 0)
                            V_OPENCL( permuteKernel.setArg(2, clInputData), "Error setting kernel argument" );
                        else
                            V_OPENCL( permuteKernel.setArg(2, clSwapData), "Error setting kernel argument" );
                        V_OPENCL( permuteKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( permuteKernel.setArg(4, bits), "Error setting a kernel argument" );
                        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );

                        if (swap == 0)
                            V_OPENCL( permuteKernel.setArg(5, clSwapData), "Error setting kernel argument" );
                        else
                            V_OPENCL( permuteKernel.setArg(5, clInputData), "Error setting kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            permuteKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
#if (DEBUG==1)
            T *swapBuffer;// = (T*)malloc(szElements * sizeof(T));
            ::cl::Event l_swapEvent;
            if(bits==0 || bits==16)
            //if(bits==0 || bits==8 || bits==16 || bits==24)
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clSwapData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clSwapData, swapBuffer);
            }
            else
            {
                swapBuffer = (T*)ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
                V_OPENCL( l_Error, "Error calling map on the result buffer" );
                bolt::cl::wait(ctl, l_swapEvent);
                printf("\n Printing swap data\n");
                for(int i=0; i<szElements;i+= RADICES)
                {
                    for(int j =0;j< RADICES;j++)
                        printf("%x %x, ",i+j,swapBuffer[i+j]);
                    printf("\n");
                }
                ctl.commandQueue().enqueueUnmapMemObject(clInputData, swapBuffer);
            }


#endif
                        if(swap==0)
                            swap = 1;
                        else
                            swap = 0;
                        if(bits == ((sizeof(T) * 8) - 2*RADIX))
                            mask = (1 << (RADIX-1) ) - 1;
                    }

                    //Sort the sign bit in the reverse direction
                    /**************************************/
                    //printf("\n******Final Bits*****\n");
                    
#if 1
                        if(comp(2,3))
                        {
                            /*Descending Sort*/
                            histKernel = radixSortIntKernels[1];
                            scanLocalKernel = radixSortIntKernels[2];
                            permuteKernel = radixSortIntKernels[4];
                        }
                        else
                        {
                            /*Ascending Sort*/
                            histKernel = radixSortIntKernels[0];
                            scanLocalKernel = radixSortIntKernels[2];
                            permuteKernel = radixSortIntKernels[3];
                        }
                        mask = 1;//(1<<(RADIX-1)); //only the sign bit needs to be set 
                        bits = sizeof(T) - 1; // We take the MSB byte or nibble
                        //Do a histogram pass locally
                        V_OPENCL( histKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(1, mask), "Error setting a kernel argument" );
                            V_OPENCL( histKernel.setArg(2, clInputData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(4, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( histKernel.setArg(5, bits), "Error setting a kernel argument" );
                        //V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            histKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

                        //Perform a global scan 
                        detail::scan_enqueue(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin(), 0, plus< T >( ));

                        //Add the results of the global scan to the local scan buffers
                        V_OPENCL( scanLocalKernel.setArg(0, RADIX), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(1, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
                        V_OPENCL( scanLocalKernel.setArg(3, localScanArray), "Error setting a kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            scanLocalKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );


                        V_OPENCL( permuteKernel.setArg(0, RADIX), "Error setting a kernel argument radix" );
                        V_OPENCL( permuteKernel.setArg(1, mask),  "Error setting a kernel argument mask" );
                            V_OPENCL( permuteKernel.setArg(2, clInputData), "Error setting kernel argument" );
                        V_OPENCL( permuteKernel.setArg(3, clHistData), "Error setting a kernel argument" );
                        V_OPENCL( permuteKernel.setArg(4, bits), "Error setting a kernel argument bits" );
                        //V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );
                            V_OPENCL( permuteKernel.setArg(5, clSwapData), "Error setting kernel argument" );
                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                                            permuteKernel,
                                            ::cl::NullRange,
                                            ::cl::NDRange(szElements/RADICES),
                                            ::cl::NDRange(groupSize),
                                            NULL,
                                            NULL);
                        //V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
                    ::cl::Event copy_event;
                    ctl.commandQueue().enqueueCopyBuffer(clSwapData, clInputData, 0, 0, szElements*sizeof(T), NULL, &copy_event);
                    copy_event.wait();
                    /**************************************/
#endif
                    if(newBuffer == true)
                    {
                        //::cl::copy(clInputData, first, last);
                        ctl.commandQueue().enqueueCopyBuffer( dvInputData.begin( )->getBuffer( ), first->getBuffer( ), 0, 0, sizeof(T)*(last-first), NULL, NULL );
                    }
                    return;
            }

            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< !(std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value || 
                                       std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type,          int >::value) >::type
            sort_enqueue(control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code)  
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    size_t szElements = (size_t)(last - first);
                    if(((szElements-1) & (szElements)) != 0)
                    {
                        sort_enqueue_non_powerOf2(ctl,first,last,comp,cl_code);
                        return;
                    }
                    static  boost::once_flag initOnlyOnce;
                    static  ::cl::Kernel masterKernel;

                    size_t temp;

                    // Set up shape of launch grid and buffers:
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    int resultCnt = computeUnits * wgPerComputeUnit;
                    cl_int l_Error = CL_SUCCESS;

                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.

                    if (boost::is_same<T, StrictWeakOrdering>::value) 
                        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompile, &masterKernel, 
                                      "\n//--User Code\n" + cl_code + 
                                      "\n//--typedef T Code\n" + ClCode<T>::get(),
                                      TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );
                    else
                        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompile, &masterKernel, 
                                      "\n//--User Code\n" + cl_code + 
                                      "\n//--typedef T Code\n" + ClCode<T>::get() + 
                                      "\n//--typedef StrictWeakOrdering Code\n" + ClCode<StrictWeakOrdering>::get(), 
                                      TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );

                    size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    if((szElements/2) < wgSize)
                    {
                        wgSize = (int)szElements/2;
                    }
                    unsigned int numStages,stage,passOfStage;

                    ::cl::Buffer A = first->getBuffer( );
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
            void sort_enqueue_non_powerOf2(control &ctl, const DVRandomAccessIterator& first, const DVRandomAccessIterator& last,
                const StrictWeakOrdering& comp, const std::string& cl_code)  
            {
                    //std::cout << "The BOLT sort routine does not support non power of 2 buffer size. Falling back to CPU std::sort" << std ::endl;
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    static boost::once_flag initOnlyOnce;
                    size_t szElements = (size_t)(last - first);

                    // Set up shape of launch grid and buffers:
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                    cl_int l_Error = CL_SUCCESS;

                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                    static std::vector< ::cl::Kernel > sortKernels;

                    if (boost::is_same<T, StrictWeakOrdering>::value) 
                        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileSelectionSort, &sortKernels, 
                                      "\n//--User Code\n" + cl_code + 
                                      "\n//--typedef T Code\n" + ClCode<T>::get(), 
                                      TypeName<T>::get(), 
                                      TypeName<StrictWeakOrdering>::get(), &ctl) );
                    else
                        boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileSelectionSort, &sortKernels, 
                                      "\n//--User Code\n" + cl_code + 
                                      "\n//--typedef T Code\n" + ClCode<T>::get() + 
                                      "\n//--typedef StrictWeakOrdering Code\n" + ClCode<StrictWeakOrdering>::get(), 
                                      TypeName<T>::get(), 
                                      TypeName<StrictWeakOrdering>::get(), &ctl) );

                    size_t wgSize  = sortKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    
                    size_t totalWorkGroups = (szElements + wgSize)/wgSize;
                    size_t globalSize = totalWorkGroups * wgSize;
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    
                    ::cl::Buffer& in = first->getBuffer( );
                    // ::cl::Buffer out(ctl.context(), CL_MEM_READ_WRITE, sizeof(T)*szElements);
                    control::buffPointer out = ctl.acquireBuffer( sizeof(T)*szElements );

                    ALIGNED( 256 ) StrictWeakOrdering aligned_comp( comp );
                    // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof( aligned_comp ), &aligned_comp );   // Create buffer wrapper so we can access host parameters.
                    control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_comp ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_comp );

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
                    ctl.commandQueue().enqueueMapBuffer(in, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sizeof(T) * szElements, NULL, NULL, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );

                    return;
            }// END of sort_enqueue_non_powerOf2

        }//namespace bolt::cl::detail
    }//namespace bolt::cl
}//namespace bolt

#endif