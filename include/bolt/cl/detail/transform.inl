#if !defined( TRANSFORM_INL )
#define TRANSFORM_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {
        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
            BinaryFunction f, const std::string& user_code )
        {
            detail::transform_detect_random_access( ctl, first1, last1, first2, result, f, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
        void transform( InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
            BinaryFunction f, const std::string& user_code )
        {
            detail::transform_detect_random_access( control::getDefault(), first1, last1, first2, result, f, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
        void transform( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, OutputIterator result, 
            UnaryFunction f, const std::string& user_code )
        {
            detail::transform_unary_detect_random_access( ctl, first1, last1, result, f, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control, two-input transform, std:: iterator
        template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
        void transform( InputIterator first1, InputIterator last1, OutputIterator result, 
            UnaryFunction f, const std::string& user_code )
        {
            detail::transform_unary_detect_random_access( control::getDefault(), first1, last1, result, f, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
    namespace cl {
        namespace detail {
            struct CallCompiler_Transform {
                static void init_(::cl::Kernel *masterKernel, std::string user_code, std::string inValueTypeName, std::string outValueTypeName, std::string functorTypeName, const control &c) {

                    std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(transformInstantiated)))\n"
                        "kernel void transformTemplate(\n"
                        "global " + inValueTypeName + "* A,\n"
                        "global " + inValueTypeName + "* B,\n"
                        "global " + outValueTypeName + "* Z,\n"
                        "const int length,\n"
                        "global " + functorTypeName + "* userFunctor);\n\n";

                    bolt::cl::constructAndCompileString( masterKernel, "transform", transform_kernels, instantiationString, user_code, inValueTypeName,  functorTypeName, c);
                    // bolt::cl::constructAndCompile(masterKernel, "transform", instantiationString, user_code, valueTypeName, functorTypeName, c);
                };
            };
            struct CallCompiler_UnaryTransform {
                static void init_(::cl::Kernel *masterKernel, std::string user_code, std::string inValueTypeName, std::string outValueTypeName, std::string functorTypeName, const control &c) {

                    std::string instantiationString = 
                        "// Host generates this instantiation string with user-specified value type and functor\n"
                        "template __attribute__((mangled_name(transformInstantiated)))\n"
                        "kernel void unaryTransformTemplate(\n"
                        "global " + inValueTypeName + "* A,\n"
                        "global " + outValueTypeName + "* Z,\n"
                        "const int length,\n"
                        "global " + functorTypeName + "* userFunctor);\n\n";

                    bolt::cl::constructAndCompileString( masterKernel, "transform", transform_kernels, instantiationString, user_code, inValueTypeName, functorTypeName, c);
                };
            };

            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            void transform_detect_random_access( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
                BinaryFunction f, const std::string& user_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            void transform_detect_random_access( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
                BinaryFunction f, const std::string& user_code, std::random_access_iterator_tag )
            {
                return transform_pick_iterator( ctl, first1, last1, first2, result, f, user_code );
            };

            // Wrapper that uses default control class, iterator interface
            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            void transform_unary_detect_random_access( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, OutputIterator result, 
                UnaryFunction f, const std::string& user_code, std::input_iterator_tag )
            {
                //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
                //  to a temporary buffer.  Should we?
                static_assert( false, "Bolt only supports random access iterator types" );
            };

            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            void transform_unary_detect_random_access( const bolt::cl::control& ctl, InputIterator first1, InputIterator last1, OutputIterator result, 
                UnaryFunction f, const std::string& user_code, std::random_access_iterator_tag )
            {
                return transform_unary_pick_iterator( ctl, first1, last1, result, f, user_code );
            };

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
             * \bug The std::is_base_of logic is combining testing the InputIterator & OutputIterator iterators to see 
             *  if they derive from each other.  This should be broken into seperate tests, one for InputIterator and 1 for
             *  OutputIterator and combined with &&.
            */
            template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, void >::type
            transform_pick_iterator(const bolt::cl::control &ctl,  InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
                BinaryFunction f, const std::string& user_code)
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                typedef std::iterator_traits<OutputIterator>::value_type oType;
                size_t sz = (last1 - first1); 
                if (sz == 0)
                    return;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                device_vector< iType > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
                device_vector< iType > dvInput2( first2, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

                // Map the output iterator to a device_vector
                device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

                transform_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ), dvOutput.begin( ), f, user_code );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename DVOutputIterator, typename BinaryFunction> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVOutputIterator>::value, void >::type
            transform_pick_iterator(const bolt::cl::control &c,  DVInputIterator first1, DVInputIterator last1, DVInputIterator first2, DVOutputIterator result, 
                BinaryFunction f, const std::string& user_code)
            {
                transform_enqueue( c, first1, last1, first2, result, f, user_code );
            }

            /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
             * \bug The std::is_base_of logic is combining testing the InputIterator & OutputIterator iterators to see 
             *  if they derive from each other.  This should be broken into seperate tests, one for InputIterator and 1 for
             *  OutputIterator and combined with &&.
            */
            template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
            typename std::enable_if< !std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,OutputIterator>::value, void >::type
            transform_unary_pick_iterator(const bolt::cl::control &ctl,  InputIterator first, InputIterator last, OutputIterator result, 
                UnaryFunction f, const std::string& user_code)
            {
                typedef std::iterator_traits<InputIterator>::value_type iType;
                typedef std::iterator_traits<OutputIterator>::value_type oType;
                size_t sz = (last - first); 
                if (sz == 0)
                    return;

                // Use host pointers memory since these arrays are only read once - no benefit to copying.

                // Map the input iterator to a device_vector
                device_vector< iType > dvInput( first, last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
                // Map the output iterator to a device_vector
                device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

                transform_unary_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), f, user_code );

                // This should immediately map/unmap the buffer
                dvOutput.data( );
            }

            // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction> 
            typename std::enable_if< std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVOutputIterator>::value, void >::type
            transform_unary_pick_iterator(const bolt::cl::control &c,  DVInputIterator first, DVInputIterator last, DVOutputIterator result, 
                UnaryFunction f, const std::string& user_code)
            {
                transform_unary_enqueue( c, first, last, result, f, user_code );
            }


            template< typename DVInputIterator, typename DVOutputIterator, typename BinaryFunction > 
            void transform_enqueue(const bolt::cl::control &c,  DVInputIterator first1, DVInputIterator last1, DVInputIterator first2, DVOutputIterator result, 
                BinaryFunction f, const std::string& user_code)
            {
                typedef std::iterator_traits<DVInputIterator>::value_type iType;
                typedef std::iterator_traits<DVOutputIterator>::value_type oType;
                size_t sz = std::distance( first1, last1 );
                if (sz == 0)
                    return;
                ::cl::Buffer userFunctor(c.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(f), &f );   // Create buffer wrapper so we can access host parameters.
                //std::cout << "sizeof(Functor)=" << sizeof(f) << std::endl;

                static boost::once_flag initOnlyOnce;
                static  ::cl::Kernel masterKernel;
                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_Transform::init_, &masterKernel, user_code + ClCode<iType>::get() + ClCode<oType>::get() + ClCode<BinaryFunction>::get(), TypeName< iType >::get( ), TypeName< oType >::get( ), TypeName< BinaryFunction >::get( ), c ) );

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel.  FIXME, need to create-kernel from the program.

                k.setArg(0, first1->getBuffer( ) );
                k.setArg(1, first2->getBuffer( ) );
                k.setArg(2, result->getBuffer( ) );
                k.setArg(3, static_cast< cl_uint >( sz ) );
                k.setArg(4, userFunctor);

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = k.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( c.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                if(sz < wgSize)
                {
                    sz = wgSize;
                }
                else if ((sz % wgSize) != 0)
                {
                    sz = sz + (wgSize - (sz % wgSize));
                }

                l_Error = c.commandQueue().enqueueNDRangeKernel(
                    k, 
                    ::cl::NullRange, 
                    ::cl::NDRange(sz), 
                    ::cl::NDRange(wgSize));
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );
            };

            template< typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction > 
            void transform_unary_enqueue(const bolt::cl::control &c,  DVInputIterator first, DVInputIterator last, DVOutputIterator result, 
                UnaryFunction f, const std::string& user_code)
            {
                typedef std::iterator_traits<DVInputIterator>::value_type iType;
                typedef std::iterator_traits<DVOutputIterator>::value_type oType;
                size_t sz = std::distance( first, last );
                if (sz == 0)
                    return;
                ::cl::Buffer userFunctor(c.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(f), &f );   // Create buffer wrapper so we can access host parameters.

                static boost::once_flag initOnlyOnce;
                static ::cl::Kernel masterKernel;
                // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
                boost::call_once( initOnlyOnce, boost::bind( CallCompiler_UnaryTransform::init_, &masterKernel, user_code + ClCode<iType>::get() + ClCode<oType>::get() + ClCode<UnaryFunction>::get(), TypeName< iType >::get( ), TypeName< oType >::get( ), TypeName< UnaryFunction >::get( ), c ) );

                ::cl::Kernel k = masterKernel;  // hopefully create a copy of the kernel.  FIXME, need to create-kernel from the program.

                k.setArg(0, first->getBuffer( ) );
                k.setArg(1, result->getBuffer( ) );
                k.setArg(2, static_cast< cl_uint >( sz ) );
                k.setArg(3, userFunctor);

                cl_int l_Error = CL_SUCCESS;
                const size_t wgSize  = k.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( c.device( ), &l_Error );
                V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );

                if(sz < wgSize)
                {  
                    sz = wgSize;
                }
                else if ((sz % wgSize) != 0)
                {
                    sz = sz + (wgSize - (sz % wgSize));
                }

                l_Error = c.commandQueue().enqueueNDRangeKernel(
                    k, 
                    ::cl::NullRange, 
                    ::cl::NDRange(sz), 
                    ::cl::NDRange(wgSize));
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );
            };
        }//End OF detail namespace
    }//End OF cl namespace
}//End OF bolt namespace

#endif