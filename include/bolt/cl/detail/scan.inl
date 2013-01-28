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
#define KERNEL0WAVES 4
#define KERNEL1WAVES 4
#define KERNEL2WAVES 4

#if !defined( SCAN_INL )
#define SCAN_INL

#include <algorithm>
#include <type_traits>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#include "bolt/cl/transform.h"
#include "bolt/cl/bolt.h"

namespace bolt
{
    namespace cl
    {
        //////////////////////////////////////////
        //  Inclusive scan overloads
        //////////////////////////////////////////
        template< typename InputIterator, typename OutputIterator >
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init; memset(&init, 0, sizeof(iType) );
            return detail::scan_detect_random_access( control::getDefault( ), first, last, result, init, true, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op, 
            const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init; memset(&init, 0, sizeof(iType) );
            return detail::scan_detect_random_access( control::getDefault( ), first, last, result, init, true, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator inclusive_scan( control &ctl, InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init; memset(&init, 0, sizeof(iType) );
            return detail::scan_detect_random_access( ctl, first, last, result, init, true, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator inclusive_scan( control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op, 
            const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init; memset(&init, 0, sizeof(iType) );
            return detail::scan_detect_random_access( ctl, first, last, result, init, true, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        //////////////////////////////////////////
        //  Exclusive scan overloads
        //////////////////////////////////////////
        template< typename InputIterator, typename OutputIterator >
        OutputIterator exclusive_scan( InputIterator first, InputIterator last, OutputIterator result, const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init; memset(&init, 0, sizeof(iType) );
            return detail::scan_detect_random_access( control::getDefault( ), first, last, result, init, false, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename T >
        OutputIterator exclusive_scan( InputIterator first, InputIterator last, OutputIterator result, T init, const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            return detail::scan_detect_random_access( control::getDefault( ), first, last, result, init, false, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction > 
        OutputIterator exclusive_scan( InputIterator first, InputIterator last, OutputIterator result, T init, BinaryFunction binary_op,
            const std::string& user_code )
        {
            return detail::scan_detect_random_access( control::getDefault( ), first, last, result, init, false, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code ) // assumes addition of numbers
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            iType init = static_cast< iType >( 0 );
            return detail::scan_detect_random_access( ctl, first, last, result, init, false, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename T >
        OutputIterator exclusive_scan( control &ctl, InputIterator first, InputIterator last, OutputIterator result, T init, 
            const std::string& user_code )
        {
            typedef std::iterator_traits<InputIterator>::value_type iType;
            return detail::scan_detect_random_access( ctl, first, last, result, init, false, plus< iType >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction > 
        OutputIterator exclusive_scan( control &ctl, InputIterator first, InputIterator last, OutputIterator result, T init, BinaryFunction binary_op,
            const std::string& user_code )
        {
            return detail::scan_detect_random_access( ctl, first, last, result, init, false, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        namespace detail
        {
            /*!
            *   \internal
            *   \addtogroup detail
            *   \ingroup scan
            *   \{
            */

            // FIXME - move to cpp file
            struct CompileTemplate
            {
                static void ScanSpecialization( std::vector< ::cl::Kernel >* scanKernels, const std::string& cl_code, const std::string& valueTypeName, const std::string& functorTypeName, const control* ctl )
                {
                    // cl_code = template<typename T> struct plus { T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;} };
                    // valueTypeName = int
                    // functorTypeName = plus<int>
                    std::vector< const std::string > kernelNames;
                    kernelNames.push_back( "perBlockInclusiveScan" );
                    kernelNames.push_back( "intraBlockInclusiveScan" );
                    kernelNames.push_back( "perBlockAddition" );

                    char k0WgSz[16];
                    sprintf_s(k0WgSz,"%i",(KERNEL0WAVES*64));
                    char k1WgSz[16];
                    sprintf_s(k1WgSz,"%i",(KERNEL1WAVES*64));
                    char k2WgSz[16];
                    sprintf_s(k2WgSz,"%i",(KERNEL2WAVES*64));

                    const std::string templateSpecializationString = 
                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 0 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size("+k0WgSz+",1,1)))\n"
                        "kernel void " + kernelNames[ 0 ] + "(\n"
                        "global " + valueTypeName + "* output,\n"
                        "global " + valueTypeName + "* input,\n"
                        "" + valueTypeName + " identity,\n"
                        "const uint vecSize,\n"
                        "local " + valueTypeName + "* lds,\n"
                        "global " + functorTypeName + "* binaryOp,\n"
                        "global " + valueTypeName + "* scanBuffer,\n"
                        "int exclusive\n"
                        ");\n\n"

                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 1 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size("+k1WgSz+",1,1)))\n"
                        "kernel void " + kernelNames[ 1 ] + "(\n"
                        "global " + valueTypeName + "* postSumArray,\n"
                        "global " + valueTypeName + "* preSumArray,\n"
                        ""+valueTypeName+" identity,\n"
                        "const uint vecSize,\n"
                        "local " + valueTypeName + "* lds,\n"
                        "const uint workPerThread,\n"
                        "global " + functorTypeName + "* binaryOp\n"
                        ");\n\n"

                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 2 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size("+k2WgSz+",1,1)))\n"
                        "kernel void " + kernelNames[ 2 ] + "(\n"
                        "global " + valueTypeName + "* output,\n"
                        "global " + valueTypeName + "* postSumArray,\n"
                        "const uint vecSize,\n"
                        "global " + functorTypeName + "* binaryOp\n"
                        ");\n\n";
                    if (0)
                    {
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_1 (Kernel Names)\n");
                        for (int i = 0; i < kernelNames.size(); i++)
                            printf("%s\n", kernelNames[i].c_str());
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_2 (Kernel String)\n%s\n\n", scan_kernels.c_str());
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_3 (Template Specialization String)\n%s\n", templateSpecializationString.c_str());
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_4 (User CL_Code String)\n%s\n", cl_code.c_str());
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_5 (Value TypeName String)\n%s\n", valueTypeName.c_str());
                        printf("\n\n\n################################################################################");
                        printf("COMPILE_ARG_6 (Functor TypeName String)\n%s\n", functorTypeName.c_str());
                        printf("\n\n\n################################################################################");
                    }
                    
                    bolt::cl::compileKernelsString( *scanKernels, kernelNames, scan_kernels, templateSpecializationString, cl_code, valueTypeName, functorTypeName, *ctl );
                }
            };

        template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
        OutputIterator scan_detect_random_access( control &ctl, const InputIterator& first, const InputIterator& last, const OutputIterator& result, const T& init, const bool& inclusive,
            BinaryFunction binary_op, std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
            //  to a temporary buffer.  Should we?
            static_assert( false, "Bolt only supports random access iterator types" );
        };

        template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
        OutputIterator scan_detect_random_access( control &ctl, const InputIterator& first, const InputIterator& last, const OutputIterator& result,  const T& init, const bool& inclusive,
            const BinaryFunction& binary_op, std::random_access_iterator_tag )
        {
            return detail::scan_pick_iterator( ctl, first, last, result, init, inclusive, binary_op );
        };

        /*! 
        * \brief This overload is called strictly for non-device_vector iterators
        * \details This template function overload is used to seperate device_vector iterators from all other iterators
        */
        template< typename InputIterator, typename OutputIterator, typename T, typename BinaryFunction >
        typename std::enable_if< 
                     !(std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
                       std::is_base_of<typename device_vector<typename std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
                 OutputIterator >::type
        scan_pick_iterator( control &ctl, const InputIterator& first, const InputIterator& last, const OutputIterator& result, const T& init, const bool& inclusive, const BinaryFunction& binary_op )
        {
            typedef typename std::iterator_traits< InputIterator >::value_type iType;
            typedef typename std::iterator_traits< OutputIterator >::value_type oType;
            static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

            unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
            if( numElements == 0 )
                return result;

            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.
            if( runMode == bolt::cl::control::SerialCpu )
            {
                std::partial_sum( first, last, result, binary_op );
                return result;
            }
            else if( runMode == bolt::cl::control::MultiCoreCpu )
            {
                std::cout << "The MultiCoreCpu version of inclusive_scan is not implemented yet." << std ::endl;
            }
            else
            {

                // Map the input iterator to a device_vector
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
                device_vector< oType > dvOutput( result, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

                //Now call the actual cl algorithm
                scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), init, binary_op, inclusive );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            return result + numElements;
        }

        /*! 
        * \brief This overload is called strictly for non-device_vector iterators
        * \details This template function overload is used to seperate device_vector iterators from all other iterators
        */
        template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
        typename std::enable_if< 
                      (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
                       std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
                 DVOutputIterator >::type

        scan_pick_iterator( control &ctl, const DVInputIterator& first, const DVInputIterator& last, const DVOutputIterator& result, const T& init, const bool& inclusive, const BinaryFunction& binary_op )
        {
            typedef typename std::iterator_traits< DVInputIterator >::value_type iType;
            typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
            static_assert( std::is_convertible< iType, oType >::value, "Input and Output iterators are incompatible" );

            unsigned int numElements = static_cast< unsigned int >( std::distance( first, last ) );
            if( numElements == 0 )
                return result;

            const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.
            if( runMode == bolt::cl::control::SerialCpu )
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
                return result;
            }
            else if( runMode == bolt::cl::control::MultiCoreCpu )
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
                return result;
            }

            //Now call the actual cl algorithm
            scan_enqueue( ctl, first, last, result, init, binary_op, inclusive );

            return result + numElements;
        }


        //  All calls to inclusive_scan end up here, unless an exception was thrown
        //  This is the function that sets up the kernels to compile (once only) and execute
        template< typename DVInputIterator, typename DVOutputIterator, typename T, typename BinaryFunction >
            void scan_enqueue( control &ctl, const DVInputIterator& first, const DVInputIterator& last, const DVOutputIterator& result, const T& init_T, const BinaryFunction& binary_op, const bool& inclusive = true )
            {
                typedef std::iterator_traits< DVInputIterator >::value_type iType;
                typedef std::iterator_traits< DVOutputIterator >::value_type oType;
                //cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );
                //iType identity = (iType)( init_T ); // identity of binary operator
                cl_uint doExclusiveScan = inclusive ? 0 : 1;

                static boost::once_flag scanCompileFlag;
                static std::vector< ::cl::Kernel > scanKernels;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( scanCompileFlag, boost::bind( CompileTemplate::ScanSpecialization, &scanKernels, ClCode<iType>::get()+ClCode< BinaryFunction >::get( ), TypeName< iType >::get( ), TypeName< BinaryFunction >::get( ), &ctl ) );

                cl_int l_Error = CL_SUCCESS;
                // for profiling
                ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent;

                // Set up shape of launch grid and buffers:
                int computeUnits     = ctl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
                int wgPerComputeUnit =  ctl.wgPerComputeUnit( );
                int resultCnt = computeUnits * wgPerComputeUnit;

                const size_t waveSize  = scanKernels.front( ).getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
                assert( (waveSize & (waveSize-1)) == 0 ); // WorkGroup must be a power of 2 for Scan to work
                const size_t maxWgSize = 4*waveSize;
                const size_t kernel0_WgSize = waveSize*KERNEL0WAVES;
                const size_t kernel1_WgSize = waveSize*KERNEL1WAVES;
                const size_t kernel2_WgSize = waveSize*KERNEL2WAVES;

                //  Ceiling function to bump the size of input to the next whole wavefront size
                cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );

                device_vector< iType >::size_type sizeInputBuff = numElements;
                size_t modWgSize = (sizeInputBuff & (maxWgSize-1));
                if( modWgSize )
                {
                    sizeInputBuff &= ~modWgSize;
                    sizeInputBuff += maxWgSize;
                }

                cl_uint numWorkGroupsK0 = static_cast< cl_uint >( sizeInputBuff / kernel0_WgSize );

                //  Ceiling function to bump the size of the sum array to the next whole wavefront size
                device_vector< iType >::size_type sizeScanBuff = numWorkGroupsK0;
                modWgSize = (sizeScanBuff & (maxWgSize-1));
                if( modWgSize )
                {
                    sizeScanBuff &= ~modWgSize;
                    sizeScanBuff += kernel0_WgSize;
                }

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ALIGNED( 256 ) BinaryFunction aligned_binary( binary_op );
                control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_binary ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary );
                control::buffPointer preSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( iType ) );
                control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( iType ) );
                //::cl::Buffer userFunctor( ctl.context( ), CL_MEM_USE_HOST_PTR, sizeof( binary_op ), &binary_op );
                //::cl::Buffer preSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );
                //::cl::Buffer postSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff*sizeof(iType) );
                cl_uint ldsSize;


                /**********************************************************************************
                 *  Kernel 0
                 *********************************************************************************/
                ldsSize  = static_cast< cl_uint >( ( kernel0_WgSize + ( kernel0_WgSize / 2 ) ) * sizeof( iType ) );
                V_OPENCL( scanKernels[ 0 ].setArg( 0, result.getBuffer( ) ),   "Error setting argument for scanKernels[ 0 ]" ); // Output buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 1, first.getBuffer( ) ),    "Error setting argument for scanKernels[ 0 ]" ); // Input buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 2, init_T ),                 "Error setting argument for scanKernels[ 0 ]" ); // Initial value used for exclusive scan
                V_OPENCL( scanKernels[ 0 ].setArg( 3, numElements ),            "Error setting argument for scanKernels[ 0 ]" ); // Size of scratch buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 4, ldsSize, NULL ),          "Error setting argument for scanKernels[ 0 ]" ); // Scratch buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 5, *userFunctor ),            "Error setting argument for scanKernels[ 0 ]" ); // User provided functor class
                V_OPENCL( scanKernels[ 0 ].setArg( 6, *preSumArray ),            "Error setting argument for scanKernels[ 0 ]" ); // Output per block sum buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 7, doExclusiveScan ),        "Error setting argument for scanKernels[ 0 ]" ); // Exclusive scan?

                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 0 ],
                    ::cl::NullRange,
                    ::cl::NDRange( sizeInputBuff ),
                    ::cl::NDRange( kernel0_WgSize ),
                    NULL,
                    &kernel0Event);
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

                //  Debug code
#if 0
                    // Enqueue the operation
                    V_OPENCL( ctl.commandQueue( ).finish( ), "Failed to call finish on the commandqueue" );

                    //  Look at the contents of those buffers
                    device_vector< oType >::pointer pResult     = result->getContainer( ).data( );
                    //device_vector< oType >::pointer pPreSum     = preSumArray.data( );

                    iType* pPreSumArray = (iType*)ctl.commandQueue().enqueueMapBuffer( preSumArray, true, CL_MAP_READ, 0, sizeScanBuff*sizeof(iType), NULL, NULL, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );

                    ::cl::Event unmapEvent;
                    V_OPENCL( ctl.commandQueue().enqueueUnmapMemObject( preSumArray, static_cast< void* >( pPreSumArray ), NULL, &unmapEvent ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                    V_OPENCL( unmapEvent.wait( ), "failed to wait for unmap event" );
#endif

                /**********************************************************************************
                 *  Kernel 1
                 *********************************************************************************/
                cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / kernel1_WgSize );
                V_OPENCL( scanKernels[ 1 ].setArg( 0, *postSumArray ), "Error setting 0th argument for scanKernels[ 1 ]" );          // Output buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 1, *preSumArray ), "Error setting 1st argument for scanKernels[ 1 ]" );            // Input buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 2, init_T ), "Error setting argument for scanKernels[ 1 ]" );   // Initial value used for exclusive scan
                V_OPENCL( scanKernels[ 1 ].setArg( 3, numWorkGroupsK0 ), "Error setting 2nd argument for scanKernels[ 1 ]" );            // Size of scratch buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 4, ldsSize, NULL ), "Error setting 3rd argument for scanKernels[ 1 ]" );  // Scratch buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 5, workPerThread ), "Error setting 4th argument for scanKernels[ 1 ]" );           // User provided functor class
                V_OPENCL( scanKernels[ 1 ].setArg( 6, *userFunctor ), "Error setting 5th argument for scanKernels[ 1 ]" );           // User provided functor class

                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 1 ],
                    ::cl::NullRange,
                    ::cl::NDRange( kernel1_WgSize ),
                    ::cl::NDRange( kernel1_WgSize ),
                    NULL,
                    &kernel1Event);
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

                //  Debug code
#if 0
                    // Enqueue the operation
                    V_OPENCL( ctl.commandQueue( ).finish( ), "Failed to call finish on the commandqueue" );

                    //  Look at the contents of those buffers
                    //device_vector< oType >::pointer pPreSum      = preSumArray.data( );
                    //device_vector< oType >::pointer pPostSum     = postSumArray.data( );

                    iType* pPreSumArray = (iType*)ctl.commandQueue().enqueueMapBuffer( preSumArray, true, CL_MAP_READ, 0, sizeScanBuff*sizeof(iType), NULL, NULL, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );

                    iType* pPostSumArray = (iType*)ctl.commandQueue().enqueueMapBuffer( postSumArray, true, CL_MAP_READ, 0, sizeScanBuff*sizeof(iType), NULL, NULL, &l_Error );
                    V_OPENCL( l_Error, "Error calling map on the result buffer" );

                    ::cl::Event unmapPreEvent;
                    V_OPENCL( ctl.commandQueue().enqueueUnmapMemObject( preSumArray, static_cast< void* >( pPreSumArray ), NULL, &unmapPreEvent ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                    V_OPENCL( unmapPreEvent.wait( ), "failed to wait for unmap event" );

                    ::cl::Event unmapPostEvent;
                    V_OPENCL( ctl.commandQueue().enqueueUnmapMemObject( postSumArray, static_cast< void* >( pPostSumArray ), NULL, &unmapPostEvent ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                    V_OPENCL( unmapPostEvent.wait( ), "failed to wait for unmap event" );
#endif

                /**********************************************************************************
                 *  Kernel 2
                 *********************************************************************************/
                //std::vector< ::cl::Event > perBlockEvent( 1 );
                V_OPENCL( scanKernels[ 2 ].setArg( 0, result.getBuffer( ) ), "Error setting 0th argument for scanKernels[ 2 ]" );          // Output buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 1, *postSumArray ), "Error setting 1st argument for scanKernels[ 2 ]" );            // Input buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 2, numElements ), "Error setting 2nd argument for scanKernels[ 2 ]" );   // Size of scratch buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 3, *userFunctor ), "Error setting 3rd argument for scanKernels[ 2 ]" );           // User provided functor class
                //printf("Kernel3 %i sizeInputBuff, %i wgSize\n", sizeInputBuff, kernel2_WgSize);
                try
                {
                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 2 ],
                    ::cl::NullRange,
                    ::cl::NDRange( sizeInputBuff ),
                    ::cl::NDRange( kernel2_WgSize ),
                    NULL,
                    &kernel2Event );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );
                }
                catch ( ::cl::Error& e )
                {
                    std::cout << ( "Kernel 3 enqueueNDRangeKernel error condition reported:" ) << std::endl << e.what() << std::endl;
                    return;
                }
                l_Error = kernel2Event.wait( );
                V_OPENCL( l_Error, "perBlockInclusiveScan failed to wait" );

                /**********************************************************************************
                 *  Print Kernel times
                 *********************************************************************************/
#if 0
                try
                {
                    double k0_globalMemory = 2.0*sizeInputBuff*sizeof(iType) + 1*sizeScanBuff*sizeof(iType);
                    double k1_globalMemory = 4.0*sizeScanBuff*sizeof(iType);
                    double k2_globalMemory = 2.0*sizeInputBuff*sizeof(iType) + 1*sizeScanBuff*sizeof(iType);
                    cl_ulong k0_start, k0_end, k1_start, k1_end, k2_start, k2_end;

                    l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
                    l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_end);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
                    
                    l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
                    l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_end);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
                    
                    l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
                    l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k2_end);
                    V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");

                    double k0_sec = (k0_end-k0_start)/1000000000.0;
                    double k1_sec = (k1_end-k1_start)/1000000000.0;
                    double k2_sec = (k2_end-k2_start)/1000000000.0;

                    double k0_GBs = k0_globalMemory/(1024*1024*1024*k0_sec);
                    double k1_GBs = k1_globalMemory/(1024*1024*1024*k1_sec);
                    double k2_GBs = k2_globalMemory/(1024*1024*1024*k2_sec);

                    double k0_ms = k0_sec*1000.0;
                    double k1_ms = k1_sec*1000.0;
                    double k2_ms = k2_sec*1000.0;

                    printf("Kernel Profile:\n\t%7.3f GB/s  (%4.0f MB in %6.3f ms)\n\t%7.3f GB/s  (%4.0f MB in %6.3f ms)\n\t%7.3f GB/s  (%4.0f MB in %6.3f ms)\n",
                        k0_GBs, k0_globalMemory/1024/1024, k0_ms,
                        k1_GBs, k1_globalMemory/1024/1024, k1_ms,
                        k2_GBs, k2_globalMemory/1024/1024, k2_ms);

                    size_t functorSize = userFunctor.getInfo< CL_MEM_SIZE >( &l_Error );
                    V_OPENCL( l_Error, "device_vector failed to request the size of the ::cl::Buffer object" );

                    printf("Functor Size = %i\n", functorSize);
                }
                catch( ::cl::Error& e )
                {
                    std::cout << ( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
                    return;
                }
#endif
                ////  Debug code
                //{
                //    // Enqueue the operation
                //    V_OPENCL( ctl.commandQueue( ).finish( ), "Failed to call finish on the commandqueue" );

                //    //  Look at the contents of those buffers
                //    device_vector< oType >::pointer pResult     = result->getContainer( ).data( );
                //}

            }   //end of inclusive_scan_enqueue( )

            /*!   \}  */
        }   //namespace detail

    }   //namespace cl
}//namespace bolt

#endif