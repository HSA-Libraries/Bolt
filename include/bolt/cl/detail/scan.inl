#include <algorithm>
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
            typedef typename std::iterator_traits< InputIterator >::value_type T;

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
                size_t szElements = std::distance( first, last );

                // Map the input iterator to a cl buffer
                ::cl::Buffer A( ctl.context( ), CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof( T ) * szElements, const_cast<T*>(&*first));
                ::cl::Buffer B( ctl.context( ), CL_MEM_ALLOC_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof( T ) * szElements, const_cast<T*>(&*result));

                //Now call the actual cl algorithm
                detail::inclusive_scan< T, BinaryFunction >( ctl, A, B, binary_op );

                //Map the buffer back to the host
                ctl.commandQueue( ).enqueueMapBuffer( A, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, szElements );
            }

            return result;
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
            struct CallCompiler_Sort
            {
                static void constructAndCompile( ::cl::Kernel* masterKernel,  std::string cl_code, std::string valueTypeName,  std::string functorTypeName, const control& ctl )
                {
                    const std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(scanInstantiated)))\n"
                        "__attribute__((reqd_work_group_size(1,1,1)))\n"
                        "kernel void scanTemplate(\n"
                        "global " + valueTypeName + "* A,\n"
                        "const uint stage,\n"
                        "const uint passOfStage,\n"
                        "global " + functorTypeName + "* userComp,\n"
                        "local " + valueTypeName + "* scratch\n"
                        ");\n\n";

                    bolt::cl::constructAndCompile( masterKernel, "scan", instantiationString, cl_code, valueTypeName, functorTypeName, ctl );
                }
            };

        template< typename value_type, typename BinaryFunction >
            void inclusive_scan( const bolt::cl::control &ctl, ::cl::Buffer A, ::cl::Buffer B, BinaryFunction comp )
            {
                static std::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;

                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                std::call_once( initOnlyOnce, detail::CallCompiler_Sort::constructAndCompile, &masterKernel, ClCode< BinaryFunction >::get( ), TypeName< value_type >::get( ), TypeName< BinaryFunction >::get( ), ctl );

                // Set up shape of launch grid and buffers:
                int computeUnits     = ctl.device().getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >();
                int wgPerComputeUnit =  ctl.wgPerComputeUnit(); 
                int resultCnt = computeUnits * wgPerComputeUnit;
                const int wgSize = 64; 
                unsigned int temp,numStages,stage,passOfStage;
                numStages = 0;

                // Create buffer wrappers so we can access the host functors, for read or writing in the kernel
                ::cl::Buffer userFunctor(ctl.context(), CL_MEM_USE_HOST_PTR, sizeof(comp), &comp );   // Create buffer wrapper so we can access host parameters.

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel. FIXME, doesn't work.

                int szElements = (int)A.getInfo< CL_MEM_SIZE >( ) / sizeof( value_type );  // FIXME - remove typecast.  Kernel only can handle 32-bit size...
                for(temp = szElements; temp > 1; temp >>= 1)
                    ++numStages;
                k.setArg(0, A);
                //1 and 2 we will add inside the loop.
                k.setArg(3, userFunctor);
                ::cl::LocalSpaceArg loc;
                loc.size_ = wgSize*sizeof( value_type );
                k.setArg(4, loc);

                for(stage = 0; stage < numStages; ++stage)
                {
                    // stage of the algorithm
                    k.setArg(1, stage);
                    // Every stage has stage + 1 passes
                    for(passOfStage = 0; passOfStage < stage + 1; ++passOfStage) {
                        // pass of the current stage
                        k.setArg(2, passOfStage);
                        /* 
                         * Enqueue a kernel run call.
                         * Each thread writes a sorted pair.
                         * So, the number of  threads (global) should be half the length of the input buffer.
                         */
                        ctl.commandQueue().enqueueNDRangeKernel(
                                k, 
                                ::cl::NullRange,
                                ::cl::NDRange(szElements/2),
                                ::cl::NDRange(1),
                                NULL,
                                NULL);
                        ctl.commandQueue().finish();
                    }//end of for passStage = 0:stage-1
                }//end of for stage = 0:numStage-1
            }//end of sort()
        }//namespace detail
    }//namespace cl
}//namespace bolt
