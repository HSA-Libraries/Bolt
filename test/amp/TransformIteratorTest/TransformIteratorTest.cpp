/***************************************************************************
*   © 2012,2014 Advanced Micro Devices, Inc. All rights reserved.
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
#define BCKND amp

#include "common/stdafx.h"
#include "bolt/amp/transform.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>

#include "bolt/amp/generate.h"

#include "bolt/amp/copy.h"
#include "bolt/amp/count.h"
#include "bolt/amp/fill.h"
#include "bolt/amp/functional.h"
#include "bolt/amp/gather.h"
#include "bolt/amp/inner_product.h"
#include "bolt/amp/transform.h"
#include "bolt/amp/reduce.h"
#include "bolt/amp/reduce_by_key.h"
#include "bolt/amp/scan.h"
#include "bolt/amp/scan_by_key.h"
#include "bolt/amp/scatter.h"
#include "bolt/amp/transform_reduce.h"
#include "bolt/amp/transform_scan.h"
#include "bolt/amp/for_each.h"


#include "common/test_common.h"
#include <bolt/amp/iterator/constant_iterator.h>
#include <bolt/amp/iterator/counting_iterator.h>
#include <bolt/amp/iterator/permutation_iterator.h>
#include <bolt/amp/iterator/transform_iterator.h>

#include <boost/shared_array.hpp>
#include <amp_math.h>

#include <boost/program_options.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/algorithm/cxx11/iota.hpp>
namespace po = boost::program_options;

#define WAVEFRNT_SIZE 256
static const unsigned int size = WAVEFRNT_SIZE;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Gold helper algorithms
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace gold
{
        template<
        typename InputIterator1,
        typename InputIterator2,
        typename OutputIterator,
        typename BinaryFunction>
    OutputIterator
    scan_by_key(
        InputIterator1 firstKey,
        InputIterator1 lastKey,
        InputIterator2 values,
        OutputIterator result,
        BinaryFunction binary_op)
    {
        if(std::distance(firstKey,lastKey) < 1)
             return result;
        typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
        typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
        typedef typename std::iterator_traits< OutputIterator >::value_type oType;

        static_assert( std::is_convertible< vType, oType >::value,
            "InputIterator2 and OutputIterator's value types are not convertible." );

        if(std::distance(firstKey,lastKey) < 1)
             return result;
        // do zeroeth element
        *result = *values; // assign value

        // scan oneth element and beyond
        for ( InputIterator1 key = (firstKey+1); key != lastKey; key++)
        {
            // move on to next element
            values++;
            result++;

            // load keys
            kType currentKey  = *(key);
            kType previousKey = *(key-1);

            // load value
            oType currentValue = *values; // convertible
            oType previousValue = *(result-1);

            // within segment
            if (currentKey == previousKey)
            {
                //std::cout << "continuing segment" << std::endl;
                oType r = binary_op( previousValue, currentValue);
                *result = r;
            }
            else // new segment
            {
                //std::cout << "new segment" << std::endl;
                *result = currentValue;
            }
        }

        return result;
    }

    template<
        typename InputIterator1,
        typename InputIterator2,
        typename OutputIterator1,
        typename OutputIterator2,
        typename BinaryPredicate,
        typename BinaryFunction>
    //std::pair<OutputIterator1, OutputIterator2>
    unsigned int
    reduce_by_key( InputIterator1 keys_first,
                   InputIterator1 keys_last,
                   InputIterator2 values_first,
                   OutputIterator1 keys_output,
                   OutputIterator2 values_output,
                   const BinaryPredicate binary_pred,
                   const BinaryFunction binary_op )
    {
        typedef typename std::iterator_traits< InputIterator1 >::value_type kType;
        typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
        typedef typename std::iterator_traits< OutputIterator1 >::value_type koType;
        typedef typename std::iterator_traits< OutputIterator2 >::value_type voType;
        static_assert( std::is_convertible< vType, voType >::value,
                       "InputIterator2 and OutputIterator's value types are not convertible." );

       int numElements = static_cast< int >( std::distance( keys_first, keys_last ) );

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

        //return std::pair(keys_output+1, values_output+1);
        return count;
    }

};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Boost tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct square_root
{
    float operator( ) (float x) const restrict ( cpu, amp )
    {
      return concurrency::fast_math::sqrt(x);
    }
    typedef float result_type;
};


struct my_negate
{
    int operator( ) (int x) const restrict (cpu,amp)
    {
      return -x;
    }
    typedef int result_type;
};


 struct gen_input
 {

    int operator() () restrict (cpu,amp) 
    { 
        return 10; 
    }

    typedef int result_type;
 };


 struct add_3
 {
    int operator() (const int x)  const restrict (cpu,amp)
    { 
        return x + 3; 
    }
    typedef int result_type;
 };

struct add_4
{
    int operator() (const int x)  const restrict (cpu,amp)
    { 
        return x + 4; 
    }
    typedef int result_type;
};

struct square
{
    int operator() (const int x)  const restrict (cpu,amp)
    { 
        return x + 2; 
    }
    typedef int result_type;
};


struct add_0
{
    int operator() (const int x)  const restrict (cpu,amp)
    { 
        return x;
    }
    typedef int result_type;
};

template<
    typename oType,
    typename BinaryFunction,
    typename T>
oType*
Serial_scan(
    oType *values,
    oType *result,
    unsigned int  num,
    const BinaryFunction binary_op,
    const bool Incl,
    const T &init)
{
    oType  sum, temp;
    if(Incl){
      *result = *values; // assign value
      sum = *values;
    }
    else {
        temp = *values;
       *result = (oType)init;
       sum = binary_op( *result, temp);
    }
    for ( unsigned int i= 1; i<num; i++)
    {
        oType currentValue = *(values + i); // convertible
        if (Incl)
        {
            oType r = binary_op( sum, currentValue);
            *(result + i) = r;
            sum = r;
        }
        else // new segment
        {
            *(result + i) = sum;
            sum = binary_op( sum, currentValue);

        }
    }
    return result;
}


template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator>

void Serial_scatter (InputIterator1 first1,
                     InputIterator1 last1,
                     InputIterator2 map,
                     OutputIterator result)
{
       size_t numElements = static_cast<  size_t >( std::distance( first1, last1 ) );

       for (int iter = 0; iter<(int)numElements; iter++)
                *(result+*(map + iter)) = *(first1 + iter);
}

template<typename InputIterator1,
         typename InputIterator2,
         typename InputIterator3,
         typename OutputIterator,
         typename Predicate>

void Serial_scatter_if (InputIterator1 first1,
                     InputIterator1 last1,
                     InputIterator2 map,
                     InputIterator3 stencil,
                     OutputIterator result,
                     Predicate pred)
{
       size_t numElements = static_cast< size_t >( std::distance( first1, last1 ) );
       for (int iter = 0; iter< (int)numElements; iter++)
       {
             if(pred(stencil[iter]) != 0)
                  result[*(map+(iter))] = first1[iter];
       }
}

template<typename InputIterator1,
         typename InputIterator2,
         typename OutputIterator>

void Serial_gather (InputIterator1 map_first,
                     InputIterator1 map_last,
                     InputIterator2 input,
                     OutputIterator result)
{
       int numElements = static_cast< int >( std::distance( map_first, map_last ) );
       typedef typename  std::iterator_traits<InputIterator1>::value_type iType1;
       iType1 temp;
       for(int iter = 0; iter < numElements; iter++)
       {
              temp = *(map_first + (int)iter);
              *(result + (int)iter) = *(input + (int)temp);
       }
}

template<typename InputIterator1,
         typename InputIterator2,
         typename InputIterator3,
         typename OutputIterator,
         typename Predicate>

void Serial_gather_if (InputIterator1 map_first,
                     InputIterator1 map_last,
                     InputIterator2 stencil,
                     InputIterator3 input,
                     OutputIterator result,
                     Predicate pred)
{
    unsigned int numElements = static_cast< unsigned int >( std::distance( map_first, map_last ) );
    for(unsigned int iter = 0; iter < numElements; iter++)
    {
        if(pred(*(stencil + (int)iter)))
             result[(int)iter] = input[map_first[(int)iter]];
    }
}


template<
    typename kType,
    typename vType,
    typename koType,
    typename voType,
    typename BinaryFunction>
unsigned int
Serial_reduce_by_key(
    kType* keys,
    vType* values,
    koType* keys_output,
    voType* values_output,
    BinaryFunction binary_op,
    unsigned int  numElements
    )
{
 
    static_assert( std::is_convertible< vType, voType >::value,
                   "InputIterator2 and OutputIterator's value types are not convertible." );

    // do zeroeth element
    values_output[0] = values[0];
    keys_output[0] = keys[0];
    unsigned int count = 1;
    // rbk oneth element and beyond

    unsigned int vi=1, vo=0, ko=0;
    for ( unsigned int i= 1; i<numElements; i++)
    {
        // load keys
        kType currentKey  = keys[i];
        kType previousKey = keys[i-1];

        // load value
        voType currentValue = values[vi];
        voType previousValue = values_output[vo];

        previousValue = values_output[vo];
        // within segment
        if (currentKey == previousKey)
        {
            voType r = binary_op( previousValue, currentValue);
            values_output[vo] = r;
            keys_output[ko] = currentKey;

        }
        else // new segment
        {
            vo++;
            ko++;
            values_output[vo] = currentValue;
            keys_output[ko] = currentKey;
            count++; //To count the number of elements in the output array
        }
        vi++;
    }

    return count;

}

template<
    typename kType,
    typename vType,
    typename oType,
    typename BinaryFunction>
oType*
Serial_inclusive_scan_by_key(
    kType *firstKey,
    unsigned int  num,
    vType* values,
    oType* result,
    BinaryFunction binary_op)
{
    if(num < 1)
         return result;
  
    result[0] = values[0]; // assign value
 
         unsigned int v=0, r=0;
         // scan oneth element and beyond
    for ( unsigned int key= 1; key<num; key++)
    {
        // move on to next element
                 v++;
                 r++;
 
                  // load keys
        kType currentKey  = firstKey[key];
        kType previousKey = firstKey[key-1];
 
                 // load value
        oType currentValue = values[v]; // convertible
        oType previousValue = result[r-1];
 
                  // within segment
        if (currentKey == previousKey)
        {
            //std::cout << "continuing segment" << std::endl;
            oType res = binary_op( previousValue, currentValue);
            result[r] = res;
        }
        else // new segment
        {
            result[r] = currentValue;
        }
 
    }
 
    return result;
}


TEST(TransformIterator, boost)
{

    typedef float etype;
    typedef std::vector<etype> fev;

    etype elements[10] = {0,1,2,3,4,5,6,7,8,9};
    fev e( elements, elements + 10 );
    fev r( 10 );
    size_t esize = sizeof(elements)/sizeof(etype);

    std::transform( boost::make_transform_iterator( e.begin( ), square_root( ) ),
                    boost::make_transform_iterator( e.end( ),   square_root( ) ),
                    r.begin( ),
                    std::negate<float>( )
                  );

    float check[10] = { -0.000000000f,
                        -1.00000000f,
                        -1.41421354f,
                        -1.73205078f,
                        -2.00000000f,
                        -2.23606801f,
                        -2.44948983f,
                        -2.64575124f,
                        -2.82842708f,
                        -3.00000000f };
    fev c( check, check + 10 );

    cmpArrays(c, r);
}


template<typename T>
struct multby2
{
    T operator( ) (T x) const 
    {
      return x*T(2);
    }
    typedef T result_type;
};

template<typename T>
struct add4
{
    T operator( ) (T x) const restrict ( cpu, amp )
    {
      return x+T(4);
    }
    typedef T result_type;
};

//Boost test
TEST( TransformIterator, BoostBolt )
{
    int x[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const int N = sizeof(x)/sizeof(int);

    bolt::amp::device_vector<int> dx(x, x+N);
    bolt::amp::device_vector<int> dout(x, x+N, true);
    typedef  multby2<int> Function;
    typedef bolt::amp::transform_iterator<Function, bolt::amp::device_vector<int>::iterator> doubling_iterator;

    doubling_iterator i( dx.begin( ), multby2< int >( ) ),
      i_end( dx.end( ), multby2< int >( ) );

    std::cout << "multiplying the array by 2:" << std::endl;
    while (i != i_end)
      std::cout << *i++ << " ";
    std::cout << std::endl;

    std::cout << "adding 4 to each element in the array:" << std::endl;
    bolt::amp::copy( bolt::amp::make_transform_iterator( dx.begin( ), add4< int >( ) ),
                     bolt::amp::make_transform_iterator( dx.end( ), add4< int >( ) ),
                     dout.begin( ) );
    bolt::amp::device_vector< int >::iterator di = dout.begin( );
    while( di != dout.end( ) )
      std::cout << *di++ << " ";
    std::cout<<std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct UDD
{
    int a; 
    int b;

    operator UDD( ) { return UDD( ); }

    UDD operator( ) (const UDD& lhs, const UDD& rhs) const restrict(amp,cpu){
        return (rhs);
    } 

	bool operator != (const UDD& other) const restrict(amp,cpu)
    {
        return ((a != other.a) || (b != other.b));
    }

    bool operator < (const UDD& other) const restrict(amp,cpu){
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const restrict(amp,cpu){
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const restrict(amp,cpu) {
        return ((a+b) == (other.a+other.b));
    }

    UDD operator + (const UDD &rhs) const restrict(amp,cpu)
    {
      UDD _result;
      _result.a = a + rhs.a;
      _result.b = b + rhs.b;
      return _result;
    }

    UDD( ) restrict(amp,cpu)
        : a(0),b(0) { }
    UDD(int _in) restrict(amp,cpu)
        : a(_in), b(_in +1)  { }
    typedef UDD result_type;
};

struct UDD3
{
    float a; 
    float b;

    operator UDD3( ) { return UDD3( ); }

    UDD3 operator( ) (const UDD3& lhs, const UDD3& rhs) const restrict(amp,cpu){
        return (rhs);
    } 

	bool operator != (const UDD3& other) const restrict(amp,cpu)
    {
        return ((a != other.a) || (b != other.b));
    }

    bool operator < (const UDD3& other) const restrict(amp,cpu){
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD3& other) const restrict(amp,cpu){
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD3& other) const restrict(amp,cpu) {
        return ((a+b) == (other.a+other.b));
    }

    UDD3 operator + (const UDD3 &rhs) const restrict(amp,cpu)
    {
      UDD3 _result;
      _result.a = a + rhs.a;
      _result.b = b + rhs.b;
      return _result;
    }

    UDD3( ) restrict(amp,cpu)
        : a(0.0f),b(0.0f) { }
    UDD3(float _in) restrict(amp,cpu)
        : a(_in), b(_in +1.0f)  { }
    typedef UDD3 result_type;
};


struct UDD2
{
    int i;
    float f;
  
	 bool operator != (const UDD2& other) const restrict(cpu, amp)
    {
        return ((i != other.i) || (f != other.f));
    }

    bool operator == (const UDD2& other) const  restrict(cpu, amp) {
        return ((i == other.i) && (f == other.f));
    }

	UDD2 operator ++ () restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = i + 1;
      _result.f = f + 1.0f;
      return _result;
    }

	UDD2 operator =  (const int rhs) restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = i + rhs;
      _result.f = f + (float)rhs;
      return _result;
    }

	UDD2 operator + (const UDD2 &rhs) const restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = this->i + rhs.i;
      _result.f = this->f + rhs.f;
      return _result;
    }

	UDD2 operator * (const UDD2 &rhs) const restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = this->i * rhs.i;
      _result.f = this->f * rhs.f;
      return _result;
    }

	UDD2 operator + (const int rhs) restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = i = i + rhs;
      _result.f = f = f + (float)rhs;
      return _result;
    }

	UDD2 operator-() const restrict(cpu, amp)
    {
        UDD2 r;
        r.i = -i;
        r.f = -f;
        return r;
    }

    UDD2() restrict(cpu, amp)
        : i(0), f(0) { }
    UDD2(int _in) restrict(cpu, amp)
        : i(_in), f((float)(_in+2) ){ }
};


struct squareUDD_resultUDD_intonly
    {
        UDD operator() (const UDD& x)  const restrict ( cpu, amp )
        { 
            UDD tmp;
            tmp.a = x.a * x.a;
            tmp.b = x.b * x.b;
            return tmp;
        }
        typedef UDD result_type;
};

struct squareUDD_resultUDD_floatonly
    {
        UDD3 operator() (const UDD3& x)  const restrict ( cpu, amp )
        { 
            UDD3 tmp;
            tmp.a = x.a * x.a;
            tmp.b = x.b * x.b;
            return tmp;
        }
        typedef UDD3 result_type;
};

struct add3UDD_resultUDD_intonly
{
        UDD operator() (const UDD& x)  const restrict ( cpu, amp )
        { 
            UDD tmp;
            tmp.a = x.a + 3;
            tmp.b = x.b + 3;
            return tmp;
        }
        typedef UDD result_type;
};


struct add4UDD_resultUDD_intonly
{
        UDD operator() (const UDD& x)  const restrict ( cpu, amp )
        { 
            UDD tmp;
            tmp.a = x.a + 4;
            tmp.b = x.b + 4;
            return tmp;
        }
        typedef UDD result_type;
};

struct UDDnegate2
{
   UDD operator( ) (const UDD &lhs) const restrict ( cpu, amp )
   {
     UDD _result;
     _result.a = -lhs.a;
     _result.b = -lhs.b;
     return _result;
   }
   typedef UDD result_type;
};

struct UDDadd_3
 {
        UDD2 operator() (const UDD2 &x) const restrict ( cpu, amp )
        { 
            UDD2 temp;
            temp.i = x.i + 3;
            temp.f = x.f + 3.0f;
            return temp; 
        }
        typedef UDD2 result_type;
};

struct squareUDD_result_float
{
        float operator() (const UDD2& x)  const restrict ( cpu, amp )
        { 
            return ((float)x.i + x.f);
        }
        typedef float result_type;
};

struct squareUDD_result_int
{
        int operator() (const UDD2& x)  const restrict ( cpu, amp )
        { 
            return (x.i + (int) x.f);
        }
        typedef int result_type;
};

struct squareUDD_resultUDD
    {
        UDD2 operator() (const UDD2& x)  const restrict ( cpu, amp )
        { 
            UDD2 tmp;
            tmp.i = x.i * x.i;
            tmp.f = x.f * x.f;
            return tmp;
        }
        typedef UDD2 result_type;
};

struct add3UDD_resultUDD
{
        UDD2 operator() (const UDD2& x)  const restrict ( cpu, amp )
        { 
            UDD2 tmp;
            tmp.i = x.i + 3;
            tmp.f = x.f + 3.f;
            return tmp;
        }
        typedef UDD2 result_type;
};

struct add4UDD_resultUDD
{
        UDD2 operator() (const UDD2 & x)  const restrict ( cpu, amp )
        { 
            UDD2 tmp;
            tmp.i = x.i + 4;
            tmp.f = x.f + 4.f;
            return tmp;
        }
        typedef UDD2 result_type;
};

struct cubeUDD
{
        float operator() (const UDD2 & x)  const restrict ( cpu, amp )
        { 
            return ((float)x.i + x.f + 3.0f);
        }
        typedef float result_type;
};

struct cubeUDD_result_int
{
        int operator() (const UDD2 & x)  const restrict ( cpu, amp )
        { 
            return (x.i + (int)x.f + 3);
        }
        typedef int result_type;
};

struct cubeUDD_resultUDD
{
        UDD2 operator() (const UDD2 & x)  const restrict ( cpu, amp )
        { 
            UDD2 tmp;
            tmp.i = x.i * x.i * x.i;
            tmp.f = x.f * x.f * x.f;
            return tmp;
        }
        typedef UDD2 result_type;
};

struct UDDplus
{
   UDD2 operator() (const UDD2 &lhs, const UDD2 &rhs) const restrict ( cpu, amp )
   {
     UDD2 _result;
     _result.i = lhs.i + rhs.i;
     _result.f = lhs.f + rhs.f;
     return _result;
   }

};

struct UDDplus_intonly
{
   UDD operator() (const UDD &lhs, const UDD &rhs) const restrict ( cpu, amp )
   {
     UDD _result;
     _result.a = lhs.a + rhs.a;
     _result.b = lhs.b + rhs.b;
     return _result;
   }

};

struct UDDnegate
{
   UDD2 operator() (const UDD2 &lhs) const restrict ( cpu, amp )
   {
     UDD2 _result;
     _result.i = -lhs.i;
     _result.f = -lhs.f;
     return _result;
   }

};

struct UDDmul
{
   UDD2 operator() (const UDD2 &lhs, const UDD2 &rhs) const restrict ( cpu, amp )
   {

     return lhs*rhs;
   }

};

struct UDDminus
{
   UDD2 operator() (const UDD2 &lhs, const UDD2 &rhs) const restrict ( cpu, amp )
   {
     UDD2 _result;
     _result.i = lhs.i - rhs.i;
     _result.f = lhs.f - rhs.f;
     return _result;
   }

};

struct gen_input_udd_intonly
{
       UDD operator() ()  const restrict ( cpu, amp )
       { 
            int i= 8;
            UDD temp;
            temp.a = i;
            temp.b = i*2;
            return temp; 
        }
        typedef UDD result_type;
};

struct gen_input_udd_floatonly
{
       UDD3 operator() ()  const restrict ( cpu, amp )
       { 
            float i= 2.0f;
            UDD3 temp;
            temp.a = i;
            temp.b = i*2.0f;
            return temp; 
        }
        typedef UDD3 result_type;
};

struct gen_input_udd
{
       UDD2 operator() ()  const restrict ( cpu, amp )
       { 
            int i= 8;
            UDD2 temp;
            temp.i = i;
            temp.f = (float)i;
            return temp; 
        }
        typedef UDD2 result_type;
};

struct gen_input_udd2
{
        UDD2 operator() ()  const  restrict ( cpu, amp )
       { 
            int i=7;
            UDD2 temp;
            temp.i = i*2;
            temp.f = (float)i*2;
            return temp; 
        }
        typedef UDD2 result_type;
};


TEST(TransformIterator, Transform)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dumpCheckV(empty, empty + WAVEFRNT_SIZE);

    std::vector<int> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<int> check(empty, empty + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator( ).default_view;

    typedef bolt::amp::transform_iterator<my_negate, bolt::amp::device_vector<int>::iterator> transf_iter;

    transf_iter tbegin(dvebegin, my_negate( ));
    transf_iter tend(dveend, my_negate( ));

    std::transform( boost::make_transform_iterator(el.begin( ), my_negate( )), 
                    boost::make_transform_iterator(el.end( ), my_negate( )),
                    check.begin( ),
                    std::identity<int>( )
                    );

    bolt::amp::transform( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                          bolt::amp::make_transform_iterator(dveend, my_negate( )),
                          dumpV.begin( ),
                          bolt::amp::identity<int>( )
                          );

    cmpArrays(check,dumpV);

}

TEST(TransformIterator, TransformUDD)
{

    typedef UDD etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    bolt::amp::device_vector<UDD, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);
    bolt::amp::device_vector<UDD, concurrency::array_view> dumpV(empty, empty + WAVEFRNT_SIZE);

    std::vector<UDD> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<UDD> check(empty, empty + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::transform( boost::make_transform_iterator(el.begin( ), UDDnegate2( ) ), 
                    boost::make_transform_iterator(el.end( ), UDDnegate2( ) ),
                    check.begin( ),
                    std::identity<UDD>( )
                    );

    bolt::amp::transform( bolt::amp::make_transform_iterator(dvebegin, UDDnegate2( )), 
                          bolt::amp::make_transform_iterator(dveend, UDDnegate2( )),
                          dumpV.begin( ),
                          bolt::amp::identity<UDD>( )
                          );

    cmpArrays(check,dumpV);

}



TEST( TransformIterator, UnaryTransformRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
        /*Generate inputs*/
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;

        typedef bolt::amp::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::amp::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;   

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;


        {/*Test case when inputs are trf Iterators*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), add3);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), add3);
            /*Compute expected results*/
			std::vector<int> bolt_out(dvOutVec.begin(), dvOutVec.end());

            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), add3);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is randomAccessIterator */
            bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), add_3());
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), add_3());
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), add_3());
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform(const_itr_begin, const_itr_end, svOutVec.begin(), add_3());
            bolt::amp::transform(const_itr_begin, const_itr_end, dvOutVec.begin(), add_3());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), add_3());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is a counting iterator */
            //bolt::amp::transform(count_itr_begin, count_itr_end, svOutVec.begin(), add_3());
            bolt::amp::transform(count_itr_begin, count_itr_end, dvOutVec.begin(), add_3());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::transform(count_vector.begin(), count_vector.end(), stlOut.begin(), add_3());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}

TEST( TransformIterator, UnaryTransformUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        //gen_input_udd genUDD;
        //bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
		for(long int i=0;i<length; i++)
		{
			svIn1Vec[i].i = i;
			svIn1Vec[i].f = (float) i;
		}

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        std::vector< float > stlOut_float( length );
        std::vector< float > svOutVec_float( length );
        bolt::BCKND::device_vector< float > dvOutVec_float( length );

        squareUDD_resultUDD sqUDD;

        squareUDD_result_float sqUDD_float;

        squareUDD_result_int sq_int;

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
     
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=0, init.f=0.0f;

        //Compilation Error
        counting_itr count_itr_begin(init);

        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is trf Iterator and UDD is returning int*/
            //typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;
            std::vector< int >                  tsvOutVec( length );
            std::vector< int >                  tstlOut( length );
            bolt::BCKND::device_vector< int >   tdvOutVec( length );

            //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
            tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

            //bolt::amp::transform(tsv_trf_begin1, tsv_trf_end1, tsvOutVec.begin(), bolt::amp::negate<int>());
            bolt::amp::transform(tdv_trf_begin1, tdv_trf_end1, tdvOutVec.begin(), bolt::amp::negate<int>());
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, tstlOut.begin(), bolt::amp::negate<int>());
            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }

        {/*Test case when input is trf Iterator*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), sqUDD);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), sqUDD);

            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), sqUDD);

            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when input is trf Iterator and Output is float vector*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svOutVec_float.begin(), sqUDD_float);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvOutVec_float.begin(), sqUDD_float);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut_float.begin(), sqUDD_float);
            /*Check the results*/
            //cmpArrays(svOutVec_float, stlOut_float);
            cmpArrays(dvOutVec_float, stlOut_float);
        }

        {/*Test case when the input is randomAccessIterator */
            bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), sqUDD);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), sqUDD);

            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), sqUDD);

            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform(const_itr_begin, const_itr_end, svOutVec.begin(), sqUDD);
            bolt::amp::transform(const_itr_begin, const_itr_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }	
        {/*Test case when the input is a counting iterator */
            //bolt::amp::transform(count_itr_begin, count_itr_end, svOutVec.begin(), sqUDD);
            bolt::amp::transform(count_itr_begin, count_itr_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::transform(count_itr_begin, count_itr_end, stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }

}


TEST( TransformIterator, UnaryTransformUDD_intRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD > svIn1Vec( length );
        std::vector< UDD > svOutVec( length );
        std::vector< UDD > stlOut( length );

        /*Generate inputs*/
        gen_input_udd_intonly genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
		/*for(long int i=0;i<length; i++)
		{
			svIn1Vec[i].a = i;
			svIn1Vec[i].b = i * 2;
		}*/

        bolt::BCKND::device_vector< UDD > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD > dvOutVec( length );

        std::vector< float > stlOut_float( length );
        std::vector< float > svOutVec_float( length );
        bolt::BCKND::device_vector< float > dvOutVec_float( length );

        squareUDD_resultUDD_intonly sqUDD;

        typedef std::vector< UDD>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD_intonly, std::vector< UDD >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD_intonly, bolt::BCKND::device_vector< UDD >::iterator>   dv_trf_itr_add3;
     
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        UDD temp;
        temp.a=1, temp.b=2;

        UDD init;
        init.a=0, init.b=0;

        //Compilation Error
        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

       

        {/*Test case when input is trf Iterator*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), sqUDD);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), sqUDD);

            std::vector<UDD> temp1_vec(dv_trf_begin1, dv_trf_end1);
            std::vector<UDD> temp2_vec(dvOutVec.begin(), dvOutVec.end());

            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the input is randomAccessIterator */
            bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), sqUDD);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), sqUDD);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform(const_itr_begin, const_itr_end, svOutVec.begin(), sqUDD);
            bolt::amp::transform(const_itr_begin, const_itr_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::vector<UDD> const_vector(length,temp);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }	

        {/*Test case when the input is a counting iterator */
            //bolt::amp::transform(count_itr_begin, count_itr_end, svOutVec.begin(), sqUDD);
            bolt::amp::transform(count_itr_begin, count_itr_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::transform(count_itr_begin, count_itr_end, stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}


TEST( TransformIterator, UnaryTransformUDD_floatRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD3 > svIn1Vec( length );
        std::vector< UDD3 > svOutVec( length );
        std::vector< UDD3 > stlOut( length );

        /*Generate inputs*/
        gen_input_udd_floatonly genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
		/*for(int i=0;i<length; i++)
		{
			svIn1Vec[i].a = (float) i;
			svIn1Vec[i].b = (float) i * 2;
		}*/

        bolt::BCKND::device_vector< UDD3 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD3 > dvOutVec( length );

        std::vector< float > stlOut_float( length );
        std::vector< float > svOutVec_float( length );
        bolt::BCKND::device_vector< float > dvOutVec_float( length );

        squareUDD_resultUDD_floatonly sqUDD;

        typedef std::vector< UDD3>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD3 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD3 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD3 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD_floatonly, std::vector< UDD3 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD_floatonly, bolt::BCKND::device_vector< UDD3 >::iterator>   dv_trf_itr_add3;
     
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        UDD3 temp;
        temp.a=1.0f, temp.b=2.5f;

        UDD3 init;
        init.a=0.0f, init.b=0.0f;


        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;


        {/*Test case when input is trf Iterator*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), sqUDD);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), sqUDD);

            std::vector<UDD3> temp1_vec(dv_trf_begin1, dv_trf_end1);
            std::vector<UDD3> temp2_vec(dvOutVec.begin(), dvOutVec.end());

            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the input is randomAccessIterator */
            bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), sqUDD);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), sqUDD);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform(const_itr_begin, const_itr_end, svOutVec.begin(), sqUDD);
            bolt::amp::transform(const_itr_begin, const_itr_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::vector<UDD3> const_vector(length,temp);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), sqUDD);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }	
    }
}




TEST( TransformIterator, BinaryTransformRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        add_4 add4;
        bolt::amp::plus<int> plus;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;    
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add4);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add4);
        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the second input is trf_itr and the first is a randomAccessIterator */
            //bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec.begin(), plus);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the both are randomAccessIterator */
            //bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin(), plus);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a constant iterator */
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, const_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, const_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::transform(dv_trf_begin1, dv_trf_end1, const_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a counting iterator */
           // bolt::amp::transform(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::transform(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::transform(const_itr_begin, const_itr_end, count_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            std::transform(const_vector.begin(), const_vector.end(), count_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}

TEST( TransformIterator, BinaryTransformUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svIn2Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< int > tsvOutVec( length );
        std::vector< UDD2 > stlOut( length );
        std::vector< int > tstlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );
        bolt::BCKND::device_vector< int > tdvOutVec( length );

        bolt::amp::plus<UDD2> plus;

        cubeUDD_resultUDD cbUDD;
        squareUDD_resultUDD sqUDD;

        squareUDD_result_int sq_int;
        cubeUDD_result_int cb_int;

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< cubeUDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< cubeUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add4;    

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< cubeUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< cubeUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add4;    


        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), cbUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), cbUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        //tsv_trf_itr_add4 tsv_trf_begin2 (svIn2Vec.begin(), cb_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
        tdv_trf_itr_add4 tdv_trf_begin2 (dvIn2Vec.begin(), cb_int);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;


       {/*Test case when both inputs are trf Iterators and Return type of UDD is int*/
            bolt::amp::plus<int> plus_int;
            //bolt::amp::transform(tsv_trf_begin1, tsv_trf_end1, tsv_trf_begin2, tsvOutVec.begin(), plus_int);
            bolt::amp::transform(tdv_trf_begin1, tdv_trf_end1, tdv_trf_begin2, tdvOutVec.begin(), plus_int);
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, tdv_trf_begin2, tstlOut.begin(), plus_int);
            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the second input is trf_itr and the first is a randomAccessIterator */
            //bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec.begin(), plus);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the both are randomAccessIterator */
            bolt::amp::transform(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin(), plus);
            bolt::amp::transform(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is trf_itr and the second is a constant iterator */
            //bolt::amp::transform(sv_trf_begin1, sv_trf_end1, const_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, const_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::transform(dv_trf_begin1, dv_trf_end1, const_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is counting iterator and the second is a trf_itr */
            //bolt::amp::transform( count_itr_begin, count_itr_end, sv_trf_begin1, svOutVec.begin(), plus);
            bolt::amp::transform( count_itr_begin, count_itr_end, dv_trf_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(count_itr_begin, count_itr_end, dv_trf_begin1, stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when the first input is counting iterator and the second is a constant iterator */
            //bolt::amp::transform(count_itr_begin, count_itr_end, const_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(count_itr_begin, count_itr_end, const_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);        
            std::transform(count_itr_begin, count_itr_end, const_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is trf_itr and the second is a counting iterator */
           // bolt::amp::transform(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<UDD2> count_vector(count_itr_begin, count_itr_end);    
            std::transform(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::transform(const_itr_begin, const_itr_end, count_itr_begin, svOutVec.begin(), plus);
            bolt::amp::transform(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::vector<UDD2> count_vector(count_itr_begin, count_itr_end); 
            std::transform(const_vector.begin(), const_vector.end(), count_vector.begin(), stlOut.begin(), plus);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Reduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(TransformIterator, Reduce)
{
    size_t view_size = WAVEFRNT_SIZE;

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype e2[WAVEFRNT_SIZE];



    std::iota(elements, elements + WAVEFRNT_SIZE, 1000);
    std::fill( e2, e2 + WAVEFRNT_SIZE, 100 );

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dve2(e2, e2 + WAVEFRNT_SIZE);

    std::vector<int> el(elements, elements + WAVEFRNT_SIZE);
    std::vector<int> el2(e2, e2 + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator( ).default_view;

    typedef bolt::amp::transform_iterator<my_negate, bolt::amp::device_vector<int>::iterator> transf_iter;

    transf_iter tbegin(dvebegin, my_negate( ));
    transf_iter tend(dveend, my_negate( ));

    int out = std::accumulate( boost::make_transform_iterator(el.begin( ),  my_negate( )), 
                               boost::make_transform_iterator(el.end( ),  my_negate( )),
                               0,
                               std::plus<int>( )
                             );

    int bolt_out = bolt::amp::reduce( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                                      bolt::amp::make_transform_iterator(dveend, my_negate( )),
                                      0,
                                      bolt::amp::plus<int>( )
                                     );
    EXPECT_EQ( out, bolt_out );

}

TEST( TransformIterator, ReduceRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        add_4 add4;
        bolt::amp::plus<int> plus;
        typedef std::vector< int >::const_iterator                                                         sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                                dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                      counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                      constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>                sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>                sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add4;    
        
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add4);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add4);
        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is trf Iterator*/
            /*int sv_result = bolt::amp::reduce(bolt::amp::make_transform_iterator(svIn1Vec.begin(), add3), 
                                             bolt::amp::make_transform_iterator(svIn1Vec.end(), add3), 0, plus);*/
            int dv_result = bolt::amp::reduce(bolt::amp::make_transform_iterator(dvIn1Vec.begin(), add3), 
                                             bolt::amp::make_transform_iterator(dvIn1Vec.end(), add3), 0, plus);
            /*Compute expected results*/
            int expected_result = std::accumulate(dv_trf_begin1, dv_trf_end1, 0, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is trf Iterator*/
            //int sv_result = bolt::amp::reduce(sv_trf_begin1, sv_trf_end1, 0, plus);
            int dv_result = bolt::amp::reduce(dv_trf_begin1, dv_trf_end1, 0, plus);
            /*Compute expected results*/
            int expected_result = std::accumulate(dv_trf_begin1, dv_trf_end1, 0, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the input is a randomAccessIterator */
            int sv_result = bolt::amp::reduce(svIn2Vec.begin(), svIn2Vec.end(), 0, plus);
            int dv_result = bolt::amp::reduce(dvIn2Vec.begin(), dvIn2Vec.end(), 0, plus);
            /*Compute expected results*/
            int expected_result = std::accumulate(svIn2Vec.begin(), svIn2Vec.end(), 0, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the input  is a constant iterator */
            //int sv_result = bolt::amp::reduce(const_itr_begin, const_itr_end, 0, plus);
            int dv_result = bolt::amp::reduce(const_itr_begin, const_itr_end, 0, plus);
            /*Compute expected results*/
            int expected_result = std::accumulate(const_itr_begin, const_itr_end, 0, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the input is a counting iterator */
            //int sv_result = bolt::amp::reduce(count_itr_begin, count_itr_end, 0, plus);
            int dv_result = bolt::amp::reduce(count_itr_begin, count_itr_end, 0, plus);
            /*Compute expected results*/
            int expected_result = std::accumulate(count_itr_begin, count_itr_end, 0, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    }
}


TEST( TransformIterator,ReduceUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svIn2Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        //gen_input_udd genUDD;
        //std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        //std::generate(svIn2Vec.begin(), svIn2Vec.end(), genUDD);
		for(long int i=0;i<length; i++)
		{
			svIn1Vec[i].i = i;
			svIn1Vec[i].f = (float) i;

			svIn2Vec[i].i = i*2;
			svIn2Vec[i].f = (float) i*2;
		}

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        UDDplus plus;
#if 0
        UDDmul mul;
#endif
        add3UDD_resultUDD sqUDD;
        add4UDD_resultUDD cbUDD;

        squareUDD_result_int sq_int;
        bolt::amp::plus<int> plus_int;

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add4UDD_resultUDD, std::vector< UDD2 >::const_iterator>                sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add4UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>       dv_trf_itr_add4;   

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;


        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), cbUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), cbUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);


        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=0, init.f=0.0f;


        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;



        UDD2 UDDzero;
        UDDzero.i = 0;
        UDDzero.f = 0.0f;

        UDD2 UDDone;
        UDDone.i = 1;
        UDDone.f = 1.0f;


        {/*Test case when inputs are trf Iterators*/
            /*int sv_result = bolt::amp::reduce(bolt::amp::make_transform_iterator(svIn1Vec.begin(), sq_int), 
                                             bolt::amp::make_transform_iterator(svIn1Vec.end(), sq_int), 0, plus_int);*/
            int dv_result = bolt::amp::reduce(bolt::amp::make_transform_iterator(dvIn1Vec.begin(), sq_int), 
                                             bolt::amp::make_transform_iterator(dvIn1Vec.end(), sq_int), 0, plus_int);
            /*Compute expected results*/
            int expected_result = std::accumulate(tdv_trf_begin1, tdv_trf_end1, 0, plus_int);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is trf Iterator and UDD is returning an int*/
            //int sv_result = bolt::amp::reduce(tsv_trf_begin1, tsv_trf_end1, 0, plus_int);
            int dv_result = bolt::amp::reduce(tdv_trf_begin1, tdv_trf_end1, 0, plus_int);
            /*Compute expected results*/
            int expected_result = std::accumulate(tdv_trf_begin1, tdv_trf_end1, 0, plus_int);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is trf Iterator*/
            //UDD2 sv_result = bolt::amp::reduce(sv_trf_begin1, sv_trf_end1, UDDzero, plus);
            UDD2 dv_result = bolt::amp::reduce(dv_trf_begin1, dv_trf_end1, UDDzero, plus);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(dv_trf_begin1, dv_trf_end1, UDDzero, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is a randomAccessIterator */
            UDD2 sv_result = bolt::amp::reduce(svIn2Vec.begin(), svIn2Vec.end(), UDDzero, plus);
            UDD2 dv_result = bolt::amp::reduce(dvIn2Vec.begin(), dvIn2Vec.end(), UDDzero, plus);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(svIn2Vec.begin(), svIn2Vec.end(), UDDzero, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is a constant iterator */
            UDD2 sv_result = bolt::amp::reduce(const_itr_begin, const_itr_end, UDDzero, plus);
            UDD2 dv_result = bolt::amp::reduce(const_itr_begin, const_itr_end, UDDzero, plus);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(const_itr_begin, const_itr_end, UDDzero, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is a counting iterator */
            UDD2 sv_result = bolt::amp::reduce(count_itr_begin, count_itr_end, UDDzero, plus);
            UDD2 dv_result = bolt::amp::reduce(count_itr_begin, count_itr_end, UDDzero, plus);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(count_itr_begin, count_itr_end, UDDzero, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

    //Failing Test cases...
#if 0
        int mul_test_length = 20;
        {/*Test case when input is trf Iterator*/
           // UDD2 sv_result = bolt::amp::reduce(sv_trf_begin1, sv_trf_begin1+mul_test_length, UDDone, mul);
            UDD2 dv_result = bolt::amp::reduce(dv_trf_begin1, dv_trf_begin1+mul_test_length, UDDone, mul);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(dv_trf_begin1, dv_trf_begin1+mul_test_length, UDDone, mul);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is a randomAccessIterator */
            UDD2 sv_result = bolt::amp::reduce(svIn2Vec.begin(), svIn2Vec.begin()+mul_test_length, UDDone, mul);
            UDD2 dv_result = bolt::amp::reduce(dvIn2Vec.begin(), dvIn2Vec.begin()+mul_test_length, UDDone, mul);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(svIn2Vec.begin(), svIn2Vec.begin()+mul_test_length, UDDone, mul);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is a constant iterator */
            //UDD2 sv_result = bolt::amp::reduce(const_itr_begin, const_itr_begin+mul_test_length, UDDone, mul);
            UDD2 dv_result = bolt::amp::reduce(const_itr_begin, const_itr_begin+mul_test_length, UDDone, mul);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(const_itr_begin, const_itr_begin+mul_test_length, UDDone, mul);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is a counting iterator */
            //UDD2 sv_result = bolt::cl::reduce(count_itr_begin, count_itr_begin+mul_test_length, UDDone, mul);
            UDD2 dv_result = bolt::amp::reduce(count_itr_begin, count_itr_begin+mul_test_length, UDDone, mul);
            /*Compute expected results*/
            UDD2 expected_result = std::accumulate(count_itr_begin, count_itr_begin+mul_test_length, UDDone, mul);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
#endif
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ReduceByKey tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST( TransformIterator, ReduceByKeyRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec1( length );
        std::vector< int > svOutVec2( length );
        std::vector< int > stlOut1( length );
        std::vector< int > stlOut2( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec1( length );
        bolt::BCKND::device_vector< int > dvOutVec2( length );

        add_3 add3;
        add_4 add4;
        bolt::amp::equal_to<int> binary_predictor;
        bolt::amp::plus<int> binary_operator;


        typedef std::vector< int >::const_iterator                                                         sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                                dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                      counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                      constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>                sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>                sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>       dv_trf_itr_add4;  

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add4);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add4);
        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;


        std::vector< int > testInput1( svIn1Vec.begin(), svIn1Vec.end() );
        std::vector< int > testInput2( svIn2Vec.begin(), svIn2Vec.end() );
        for(int i=0; i<length; i++)
        {
            testInput1[i] = testInput1[i] + 3;
            testInput2[i] = testInput2[i] + 4;
        }

        std::vector< int > constVector(length, 1);
        std::vector< int > countVector(length);
        for(int i=0; i<length; i++)
        {
            countVector[i]=i;
        }

        {/*Test case when inputs are trf Iterators*/
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&testInput1[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&testInput1[0], &svIn2Vec[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

         {/*Test case when the first input is randomAccessIterator and the second is a trf_itr*/
            //auto sv_result = bolt::amp::reduce_by_key(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&svIn1Vec[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }


       
        {/*Test case when the first input is trf_itr and the second is a constant iterator */
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, const_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, const_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&testInput1[0], &constVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is constant iterator and the second is a  trf_itr */
            //auto sv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&constVector[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }
        {/*Test case when the first input is trf_itr and the second is a counting iterator */      
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&testInput1[0], &countVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is counting iterator and the second is a trf_itr */
            //auto sv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&countVector[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }


         {/*Test case when the both inputs are randomAccessIterators*/
            auto sv_result = bolt::amp::reduce_by_key(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&svIn1Vec[0], &svIn2Vec[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //auto sv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, count_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&constVector[0], &countVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }

         {/*Test case when the first input is counting iterator and the second is a constant iterator */
            //auto sv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, const_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, const_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<int, int, int, int, bolt::amp::plus< int >> (&countVector[0], &constVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

    }
}


TEST( TransformIterator, ReduceByKeyUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svIn2Vec( length );
        std::vector< UDD2 > svOutVec1( length );
        std::vector< UDD2 > svOutVec2( length );
        std::vector< UDD2 > stlOut1( length );
        std::vector< UDD2 > stlOut2( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec1( length );
        bolt::BCKND::device_vector< UDD2 > dvOutVec2( length );

        bolt::amp::equal_to<UDD2> binary_predictor;
        bolt::amp::plus<UDD2> binary_operator;
        add3UDD_resultUDD sqUDD;
        add4UDD_resultUDD cbUDD;
        

        squareUDD_result_int sq_int;
        cubeUDD_result_int cb_int;

        std::vector< int > stlOut1_int( length );
        std::vector< int > stlOut2_int( length );

        typedef std::vector< UDD2 >::const_iterator                                                         sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                                dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                      counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                      constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>                sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>       dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add4UDD_resultUDD, std::vector< UDD2 >::const_iterator>                sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add4UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>       dv_trf_itr_add4;    

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>              tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>     tdv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< cubeUDD_result_int, std::vector< UDD2 >::const_iterator>                tsv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< cubeUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>       tdv_trf_itr_add4;


        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), cbUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), cbUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        //tsv_trf_itr_add4 tsv_trf_begin2 (svIn2Vec.begin(), cb_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
        tdv_trf_itr_add4 tdv_trf_begin2 (dvIn2Vec.begin(), cb_int);

        UDD2 temp;
        temp.i = (int) rand()%10;
        temp.f = (float) (rand()%10);
        UDD2 t;
        t.i = 0;
        t.f = 0.0f;

        counting_itr count_itr_begin(t);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;



        std::vector< UDD2 > testInput1( svIn1Vec.begin(), svIn1Vec.end() );
        std::vector< UDD2 > testInput2( svIn2Vec.begin(), svIn2Vec.end() );
        for(int i=0; i<length; i++)
        {
            testInput1[i].i = testInput1[i].i + 3;   
            testInput1[i].f = testInput1[i].f + 3.f; 
            testInput2[i].i = testInput2[i].i + 4;   
            testInput2[i].f = testInput2[i].f + 4.f; 
        }

        std::vector< int > ttestInput1( length);
        std::vector< int > ttestInput2( length);
        for(int i=0; i<length; i++)
        {
            ttestInput1[i] = svIn1Vec[i].i + (int) svIn1Vec[i].f;   
            ttestInput2[i] = svIn2Vec[i].i + (int) svIn2Vec[i].f + 3;   
        }

        std::vector< UDD2 > constVector(const_itr_begin, const_itr_end);
        std::vector< UDD2 > countVector(count_itr_begin, count_itr_end);
    

        {/*Test case when inputs are trf Iterators*/
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2 >> (&testInput1[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2 >> (&testInput1[0], &svIn2Vec[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is randomAccessIterator and the second is a trf_itr*/
            //auto sv_result = bolt::amp::reduce_by_key(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2 >> (&svIn1Vec[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }


        {/*Test case when the first input is trf_itr and the second is a constant iterator */
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, const_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, const_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
			std::vector<UDD2> bolt_out1 (dvOutVec1.begin(), dvOutVec1.end());
			std::vector<UDD2> bolt_out2 (dvOutVec2.begin(), dvOutVec2.end());
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&testInput1[0], &constVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is constant iterator and the second is a  trf_itr */
            //auto sv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&constVector[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }


        {/*Test case when the first input is trf_itr and the second is a counting iterator */      
            //auto sv_result = bolt::amp::reduce_by_key(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&testInput1[0], &countVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is counting iterator and the second is a trf_itr */
            //auto sv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, sv_trf_begin2, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, dv_trf_begin2, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&countVector[0], &testInput2[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }


         {/*Test case when the both inputs are randomAccessIterators*/
            auto sv_result = bolt::amp::reduce_by_key(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&svIn1Vec[0], &svIn2Vec[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //auto sv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, count_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&constVector[0], &countVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
        	//cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
        	cmpArrays(dvOutVec2, stlOut2);
        }
         {/*Test case when the first input is counting iterator and the second is a constant iterator */
            //auto sv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, const_itr_begin, svOutVec1.begin(), svOutVec2.begin(), binary_predictor, binary_operator);
            auto dv_result = bolt::amp::reduce_by_key(count_itr_begin, count_itr_end, const_itr_begin, dvOutVec1.begin(), dvOutVec2.begin(), binary_predictor, binary_operator);
            /*Compute expected results*/
            unsigned int n= Serial_reduce_by_key<UDD2, UDD2, UDD2, UDD2, bolt::amp::plus< UDD2>> (&countVector[0], &constVector[0], &stlOut1[0], &stlOut2[0], binary_operator, length);
            /*Check the results*/
            //cmpArrays(svOutVec1, stlOut1);
            //cmpArrays(svOutVec2, stlOut2);
            cmpArrays(dvOutVec1, stlOut1);
            cmpArrays(dvOutVec2, stlOut2);
        }

    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TransformReduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST(TransformIterator, TransformReduce)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);

    std::vector<int> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<int> check(empty, empty + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::transform( boost::make_transform_iterator(el.begin( ), my_negate( )), 
                    boost::make_transform_iterator(el.end( ), my_negate( )),
                    check.begin( ),
                    std::identity<int>( )
                    );
    int acc = std::accumulate( check.begin( ), check.end( ), 0, std::plus< int >( ) );

    int bolt_acc = bolt::amp::transform_reduce( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                                                bolt::amp::make_transform_iterator(dveend, my_negate( )),
                                                bolt::amp::identity<int>( ),
                                                0,
                                                bolt::amp::plus<int>( )
                                              );

    EXPECT_EQ( acc, bolt_acc );

}

TEST(TransformIterator, TransformReduceFloat)
{

    typedef float etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000.0f);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0.0f);

    bolt::amp::device_vector<float, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);

    std::vector<float> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<float> check(empty, empty + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::transform( boost::make_transform_iterator( el.begin( ), square_root( ) ), 
                    boost::make_transform_iterator( el.end( ), square_root( )),
                    check.begin( ),
                    std::identity< float >( )
                    );
    float acc = std::accumulate( check.begin( ), check.end( ), 0.0f, std::plus< float >( ) );

    float bolt_acc = bolt::amp::transform_reduce( bolt::amp::make_transform_iterator(dvebegin, square_root( )), 
                                                  bolt::amp::make_transform_iterator(dveend, square_root( )),
                                                  bolt::amp::identity< float >( ),
                                                  0.0f,
                                                  bolt::amp::plus< float >( )
                                                );

    EXPECT_FLOAT_EQ( acc, bolt_acc );

}

TEST( TransformIterator, TransformReduceRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > stlOut( length );
        
        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        add_3 add3;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;   

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        int init = (int) rand();
        bolt::amp::plus<int> plus;
        {/*Test case when input is trf Iterators*/
            //int sv_result = bolt::amp::transform_reduce(sv_trf_begin1, sv_trf_end1, add3, init, plus);
            int dv_result = bolt::amp::transform_reduce(dv_trf_begin1, dv_trf_end1,  add3, init, plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), add3);
            int expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is randomAccessIterator */
            int sv_result = bolt::amp::transform_reduce(svIn1Vec.begin(), svIn1Vec.end(), add3, init, plus);
            int dv_result = bolt::amp::transform_reduce(dvIn1Vec.begin(), dvIn1Vec.end(), add3, init, plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), add3);
            int expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when  input is constant iterator */
            int sv_result = bolt::amp::transform_reduce(const_itr_begin, const_itr_end, add3, init, plus);
            int dv_result = bolt::amp::transform_reduce(const_itr_begin, const_itr_end, add3, init, plus);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), add3);
            int expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is counting iterator */
            int sv_result = bolt::amp::transform_reduce(count_itr_begin, count_itr_end, add3, init, plus);
            int dv_result = bolt::amp::transform_reduce(count_itr_begin, count_itr_end, add3, init, plus);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::transform(count_vector.begin(), count_vector.end(), stlOut.begin(), add3);
            int expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    }
}


TEST( TransformIterator, TransformReduceUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > stlOut( length );

        gen_input_udd genUDD;
        /*Generate inputs*/
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

        typedef std::vector< UDD2 >::const_iterator            sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator   dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >         counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >         constant_itr;

        add3UDD_resultUDD add3UDD;

        squareUDD_result_int sq_int;

        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3UDD ), sv_trf_end1 (svIn1Vec.end(), add3UDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3UDD ), dv_trf_end1 (dvIn1Vec.end(), add3UDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int ), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int ), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

        std::vector< int > stlOut_int( length );


        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 t;
        t.i=0, t.f=0.0f;

        counting_itr count_itr_begin(t);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;



        UDD2 init;
        init.i = (int) rand();
        init.f = (float) rand();

        bolt::amp::plus<UDD2> plus;

        int init_int = rand();
        bolt::amp::plus<int> plus_int;

        
        {/*Test case when input is a trf Iterator and return type of UDD is int*/
            //int sv_result = bolt::amp::transform_reduce(tsv_trf_begin1, tsv_trf_end1, sq_int, init_int, plus_int);
            int dv_result = bolt::amp::transform_reduce(tdv_trf_begin1, tdv_trf_end1, sq_int, init_int, plus_int);
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, stlOut_int.begin(), sq_int);
            int expected_result = std::accumulate(stlOut_int.begin(), stlOut_int.end(), init_int, plus_int);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when input is a trf Iterator*/
           // UDD2 sv_result = bolt::amp::transform_reduce(sv_trf_begin1, sv_trf_end1, add3UDD, init, plus);
            UDD2 dv_result = bolt::amp::transform_reduce(dv_trf_begin1, dv_trf_end1, add3UDD, init, plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), add3UDD);
            UDD2 expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when input is a randomAccessIterator */
            UDD2 sv_result = bolt::amp::transform_reduce(svIn1Vec.begin(), svIn1Vec.end(), add3UDD, init, plus);
            UDD2 dv_result = bolt::amp::transform_reduce(dvIn1Vec.begin(), dvIn1Vec.end(), add3UDD, init, plus);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), add3UDD);
            UDD2 expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        
        {/*Test case when input is constant iterator */
            //UDD2 sv_result = bolt::amp::transform_reduce(const_itr_begin, const_itr_end, add3UDD, init, plus);
            UDD2 dv_result = bolt::amp::transform_reduce(const_itr_begin, const_itr_end, add3UDD, init, plus);
            /*Compute expected results*/
            //std::vector<UDD> const_vector(length,temp);
            std::transform(const_itr_begin, const_itr_end, stlOut.begin(), add3UDD); //causes SEH Exception
            UDD2 expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    
        {/*Test case when  input a counting iterator */
            UDD2 sv_result = bolt::amp::transform_reduce(count_itr_begin, count_itr_end, add3UDD, init, plus);
            UDD2 dv_result = bolt::amp::transform_reduce(count_itr_begin, count_itr_end, add3UDD, init, plus);
            /*Compute expected results*/
            std::transform(count_itr_begin, count_itr_end, stlOut.begin(), add3UDD);
            UDD2 expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Inner Product tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////




TEST(TransformIteratorSimpleTest, InnerProduct)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype elements2[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 10);
    std::iota(elements2, elements2+WAVEFRNT_SIZE, 99);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    std::vector<int> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<int> el2(elements2, elements2+WAVEFRNT_SIZE);
    std::vector<int> check(empty, empty + WAVEFRNT_SIZE);

	bolt::amp::device_vector<int> dve(el.begin(), el.end());
    bolt::amp::device_vector<int> dve2(el2.begin(), el2.end());


	my_negate func;

	typedef bolt::BCKND::transform_iterator< my_negate, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_input1, dv_trf_itr_input2;
	dv_trf_itr_input1 dv_trf_begin1 (dve.begin(), func), dv_trf_end1 (dve.end(), func);
	dv_trf_itr_input2 dv_trf_begin2 (dve2.begin(), func);


	std::transform( dv_trf_begin1, 
                    dv_trf_end1,
                    dv_trf_begin2, 
                    check.begin( ),
                    std::plus<int>( )
                    );
    int acc = std::accumulate( check.begin( ), check.end( ), 0, std::plus< int >( ) );

	int bolt_acc = bolt::amp::inner_product( dv_trf_begin1, 
                                             dv_trf_end1,
                                             dv_trf_begin2, 
                                             0,
                                             bolt::amp::plus<int>( ),
                                             bolt::amp::plus<int>( )
                                            ); 

	EXPECT_EQ( acc, bolt_acc );


}


TEST(TransformIterator, InnerProduct)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype elements2[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 10);
    std::iota(elements2, elements2+WAVEFRNT_SIZE, 99);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    std::vector<int> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<int> el2(elements2, elements2+WAVEFRNT_SIZE);
    std::vector<int> check(empty, empty + WAVEFRNT_SIZE);

	bolt::amp::device_vector<int> dve(el.begin(), el.end());
    bolt::amp::device_vector<int> dve2(el2.begin(), el2.end());

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );
    auto dve2begin = dve.begin( );

	

   /* std::transform( boost::make_transform_iterator(el.begin( ), my_negate( )), 
                    boost::make_transform_iterator(el.end( ), my_negate( )),
                    boost::make_transform_iterator(el2.begin( ), my_negate( )), 
                    check.begin( ),
                    std::plus<int>( )
                    );*/

	std::transform( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                    bolt::amp::make_transform_iterator(dveend, my_negate( )),
                    bolt::amp::make_transform_iterator(dve2begin, my_negate( )),
                    check.begin( ),
                    std::plus<int>( )
                    );

    int acc = std::accumulate( check.begin( ), check.end( ), 0, std::plus< int >( ) );

    int bolt_acc = bolt::amp::inner_product( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                                             bolt::amp::make_transform_iterator(dveend, my_negate( )),
                                             bolt::amp::make_transform_iterator(dve2begin, my_negate( )),
                                             0,
                                             bolt::amp::plus<int>( ),
                                             bolt::amp::plus<int>( )
                                            ); 


	EXPECT_EQ( acc, bolt_acc );


}


TEST( TransformIterator, InnerProductRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length);
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), rand);


        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end());
        
        add_3 add3;
        bolt::amp::plus<int> plus;
        bolt::amp::minus<int> minus;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
       
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add3 sv_trf_begin2 (svIn2Vec.begin(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        counting_itr count_itr_begin2(10);
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;
        constant_itr const_itr_begin2(5);

        dv_trf_itr_add3 dv_trf_begin2 (dvIn2Vec.begin(), add3);
        int init = (int) rand();

        {/*Test case when both inputs are trf Iterators*/
            //int sv_result = bolt::amp::inner_product(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, init, plus, minus);
            int dv_result = bolt::amp::inner_product(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, init, plus, minus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, stlOut.begin(), minus);
            int expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the both inputs are randomAccessIterator */
            int sv_result = bolt::amp::inner_product(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), init, plus, minus);
            int dv_result = bolt::amp::inner_product(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), init, plus, minus);
            /*Compute expected results*/
            int expected_result = std::inner_product(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), init, std::plus<int>(), std::minus<int>());
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when both inputs are constant iterator */
            int sv_result = bolt::amp::inner_product(const_itr_begin, const_itr_end, const_itr_begin2, init, plus, minus);
            int dv_result = bolt::amp::inner_product(const_itr_begin, const_itr_end, const_itr_begin2, init, plus, minus);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> const_vector2(length,5);
            int expected_result = std::inner_product(const_vector.begin(), const_vector.end(), const_vector2.begin(), init, std::plus<int>(), std::minus<int>());
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when both inputs are counting iterator */
            int sv_result = bolt::amp::inner_product(count_itr_begin, count_itr_end, count_itr_begin2, init, plus, minus);
            int dv_result = bolt::amp::inner_product(count_itr_begin, count_itr_end, count_itr_begin2, init, plus, minus);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            std::vector<int> count_vector2(length);
            for (int index=0;index<length;index++)
            {
                count_vector[index] = index;
                count_vector2[index] = 10 + index;
            }
            int expected_result = std::inner_product(count_vector.begin(), count_vector.end(), count_vector2.begin(), init, std::plus<int>(), std::minus<int>());
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    }
}


TEST( TransformIterator, InnerProductUDDRoutine)
{
    {
        const int length = 1<<8;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svIn2Vec( length);
        std::vector< int > stlOutVec_int( length );

        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        gen_input_udd2 genUDD2;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), genUDD2);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end());
        
        bolt::amp::plus<UDD2> plus;
        bolt::amp::multiplies<UDD2> mul;

        //UDDminus minus;
        add3UDD_resultUDD sqUDD;

        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
       
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        //sv_trf_itr_add3 sv_trf_begin2 (svIn2Vec.begin(), sqUDD), sv_trf_end2 (svIn2Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(),sqUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        //tsv_trf_itr_add3 tsv_trf_begin2 (svIn2Vec.begin(), sq_int), tsv_trf_end2 (svIn2Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;
        UDD2 temp2;
        temp2.i=15, temp2.f=7.5f;
        UDD2 init1;
        init1.i=1, init1.f=1.0f;
        UDD2 init2;
        init2.i=10, init2.f=10.0f;

        counting_itr count_itr_begin(init1);
        counting_itr count_itr_end = count_itr_begin + length;
        counting_itr count_itr_begin2(init2);
        counting_itr count_itr_end2 = count_itr_begin2 + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;
        constant_itr const_itr_begin2(temp2);
        constant_itr const_itr_end2 = const_itr_begin2 + length;

        dv_trf_itr_add3 dv_trf_begin2 (dvIn2Vec.begin(), sqUDD);
        tdv_trf_itr_add3 tdv_trf_begin2 (dvIn2Vec.begin(), sq_int);

        UDD2 init;
        init.i = rand();
        init.f = (float) rand();

        int init_int = rand()%10;

        //std::vector< UDD2 > sv_trf_begin2_copy( sv_trf_begin2, sv_trf_end2);
        //std::vector< int> tsv_trf_begin2_copy( tsv_trf_begin2, tsv_trf_end2);
        std::vector< UDD2 > sv_trf_begin2_copy( dv_trf_begin2, dv_trf_begin2 + length);
        std::vector< int> tsv_trf_begin2_copy( tdv_trf_begin2, tdv_trf_begin2 + length);

     
        {/*Test case when both inputs are trf Iterators with UDD returning int*/
            bolt::amp::multiplies<int> mul_int;
            bolt::amp::plus<int> plus_int;
            //int sv_result = bolt::amp::inner_product(tsv_trf_begin1, tsv_trf_end1, tsv_trf_begin2, init_int, plus_int, mul_int);
            int dv_result = bolt::amp::inner_product(tdv_trf_begin1, tdv_trf_end1, tdv_trf_begin2, init_int, plus_int, mul_int);
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, tsv_trf_begin2_copy.begin(), stlOutVec_int.begin(), mul_int);
            int expected_result = std::accumulate(stlOutVec_int.begin(), stlOutVec_int.end(), init_int, plus_int);

            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        
        {/*Test case when both inputs are trf Iterators*/
            //UDD2 sv_result = bolt::amp::inner_product(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, init, plus, mul);
            UDD2 dv_result = bolt::amp::inner_product(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, init, plus, mul);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, sv_trf_begin2_copy.begin(), stlOut.begin(), mul);
            UDD2 expected_result = std::accumulate(stlOut.begin(), stlOut.end(), init, plus);

            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        
        {/*Test case when both inputs are constant iterators */
            //UDD2 sv_result = bolt::amp::inner_product(const_itr_begin, const_itr_end, const_itr_begin2, init, plus, mul);
            UDD2 dv_result = bolt::amp::inner_product(const_itr_begin, const_itr_end, const_itr_begin2, init, plus, mul);
            /*Compute expected results*/
            std::vector<UDD2> const_vector2(const_itr_begin2, const_itr_end2); //No Compilation Error. But no Output!
            UDD2 expected_result = std::inner_product(const_itr_begin, const_itr_end, const_vector2.begin(), init, plus, mul);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when the both inputs are randomAccessIterator */
            UDD2 sv_result = bolt::amp::inner_product(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), init, plus, mul);
            UDD2 dv_result = bolt::amp::inner_product(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), init, plus, mul);
            /*Compute expected results*/
            UDD2 expected_result = std::inner_product(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), init, plus, mul);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when both inputs are counting iterators */
            UDD2 sv_result = bolt::amp::inner_product(count_itr_begin, count_itr_end, count_itr_begin2, init, plus, mul);
            UDD2 dv_result = bolt::amp::inner_product(count_itr_begin, count_itr_end, count_itr_begin2, init, plus, mul);
            /*Compute expected results*/
            std::vector<UDD2> count_vector2(count_itr_begin2, count_itr_end2); 
            UDD2 expected_result = std::inner_product(count_itr_begin, count_itr_end, count_vector2.begin(), init, plus, mul);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copy tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(TransformIterator, Copy)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + WAVEFRNT_SIZE);

    std::vector<int> el(elements, elements+WAVEFRNT_SIZE);
    std::vector<int> check(empty, empty + WAVEFRNT_SIZE);

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::copy( boost::make_transform_iterator(el.begin( ), my_negate( )), 
               boost::make_transform_iterator(el.end( ), my_negate( )),
               check.begin( )
               );

    bolt::amp::copy( bolt::amp::make_transform_iterator(dvebegin, my_negate( )), 
                     bolt::amp::make_transform_iterator(dveend, my_negate( )),
                     dumpV.begin( )
                   );

    cmpArrays(check,dumpV);

}

TEST( TransformIterator, CopyRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
       
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;

        gen_input gen;
        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>          sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add4;  

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        /*Generate inputs*/
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        {/*Test case when input is trf Iterator*/
            //bolt::amp::copy(sv_trf_begin1, sv_trf_end1, svOutVec.begin());
            bolt::amp::copy(dv_trf_begin1, dv_trf_end1, dvOutVec.begin());
            /*Compute expected results*/
            std::copy(dv_trf_begin1, dv_trf_end1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is randomAccessIterator */
            bolt::amp::copy(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin());
            bolt::amp::copy(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin());
            /*Compute expected results*/
            std::copy(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin());
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the  input is constant iterator */
            //bolt::amp::copy(const_itr_begin, const_itr_end, svOutVec.begin());
            bolt::amp::copy(const_itr_begin, const_itr_end, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::copy(const_vector.begin(), const_vector.end(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is a counting iterator */
            //bolt::amp::copy_n(count_itr_begin, length, svOutVec.begin());
            bolt::amp::copy_n(count_itr_begin, length, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::copy(count_vector.begin(), count_vector.end(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Count tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (TransformIterator, CountIfTest)
{
    int aSize = 1024;
    std::vector<int> A(aSize);

    for (int i=0; i < aSize; i++) {
        A[i] = rand( ) % 10 + 1;
    }
    bolt::amp::device_vector< int > dA(A.begin( ), aSize);
    int intVal = 1;
    
    int stdInRangeCount =  static_cast<int>( std::count(
         boost::make_transform_iterator( A.begin( ), my_negate( ) ),
         boost::make_transform_iterator( A.end( ), my_negate( ) ),
         intVal ) );
    int boltInRangeCount = static_cast<int>( bolt::amp::count(
         bolt::amp::make_transform_iterator( dA.begin( ), my_negate( ) ),
         bolt::amp::make_transform_iterator( dA.end( ), my_negate( ) ),
         intVal ) );

    EXPECT_EQ(stdInRangeCount, boltInRangeCount);
}

TEST( TransformIterator, CountRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        
        add_3 add3;
        gen_input gen;

        /*Generate inputs*/
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
   
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        int val = (int) rand();

        {/*Test case when inputs are trf Iterators*/
            //bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type sv_result = (int) bolt::amp::count(sv_trf_begin1, sv_trf_end1, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type dv_result = (int) bolt::amp::count(dv_trf_begin1, dv_trf_end1, val);
            /*Compute expected results*/
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(dv_trf_begin1, dv_trf_end1, val);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the both are randomAccessIterator */
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type sv_result = (int) bolt::amp::count(svIn1Vec.begin(), svIn1Vec.end(), val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type dv_result = (int) bolt::amp::count(dvIn1Vec.begin(), dvIn1Vec.end(), val);
            /*Compute expected results*/
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(svIn1Vec.begin(), svIn1Vec.end(), val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type sv_result = (int) bolt::amp::count(const_itr_begin, const_itr_end, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type dv_result = (int) bolt::amp::count(const_itr_begin, const_itr_end, val);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(const_vector.begin(), const_vector.end(), val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type sv_result = (int) bolt::amp::count(count_itr_begin, count_itr_end, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type dv_result = (int) bolt::amp::count(count_itr_begin, count_itr_end, val);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(count_vector.begin(), count_vector.end(), val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }     
    }
}

TEST( TransformIterator, CountUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );

        squareUDD_resultUDD sqUDD;
        squareUDD_result_int sq_int;

        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        
        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;
       
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);


        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=1, init.f=1.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        UDD2 val;
        val.i = rand();
        val.f = (float) rand();

        int val_int = rand();

        {/*Test case when inputs are trf Iterators*/
            //bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type sv_result =  bolt::amp::count(tsv_trf_begin1, tsv_trf_end1, val_int);
            bolt::amp::iterator_traits<bolt::amp::device_vector<int>::iterator>::difference_type dv_result =  bolt::amp::count(tdv_trf_begin1, tdv_trf_end1, val_int);
            /*Compute expected results*/
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(tdv_trf_begin1, tdv_trf_end1, val_int);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }


        {/*Test case when inputs are trf Iterators*/
            //bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type sv_result =  bolt::amp::count(sv_trf_begin1, sv_trf_end1, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type dv_result =  bolt::amp::count(dv_trf_begin1, dv_trf_end1, val);
            /*Compute expected results*/
            std::iterator_traits<std::vector<UDD2>::iterator>::difference_type expected_result = std::count(dv_trf_begin1, dv_trf_end1, val);
            /*Check the results*/
            //EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
        {/*Test case when the both are randomAccessIterator */
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type sv_result =  bolt::amp::count(svIn1Vec.begin(), svIn1Vec.end(), val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type dv_result =  bolt::amp::count(dvIn1Vec.begin(), dvIn1Vec.end(), val);
            /*Compute expected results*/
            std::iterator_traits<std::vector<UDD2>::iterator>::difference_type expected_result = std::count(svIn1Vec.begin(), svIn1Vec.end(), val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when the first input is constant iterator*/
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type sv_result =  bolt::amp::count(const_itr_begin, const_itr_end, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type dv_result =  bolt::amp::count(const_itr_begin, const_itr_end, val);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::iterator_traits<std::vector<UDD2>::iterator>::difference_type expected_result = std::count(const_vector.begin(), const_vector.end(), val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }

        {/*Test case when the first input is counting iterator */
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type sv_result =  bolt::amp::count(count_itr_begin, count_itr_end, val);
            bolt::amp::iterator_traits<bolt::amp::device_vector<UDD2>::iterator>::difference_type dv_result =  bolt::amp::count(count_itr_begin, count_itr_end, val);
            /*Compute expected results*/
            std::iterator_traits<std::vector<int>::iterator>::difference_type expected_result = std::count(count_itr_begin, count_itr_end, val);
            /*Check the results*/
            EXPECT_EQ( expected_result, sv_result );
            EXPECT_EQ( expected_result, dv_result );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scatter tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct is_even{
    bool operator ( ) (int x) const restrict(cpu,amp)
    {
        return ( (x % 2)==0);
    }
};

TEST( TransformIterator, ScatterIf )
{
    int n_input[10] =  {-11,-1,-2,-3,-4,-5,-6,-7,-8,-9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    int n_stencil[10] =  {0,1,0,1,0,1,0,1,0,1};

    std::vector<int> exp_result;
    {
        exp_result.push_back(-1);exp_result.push_back(8);
        exp_result.push_back(-1);exp_result.push_back(6);
        exp_result.push_back(-1);exp_result.push_back(4);
        exp_result.push_back(-1);exp_result.push_back(2);
        exp_result.push_back(-1);exp_result.push_back(11);
    }
    bolt::amp::device_vector<int> result ( 10, -1 );
    bolt::amp::device_vector<int> input ( n_input, n_input + 10 );
    bolt::amp::device_vector<int> map ( n_map, n_map + 10 );
    bolt::amp::device_vector<int> stencil ( n_stencil, n_stencil + 10 );

    
    is_even iepred;
    bolt::amp::scatter_if( bolt::amp::make_transform_iterator( input.begin( ), my_negate( ) ),
                           bolt::amp::make_transform_iterator( input.end( ), my_negate( ) ),
                           map.begin( ),
                           stencil.begin( ),
                           result.begin( ),
                           iepred );

    cmpArrays( exp_result, result );
}


TEST( TransformIterator, ScatterRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        add_0 add0;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;   


        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0);
        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::scatter(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::scatter(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the both are randomAccessIterator */
            bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_scatter(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter(const_itr_begin, const_itr_end, svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(const_itr_begin, const_itr_end, dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);          
            Serial_scatter(const_vector.begin(), const_vector.end(), dvIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter(const_itr_begin, const_itr_end, count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter(const_vector.begin(), const_vector.end(), count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }

 TEST( TransformIterator, ScatterUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< int > tsvOutVec( length );
        std::vector< UDD2 > stlOut( length );
        std::vector< int > tstlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );
        bolt::BCKND::device_vector< int > tdvOutVec( length );

        add3UDD_resultUDD add3;
        add_0 add0;

        squareUDD_result_int sq_int;


        typedef std::vector< UDD2 >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2  >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2  >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;  

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;


        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

        UDD2 temp;
        temp.i = rand()%10, temp.f = (float) rand();

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when both inputs are trf Iterators and UDD returns an int*/
            //bolt:::amp::scatter(tsv_trf_begin1, tsv_trf_end1, sv_trf_begin2, tsvOutVec.begin());
            bolt::amp::scatter(tdv_trf_begin1, tdv_trf_end1, dv_trf_begin2, tdvOutVec.begin());
            /*Compute expected results*/
			Serial_scatter(tdv_trf_begin1, tdv_trf_end1, dv_trf_begin2, tstlOut.begin());

            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::scatter(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, stlOut.begin());
            /*Check the results*/
           // cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt:::amp::scatter(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the both are randomAccessIterator */
            bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter(sv_trf_begin1, sv_trf_end1, count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
            {
                count_vector[index] = index;
            }
            Serial_scatter(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter(const_itr_begin, const_itr_end, svIn2Vec.begin(), svOutVec.begin());
            bolt::amp::scatter(const_itr_begin, const_itr_end, dvIn2Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);          
            Serial_scatter(const_vector.begin(), const_vector.end(), svIn2Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter(const_itr_begin, const_itr_end, count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(const_itr_begin, const_itr_end, count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
            {
                count_vector[index] = index;
            }          
            Serial_scatter(const_vector.begin(), const_vector.end(), count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::scatter(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svOutVec.begin());
            bolt::amp::scatter(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
            {
                count_vector[index] = index;
            }           
            Serial_scatter(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }


 TEST( TransformIterator, ScatterIfRoutine)
 {
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length ); // input
        std::vector< int > svIn2Vec( length ); // map
        std::vector< int > svIn3Vec( length ); // stencil
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        for(int i=0; i<length; i++)
        {
            if(i%2 == 0)
                svIn3Vec[i] = 0;
            else
                svIn3Vec[i] = 1;
        }
        bolt::BCKND::device_vector< int > dvIn3Vec( svIn3Vec.begin(), svIn3Vec.end() );

        add_3 add3;
        add_0 add0;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;  
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add0;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add0; 

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0);
        //sv_trf_itr_add0 sv_trf_begin3 (svIn3Vec.begin(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0);
        dv_trf_itr_add0 dv_trf_begin3 (dvIn3Vec.begin(), add0);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;
        constant_itr stencil_itr_begin(1);
        constant_itr stencil_itr_end = stencil_itr_begin + length;

        is_even iepred;

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, count_vector.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length), stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
                count_vector[index] = index;
        		stencil_vector[index] = 1;
        	}
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the both are randomAccessIterator */
            bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
       
        {/*Test case when the both are randomAccessIterator */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
            {
                stencil_vector[index] = 1;
            }
            Serial_scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);          
            Serial_scatter_if(const_vector.begin(), const_vector.end(), svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, svIn2Vec.begin(), stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, dvIn2Vec.begin(), stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1); 
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        	}
            Serial_scatter_if(const_vector.begin(), const_vector.end(), dvIn2Vec.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter_if(const_vector.begin(), const_vector.end(), count_vector.begin(), dvIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
                           
            Serial_scatter_if(const_vector.begin(), const_vector.end(), count_vector.begin(),  stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */     
			bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }


 TEST( TransformIterator, ScatterIfUDDRoutine)
 {
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length ); // input
        std::vector< int > svIn2Vec( length ); // map
        std::vector< int > svIn3Vec( length ); // stencil
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );
        std::vector< int > tsvOutVec( length );
        std::vector< int > tstlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);


        bolt::BCKND::device_vector< UDD2> dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );
        bolt::BCKND::device_vector< int > tdvOutVec( length );

        for(int i=0; i<length; i++)
        {
            if(i%2 == 0)
                svIn3Vec[i] = 0;
            else
                svIn3Vec[i] = 1;
        }
        bolt::BCKND::device_vector< int > dvIn3Vec( svIn3Vec.begin(), svIn3Vec.end() );

        add3UDD_resultUDD add3;
        add_0 add0;

        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  const_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;  
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add0;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add0; 

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0);
        //sv_trf_itr_add0 sv_trf_begin3 (svIn3Vec.begin(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0);
        dv_trf_itr_add0 dv_trf_begin3 (dvIn3Vec.begin(), add0);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

        UDD2 t;
        t.i = (int)rand()%10;
        t.f = (float)rand();

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(t);
        constant_itr const_itr_end = const_itr_begin + length;
        const_itr stencil_itr_begin(1);
        const_itr stencil_itr_end = stencil_itr_begin + length;

        is_even iepred;

        {/*Test case when both inputs are trf Iterators and UDD is returning an int*/
            //bolt::amp::scatter_if(tsv_trf_begin1, tsv_trf_end1, sv_trf_begin2, sv_trf_begin3, tsvOutVec.begin(), iepred);
            bolt::amp::scatter_if(tdv_trf_begin1, tdv_trf_end1, dv_trf_begin2, dv_trf_begin3, tdvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(tdv_trf_begin1, tdv_trf_end1, dv_trf_begin2, dv_trf_begin3, tstlOut.begin(), iepred);

           /* bolt::amp::control ctl = bolt::amp::control::getDefault( );
            ctl.setForceRunMode(bolt::amp::control::SerialCpu);
            bolt::amp::scatter_if(ctl, tdv_trf_begin1, tdv_trf_end1, dv_trf_begin2, dv_trf_begin3, tstlOut.begin(), iepred);*/

            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }
        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
           // cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, sv_trf_begin3, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dv_trf_begin3, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, dv_trf_begin3, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), sv_trf_begin2, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dv_trf_begin2, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), dv_trf_begin2, svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, count_vector.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::scatter_if(sv_trf_begin1, sv_trf_end1, count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dv_trf_begin1, dv_trf_end1, count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length), stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
                count_vector[index] = index;
        		stencil_vector[index] = 1;
        	}
            Serial_scatter_if(dv_trf_begin1, dv_trf_end1, count_vector.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the both are randomAccessIterator */
            bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
           // cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
       
        {/*Test case when the both are randomAccessIterator */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
            {
                stencil_vector[index] = 1;
            }
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        
        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, svIn2Vec.begin(), svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, dvIn2Vec.begin(), dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);          
            Serial_scatter_if(const_vector.begin(), const_vector.end(), svIn2Vec.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    
        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, svIn2Vec.begin(), stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, dvIn2Vec.begin(), stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t); 
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        	}
            Serial_scatter_if(const_vector.begin(), const_vector.end(), svIn2Vec.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter_if(const_vector.begin(), const_vector.end(), count_vector.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(const_itr_begin, const_itr_end, count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
                           
            Serial_scatter_if(const_vector.begin(), const_vector.end(), count_vector.begin(),  stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svIn3Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvIn3Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), svIn3Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, stencil_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::scatter_if(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, stencil_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
            Serial_scatter_if(svIn1Vec.begin(), svIn1Vec.end(), count_vector.begin(), stencil_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Gather tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


//TEST( TransformIterator, GatherIf )
//{
//    int n_map[10]     =  {0,1,2,3,4,5,6,7,8,9};
//    int n_input[10]   =  {-9,-8,-7,-6,-5,-4,-3,-2,-1,-0};
//    int n_stencil[10] =  {0,1,0,1,0,1,0,1,0,1};
//
//    std::vector<int> exp_result;
//    {
//        exp_result.push_back(9);exp_result.push_back(-1);
//        exp_result.push_back(7);exp_result.push_back(-1);
//        exp_result.push_back(5);exp_result.push_back(-1);
//        exp_result.push_back(3);exp_result.push_back(-1);
//        exp_result.push_back(1);exp_result.push_back(-1);
//    }
//    std::vector<int> result ( 10, -1 );
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//    std::vector<int> stencil ( n_stencil, n_stencil + 10 );
//
//    bolt::amp::device_vector<int> dmap ( map.begin( ), map.end( ) );
//    bolt::amp::device_vector<int> dinput ( input.begin( ), input.end( ) );
//    bolt::amp::device_vector<int> dstencil ( stencil.begin( ), stencil.end( ) );
//    bolt::amp::device_vector<int> dresult ( stencil.begin( ), stencil.end( ) );
//
//    is_even iepred;
//    bolt::amp::gather_if( map.begin( ),
//                          map.end( ),
//                          stencil.begin( ),
//                          bolt::amp::make_transform_iterator( dinput.begin( ), my_negate( ) ),
//                          result.begin( ),
//                          iepred );
//
//    EXPECT_EQ(exp_result, result);
//}

TEST( TransformIterator, GatherIfDV )
{
    int n_map[10]     =  {0,1,2,3,4,5,6,7,8,9};
    int n_input[10]   =  {-9,-8,-7,-6,-5,-4,-3,-2,-1,-0};
    int n_stencil[10] =  {0,1,0,1,0,1,0,1,0,1};

    std::vector<int> exp_result;
    {
        exp_result.push_back(9);exp_result.push_back(-1);
        exp_result.push_back(7);exp_result.push_back(-1);
        exp_result.push_back(5);exp_result.push_back(-1);
        exp_result.push_back(3);exp_result.push_back(-1);
        exp_result.push_back(1);exp_result.push_back(-1);
    }
    std::vector<int> result ( 10, -1 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );
    std::vector<int> stencil ( n_stencil, n_stencil + 10 );

    bolt::amp::device_vector<int> dmap ( map.begin( ), map.end( ) );
    bolt::amp::device_vector<int> dinput ( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dstencil ( stencil.begin( ), stencil.end( ) );
    bolt::amp::device_vector<int> dresult ( result.begin( ), result.end( ) );

    is_even iepred;
    bolt::amp::gather_if( dmap.begin( ),
                          dmap.end( ),
                          dstencil.begin( ),
                          bolt::amp::make_transform_iterator( dinput.begin( ), my_negate( ) ),
                          dresult.begin( ),
                          iepred );

    cmpArrays(exp_result, dresult);
}


TEST( TransformIterator, GatherRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);
        
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        add_0 add0;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;    

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3); 
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0), sv_trf_end2 (svIn2Vec.end(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0), dv_trf_end2 (dvIn2Vec.end(), add0);
        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;


        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::gather(sv_trf_begin2, sv_trf_end2, sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(dv_trf_begin2, dv_trf_end2, dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(dv_trf_begin2, dv_trf_end2, dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and map is a randomAccessIterator */
            //bolt::amp::gather(sv_trf_begin2, sv_trf_end2, svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(dv_trf_begin2, dv_trf_end2, dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(dv_trf_begin2, dv_trf_end2, svIn1Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and map is a trf_itr */
            //bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the both are randomAccessIterator */
            bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), svIn1Vec.begin(), stlOut.begin());
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin, count_itr_end, sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(count_itr_begin, count_itr_end, dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_gather(count_vector.begin(), count_vector.end(), dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and map is a randomAccessIterator */
            //bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), const_itr_begin, svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), const_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);          
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), const_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin,  count_itr_end, const_itr_begin, svOutVec.begin());
            bolt::amp::gather(count_itr_begin,  count_itr_end, const_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather( count_vector.begin(), count_vector.end(), const_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when the first input is a randomAccessIterator and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin, count_itr_end, svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(count_itr_begin, count_itr_end, dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather(count_vector.begin(), count_vector.end(), svIn1Vec.begin(),stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }


 TEST( TransformIterator, GatherUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2> svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        gen_input_udd genUDD;
        gen_input gen;
        /*Generate inputs*/
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        add3UDD_resultUDD add3;
        add_0 add0;

        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4; 

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>           tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>  tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3); 
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0), sv_trf_end2 (svIn2Vec.end(), add0);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0), dv_trf_end2 (dvIn2Vec.end(), add0);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int); 
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int);

        UDD2 t;
        t.i = (int)rand()%10;
        t.f = (float) rand();

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(t);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when both inputs are trf Iterators and UDD returns int*/
            //bolt::amp::gather(sv_trf_begin2, sv_trf_end2, tsv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(dv_trf_begin2, dv_trf_end2, tdv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(dv_trf_begin2, dv_trf_end2, tdv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::gather(sv_trf_begin2, sv_trf_end2, sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(dv_trf_begin2, dv_trf_end2, dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(dv_trf_begin2, dv_trf_end2, dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and map is a randomAccessIterator */
            //bolt::amp::gather(sv_trf_begin2, sv_trf_end2, svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(dv_trf_begin2, dv_trf_end2, dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(dv_trf_begin2, dv_trf_end2, svIn1Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and map is a trf_itr */
            //bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the both are randomAccessIterator */
            bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), svIn1Vec.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin, count_itr_end, sv_trf_begin1, svOutVec.begin());
            bolt::amp::gather(count_itr_begin, count_itr_end, dv_trf_begin1, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_gather(count_vector.begin(), count_vector.end(), dv_trf_begin1, stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and map is a randomAccessIterator */
            //bolt::amp::gather(svIn2Vec.begin(), svIn2Vec.end(), const_itr_begin, svOutVec.begin());
            bolt::amp::gather(dvIn2Vec.begin(), dvIn2Vec.end(), const_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);          
            Serial_gather(svIn2Vec.begin(), svIn2Vec.end(), const_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin,  count_itr_end, const_itr_begin, svOutVec.begin());
            bolt::amp::gather(count_itr_begin,  count_itr_end, const_itr_begin, dvOutVec.begin());
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather( count_vector.begin(), count_vector.end(), const_vector.begin(), stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

         {/*Test case when the first input is a randomAccessIterator and map is a counting iterator */
            //bolt::amp::gather(count_itr_begin, count_itr_end, svIn1Vec.begin(), svOutVec.begin());
            bolt::amp::gather(count_itr_begin, count_itr_end, dvIn1Vec.begin(), dvOutVec.begin());
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather(count_vector.begin(), count_vector.end(), svIn1Vec.begin(),stlOut.begin());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }



 TEST( TransformIterator, GatherIfRoutine)
 {
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length ); // input
        std::vector< int > svIn2Vec( length ); // map
        std::vector< int > svIn3Vec( length ); // stencil
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        for(int i=0; i<length; i++)
        {
            if(i%2 == 0)
                svIn3Vec[i] = 0;
            else
                svIn3Vec[i] = 1;
        }
        bolt::BCKND::device_vector< int > dvIn3Vec( svIn3Vec.begin(), svIn3Vec.end() );

        add_3 add3;
        add_0 add0;

        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;  
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add0;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add0; 

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3); // Input
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0), sv_trf_end2 (svIn2Vec.end(), add0); //Map
        //sv_trf_itr_add0 sv_trf_begin3 (svIn3Vec.begin(), add0); // Stencil
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0), dv_trf_end2 (dvIn2Vec.end(), add0);
        dv_trf_itr_add0 dv_trf_begin3 (dvIn3Vec.begin(), add0);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;
        constant_itr stencil_itr_begin(1);
        constant_itr stencil_itr_end = stencil_itr_begin + length;


        is_even iepred;

        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, sv_trf_begin3, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), sv_trf_begin3, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dv_trf_begin3, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), dv_trf_begin3, dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, sv_trf_begin3, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if( dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dv_trf_begin2, dv_trf_end2, dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, svIn3Vec.begin(), sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, dvIn3Vec.begin(), dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length), stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
                count_vector[index] = index;
        		stencil_vector[index] = 1;
        	}
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the both are randomAccessIterator */
            bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
       
        {/*Test case when the both are randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_itr_begin, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), stencil_itr_begin,  dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
            {
                stencil_vector[index] = 1;
            }
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_vector.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);          
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), const_vector.begin(),stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_itr_begin, const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), stencil_itr_begin, const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1); 
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        	}
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_vector.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::gather_if( count_itr_begin, count_itr_end, svIn3Vec.begin(), const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if( count_itr_begin, count_itr_end, dvIn3Vec.begin(), const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
                           
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::gather_if( count_itr_begin, count_itr_end, svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if( count_itr_begin, count_itr_end, dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }

 TEST( TransformIterator, GatherIfUDDRoutine)
 {
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length ); // input
        std::vector< int > svIn2Vec( length ); // map
        std::vector< int > svIn3Vec( length ); // stencil
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        for(int i=0; i<length; i++)
        {
            if(i%2 == 0)
                svIn3Vec[i] = 0;
            else
                svIn3Vec[i] = 1;
        }
        bolt::BCKND::device_vector< int > dvIn3Vec( svIn3Vec.begin(), svIn3Vec.end() );

        add3UDD_resultUDD add3;
        add_0 add0;


        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  const_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;  
        typedef bolt::BCKND::transform_iterator< add_0, std::vector< int >::const_iterator>            sv_trf_itr_add0;
        typedef bolt::BCKND::transform_iterator< add_0, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add0; 

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3); // Input
        //sv_trf_itr_add4 sv_trf_begin2 (svIn2Vec.begin(), add0), sv_trf_end2 (svIn2Vec.end(), add0); //Map
        //sv_trf_itr_add0 sv_trf_begin3 (svIn3Vec.begin(), add0); // Stencil
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3);
        dv_trf_itr_add4 dv_trf_begin2 (dvIn2Vec.begin(), add0), dv_trf_end2 (dvIn2Vec.end(), add0);
        dv_trf_itr_add0 dv_trf_begin3 (dvIn3Vec.begin(), add0);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int); // Input
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int);

        UDD2 t;
        t.i = (int)rand()%10;
        t.f = (float)rand();

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(t);
        constant_itr const_itr_end = const_itr_begin + length;
        const_itr stencil_itr_begin(1);
        const_itr stencil_itr_end = stencil_itr_begin + length;

        is_even iepred;

    
        {/*Test case when both inputs are trf Iterators with UDD returning int*/
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, sv_trf_begin3, tsv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, tdv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, tdv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when both inputs are trf Iterators*/
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, sv_trf_begin3, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), sv_trf_begin3, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dv_trf_begin3, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), dv_trf_begin3, dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, sv_trf_begin3, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if( dv_trf_begin2, dv_trf_end2, dv_trf_begin3, dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, dv_trf_begin3, svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator  and second is a trf_itr */
            //bolt::amp::gather_if(sv_trf_begin2, sv_trf_end2, svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dv_trf_begin2, dv_trf_end2, dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(dv_trf_begin2, dv_trf_end2, svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, svIn3Vec.begin(), sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, dvIn3Vec.begin(), dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is trf_itr and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, sv_trf_begin1, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, dv_trf_begin1, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length), stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
                count_vector[index] = index;
        		stencil_vector[index] = 1;
        	}
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), dv_trf_begin1, stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the both are randomAccessIterator */
            bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
       
        {/*Test case when the both are randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_itr_begin, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), stencil_itr_begin,  dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
            {
                stencil_vector[index] = 1;
            }
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_vector.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), dvIn3Vec.begin(), const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);          
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), svIn3Vec.begin(), const_vector.begin(),stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is constant iterator and the second is a randomAccessIterator */
            //bolt::amp::gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_itr_begin, const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(dvIn2Vec.begin(), dvIn2Vec.end(), stencil_itr_begin, const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t); 
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        	}
            Serial_gather_if(svIn2Vec.begin(), svIn2Vec.end(), stencil_vector.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::gather_if( count_itr_begin, count_itr_end, svIn3Vec.begin(), const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if( count_itr_begin, count_itr_end, dvIn3Vec.begin(), const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the first input is constant iterator and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, const_itr_begin, svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, const_itr_begin, dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,t);
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
                           
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), const_vector.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }


        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::gather_if( count_itr_begin, count_itr_end, svIn3Vec.begin(), svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if( count_itr_begin, count_itr_end, dvIn3Vec.begin(), dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;            
            Serial_gather_if(count_vector.begin(), count_vector.end(), svIn3Vec.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when the first input is a randomAccessIterator and the second is a counting iterator */
            //bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, svIn1Vec.begin(), svOutVec.begin(), iepred);
            bolt::amp::gather_if(count_itr_begin, count_itr_end, stencil_itr_begin, dvIn1Vec.begin(), dvOutVec.begin(), iepred);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
        	std::vector<int> stencil_vector(length);
            for (int index=0;index<length;index++)
        	{
        		stencil_vector[index] = 1;
        		count_vector[index] = index; 
        	}
            Serial_gather_if(count_vector.begin(), count_vector.end(), stencil_vector.begin(), svIn1Vec.begin(), stlOut.begin(), iepred);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

    }
 }

 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TEST(TransformIterator, InclusiveScanFloat)
//{
//    int length = 1<<10;
//    std::vector< float > refInput( length );
//   
//    for(int i=0; i<length; i++)
//    {
//      refInput[i] = 1.0f + rand( )%3;
//    }
//	bolt::amp::device_vector< float > input( refInput.begin( ), refInput.end( ) );
//    bolt::amp::device_vector< float > output( refInput.begin( ), refInput.end( ), true );
//
//    bolt::amp::plus< float > ai2;
//
//	bolt::amp::inclusive_scan( bolt::amp::make_transform_iterator( input.begin( ), square_root( ) ),
//                               bolt::amp::make_transform_iterator( input.end( ), square_root( ) ),
//                               output.begin( ), ai2 );
//
//    ::std::partial_sum( boost::make_transform_iterator( refInput.begin( ), square_root( ) ),
//                        boost::make_transform_iterator( refInput.end( ), square_root( ) ),
//                        refInput.begin( ), ai2 );
//
//    // compare results
//    cmpArrays(refInput, output);
//}


 TEST( TransformIterator, InclusiveScanRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        bolt::amp::plus<int> addI2;

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>          sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add4;   

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;
        

        {/*Test case when input is trf Iterator*/
            //bolt::amp::inclusive_scan(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), addI2);
            /*Compute expected results*/

            std::partial_sum(dv_trf_begin1, dv_trf_end1, stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is randomAccessIterator */
            bolt::amp::inclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(),  addI2);
            bolt::amp::inclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(),  addI2);
            /*Compute expected results*/
            std::partial_sum(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), addI2);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a constant iterator  */
            //bolt::amp::inclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), addI2);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::partial_sum(const_vector.begin(), const_vector.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a counting iterator */
            //bolt::amp::inclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), addI2);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::partial_sum(count_vector.begin(), count_vector.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}

TEST( TransformIterator, UDDInclusiveScanRoutine)
{  
        const int length = 10;
        std::vector< UDD2 > svIn1Vec( length);
        std::vector< UDD2 > svOutVec( length );
        std::vector< int > tsvOutVec( length );
        std::vector< UDD2 > stlOut( length );
        std::vector< int > tstlOut( length );
        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );
        bolt::BCKND::device_vector< int > tdvOutVec( length );

        UDDadd_3 add3;
        bolt::amp::plus<UDD2> addI2;

        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=0, init.f=0.0f;

        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< UDDadd_3, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< UDDadd_3, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;
          
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);


        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;
        

        {/*Test case when input is trf Iterator and UDD is returning an int*/
            bolt::amp::plus<int> addI2_int;
            //bolt::amp::inclusive_scan(tsv_trf_begin1, tsv_trf_end1, tsvOutVec.begin(), addI2_int);
            bolt::amp::inclusive_scan(tdv_trf_begin1, tdv_trf_end1, tdvOutVec.begin(), addI2_int);
            /*Compute expected results*/
			std::partial_sum( tdv_trf_begin1, tdv_trf_end1, tstlOut.begin(), addI2_int);
            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }

        {/*Test case when input is trf Iterator*/
            //bolt::amp::inclusive_scan(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), addI2);
            /*Compute expected results*/
            std::partial_sum(dv_trf_begin1, dv_trf_end1, stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is randomAccessIterator */
            bolt::amp::inclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(),  addI2);
            bolt::amp::inclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(),  addI2);
            /*Compute expected results*/
            std::partial_sum(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), addI2);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a constant iterator  */
            //bolt::amp::inclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), addI2);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::partial_sum(const_vector.begin(), const_vector.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a counting iterator */
            //bolt::amp::inclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), addI2);
            bolt::amp::inclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), addI2);

            /*Compute expected results*/			
            std::vector<UDD2> count_vector(count_itr_begin, count_itr_end);
            std::partial_sum(count_vector.begin(), count_vector.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan by Key tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



//TEST( TransformIterator, ScanByKey )
//{
//    int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
//    int vals[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 }; 
//    int out[11]; 
//   
//    bolt::amp::equal_to<int> eq; 
//    bolt::amp::multiplies<int> mult; 
//    bolt::amp::device_vector<int> dkeys ( keys, keys + 11 );
//    bolt::amp::device_vector<int> dvals ( vals, vals + 11 );
//    bolt::amp::device_vector<int> dout ( vals, vals + 11, true );
//   
//    bolt::amp::inclusive_scan_by_key( bolt::amp::make_transform_iterator( dkeys.begin( ), my_negate( ) ),
//                                      bolt::amp::make_transform_iterator( dkeys.end( ), my_negate( ) ),
//                                      dvals.begin( ), out, eq, mult ); 
//   
//    int arrToMatch[11] = { 2, 2, 4, 2, 4, 8, 2, 4, 8, 16, 2 };
//
//    // compare results
//    cmpArrays<int,11>( arrToMatch, out );
//}

TEST( TransformIterator, InclusiveScanbykeyRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svIn2Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);
        /*Create Iterators*/
        int segmentLength = 0;
        int segmentIndex = 0;
        std::vector<int> key(1);
        key[0] = 0;

        for (int i = 0; i < length; i++)
        {
            // start over, i.e., begin assigning new key
            if (segmentIndex == segmentLength)
            {
                segmentLength++;
                segmentIndex = 0;
                key[0] = key[0]+1 ; 
            }
            svIn1Vec[i] = key[0];
            segmentIndex++;
        }

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;
        bolt::amp::equal_to<int> equal_to;
        bolt::amp::plus<int> addI2;


        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>          sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add4;    


        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add3 sv_trf_begin2 (svIn2Vec.begin(), add3), sv_trf_end2 (svIn2Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin2 (dvIn2Vec.begin(), add3), dv_trf_end2 (dvIn2Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        std::vector<int> input1(dv_trf_begin1, dv_trf_end1);
        std::vector<int> input2(dv_trf_begin2, dv_trf_end2);

        {/*Test case when inputs are trf Iterator*/
            //bolt::amp::inclusive_scan_by_key(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin(), equal_to, addI2);

            /*Compute expected results*/
            Serial_inclusive_scan_by_key<int, int, int,  bolt::amp::plus< int >>(&input1[0], length, &input2[0], &stlOut[0], addI2 );

            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when inputs are randomAccessIterator */
            bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            Serial_inclusive_scan_by_key<int, int, int,  bolt::amp::plus< int >>(&svIn1Vec[0], length, &svIn2Vec[0], &stlOut[0], addI2 );
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when value input is a constant iterator while key input is stil randomAccessIterator  */
            //bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), const_itr_begin, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), const_itr_begin, dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            Serial_inclusive_scan_by_key<int, int, int,  bolt::amp::plus< int >>(&svIn1Vec[0], length, &const_vector[0], &stlOut[0], addI2 );
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a counting iterator */
            //bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            Serial_inclusive_scan_by_key<int, int, int,  bolt::amp::plus< int >>(&svIn1Vec[0], length, &count_vector[0], &stlOut[0], addI2 );
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}


TEST( TransformIterator, UDDInclusiveScanbykeyRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svIn2Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< int > svOutVec_int( length );
        std::vector< UDD2 > stlOut( length );
        std::vector< int > stlOut_int( length );

        /*Generate inputs*/
        gen_input_udd gen;
        std::generate(svIn2Vec.begin(), svIn2Vec.end(), gen);

        /*Create Iterators*/
        UDD2 Zero;
        Zero.i=0, Zero.f=0.0f; 

        int segmentLength = 0;
        int segmentIndex = 0;
        std::vector<UDD2> key(1);
        key[0] = Zero;

        for (int i = 0; i < length; i++)
        {
                 // start over, i.e., begin assigning new key
                 if (segmentIndex == segmentLength)
                 {
                          segmentLength++;
                          segmentIndex = 0;
                          key[0] = key[0]+1 ; 
                 }
                 svIn1Vec[i] = key[0];
                 segmentIndex++;
        }

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvIn2Vec( svIn2Vec.begin(), svIn2Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );
        bolt::BCKND::device_vector< int > dvOutVec_int( length );

        UDDadd_3 add3;
        bolt::amp::equal_to<UDD2> equal_to;
        bolt::amp::plus<UDD2> addI2;


        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< UDDadd_3, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< UDDadd_3, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;

        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

 
 
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        //sv_trf_itr_add3 sv_trf_begin2 (svIn2Vec.begin(), add3), sv_trf_end2 (svIn2Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin2 (dvIn2Vec.begin(), add3), dv_trf_end2 (dvIn2Vec.end(), add3);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
        //tsv_trf_itr_add3 tsv_trf_begin2 (svIn2Vec.begin(), sq_int), tsv_trf_end2 (svIn2Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin2 (dvIn2Vec.begin(), sq_int), tdv_trf_end2 (dvIn2Vec.end(), sq_int);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 init;
        init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        std::vector<int> tinput1(tdv_trf_begin1, tdv_trf_end1);
        std::vector<int> tinput2(tdv_trf_begin2, tdv_trf_end2);

        std::vector<UDD2> input1(dv_trf_begin1, dv_trf_end1);
        std::vector<UDD2> input2(dv_trf_begin2, dv_trf_end2);

        {/*Test case when inputs are trf Iterators and return type of UDD is int*/
            bolt::amp::equal_to<int> equal_to_int;
            bolt::amp::plus<int> addI2_int;
            //bolt::amp::inclusive_scan_by_key(tsv_trf_begin1, tsv_trf_end1, tsv_trf_begin2, svOutVec_int.begin(), equal_to_int, addI2_int);
            bolt::amp::inclusive_scan_by_key(tdv_trf_begin1, tdv_trf_end1, tdv_trf_begin2, dvOutVec_int.begin(), equal_to_int, addI2_int);
            /*Compute expected results*/

            Serial_inclusive_scan_by_key<int, int, int,  bolt::amp::plus< int >>(&tinput1[0], length, &tinput2[0], &stlOut_int[0], addI2_int );
            /*Check the results*/
            //cmpArrays(stlOut_int, svOutVec_int);
            cmpArrays(stlOut_int, dvOutVec_int);
        }
        {/*Test case when inputs are trf Iterator*/
            //bolt::amp::inclusive_scan_by_key(sv_trf_begin1, sv_trf_end1, sv_trf_begin2, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dv_trf_begin1, dv_trf_end1, dv_trf_begin2, dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            Serial_inclusive_scan_by_key<UDD2, UDD2, UDD2,  bolt::amp::plus< UDD2 >>(&input1[0], length, &input2[0], &stlOut[0], addI2 );
            /*Check the results*/
            //cmpArrays(stlOut, svOutVec);
            cmpArrays(stlOut, dvOutVec);
        }
        {/*Test case when inputs are randomAccessIterator */
            bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), svIn2Vec.begin(), svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), dvIn2Vec.begin(), dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            Serial_inclusive_scan_by_key<UDD2, UDD2, UDD2,  bolt::amp::plus< UDD2 >>(&svIn1Vec[0], length, &svIn2Vec[0], &stlOut[0], addI2 );
            /*Check the results*/
            cmpArrays(stlOut, svOutVec);
            cmpArrays(stlOut, dvOutVec);
        }

        {/*Test case when value input is a constant iterator while key input is stil randomAccessIterator  */
           // bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), const_itr_begin, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), const_itr_begin, dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(const_itr_begin, const_itr_end);
            Serial_inclusive_scan_by_key<UDD2, UDD2, UDD2,  bolt::amp::plus< UDD2 >>(&svIn1Vec[0], length, &const_vector[0], &stlOut[0], addI2 );                
            /*Check the results*/
            //cmpArrays(stlOut, svOutVec);
            cmpArrays(stlOut, dvOutVec);
        }
        {/*Test case when input is a counting iterator */
            //bolt::amp::inclusive_scan_by_key(svIn1Vec.begin(), svIn1Vec.end(), count_itr_begin, svOutVec.begin(), equal_to, addI2);
            bolt::amp::inclusive_scan_by_key(dvIn1Vec.begin(), dvIn1Vec.end(), count_itr_begin, dvOutVec.begin(), equal_to, addI2);
            /*Compute expected results*/
            std::vector<UDD2> count_vector(count_itr_begin , count_itr_end);
            Serial_inclusive_scan_by_key<UDD2, UDD2, UDD2,  bolt::amp::plus< UDD2 >>(&svIn1Vec[0], length, &count_vector[0], &stlOut[0], addI2 );      
            /*Check the results*/
            //cmpArrays(stlOut, svOutVec);
            cmpArrays(stlOut, dvOutVec);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TransformScan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct uddtI2
{
    int a;
    int b;

    bool operator==(const uddtI2& rhs) const  restrict(cpu, amp)
    {
        bool equal = true;
        equal = ( a == rhs.a ) ? equal : false;
        equal = ( b == rhs.b ) ? equal : false;
        return equal;
    }
    uddtI2 operator-() const  restrict(cpu, amp)
    {
        uddtI2 r;
        r.a = -a;
        r.b = -b;
        return r;
    }
    uddtI2 operator*(const uddtI2& rhs)  restrict(cpu, amp)
    {
        uddtI2 r;
        r.a = a*a;
        r.b = b*b;
        return r;
    }
    typedef uddtI2 result_type;
};

struct AddI2
{
    uddtI2 operator()(const uddtI2 &lhs, const uddtI2 &rhs) const  restrict(cpu, amp)
    {
        uddtI2 _result;
        _result.a = lhs.a+rhs.a;
        _result.b = lhs.b+rhs.b;
        return _result;
    };
    typedef uddtI2 result_type;
};

uddtI2 identityAddI2 = {  0, 0 };
uddtI2 initialAddI2  = { -1, 2 };


 struct NegateI2
 {
     uddtI2 operator()(const uddtI2& rhs) const restrict(cpu, amp)
     {
         uddtI2 ret;
         ret.a = -rhs.a;
         ret.b = -rhs.b;
         return ret;
     }
    typedef uddtI2 result_type;
 };
NegateI2 nI2;

struct SquareI2
{
    uddtI2 operator()(const uddtI2& rhs) const  restrict(cpu, amp)
    {
        uddtI2 ret;
        ret.a = rhs.a*rhs.a;
        ret.b = rhs.b*rhs.b;
        return ret;
    }
    typedef uddtI2 result_type;
};

SquareI2 sI2;

TEST( TransformIterator, TransformScanUDD )
{
    //setup containers
    int length = (1<<16)+23;
    bolt::amp::device_vector< uddtI2 > input(  length, initialAddI2);
    std::vector< uddtI2 > refInput( length, initialAddI2 );

    // call transform_scan
    AddI2 aI2;
    bolt::amp::transform_inclusive_scan( bolt::amp::make_transform_iterator( input.begin( ), NegateI2( ) ),
                                         bolt::amp::make_transform_iterator( input.end( ), NegateI2( ) ),
                                         input.begin( ), nI2, aI2 );

    ::std::transform( boost::make_transform_iterator( refInput.begin( ), NegateI2( ) ),
                      boost::make_transform_iterator( refInput.end( ), NegateI2( ) ),
                      refInput.begin( ), nI2); // transform in-place
    ::std::partial_sum( refInput.begin(), refInput.end(), refInput.begin(), aI2); // in-place scan

    // compare results
    cmpArrays(refInput, input);
}

TEST( TransformIterator, InclusiveTransformScanRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;

        bolt::amp::negate<int> nI2;
        bolt::amp::plus<int> addI2;

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>          sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator> dv_trf_itr_add4;    
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is trf Iterator*/
            //bolt::amp::transform_inclusive_scan(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut, length);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a randomAccessIterator */
            bolt::amp::transform_inclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform_inclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a counting iterator */
            //bolt::amp::transform_inclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::transform(count_vector.begin(), count_vector.end(), stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        
    }
}

TEST( TransformIterator, InclusiveTransformScanUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        bolt::amp::negate<UDD2> nI2;
        bolt::amp::plus<UDD2> addI2;

        add3UDD_resultUDD sqUDD;
        squareUDD_result_int sq_int;

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;
        UDD2 t;
        t.i=0, t.f=0.0f;

        counting_itr count_itr_begin(t);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;


        {/*Test case when input is trf Iterator and return type of UDD is int*/
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;
            //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
            tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
            bolt::BCKND::device_vector< int > tdvOutVec( length );
            std::vector< int > tstlOut( length );
            std::vector< int > tsvOutVec( length );

            bolt::amp::negate<int> nI2_int;
            bolt::amp::plus<int> addI2_int;

            //bolt::amp::transform_inclusive_scan(tsv_trf_begin1, tsv_trf_end1, tsvOutVec.begin(), nI2_int, addI2_int);
            bolt::amp::transform_inclusive_scan(tdv_trf_begin1, tdv_trf_end1, tdvOutVec.begin(), nI2_int, addI2_int);
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, tstlOut.begin(), nI2_int);
            std::partial_sum(tstlOut.begin(), tstlOut.end(), tstlOut.begin(), addI2_int);
            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }

        {/*Test case when input is trf Iterator*/
            //bolt::amp::transform_inclusive_scan(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is randomAccessIterator */
            bolt::amp::transform_inclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
		}

        {/*Test case when the input is constant iterator */
            //bolt::amp::transform_inclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(const_itr_begin, const_itr_end);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        
        {/*Test case when the input is a counting iterator */
            //bolt::amp::transform_inclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), nI2, addI2);
            bolt::amp::transform_inclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), nI2, addI2);
            /*Compute expected results*/  
            std::transform(count_itr_begin, count_itr_end, stlOut.begin(), nI2);
            std::partial_sum(stlOut.begin(), stlOut.end(), stlOut.begin(), addI2);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}

TEST( TransformIterator, ExclusiveTransformScanRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );

        /*Generate inputs*/
        gen_input gen;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);

        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< int > dvOutVec( length );

        add_3 add3;

        bolt::amp::negate<int> nI2;
        bolt::amp::plus<int> addI2;
        int n = (int) 1 + rand()%10;


        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< int >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< int >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::BCKND::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4; 

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;


        {/*Test case when inputs are trf Iterators*/
            //bolt::amp::transform_exclusive_scan(sv_trf_begin1, sv_trf_end1, svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(dv_trf_begin1, dv_trf_end1, dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, stlOut.begin(), nI2);
            Serial_scan<int,  bolt::amp::plus< int >, int>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the both are randomAccessIterator */
            bolt::amp::transform_exclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), nI2);
            Serial_scan<int,  bolt::amp::plus< int >, int>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the input is constant iterator  */
            //bolt::amp::transform_exclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), nI2);
            Serial_scan<int,  bolt::amp::plus< int >, int>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when the  input is a counting iterator */
            //bolt::amp::transform_exclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::transform(count_vector.begin(), count_vector.end(), stlOut.begin(), nI2);
            Serial_scan<int,  bolt::amp::plus< int >, int>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
       
    }
}

TEST( TransformIterator, ExclusiveTransformScanUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        std::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);


        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
        bolt::BCKND::device_vector< UDD2 > dvOutVec( length );

        bolt::amp::negate<UDD2> nI2;
        bolt::amp::plus<UDD2> addI2;

        add3UDD_resultUDD sqUDD;


        squareUDD_result_int sq_int;
        UDD2 n;
        n.i = (int) 1 + rand()%10;
        n.f = (float) rand();

        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                          dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                constant_itr;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, std::vector< UDD2 >::const_iterator>          sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< add3UDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator> dv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;

    
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

        //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
        tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);

        UDD2 temp;
        temp.i=1, temp.f=2.5f;

        UDD2 t;
        t.i=0, t.f=0.0f;

        counting_itr count_itr_begin(t);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is trf Iterator and return type of UDD is int*/
            int nint = rand()%10;            
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>          tsv_trf_itr_add3;
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator> tdv_trf_itr_add3;
            //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
            tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
            bolt::BCKND::device_vector< int > tdvOutVec( length );
            std::vector< int > tstlOut( length );
            std::vector< int > tsvOutVec( length );

            bolt::amp::negate<int> nI2_int;
            bolt::amp::plus<int> addI2_int;

            //bolt::amp::transform_exclusive_scan(tsv_trf_begin1, tsv_trf_end1, tsvOutVec.begin(), nI2_int, nint, addI2_int);
            bolt::amp::transform_exclusive_scan(tdv_trf_begin1, tdv_trf_end1, tdvOutVec.begin(), nI2_int, nint, addI2_int);
            /*Compute expected results*/
            std::transform(tdv_trf_begin1, tdv_trf_end1, tstlOut.begin(), nI2_int);
            Serial_scan<int,  bolt::amp::plus< int >, int>(&tstlOut[0], &tstlOut[0], length, addI2_int, false, nint);
            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(tdvOutVec, tstlOut);
        }


        {/*Test case when input is a randomAccessIterator */
            bolt::amp::transform_exclusive_scan(svIn1Vec.begin(), svIn1Vec.end(), svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(dvIn1Vec.begin(), dvIn1Vec.end(), dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::transform(svIn1Vec.begin(), svIn1Vec.end(), stlOut.begin(), nI2);
            Serial_scan<UDD2,  bolt::amp::plus< UDD2 >, UDD2>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when first input is a constant iterator */
            //bolt::amp::transform_exclusive_scan(const_itr_begin, const_itr_end, svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(const_itr_begin, const_itr_end, dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::transform(const_vector.begin(), const_vector.end(), stlOut.begin(), nI2);
            Serial_scan<UDD2,  bolt::amp::plus< UDD2 >, UDD2>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }	
        {/*Test case when the input is a counting iterator */
            //bolt::amp::transform_exclusive_scan(count_itr_begin, count_itr_end, svOutVec.begin(), nI2, n, addI2);
            bolt::amp::transform_exclusive_scan(count_itr_begin, count_itr_end, dvOutVec.begin(), nI2, n, addI2);
            /*Compute expected results*/
            std::transform(count_itr_begin,  count_itr_end, stlOut.begin(), nI2);
            Serial_scan<UDD2,  bolt::amp::plus< UDD2 >, UDD2>(&stlOut[0], &stlOut[0], length, addI2, false, n);
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(dvOutVec, stlOut);
        }
    }
}


TEST( TransformIterator, ForEachRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );

        /*Generate inputs*/
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

		add_3 add3;

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;

        typedef bolt::amp::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::amp::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;   

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

		
        {/*Test case when inputs are trf Iterators*/
		    std::vector<int> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each(sv_trf_begin1, sv_trf_end1, bolt::amp::negate<int>());
            bolt::amp::for_each(dv_trf_begin1, dv_trf_end1, bolt::amp::negate<int>());
			std::vector<int> bolt_output(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.end(), std::negate<int>() );
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(bolt_output, std_input);
        }
        {/*Test case when input is randomAccessIterator */
			std::vector<int> std_input(dvIn1Vec.begin(), dvIn1Vec.end());
            bolt::amp::for_each(svIn1Vec.begin(), svIn1Vec.end(), bolt::amp::negate<int>());
            bolt::amp::for_each(dvIn1Vec.begin(), dvIn1Vec.end(), bolt::amp::negate<int>());
            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.end(), std::negate<int>());
            /*Check the results*/
            cmpArrays(svIn1Vec, std_input);
            cmpArrays(dvIn1Vec, std_input);
        }
        {/*Test case when the input is constant iterator  */
            bolt::amp::for_each(const_itr_begin, const_itr_end,  bolt::amp::negate<int>());
			std::vector<int> bolt_out(const_itr_begin, const_itr_end);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::for_each(const_vector.begin(), const_vector.end(), std::negate<int>());
            /*Check the results*/
            cmpArrays(bolt_out, const_vector);
        }

        {/*Test case when the input is a counting iterator */
            bolt::amp::for_each(count_itr_begin, count_itr_end, bolt::amp::negate<int>());
			std::vector<int> bolt_out(count_itr_begin, count_itr_end);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::for_each(count_vector.begin(), count_vector.end(), std::negate<int>());
            /*Check the results*/
            cmpArrays(bolt_out, count_vector);
        }
    }
}


TEST( TransformIterator, ForEachUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
	

        squareUDD_resultUDD sqUDD;
		squareUDD_result_int sq_int;

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
     
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

		UDD2 temp;
		temp.i=1, temp.f=2.5f;

		UDD2 init;
		init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

		{/*Test case when input is trf Iterator and UDD is returning int*/
		    typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;

		    //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
            tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
			std::vector<int> temp_vec(tdv_trf_begin1, tdv_trf_end1);

            //bolt::amp::for_each(tsv_trf_begin1, tsv_trf_end1, bolt::amp::negate<int>());

			std::vector<UDD2> std_input(tdv_trf_begin1, tdv_trf_end1);
            bolt::amp::for_each(tdv_trf_begin1, tdv_trf_end1, bolt::amp::negate<int>());
			std::vector<UDD2> bolt_out(tdv_trf_begin1, tdv_trf_end1);

            /*Compute expected results*/
            std::for_each(temp_vec.begin(), temp_vec.end(), bolt::amp::negate<int>());

            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(bolt_out, std_input);
        }

        {/*Test case when input is trf Iterator*/
			std::vector<UDD2> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each(sv_trf_begin1, sv_trf_end1, bolt::amp::negate<UDD2>());
            bolt::amp::for_each(dv_trf_begin1, dv_trf_end1, bolt::amp::negate<UDD2>());
			std::vector<UDD2> temp1_vec(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.end(), std::negate<UDD2>());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(temp1_vec, std_input);
        }


        {/*Test case when the input is randomAccessIterator */
			std::vector<UDD2> std_input(svIn1Vec.begin(), svIn1Vec.end());
            bolt::amp::for_each(svIn1Vec.begin(), svIn1Vec.end(), bolt::amp::negate<UDD2>());
            bolt::amp::for_each(dvIn1Vec.begin(), dvIn1Vec.end(), bolt::amp::negate<UDD2>());
            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.end(), std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(svIn1Vec, std_input);
            cmpArrays(dvIn1Vec, std_input);
        }
        {/*Test case when the input is constant iterator  */
            bolt::amp::for_each(const_itr_begin, const_itr_end, bolt::amp::negate<UDD2>());
			std::vector<UDD2> bolt_out(const_itr_begin, const_itr_end);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::for_each(const_vector.begin(), const_vector.end(), std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(bolt_out, const_vector);
        }	

        {/*Test case when the input is a counting iterator */
			std::vector<UDD2> std_out(count_itr_begin, count_itr_end);
            bolt::amp::for_each(count_itr_begin, count_itr_end, bolt::amp::negate<UDD2>());
			std::vector<UDD2> bolt_out(count_itr_begin, count_itr_end);
            /*Compute expected results*/
            std::for_each(std_out.begin(), std_out.end(), std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(bolt_out, std_out);
        }
    }
}

TEST( TransformIterator, ForEach_n_Routine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );

		int n = rand()%length;

        /*Generate inputs*/
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::BCKND::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

		add_3 add3;

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;

        typedef bolt::amp::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_3, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::amp::transform_iterator< add_4, bolt::BCKND::device_vector< int >::iterator>   dv_trf_itr_add4;   

        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), add3), sv_trf_end1 (svIn1Vec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), add3), dv_trf_end1 (dvIn1Vec.end(), add3);

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

		
        {/*Test case when inputs are trf Iterators*/
		    std::vector<int> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each(sv_trf_begin1, sv_trf_end1, bolt::amp::negate<int>());
            bolt::amp::for_each_n(dv_trf_begin1, n, bolt::amp::negate<int>());
			std::vector<int> bolt_output(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.begin() + n, std::negate<int>() );
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(bolt_output, std_input);
        }
        {/*Test case when input is randomAccessIterator */
			std::vector<int> std_input(dvIn1Vec.begin(), dvIn1Vec.end());
            bolt::amp::for_each_n(svIn1Vec.begin(), n, bolt::amp::negate<int>());
            bolt::amp::for_each_n(dvIn1Vec.begin(), n, bolt::amp::negate<int>());
            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.begin() + n, std::negate<int>());
            /*Check the results*/
            cmpArrays(svIn1Vec, std_input);
            cmpArrays(dvIn1Vec, std_input);
        }

        {/*Test case when the input is constant iterator  */
            bolt::amp::for_each_n(const_itr_begin, n,  bolt::amp::negate<int>());
			std::vector<int> bolt_out(const_itr_begin, const_itr_end);
            /*Compute expected results*/
            std::vector<int> const_vector(length,1);
            std::for_each(const_vector.begin(), const_vector.begin() + n, std::negate<int>());
            /*Check the results*/
            cmpArrays(bolt_out, const_vector);
        }

        {/*Test case when the input is a counting iterator */
            bolt::amp::for_each_n(count_itr_begin, n, bolt::amp::negate<int>());
			std::vector<int> bolt_out(count_itr_begin, count_itr_end);
            /*Compute expected results*/
            std::vector<int> count_vector(length);
            for (int index=0;index<length;index++)
                count_vector[index] = index;
            std::for_each(count_vector.begin(), count_vector.begin() + n, std::negate<int>());
            /*Check the results*/
            cmpArrays(bolt_out, count_vector);
        }
    }
}


TEST( TransformIterator, ForEach_n_UDDRoutine)
{
    {
        const int length = 1<<10;

		int n = rand()%length;

        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );

        /*Generate inputs*/
        gen_input_udd genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::BCKND::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );
	

        squareUDD_resultUDD sqUDD;
		squareUDD_result_int sq_int;

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::BCKND::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::BCKND::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::BCKND::constant_iterator< UDD2 >                                                  constant_itr;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, std::vector< UDD2 >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::BCKND::transform_iterator< squareUDD_resultUDD, bolt::BCKND::device_vector< UDD2 >::iterator>   dv_trf_itr_add3;
     
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIn1Vec.begin(), sqUDD), sv_trf_end1 (svIn1Vec.end(), sqUDD);
        dv_trf_itr_add3 dv_trf_begin1 (dvIn1Vec.begin(), sqUDD), dv_trf_end1 (dvIn1Vec.end(), sqUDD);

		UDD2 temp;
		temp.i=1, temp.f=2.5f;

		UDD2 init;
		init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

		{/*Test case when input is trf Iterator and UDD is returning int*/
		    typedef bolt::BCKND::transform_iterator< squareUDD_result_int, std::vector< UDD2 >::const_iterator>            tsv_trf_itr_add3;
            typedef bolt::BCKND::transform_iterator< squareUDD_result_int, bolt::BCKND::device_vector< UDD2 >::iterator>   tdv_trf_itr_add3;

		    //tsv_trf_itr_add3 tsv_trf_begin1 (svIn1Vec.begin(), sq_int), tsv_trf_end1 (svIn1Vec.end(), sq_int);
            tdv_trf_itr_add3 tdv_trf_begin1 (dvIn1Vec.begin(), sq_int), tdv_trf_end1 (dvIn1Vec.end(), sq_int);
			std::vector<int> temp_vec(tdv_trf_begin1, tdv_trf_end1);

            //bolt::amp::for_each(tsv_trf_begin1, tsv_trf_end1, bolt::amp::negate<int>());

			std::vector<UDD2> std_input(tdv_trf_begin1, tdv_trf_end1);
            bolt::amp::for_each_n(tdv_trf_begin1, n, bolt::amp::negate<int>());
			std::vector<UDD2> bolt_out(tdv_trf_begin1, tdv_trf_end1);

            /*Compute expected results*/
            std::for_each(temp_vec.begin(), temp_vec.begin() + n, bolt::amp::negate<int>());

            /*Check the results*/
            //cmpArrays(tsvOutVec, tstlOut);
            cmpArrays(bolt_out, std_input);
        }

        {/*Test case when input is trf Iterator*/
			std::vector<UDD2> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each_n(sv_trf_begin1, n, bolt::amp::negate<UDD2>());
            bolt::amp::for_each_n(dv_trf_begin1, n, bolt::amp::negate<UDD2>());
			std::vector<UDD2> temp1_vec(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.begin() + n, std::negate<UDD2>());
            /*Check the results*/
            //cmpArrays(svOutVec, stlOut);
            cmpArrays(temp1_vec, std_input);
        }


        {/*Test case when the input is randomAccessIterator */
			std::vector<UDD2> std_input(svIn1Vec.begin(), svIn1Vec.end());
            bolt::amp::for_each_n(svIn1Vec.begin(), n, bolt::amp::negate<UDD2>());
            bolt::amp::for_each_n(dvIn1Vec.begin(), n, bolt::amp::negate<UDD2>());
            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.begin() + n, std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(svIn1Vec, std_input);
            cmpArrays(dvIn1Vec, std_input);
        }

        {/*Test case when the input is constant iterator  */
            bolt::amp::for_each_n(const_itr_begin, n, bolt::amp::negate<UDD2>());
			std::vector<UDD2> bolt_out(const_itr_begin, const_itr_end);
            /*Compute expected results*/
            std::vector<UDD2> const_vector(length,temp);
            std::for_each(const_vector.begin(), const_vector.begin() + n, std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(bolt_out, const_vector);
        }	
        {/*Test case when the input is a counting iterator */
			std::vector<UDD2> std_out(count_itr_begin, count_itr_end);
            bolt::amp::for_each_n(count_itr_begin, n, bolt::amp::negate<UDD2>());
			std::vector<UDD2> bolt_out(count_itr_begin, count_itr_end);
            /*Compute expected results*/
            std::for_each(std_out.begin(), std_out.begin() + n, std::negate<UDD2>());
            /*Check the results*/
            cmpArrays(bolt_out, std_out);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Misc tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(TransformIterator, ti)
{

    typedef int etype;
    etype elements[WAVEFRNT_SIZE];
    etype empty[WAVEFRNT_SIZE];

    size_t view_size = WAVEFRNT_SIZE;

    std::iota(elements, elements+WAVEFRNT_SIZE, 1000);
    std::fill(empty, empty+WAVEFRNT_SIZE, 0);

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + WAVEFRNT_SIZE);
    bolt::amp::device_vector<int, concurrency::array_view> dumpCheckV(empty, empty + WAVEFRNT_SIZE);

    bolt::amp::transform( dumpCheckV.begin( ),
                          dumpCheckV.end( ),
                          dumpCheckV.begin( ),
                          bolt::amp::negate<int>( ) );

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator( ).default_view;

    typedef bolt::amp::transform_iterator<my_negate, bolt::amp::device_vector<int>::iterator> transf_iter;

    transf_iter tbegin(dvebegin, my_negate( ));
    transf_iter tend(dveend, my_negate( ));

    auto dumpAV = dumpV.begin( ).getContainer( ).getBuffer( );

    concurrency::extent< 1 > inputExtent( WAVEFRNT_SIZE );
    concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
    {
      int gidx = idx[0];

      dumpAV[gidx] = tbegin[gidx];
       
    });
    dumpAV.synchronize( );

    cmpArrays(dumpCheckV,dumpV);

}


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );

    int retVal = RUN_ALL_TESTS( ); //choose AMP

    myControl.setForceRunMode( bolt::amp::control::MultiCoreCpu );  // choose tbb
    retVal = RUN_ALL_TESTS( );

	myControl.setForceRunMode( bolt::amp::control::SerialCpu );  // choose serial
    retVal = RUN_ALL_TESTS( );

    
    //  Reflection code to inspect how many tests failed in gTest
    ::testing::UnitTest& unitTest = *::testing::UnitTest::GetInstance( );

    unsigned int failedTests = 0;
    for( int i = 0; i < unitTest.total_test_case_count( ); ++i )
    {
        const ::testing::TestCase& testCase = *unitTest.GetTestCase( i );
        for( int j = 0; j < testCase.total_test_count( ); ++j )
        {
            const ::testing::TestInfo& testInfo = *testCase.GetTestInfo( j );
            if( testInfo.result( )->Failed( ) )
                ++failedTests;
        }
    }

    //  Print helpful message at termination if we detect errors, to help users figure out what to do next
    if( failedTests )
    {
        bolt::tout << _T( "\nFailed tests detected in test pass; please run test again with:" ) << std::endl;
        bolt::tout << _T( "\t--gtest_filter=<XXX> to select a specific failing test of interest" ) << std::endl;
        bolt::tout << _T( "\t--gtest_catch_exceptions=0 to generate minidump of failing test, or" ) << std::endl;      
        bolt::tout << _T( "\t--gtest_break_on_failure to debug interactively with debugger" ) << std::endl;
        bolt::tout << _T( "\t    (only on googletest assertion failures, not SEH exceptions)" ) << std::endl;
    }

    return retVal;


}
