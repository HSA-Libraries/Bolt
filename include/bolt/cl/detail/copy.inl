/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
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

#ifndef BURST_SIZE
#define BURST_SIZE 4
#endif

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits>

#include "bolt/cl/bolt.h"

// bumps dividend up (if needed) to be evenly divisible by divisor
// returns whether dividend changed
// makeDivisible(9,4) -> 12,true
// makeDivisible(9,3) -> 9, false
template< typename Type1, typename Type2 >
bool makeDivisible( Type1& dividend, Type2 divisor)
{
    size_t lowerBits = static_cast<size_t>( dividend & (divisor-1) );
    if( lowerBits )
    { // bump it up
        dividend &= ~lowerBits;
        dividend += divisor;
        return true;
    }
    else
    { // already evenly divisible
      return false;
    }
}

// bumps dividend up (if needed) to be evenly divisible by divisor
// returns whether dividend changed
// roundUpDivide(9,4,?)  -> 12,4,3,true
// roundUpDivide(10,2,?) -> 10,2,5,false
template< typename Type1, typename Type2, typename Type3 >
bool roundUpDivide( Type1& dividend, Type2 divisor, Type3& quotient)
{
    size_t lowerBits = static_cast<size_t>( dividend & (divisor-1) );
    if( lowerBits )
    { // bump it up
        dividend &= ~lowerBits;
        dividend += divisor;
        quotient = dividend / divisor;
        return true;
    }
    else
    { // already evenly divisible
      quotient = dividend / divisor;
      return false;
    }
}

namespace bolt {
namespace cl {

// user control
template<typename InputIterator, typename OutputIterator>
OutputIterator copy(const bolt::cl::control &ctrl,  InputIterator first, InputIterator last, OutputIterator result,    
            const std::string& user_code)
{
    int n = static_cast<int>( std::distance( first, last ) );
    return detail::copy_detect_random_access( ctrl, first, n, result, user_code,
        std::iterator_traits< InputIterator >::iterator_category( ) );
}

// default control
template<typename InputIterator, typename OutputIterator>
OutputIterator copy( InputIterator first, InputIterator last, OutputIterator result,
            const std::string& user_code)
{
    int n = static_cast<int>( std::distance( first, last ) );
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code,
                std::iterator_traits< InputIterator >::iterator_category( ) );
}

// default control
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_n(InputIterator first, Size n, OutputIterator result,
            const std::string& user_code)
{
            return detail::copy_detect_random_access( control::getDefault(), first, n, result, user_code,
                std::iterator_traits< InputIterator >::iterator_category( ) );
}

// user control
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_n(const bolt::cl::control &ctrl, InputIterator first, Size n, OutputIterator result,
            const std::string& user_code)
{
    return detail::copy_detect_random_access( ctrl, first, n, result, user_code,
        std::iterator_traits< InputIterator >::iterator_category( ) );
}

}//end of cl namespace
};//end of bolt namespace


namespace bolt {
namespace cl {
namespace detail {

enum copyTypeName { copy_iType, copy_DVInputIterator/*added*/, copy_oType, copy_DVOutputIterator/*added*/, end_copy };

/**********************************************************************************************************************
 * Kernel Template Specializer
 *********************************************************************************************************************/
class Copy_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
    public:

    Copy_KernelTemplateSpecializer() : KernelTemplateSpecializer()
        {
        addKernelName( "copy_I"     );
        addKernelName( "copy_II"    );
        addKernelName( "copy_III"   );
        addKernelName( "copy_IV"    );
        // addKernelName( "copy_V"     );
        }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[copy_iType] + " * restrict src,\n"
             + typeNames[copy_DVInputIterator] + " input_iter,\n"  // Added
            "global " + typeNames[copy_oType] + " * restrict dst,\n"
             + typeNames[copy_DVOutputIterator] + " output_iter,\n" // added
            "const uint numElements,\n"
            "const uint srcOffset,\n"
            "const uint dstOffset\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global " + typeNames[copy_iType] + " * restrict src,\n"
            "global " + typeNames[copy_oType] + " * restrict dst,\n"
            "const uint numElements\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(2) + "(\n"
            "global " + typeNames[copy_iType] + " * restrict src,\n"
            "global " + typeNames[copy_oType] + " * restrict dst,\n"
            "const uint numElements\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(256,1,1)))\n"
            "__kernel void " + name(3) + "(\n"
            "global " + typeNames[copy_iType] + " * restrict src,\n"
            "global " + typeNames[copy_oType] + " * restrict dst,\n"
            "const uint numElements\n"
            ");\n\n"
            ;

        return templateSpecializationString;
    }
};

// Wrapper that uses default control class, iterator interface
template<typename InputIterator, typename Size, typename OutputIterator>
OutputIterator copy_detect_random_access( const bolt::cl::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, const std::string& user_code, std::input_iterator_tag )
{
    static_assert( false, "Bolt only supports random access iterator types" );
    return NULL;
};

template<typename InputIterator, typename Size, typename OutputIterator >
OutputIterator copy_detect_random_access( const bolt::cl::control& ctrl, const InputIterator& first, const Size& n,
                const OutputIterator& result, const std::string& user_code, std::random_access_iterator_tag )
{
    if (n < 0)
    {
        std::cout<<"\n Number of elements to copy cannot be negative! "<< std::endl;
    }
    if (n > 0)
    {
        copy_pick_iterator( ctrl, first, n, result, user_code,
            std::iterator_traits< InputIterator >::iterator_category( ),
            std::iterator_traits< OutputIterator >::iterator_category( ) );
    }
    return (result+n);
};

/*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes
             *  random access iterators.  This overload is called strictly for non-device_vector iterators
            */
template<typename InputIterator, typename Size, typename OutputIterator>
void copy_pick_iterator(const bolt::cl::control &ctrl,  const InputIterator& first, const Size& n,
        const OutputIterator& result, const std::string& user_code, std::random_access_iterator_tag,
        std::random_access_iterator_tag )
{

    typedef std::iterator_traits<InputIterator>::value_type iType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;

    // Use host pointers memory since these arrays are only read once - no benefit to copying.

    //// Map the input iterator to a device_vector
    //device_vector< iType >  dvInput( first, n, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, true, ctrl );

    //            // Map the output iterator to a device_vector
    //device_vector< oType > dvOutput( result, n, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctrl );

    //copy_enqueue( ctrl, dvInput.begin( ), n, dvOutput.begin( ), user_code );

    //// This should immediately map/unmap the buffer
    //dvOutput.data( );

    // A host 2 host copy operation, just fallback on the optimized std:: implementation
     bolt::cl::control::e_RunMode runMode = ctrl.getForceRunMode( );

     if( runMode == bolt::cl::control::Automatic )
     {
                runMode = ctrl.getDefaultPathToRun( );
     }

     if( runMode == bolt::cl::control::SerialCpu )
     {
                std::copy_n( first, n, result );
     }
     else if( runMode == bolt::cl::control::MultiCoreCpu )
     {

        #ifdef ENABLE_TBB
               throw std::exception( "The MultiCoreCpu version of Copy is not Implemented yet!" );
        #else
               throw std::exception( "The MultiCoreCpu version of Copy is not enabled to be built." );
        #endif
     }
     else
     {
                std::copy_n( first, n, result );
     }
}

/*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of inclusive_scan, it already assumes
             *   random access iterators.  This overload is called strictly for non-device_vector iterators
            */
template<typename InputIterator, typename Size, typename OutputIterator>
void copy_pick_iterator(const bolt::cl::control &ctrl,  const InputIterator& first, const Size& n,
        const OutputIterator& result, const std::string& user_code, bolt::cl::fancy_iterator_tag,
        std::random_access_iterator_tag )
{

    typedef std::iterator_traits<InputIterator>::value_type iType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;

     bolt::cl::control::e_RunMode runMode = ctrl.getForceRunMode( );

     if( runMode == bolt::cl::control::Automatic )
     {
                runMode = ctrl.getDefaultPathToRun( );
     }

     if( runMode == bolt::cl::control::SerialCpu )
     {

                std::copy_n( first, n, result );
     }
     else if( runMode == bolt::cl::control::MultiCoreCpu )
     {

         #ifdef ENABLE_TBB
               throw std::exception( "The MultiCoreCpu version of Copy is not Implemented yet!" );
         #else
               throw std::exception( "The MultiCoreCpu version of Copy is not enabled to be built." );
         #endif
     }
     else
     {

        // Use host pointers memory since these arrays are only read once - no benefit to copying.

        // Map the output iterator to a device_vector
        device_vector< oType > dvOutput( result, n, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctrl );

        copy_enqueue( ctrl, first, n, dvOutput.begin( ), user_code );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
     }
}

// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator(const bolt::cl::control &ctrl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result, const std::string& user_code, bolt::cl::device_vector_tag,
    bolt::cl::device_vector_tag )
{

     bolt::cl::control::e_RunMode runMode = ctrl.getForceRunMode( );

     if( runMode == bolt::cl::control::Automatic )
     {
               runMode = ctrl.getDefaultPathToRun( );
     }

     if( runMode == bolt::cl::control::SerialCpu )
     {
                std::copy_n( first, n, result );
     }
     else if( runMode == bolt::cl::control::MultiCoreCpu )
     {

         #ifdef ENABLE_TBB
                throw std::exception( "The MultiCoreCpu version of Copy is not Implemented yet!" );
         #else
                throw std::exception( "The MultiCoreCpu version of Copy is not enabled to be built." );
         #endif
     }
     else
     {
              copy_enqueue( ctrl, first, n, result, user_code );
     }
}

// This template is called by the non-detail versions of inclusive_scan, it already assumes random access iterators
// This is called strictly for iterators that are derived from device_vector< T >::iterator
template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator(const bolt::cl::control &ctrl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result, const std::string& user_code, bolt::cl::fancy_iterator_tag,
    bolt::cl::device_vector_tag )
{
     bolt::cl::control::e_RunMode runMode = ctrl.getForceRunMode( );

     if( runMode == bolt::cl::control::Automatic )
     {
               runMode = ctrl.getDefaultPathToRun( );
     }

     if( runMode == bolt::cl::control::SerialCpu )
     {
               std::copy_n( first, n, result );
     }
     else if( runMode == bolt::cl::control::MultiCoreCpu )
     {
        #ifdef ENABLE_TBB
              throw std::exception( "The MultiCoreCpu version of Copy is not Implemented yet!" );
        #else
              throw std::exception( "The MultiCoreCpu version of Copy is not enabled to be built." );
        #endif
     }
     else
     {
              copy_enqueue( ctrl, first, n, result, user_code );
     }
}

template<typename DVInputIterator, typename Size, typename DVOutputIterator>
void copy_pick_iterator(const bolt::cl::control &ctl,  const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result, const std::string& user_code, std::random_access_iterator_tag,
    bolt::cl::fancy_iterator_tag )
{
    static_assert( false, "It is not possible to copy into fancy iterators. They are not mutable" );
}


template< typename DVInputIterator, typename Size, typename DVOutputIterator >
typename std::enable_if< std::is_same< typename std::iterator_traits<DVInputIterator >::iterator_category,
                                       typename std::iterator_traits<DVOutputIterator >::iterator_category
                                     >::value >::type  /*If enabled then this typename will be evaluated to void*/
    copy_enqueue(const bolt::cl::control &ctrl, const DVInputIterator& first, const Size& n, 
    const DVOutputIterator& result, const std::string& cl_code)
{ 
    typedef std::iterator_traits<DVInputIterator>::value_type iType;
    typedef std::iterator_traits<DVOutputIterator>::value_type oType;
    ::cl::Event copyEvent;
    ctrl.getCommandQueue( ).enqueueCopyBuffer(
                        first.getContainer().getBuffer(),
                        result.getContainer().getBuffer(),
                        first.m_Index * sizeof(iType),
                        result.m_Index * sizeof(oType),
                        n*sizeof(oType),
                        NULL,
                        &copyEvent);
    // wait for results
    bolt::cl::wait(ctrl, copyEvent);
}

template< typename DVInputIterator, typename Size, typename DVOutputIterator >
typename std::enable_if< !std::is_same< typename std::iterator_traits<DVInputIterator >::iterator_category,
                                       typename std::iterator_traits<DVOutputIterator >::iterator_category
                                     >::value
                       >::type  /*If enabled then this typename will be evaluated to void*/
    copy_enqueue(const bolt::cl::control &ctrl, const DVInputIterator& first, const Size& n,
    const DVOutputIterator& result, const std::string& cl_code)
{  
    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef std::iterator_traits<DVInputIterator>::value_type iType;
    typedef std::iterator_traits<DVOutputIterator>::value_type oType;
    std::vector<std::string> typeNames(end_copy);
    typeNames[copy_iType] = TypeName< iType >::get( );
    typeNames[copy_DVInputIterator] = TypeName< DVInputIterator >::get( ); // added
    typeNames[copy_oType] = TypeName< oType >::get( );
    typeNames[copy_DVOutputIterator] = TypeName< DVOutputIterator >::get( ); // added

    /**********************************************************************************
     * Type Definitions - directrly concatenated into kernel string (order may matter)
     *********************************************************************************/
    std::vector<std::string> typeDefs;
    PUSH_BACK_UNIQUE( typeDefs, ClCode< iType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< DVInputIterator >::get() ) // added
    PUSH_BACK_UNIQUE( typeDefs, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< DVOutputIterator >::get() ) // added

    //kernelWithBoundsCheck.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE >( ctrl.device( ), &l_Error )
    const size_t workGroupSize  = 256;
    const size_t numComputeUnits = 40; //ctrl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( ); // = 28
    const size_t numWorkGroupsPerComputeUnit = 10; //ctrl.wgPerComputeUnit( );
    const size_t numWorkGroups = numComputeUnits * numWorkGroupsPerComputeUnit;

    const cl_uint numThreadsIdeal = static_cast<cl_uint>( numWorkGroups * workGroupSize );
    cl_uint numElementsPerThread = n / numThreadsIdeal;
    cl_uint numThreadsRUP = n;
    size_t mod = (n & (workGroupSize-1));
    int doBoundaryCheck = 0;
    if( mod )
            {
        numThreadsRUP &= ~mod;
        numThreadsRUP += workGroupSize;
        doBoundaryCheck = 1;
            }

    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    std::string compileOptions;
    std::ostringstream oss;
    oss << " -DBURST_SIZE=" << BURST_SIZE;
    oss << " -DBOUNDARY_CHECK=" << doBoundaryCheck;
    compileOptions = oss.str();

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    Copy_KernelTemplateSpecializer c_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctrl,
        typeNames,
        &c_kts,
        typeDefs,
        copy_kernels,
        compileOptions);

    /**********************************************************************************
     *  Kernel
     *********************************************************************************/
    ::cl::Event kernelEvent;
    cl_int l_Error;
    try
    {
        int whichKernel = 0;
        cl_uint numThreadsChosen;
        cl_uint workGroupSizeChosen = workGroupSize;
        switch( whichKernel )
            {
        case 0: // I: 1 thread per element
            numThreadsChosen = numThreadsRUP;
            break;
        case 1: // II: 1 element per thread / BURST_SIZE
            numThreadsChosen = numThreadsRUP / BURST_SIZE;
            break;
        case 2: // III: ideal threads
        case 3: // IV: ideal threads w/ BURST
        case 4: // V: ideal threads unrolled BURST
            numThreadsChosen = numThreadsIdeal;
            break;
        } // switch


        //std::cout << "NumElem: " << n << "; NumThreads: " << numThreadsChosen << ";
        //NumWorkGroups: " << numThreadsChosen/workGroupSizeChosen << std::endl;

        // Input buffer

        // Input keys
        V_OPENCL( kernels[whichKernel].setArg( 0, first.getContainer().getBuffer()), "Error setArg kernels[ 0 ]" ); 
        V_OPENCL( kernels[whichKernel].setArg( 1, first.gpuPayloadSize( ), &first.gpuPayload( ) ),  // added
                                                           "Error setting a kernel argument" );
        // Output buffer

         // Input buffer
        V_OPENCL( kernels[whichKernel].setArg( 2, result.getContainer().getBuffer()),"Error setArg kernels[ 0 ]" );    
        V_OPENCL( kernels[whichKernel].setArg( 3, result.gpuPayloadSize( ), &result.gpuPayload( ) ),  // added
                                                           "Error setting a kernel argument" );
        //Buffer Size
        V_OPENCL( kernels[whichKernel].setArg( 4, static_cast<cl_uint>( n ) ),"Error setArg kernels[0]" );
        V_OPENCL( kernels[whichKernel].setArg( 5, first.m_Index ), "Error setArg kernels[ 0 ]" ); 
        V_OPENCL( kernels[whichKernel].setArg( 6, result.m_Index ), "Error setArg kernels[ 0 ]" );

        l_Error = ctrl.getCommandQueue( ).enqueueNDRangeKernel(
            kernels[whichKernel],
            ::cl::NullRange,
            ::cl::NDRange( numThreadsChosen ),
            ::cl::NDRange( workGroupSizeChosen ),
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
    bolt::cl::wait(ctrl, kernelEvent);


    // profiling
    cl_command_queue_properties queueProperties;
    l_Error = ctrl.getCommandQueue().getInfo<cl_command_queue_properties>(CL_QUEUE_PROPERTIES, &queueProperties);
    unsigned int profilingEnabled = queueProperties&CL_QUEUE_PROFILING_ENABLE;
    if ( profilingEnabled ) {
        cl_ulong start_time, stop_time;

        V_OPENCL( kernelEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &start_time),
            "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        V_OPENCL( kernelEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &stop_time),
            "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        size_t time = stop_time - start_time;
        double gb = (n*(sizeof(iType)+sizeof(oType))/1024.0/1024.0/1024.0);
        double sec = time/1000000000.0;
        std::cout << "Global Memory Bandwidth: " << ( gb / sec) << " ( "
          << time/1000000.0 << " ms)" << std::endl;
    }
};
}//End OF detail namespace
}//End OF cl namespace
}//End OF bolt namespace

#endif
