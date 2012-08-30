#if !defined( SCAN_INL )
#define SCAN_INL

#include <algorithm>
#include <mutex>
#include <type_traits>

// #include <boost/thread/once.hpp>

namespace bolt
{
    namespace cl
    {
        // This template is called by all other "convenience" version of sort.
        // It also implements the CPU-side mappings of the algorithm for SerialCpu and MultiCoreCpu
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator
            inclusive_scan( const bolt::cl::control &ctl, InputIterator first, InputIterator last, 
            OutputIterator result, BinaryFunction binary_op )
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
                unsigned int elemBytes = numElements * sizeof( iType );

                // Map the input iterator to a cl buffer
                ::cl::Buffer A( ctl.context( ), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, elemBytes, const_cast< iType* >( &*first ) );
                ::cl::Buffer B( ctl.context( ), CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, elemBytes, const_cast< oType* >( &*result ) );

                //Now call the actual cl algorithm
                detail::inclusive_scan< iType, BinaryFunction >( ctl, numElements, A, B, binary_op );

                //Map the buffer back to the host
                ctl.commandQueue( ).enqueueMapBuffer( B, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, elemBytes );
            }

            return result + numElements;
        }


        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator inclusive_scan( InputIterator begin, InputIterator end, OutputIterator result,
            BinaryFunction binary_op, std::input_iterator_tag )
        {
            return inclusive_scan( bolt::cl::control::getDefault( ), begin, end, result, binary_op);
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
        {
            return inclusive_scan( first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;

            return inclusive_scan( first, last, result, bolt::cl::plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        namespace detail
        {
            // FIXME - move to cpp file
            struct CompileTemplate
            {
                static void ScanSpecialization( std::vector< ::cl::Kernel >* scanKernels, const std::string& cl_code, const std::string& valueTypeName, const std::string& functorTypeName, const control& ctl )
                {
                    // cl_code = template<typename T> struct plus { T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;} };
                    // valueTypeName = int
                    // functorTypeName = plus<int>
                    std::vector< const std::string > kernelNames;
                    kernelNames.push_back( "perBlockInclusiveScan" );
                    kernelNames.push_back( "intraBlockExclusiveScan" );
                    kernelNames.push_back( "perBlockAddition" );

                    const std::string templateSpecializationString = 
                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 0 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void " + kernelNames[ 0 ] + "(\n"
                        "global " + valueTypeName + "* output,\n"
                        "global " + valueTypeName + "* input,\n"
                        "const uint vecSize,\n"
                        "local volatile " + valueTypeName + "* lds,\n"
                        "global " + functorTypeName + "* binaryOp,\n"
                        "global " + valueTypeName + "* scanBuffer\n"
                        ");\n\n"

                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 1 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void " + kernelNames[ 1 ] + "(\n"
                        "global " + valueTypeName + "* postSumArray,\n"
                        "global " + valueTypeName + "* preSumArray,\n"
                        "const uint vecSize,\n"
                        "local volatile " + valueTypeName + "* lds,\n"
                        "const uint workPerThread,\n"
                        "global " + functorTypeName + "* binaryOp\n"
                        ");\n\n"

                        "// Dynamic specialization of generic template definition, using user supplied types\n"
                        "template __attribute__((mangled_name(" + kernelNames[ 2 ] + "Instantiated)))\n"
                        "__attribute__((reqd_work_group_size(64,1,1)))\n"
                        "kernel void " + kernelNames[ 2 ] + "(\n"
                        "global " + valueTypeName + "* output,\n"
                        "global " + valueTypeName + "* postSumArray,\n"
                        "const uint vecSize,\n"
                        "global " + functorTypeName + "* binaryOp\n"
                        ");\n\n";

                    bolt::cl::compileKernels( *scanKernels, kernelNames, "scan", templateSpecializationString, cl_code, valueTypeName, functorTypeName, ctl );
                }
            };

        template< typename value_type, typename BinaryFunction >
            void inclusive_scan( const bolt::cl::control& ctl, unsigned int numElements, ::cl::Buffer& A, ::cl::Buffer& B, BinaryFunction binary_op )
            {
                static std::once_flag scanCompileFlag;
                static std::vector< ::cl::Kernel > scanKernels;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once( scanCompileFlag, detail::CompileTemplate::ScanSpecialization, &scanKernels, ClCode< BinaryFunction >::get( ), TypeName< value_type >::get( ), TypeName< BinaryFunction >::get( ), ctl );

                // Set up shape of launch grid and buffers:
                int computeUnits     = ctl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
                int wgPerComputeUnit =  ctl.wgPerComputeUnit( );
                int resultCnt = computeUnits * wgPerComputeUnit;
                static const unsigned int waveSize  = 64; // FIXME, read from device attributes.
                static_assert( (waveSize & (waveSize-1)) == 0, "Scan depends on wavefronts being a power of 2" );

                //  Ceiling function to bump the size of input to the next whole wavefront size
                unsigned int sizeInputBuff = numElements;
                size_t modWaveFront = (numElements & (waveSize-1));
                if( modWaveFront )
                {
                    sizeInputBuff &= ~modWaveFront;
                    sizeInputBuff += waveSize;
                }

                unsigned int numWorkGroups = sizeInputBuff / waveSize;

                //  Ceiling function to bump the size of the sum array to the next whole wavefront size
                unsigned int sizeScanBuff = numWorkGroups;
                modWaveFront = (sizeScanBuff & (waveSize-1));
                if( modWaveFront )
                {
                    sizeScanBuff &= ~modWaveFront;
                    sizeScanBuff += waveSize;
                }

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer userFunctor( ctl.context( ), CL_MEM_USE_HOST_PTR, sizeof( binary_op ), &binary_op );   // Create buffer wrapper so we can access host parameters.
                //std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;

                ::cl::Buffer preSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff * sizeof( value_type ) );
                ::cl::Buffer postSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff * sizeof( value_type ) );

                cl_int l_Error = CL_SUCCESS;

                V_OPENCL( scanKernels[ 0 ].setArg( 0, B ), "Error setting 0th argument for scanKernels[ 0 ]" );        // Output buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 1, A ), "Error setting 1st argument for scanKernels[ 0 ]" );       // Input buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 2, numElements ), "Error setting 2nd argument for scanKernels[ 0 ]" );   // Size of scratch buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 3, waveSize + ( waveSize / 2 ), NULL ), "Error setting 3rd argument for scanKernels[ 0 ]" );    // Scratch buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 4, userFunctor ), "Error setting 4th argument for scanKernels[ 0 ]" );      // User provided functor class
                V_OPENCL( scanKernels[ 0 ].setArg( 5, preSumArray ), "Error setting 5th argument for scanKernels[ 0 ]" );      // Output per block sum buffer

                std::vector< ::cl::Event > intraScanEvent( 1 );

                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 0 ],
                    ::cl::NullRange,
                    ::cl::NDRange( sizeInputBuff ),
                    ::cl::NDRange( waveSize ),
                    NULL,
                    &intraScanEvent.front( ) );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

                //l_Error = ctl.commandQueue( ).enqueueWaitForEvents( intraScanEvent );
                //V_OPENCL( l_Error, "intraScanEvent failed to wait" );
                //return;

                unsigned int workPerThread = sizeScanBuff / waveSize;

                V_OPENCL( scanKernels[ 1 ].setArg( 0, postSumArray ), "Error setting 0th argument for scanKernels[ 1 ]" );          // Output buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 1, preSumArray ), "Error setting 1st argument for scanKernels[ 1 ]" );            // Input buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 2, numWorkGroups ), "Error setting 2nd argument for scanKernels[ 1 ]" );            // Size of scratch buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 3, waveSize + ( waveSize / 2 ), NULL ), "Error setting 3rd argument for scanKernels[ 1 ]" );  // Scratch buffer
                V_OPENCL( scanKernels[ 1 ].setArg( 4, workPerThread ), "Error setting 4th argument for scanKernels[ 1 ]" );           // User provided functor class
                V_OPENCL( scanKernels[ 1 ].setArg( 5, userFunctor ), "Error setting 5th argument for scanKernels[ 1 ]" );           // User provided functor class

                std::vector< ::cl::Event > interScanEvent( 1 );

                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 1 ],
                    ::cl::NullRange,
                    ::cl::NDRange( sizeScanBuff ),
                    ::cl::NDRange( waveSize ),
                    &intraScanEvent,
                    &interScanEvent.front( ) );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

                //l_Error = ctl.commandQueue( ).enqueueWaitForEvents( interScanEvent );
                //V_OPENCL( l_Error, "interScanEvent failed to wait" );
                //return;

                std::vector< ::cl::Event > perBlockEvent( 1 );

                V_OPENCL( scanKernels[ 2 ].setArg( 0, B ), "Error setting 0th argument for scanKernels[ 2 ]" );          // Output buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 1, postSumArray ), "Error setting 1st argument for scanKernels[ 2 ]" );            // Input buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 2, numElements ), "Error setting 2nd argument for scanKernels[ 2 ]" );   // Size of scratch buffer
                V_OPENCL( scanKernels[ 2 ].setArg( 3, userFunctor ), "Error setting 3rd argument for scanKernels[ 2 ]" );           // User provided functor class

                l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
                    scanKernels[ 2 ],
                    ::cl::NullRange,
                    ::cl::NDRange( sizeInputBuff ),
                    ::cl::NDRange( waveSize ),
                    &interScanEvent,
                    &perBlockEvent.front( ) );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for perBlockInclusiveScan kernel" );

                l_Error = ctl.commandQueue( ).enqueueWaitForEvents( perBlockEvent );
                V_OPENCL( l_Error, "perBlockInclusiveScan failed to wait" );
            }//end of inclusive_scan( )
        }//namespace detail
    }//namespace cl
}//namespace bolt

#endif