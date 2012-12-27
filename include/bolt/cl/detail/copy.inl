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

#if !defined( COPY_INL )
#define COPY_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 

#include "bolt/cl/bolt.h"

namespace bolt {
    namespace cl {

        // user control
        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy(const bolt::cl::control &ctl,  InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code)
        {
            int n = static_cast<int>( std::distance( first, last ) );
            return detail::copy_detect_random_access( ctl, first, n, result, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control
        template<typename InputIterator, typename OutputIterator> 
        OutputIterator copy( InputIterator first, InputIterator last, OutputIterator result, 
            const std::string& user_code)
        {
            int n = static_cast<int>( std::distance( first, last ) );
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // default control
        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code)
        {
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }

        // user control
        template<typename InputIterator, typename Size, typename OutputIterator> 
        OutputIterator copy_n(const bolt::cl::control &ctl, InputIterator first, Size n, OutputIterator result, 
            const std::string& user_code)
        {
            return detail::copy_detect_random_access( ctl, first, n, result, user_code, std::iterator_traits< InputIterator >::iterator_category( ) );
        }


    }//end of cl namespace
};//end of bolt namespace


namespace bolt {
namespace cl {
namespace detail {

        enum typeName { e_iType, e_oType };

/***********************************************************************************************************************
 * Kernel Template Specializer
 **********************************************************************************************************************/
class Copy_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
    public:

    Copy_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("copyBoundsCheck");
        addKernelName("copyNoBoundsCheck");
    }
    
    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString = 
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[e_iType] + " *src,\n"
            "global " + typeNames[e_oType] + " *dst,\n"
            "const uint length\n"
            ");\n\n"


            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global " + typeNames[e_iType] + " *src,\n"
            "global " + typeNames[e_oType] + " *dst,\n"
            "const uint length\n"
            ");\n\n"
            ;
    
        return templateSpecializationString;
    }
};



// Wrapper that uses default control class, iterator interface
template<typename InputIterator, typename Size, typename OutputIterator> 
OutputIterator copy_detect_random_access( const bolt::cl::control& ctl, const InputIterator& first, const Size& n, 
    const OutputIterator& result, const std::string& user_code, std::input_iterator_tag )
{
    static_assert( false, "Bolt only supports random access iterator types" );
    return NULL;
};

template<typename InputIterator, typename Size, typename OutputIterator> 
OutputIterator copy_detect_random_access( const bolt::cl::control& ctl, const InputIterator& first, const Size& n, 
    const OutputIterator& result, const std::string& user_code, std::random_access_iterator_tag )
{
    if (n > 0)
    {
        copy_pick_iterator( ctl, first, n, result, user_code );
    }
    return (result+n);
};

/*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
    \detail This template is called by the non-detail versions of inclusive_scan, it already assumes random access
 *  iterators.  This overload is called strictly for non-device_vector iterators
*/
template<typename InputIterator, typename Size, typename OutputIterator> 
typename std::enable_if< 
             !(std::is_base_of<typename device_vector<typename std::iterator_traits<InputIterator>::value_type>::iterator,InputIterator>::value &&
               std::is_base_of<typename device_vector<typename std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
         void >::type
copy_pick_iterator(const bolt::cl::control &ctl,  const InputIterator& first, const Size& n, 
        const OutputIterator& result, const std::string& user_code)
{
    typedef std::iterator_traits<InputIterator>::value_type iType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;

    // Use host pointers memory since these arrays are only read once - no benefit to copying.

    // Map the input iterator to a device_vector
    device_vector< iType >  dvInput( first,  n, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctl );

    // Map the output iterator to a device_vector
    device_vector< oType > dvOutput( result, n, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

    copy_enqueue( ctl, dvInput.begin( ), n, dvOutput.begin( ), user_code );

    // This should immediately map/unmap the buffer
    dvOutput.data( );
}

// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator> 
typename std::enable_if< 
              (std::is_base_of<typename device_vector<typename std::iterator_traits<DVInputIterator>::value_type>::iterator,DVInputIterator>::value &&
               std::is_base_of<typename device_vector<typename std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
         void >::type
copy_pick_iterator(const bolt::cl::control &ctl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result, const std::string& user_code)
{
    copy_enqueue( ctl, first, n, result, user_code );
}

template< typename DVInputIterator, typename Size, typename DVOutputIterator > 
void copy_enqueue(const bolt::cl::control &ctl, const DVInputIterator& first, const Size& n, 
    const DVOutputIterator& result, const std::string& cl_code)
{
    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef std::iterator_traits<DVInputIterator>::value_type iType;
    typedef std::iterator_traits<DVOutputIterator>::value_type oType;
    std::vector<std::string> typeNames(2);
    typeNames[e_iType] = TypeName< iType >::get( );
    typeNames[e_oType] = TypeName< oType >::get( );

    /**********************************************************************************
     * Type Definitions - directly concatenated into kernel string (order may matter)
     *********************************************************************************/
    std::vector<std::string> typeDefs;
    PUSH_BACK_UNIQUE( typeDefs, ClCode< iType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< oType >::get() )

    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
     std::string compileOptions = "";

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    Copy_KernelTemplateSpecializer c_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &c_kts,
        typeDefs,
        copy_kernels,
        compileOptions);

    //  Ceiling function to bump the size of input to the next whole wavefront size
    unsigned int kernelWgSize = 256;
    cl_uint numThreads = n;
    size_t modWgSize = (numThreads & (kernelWgSize-1));
    int whichKernel = 0;
    if( modWgSize )
    {
        whichKernel = 1; // will need to do bounds check
        numThreads &= ~modWgSize;
        numThreads += kernelWgSize;
    }
    cl_uint numWorkGroups = static_cast< cl_uint >( numThreads / kernelWgSize );

    /**********************************************************************************
     *  Kernel
     *********************************************************************************/
    ::cl::Event kernelEvent;
    cl_int l_Error;
    try
    {
    V_OPENCL( kernels[whichKernel].setArg( 0, first->getBuffer()), "Error setArg kernels[ 0 ]" ); // Input keys
    V_OPENCL( kernels[whichKernel].setArg( 1, result->getBuffer()),"Error setArg kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[whichKernel].setArg( 2, n ),          "Error setArg kernels[ 0 ]" ); // Size of buffer
    
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[whichKernel],
        ::cl::NullRange,
        ::cl::NDRange( numThreads ),
        ::cl::NDRange( kernelWgSize ),
        NULL,
        &kernelEvent);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( ) in bolt::cl::copy_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    // wait for results
    bolt::cl::wait(ctl, kernelEvent);


#if 0
    ::cl::Event copyEvent;
    cl_int l_Error = ctl.commandQueue().enqueueCopyBuffer(
        first->getBuffer(),
        result->getBuffer(),
        first->getIndex(),
        result->getIndex(),
        n*sizeof(iType),
        //0,
        NULL,
        &copyEvent);
    V_OPENCL( l_Error, "enqueueCopyBuffer() failed for copy()" );
    bolt::cl::wait(ctl, copyEvent);
#endif

};
}//End OF detail namespace
}//End OF cl namespace
}//End OF bolt namespace

#endif
