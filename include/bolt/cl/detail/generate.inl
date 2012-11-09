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

#if !defined( GENERATE_INL )
#define GENERATE_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 

#define STATIC /*static*/  /* FIXME - hack to approximate buffer pool management of the functor containing the buffer */

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {

        // default control, start->stop
        template<typename ForwardIterator, typename Generator> 
        void generate( ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( bolt::cl::control::getDefault(), first, last, gen, cl_code, std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        // user specified control, start->stop
        template<typename ForwardIterator, typename Generator> 
        void generate( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( ctl, first, last, gen, cl_code, std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

        // default control, start-> +n
        template<typename OutputIterator, typename Size, typename Generator> 
        OutputIterator generate_n( OutputIterator first, Size n, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( bolt::cl::control::getDefault(), first, first+n, gen, cl_code, std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
        }

        // user specified control, start-> +n
        template<typename OutputIterator, typename Size, typename Generator> 
        OutputIterator generate_n( const bolt::cl::control &ctl, OutputIterator first, Size n, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( ctl, first, n, gen, cl_code, std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
        }

    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
    namespace cl {
        namespace detail {

            struct CallCompiler_Generator {
                static void init_(std::vector< ::cl::Kernel >* kernels, std::string cl_code, std::string valueTypeName, std::string generatorTypeName, const control *ctl) {

                    std::vector< const std::string > kernelNames;
                    kernelNames.push_back( "generateNoBoundaryCheck" );
                    kernelNames.push_back( "generate" );
                    kernelNames.push_back( "generateSingleThread" );

                    std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and generator\n"
                        "template __attribute__((mangled_name(generateNoBoundaryCheckInstantiated)))\n"
                        "kernel void generateNoBoundaryCheckTemplate(\n"
                        "global " + valueTypeName + " *Z,\n"
                        "const int length,\n"
                        "global " + generatorTypeName + " *gen);\n\n"

                        "// Host generates this instantiation string with user-specified value type and generator\n"
                        "template __attribute__((mangled_name(generateInstantiated)))\n"
                        "kernel void generateTemplate(\n"
                        "global " + valueTypeName + " *Z,\n"
                        "const int length,\n"
                        "global " + generatorTypeName + " *gen);\n\n"

                        "// Host generates this instantiation string with user-specified value type and generator\n"
                        "template __attribute__((mangled_name(generateSingleThreadInstantiated)))\n"
                        "kernel void generateSingleThreadTemplate(\n"
                        "global " + valueTypeName + " *Z,\n"
                        "const int length,\n"
                        "global " + generatorTypeName + " *gen);\n\n"
                        
                        ;

                    bolt::cl::compileKernelsString( *kernels, kernelNames, generate_kernels, instantiationString, cl_code, valueTypeName,  generatorTypeName, *ctl);
                };
            };

            /*****************************************************************************
             * Random Access
             ****************************************************************************/

            // generate, not random-access
            template<typename ForwardIterator, typename Generator>
            void generate_detect_random_access( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, Generator gen, const std::string &cl_code, std::forward_iterator_tag )
            {
                static_assert( false, "Bolt only supports random access iterator types" );
            }

            // generate, yes random-access
            template<typename ForwardIterator, typename Generator>
            void generate_detect_random_access( const bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, Generator gen, const std::string &cl_code, std::random_access_iterator_tag )
            {
                generate_pick_iterator(ctl, first, last, gen, cl_code);
            }
           

            /*****************************************************************************
             * Pick Iterator
             ****************************************************************************/

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of generate, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename ForwardIterator, typename Generator> 
            typename std::enable_if< !(std::is_base_of<typename device_vector<typename std::iterator_traits<ForwardIterator>::value_type>::iterator, ForwardIterator>::value), void >::type
            generate_pick_iterator(const bolt::cl::control &ctl,  const ForwardIterator &first, const ForwardIterator &last, const Generator &gen, const std::string &user_code)
            {
                typedef std::iterator_traits<ForwardIterator>::value_type Type;

                size_t sz = (last - first); 
                if (sz < 1)
                    return;

                // Use host pointers memory since these arrays are only write once - no benefit to copying.
                // Map the forward iterator to a device_vector
                device_vector< Type > range( first, sz, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

                generate_enqueue( ctl, range.begin( ), range.end( ), gen, user_code );

                //printf("I'm here @ std::vector.\n"); fflush(stdout);
                //Sleep(3000);
                // This should immediately map/unmap the buffer
                range.data( );
            }

            // This template is called by the non-detail versions of generate, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVForwardIterator, typename Generator> 
            typename std::enable_if< (std::is_base_of<typename device_vector<typename std::iterator_traits<DVForwardIterator>::value_type>::iterator,DVForwardIterator>::value), void >::type
            generate_pick_iterator(const bolt::cl::control &ctl,  const DVForwardIterator &first, const DVForwardIterator &last, const Generator &gen, const std::string& user_code)
            {
                generate_enqueue( ctl, first, last, gen, user_code );
                /*for (DVForwardIterator iter = first; iter != last; iter++)
                {
                    printf("Val=%f\n", *iter);
                }*/
            }


            /*****************************************************************************
             * Enqueue
             ****************************************************************************/

            template< typename DVForwardIterator, typename Generator > 
            void generate_enqueue(const bolt::cl::control &ctl, const DVForwardIterator &first, const DVForwardIterator &last, const Generator &gen, const std::string& cl_code)
            {
                typedef std::iterator_traits<DVForwardIterator>::value_type Type;

                // how many elements to generate
                //size_t sz = std::distance( first, last );
                cl_uint sz = static_cast< cl_uint >( std::distance( first, last ) );
                if (sz < 1)
                    return;

                // copy generatory to device
                __declspec( align( 256 ) ) Generator aligned_generator( gen );
                ::cl::Buffer userGenerator(ctl.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof( aligned_generator ), const_cast< Generator* >( &aligned_generator ) );   // Create buffer wrapper so we can access host parameters.

                // compile kernels
                static std::vector< ::cl::Kernel > generateKernels;
                static boost::once_flag initOnlyOnce;
                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Generator::init_, &generateKernels, cl_code + ClCode<Type>::get() + ClCode<Generator>::get(), TypeName< Type >::get(), TypeName< Generator >::get(), &ctl ) );
                
                // name the kernels
                ::cl::Kernel kernelNoBoundaryCheck  = generateKernels[0];
                ::cl::Kernel kernelYesBoundaryCheck = generateKernels[1];
                ::cl::Kernel kernelSingleThread     = generateKernels[2];

                // query work group size
                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = kernelYesBoundaryCheck.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2
                size_t wgMultiple = sz;
                size_t lowerBits = ( sz & (wgSize-1) );

                // choose which kernel to use
                ::cl::Kernel k;
                if( lowerBits )
                {
                    //  Bump the workitem count to the next multiple of wgSize
                    wgMultiple &= ~lowerBits;
                    wgMultiple += wgSize;
                    k = kernelYesBoundaryCheck;
                }
                else
                {
                    k = kernelNoBoundaryCheck;
                }



#if 0
                if ((sz % wgSize) != 0) {
                    sz = sz + (wgSize - (sz % wgSize));
                    k = kernelYesBoundaryCheck;
                } else if(sz < wgSize) {  
                    sz = wgSize;
                    k = kernelYesBoundaryCheck; 
                } else {
                    k = kernelNoBoundaryCheck;
                }
#endif

                // set kernel arguments
                k.setArg(0, first->getBuffer() );
                k.setArg(1, sz );
                k.setArg(2, userGenerator);

                // enqueue kernel
                ::cl::Event generateEvent;
                l_Error = ctl.commandQueue().enqueueNDRangeKernel(
                    k, 
                    ::cl::NullRange,
                    ::cl::NDRange(wgMultiple),
                    ::cl::NDRange(wgSize),
                    NULL,
                    &generateEvent );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for generate() kernel" );

                // wait to kernel completion
                bolt::cl::wait(ctl, generateEvent);
            }; // end generate_enqueue

        }//End OF detail namespace
    }//End OF cl namespace
}//End OF bolt namespace

#endif
