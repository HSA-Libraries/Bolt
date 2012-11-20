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

#include "bolt/cl/bolt.h"
#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/device_vector.h"


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
        void sort(const control &ctl,
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
        void sort(const control &ctl,
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

                static void constructAndCompileRadixSortUint(std::vector< ::cl::Kernel >* radixSortKernels,  std::string cl_code_dataType, std::string valueTypeName,  std::string compareTypeName, const control *ctl) {

                    std::vector< const std::string > kernelNames;
                    kernelNames.push_back( "histogram" );
                    kernelNames.push_back( "scanLocal" );
                    kernelNames.push_back( "permute" );


                    const std::string instantiationString = "";

                    bolt::cl::compileKernelsString( *radixSortKernels, kernelNames, sort_uint_kernels, instantiationString, cl_code_dataType, valueTypeName, "", *ctl );
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
                    // bolt::cl::compileKernels( *sortKernels, kernelNames, "sort", instantiationString, cl_code_dataType, valueTypeName, "", ctl );
                    //bolt::cl::constructAndCompile(masterKernel, "sort", instantiationString, cl_code_dataType, valueTypeName, "", ctl);
                }

            }; //End of struct CallCompiler_Sort  

            // Wrapper that uses default control class, iterator interface
            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename RandomAccessIterator, typename StrictWeakOrdering> 
            void sort_detect_random_access( const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code, std::random_access_iterator_tag )
            {
                return sort_pick_iterator(ctl, first, last, comp, cl_code);
            };

            //Device Vector specialization
            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVRandomAccessIterator>::value_type>::iterator,DVRandomAccessIterator>::value >::type
            sort_pick_iterator(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code) 
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
            sort_pick_iterator(const control &ctl, RandomAccessIterator first, RandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)  
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

	/*printf("\nSorted Result\n");
	for (int i=0; i< szElements; i+=1024)
	{
		printf ("%x %x, ", i , *(first +i));
	}
	printf("\nEnd of Sorted Result\n");*/

                    return;
                }
            }

/****** sort_enqueue specailization for unsigned int data types. ******
 * THE FOLLOWING CODE IMPLEMENTS THE RADIX SORT ALGORITHM FOR sort()
 *********************************************************************/
#define ENABLE_INCLUSIVE_SCAN 1
#define DEBUG 0
#if (ENABLE_INCLUSIVE_SCAN == 1)
            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
			typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
	        sort_enqueue(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    const int RADIX = 4;
                    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
					size_t szElements = (size_t)(last - first);

					//std::cout << "Calling unsigned int sort_enqueue sizeof T = "<< sizeof(T) << "\n";
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
					//std::cout << "CU = " << computeUnits << "wgPerComputeUnit = "<< wgPerComputeUnit << "\n";
					cl_int l_Error = CL_SUCCESS;

                    static  boost::once_flag initOnlyOnce;
					static std::vector< ::cl::Kernel > radixSortUintKernels;
					
                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
					boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileRadixSortUint, &radixSortUintKernels, cl_code +ClCode<T>::get(), "", TypeName<StrictWeakOrdering>::get(), &ctl) );
                    size_t groupSize  = radixSortUintKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
					V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
					groupSize = 16;
					size_t num_of_elems_per_group = RADICES  * groupSize;

					int i = 0;
					size_t mulFactor = groupSize * RADICES;

					if(szElements < mulFactor)
						szElements = mulFactor;
					else
						szElements = (szElements / mulFactor) * mulFactor;

					size_t numGroups = szElements / mulFactor;
					if(szElements%mulFactor != 0)
					{
                        sort_enqueue_non_powerOf2(ctl,first,last,comp,cl_code);
                        return;
					}

                    //Create local device_vector's 
					T *swapBuffer = (T*)malloc(szElements * sizeof(T));
					T *histBuffer = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
					T *histScanBuffer = (T*)calloc(1, numGroups* RADICES * sizeof(T));
                    
					
					device_vector< T > dvSwapInputData( swapBuffer, swapBuffer + szElements, CL_MEM_HOST_NO_ACCESS|CL_MEM_READ_WRITE, ctl);
					device_vector< T > dvHistogramBins( histBuffer, histBuffer+numGroups* groupSize * RADICES, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, ctl);
					device_vector< T > dvHistogramScanBuffer( histScanBuffer, histScanBuffer + numGroups* RADICES, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE, ctl);
                    
                    ::cl::Buffer clInputData = first->getBuffer( );
					::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );
					::cl::Buffer clSwapData = dvSwapInputData.begin( )->getBuffer( );
					::cl::Buffer clHistData = dvHistogramBins.begin( )->getBuffer( );
					::cl::Buffer clHistScanData = dvHistogramScanBuffer.begin( )->getBuffer( );
					
                    ::cl::Kernel histKernel = radixSortUintKernels[0];
                    ::cl::Kernel scanLocalKernel = radixSortUintKernels[1];  
					::cl::Kernel permuteKernel = radixSortUintKernels[2];  

					::cl::LocalSpaceArg loc;
					::cl::LocalSpaceArg localScanArray;
					loc.size_ = groupSize*RADICES* sizeof(cl_uint);
					localScanArray.size_ = 2*RADICES* sizeof(cl_uint);

					for(int bits = 0; bits < sizeof(T) * 8/*bits*/; bits += RADIX)
					{
                        //Do a histogram pass locally
						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
						else
							V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(3, bits), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
											histKernel,
											::cl::NullRange,
											::cl::NDRange(szElements/RADICES),
											::cl::NDRange(groupSize),
											NULL,
											NULL);
						V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
		::cl::Event l_histEvent;
        ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
		bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
		printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
		for (int ng=0; ng<numGroups; ng++)
		{ printf ("\nGroup-Block =%d",ng);
			for(int gS=0;gS<groupSize; gS++)
			{ printf ("\nGroup =%d\n",gS);
				for(int i=0; i<RADICES;i++)
				{
					size_t index = ng * groupSize * RADICES + gS * RADICES + i;
					int value = histBuffer[ index ];
			        printf("%d %d, ",index, value);
				}
			}
		}
#endif

                        //Perform a global scan 
                        bolt::cl::inclusive_scan(ctl, dvHistogramScanBuffer.begin(),dvHistogramScanBuffer.end(),dvHistogramScanBuffer.begin());
#if (DEBUG==1)
        ::cl::Event l_histScanBufferEvent;
        ctl.commandQueue().enqueueMapBuffer(clHistScanData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups * RADICES, NULL, &l_histScanBufferEvent, &l_Error );
		bolt::cl::wait(ctl, l_histScanBufferEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );
		for(int i=0; i<RADICES;i++)
		{ 
            printf ("\nRadix = %d\n",i);
			for (int ng=0; ng<2/*numGroups*/; ng++)
            {
                printf ("%d, ",histScanBuffer[i*numGroups + ng]);
            }
        }
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
                        V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
        //This map is required since the data is not available to the host when scanning.
		::cl::Event l_histEvent;
        ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histEvent, &l_Error );
		bolt::cl::wait(ctl, l_histEvent);
        V_OPENCL( l_Error, "Error calling map on the result buffer" );

		printf("\n\nAfter Scan");
		for (int ng=0; ng<numGroups; ng++)
		{ printf ("\nGroup-Block =%d",ng);
			for(int gS=0;gS<groupSize; gS++)
			{ printf ("\nGroup =%d\n",gS);
				for(int i=0; i<RADICES;i++)
				{
					size_t index = ng * groupSize * RADICES + gS * RADICES + i;
					int value = histBuffer[ index ];
			        printf("%d %d, ",index, value);
				}
			}
		}
#endif 

						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
						else
							V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
						V_OPENCL( permuteKernel.setArg(1, clHistData), "Error setting a kernel argument" );
						V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );
						V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );
				
						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( permuteKernel.setArg(4, clSwapData), "Error setting kernel argument" );
						else
							V_OPENCL( permuteKernel.setArg(4, clInputData), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
											permuteKernel,
											::cl::NullRange,
											::cl::NDRange(szElements/RADICES),
											::cl::NDRange(groupSize),
											NULL,
											NULL);
						V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
if(bits==0 || bits==8 || bits==16 || bits==24)
{
            ::cl::Event l_swapEvent;
            ctl.commandQueue().enqueueMapBuffer(clSwapData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_swapEvent, &l_Error );
            V_OPENCL( l_Error, "Error calling map on the result buffer" );
			bolt::cl::wait(ctl, l_swapEvent);
			printf("\n Printing swap data\n");
			for(int i=0; i<szElements;i+= 16)
			{
				for(int j =0;j< 16;j++)
					printf("%x %x, ",i+j,swapBuffer[i+j]);
				printf("\n");
			}
}
#endif
					}
                    free(swapBuffer);
                    free(histBuffer);
                    free(histScanBuffer);

                    ::cl::Event l_mapEvent;
                    ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_mapEvent, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );
					bolt::cl::wait(ctl, l_mapEvent);
                    return;
			}
#else
#define INCLUSIVE_SCAN 0
			template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
			typename std::enable_if< std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
	        sort_enqueue(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)
            {
                    typedef typename std::iterator_traits< DVRandomAccessIterator >::value_type T;
                    const int RADIX = 4;
                    const int RADICES = (1 << RADIX);	//Values handeled by each work-item?
					size_t szElements = (size_t)(last - first);

					//std::cout << "Calling unsigned int sort_enqueue sizeof T = "<< sizeof(T) << "\n";
                    int computeUnits     = ctl.device().getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                    int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
					//std::cout << "CU = " << computeUnits << "wgPerComputeUnit = "<< wgPerComputeUnit << "\n";
					cl_int l_Error = CL_SUCCESS;

                    static  boost::once_flag initOnlyOnce;
					static std::vector< ::cl::Kernel > radixSortUintKernels;
					
                    //Power of 2 buffer size
                    // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
					boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileRadixSortUint, &radixSortUintKernels, cl_code +ClCode<T>::get(), "", TypeName<StrictWeakOrdering>::get(), &ctl) );
                    size_t groupSize  = radixSortUintKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
					V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
					groupSize = 16;
					size_t num_of_elems_per_group = RADICES  * groupSize;

					int i = 0;
					size_t mulFactor = groupSize * RADICES;

					if(szElements < mulFactor)
						szElements = mulFactor;
					else
						szElements = (szElements / mulFactor) * mulFactor;

					size_t numGroups = szElements / mulFactor;
					if(szElements%mulFactor != 0)
					{
                        sort_enqueue_non_powerOf2(ctl,first,last,comp,cl_code);
                        return;
					}

                    //Create local device_vector's 
					T *swapBuffer = (T*)malloc(szElements * sizeof(T));
					T *histBuffer = (T*)malloc(numGroups* groupSize * RADICES * sizeof(T));
					T *histScanBuffer = (T*)calloc(1, numGroups* RADICES * sizeof(T));
					
					device_vector< T > dvSwapInputData( swapBuffer, swapBuffer + szElements, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE);
					device_vector< T > dvHistogramBins( histBuffer, histBuffer+numGroups* groupSize * RADICES, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE);
					device_vector< T > dvHistogramScanBuffer( histScanBuffer, histScanBuffer + numGroups* RADICES, CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE);
                    
                    ::cl::Buffer clInputData = first->getBuffer( );
					::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );
					::cl::Buffer clSwapData = dvSwapInputData.begin( )->getBuffer( );
					::cl::Buffer clHistData = dvHistogramBins.begin( )->getBuffer( );
					::cl::Buffer clHistScanData = dvHistogramScanBuffer.begin( )->getBuffer( );
					
                    ::cl::Kernel histKernel = radixSortUintKernels[0];     
                    ::cl::Kernel scanLocalKernel = radixSortUintKernels[1];  
					::cl::Kernel permuteKernel = radixSortUintKernels[2];  

					::cl::LocalSpaceArg loc;
					::cl::LocalSpaceArg localScanArray;
					loc.size_ = groupSize*RADICES* sizeof(cl_uint);
					localScanArray.size_ = 2*RADICES* sizeof(cl_uint);

					for(int bits = 0; bits < sizeof(T) * 8/*bits*/; bits += RADIX)
					{
						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( histKernel.setArg(0, clInputData), "Error setting a kernel argument" );
						else
							V_OPENCL( histKernel.setArg(0, clSwapData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(1, clHistData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(2, clHistScanData), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(3, bits), "Error setting a kernel argument" );
						V_OPENCL( histKernel.setArg(4, loc), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
											histKernel,
											::cl::NullRange,
											::cl::NDRange(szElements/RADICES),
											::cl::NDRange(groupSize),
											NULL,
											NULL);

						V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );

#if (DEBUG==1)
		printf("\n\n\n\n\nBITS = %d\nAfter Histogram", bits);
		for (int ng=0; ng<numGroups; ng++)
		{ printf ("\nGroup-Block =%d",ng);
			for(int gS=0;gS<groupSize; gS++)
			{ printf ("\nGroup =%d\n",gS);
				for(int i=0; i<RADICES;i++)
				{
					size_t index = ng * groupSize * RADICES + gS * RADICES + i;
					int value = histBuffer[ index ];
			        printf("%d %d, ",index, value);
				}
			}
		}

#endif
                        ::cl::Event l_histBufferEvent;
                        ctl.commandQueue().enqueueMapBuffer(clHistData, false, CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(T) * numGroups* groupSize * RADICES, NULL, &l_histBufferEvent, &l_Error );
						bolt::cl::wait(ctl, l_histBufferEvent);
                        V_OPENCL( l_Error, "Error calling map on the result buffer" );

						// Scan the histogram
						int sum = 0;
						for(int i = 0; i < RADICES; ++i)
						{
							for(int j = 0; j < numGroups; ++j)
							{
								for(int k = 0; k < groupSize; ++k)
								{
									size_t index = j * groupSize * RADICES + k * RADICES + i;
									int value = histBuffer[index];
									histBuffer[index] = sum;
									sum += value;
								}
							}
						}

#if (DEBUG==1)
		printf("\n\nAfter Scan");
		for (int ng=0; ng<numGroups; ng++)
		{ printf ("\nGroup-Block =%d",ng);
			for(int gS=0;gS<groupSize; gS++)
			{ printf ("\nGroup =%d\n",gS);
				for(int i=0; i<RADICES;i++)
				{
					size_t index = ng * groupSize * RADICES + gS * RADICES + i;
					int value = histBuffer[ index ];
			        printf("%d %d, ",index, value);
				}
			}
		}
#endif


						device_vector< T > dvScannedHistogramBins( histBuffer, histBuffer+(numGroups*groupSize*RADICES), CL_MEM_USE_HOST_PTR|CL_MEM_READ_WRITE);
                        ::cl::Buffer clScannedData = dvScannedHistogramBins.begin()->getBuffer();
						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( permuteKernel.setArg(0, clInputData), "Error setting kernel argument" );
						else
							V_OPENCL( permuteKernel.setArg(0, clSwapData), "Error setting kernel argument" );
						V_OPENCL( permuteKernel.setArg(1, clScannedData), "Error setting a kernel argument" );
						V_OPENCL( permuteKernel.setArg(2, bits), "Error setting a kernel argument" );
						V_OPENCL( permuteKernel.setArg(3, loc), "Error setting kernel argument" );
				
						if(bits==0 || bits==8 || bits==16 || bits==24)
							V_OPENCL( permuteKernel.setArg(4, clSwapData), "Error setting kernel argument" );
						else
							V_OPENCL( permuteKernel.setArg(4, clInputData), "Error setting kernel argument" );

                        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
											permuteKernel,
											::cl::NullRange,
											::cl::NDRange(szElements/RADICES),
											::cl::NDRange(groupSize),
											NULL,
											NULL);
						V_OPENCL( ctl.commandQueue().finish(), "Error calling finish on the command queue" );
					}
                    free(swapBuffer);
                    free(histBuffer);
                    free(histScanBuffer);

                    ::cl::Event l_mapEvent;
                    ctl.commandQueue().enqueueMapBuffer(clInputData, false, CL_MAP_READ, 0, sizeof(T) * szElements, NULL, &l_mapEvent, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );
					bolt::cl::wait(ctl, l_mapEvent);
                    return;
			}
#endif

            template<typename DVRandomAccessIterator, typename StrictWeakOrdering> 
			typename std::enable_if< !std::is_same< typename std::iterator_traits<DVRandomAccessIterator >::value_type, unsigned int >::value >::type
            sort_enqueue(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)  
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
                    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompile, &masterKernel, cl_code + ClCode<T>::get(), TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );

                    size_t wgSize  = masterKernel.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    if((szElements/2) < wgSize)
                    {
                        wgSize = (int)szElements/2;
                    }
                    unsigned int numStages,stage,passOfStage;

                    ::cl::Buffer A = first->getBuffer( );
                    ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.
    
                    ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.
                    numStages = 0;
                    for(temp = szElements; temp > 1; temp >>= 1)
                        ++numStages;
                    V_OPENCL( k.setArg(0, A), "Error setting a kernel argument" );
                    V_OPENCL( k.setArg(3, userFunctor), "Error setting a kernel argument" );
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
            void sort_enqueue_non_powerOf2(const control &ctl, DVRandomAccessIterator first, DVRandomAccessIterator last,
                StrictWeakOrdering comp, const std::string& cl_code)  
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
                    boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Sort::constructAndCompileSelectionSort, &sortKernels, cl_code + ClCode<T>::get(), TypeName<T>::get(), TypeName<StrictWeakOrdering>::get(), &ctl) );

                    size_t wgSize  = sortKernels[0].getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                    
                    size_t totalWorkGroups = (szElements + wgSize)/wgSize;
                    size_t globalSize = totalWorkGroups * wgSize;
                    V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                    
                    ::cl::Buffer in = first->getBuffer( );
                    ::cl::Buffer out(ctl.context(), CL_MEM_READ_WRITE, sizeof(T)*szElements);
                    ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.
                    ::cl::LocalSpaceArg loc;
                    loc.size_ = wgSize*sizeof(T);
    
                    V_OPENCL( sortKernels[0].setArg(0, in), "Error setting a kernel argument in" );
                    V_OPENCL( sortKernels[0].setArg(1, out), "Error setting a kernel argument out" );
                    V_OPENCL( sortKernels[0].setArg(2, userFunctor), "Error setting a kernel argument userFunctor" );
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

                    V_OPENCL( sortKernels[1].setArg(0, out), "Error setting a kernel argument in" );
                    V_OPENCL( sortKernels[1].setArg(1, in), "Error setting a kernel argument out" );
                    V_OPENCL( sortKernels[1].setArg(2, userFunctor), "Error setting a kernel argument userFunctor" );
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