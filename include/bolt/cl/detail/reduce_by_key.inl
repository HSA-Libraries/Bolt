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


#if !defined( BOLT_CL_REDUCE_BY_KEY_INL )
#define BOLT_CL_REDUCE_BY_KEY_INL

#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define WAVESIZE 64

#define LENGTH_TEST 10
#define ENABLE_PRINTS 0

#include <iostream>
#include <fstream>

#ifdef ENABLE_TBB
//TBB Includes
#include "bolt/btbb/reduce_by_key.h"
#endif

namespace bolt
{

namespace cl
{

namespace detail
{
/*!
*   \internal
*   \addtogroup detail
*   \ingroup reduction
*   \{
*/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
//bolt::cl::pair<OutputIterator1, OutputIterator2>
unsigned int
gold_reduce_by_key_enqueue( InputIterator1 keys_first,
                            InputIterator1 keys_last,
                            InputIterator2 values_first,
                            OutputIterator1 keys_output,
                            OutputIterator2 values_output,
                            BinaryPredicate binary_pred,
                            BinaryFunction binary_op )
{
    typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value,
                   "InputIterator2 and OutputIterator's value types are not convertible." );

   unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );

    // do zeroeth element
    *values_output = *values_first;
    *keys_output = *keys_first;
    unsigned int count = 1;
    // rbk oneth element and beyond

    values_first++;
    for ( InputIterator1 key = (keys_first+1); key != keys_last; key++)
    {
        // load keys
        kType currentKey  = *(key);
        kType previousKey = *(key-1);

        // load value
        voType currentValue = *values_first;
        voType previousValue = *values_output;

        previousValue = *values_output;
        // within segment
        if (binary_pred(currentKey, previousKey))
        {
            voType r = binary_op( previousValue, currentValue);
            *values_output = r;
            *keys_output = currentKey;

        }
        else // new segment
        {
            values_output++;
            keys_output++;
            *values_output = currentValue;
            *keys_output = currentKey;
            count++; //To count the number of elements in the output array
        }
        values_first++;
    }

    //return bolt::cl::make_pair(keys_output+1, values_output+1);
    return count;
}
    enum Reduce_By_Key_Types {  e_kType, e_kIterType,
                     e_vType, e_vIterType,
                     e_koType, e_koIterType,
                     e_voType, e_voIterType ,
                     e_BinaryPredicate, e_BinaryFunction,
                     e_end };



    /* "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(3) + "(\n"
            "global  int *h_result\n"
            ");\n\n"*/


/*********************************************************************************************************************
 * Kernel Template Specializer
 *********************************************************************************************************************/
class ReduceByKey_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
    public:

    ReduceByKey_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
		addKernelName("OffsetCalculation");
        addKernelName("perBlockScanByKey");
        addKernelName("intraBlockInclusiveScanByKey");
        addKernelName("perBlockAdditionByKey");
        addKernelName("keyValueMapping");
    }

    const ::std::string operator() ( const ::std::vector< ::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[e_kType] + "* ikeys,\n"
            + typeNames[e_kIterType] + " keys,\n"
            "global int * output2,\n"
            "const uint vecSize,\n"
            "global " + typeNames[e_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[e_BinaryFunction]  + "* binaryFunct\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global int *keys,\n"
            "global " + typeNames[e_vType] + "* ivals,\n"
            + typeNames[e_vIterType] + " vals,\n"
            "global " + typeNames[e_voType] + "* output,\n"
            "const uint vecSize,\n"
            "local int * ldsKeys,\n"
            "local "  + typeNames[e_voType] + "* ldsVals,\n"
            "global " + typeNames[e_BinaryFunction]  + "* binaryFunct,\n"
            "global int * keyBuffer,\n"
            "global " + typeNames[e_voType] + "* valBuffer\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(2) + "(\n"
            "global int * keySumArray,\n"
            "global " + typeNames[e_voType] + "* preSumArray,\n"
            "global " + typeNames[e_voType] + "* postSumArray,\n"
            "const uint vecSize,\n"
            "local int * ldsKeys,\n"
            "local "  + typeNames[e_voType] + "* ldsVals,\n"
            "const uint workPerThread,\n"
            "global " + typeNames[e_BinaryFunction] + "* binaryFunct\n"
            ");\n\n"


            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(3) + "(\n"
            "global int * keySumArray,\n"
            "global " + typeNames[e_voType] + "* postSumArray,\n"
            "global int *keys,\n"
            "global " + typeNames[e_voType] + "* output,\n"
            "const uint vecSize,\n"
            "global " + typeNames[e_BinaryFunction] + "* binaryFunct\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(4) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(4) + "(\n"
            "global " + typeNames[e_kType] + "*ikeys,\n"
            + typeNames[e_kIterType] + " keys,\n"
            "global " + typeNames[e_koType] + "*ikeys_output,\n"
            + typeNames[e_koIterType] + " keys_output,\n"
            "global " + typeNames[e_voType] + "*ivals_output,\n"
            + typeNames[e_voIterType] + " vals_output,\n"
            "global int *offsetArray,\n"
            "global " + typeNames[e_voType] + "*offsetValArray,\n"
            "const uint vecSize,\n"
            "int numSections\n"
            ");\n\n";

        return templateSpecializationString;
    }
};


//  All calls to reduce_by_key end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator1,
    typename DVOutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction >
unsigned int
reduce_by_key_enqueue(
    control& ctl,
    const DVInputIterator1& keys_first,
    const DVInputIterator1& keys_last,
    const DVInputIterator2& values_first,
    const DVOutputIterator1& keys_output,
    const DVOutputIterator2& values_output,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_op,
    const std::string& user_code)
{
    cl_int l_Error;

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef typename std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< DVInputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< DVOutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< DVOutputIterator2 >::value_type voType;
    std::vector<std::string> typeNames(e_end);
    typeNames[e_kType] = TypeName< kType >::get( );
    typeNames[e_kIterType] = TypeName< DVInputIterator1 >::get( );
    typeNames[e_vType] = TypeName< vType >::get( );
    typeNames[e_vIterType] = TypeName< DVInputIterator2 >::get( );
    typeNames[e_koType] = TypeName< koType >::get( );
    typeNames[e_koIterType] = TypeName< DVInputIterator1 >::get( );
    typeNames[e_voType] = TypeName< voType >::get( );
    typeNames[e_voIterType] = TypeName< DVInputIterator2 >::get( );
    typeNames[e_BinaryPredicate] = TypeName< BinaryPredicate >::get( );
    typeNames[e_BinaryFunction]  = TypeName< BinaryFunction >::get( );

    /**********************************************************************************
     * Type Definitions - directly concatenated into kernel string
     *********************************************************************************/
    /*std::vector<std::string> typeDefs; // try substituting a map
    typeDefs.push_back( ClCode< kType >::get() );
    if (TypeName< vType >::get() != TypeName< kType >::get())
    {
        typeDefs.push_back( ClCode< vType >::get() );
    }
    if (TypeName< oType >::get() != TypeName< kType >::get() &&
        TypeName< oType >::get() != TypeName< vType >::get())
    {
        typeDefs.push_back( ClCode< oType >::get() );
    }
    typeDefs.push_back( ClCode< BinaryPredicate >::get() );
    typeDefs.push_back( ClCode< BinaryFunction >::get() );*/
    std::vector<std::string> typeDefs; // typeDefs must be unique and order does matter
    PUSH_BACK_UNIQUE( typeDefs, ClCode< kType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< DVInputIterator1 >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< vType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< DVInputIterator2 >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< koType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< voType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< BinaryPredicate >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< BinaryFunction  >::get() )

    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    bool cpuDevice = ctl.getDevice().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
    //std::cout << "Device is CPU: " << (cpuDevice?"TRUE":"FALSE") << std::endl;
    const size_t kernel0_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    const size_t kernel1_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL1WAVES;
    const size_t kernel2_WgSize = (cpuDevice) ? 1 : WAVESIZE*KERNEL02WAVES;
    std::string compileOptions;
    std::ostringstream oss;
    oss << " -DKERNEL0WORKGROUPSIZE=" << kernel0_WgSize;
    oss << " -DKERNEL1WORKGROUPSIZE=" << kernel1_WgSize;
    oss << " -DKERNEL2WORKGROUPSIZE=" << kernel2_WgSize;
    compileOptions = oss.str();

    /**********************************************************************************
     * Request Compiled Kernels
     *********************************************************************************/
    ReduceByKey_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefs,
        reduce_by_key_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor

    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent, kernel3Event, kernel4Event;

    // Set up shape of launch grid and buffers:
    int computeUnits     = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    int wgPerComputeUnit =  ctl.getWGPerComputeUnit( );
    int resultCnt = computeUnits * wgPerComputeUnit;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( keys_first, keys_last ) );
    typename device_vector< kType >::size_type sizeInputBuff = numElements;
    size_t modWgSize = (sizeInputBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += kernel0_WgSize;
    }
    cl_uint numWorkGroupsK0 = static_cast< cl_uint >( sizeInputBuff / kernel0_WgSize );

    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    typename device_vector< kType >::size_type sizeScanBuff = numWorkGroupsK0;
    modWgSize = (sizeScanBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeScanBuff &= ~modWgSize;
        sizeScanBuff += kernel0_WgSize;
    }

    // Create buffer wrappers so we can access the host functors, for read or writing in the kernel

    ALIGNED( 256 ) BinaryPredicate aligned_binary_pred( binary_pred );
    control::buffPointer binaryPredicateBuffer = ctl.acquireBuffer( sizeof( aligned_binary_pred ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary_pred );
     ALIGNED( 256 ) BinaryFunction aligned_binary_op( binary_op );
    control::buffPointer binaryFunctionBuffer = ctl.acquireBuffer( sizeof( aligned_binary_op ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary_op );

    control::buffPointer keySumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( int ) );
    control::buffPointer preSumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( voType ) );
    control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( voType ) );
    control::buffPointer offsetValArray  = ctl.acquireBuffer( numElements *sizeof( voType ) );


    device_vector< int > offsetArrayVec( numElements, 0, CL_MEM_READ_WRITE, false, ctl);
    ::cl::Buffer offsetArray = offsetArrayVec.begin( ).getContainer().getBuffer();



    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
    typename DVInputIterator1::Payload keys_first_payload = keys_first.gpuPayload( );
    try
    {
    V_OPENCL( kernels[0].setArg( 0, keys_first.getContainer().getBuffer()), "Error setArg kernels[ 0 ]" ); // Input keys
    V_OPENCL( kernels[0].setArg( 1, keys_first.gpuPayloadSize( ),&keys_first_payload ), "Error setArg kernels[ 0 ]" );
    V_OPENCL( kernels[0].setArg( 2, offsetArray ), "Error setArg kernels[ 0 ]" ); // Output keys
    V_OPENCL( kernels[0].setArg( 3, numElements ), "Error setArg kernels[ 0 ]" ); // vecSize
    V_OPENCL( kernels[0].setArg( 4, *binaryPredicateBuffer),"Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 5, *binaryFunctionBuffer ),"Error setArg kernels[ 0 ]" ); // User provided functor

    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[0],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel0Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[0]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 0 ) in bolt::cl::reduce_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }
    // wait for results
    l_Error = kernel0Event.wait( );
    V_OPENCL( l_Error, "post-kernel[0] failed wait" );

    detail::scan_enqueue(ctl, offsetArrayVec.begin(), offsetArrayVec.end(), offsetArrayVec.begin(), 0, plus< int >( ), true);

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
    cl_uint ldsKeySize, ldsValueSize;
    typename DVInputIterator2::Payload values_first_payload = values_first.gpuPayload( );
    try
    {
    ldsKeySize   = static_cast< cl_uint >( kernel0_WgSize * sizeof( int ) );
    ldsValueSize = static_cast< cl_uint >( kernel0_WgSize * sizeof( voType ) );
    V_OPENCL( kernels[1].setArg( 0, offsetArray), "Error setArg kernels[ 1 ]" ); // Input keys
    V_OPENCL( kernels[1].setArg( 1, values_first.getContainer().getBuffer()),"Error setArg kernels[ 1 ]" ); // Input values
    V_OPENCL( kernels[1].setArg( 2, values_first.gpuPayloadSize( ),&values_first_payload ), "Error setArg kernels[ 1 ]" ); // Input values
    V_OPENCL( kernels[1].setArg( 3, *offsetValArray ), "Error setArg kernels[ 1 ]" ); // Output values
    V_OPENCL( kernels[1].setArg( 4, numElements ), "Error setArg kernels[ 1 ]" ); // vecSize
    V_OPENCL( kernels[1].setArg( 5, ldsKeySize, NULL ),     "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 6, ldsValueSize, NULL ),   "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 7, *binaryFunctionBuffer ),"Error setArg kernels[ 1 ]" ); // User provided functor
    V_OPENCL( kernels[1].setArg( 8, *keySumArray ),         "Error setArg kernels[ 1 ]" ); // Output per block sum
    V_OPENCL( kernels[1].setArg( 9, *preSumArray ),         "Error setArg kernels[ 1 ]" ); // Output per block sum

    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[1],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel1Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[1]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 1 ) in bolt::cl::reduce_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    /**********************************************************************************
     *  Kernel 2
     *********************************************************************************/
    cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / kernel1_WgSize );
    V_OPENCL( kernels[2].setArg( 0, *keySumArray ),         "Error setArg kernels[ 2 ]" ); // Input keys
    V_OPENCL( kernels[2].setArg( 1, *preSumArray ),         "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 2, *postSumArray ),        "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 3, numWorkGroupsK0 ),      "Error setArg kernels[ 2 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[2].setArg( 4, ldsKeySize, NULL ),     "Error setArg kernels[ 2 ]" ); // Scratch buffer
    V_OPENCL( kernels[2].setArg( 5, ldsValueSize, NULL ),   "Error setArg kernels[ 2 ]" ); // Scratch buffer
    V_OPENCL( kernels[2].setArg( 6, workPerThread ),        "Error setArg kernels[ 2 ]" ); // Work Per Thread
    V_OPENCL( kernels[2].setArg( 7, *binaryFunctionBuffer ),"Error setArg kernels[ 2 ]" ); // User provided functor

    try
    {
    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[2],
        ::cl::NullRange,
        ::cl::NDRange( kernel1_WgSize ), // only 1 work-group
        ::cl::NDRange( kernel1_WgSize ),
        NULL,
        &kernel2Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[2]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 2 ) in bolt::cl::reduce_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

#if ENABLE_PRINTS
    //delete this code -start
    bolt::cl::wait(ctl, kernel0Event);
    bolt::cl::wait(ctl, kernel1Event);
    ::cl::Event l_mapEvent_k1;
    voType *post_sum_k1= (voType*)ctl.commandQueue().enqueueMapBuffer( *postSumArray,
                                                                    false,
                                                                    CL_MAP_READ,
                                                                    0,
                                                                    sizeof(voType)*sizeScanBuff,
                                                                    NULL,
                                                                    &l_mapEvent_k1,
                                                                    &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
    std::cout<<"Myval-------------------------starts"<<std::endl;
    std::ofstream postsum("postsum.txt");
    for(unsigned int i = 0; i < sizeScanBuff ; i++)
    {
        postsum<<post_sum_k1[i]<<std::endl;
    }
    postsum.close();
    std::cout<<"Myval-------------------------ends"<<std::endl;
    bolt::cl::wait(ctl, l_mapEvent_k1);
    //delete this code -end

#endif

    /**********************************************************************************
     *  Kernel 3
     *********************************************************************************/
    V_OPENCL( kernels[3].setArg( 0, *keySumArray ),         "Error setArg kernels[ 3 ]" ); // Input buffer
    V_OPENCL( kernels[3].setArg( 1, *postSumArray ),        "Error setArg kernels[ 3 ]" ); // Input buffer
    V_OPENCL( kernels[3].setArg( 2, offsetArray), "Error setArg kernels[ 3 ]" ); // Input keys
    V_OPENCL( kernels[3].setArg( 3, *offsetValArray),   "Error setArg kernels[ 3 ]" ); // Output buffer
    V_OPENCL( kernels[3].setArg( 4, numElements ),          "Error setArg kernels[ 3 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[3].setArg( 5, *binaryFunctionBuffer ),"Error setArg kernels[ 3 ]" ); // User provided functor

    try
    {
    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[3],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel2_WgSize ),
        NULL,
        &kernel2Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[3]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 3 ) in bolt::cl::reduce_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }


#if ENABLE_PRINTS
    //delete this code -start
    ::cl::Event l_mapEvent2;
    voType *v_result = (voType*)ctl.commandQueue().enqueueMapBuffer( *offsetValArray,
                                                                    false,
                                                                    CL_MAP_READ,
                                                                    0,
                                                                    sizeof(voType)*numElements,
                                                                    NULL,
                                                                    &l_mapEvent2,
                                                                    &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
    std::cout<<"Myval-------------------------starts"<<std::endl;
    std::ofstream val_result("offsetValArray.txt");
    val_result<<numElements<<std::endl;
    for(unsigned int i = 0; i < LENGTH_TEST ; i++)
    {
        val_result<<v_result[i]<<std::endl;
    }
    val_result.close();
    std::cout<<"Myval-------------------------ends"<<std::endl;
    bolt::cl::wait(ctl, l_mapEvent2);
    //delete this code -end
    std::ofstream result_b4_ser("result_b4_ser.txt");
    for(unsigned int i = 0; i < LENGTH_TEST ; i++)
    {
        //std::cout<<h_result[i]<<std::endl;

        result_b4_ser<<h_result[i]<<std::endl;
    }
    result_b4_ser.close();
#endif




#if ENABLE_PRINTS
    std::cout<<count_number_of_sections<<std::endl;
    std::ofstream result_file("offsetArray.txt");
    for(unsigned int i = 0; i < LENGTH_TEST ; i++)
    {
        result_file<<h_result[i]<<std::endl;
    }
    result_file.close();

#endif
    unsigned int count_number_of_sections = 0;
    /**********************************************************************************
     *  Kernel 4
     *********************************************************************************/
    typename DVInputIterator2::Payload keys_first1_payload = keys_first.gpuPayload( );
    typename DVOutputIterator1::Payload keys_output_payload = keys_output.gpuPayload( );
    typename DVOutputIterator2::Payload values_output_payload = values_output.gpuPayload( );

    V_OPENCL( kernels[4].setArg( 0, keys_first.getContainer().getBuffer()),    "Error setArg kernels[ 4 ]" ); // Input buffer
    V_OPENCL( kernels[4].setArg( 1, keys_first.gpuPayloadSize( ), &keys_first1_payload), "Error setArg kernels[ 4 ]" );
    V_OPENCL( kernels[4].setArg( 2, keys_output.getContainer().getBuffer() ),  "Error setArg kernels[ 4 ]" ); // Output buffer
    V_OPENCL( kernels[4].setArg( 3, keys_output.gpuPayloadSize( ),&keys_output_payload ), "Error setArg kernels[ 4 ]" );
    V_OPENCL( kernels[4].setArg( 4, values_output.getContainer().getBuffer()), "Error setArg kernels[ 4 ]" ); // Output buffer
    V_OPENCL( kernels[4].setArg( 5, values_output.gpuPayloadSize( ),&values_output_payload ), "Error setArg kernels[ 4 ]" );
    V_OPENCL( kernels[4].setArg( 6, offsetArray),                "Error setArg kernels[ 4 ]" ); // Input buffer
    V_OPENCL( kernels[4].setArg( 7, *offsetValArray),             "Error setArg kernels[ 4 ]"  );
    V_OPENCL( kernels[4].setArg( 8, numElements ),             "Error setArg kernels[ 4 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[4].setArg( 9, count_number_of_sections), "Error setArg kernels[ 4 ]" ); // Size of scratch buffer

    try
    {
    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
        kernels[4],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel4Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[3]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 3 ) in bolt::cl::reduce_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }
    // wait for results
    l_Error = kernel4Event.wait( );
    V_OPENCL( l_Error, "post-kernel[3] failed wait" );

    ::cl::Event l_mapEvent;
    int *h_result = (int*)ctl.getCommandQueue().enqueueMapBuffer( offsetArray,
                                                                    false,
                                                                    CL_MAP_READ | CL_MAP_WRITE,
                                                                    (numElements-1)*sizeof(int),
                                                                    sizeof(int)*1,
                                                                    NULL,
                                                                    &l_mapEvent,
                                                                    &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );

    bolt::cl::wait(ctl, l_mapEvent);


    count_number_of_sections = *(h_result);



#if ENABLE_PRINTS
    //delete this code -start
    ::cl::Event l_mapEvent3;
    voType *v_result2 = (voType*)ctl.commandQueue().enqueueMapBuffer( *offsetValArray,
                                                                    false,
                                                                    CL_MAP_READ,
                                                                    0,
                                                                    sizeof(voType)*numElements,
                                                                    NULL,
                                                                    &l_mapEvent3,
                                                                    &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
    std::cout<<"Myval-------------------------starts"<<std::endl;
    std::ofstream result_val_after_launch("result_val_after_launch.txt");
    for(unsigned int i = 0; i < LENGTH_TEST ; i++)
    {
        result_val_after_launch<<v_result2[i]<<std::endl;
    }
    result_val_after_launch.close();
    std::cout<<"Myval-------------------------ends"<<std::endl;
    bolt::cl::wait(ctl, l_mapEvent3);
    //delete this code -end

#endif
    return count_number_of_sections;
    }   //end of reduce_by_key_enqueue( )



/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if<
             !(std::is_base_of<typename device_vector<typename
               std::iterator_traits<InputIterator1>::value_type>::iterator,InputIterator1>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<InputIterator2>::value_type>::iterator,InputIterator2>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<OutputIterator1>::value_type>::iterator,OutputIterator2>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<OutputIterator1>::value_type>::iterator,OutputIterator2>::value),
         bolt::cl::pair<OutputIterator1, OutputIterator2> >::type
reduce_by_key_pick_iterator(
    control& ctl,
    const InputIterator1& keys_first,
    const InputIterator1& keys_last,
    const InputIterator2& values_first,
    const OutputIterator1& keys_output,
    const OutputIterator2& values_output,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_op,
    const std::string& user_code)
{
    //std::cout<<"Input Iterator, Output Iterator"<<std::endl;

    typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );
    if( numElements == 1 )
        return bolt::cl::make_pair( keys_last, values_first+numElements );

    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode();  // could be dynamic choice some day.
    if(runMode == bolt::cl::control::Automatic) {
        runMode = ctl.getDefaultPathToRun();
    }
	#if defined(BOLT_DEBUG_LOG)
    BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
    #endif
				
    if (runMode == bolt::cl::control::SerialCpu) {
            #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce_By_Key::SERIAL_CPU");
            #endif
            unsigned int sizeOfOut = gold_reduce_by_key_enqueue( keys_first, keys_last, values_first, keys_output,
            values_output, binary_pred, binary_op);

            return  bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);

    } else if (runMode == bolt::cl::control::MultiCoreCpu) {

        #ifdef ENABLE_TBB
		    #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_MULTICORE_CPU,"::Reduce_By_Key::MULTICORE_CPU");
            #endif
            unsigned int sizeOfOut = bolt::btbb::reduce_by_key( keys_first, keys_last, values_first, keys_output, values_output, binary_pred, binary_op);
            return  bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);
        #else
            throw std::runtime_error("MultiCoreCPU Version of ReduceByKey not Enabled! \n");
        #endif
    }
    else {
     #if defined(BOLT_DEBUG_LOG)
     dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_OPENCL_GPU,"::Reduce_By_Key::OPENCL_GPU");
     #endif
    unsigned int sizeOfOut;
    {

        // Map the input iterator to a device_vector
        device_vector< kType > dvKeys( keys_first, keys_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        device_vector< vType > dvValues( values_first, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,true,ctl );
        device_vector< koType > dvKOutput( keys_output, numElements,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,false,ctl);
        device_vector< voType > dvVOutput(values_output,numElements,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,false,ctl);

        //Now call the actual cl algorithm
        sizeOfOut = reduce_by_key_enqueue( ctl, dvKeys.begin( ), dvKeys.end( ), dvValues.begin(), dvKOutput.begin( ),
                                          dvVOutput.begin( ), binary_pred, binary_op, user_code);

        // This should immediately map/unmap the buffer
        dvKOutput.data( );
        dvVOutput.data( );
    }
    return bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);
    }
}

/*!
* \brief This overload is called strictly for device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/

    template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator1,
    typename DVOutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if<
             (std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVInputIterator1>::value_type>::iterator,DVInputIterator1>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVInputIterator2>::value_type>::iterator,DVInputIterator2>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVOutputIterator1>::value_type>::iterator,DVOutputIterator1>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVOutputIterator2>::value_type>::iterator,DVOutputIterator2>::value),
          bolt::cl::pair<DVOutputIterator1, DVOutputIterator2> >::type
reduce_by_key_pick_iterator(
    control& ctl,
    const DVInputIterator1& keys_first,
    const DVInputIterator1& keys_last,
    const DVInputIterator2& values_first,
    const DVOutputIterator1& keys_output,
    const DVOutputIterator2& values_output,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_op,
    const std::string& user_code)
{
    //std::cout<<"DVInput Iterator, DVOutput Iterator"<<std::endl;

    typedef typename std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< DVInputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< DVOutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< DVOutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );
     if( numElements == 1 )
        return bolt::cl::make_pair( keys_last, values_first+numElements );

    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );  // could be dynamic choice some day.
    if(runMode == bolt::cl::control::Automatic)
    {
        runMode = ctl.getDefaultPathToRun();
    }
    #if defined(BOLT_DEBUG_LOG)
    BOLTLOG::CaptureLog *dblog = BOLTLOG::CaptureLog::getInstance();
    #endif
	
    if( runMode == bolt::cl::control::SerialCpu )
    {
	    #if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_SERIAL_CPU,"::Reduce_By_Key::SERIAL_CPU");
        #endif
			
        typename bolt::cl::device_vector< kType >::pointer keysPtr =  keys_first.getContainer( ).data( );
        typename bolt::cl::device_vector< vType >::pointer valsPtr =  values_first.getContainer( ).data( );
        typename bolt::cl::device_vector< koType >::pointer oKeysPtr =  keys_output.getContainer( ).data( );
        typename bolt::cl::device_vector< voType >::pointer oValsPtr =  values_output.getContainer( ).data( );
        unsigned int sizeOfOut = gold_reduce_by_key_enqueue( &keysPtr[keys_first.m_Index], &keysPtr[numElements],
                                           &valsPtr[values_first.m_Index], &oKeysPtr[keys_output.m_Index],
                                          &oValsPtr[values_output.m_Index], binary_pred, binary_op);
        return bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);

    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        #ifdef ENABLE_TBB
		#if defined(BOLT_DEBUG_LOG)
        dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_MULTICORE_CPU,"::Reduce_By_Key::MULTICORE_CPU");
        #endif
        typename bolt::cl::device_vector< kType >::pointer keysPtr =  keys_first.getContainer( ).data( );
        typename bolt::cl::device_vector< vType >::pointer valsPtr =  values_first.getContainer( ).data( );
        typename bolt::cl::device_vector< koType >::pointer oKeysPtr =  keys_output.getContainer( ).data( );
        typename bolt::cl::device_vector< voType >::pointer oValsPtr =  values_output.getContainer( ).data( );

        unsigned int sizeOfOut = bolt::btbb::reduce_by_key( &keysPtr[keys_first.m_Index],/*&keysPtr[keys_first.m_Index]+numElements*/ &keysPtr[numElements],
                                           &valsPtr[values_first.m_Index], &oKeysPtr[keys_output.m_Index],
                                          &oValsPtr[values_output.m_Index], binary_pred, binary_op);
        return bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);
        #else
            throw std::runtime_error("MultiCoreCPU Version of ReduceByKey not Enabled! \n");
        #endif
    }
    else
    {
	        #if defined(BOLT_DEBUG_LOG)
            dblog->CodePathTaken(BOLTLOG::BOLT_REDUCEBYKEY,BOLTLOG::BOLT_OPENCL_GPU,"::Reduce_By_Key::OPENCL_GPU");
            #endif
            //Now call the actual cl algorithm
            unsigned int sizeOfOut = reduce_by_key_enqueue( ctl, keys_first, keys_last, values_first, keys_output,
            values_output, binary_pred, binary_op, user_code);

            return  bolt::cl::make_pair(keys_output+sizeOfOut, values_output+sizeOfOut);
    }
}

/*********************************************************************************************************************
 * Detect Random Access
 ********************************************************************************************************************/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction
    >
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key_detect_random_access(
    control &ctl,
    const InputIterator1  keys_first,
    const InputIterator1  keys_last,
    const InputIterator2  values_first,
    const OutputIterator1  keys_output,
    const OutputIterator2  values_output,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_op,
    const std::string& user_code,
    std::random_access_iterator_tag )
{
    return detail::reduce_by_key_pick_iterator( ctl, keys_first, keys_last, values_first, keys_output, values_output,
        binary_pred, binary_op, user_code);
}
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key_detect_random_access(
    control &ctl,
    const InputIterator1  keys_first,
    const InputIterator1  keys_last,
    const InputIterator2  values_first,
    const OutputIterator1  keys_output,
    const OutputIterator2  values_output,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_op,
    const std::string& user_code,
    std::input_iterator_tag )
{
    //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( std::is_same< InputIterator1, std::input_iterator_tag >::value , "Bolt only supports random access iterator types" );
};


    /*!   \}  */
} //namespace detail



/**********************************************************************************************************************
 * REDUCE BY KEY
 *********************************************************************************************************************/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    BinaryPredicate binary_pred,
    BinaryFunction binary_op,
    const std::string& user_code )
{
    control& ctl = control::getDefault();
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<OutputIterator2>::value_type ValOType;
    control& ctl = control::getDefault();
    plus<ValOType> binary_op;
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator1>::value_type kType;
    typedef typename std::iterator_traits<OutputIterator2>::value_type ValOType;
    control& ctl = control::getDefault();
    equal_to <kType> binary_pred;
    plus <ValOType> binary_op;
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}


///////////////////////////// CTRL ////////////////////////////////////////////

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    control &ctl,
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    BinaryPredicate binary_pred,
    BinaryFunction binary_op,
    const std::string& user_code )
{
    //typedef std::iterator_traits<OutputIterator1>::value_type KeyOType;
    //KeyOType init; memset(&init, 0, sizeof(KeyOType) );
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    control &ctl,
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<OutputIterator2>::value_type ValOType;
    //KeyOType init; memset(&init, 0, sizeof(KeyOType) );
    plus<ValOType> binary_op;
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2>
bolt::cl::pair<OutputIterator1, OutputIterator2>
reduce_by_key(
    control &ctl,
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    const std::string& user_code )
{
    typedef typename std::iterator_traits<InputIterator1>::value_type kType;
    typedef typename std::iterator_traits<OutputIterator2>::value_type ValOType;
    //KeyOType init; memset(&init, 0, sizeof(KeyOType) );
    equal_to <kType> binary_pred;
    plus<ValOType> binary_op;
    return detail::reduce_by_key_detect_random_access(
        ctl,
        keys_first,
        keys_last,
        values_first,
        keys_output,
        values_output,
        binary_pred,
        binary_op,
        user_code,
        typename std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}



} //namespace cl
} //namespace bolt

#endif
