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

#pragma once
#if !defined( TRANSFORM_INL )
#define TRANSFORM_INL

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 

#include "bolt/cl/bolt.h"
#include "bolt/cl/device_vector.h"
#include "bolt/cl/iterator/iterator_traits.h"

namespace bolt {
namespace cl {
    // two-input transform, std:: iterator
    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform( bolt::cl::control& ctl, InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, 
        OutputIterator result, BinaryFunction f, const std::string& user_code )
    {
        detail::transform_detect_random_access( ctl, first1, last1, first2, result, f, user_code, 
            std::iterator_traits< InputIterator1 >::iterator_category( ), 
            std::iterator_traits< InputIterator2 >::iterator_category( ) );
    }

    // default control, two-input transform, std:: iterator
    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform( InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, 
        BinaryFunction f, const std::string& user_code )
    {
            detail::transform_detect_random_access( control::getDefault(), first1, last1, first2, result, f, user_code,
                std::iterator_traits< InputIterator1 >::iterator_category( ), 
                std::iterator_traits< InputIterator2 >::iterator_category( ) );
    }

    // one-input transform, std:: iterator
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    void transform( bolt::cl::control& ctl, InputIterator first1, InputIterator last1, OutputIterator result, 
        UnaryFunction f, const std::string& user_code )
    {
        detail::transform_unary_detect_random_access( ctl, first1, last1, result, f, user_code, 
            std::iterator_traits< InputIterator >::iterator_category( ) );
    }

    // default control, one-input transform, std:: iterator
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
    void transform( InputIterator first1, InputIterator last1, OutputIterator result, 
        UnaryFunction f, const std::string& user_code )
    {
            detail::transform_unary_detect_random_access( control::getDefault(), first1, last1, result, f, user_code, 
                std::iterator_traits< InputIterator >::iterator_category( ) );
    }

namespace detail {
    struct CallCompiler_BinaryTransform {
        static void init_(
            std::vector< ::cl::Kernel >* kernels,
            std::string cl_code,
            std::string inValueType1Ptr,
            std::string inValueType1Iter,
            std::string inValueType2Ptr,
            std::string inValueType2Iter,
            std::string outValueTypePtr,
            std::string outValueTypeIter,
            std::string functorTypeName,
            const control *ctl) {

            std::vector< const std::string > kernelNames;
            kernelNames.push_back( "transform" );
            kernelNames.push_back( "transformNoBoundsCheck" );

            std::string instantiationString = 
                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(transformInstantiated)))\n"
                "kernel void transformTemplate(\n"
                "global " + inValueType1Ptr + "* A_ptr,\n"
                 + inValueType1Iter + " A_iter,\n"
                "global " + inValueType2Ptr + "* B_ptr,\n"
                 + inValueType2Iter + " B_iter,\n"
                "global " + outValueTypePtr + "* Z_ptr,\n"
                 + outValueTypeIter + " Z_iter,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n"

                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(transformNoBoundsCheckInstantiated)))\n"
                "kernel void transformNoBoundsCheckTemplate(\n"
                "global " + inValueType1Ptr + "* A_ptr,\n"
                 + inValueType1Iter + " A_iter,\n"
                "global " + inValueType2Ptr + "* B_ptr,\n"
                 + inValueType2Iter + " B_iter,\n"
                "global " + outValueTypePtr + "* Z_ptr,\n"
                 + outValueTypeIter + " Z_iter,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n";

            bolt::cl::compileKernelsString( *kernels, kernelNames, transform_kernels, instantiationString, 
                cl_code, inValueType1Ptr + inValueType2Ptr, functorTypeName, *ctl);
        };
    };

    struct CallCompiler_BinaryTransformFancy {
        static void init_(
            std::vector< ::cl::Kernel >* kernels,
            std::string cl_code,
            std::string inValueType1Name,
            std::string inValueType2Name,
            std::string outValueTypeName,
            std::string functorTypeName,
            const control *ctl) {

            std::vector< const std::string > kernelNames;
            kernelNames.push_back( "transformFancy" );
            kernelNames.push_back( "transformNoBoundsCheckFancy" );

            std::string instantiationString = 
                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(transformFancyInstantiated)))\n"
                "kernel void transformFancyTemplate(\n"
                "global " + inValueType1Name + "* A,\n"
                    + inValueType2Name + " B,\n"
                "global " + outValueTypeName + "* Z,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n"

                 
                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(transformNoBoundsCheckFancyInstantiated)))\n"
                "kernel void transformNoBoundsCheckFancyTemplate(\n"
                "global " + inValueType1Name + "* A,\n"
                    + inValueType2Name + " B,\n"
                "global " + outValueTypeName + "* Z,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n";

            bolt::cl::compileKernelsString( *kernels, kernelNames, transform_kernels, instantiationString, 
                cl_code, inValueType1Name + inValueType2Name, functorTypeName, *ctl);
        };
    };

    struct CallCompiler_UnaryTransform {
        static void init_(std::vector< ::cl::Kernel >* kernels, std::string cl_code, std::string inValueTypeName, 
            std::string outValueTypeName, std::string functorTypeName, const control *ctl) {

            std::vector< const std::string > kernelNames;
            kernelNames.push_back( "unaryTransform" );
            kernelNames.push_back( "unaryTransformNoBoundsCheck" );

            std::string instantiationString = 
                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(unaryTransformInstantiated)))\n"
                "kernel void unaryTransformTemplate(\n"
                "global " + inValueTypeName + "* A,\n"
                "global " + outValueTypeName + "* Z,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n"

                "// Host generates this instantiation string with user-specified value type and functor\n"
                "template __attribute__((mangled_name(unaryTransformNoBoundsCheckInstantiated)))\n"
                "kernel void unaryTransformNoBoundsCheckTemplate(\n"
                "global " + inValueTypeName + "* A,\n"
                "global " + outValueTypeName + "* Z,\n"
                "const uint length,\n"
                "global " + functorTypeName + "* userFunctor);\n\n"
                ;

            bolt::cl::compileKernelsString( *kernels, kernelNames, transform_kernels, instantiationString, 
                cl_code, inValueTypeName,  functorTypeName, *ctl);
        };
    };

    // Wrapper that uses default control class, iterator interface
template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform_detect_random_access( bolt::cl::control& ctl, const InputIterator1& first1, const InputIterator1& last1, const InputIterator2& first2, 
        const OutputIterator& result, const BinaryFunction& f, const std::string& user_code, std::input_iterator_tag, std::input_iterator_tag )
    {
        //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
        //  to a temporary buffer.  Should we?
        static_assert( false, "Bolt only supports random access iterator types" );
    };

template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform_detect_random_access( bolt::cl::control& ctl, const InputIterator1& first1, const InputIterator1& last1, const InputIterator2& first2, 
        const OutputIterator& result, const BinaryFunction& f, const std::string& user_code, std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        transform_pick_iterator( ctl, first1, last1, first2, result, f, user_code, 
            std::iterator_traits< InputIterator1 >::iterator_category( ),
            std::iterator_traits< InputIterator2 >::iterator_category( ) );
    };

    // Wrapper that uses default control class, iterator interface
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
    void transform_unary_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1, const InputIterator& last1, 
        const OutputIterator& result, const UnaryFunction& f, const std::string& user_code, std::input_iterator_tag )
    {
        //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data 
        //  to a temporary buffer.  Should we?
        static_assert( false, "Bolt only supports random access iterator types" );
    };

    template<typename InputIterator, typename OutputIterator, typename UnaryFunction> 
    void transform_unary_detect_random_access( bolt::cl::control& ctl, const InputIterator& first1, const InputIterator& last1, 
        const OutputIterator& result, const UnaryFunction& f, const std::string& user_code, std::random_access_iterator_tag )
    {
        transform_unary_pick_iterator( ctl, first1, last1, result, f, user_code,
            std::iterator_traits< InputIterator >::iterator_category( ) );
    };

    /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
        \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
        *  iterators.  This overload is called strictly for non-device_vector iterators
    */
    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform_pick_iterator( bolt::cl::control &ctl,  const InputIterator1& first1, const InputIterator1& last1,
        const InputIterator2& first2, const OutputIterator& result, const BinaryFunction& f, 
        const std::string& user_code, std::random_access_iterator_tag, std::random_access_iterator_tag )
    {
        typedef std::iterator_traits<InputIterator1>::value_type iType1;
        typedef std::iterator_traits<InputIterator2>::value_type iType2;
        typedef std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = (last1 - first1); 
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.

        // Map the input iterator to a device_vector
        device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );
        device_vector< iType2 > dvInput2( first2, sz, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, true, ctl );

        // Map the output iterator to a device_vector
        device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

        transform_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvInput2.begin( ), dvOutput.begin( ), f, user_code );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    template< typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryFunction > 
    void transform_pick_iterator( bolt::cl::control &ctl,  const InputIterator1& first1, const InputIterator1& last1,
        const InputIterator2& fancyIter, const OutputIterator& result, const BinaryFunction& f, 
        const std::string& user_code, std::random_access_iterator_tag, bolt::cl::fancy_iterator_tag )
    {
        typedef std::iterator_traits<InputIterator1>::value_type iType1;
        typedef std::iterator_traits<InputIterator2>::value_type iType2;
        typedef std::iterator_traits<OutputIterator>::value_type oType;
        size_t sz = std::distance( first1, last1 );
        if (sz == 0)
            return;

        // Use host pointers memory since these arrays are only read once - no benefit to copying.

        // Map the input iterator to a device_vector
        device_vector< iType1 > dvInput( first1, last1, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, ctl );

        // Map the output iterator to a device_vector
        device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY, false, ctl );

        transform_enqueue_fancy( ctl, dvInput.begin( ), dvInput.end( ), fancyIter, dvOutput.begin( ), f, user_code );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction> 
    void transform_pick_iterator( bolt::cl::control &ctl,  const DVInputIterator1& first1, const DVInputIterator1& last1, 
        const DVInputIterator2& first2, const DVOutputIterator& result, const BinaryFunction& f, 
        const std::string& user_code, bolt::cl::device_vector_tag, bolt::cl::device_vector_tag )
    {
        transform_enqueue( ctl, first1, last1, first2, result, f, user_code );
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction> 
    void transform_pick_iterator( bolt::cl::control &ctl,  const DVInputIterator1& first1, const DVInputIterator1& last1, 
        const DVInputIterator2& fancyIter, const DVOutputIterator& result, const BinaryFunction& f, 
        const std::string& user_code, bolt::cl::device_vector_tag, bolt::cl::fancy_iterator_tag )
    {
        transform_enqueue( ctl, first1, last1, fancyIter, result, f, user_code );
    }

    /*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
        \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
        *  iterators.  This overload is called strictly for non-device_vector iterators
    */
    template<typename InputIterator, typename OutputIterator, typename UnaryFunction>
    void
    transform_unary_pick_iterator( bolt::cl::control &ctl, const InputIterator& first, const InputIterator& last, 
    const OutputIterator& result, const UnaryFunction& f, const std::string& user_code, 
        std::random_access_iterator_tag )
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
        device_vector< oType > dvOutput( result, sz, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

        transform_unary_enqueue( ctl, dvInput.begin( ), dvInput.end( ), dvOutput.begin( ), f, user_code );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    // This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
    // This is called strictly for iterators that are derived from device_vector< T >::iterator
    template<typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction>
    void
    transform_unary_pick_iterator( bolt::cl::control &ctl, const DVInputIterator& first, const DVInputIterator& last, 
    const DVOutputIterator& result, const UnaryFunction& f, const std::string& user_code,
        bolt::cl::device_vector_tag )
    {
        transform_unary_enqueue( ctl, first, last, result, f, user_code );
    }

    template<typename DVInputIterator1, typename DVInputIterator2, typename DVOutputIterator, typename BinaryFunction> 
    void transform_enqueue( bolt::cl::control &ctl, const DVInputIterator1& first1, const DVInputIterator1& last1, const DVInputIterator2& first2, 
        const DVOutputIterator& result, const BinaryFunction& f, const std::string& cl_code)
    {
        typedef std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef std::iterator_traits<DVInputIterator2>::value_type iType2;
        typedef std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >( std::distance( first1, last1 ) );
        if( distVec == 0 )
            return;

        //typedef std::aligned_storage< sizeof( UnaryFunction ), 256 >::type alignedUnary;
        __declspec( align( 256 ) ) BinaryFunction aligned_functor( f );
        // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof( aligned_functor ), const_cast< BinaryFunction* >( &aligned_functor ) );   // Create buffer wrapper so we can access host parameters.
        control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_functor ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_functor );

        static boost::once_flag initOnlyOnce;
        static std::vector< ::cl::Kernel > binaryTransformKernels;

        // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retrieve the name here.
        //  TODO: I think this check needs some work now that we added an iType2
        if (boost::is_same<iType1, oType>::value)
        {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_BinaryTransform::init_, &binaryTransformKernels, 
                cl_code + ClCode<DVInputIterator1>::get() + ClCode<DVInputIterator2>::get() + ClCode<BinaryFunction>::get(), 
                TypeName< iType1 >::get( ), TypeName< DVInputIterator1 >::get( ), 
                TypeName< iType2 >::get( ), TypeName< DVInputIterator2 >::get( ), 
                TypeName< oType >::get( ), TypeName< DVOutputIterator >::get( ), 
                TypeName< BinaryFunction >::get( ), &ctl ) );
        } 
        else
        {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_BinaryTransform::init_, &binaryTransformKernels, 
                cl_code + ClCode<DVInputIterator1>::get() + ClCode<DVInputIterator2>::get() + 
                ClCode<oType>::get() + ClCode<BinaryFunction>::get(), 
                TypeName< iType1 >::get( ), TypeName< DVInputIterator1 >::get( ), 
                TypeName< iType2 >::get( ), TypeName< DVInputIterator2 >::get( ), 
                TypeName< oType >::get( ), TypeName< DVOutputIterator >::get( ), 
                TypeName< BinaryFunction >::get( ), &ctl ) );
        }

        const ::cl::Kernel& kernelWithBoundsCheck = binaryTransformKernels[0];
        const ::cl::Kernel& kernelNoBoundsCheck   = binaryTransformKernels[1];
        ::cl::Kernel k;

        cl_int l_Error = CL_SUCCESS;
        const size_t wgSize  = kernelNoBoundsCheck.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
        assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2

        size_t wgMultiple = distVec;
        size_t lowerBits = ( distVec & (wgSize-1) );
        if( lowerBits )
        {
            //  Bump the workitem count to the next multiple of wgSize
            wgMultiple &= ~lowerBits;
            wgMultiple += wgSize;
            k = kernelWithBoundsCheck;
        }
        else
        {
            k = kernelNoBoundsCheck;
        }

        k.setArg(0, first1->getBuffer( ) );
        k.setArg(1, first1.m_Index );
        k.setArg(2, first2->getBuffer( ) );
        k.setArg(3, first2.m_Index );
        k.setArg(4, result->getBuffer( ) );
        k.setArg(5, result.m_Index );
        k.setArg(6, distVec );
        k.setArg(7, *userFunctor);

        ::cl::Event transformEvent;
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
            k, 
            ::cl::NullRange, 
            ::cl::NDRange(wgMultiple),
            ::cl::NDRange(wgSize),
            NULL,
            &transformEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );

        bolt::cl::wait(ctl, transformEvent);
    };

    template<typename DVInputIterator1, typename FancyIterator, typename DVOutputIterator, typename BinaryFunction> 
    void transform_enqueue_fancy( bolt::cl::control &ctl, const DVInputIterator1& first1, const DVInputIterator1& last1, const FancyIterator& first2, 
        const DVOutputIterator& result, const BinaryFunction& f, const std::string& cl_code)
    {
        typedef std::iterator_traits<DVInputIterator1>::value_type iType1;
        typedef std::iterator_traits<FancyIterator>::value_type iType2;
        typedef std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >( std::distance( first1, last1 ) );
        if( distVec == 0 )
            return;

        //typedef std::aligned_storage< sizeof( UnaryFunction ), 256 >::type alignedUnary;
        __declspec( align( 256 ) ) BinaryFunction aligned_functor( f );
        // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof( aligned_functor ), const_cast< BinaryFunction* >( &aligned_functor ) );   // Create buffer wrapper so we can access host parameters.
        control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_functor ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_functor );

        static boost::once_flag initOnlyOnce;
        static std::vector< ::cl::Kernel > binaryTransformKernels;

        // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retrieve the name here.
        //  TODO: I think this check needs some work now that we added an iType2
        if (boost::is_same<iType1, oType>::value)
        {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_BinaryTransformFancy::init_, &binaryTransformKernels, 
                cl_code + ClCode<iType1>::get() + ClCode< FancyIterator >::get() + ClCode<BinaryFunction>::get(), 
                TypeName< iType1 >::get( ), TypeName< FancyIterator >::get( ), TypeName< oType >::get( ), 
                TypeName< BinaryFunction >::get( ), &ctl ) );
        } 
        else
        {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_BinaryTransformFancy::init_, &binaryTransformKernels, 
                cl_code + ClCode<iType1>::get() + ClCode< FancyIterator >::get() + ClCode<oType>::get() + ClCode<BinaryFunction>::get(), 
                TypeName< iType1 >::get( ), TypeName< FancyIterator >::get( ), TypeName< oType >::get( ), 
                TypeName< BinaryFunction >::get( ), &ctl ) );
        }

        const ::cl::Kernel& kernelWithBoundsCheck = binaryTransformKernels[0];
        const ::cl::Kernel& kernelNoBoundsCheck   = binaryTransformKernels[1];
        ::cl::Kernel k;

        cl_int l_Error = CL_SUCCESS;
        const size_t wgSize  = kernelNoBoundsCheck.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
        assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2

        size_t wgMultiple = distVec;
        size_t lowerBits = ( distVec & (wgSize-1) );
        if( lowerBits )
        {
            //  Bump the workitem count to the next multiple of wgSize
            wgMultiple &= ~lowerBits;
            wgMultiple += wgSize;
            k = kernelWithBoundsCheck;
        }
        else
        {
            k = kernelNoBoundsCheck;
        }

        //control::buffPointer constIterBuff = ctl.acquireBuffer( sizeof( iType2 ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &first2 );
        iType2 constValue = *first2;

        k.setArg(0, first1->getBuffer( ) );
        k.setArg(1, constValue );
        k.setArg(2, result->getBuffer( ) );
        k.setArg(3, distVec );
        k.setArg(4, *userFunctor);

        ::cl::Event transformEvent;
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
            k, 
            ::cl::NullRange, 
            ::cl::NDRange(wgMultiple),
            ::cl::NDRange(wgSize),
            NULL,
            &transformEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );

        bolt::cl::wait(ctl, transformEvent);
    };

    template< typename DVInputIterator, typename DVOutputIterator, typename UnaryFunction > 
    void transform_unary_enqueue( bolt::cl::control &ctl, const DVInputIterator& first, const DVInputIterator& last, 
        const DVOutputIterator& result, const UnaryFunction& f, const std::string& cl_code)
    {
        typedef std::iterator_traits<DVInputIterator>::value_type iType;
        typedef std::iterator_traits<DVOutputIterator>::value_type oType;

        cl_uint distVec = static_cast< cl_uint >( std::distance( first, last ) );
        if( distVec == 0 )
            return;

        //typedef std::aligned_storage< sizeof( UnaryFunction ), 256 >::type alignedUnary;
        ALIGNED( 256 ) UnaryFunction aligned_functor( f );
        // ::cl::Buffer userFunctor(ctl.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof( aligned_functor ), const_cast< UnaryFunction* >( &aligned_functor ) );   // Create buffer wrapper so we can access host parameters.
        control::buffPointer userFunctor = ctl.acquireBuffer( sizeof( aligned_functor ), CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_functor );

        static boost::once_flag initOnlyOnce;
        static std::vector< ::cl::Kernel > unaryTransformKernels;

        // For user-defined types, the user must create a TypeName trait which returns the name of the class - note use of TypeName<>::get to retreive the name here.
        if (boost::is_same<iType, oType>::value) {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_UnaryTransform::init_, &unaryTransformKernels, cl_code + ClCode<iType>::get() + ClCode<UnaryFunction>::get(), TypeName< iType >::get( ), TypeName< oType >::get( ), TypeName< UnaryFunction >::get( ), &ctl ) );
        } else {
            boost::call_once( initOnlyOnce, boost::bind( CallCompiler_UnaryTransform::init_, &unaryTransformKernels, cl_code + ClCode<iType>::get() + ClCode<oType>::get() + ClCode<UnaryFunction>::get(), TypeName< iType >::get( ), TypeName< oType >::get( ), TypeName< UnaryFunction >::get( ), &ctl ) );
        }

        const ::cl::Kernel& kernelWithBoundsCheck = unaryTransformKernels[0];
        const ::cl::Kernel& kernelNoBoundsCheck   = unaryTransformKernels[1];
        ::cl::Kernel k;

        cl_int l_Error = CL_SUCCESS;
        const size_t wgSize  = kernelWithBoundsCheck.getWorkGroupInfo< CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctl.device( ), &l_Error );
        V_OPENCL( l_Error, "Error querying kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE" );
        assert( (wgSize & (wgSize-1) ) == 0 ); // The bitwise &,~ logic below requires wgSize to be a power of 2

        size_t wgMultiple = distVec;
        size_t lowerBits = ( distVec & (wgSize-1) );
        if( lowerBits )
        {
            //  Bump the workitem count to the next multiple of wgSize
            wgMultiple &= ~lowerBits;
            wgMultiple += wgSize;
            k = kernelWithBoundsCheck;
        }
        else
        {
            k = kernelNoBoundsCheck;
        }

        //void* h_result = (void*)ctl.commandQueue().enqueueMapBuffer( userFunctor, true, CL_MAP_READ, 0, sizeof(aligned_functor), NULL, NULL, &l_Error );
        //V_OPENCL( l_Error, "Error calling map on the result buffer" );

        k.setArg(0, first->getBuffer( ) );
        k.setArg(1, result->getBuffer( ) );
        k.setArg(2, distVec );
        k.setArg(3, *userFunctor);

        ::cl::Event transformEvent;
        l_Error = ctl.commandQueue().enqueueNDRangeKernel(
            k, 
            ::cl::NullRange, 
            ::cl::NDRange(wgMultiple), 
            ::cl::NDRange(wgSize),
            NULL,
            &transformEvent );
        V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for transform() kernel" );

        bolt::cl::wait(ctl, transformEvent);
    };

} //End of detail namespace
} //End of cl namespace
} //End of bolt namespace

#endif
