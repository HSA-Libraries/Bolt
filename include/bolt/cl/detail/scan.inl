#if !defined( SCAN_INL )
#define SCAN_INL

#include <bolt/cl/transform.h>
#include <algorithm>
#include <mutex>
#include <type_traits>

// #include <boost/thread/once.hpp>

namespace bolt
{
    namespace cl
    {
        //  Inclusive scan overloads
        //////////////////////////////////////////
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result,
            BinaryFunction binary_op, std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
            //  to a temporary buffer.  Should we?
            throw ::cl::Error( CL_INVALID_OPERATION, "Scan currently only supports random access iterator types" );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result,
            BinaryFunction binary_op, std::random_access_iterator_tag )
        {
            return detail::inclusive_scan( ctl, first, last, result, binary_op );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
        {
            return inclusive_scan( ctl, first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;

            return inclusive_scan( ctl, first, last, result, plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
        {
            return inclusive_scan( control::getDefault( ), first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator inclusive_scan( InputIterator first, InputIterator last, OutputIterator result )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;

            return inclusive_scan( control::getDefault( ), first, last, result, plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        //  Exclusive scan overloads
        //////////////////////////////////////////
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result,
            BinaryFunction binary_op, std::input_iterator_tag )
        {
            //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
            //  to a temporary buffer.  Should we?
            throw ::cl::Error( CL_INVALID_OPERATION, "Scan currently only supports random access iterator types" );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        OutputIterator exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result,
            BinaryFunction binary_op, std::random_access_iterator_tag )
        {
            return detail::exclusive_scan( ctl, first, last, result, binary_op );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
        {
            return exclusive_scan( ctl, first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;

            return exclusive_scan( ctl, first, last, result, plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator, typename BinaryFunction > 
        OutputIterator exclusive_scan( InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
        {
            return exclusive_scan( control::getDefault( ), first, last, result, binary_op, std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        template< typename InputIterator, typename OutputIterator >
        OutputIterator exclusive_scan( InputIterator first, InputIterator last, OutputIterator result )
        {
            typedef std::iterator_traits<InputIterator>::value_type T;

            return exclusive_scan( control::getDefault( ), first, last, result, plus< T >( ), std::iterator_traits< InputIterator >::iterator_category( ) );
        };

        namespace detail
        {
            /*! \brief Class used with shared_ptr<> as a custom deleter, to unmap a buffer that has been mapped with 
            *   device_vector data() method
            */
            class UnMapBufferFunctor
            {
                ::cl::CommandQueue& m_Queue;
                ::cl::Buffer& m_Buffer;

            public:
                //  Basic constructor requires a reference to the container and a positional element
                UnMapBufferFunctor( ::cl::Buffer& buffer, ::cl::CommandQueue& queue ): m_Buffer( buffer ), m_Queue( queue )
                {}

                void operator( )( const void* pBuff )
                {
                    V_OPENCL( m_Queue.enqueueUnmapMemObject( m_Buffer, const_cast< void* >( pBuff ) ),
                            "shared_ptr failed to unmap host memory back to device memory" );
                }
            };

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

        // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
        // This is called strictly for any non-device_vector iterator
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, OutputIterator >::type
            inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
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

                // TODO:  Create a device_vector constructor that takes an iterator and a size
                unsigned int elemBytes = numElements * sizeof( oType );
                ::cl::Buffer output( ctl.context( ), CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, elemBytes, const_cast< oType* >( &*result ) );
                device_vector< oType > dvOutput( output, ctl );

                //Now call the actual cl algorithm
                inclusive_scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), binary_op );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            return result + numElements;
        }

        // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
        // This is called strictly for iterators that are derived from device_vector< T >::iterator
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, OutputIterator >::type
            inclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
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
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );

                //std::partial_sum( first, last, result, binary_op );
                return result;
            }
            else if( runMode == bolt::cl::control::MultiCoreCpu )
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
                return result;
            }

            //Now call the actual cl algorithm
            inclusive_scan_enqueue( ctl, first, last, result, binary_op );

            return result + numElements;
        }

        //  Exclusive scan overloads
        //////////////////////////////////////////

        // This template is called by the non-detail versions of exclusive_scan, it already assumes random access iterators
        // This is called strictly for any non-device_vector iterator
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, OutputIterator >::type
            exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
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

                // TODO:  Create a device_vector constructor that takes an iterator and a size
                unsigned int elemBytes = numElements * sizeof( oType );
                ::cl::Buffer output( ctl.context( ), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, elemBytes, const_cast< oType* >( &*result ) );
                device_vector< oType > dvOutput( output, ctl );

                //Now call the actual cl algorithm
                inclusive_scan_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), binary_op );
                dvInput.data( );

                //  TODO:  This extra pass over the data is unnecessary; future optimization should fold this subtraction into the kernels
                //  BUG:  This hack doesn't work if the inclusive_scan is inplace
                transform( ctl, dvOutput.begin( ), dvOutput.end( ), dvInput.begin( ), dvOutput.begin( ), minus< oType >( ) );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            return result + numElements;
        }

        // This template is called by the non-detail versions of exclusive_scan, it already assumes random access iterators
        // This is called strictly for iterators that are derived from device_vector< T >::iterator
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
        typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, OutputIterator >::type
            exclusive_scan( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
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
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );

                //std::partial_sum( first, last, result, binary_op );
                return result;
            }
            else if( runMode == bolt::cl::control::MultiCoreCpu )
            {
                //  TODO:  Need access to the device_vector .data method to get a host pointer
                throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
                return result;
            }

            //Now call the actual cl algorithm
            inclusive_scan_enqueue( ctl, first, last, result, binary_op );

            //  TODO:  This extra pass over the data is unnecessary; future optimization should fold this subtraction into the kernels
            //  BUG:  This hack doesn't work if the inclusive_scan is inplace
            transform( ctl, dvOutput.begin( ), dvOutput.end( ), dvInput.begin( ), dvOutput.begin( ), minus< oType >( ) );

            return result + numElements;
        }

        //  All calls to inclusive_scan end up here, unless an exception was thrown
        //  This is the function that sets up the kernels to compile (once only) and execute
        template< typename InputIterator, typename OutputIterator, typename BinaryFunction >
            void inclusive_scan_enqueue( const control &ctl, InputIterator first, InputIterator last, OutputIterator result, BinaryFunction binary_op )
            {
                typedef typename std::iterator_traits< InputIterator >::value_type iType;
                typedef typename std::iterator_traits< OutputIterator >::value_type oType;

                static std::once_flag scanCompileFlag;
                static std::vector< ::cl::Kernel > scanKernels;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once( scanCompileFlag, detail::CompileTemplate::ScanSpecialization, &scanKernels, ClCode< BinaryFunction >::get( ), TypeName< iType >::get( ), TypeName< BinaryFunction >::get( ), ctl );

                // Set up shape of launch grid and buffers:
                int computeUnits     = ctl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
                int wgPerComputeUnit =  ctl.wgPerComputeUnit( );
                int resultCnt = computeUnits * wgPerComputeUnit;
                static const cl_uint waveSize  = 64; // FIXME, read from device attributes.
                static_assert( (waveSize & (waveSize-1)) == 0, "Scan depends on wavefronts being a power of 2" );

                //  Ceiling function to bump the size of input to the next whole wavefront size
                cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );

                device_vector< iType >::size_type sizeInputBuff = numElements;
                size_t modWaveFront = (sizeInputBuff & (waveSize-1));
                if( modWaveFront )
                {
                    sizeInputBuff &= ~modWaveFront;
                    sizeInputBuff += waveSize;
                }

                cl_uint numWorkGroups = static_cast< cl_uint >( sizeInputBuff / waveSize );

                //  Ceiling function to bump the size of the sum array to the next whole wavefront size
                device_vector< iType >::size_type sizeScanBuff = numWorkGroups;
                modWaveFront = (sizeScanBuff & (waveSize-1));
                if( modWaveFront )
                {
                    sizeScanBuff &= ~modWaveFront;
                    sizeScanBuff += waveSize;
                }

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer userFunctor( ctl.context( ), CL_MEM_USE_HOST_PTR, sizeof( binary_op ), &binary_op );   // Create buffer wrapper so we can access host parameters.
                //std::cout << "sizeof(Functor)=" << sizeof(binary_op) << std::endl;

                ::cl::Buffer preSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff * sizeof( iType ) );
                ::cl::Buffer postSumArray( ctl.context( ), CL_MEM_READ_WRITE, sizeScanBuff * sizeof( iType ) );

                cl_int l_Error = CL_SUCCESS;

                V_OPENCL( scanKernels[ 0 ].setArg( 0, (*result).getBuffer( ) ), "Error setting 0th argument for scanKernels[ 0 ]" );        // Output buffer
                V_OPENCL( scanKernels[ 0 ].setArg( 1, (*first).getBuffer( ) ), "Error setting 1st argument for scanKernels[ 0 ]" );       // Input buffer
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

                ////Map the buffer back to the host
                //boost::shared_array< iType > pPreSum( reinterpret_cast< value_type* >( ctl.commandQueue( ).enqueueMapBuffer( preSumArray, true, CL_MAP_READ, 0, sizeScanBuff * sizeof( value_type ) ) ), 
                //    UnMapBufferFunctor( preSumArray, ctl.commandQueue( ) ) );
                //boost::shared_array< iType > pB( reinterpret_cast< value_type* >( ctl.commandQueue( ).enqueueMapBuffer( B, true, CL_MAP_READ, 0, dvInput.size( ) * sizeof( value_type ) ) ), 
                //    UnMapBufferFunctor( B, ctl.commandQueue( ) ) );

                //pB.reset( );
                //pPreSum.reset( );
                //l_Error = ctl.commandQueue( ).enqueueWaitForEvents( intraScanEvent );
                //V_OPENCL( l_Error, "intraScanEvent failed to wait" );
                //return;

                cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / waveSize );

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

                V_OPENCL( scanKernels[ 2 ].setArg( 0, (*result).getBuffer( ) ), "Error setting 0th argument for scanKernels[ 2 ]" );          // Output buffer
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

                //l_Error = ctl.commandQueue( ).enqueueWaitForEvents( perBlockEvent );
                //V_OPENCL( l_Error, "perBlockInclusiveScan failed to wait" );

            }   //end of inclusive_scan_enqueue( )
        }   //namespace detail
    }   //namespace cl
}//namespace bolt

#endif