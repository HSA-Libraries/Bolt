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
#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define WAVESIZE 64


#if !defined( REDUCE_BY_KEY_INL )
#define REDUCE_BY_KEY_INL


namespace bolt
{

namespace cl
{

/***********************************************************************************************************************
 * REDUCE BY KEY
 **********************************************************************************************************************/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
void
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
    //typedef std::iterator_traits<OutputIterator1>::value_type KeyOType;
    control& ctl = control::getDefault();
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate>
void
reduce_by_key(
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator2>::value_type ValOType;
    control& ctl = control::getDefault();
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2>
void
reduce_by_key(
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator2>::value_type ValOType;
    control& ctl = control::getDefault();
    //KeyOType init; memset(&init, 0, sizeof(KeyOType) );
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
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
void
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate>
void
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
    typedef std::iterator_traits<OutputIterator2>::value_type ValOType;
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2>
void
reduce_by_key(
    control &ctl,
    InputIterator1  keys_first,
    InputIterator1  keys_last,
    InputIterator2  values_first,
    OutputIterator1  keys_output,
    OutputIterator2  values_output,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator2>::value_type ValOType;
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
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}



namespace detail
{
/*!
*   \internal
*   \addtogroup detail
*   \ingroup scan
*   \{
*/
    enum typeName {e_kType, e_vType, e_koType, e_voType ,e_BinaryPredicate, e_BinaryFunction};

/***********************************************************************************************************************
 * Kernel Template Specializer
 **********************************************************************************************************************/
class ScanByKey_KernelTemplateSpecializer : public KernelTemplateSpecializer
{
    public:

    ScanByKey_KernelTemplateSpecializer() : KernelTemplateSpecializer()
    {
        addKernelName("perBlockScanByKey");
        addKernelName("intraBlockInclusiveScanByKey");
        addKernelName("perBlockAdditionByKey");
        addKernelName("keyValueMapping");
    }
    
    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString = 
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[e_kType] + "* keys,\n"
            "global " + typeNames[e_vType] + "* vals,\n"
            "global " + typeNames[e_koType] + "* koutput,\n"
            "global " + typeNames[e_voType] + "* voutput,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[e_kType] + "* ldsKeys,\n"
            "local "  + typeNames[e_voType] + "* ldsVals,\n"
            "global " + typeNames[e_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[e_BinaryFunction]  + "* binaryFunct,\n"
            "global " + typeNames[e_kType] + "* keyBuffer,\n"
            "global " + typeNames[e_voType] + "* valBuffer\n"
            ");\n\n"
            
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global " + typeNames[e_kType] + "* keySumArray,\n"
            "global " + typeNames[e_voType] + "* preSumArray,\n"
            "global " + typeNames[e_voType] + "* postSumArray,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[e_kType] + "* ldsKeys,\n"
            "local "  + typeNames[e_voType] + "* ldsVals,\n"
            "const uint workPerThread,\n"
            "global " + typeNames[e_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[e_BinaryFunction] + "* binaryFunct\n"
            ");\n\n"
    

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(2) + "(\n"
            "global " + typeNames[e_kType] + "* keySumArray,\n"
            "global " + typeNames[e_voType] + "* postSumArray,\n"
            "global " + typeNames[e_kType] + "* keys,\n"
            "global " + typeNames[e_voType] + "* output,\n"
            "const uint vecSize,\n"
            "global " + typeNames[e_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[e_BinaryFunction] + "* binaryFunct\n"
            ");\n\n"

            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(3) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(3) + "(\n"
	        "global " + typeNames[e_kType] + "*keys,\n"
	        "global " + typeNames[e_koType] + "*keys_output,\n"
            "global " + typeNames[e_voType] + "*vals_output,\n"
	        "const uint vecSize\n"
            ");\n\n";            
    
        return templateSpecializationString;
    }
};

/***********************************************************************************************************************
 * Detect Random Access
 **********************************************************************************************************************/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction>
void
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
    static_assert( false, "Bolt only supports random access iterator types" );
};

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction
    >
void
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
         void >::type
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
    typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );
    if( numElements < 1 )
        return;

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.

    {

        // Map the input iterator to a device_vector
        device_vector< kType > dvKeys( keys_first, keys_last, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        device_vector< vType > dvValues( values_first, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, true, ctl );
        device_vector< koType > dvKOutput( keys_output, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );
        device_vector< voType > dvVOutput( values_output, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

        //Now call the actual cl algorithm
        reduce_by_key_enqueue( ctl, dvKeys.begin( ), dvKeys.end( ), dvValues.begin(), dvKOutput.begin( ),dvVOutput.end( ), binary_pred, binary_op, user_code);

        // This should immediately map/unmap the buffer
        dvKOutput.data( );
        dvVOutput.data( );
    }

    return;
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
         void >::type
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
    typedef typename std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< DVInputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< DVOutputIterator1 >::value_type koType;
    typedef typename std::iterator_traits< DVOutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );
    if( numElements < 1 )
        return void;

    const bolt::cl::control::e_RunMode runMode = ctl.forceRunMode( );  // could be dynamic choice some day.
    if( runMode == bolt::cl::control::SerialCpu )
    {
        //  TODO:  Need access to the device_vector .data method to get a host pointer
        throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
        return void;
    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
        //  TODO:  Need access to the device_vector .data method to get a host pointer
        throw ::cl::Error( CL_INVALID_DEVICE, "Scan device_vector CPU device not implemented" );
        return void;
    }

    //Now call the actual cl algorithm
    reduce_by_key_enqueue( ctl, keys_first, keys_last, values_first, keys_output,
            values_output, binary_pred, binary_op, user_code);

    return void;
}


//  All calls to scan_by_key end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator1,
    typename DVOutputIterator2,
    typename BinaryPredicate,
    typename BinaryFunction >
void
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
    std::vector<std::string> typeNames(6);
    typeNames[e_kType] = TypeName< kType >::get( );
    typeNames[e_vType] = TypeName< vType >::get( );
    typeNames[e_koType] = TypeName< koType >::get( );
    typeNames[e_voType] = TypeName< voType >::get( );
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
    PUSH_BACK_UNIQUE( typeDefs, ClCode< vType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< koType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< voType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< BinaryPredicate >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< BinaryFunction  >::get() )

    /**********************************************************************************
     * Compile Options
     *********************************************************************************/
    bool cpuDevice = ctl.device().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;
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
    ScanByKey_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefs,
        reduce_by_key_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor

    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent, kernel3Event;
    //cl_uint doExclusiveScan = inclusive ? 0 : 1;
    // Set up shape of launch grid and buffers:
    int computeUnits     = ctl.device( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    int wgPerComputeUnit =  ctl.wgPerComputeUnit( );
    int resultCnt = computeUnits * wgPerComputeUnit;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( keys_first, keys_last ) );
    device_vector< kType >::size_type sizeInputBuff = numElements;
    size_t modWgSize = (sizeInputBuff & (kernel0_WgSize-1));
    if( modWgSize )
    {
        sizeInputBuff &= ~modWgSize;
        sizeInputBuff += kernel0_WgSize;
    }
    cl_uint numWorkGroupsK0 = static_cast< cl_uint >( sizeInputBuff / kernel0_WgSize );

    //  Ceiling function to bump the size of the sum array to the next whole wavefront size
    device_vector< kType >::size_type sizeScanBuff = numWorkGroupsK0;
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

    control::buffPointer keySumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( kType ) );
    control::buffPointer preSumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( voType ) );
    control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( voType ) );
    //control::buffPointer offsetArray  = ctl.acquireBuffer( numElements *sizeof( kType ) );
    cl_uint ldsKeySize, ldsValueSize;


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
    try
    {
    ldsKeySize   = static_cast< cl_uint >( kernel0_WgSize * sizeof( kType ) );
    ldsValueSize = static_cast< cl_uint >( kernel0_WgSize * sizeof( voType ) );
    V_OPENCL( kernels[0].setArg( 0, keys_first->getBuffer()), "Error setArg kernels[ 0 ]" ); // Input keys
    V_OPENCL( kernels[0].setArg( 1, values_first->getBuffer()),"Error setArg kernels[ 0 ]" ); // Input values
    V_OPENCL( kernels[0].setArg( 2, values_output->getBuffer( ) ), "Error setArg kernels[ 0 ]" ); // Output values
    V_OPENCL( kernels[0].setArg( 3, keys_output->getBuffer( ) ), "Error setArg kernels[ 0 ]" ); // Output keys
    V_OPENCL( kernels[0].setArg( 4, numElements ), "Error setArg kernels[ 0 ]" ); // vecSize
    V_OPENCL( kernels[0].setArg( 5, ldsKeySize, NULL ),     "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[0].setArg( 6, ldsValueSize, NULL ),   "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[0].setArg( 7, *binaryPredicateBuffer),"Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 8, *binaryFunctionBuffer ),"Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 9, *keySumArray ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[0].setArg( 10, *preSumArray ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
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
        std::cerr << "::cl::enqueueNDRangeKernel( 0 ) in bolt::cl::scan_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
    cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / kernel1_WgSize );
    V_OPENCL( kernels[1].setArg( 0, *keySumArray ),         "Error setArg kernels[ 1 ]" ); // Input keys
    V_OPENCL( kernels[1].setArg( 1, *preSumArray ),         "Error setArg kernels[ 1 ]" ); // Input buffer
    V_OPENCL( kernels[1].setArg( 2, *postSumArray ),        "Error setArg kernels[ 1 ]" ); // Output buffer
    V_OPENCL( kernels[1].setArg( 3, numWorkGroupsK0 ),      "Error setArg kernels[ 1 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[1].setArg( 4, ldsKeySize, NULL ),     "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 5, ldsValueSize, NULL ),   "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 6, workPerThread ),        "Error setArg kernels[ 1 ]" ); // Work Per Thread
    V_OPENCL( kernels[1].setArg( 7, *binaryPredicateBuffer ),"Error setArg kernels[ 1 ]" ); // User provided functor
    V_OPENCL( kernels[1].setArg( 8, *binaryFunctionBuffer ),"Error setArg kernels[ 1 ]" ); // User provided functor

    try
    {
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[1],
        ::cl::NullRange,
        ::cl::NDRange( kernel1_WgSize ), // only 1 work-group
        ::cl::NDRange( kernel1_WgSize ),
        NULL,
        &kernel1Event);
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[1]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 1 ) in bolt::cl::scan_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    /**********************************************************************************
     *  Kernel 2
     *********************************************************************************/
    V_OPENCL( kernels[2].setArg( 0, *keySumArray ),         "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 1, *postSumArray ),        "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 2, keys_first->getBuffer()), "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 3, values_output->getBuffer()),   "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 4, numElements ),          "Error setArg kernels[ 2 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[2].setArg( 5, *binaryPredicateBuffer ),"Error setArg kernels[ 2 ]" ); // User provided functor
    V_OPENCL( kernels[2].setArg( 6, *binaryFunctionBuffer ),"Error setArg kernels[ 2 ]" ); // User provided functor

    try
    {
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[2],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel2_WgSize ),
        NULL,
        &kernel2Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[2]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 2 ) in bolt::cl::scan_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    // wait for results
    l_Error = kernel2Event.wait( );
    V_OPENCL( l_Error, "post-kernel[2] failed wait" );

    //
    // Now take the output keys and increment each non-zero value from previous value.
    // This is done serially. It doesn't look GPU friendly.
    //

    ::cl::Event l_mapEvent;
    voType *h_result = (voType*)ctl.commandQueue().enqueueMapBuffer(keys_output->getBuffer(),
                                                                    false, CL_MAP_WRITE | CL_MAP_READ,
                                                                    0, sizeof(voType)*numElements,
                                                                    NULL, &l_mapEvent, &l_Error );
    V_OPENCL( l_Error, "Error calling map on the result buffer" );
    
    bolt::cl::wait(ctl, l_mapEvent);

    // Doing this serially; Performance killer!!
    unsigned int count_number_of_sections = 1;
    for(unsigned int i = 1; i < numElements; i++)
    {        
        if(h_result[i])
        {
            count_number_of_sections++;
            h_result[i] = count_number_of_sections;
        }
    }

    //for(unsigned int i = 0; i < 256 ; i++)
    //{
    //    std::cout<<h_result[i]<<std::endl;
    //}

    //
    // Now unmap it. We map the indices back to keys_output using "keys_output"
    //

    ::cl::Event l_unmapEvent;
    cl_int l_unmapError = CL_SUCCESS;
    l_Error = ctl.commandQueue().enqueueUnmapMemObject( keys_output->getBuffer(), h_result, NULL, &l_unmapEvent );
    V_OPENCL( l_unmapError, "device_vector failed to unmap host memory back to device memory" );
    V_OPENCL( l_unmapEvent.wait( ), "failed to wait for unmap event" );



    /**********************************************************************************
     *  Kernel 3
     *********************************************************************************/
    V_OPENCL( kernels[3].setArg( 0, keys_first->getBuffer()),    "Error setArg kernels[ 3 ]" ); // Input buffer
    V_OPENCL( kernels[3].setArg( 1, keys_output->getBuffer() ),  "Error setArg kernels[ 3 ]" ); // Input buffer
    V_OPENCL( kernels[3].setArg( 2, values_output->getBuffer()), "Error setArg kernels[ 3 ]" ); // Output buffer
    V_OPENCL( kernels[3].setArg( 3, numElements ),               "Error setArg kernels[ 3 ]" ); // Size of scratch buffer

    try
    {
    l_Error = ctl.commandQueue( ).enqueueNDRangeKernel(
        kernels[3],
        ::cl::NullRange,
        ::cl::NDRange( sizeInputBuff ),
        ::cl::NDRange( kernel0_WgSize ),
        NULL,
        &kernel3Event );
    V_OPENCL( l_Error, "enqueueNDRangeKernel() failed for kernel[3]" );
    }
    catch( const ::cl::Error& e)
    {
        std::cerr << "::cl::enqueueNDRangeKernel( 3 ) in bolt::cl::scan_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }
    // wait for results
    l_Error = kernel3Event.wait( );
    V_OPENCL( l_Error, "post-kernel[3] failed wait" );



#if 0
    try
    {
        double k0_globalMemory = 2.0*sizeInputBuff*sizeof(iType) + 1*sizeScanBuff*sizeof(iType);
        double k1_globalMemory = 4.0*sizeScanBuff*sizeof(iType);
        double k2_globalMemory = 2.0*sizeInputBuff*sizeof(iType) + 1*sizeScanBuff*sizeof(iType);
        cl_ulong k0_start, k0_end, k1_start, k1_end, k2_start, k2_end;

        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_end);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_end);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k2_end);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");

        double k0_sec = (k0_end-k0_start)/1000000000.0;
        double k1_sec = (k1_end-k1_start)/1000000000.0;
        double k2_sec = (k2_end-k2_start)/1000000000.0;

        double k0_GBs = k0_globalMemory/(1024*1024*1024*k0_sec);
        double k1_GBs = k1_globalMemory/(1024*1024*1024*k1_sec);
        double k2_GBs = k2_globalMemory/(1024*1024*1024*k2_sec);

        double k0_ms = k0_sec*1000.0;
        double k1_ms = k1_sec*1000.0;
        double k2_ms = k2_sec*1000.0;

        printf("Kernel Profile:\n\t%7.3f GB/s  (%4.0f MB in %6.3f ms)\n\t%7.3f GB/s"
            "  (%4.0f MB in %6.3f ms)\n\t%7.3f GB/s  (%4.0f MB in %6.3f ms)\n",
            k0_GBs, k0_globalMemory/1024/1024, k0_ms,
            k1_GBs, k1_globalMemory/1024/1024, k1_ms,
            k2_GBs, k2_globalMemory/1024/1024, k2_ms);

    }
    catch( ::cl::Error& e )
    {
        std::cout << ( "Reduce By Key Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return;
    }
#endif

}   //end of reduce_by_key_enqueue( )

    /*!   \}  */
} //namespace detail
} //namespace cl
} //namespace bolt

#endif
