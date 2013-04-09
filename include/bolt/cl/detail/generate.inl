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

#if !defined( GENERATE_INL )
#define GENERATE_INL
#pragma once

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include <type_traits> 

#include "bolt/cl/bolt.h"

#define BURST 1

namespace bolt {
namespace cl {

// default control, start->stop
template<typename ForwardIterator, typename Generator> 
void generate( ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code)
{
            detail::generate_detect_random_access( bolt::cl::control::getDefault(), first, last, gen, cl_code, 
                std::iterator_traits< ForwardIterator >::iterator_category( ) );
}

        // user specified control, start->stop
        template<typename ForwardIterator, typename Generator> 
        void generate( bolt::cl::control &ctl, ForwardIterator first, ForwardIterator last, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( ctl, first, last, gen, cl_code, 
                std::iterator_traits< ForwardIterator >::iterator_category( ) );
        }

// default control, start-> +n
template<typename OutputIterator, typename Size, typename Generator> 
OutputIterator generate_n( OutputIterator first, Size n, Generator gen, const std::string& cl_code)
{
            detail::generate_detect_random_access( bolt::cl::control::getDefault(), first, first+n, gen, cl_code, 
                std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
}

        // user specified control, start-> +n
        template<typename OutputIterator, typename Size, typename Generator> 
        OutputIterator generate_n( bolt::cl::control &ctl, OutputIterator first, Size n, Generator gen, const std::string& cl_code)
        {
            detail::generate_detect_random_access( ctl, first, n, gen, cl_code, 
                std::iterator_traits< OutputIterator >::iterator_category( ) );
            return (first+n);
}

}//end of cl namespace
};//end of bolt namespace


namespace bolt {
namespace cl {
namespace detail {

enum typeName { gen_oType, gen_genType };

/***********************************************************************************************************************
 * Kernel Template Specializer
 **********************************************************************************************************************/
class Generate_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
    public:

    Generate_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName( "generate_I"   );
        addKernelName( "generate_II"  );
        addKernelName( "generate_III" );
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =
                        "// Host generates this instantiation string with user-specified value type and generator\n"
                "template __attribute__((mangled_name("+name(0)+"Instantiated)))\n"
                "kernel void "+name(0)+"(\n"
                "global " + typeNames[gen_oType] + " * restrict dst,\n"
                "const int numElements,\n"
                "global " + typeNames[gen_genType] + " * restrict genPtr);\n\n"

                        "// Host generates this instantiation string with user-specified value type and generator\n"
                "template __attribute__((mangled_name("+name(1)+"Instantiated)))\n"
                "kernel void "+name(1)+"(\n"
                "global " + typeNames[gen_oType] + " * restrict dst,\n"
                "const int numElements,\n"
                "global " + typeNames[gen_genType] + " * restrict genPtr);\n\n"

                        "// Host generates this instantiation string with user-specified value type and generator\n"
                "template __attribute__((mangled_name("+name(2)+"Instantiated)))\n"
                "kernel void "+name(2)+"(\n"
                "global " + typeNames[gen_oType] + " * restrict dst,\n"
                "const int numElements,\n"
                "global " + typeNames[gen_genType] + " * restrict genPtr);\n\n"
                ;
    
        return templateSpecializationString;
    }
};
                        


/*****************************************************************************
             * Random Access
             ****************************************************************************/

// generate, not random-access
template<typename ForwardIterator, typename Generator>
void generate_detect_random_access( bolt::cl::control &ctrl, const ForwardIterator& first, const ForwardIterator& last, 
                        const Generator& gen, const std::string &cl_code, std::forward_iterator_tag )
{
                static_assert( false, "Bolt only supports random access iterator types" );
}

// generate, yes random-access
template<typename ForwardIterator, typename Generator>
void generate_detect_random_access( bolt::cl::control &ctrl, const ForwardIterator& first, const ForwardIterator& last, 
                        const Generator& gen, const std::string &cl_code, std::random_access_iterator_tag )
            {
                generate_pick_iterator(ctrl, first, last, gen, cl_code, 
                    std::iterator_traits< ForwardIterator >::iterator_category( ) );
            }
           

/*****************************************************************************
             * Pick Iterator
             ****************************************************************************/

/*! \brief This template function overload is used to seperate device_vector iterators from all other iterators
                \detail This template is called by the non-detail versions of generate, it already assumes random access
             *  iterators.  This overload is called strictly for non-device_vector iterators
            */
            template<typename ForwardIterator, typename Generator> 
            void generate_pick_iterator(bolt::cl::control &ctrl,  const ForwardIterator &first, const ForwardIterator &last, 
                const Generator &gen, const std::string &user_code, std::random_access_iterator_tag )
            {
                typedef std::iterator_traits<ForwardIterator>::value_type Type;

                size_t sz = (last - first); 
                if (sz < 1)
                    return;

                // Use host pointers memory since these arrays are only write once - no benefit to copying.
                // Map the forward iterator to a device_vector
    device_vector< Type > range( first, sz, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctrl );

    generate_enqueue( ctrl, range.begin( ), range.end( ), gen, user_code );

                // This should immediately map/unmap the buffer
                range.data( );
}

            // This template is called by the non-detail versions of generate, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVForwardIterator, typename Generator> 
            void generate_pick_iterator(bolt::cl::control &ctl, const DVForwardIterator &first, const DVForwardIterator &last, 
                const Generator &gen, const std::string& user_code, bolt::cl::device_vector_tag )
            {
                generate_enqueue( ctl, first, last, gen, user_code );
            }

            // This template is called by the non-detail versions of generate, it already assumes random access iterators
            // This is called strictly for iterators that are derived from device_vector< T >::iterator
            template<typename DVForwardIterator, typename Generator> 
            void generate_pick_iterator(bolt::cl::control &ctl,  const DVForwardIterator &first, const DVForwardIterator &last, 
                const Generator &gen, const std::string& user_code, bolt::cl::fancy_iterator_tag )
            {
                static_assert( false, "It is not possible to generate into fancy iterators. They are not mutable" );
            }

/*****************************************************************************
             * Enqueue
             ****************************************************************************/

template< typename DVForwardIterator, typename Generator > 
void generate_enqueue(
    bolt::cl::control &ctrl,
    const DVForwardIterator &first,
    const DVForwardIterator &last,
    const Generator &gen,
    const std::string& cl_code )
{
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("generate");
aProfiler.startTrial();
aProfiler.setStepName("Acquire Kernel");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif
    cl_int l_Error;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
     typedef std::iterator_traits<DVForwardIterator>::value_type oType;
    std::vector<std::string> typeNames(2);
    typeNames[gen_oType] = TypeName< oType >::get( );
    typeNames[gen_genType] = TypeName< Generator >::get( );

    /**********************************************************************************
     * Type Definitions - directly concatenated into kernel string (order may matter)
     *********************************************************************************/
    std::vector<std::string> typeDefs;
    PUSH_BACK_UNIQUE( typeDefs, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< Generator >::get() )

    /**********************************************************************************
     * Number of Threads
     *********************************************************************************/
    const cl_uint numElements = static_cast< cl_uint >( std::distance( first, last ) );
    if (numElements < 1) return;
    const size_t workGroupSize  = 256;
    const size_t numComputeUnits = ctrl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( ); // = 28
    const size_t numWorkGroupsPerComputeUnit = ctrl.getWGPerComputeUnit( );
    const size_t numWorkGroupsIdeal = numComputeUnits * numWorkGroupsPerComputeUnit;
    const cl_uint numThreadsIdeal = static_cast<cl_uint>( numWorkGroupsIdeal * workGroupSize );
    int doBoundaryCheck = 0;
    cl_uint numThreadsRUP = numElements;
    size_t mod = (numElements & (workGroupSize-1));
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
    oss << " -DBURST=" << BURST;
    oss << " -DBOUNDARY_CHECK=" << doBoundaryCheck;
    compileOptions = oss.str();

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    Generate_KernelTemplateSpecializer kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctrl,
        typeNames,
        &kts,
        typeDefs,
        generate_kernels,
        compileOptions);

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Acquire Intermediate Buffers");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    /**********************************************************************************
     * Temporary Buffers
     *********************************************************************************/
                ALIGNED( 256 ) Generator aligned_generator( gen );
                // ::cl::Buffer userGenerator(ctl.context(), CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, 
                //  sizeof( aligned_generator ), const_cast< Generator* >( &aligned_generator ) );
                control::buffPointer userGenerator = ctrl.acquireBuffer( sizeof( aligned_generator ), 
                    CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, &aligned_generator );

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Kernel 0 Setup");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif
                
    int whichKernel = 0;
    cl_uint numThreadsChosen;
    cl_uint workGroupSizeChosen = workGroupSize;
    switch( whichKernel )
    {
    case 0: // I: thread per element
        numThreadsChosen = numThreadsRUP;
        break;
    case 1: // II: ideal threads
        numThreadsChosen = numThreadsIdeal;
        break;
    case 2: // III:   ideal threads, BURST
        numThreadsChosen = numThreadsIdeal;
        break;
    } // switch kernel


    V_OPENCL( kernels[whichKernel].setArg( 0, first.getBuffer()),  "Error setArg kernels[ 0 ]" ); // Input keys
    V_OPENCL( kernels[whichKernel].setArg( 1, numElements),         "Error setArg kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[whichKernel].setArg( 2, *userGenerator ),     "Error setArg kernels[ 0 ]" ); // Size of buffer

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Kernel 0 Enqueue");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.nextStep();
aProfiler.setStepName("Kernel 0");
aProfiler.set(AsyncProfiler::device, ctrl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 1*numElements);
aProfiler.set(AsyncProfiler::memory, 1*numElements*sizeof(oType)
        + (numThreadsChosen/workGroupSizeChosen)*sizeof(Generator));
#endif

                // enqueue kernel
                ::cl::Event generateEvent;
    l_Error = ctrl.getCommandQueue().enqueueNDRangeKernel(
        kernels[whichKernel], 
                    ::cl::NullRange,
        ::cl::NDRange(numThreadsChosen),
        ::cl::NDRange(workGroupSizeChosen),
                    NULL,
                    &generateEvent );
                V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for generate() kernel" );

                // wait to kernel completion
    bolt::cl::wait(ctrl, generateEvent);
#if 0
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Querying Kernel Times");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

aProfiler.setDataSize(numElements*sizeof(iType));
std::string strDeviceName = ctrl.device().getInfo< CL_DEVICE_NAME >( &l_Error );
bolt::cl::V_OPENCL( l_Error, "Device::getInfo< CL_DEVICE_NAME > failed" );
aProfiler.setArchitecture(strDeviceName);

    try
    {
        cl_ulong k0_start, k0_stop, k1_stop, k2_stop;
        cl_ulong k1_start, k2_start;
        
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k2_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");

        size_t k0_start_cpu = aProfiler.get(k0_stepNum, AsyncProfiler::startTime);
        size_t shift = k0_start - k0_start_cpu;
        //size_t shift = k0_start_cpu - k0_start;

        //std::cout << "setting step " << k0_stepNum << " attribute " << AsyncProfiler::stopTime << " to " << k0_stop-shift << std::endl;
        aProfiler.set(k0_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k0_stop-shift) );

        aProfiler.set(k1_stepNum, AsyncProfiler::startTime, static_cast<size_t>(k0_stop-shift) );
        aProfiler.set(k1_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k1_stop-shift) );

        aProfiler.set(k2_stepNum, AsyncProfiler::startTime, static_cast<size_t>(k1_stop-shift) );
        aProfiler.set(k2_stepNum, AsyncProfiler::stopTime,  static_cast<size_t>(k2_stop-shift) );

    }
    catch( ::cl::Error& e )
    {
        std::cout << ( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return;
    }

aProfiler.stopTrial();
#endif // ENABLE_PROFILING
#endif
    // profiling
    cl_command_queue_properties queueProperties;
    l_Error = ctrl.getCommandQueue().getInfo<cl_command_queue_properties>(CL_QUEUE_PROPERTIES, &queueProperties);
    unsigned int profilingEnabled = queueProperties&CL_QUEUE_PROFILING_ENABLE;
    if (1 && profilingEnabled) {
        cl_ulong start_time, stop_time;
        l_Error = generateEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &start_time);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = generateEvent.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &stop_time);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");
        size_t time = stop_time - start_time;
        double gb = (numElements*sizeof(oType))/1024.0/1024.0/1024.0;
        double sec = time/1000000000.0;
        std::cout << "Global Memory Bandwidth: " << ( gb / sec) << " ( "
          << time/1000000.0 << " ms)" << std::endl;
    }
}; // end generate_enqueue

}//End of detail namespace
}//End of cl namespace
}//End of bolt namespace

#endif
