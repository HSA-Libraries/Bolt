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
#define KERNEL02WAVES 4
#define KERNEL1WAVES 4
#define WAVESIZE 64


#if !defined( SCAN_BY_KEY_INL )
#define SCAN_BY_KEY_INL

#ifdef ENABLE_TBB
//TBB Includes
#include "tbb/parallel_scan.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#endif
#ifdef BOLT_ENABLE_PROFILING
#include "bolt/AsyncProfiler.h"
//AsyncProfiler aProfiler("transform_scan");
#endif


namespace bolt
{
namespace cl
{

/***********************************************************************************************************************
 * Inclusive Segmented Scan
 **********************************************************************************************************************/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    oType init; memset(&init, 0, sizeof(oType) );
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    oType init; memset(&init, 0, sizeof(oType) );
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    oType init; memset(&init, 0, sizeof(oType) );
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

///////////////////////////// CTRL ////////////////////////////////////////////

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    oType init; memset(&init, 0, sizeof(oType) );
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        true, // inclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename kType,
    typename vType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction>
oType*
Serial_inclusive_scan_by_key(
    kType *firstKey,
    vType *values,
    oType *result,
    unsigned int  num,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_op)
{
    // do zeroeth element
    *result = *values; // assign value
    // scan oneth element and beyond
    for ( unsigned int i=1; i< num;  i++)
    {
        // load keys
        kType currentKey  = *(firstKey+i);
        kType previousKey = *(firstKey-1 + i);
        // load value
        oType currentValue = *(values+i); // convertible
        oType previousValue = *(result-1+i);

        // within segment
        if (binary_pred(currentKey, previousKey))
        {
            oType r = binary_op( previousValue, currentValue);
            *(result+i) = r;
        }
        else // new segment
        {
            *(result + i) = currentValue;
        }
    }

    return result;
}




/***********************************************************************************************************************
 * Exclusive Segmented Scan
 **********************************************************************************************************************/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    control& ctl = control::getDefault();
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    control& ctl = control::getDefault();
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        0,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

///////////////////////////// CTRL ////////////////////////////////////////////

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    BinaryFunction  binary_funct,
    const std::string& user_code )
{
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    BinaryPredicate binary_pred,
    const std::string& user_code )
{
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    T               init,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        init,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
    control &ctl,
    InputIterator1  first1,
    InputIterator1  last1,
    InputIterator2  first2,
    OutputIterator  result,
    const std::string& user_code )
{
    typedef std::iterator_traits<InputIterator1>::value_type kType;
    typedef std::iterator_traits<OutputIterator>::value_type oType;
    equal_to<kType> binary_pred;
    plus<oType> binary_funct;
    return detail::scan_by_key_detect_random_access(
        ctl,
        first1,
        last1,
        first2,
        result,
        0,
        binary_pred,
        binary_funct,
        user_code,
        false, // exclusive
        std::iterator_traits< InputIterator1 >::iterator_category( )
    ); // return
}

template<
    typename kType,
    typename vType,
    typename oType,
    typename BinaryPredicate,
    typename BinaryFunction,
    typename T>
oType*
Serial_exclusive_scan_by_key(
    kType *firstKey,
    vType *values,
    oType *result,
    unsigned int  num,
    const BinaryPredicate binary_pred,
    const BinaryFunction binary_op,
    const T &init)
{
    // do zeroeth element
    //*result = *values; // assign value
    *result = (vType)init;
    // scan oneth element and beyond
    for ( unsigned int i= 1; i<num; i++)
    {
        // load keys
        kType currentKey  = *(firstKey + i);
        kType previousKey = *(firstKey-1 + i);

        // load value
        oType currentValue = *(values + i); // convertible
        oType previousValue = *(result-1 + i);

        // within segment
        if (binary_pred(currentKey,previousKey))
        {
            oType r = binary_op( previousValue, currentValue);
            *(result + i) = r;
        }
        else // new segment
        {
            *(result + i) = (vType)init;
        }
    }

    return result;
}


namespace detail
{
/*!
*   \internal
*   \addtogroup detail
*   \ingroup scan
*   \{
*/
    enum  {scanByKey_kType, scanByKey_vType, scanByKey_oType, scanByKey_initType, scanByKey_BinaryPredicate, scanByKey_BinaryFunction};

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
    }

    const ::std::string operator() ( const ::std::vector<::std::string>& typeNames ) const
    {
        const std::string templateSpecializationString =
            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(0) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL0WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(0) + "(\n"
            "global " + typeNames[scanByKey_kType] + "* keys,\n"
            "global " + typeNames[scanByKey_vType] + "* vals,\n"
            "global " + typeNames[scanByKey_oType] + "* output,\n"
            ""        + typeNames[scanByKey_initType] + " init,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[scanByKey_kType] + "* ldsKeys,\n"
            "local "  + typeNames[scanByKey_oType] + "* ldsVals,\n"
            "global " + typeNames[scanByKey_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[scanByKey_BinaryFunction]  + "* binaryFunct,\n"
            "global " + typeNames[scanByKey_kType] + "* keyBuffer,\n"
            "global " + typeNames[scanByKey_oType] + "* valBuffer,\n"
            "int exclusive\n"
            ");\n\n"


            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(1) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL1WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(1) + "(\n"
            "global " + typeNames[scanByKey_kType] + "* keySumArray,\n"
            "global " + typeNames[scanByKey_oType] + "* preSumArray,\n"
            "global " + typeNames[scanByKey_oType] + "* postSumArray,\n"
            "const uint vecSize,\n"
            "local "  + typeNames[scanByKey_kType] + "* ldsKeys,\n"
            "local "  + typeNames[scanByKey_oType] + "* ldsVals,\n"
            "const uint workPerThread,\n"
            "global " + typeNames[scanByKey_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[scanByKey_BinaryFunction] + "* binaryFunct\n"
            ");\n\n"


            "// Dynamic specialization of generic template definition, using user supplied types\n"
            "template __attribute__((mangled_name(" + name(2) + "Instantiated)))\n"
            "__attribute__((reqd_work_group_size(KERNEL2WORKGROUPSIZE,1,1)))\n"
            "__kernel void " + name(2) + "(\n"
            "global " + typeNames[scanByKey_kType] + "* keySumArray,\n"
            "global " + typeNames[scanByKey_oType] + "* postSumArray,\n"
            "global " + typeNames[scanByKey_kType] + "* keys,\n"
            "global " + typeNames[scanByKey_oType] + "* output,\n"
            "const uint vecSize,\n"
            "global " + typeNames[scanByKey_BinaryPredicate] + "* binaryPred,\n"
            "global " + typeNames[scanByKey_BinaryFunction] + "* binaryFunct\n"
            ");\n\n";

        return templateSpecializationString;
    }
};


#ifdef ENABLE_TBB
      template <typename T, typename InputIterator1, typename InputIterator2, typename OutputIterator,
                 typename BinaryFunction, typename BinaryPredicate>
      struct ScanKey_tbb{
          typedef typename std::iterator_traits< OutputIterator >::value_type oType;
          oType sum;
          oType start;
          InputIterator1& first_key;
          InputIterator2& first_value;
          OutputIterator& result;
          const BinaryFunction binary_op;
          const BinaryPredicate binary_pred;
          const bool inclusive;
          bool flag, pre_flag;
          public:
          ScanKey_tbb() : sum(0) {}
          ScanKey_tbb( InputIterator1&  _first,
            InputIterator2& first_val,
            OutputIterator& _result,
            const BinaryFunction &_opr,
            const BinaryPredicate &_pred,
            const bool& _incl,
            const oType &init) : first_key(_first), first_value(first_val), result(_result), binary_op(_opr), binary_pred(_pred),
                             inclusive(_incl), start(init), flag(FALSE), pre_flag(TRUE){}
          oType get_sum() const {return sum;}
          template<typename Tag>
          void operator()( const tbb::blocked_range<int>& r, Tag ) {
              oType temp = sum;
              flag=FALSE;
              for( int i=r.begin(); i<r.end(); ++i ) {
                 if( Tag::is_final_scan() ) {
                     if(!inclusive){
                          if( i==0){
                             *(result + i) = start;
                             temp = binary_op(start, *(first_value+i));
                          }
                          else if(binary_pred(*(first_key+i), *(first_key +i- 1))){
                             *(result + i) = temp;
                             temp = binary_op(temp, *(first_value+i));
                          }
                          else{
                             *(result + i) = start;
                             temp = binary_op(start, *(first_value+i));
                             flag = TRUE;
                          }
                          continue;
                     }
                     else if(i == 0 ){
                        temp = *(first_value+i);
                     }
                     else if(binary_pred(*(first_key+i), *(first_key +i- 1))) {
                        temp = binary_op(temp, *(first_value+i));
                     }
                     else{
                        flag = TRUE;
                        temp = *(first_value+i);
                     }
                     *(result + i) = temp;
                 }
                 else if(pre_flag){
                   temp = *(first_value+i);
                   pre_flag = FALSE;

                 }
                 else if(binary_pred(*(first_key+i), *(first_key +i - 1)))
                     temp = binary_op(temp, *(first_value+i));
                 else if (!inclusive){
                     flag = TRUE;
                     temp = binary_op(start, *(first_value+i));
                 }
                 else {
                     flag = TRUE;
                     temp = *(first_value+i);
                 }
             }
             sum = temp;
          }
          ScanKey_tbb( ScanKey_tbb& b, tbb::split):first_key(b.first_key),result(b.result),first_value(b.first_value),
                                                   inclusive(b.inclusive),start(b.start),pre_flag(TRUE){}
          void reverse_join( ScanKey_tbb& a ) {
            if(!flag)
                sum = binary_op(a.sum,sum);
          }
          void assign( ScanKey_tbb& b ) {
             sum = b.sum;
          }
      };
#endif

/***********************************************************************************************************************
 * Detect Random Access
 **********************************************************************************************************************/

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
OutputIterator
scan_by_key_detect_random_access(
    control& ctl,
    const InputIterator1& firstKey,
    const InputIterator1& lastKey,
    const InputIterator2& firstValue,
    const OutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
    const std::string& user_code,
    const bool& inclusive,
    std::input_iterator_tag )
{
    //  TODO:  It should be possible to support non-random_access_iterator_tag iterators, if we copied the data
    //  to a temporary buffer.  Should we?
    static_assert( false, "Bolt only supports random access iterator types" );
};

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
OutputIterator
scan_by_key_detect_random_access(
    control& ctl,
    const InputIterator1& firstKey,
    const InputIterator1& lastKey,
    const InputIterator2& firstValue,
    const OutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
    const std::string& user_code,
    const bool& inclusive,
    std::random_access_iterator_tag )
{
    return detail::scan_by_key_pick_iterator( ctl, firstKey, lastKey, firstValue, result, init,
        binary_pred, binary_funct, user_code, inclusive );
}

/*!
* \brief This overload is called strictly for non-device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/
template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if<
             !(std::is_base_of<typename device_vector<typename
               std::iterator_traits<InputIterator1>::value_type>::iterator,InputIterator1>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<InputIterator2>::value_type>::iterator,InputIterator2>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<OutputIterator>::value_type>::iterator,OutputIterator>::value),
         OutputIterator >::type
scan_by_key_pick_iterator(
    control& ctl,
    const InputIterator1& firstKey,
    const InputIterator1& lastKey,
    const InputIterator2& firstValue,
    const OutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
    const std::string& user_code,
    const bool& inclusive )
{
    typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< OutputIterator >::value_type oType;
    static_assert( std::is_convertible< vType, oType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( firstKey, lastKey ) );
    if( numElements < 1 )
        return result;


    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::cl::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun( );
    }

  if( runMode == bolt::cl::control::SerialCpu )
    {

       if(inclusive){
          Serial_inclusive_scan_by_key<kType, vType, oType, BinaryPredicate, BinaryFunction>(&(*firstKey), &(*firstValue), &(*result), numElements, binary_pred, binary_funct);
       }
       else{
          Serial_exclusive_scan_by_key<kType, vType, oType, BinaryPredicate, BinaryFunction, T>(&(*firstKey), &(*firstValue), &(*result), numElements, binary_pred, binary_funct, init);
       }
       return result + numElements;
    }
  else if(runMode == bolt::cl::control::MultiCoreCpu)
  {
#ifdef ENABLE_TBB
        tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
        ScanKey_tbb<T, InputIterator1, InputIterator2, OutputIterator, BinaryFunction, BinaryPredicate> tbbkey_scan((InputIterator1 &)firstKey,
            (InputIterator2&) firstValue,(OutputIterator &)result, binary_funct, binary_pred, inclusive, init);
        tbb::parallel_scan( tbb::blocked_range<int>(  0, static_cast< int >( std::distance( firstKey, lastKey ))), tbbkey_scan, tbb::auto_partitioner());
        return result + numElements;
#else
        //std::cout << "The MultiCoreCpu version of Scan by key is not enabled." << std ::endl;
        throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of scan by key is not enabled to be built." );
#endif
  }
  else
  {

        // Map the input iterator to a device_vector
        device_vector< kType > dvKeys( firstKey, lastKey, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, ctl );
        device_vector< vType > dvValues( firstValue, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, true, ctl );
        device_vector< oType > dvOutput( result, numElements, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, false, ctl );

        //Now call the actual cl algorithm
        scan_by_key_enqueue( ctl, dvKeys.begin( ), dvKeys.end( ), dvValues.begin(), dvOutput.begin( ),
            init, binary_pred, binary_funct, user_code, inclusive );

        // This should immediately map/unmap the buffer
        dvOutput.data( );
    }

    return result + numElements;
}

/*!
* \brief This overload is called strictly for device_vector iterators
* \details This template function overload is used to seperate device_vector iterators from all other iterators
*/

    template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
typename std::enable_if<
             (std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVInputIterator1>::value_type>::iterator,DVInputIterator1>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVInputIterator2>::value_type>::iterator,DVInputIterator2>::value &&
               std::is_base_of<typename device_vector<typename
               std::iterator_traits<DVOutputIterator>::value_type>::iterator,DVOutputIterator>::value),
         DVOutputIterator >::type
scan_by_key_pick_iterator(
    control& ctl,
    const DVInputIterator1& firstKey,
    const DVInputIterator1& lastKey,
    const DVInputIterator2& firstValue,
    const DVOutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
    const std::string& user_code,
    const bool& inclusive )
{
    typedef typename std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< DVInputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    static_assert( std::is_convertible< vType, oType >::value, "InputValue and Output iterators are incompatible" );

    unsigned int numElements = static_cast< unsigned int >( std::distance( firstKey, lastKey ) );
    if( numElements < 1 )
        return result;

    bolt::cl::control::e_RunMode runMode = ctl.getForceRunMode( );

    if( runMode == bolt::cl::control::Automatic )
    {
        runMode = ctl.getDefaultPathToRun( );
    }

    if( runMode == bolt::cl::control::SerialCpu )
    {
        ::cl::Event serialCPUEvent;
        cl_int l_Error = CL_SUCCESS;

        kType *scanInputkey = (kType*)ctl.getCommandQueue().enqueueMapBuffer(firstKey.getBuffer(), false,
                            CL_MAP_READ, 0, sizeof(kType) * numElements, NULL, &serialCPUEvent, &l_Error );
        vType *scanInputBuffer = (vType*)ctl.getCommandQueue().enqueueMapBuffer(firstValue.getBuffer(), false,
                            CL_MAP_READ, 0, sizeof(vType) * numElements, NULL, &serialCPUEvent, &l_Error );
        oType *scanResultBuffer = (oType*)ctl.getCommandQueue().enqueueMapBuffer(result.getBuffer(), false,
                            CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(oType) * numElements, NULL, &serialCPUEvent, &l_Error );
        serialCPUEvent.wait();

        if(inclusive)
            Serial_inclusive_scan_by_key<kType, vType, oType, BinaryPredicate, BinaryFunction>(scanInputkey, scanInputBuffer, scanResultBuffer, numElements, binary_pred, binary_funct);
        else
            Serial_exclusive_scan_by_key<kType, vType, oType, BinaryPredicate, BinaryFunction, T>(scanInputkey, scanInputBuffer, scanResultBuffer, numElements, binary_pred, binary_funct, init);

        ctl.getCommandQueue().enqueueUnmapMemObject(firstKey.getBuffer(), scanInputkey);
        ctl.getCommandQueue().enqueueUnmapMemObject(firstValue.getBuffer(), scanInputBuffer);
        ctl.getCommandQueue().enqueueUnmapMemObject(result.getBuffer(), scanResultBuffer);

        return result + numElements;

    }
    else if( runMode == bolt::cl::control::MultiCoreCpu )
    {
#ifdef ENABLE_TBB
                ::cl::Event multiCoreCPUEvent;
                cl_int l_Error = CL_SUCCESS;
                /*Map the device buffer to CPU*/
                kType *scanInputkey = (kType*)ctl.getCommandQueue().enqueueMapBuffer(firstKey.getBuffer(), false,
                                   CL_MAP_READ, 0, sizeof(kType) * numElements, NULL, &multiCoreCPUEvent, &l_Error );
                vType *scanInputBuffer = (vType*)ctl.getCommandQueue().enqueueMapBuffer(firstValue.getBuffer(), false,
                                   CL_MAP_READ, 0, sizeof(vType) * numElements, NULL, &multiCoreCPUEvent, &l_Error );
                oType *scanResultBuffer = (oType*)ctl.getCommandQueue().enqueueMapBuffer(result.getBuffer(), false,
                                   CL_MAP_READ|CL_MAP_WRITE, 0, sizeof(oType) * numElements, NULL, &multiCoreCPUEvent, &l_Error );
                multiCoreCPUEvent.wait();

                tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
                ScanKey_tbb<T, kType*, vType*, oType*, BinaryFunction, BinaryPredicate> tbbkey_scan(scanInputkey, scanInputBuffer, scanResultBuffer, binary_funct, binary_pred, inclusive, init);
                tbb::parallel_scan( tbb::blocked_range<int>(  0, numElements), tbbkey_scan, tbb::auto_partitioner());

                ctl.getCommandQueue().enqueueUnmapMemObject(firstKey.getBuffer(), scanInputkey);
                ctl.getCommandQueue().enqueueUnmapMemObject(firstValue.getBuffer(), scanInputBuffer);
                ctl.getCommandQueue().enqueueUnmapMemObject(result.getBuffer(), scanResultBuffer);
                return result + numElements;
#else
                //std::cout << "The MultiCoreCpu version of scan by key is not enabled. " << std ::endl;
                throw ::cl::Error( CL_INVALID_OPERATION, "The MultiCoreCpu version of scan by key is not enabled to be built." );
#endif

     }
     else{

    //Now call the actual cl algorithm
              scan_by_key_enqueue( ctl, firstKey, lastKey, firstValue, result,
                       init, binary_pred, binary_funct, user_code, inclusive );
     }
    return result + numElements;
}


//  All calls to scan_by_key end up here, unless an exception was thrown
//  This is the function that sets up the kernels to compile (once only) and execute
template<
    typename DVInputIterator1,
    typename DVInputIterator2,
    typename DVOutputIterator,
    typename T,
    typename BinaryPredicate,
    typename BinaryFunction >
void
scan_by_key_enqueue(
    control& ctl,
    const DVInputIterator1& firstKey,
    const DVInputIterator1& lastKey,
    const DVInputIterator2& firstValue,
    const DVOutputIterator& result,
    const T& init,
    const BinaryPredicate& binary_pred,
    const BinaryFunction& binary_funct,
    const std::string& user_code,
    const bool& inclusive )
{
    cl_int l_Error;
#ifdef BOLT_ENABLE_PROFILING
aProfiler.setName("scan_by_key");
aProfiler.startTrial();
aProfiler.setStepName("Setup");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

size_t k0_stepNum, k1_stepNum, k2_stepNum;
#endif

    /**********************************************************************************
     * Type Names - used in KernelTemplateSpecializer
     *********************************************************************************/
    typedef typename std::iterator_traits< DVInputIterator1 >::value_type kType;
    typedef typename std::iterator_traits< DVInputIterator2 >::value_type vType;
    typedef typename std::iterator_traits< DVOutputIterator >::value_type oType;
    std::vector<std::string> typeNames(6);
    typeNames[scanByKey_kType] = TypeName< kType >::get( );
    typeNames[scanByKey_vType] = TypeName< vType >::get( );
    typeNames[scanByKey_oType] = TypeName< oType >::get( );
    typeNames[scanByKey_initType] = TypeName< T >::get( );
    typeNames[scanByKey_BinaryPredicate] = TypeName< BinaryPredicate >::get( );
    typeNames[scanByKey_BinaryFunction]  = TypeName< BinaryFunction >::get( );

    /**********************************************************************************
     * Type Definitions - directly concatenated into kernel string
     *********************************************************************************/
    std::vector<std::string> typeDefs; // typeDefs must be unique and order does matter
    PUSH_BACK_UNIQUE( typeDefs, ClCode< kType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< vType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< oType >::get() )
    PUSH_BACK_UNIQUE( typeDefs, ClCode< T >::get() )
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
    ScanByKey_KernelTemplateSpecializer ts_kts;
    std::vector< ::cl::Kernel > kernels = bolt::cl::getKernels(
        ctl,
        typeNames,
        &ts_kts,
        typeDefs,
        scan_by_key_kernels,
        compileOptions);
    // kernels returned in same order as added in KernelTemplaceSpecializer constructor

    // for profiling
    ::cl::Event kernel0Event, kernel1Event, kernel2Event, kernelAEvent;
    cl_uint doExclusiveScan = inclusive ? 0 : 1;
    // Set up shape of launch grid and buffers:
    int computeUnits     = ctl.getDevice( ).getInfo< CL_DEVICE_MAX_COMPUTE_UNITS >( );
    int wgPerComputeUnit =  ctl.getWGPerComputeUnit( );
    int resultCnt = computeUnits * wgPerComputeUnit;

    //  Ceiling function to bump the size of input to the next whole wavefront size
    cl_uint numElements = static_cast< cl_uint >( std::distance( firstKey, lastKey ) );
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
     ALIGNED( 256 ) BinaryFunction aligned_binary_funct( binary_funct );
    control::buffPointer binaryFunctionBuffer = ctl.acquireBuffer( sizeof( aligned_binary_funct ),
        CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY, &aligned_binary_funct );

    control::buffPointer keySumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( kType ) );
    control::buffPointer preSumArray  = ctl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    control::buffPointer postSumArray = ctl.acquireBuffer( sizeScanBuff*sizeof( oType ) );
    cl_uint ldsKeySize, ldsValueSize;


    /**********************************************************************************
     *  Kernel 0
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 0");
aProfiler.set(AsyncProfiler::getDevice, control::SerialCpu);
#endif
    try
    {
    ldsKeySize   = static_cast< cl_uint >( kernel0_WgSize * sizeof( kType ) );
    ldsValueSize = static_cast< cl_uint >( kernel0_WgSize * sizeof( oType ) );
    V_OPENCL( kernels[0].setArg( 0, firstKey.getBuffer()), "Error setArg kernels[ 0 ]" ); // Input keys
    V_OPENCL( kernels[0].setArg( 1, firstValue.getBuffer()),"Error setArg kernels[ 0 ]" ); // Input buffer
    V_OPENCL( kernels[0].setArg( 2, result.getBuffer( ) ), "Error setArg kernels[ 0 ]" ); // Output buffer
    V_OPENCL( kernels[0].setArg( 3, init ),                 "Error setArg kernels[ 0 ]" ); // Initial value exclusive
    V_OPENCL( kernels[0].setArg( 4, numElements ),          "Error setArg kernels[ 0 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[0].setArg( 5, ldsKeySize, NULL ),     "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[0].setArg( 6, ldsValueSize, NULL ),   "Error setArg kernels[ 0 ]" ); // Scratch buffer
    V_OPENCL( kernels[0].setArg( 7, *binaryPredicateBuffer),"Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 8, *binaryFunctionBuffer ),"Error setArg kernels[ 0 ]" ); // User provided functor
    V_OPENCL( kernels[0].setArg( 9, *keySumArray ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[0].setArg(10, *preSumArray ),         "Error setArg kernels[ 0 ]" ); // Output per block sum
    V_OPENCL( kernels[0].setArg(11, doExclusiveScan ),      "Error setArg kernels[ 0 ]" ); // Exclusive scan?

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k0_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 0");
aProfiler.set(AsyncProfiler::getDevice, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*numElements);
aProfiler.set(AsyncProfiler::memory, 2*numElements*(sizeof(vType)+sizeof(kType)) + 1*sizeScanBuff*(sizeof(vType)+sizeof(kType)) );
#endif

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
        std::cerr << "::cl::enqueueNDRangeKernel( 0 ) in bolt::cl::scan_by_key_enqueue()" << std::endl;
        std::cerr << "Error Code:   " << clErrorStringA(e.err()) << " (" << e.err() << ")" << std::endl;
        std::cerr << "File:         " << __FILE__ << ", line " << __LINE__ << std::endl;
        std::cerr << "Error String: " << e.what() << std::endl;
    }

    /**********************************************************************************
     *  Kernel 1
     *********************************************************************************/
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 1");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    cl_uint workPerThread = static_cast< cl_uint >( sizeScanBuff / kernel1_WgSize );
    V_OPENCL( kernels[1].setArg( 0, *keySumArray ),         "Error setArg kernels[ 1 ]" ); // Input keys
    V_OPENCL( kernels[1].setArg( 1, *preSumArray ),         "Error setArg kernels[ 1 ]" ); // Input buffer
    V_OPENCL( kernels[1].setArg( 2, *postSumArray ),        "Error setArg kernels[ 1 ]" ); // Output buffer
    V_OPENCL( kernels[1].setArg( 3, numWorkGroupsK0 ),      "Error setArg kernels[ 1 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[1].setArg( 4, ldsKeySize, NULL ),     "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 5, ldsValueSize, NULL ),   "Error setArg kernels[ 1 ]" ); // Scratch buffer
    V_OPENCL( kernels[1].setArg( 6, workPerThread ),        "Error setArg kernels[ 1 ]" ); // User provided functor
    V_OPENCL( kernels[1].setArg( 7, *binaryPredicateBuffer ),"Error setArg kernels[ 1 ]" ); // User provided functor
    V_OPENCL( kernels[1].setArg( 8, *binaryFunctionBuffer ),"Error setArg kernels[ 1 ]" ); // User provided functor

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k1_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 1");
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, 2*sizeScanBuff);
aProfiler.set(AsyncProfiler::memory, 4*sizeScanBuff*(sizeof(kType)+sizeof(vType)));
#endif

    try
    {
    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
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
#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Setup Kernel 2");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);
#endif

    V_OPENCL( kernels[2].setArg( 0, *keySumArray ),         "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 1, *postSumArray ),        "Error setArg kernels[ 2 ]" ); // Input buffer
    V_OPENCL( kernels[2].setArg( 2, firstKey.getBuffer()), "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 3, result.getBuffer()),   "Error setArg kernels[ 2 ]" ); // Output buffer
    V_OPENCL( kernels[2].setArg( 4, numElements ),          "Error setArg kernels[ 2 ]" ); // Size of scratch buffer
    V_OPENCL( kernels[2].setArg( 5, *binaryPredicateBuffer ),"Error setArg kernels[ 2 ]" ); // User provided functor
    V_OPENCL( kernels[2].setArg( 6, *binaryFunctionBuffer ),"Error setArg kernels[ 2 ]" ); // User provided functor

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
k2_stepNum = aProfiler.getStepNum();
aProfiler.setStepName("Kernel 2");
aProfiler.set(AsyncProfiler::device, ctl.forceRunMode());
aProfiler.set(AsyncProfiler::flops, numElements);
aProfiler.set(
    AsyncProfiler::memory,
    2*numElements*sizeof(vType)+numElements*sizeof(kType)+1*sizeScanBuff*(sizeof(kType)+sizeof(vType)));
#endif

    try
    {
    l_Error = ctl.getCommandQueue( ).enqueueNDRangeKernel(
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

#ifdef BOLT_ENABLE_PROFILING
aProfiler.nextStep();
aProfiler.setStepName("Querying Kernel Times");
aProfiler.set(AsyncProfiler::device, control::SerialCpu);

aProfiler.setDataSize(numElements*sizeof(oType));
std::string strDeviceName = ctl.getDevice().getInfo< CL_DEVICE_NAME >( &l_Error );
bolt::cl::V_OPENCL( l_Error, "Device::getInfo< CL_DEVICE_NAME > failed" );
aProfiler.setArchitecture(strDeviceName);

    try
    {
        cl_ulong k0_start, k0_stop, k1_stop, k2_stop;

        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k0_start);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>()");
        l_Error = kernel0Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k0_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");

        //l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k1_start);
        //V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
        l_Error = kernel1Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &k1_stop);
        V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_END>()");

        //l_Error = kernel2Event.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &k2_start);
        //V_OPENCL( l_Error, "failed on getProfilingInfo<CL_PROFILING_COMMAND_START>()");
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

#endif

}   //end of scan_by_key_enqueue( )

    /*!   \}  */
} //namespace detail
} //namespace cl
} //namespace bolt

#endif
