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

/***************************************************************************

 * Copyright (C) 2004 Jeremy Siek <jsiek@cs.indiana.edu>
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)

***************************************************************************/

#include "common/stdafx.h"

#include "bolt/amp/for_each.h"
#include "bolt/amp/generate.h"
#include "bolt/amp/fill.h"
#include "bolt/amp/copy.h"
#include "bolt/amp/count.h"
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

#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>
#include "bolt/amp/functional.h"
#include "common/test_common.h"
#include "bolt/amp/iterator/constant_iterator.h"
#include "bolt/amp/iterator/counting_iterator.h"
#include "bolt/amp/iterator/permutation_iterator.h"
#include "bolt/amp/iterator/transform_iterator.h"
#include <boost/range/algorithm/transform.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/zip_iterator.hpp>

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

struct gen_input
 {

    int operator() () const restrict (cpu,amp) 
	{ 
		return 10; 
	}

    typedef int result_type;
 };

//struct UDD2
//{
//    int i;
//    float f;
//  
//    bool operator == (const UDD2& other) const restrict(cpu, amp)
//	{
//        return ((i == other.i) && (f == other.f));
//    }
//
//	bool operator >= (const UDD2& other) const restrict(cpu, amp)
//	{
//        return ((i >= other.i) && (f >= other.f));
//    }
//
//	bool operator <= (const UDD2& other) const restrict(cpu, amp)
//	{
//        return ((i <= other.i) && (f <= other.f));
//    }
//
//	bool operator != (const UDD2& other) const restrict(cpu, amp)
//	{
//        return ((i != other.i) || (f != other.f));
//    }
//
//	UDD2 operator ++ () restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = i + 1;
//      _result.f = f + 1.0f;
//      return _result;
//    }
//
//	UDD2 operator = (const int rhs) const restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = i + rhs;
//      _result.f = f + (float)rhs;
//      return _result;
//    }
//
//	UDD2 operator = (const UDD2 &rhs) const restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = i + rhs.i;
//      _result.f = f + rhs.f;
//      return _result;
//    }
//
//	UDD2 operator + (const UDD2 &rhs) const restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = this->i + rhs.i;
//      _result.f = this->f + rhs.f;
//      return _result;
//    }
//
//	UDD2 operator * (const UDD2 &rhs) const restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = this->i * rhs.i;
//      _result.f = this->f * rhs.f;
//      return _result;
//    }
//
//	UDD2 operator + (const int rhs)  restrict(cpu, amp)
//    {
//      UDD2 _result;
//      _result.i = i = i + rhs;
//      _result.f = f = f + (float)rhs;
//      return _result;
//    }
//
//	UDD2 operator-() const restrict(cpu, amp)
//    {
//        UDD2 r;
//        r.i = -i;
//        r.f = -f;
//        return r;
//    }
//
//    UDD2() restrict(cpu, amp)
//        : i(0), f(0) { }
//    UDD2(int _in) restrict(amp,cpu)
//        : i(_in), f((float)(_in+2) ){ }
//};


struct UDD2
{
    int i;
    float f;
  
	bool operator <= (const UDD2& other) const restrict(cpu, amp)
	{
        return ((i <= other.i) && (f <= other.f));
    }

	bool operator >= (const UDD2& other) const restrict(cpu, amp)
	{
        return ((i >= other.i) && (f >= other.f));
    }

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

	/*UDD2 operator =  (const int rhs) restrict(cpu, amp)
    {
      UDD2 _result;
      _result.i = i + rhs;
      _result.f = f + (float)rhs;
      return _result;
    }*/

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


struct gen_input_udd
{
       UDD2 operator() ()  const restrict ( cpu, amp )
       { 
            int i=8;
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

struct add_3
{
      int operator() (const int x)  const restrict ( cpu, amp )
	  { 
		  return x + 3; 
	  }
      typedef int result_type;
};

struct gen_input_plus_1
{
      int operator() ()  const restrict ( cpu, amp )
	  { 
		  return 5; 
	  }
      typedef int result_type;
};

struct add_4
{
     int operator() (const int x)  const restrict ( cpu, amp )
	 { 
		 return x + 4; 
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(simple1,counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;
    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}

TEST(simple1,Serial_counting)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);
    std::vector<int> input2(1024);
    std::vector<int> stdOutput(1024);
     std::vector<int> boltOutput(1024);
     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    input2 = input1;

    bolt::amp::control ctl = bolt::amp::control::getDefault( );
    ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    std::transform( input1.begin(), input1.end(), input2.begin(), stdOutput.begin(), bolt::amp::plus<int>());
    bolt::amp::transform(ctl, iter,iter2,input1.begin(),boltOutput.begin(),bolt::amp::plus<int>());
    cmpArrays( stdOutput, boltOutput, 1024 );
}

TEST( PermutationIterator, UnaryTransformRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIndexVec( length );
        std::vector< int > svElementVec( length );
        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
        bolt::amp::device_vector< int > dvIndexVec( length );
        bolt::amp::device_vector< int > dvElementVec( length );
        bolt::amp::device_vector< int > dvOutVec( length );

        add_3 add3;
        gen_input gen;
        gen_input_plus_1 gen_plus_1;
        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;
        typedef bolt::amp::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_3, bolt::amp::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::amp::transform_iterator< add_4, bolt::amp::device_vector< int >::iterator>   dv_trf_itr_add4;    
        typedef bolt::amp::permutation_iterator< bolt::amp::device_vector< int >::iterator, 
                                                   bolt::amp::device_vector< int >::iterator>        dv_perm_itr;
        typedef bolt::amp::permutation_iterator< std::vector< int >::iterator, 
                                                   std::vector< int >::iterator>                       sv_perm_itr;
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIndexVec.begin(), add3), sv_trf_end1 (svIndexVec.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIndexVec.begin(), add3), dv_trf_end1 (dvIndexVec.end(), add3);

        //sv_perm_itr sv_perm_begin (svElementVec.begin(), svIndexVec.begin()), sv_perm_end (svElementVec.end(), svIndexVec.end());
        dv_perm_itr dv_perm_begin (dvElementVec.begin(), dvIndexVec.begin()), dv_perm_end (dvElementVec.end(), dvIndexVec.end());

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        /*Generate inputs*/
        std::generate(svIndexVec.begin(),   svIndexVec.end(),   gen); 
        std::generate(svElementVec.begin(), svElementVec.end(), gen_plus_1); 
        bolt::amp::generate(dvIndexVec.begin(), dvIndexVec.end(), gen);
        bolt::amp::generate(dvElementVec.begin(), dvElementVec.end(), gen_plus_1);

        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin, dv_perm_end, dvOutVec.begin(), add3);
            /*Compute expected results*/
            std::transform(dv_perm_begin, dv_perm_end, stlOut.begin(), add3);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
            //cmpArrays(svOutVec, stlOut);
        }
    }
}

TEST( PermutationIterator, UDDUnaryTransformRoutine)
{
    {

        const int length = 1<<10;
        std::vector< int > svIndexVec( length );
        std::vector< UDD2 > svElementVec( length );
        std::vector< UDD2 > svOutVec( length );
        std::vector< UDD2 > stlOut( length );
       
        bolt::amp::device_vector< UDD2 > dvOutVec( length );

        squareUDD_resultUDD sqUDD;
        //gen_input_udd2 gen_plus_1;
		//gen_input gen;


        typedef std::vector< UDD2 >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< UDD2 >                                                  constant_itr;
  
        typedef bolt::amp::permutation_iterator< bolt::amp::device_vector< UDD2 >::iterator, 
                                                   bolt::amp::device_vector< int >::iterator>        dv_perm_itr;
        typedef bolt::amp::permutation_iterator< std::vector< UDD2 >::iterator, 
                                                   std::vector< int >::iterator>                     sv_perm_itr;
     

		UDD2 temp;
		temp.i=1, temp.f=2.5f;

		UDD2 init;
		init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        /*Generate inputs*/
        /*std::generate(svIndexVec.begin(),   svIndexVec.end(),   gen); 
        std::generate(svElementVec.begin(), svElementVec.end(), gen_plus_1); 
        bolt::amp::generate(dvIndexVec.begin(), dvIndexVec.end(), gen);
        bolt::amp::generate(dvElementVec.begin(), dvElementVec.end(), gen_plus_1);*/

		UDD2 t;
		//t.i = 7, t.f = 7.f;
		std::iota(svIndexVec.begin(),   svIndexVec.end(), 0);

		//std::iota(svElementVec.begin(), svElementVec.end(), t);
		//bolt::amp::fill(svElementVec.begin(), svElementVec.end(), t); 

		for(long int i=0;i<length; i++)
		{
			svElementVec[i].i = i;
			svElementVec[i].f = (float) i;
		}
		
		bolt::amp::device_vector< int > dvIndexVec( svIndexVec.begin(),   svIndexVec.end() );
        bolt::amp::device_vector< UDD2 > dvElementVec( svElementVec.begin(), svElementVec.end() );

		/*Create Iterators*/

        //sv_perm_itr sv_perm_begin (svElementVec.begin(), svIndexVec.begin()), sv_perm_end (svElementVec.end(), svIndexVec.end());
        dv_perm_itr dv_perm_begin (dvElementVec.begin(), dvIndexVec.begin()), dv_perm_end (dvElementVec.end(), dvIndexVec.end());

        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin, dv_perm_end, dvOutVec.begin(), sqUDD);
            /*Compute expected results*/
            std::transform(dv_perm_begin, dv_perm_end, stlOut.begin(), sqUDD);
            /*Check the results*/

			std::vector<UDD2> bolt_out(dvOutVec.begin(), dvOutVec.end());

            cmpArrays(dvOutVec, stlOut);
            //cmpArrays(svOutVec, stlOut);
        }
    }
}

TEST( PermutationIterator, BinaryTransformRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIndexVec1( length );
        std::vector< int > svElementVec1( length );
        std::vector< int > svIndexVec2( length );
        std::vector< int > svElementVec2( length );

        std::vector< int > svOutVec( length );
        std::vector< int > stlOut( length );
        bolt::amp::device_vector< int > dvIndexVec1( length );
        bolt::amp::device_vector< int > dvElementVec1( length );
        bolt::amp::device_vector< int > dvIndexVec2( length );
        bolt::amp::device_vector< int > dvElementVec2( length );
        bolt::amp::device_vector< int > dvOutVec( length );

        add_3 add3;
        gen_input gen;
        bolt::amp::plus<int> plus;
        gen_input_plus_1 gen_plus_1;
        typedef std::vector< int >::const_iterator                                                     sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;
        typedef bolt::amp::transform_iterator< add_3, std::vector< int >::const_iterator>            sv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_3, bolt::amp::device_vector< int >::iterator>   dv_trf_itr_add3;
        typedef bolt::amp::transform_iterator< add_4, std::vector< int >::const_iterator>            sv_trf_itr_add4;
        typedef bolt::amp::transform_iterator< add_4, bolt::amp::device_vector< int >::iterator>   dv_trf_itr_add4;    
        typedef bolt::amp::permutation_iterator< bolt::amp::device_vector< int >::iterator, 
                                                   bolt::amp::device_vector< int >::iterator>        dv_perm_itr;
        typedef bolt::amp::permutation_iterator< std::vector< int >::iterator, 
                                                   std::vector< int >::iterator>                       sv_perm_itr;
        /*Create Iterators*/
        //sv_trf_itr_add3 sv_trf_begin1 (svIndexVec1.begin(), add3), sv_trf_end1 (svIndexVec1.end(), add3);
        dv_trf_itr_add3 dv_trf_begin1 (dvIndexVec1.begin(), add3), dv_trf_end1 (dvIndexVec1.end(), add3);

        //sv_perm_itr sv_perm_begin1 (svElementVec1.begin(), svIndexVec1.begin()), sv_perm_end1 (svElementVec1.end(), svIndexVec1.end());
        //sv_perm_itr sv_perm_begin2 (svElementVec2.begin(), svIndexVec2.begin());
        dv_perm_itr dv_perm_begin1 (dvElementVec1.begin(), dvIndexVec1.begin()), dv_perm_end1 (dvElementVec1.end(), dvIndexVec1.end());
        dv_perm_itr dv_perm_begin2 (dvElementVec2.begin(), dvIndexVec2.begin());

        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        /*Generate inputs*/
        std::generate(svIndexVec1.begin(),   svIndexVec1.end(),   gen); 
        std::generate(svElementVec1.begin(), svElementVec1.end(), gen_plus_1); 
        bolt::amp::generate(dvIndexVec1.begin(), dvIndexVec1.end(), gen);
        bolt::amp::generate(dvElementVec1.begin(), dvElementVec1.end(), gen_plus_1);

        std::generate(svIndexVec2.begin(),   svIndexVec2.end(),   gen); 
        std::generate(svElementVec2.begin(), svElementVec2.end(), gen_plus_1); 
        bolt::amp::generate(dvIndexVec2.begin(), dvIndexVec2.end(), gen);
        bolt::amp::generate(dvElementVec2.begin(), dvElementVec2.end(), gen_plus_1);

        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin1, dv_perm_end1, dv_perm_begin2, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_perm_begin1, dv_perm_end1, dv_perm_begin2, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin1, dv_perm_end1, dv_trf_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_perm_begin1, dv_perm_end1, dv_trf_begin1, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin1, dv_perm_end1, const_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_perm_begin1, dv_perm_end1, const_itr_begin, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin1, dv_perm_end1, count_itr_begin, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_perm_begin1, dv_perm_end1, count_itr_begin, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_perm_begin1, dv_perm_end1, dvElementVec2.begin(), dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_perm_begin1, dv_perm_end1, dvElementVec2.begin(), stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a permutation Iterator*/
            bolt::amp::transform(dvElementVec2.begin(), dvElementVec2.end(), dv_perm_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dvElementVec2.begin(), dvElementVec2.end(), dv_perm_begin1, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }

        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(const_itr_begin, const_itr_end, dv_perm_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(const_itr_begin, const_itr_end, dv_perm_begin1, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(dv_trf_begin1, dv_trf_end1, dv_perm_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(dv_trf_begin1, dv_trf_end1, dv_perm_begin1, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
        {/*Test case when input is a permutation Iterators*/
            bolt::amp::transform(count_itr_begin, count_itr_end, dv_perm_begin1, dvOutVec.begin(), plus);
            /*Compute expected results*/
            std::transform(count_itr_begin, count_itr_end, dv_perm_begin1, stlOut.begin(), plus);
            /*Check the results*/
            cmpArrays(dvOutVec, stlOut);
        }
    }
}



TEST(AMPIterators, AVPermutation)
{
    int __index = 1;
    const unsigned int size = 256;

    typedef int etype;
    etype elements[size];
    etype empty[size];

    size_t view_size = size;


    std::fill(elements, elements+size, 100);
    std::fill(empty, empty+size, 0);


    bolt::amp::device_vector<int, concurrency::array_view> resultV(elements, elements + size);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);
    auto dvbegin = resultV.begin();
    auto dvend = resultV.end();
    auto dumpbegin = dumpV.begin().getContainer().getBuffer();

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;

    concurrency::extent< 1 > inputExtent( size );
    concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
    {
      dumpbegin[idx[0]] = dvbegin[idx[0]];

    });
    dumpbegin.synchronize();
    cmpArrays(resultV, dumpV);

}


TEST(AMPIterators, PermutationPFE)
{

    const unsigned int size = 256;

    typedef int etype;
    etype elements[size];
    etype key[size];
    etype empty[size];

    size_t view_size = size;

    std::iota(elements, elements+size, 1000);
    std::iota(key, key+size, 0);
    std::fill(empty, empty+size, 0);

    std::random_shuffle ( key, key+size );

    bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + size);
    bolt::amp::device_vector<int, concurrency::array_view> dvk(key, key + size);
    bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);

    auto dvebegin = dve.begin();
    auto dveend = dve.end();

    auto dvkbegin = dvk.begin();
    auto dvkend = dvk.end();

    auto __ebegin =  dvebegin;
    auto __eend =  dveend;

    auto __kbegin =  dvkbegin;
    auto __kend =    dvkend;

    typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                             bolt::amp::device_vector<int>::iterator> intvpi;

    intvpi first = bolt::amp::make_permutation_iterator(__kbegin, __ebegin);
    //i = first;
    intvpi last = bolt::amp::make_permutation_iterator(__kend, __eend);

    bolt::amp::control ctl;
    concurrency::accelerator_view av = ctl.getAccelerator().default_view;
    auto dumpAV = dumpV.begin().getContainer().getBuffer();

    concurrency::extent< 1 > inputExtent( size );
    concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
    {
      int gidx = idx[0];

      dumpAV[gidx] = first[gidx];

    });
    dumpAV.synchronize();

}

// Warning 4996
TEST(AMPIterators, PermutationGatherTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());

    // warning 4996
//    boost::transform( exp_result, boost::make_permutation_iterator( input.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    bolt::amp::transform( bolt::amp::make_permutation_iterator( dinput.begin(), dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dinput.end(), dmap.end() ),
                          dresult.begin(),
                          bolt::amp::identity<int>( ) );
  //  cmpArrays(exp_result, result);
}


TEST(AMPIterators, UDDPermutationGatherTest)
{

	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::transform( boost::make_permutation_iterator(el.begin( ), map.begin()), 
                    boost::make_permutation_iterator(el.end( ), map.end() ),
                    result.begin(),
                    std::identity<UDD2>( )
                    );

    bolt::amp::transform( bolt::amp::make_permutation_iterator(dvebegin, dmap.begin()), 
                          bolt::amp::make_permutation_iterator(dveend, dmap.end()),
						  dresult.begin(),
                          bolt::amp::identity<UDD2>()
                          );

    cmpArrays(result,dresult);

}



#if 0
TEST(AMPIterators, PermutationGatherTestStd)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    // warning 4996
    boost::transform( exp_result, boost::make_permutation_iterator( input.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    bolt::amp::transform( bolt::amp::make_permutation_iterator( input.begin(), map.begin() ),
                          bolt::amp::make_permutation_iterator( input.end(), map.end() ),
                          result.begin(),
                          bolt::amp::identity<int>( ) );
    cmpArrays(exp_result, result);
}


TEST(AMPIterators, PermutationScatterTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> exp_result(10,0);
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());

    // warning 4996
    boost::transform( input, boost::make_permutation_iterator( exp_result.begin(), map.begin() ),bolt::amp::identity<int>( ) );

    // Set CPU control
    //bolt::amp::control ctl;
    //ctl.setForceRunMode(bolt::amp::control::SerialCpu);

    bolt::amp::transform( dinput.begin(),
                          dinput.end(),
                          bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ),
                          bolt::amp::identity<int>( ) );
    cmpArrays(exp_result, result);

    std::cout<<"element"<<"     index"<<"       output"<<std::endl;
    for ( int i = 0 ; i < 10 ; i++ )
    {
        std::cout<<input[i]<<"     "<<map[i]<<"       "<<result[i]<<std::endl;
    }


    // Test for permutation iterator as a mutable iterator
    //auto indxx = boost::make_permutation_iterator( result.end()-1, map.end()-1 );
    //indxx[0] = 89;
    //std::cout<<result[9]<<"     "<<input[9]<<std::endl;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Reduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(AMPIterators, PermutationReduceTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());

    int exp_out = std::accumulate( boost::make_permutation_iterator( input.begin(), map.begin() ),
      boost::make_permutation_iterator( input.end(), map.end() ), int(0), std::plus<int>( ) );

    //bolt::amp::transform( dinput.begin(), dinput.end(), bolt::amp::make_permutation_iterator( dresult.end(), dmap.end() ), bolt::amp::identity<int>( ) );
    int out =  bolt::amp::reduce( bolt::amp::make_permutation_iterator( dinput.begin(), dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dinput.end(), dmap.end() ),
                          int(0),
                          bolt::amp::plus<int>( ) );
    EXPECT_EQ(exp_out, out);
}


TEST(AMPIterators, UDDPermutationReduceTest)
{

	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

	UDD2 init;
	init.i = 0, init.f = 0.0f;

    UDD2 exp_out = std::accumulate( boost::make_permutation_iterator( el.begin(), map.begin() ),
      boost::make_permutation_iterator( el.end(), map.end() ), UDD2(init), std::plus<UDD2>( ) );

    
    UDD2 out =  bolt::amp::reduce( bolt::amp::make_permutation_iterator( dvebegin, dmap.begin() ),
                          bolt::amp::make_permutation_iterator( dveend, dmap.end() ),
                          UDD2(init),
                          bolt::amp::plus<UDD2>( ) );
    EXPECT_EQ(exp_out, out);

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform Reduce tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationTransformReduceTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult(result.begin(), result.end());
    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());

    std::transform( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                    boost::make_permutation_iterator( input.end( ), map.end( ) ),
                    result.begin( ), std::negate<int>( ) );
    int exp_out = std::accumulate( result.begin( ),
                                   result.end( ),
                                   int( 0 ), std::plus<int>( ) );

    int out = bolt::amp::transform_reduce( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                                           bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                                           bolt::amp::negate<int>( ),
                                           int( 0 ),
                                           bolt::amp::plus<int>( ) );
    EXPECT_EQ(exp_out, out);
}


TEST(AMPIterators, UDDPermutationTransformReduceTest)
{

	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

	UDD2 init;
	init.i = 0, init.f = 0.0f;

    std::transform( boost::make_permutation_iterator( el.begin( ), map.begin( ) ),
                    boost::make_permutation_iterator( el.end( ), map.end( ) ),
                    result.begin( ), std::negate<UDD2>( ) );
    UDD2 exp_out = std::accumulate( result.begin( ),
                                   result.end( ),
                                   UDD2( init ), std::plus<UDD2>( ) );

    UDD2 out = bolt::amp::transform_reduce( bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                                           bolt::amp::make_permutation_iterator( dveend, dmap.end( ) ),
                                           bolt::amp::negate<UDD2>( ),
                                           UDD2( init ),
                                           bolt::amp::plus<UDD2>( ) );
    EXPECT_EQ(exp_out, out);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform Copy tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationCopyTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    std::copy( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( input.end( ), map.end( ) ),
               exp_result.begin( ) );

    bolt::amp::copy( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                     dresult.begin( ) );


    cmpArrays(exp_result, dresult);
}

TEST(AMPIterators, UDDPermutationCopyTest)
{

	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::copy( boost::make_permutation_iterator( el.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( el.end( ), map.end( ) ),
               result.begin( ) );

    bolt::amp::copy( bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                     bolt::amp::make_permutation_iterator( dveend, dmap.end( ) ),
                     dresult.begin( ) );


    cmpArrays(result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Inner Product tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationInnerProductTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_input2[10] =  {5,10,15,20,25,30,35,40,45,50};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> input2 ( n_input2, n_input2 + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
    bolt::amp::device_vector<int> dinput2(input2.begin(), input2.end());
    bolt::amp::device_vector<int> dmap(map.begin(), map.end());

    std::copy( boost::make_permutation_iterator( input2.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( input2.end( ), map.end( ) ),
               result.begin( ) );

    int exp_out = std::inner_product( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                                      boost::make_permutation_iterator( input.end( ), map.end( ) ),
                                      result.begin( ),
                                      int( 5 ),
                                      std::plus<int>( ),
                                      std::plus<int>( ) );

    int out = bolt::amp::inner_product( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                                        bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                                        bolt::amp::make_permutation_iterator( dinput2.begin( ), dmap.begin( ) ),
                                        int( 5 ),
                                        bolt::amp::plus<int>( ),
                                        bolt::amp::plus<int>( ) );
    EXPECT_EQ(exp_out, out);
}


TEST(AMPIterators, UDDPermutationInnerProductTest)
{

	typedef UDD2 etype;
    etype elements[10], elements2[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);
	std::iota(elements2, elements2+10, 100);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);
	bolt::amp::device_vector<UDD2, concurrency::array_view> dve2(elements2, elements2 + 10);

    std::vector<UDD2> el(elements, elements+10);
	std::vector<UDD2> el2(elements2, elements2+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );


	UDD2 t;
	t.i = 5, t.f = 5.f;

    std::copy( boost::make_permutation_iterator( el2.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( el2.end( ), map.end( ) ),
               result.begin( ) );

    UDD2 exp_out = std::inner_product( boost::make_permutation_iterator( el.begin( ), map.begin( ) ),
                                      boost::make_permutation_iterator( el.end( ), map.end( ) ),
                                      result.begin( ),
                                      UDD2( t ),
                                      std::plus<UDD2>( ),
                                      std::plus<UDD2>( ) );

    UDD2 out = bolt::amp::inner_product( bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                                        bolt::amp::make_permutation_iterator( dveend, dmap.end( ) ),
                                        bolt::amp::make_permutation_iterator(dve2.begin( ), dmap.begin( ) ),
                                        UDD2( t ),
                                        bolt::amp::plus<UDD2>( ),
                                        bolt::amp::plus<UDD2>( ) );
    EXPECT_EQ(exp_out, out);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Max_element tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TEST(AMPIterators, PermutationMinElementTest)
//{
//    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
//    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//    //typedef typename bolt::amp::device_vector<int>::iterator iter
//
//    bolt::amp::device_vector<int> dinput(input.begin(), input.end());
//    bolt::amp::device_vector<int> dmap(map.begin(), map.end());
//
//    std::vector<int>::iterator stlElement =
//        std::min_element( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
//                          boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
//                          std::less< int >( ) );
//    //bolt::amp::permutation_iterator<iter,iter> boltReduce = bolt::amp::min_element(first, last, bolt::amp::less< int >( ));
//
//    //EXPECT_EQ(*stlReduce, *boltReduce);
//
//}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AMPIterators, PermutationScanTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    bolt::amp::inclusive_scan( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                               bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                               dresult.begin( ), bolt::amp::plus<int>( ) );

    ::std::partial_sum( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                        boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
                        result.begin( ), std::plus<int>( ) );
    // compare results
    cmpArrays(result, dresult);
}

TEST(AMPIterators, UDDPermutationScanTest)
{
    typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::inclusive_scan( bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                               bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                               dresult.begin( ), bolt::amp::plus<UDD2>( ) );

    ::std::partial_sum( boost::make_permutation_iterator( el.begin( ), map.begin( ) ),
                        boost::make_permutation_iterator( el.begin( ), map.begin( ) ),
                        result.begin( ), std::plus<UDD2>( ) );
    // compare results
    cmpArrays(result, dresult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Scan_By_Key tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST(AMPIterators, PermutationScanByKeyTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_keys[10] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5 };
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> keys ( n_keys, n_keys + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dkeys( keys.begin( ), keys.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    bolt::amp::equal_to<int> eq;
    bolt::amp::multiplies<int> mult;

    bolt::amp::inclusive_scan_by_key( bolt::amp::make_permutation_iterator(dkeys.begin( ),dmap.begin()),
        bolt::amp::make_permutation_iterator(dkeys.end( ),dmap.end()),
        dinput.begin( ), dresult.begin( ), eq, mult );

    gold::scan_by_key( boost::make_permutation_iterator(keys.begin( ),map.begin()),
        boost::make_permutation_iterator(keys.end( ),map.end()), input.begin( ), result.begin( ), mult );
    // compare results
    cmpArrays(result, dresult);
}


TEST(AMPIterators, UDDPermutationScanByKeyTest)
{

	typedef UDD2 etype;
    etype elements[10], keys[10];;

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);
	std::iota(keys, keys+10, -4);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);
	bolt::amp::device_vector<UDD2, concurrency::array_view> dvk(keys, keys + 10);

    std::vector<UDD2> el(elements, elements+10);
	std::vector<UDD2> key(keys, keys+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::equal_to<UDD2> eq;
    bolt::amp::multiplies<UDD2> mult;

    bolt::amp::inclusive_scan_by_key( bolt::amp::make_permutation_iterator(dvk.begin( ),dmap.begin()),
        bolt::amp::make_permutation_iterator(dvk.end( ),dmap.end()),
        dvebegin, dresult.begin( ), eq, mult );

    gold::scan_by_key( boost::make_permutation_iterator(key.begin( ),map.begin()),
        boost::make_permutation_iterator(key.end( ),map.end()), el.begin( ), result.begin( ), mult );
    // compare results
    cmpArrays(result, dresult);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Reduce_By_Key tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Add pair support
//TEST(AMPIterators, PermutationReduceByKeyTest)
//{
//    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
//    int n_keys[10] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5 };
//    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
//    std::vector<int> result ( 10, 0 );
//    std::vector<int> exp_result ( 10, 0 );
//    std::vector<int> input ( n_input, n_input + 10 );
//    std::vector<int> keys ( n_keys, n_keys + 10 );
//    std::vector<int> map ( n_map, n_map + 10 );
//
//
//    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
//    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
//    bolt::amp::device_vector<int> dkeys( keys.begin( ), keys.end( ) );
//    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
//
//  std::vector<int>  std_koutput( 10 );
//  std::vector<int>  std_voutput( 10);
//
//    bolt::amp::device_vector<int>  koutput( std_koutput.begin(), std_koutput.end() );
//    bolt::amp::device_vector<int>  voutput( std_voutput.begin(), std_voutput.end() );
//
//    bolt::amp::equal_to<int> binary_predictor;
//    bolt::amp::plus<int> binary_operator;
//
//
//    bolt::amp::reduce_by_key( bolt::amp::make_permutation_iterator(dkeys.begin( ),dmap.begin()),
//        bolt::amp::make_permutation_iterator(dkeys.end( ),dmap.end()),
//        dinput.begin( ), koutput.begin( ), voutput.begin( ), binary_predictor, binary_operator );
//
//    gold::reduce_by_key( boost::make_permutation_iterator(keys.begin( ),map.begin()),
//        boost::make_permutation_iterator(keys.end( ),map.end()), input.begin( ),
//        std_koutput.begin( ), std_voutput.begin( ), binary_predictor,  binary_operator );
//    // compare results
//    cmpArrays(result, dresult);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Transform_Scan tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST(AMPIterators, UDDPermutationTransformScanTest)
{
	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	std::vector<UDD2> result ( 10 );
	bolt::amp::device_vector<UDD2> dresult(result.begin(), result.end());
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    bolt::amp::plus<UDD2> aI2;
    bolt::amp::negate<UDD2> nI2;

    bolt::amp::transform_inclusive_scan(
                            bolt::amp::make_permutation_iterator(dvebegin,dmap.begin()),
                            bolt::amp::make_permutation_iterator(dveend,dmap.end()),
                            dresult.begin(), nI2, aI2 );

    ::std::transform( boost::make_permutation_iterator(el.begin(),map.begin()),
                      boost::make_permutation_iterator(el.end(),map.end()),  result.begin(), nI2);
    ::std::partial_sum( result.begin(), result.end(),  result.begin(), aI2);

    cmpArrays(result, dresult);
}


TEST(AMPIterators, PermutationTransformScanTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
    int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> result ( 10, 0 );
    std::vector<int> exp_result ( 10, 0 );
    std::vector<int> input ( n_input, n_input + 10 );
    std::vector<int> map ( n_map, n_map + 10 );


    bolt::amp::device_vector<int> dresult( result.begin( ), result.end( ) );
    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    bolt::amp::plus<int> aI2;
    bolt::amp::negate<int> nI2;

    bolt::amp::transform_inclusive_scan(
                            bolt::amp::make_permutation_iterator(dinput.begin(),dmap.begin()),
                            bolt::amp::make_permutation_iterator(dinput.end(),dmap.end()),
                            dresult.begin(), nI2, aI2 );

    ::std::transform( boost::make_permutation_iterator(input.begin(),map.begin()),
                      boost::make_permutation_iterator(input.end(),map.end()),  exp_result.begin(), nI2);
    ::std::partial_sum( exp_result.begin(), exp_result.end(),  exp_result.begin(), aI2);

    cmpArrays(exp_result, dresult);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Count tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
// Functor for range checking.
struct InRange {
    InRange (T low, T high) restrict (amp,cpu) {
        _low=low;
        _high=high;
    };

    bool operator()(const T& value) const restrict (amp,cpu) {
        //printf("Val=%4.1f, Range:%4.1f ... %4.1f\n", value, _low, _high);
        return (value >= _low) && (value <= _high) ;
    };

    T _low;
    T _high;
};


TEST(AMPIterators, PermutationCountTest)
{
    std::vector<int> input(10);
    std::vector<int> map(10);

    for ( int i=0, j=9 ; i < 10; i++, j--) {
        input[ i ] = ( i + 1 );
        map[ i ]  = ( j );
    };


    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
    bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
    //  4996 expected
    //int stdCount = std::count_if (
    //                              boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
    //                              boost::make_permutation_iterator( input.end( ), map.end( ) ),
    //                              InRange<float>( 6.0f, 10.0f )
    //                              );
    //int boltCount = bolt::amp::count_if (
    //                                     bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
    //                                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
    //                                     InRange<float>( 6, 10 )
    //                                     );

    //EXPECT_EQ (stdCount, boltCount);

    auto stdCount = std::count_if (
                                  input.begin( ),
                                  input.end( )  ,
                                  InRange<int>( 6, 10 )
                                  );
    auto boltCount = bolt::amp::count_if (
                                         bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                                         bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                                         InRange<int>( 6, 10 )
                                         );

    EXPECT_EQ (stdCount, boltCount);

}

TEST(AMPIterators, UDDPermutationCountTest)
{
	typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=9-i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );
	
    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

	UDD2 first, last;
	first.i = 6, first.f = 6.f;
	last.i = 10, last.f = 10.f;

    auto stdCount = std::count_if (
                                  el.begin( ),
                                  el.end( )  ,
                                  InRange<UDD2>( first, last )
                                  );
    auto boltCount = bolt::amp::count_if (
                                         bolt::amp::make_permutation_iterator( dvebegin, dmap.begin( ) ),
                                         bolt::amp::make_permutation_iterator( dveend, dmap.end( ) ),
                                         InRange<UDD2>( first, last )
                                         );

    EXPECT_EQ (stdCount, boltCount);

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sanity tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(sanity_AMPIterators1, AVPermutation)
{
  int __index = 1;
  const unsigned int size = 2000;

  typedef int etype;
  etype elements[size];
  etype empty[size];

  size_t view_size = size;

  std::fill(elements, elements+size, 100);
  std::fill(empty, empty+size, 0);

  bolt::amp::device_vector<int, concurrency::array_view> resultV(elements, elements + size);
  bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);
  auto dvbegin = resultV.begin();
  auto dvend = resultV.end();
  auto dumpbegin = dumpV.begin().getContainer().getBuffer();

  bolt::amp::control ctl;
  concurrency::accelerator_view av = ctl.getAccelerator().default_view;

  concurrency::extent< 1 > inputExtent( size );
  concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
  {
    dumpbegin[idx[0]] = dvbegin[idx[0]];

  });
  dumpbegin.synchronize();

  for (int i=0; i<size;i++)
  {
    EXPECT_EQ(resultV[i],dumpV[i]);

  }
}


TEST(sanity_AMPIterators2, PermutationPFE)
{

  const unsigned int size = 10;

  typedef int etype;
  etype elements[size];
  etype key[size];
  etype empty[size];

  size_t view_size = size;

  std::iota(elements, elements+size, 1000);
  std::iota(key, key+size, 0);
  std::fill(empty, empty+size, 0);

  std::random_shuffle ( key, key+size );

  bolt::amp::device_vector<int, concurrency::array_view> dve(elements, elements + size);
  bolt::amp::device_vector<int, concurrency::array_view> dvk(key, key + size);
  bolt::amp::device_vector<int, concurrency::array_view> dumpV(empty, empty + size);

  auto dvebegin = dve.begin();
  auto dveend = dve.end();

  auto dvkbegin = dvk.begin();
  auto dvkend = dvk.end();

  auto __ebegin =  dvebegin;
  auto __eend =  dveend;

  auto __kbegin =  dvkbegin;
  auto __kend =    dvkend;

  typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                           bolt::amp::device_vector<int>::iterator> intvpi;

  intvpi first = bolt::amp::make_permutation_iterator(__ebegin, __kbegin  );
  //i = first;
  intvpi last = bolt::amp::make_permutation_iterator(__eend,__kend );

  bolt::amp::control ctl;
  concurrency::accelerator_view av = ctl.getAccelerator().default_view;
  auto dumpAV = dumpV.begin().getContainer().getBuffer();

  concurrency::extent< 1 > inputExtent( size );
  concurrency::parallel_for_each(av, inputExtent, [=](concurrency::index<1> idx)restrict(amp)
  {
    int gidx = idx[0];
    dumpAV[gidx] = first[gidx];
  });
  dumpAV.synchronize();

  for (int i=0; i<size;i++)
  {
    EXPECT_EQ(first[i],dumpV[i]);
  }
}


TEST(sanity_permutation_gather, Permutation_GatherTest)
{
  const unsigned int size = 10;

  int n_input[size];
  int n_map[size];

  std::iota(n_input, n_input+size, 1000);
  std::iota(n_map, n_map+size, 0);
  std::random_shuffle ( n_map, n_map+size );

  std::vector<int> exp_result(size);
  std::vector<int> result (size);
  std::vector<int> input ( n_input, n_input + size );
  std::vector<int> map ( n_map, n_map + size );

  bolt::amp::device_vector<int> dresult(size);
  bolt::amp::device_vector<int> dinput(n_input, n_input+size);
  bolt::amp::device_vector<int> dmap(n_map, n_map+size);

  typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                           bolt::amp::device_vector<int>::iterator> intvpi;

  auto __dinputbg = dinput.begin();
  auto __dmapbg = dmap.begin();

  auto __dinpute = dinput.end();
  auto __dmape = dmap.end();

  intvpi first = bolt::amp::make_permutation_iterator(__dinputbg, __dmapbg);
  intvpi last = bolt::amp::make_permutation_iterator(__dinpute, __dmape);

  bolt::amp::transform( first,last,dresult.begin(),bolt::amp::identity<int>() );

  bolt::amp::gather(map.begin(), map.end(), input.begin(), result.begin());
  std::transform(result.begin(), result.end(), exp_result.begin(), std::identity<int>());

  for (int i=0; i<size;i++)
  {
    EXPECT_EQ(dresult[i],result[i]);
  }
}



TEST(sanity_AMPIterators, PermutationReduceTest)
{
  const unsigned int size = 5;

  typedef int etype;
  etype n_input[size];
  etype n_map[size];

  size_t view_size = size;

  std::iota(n_input, n_input+size, 1000);
  std::iota(n_map, n_map+size, 0);

  std::random_shuffle ( n_map, n_map+size );

  std::vector<int> input ( n_input, n_input + size);
  std::vector<int> map ( n_map, n_map + size );

  bolt::amp::device_vector<int> dinput(n_input, n_input+size);
  bolt::amp::device_vector<int> dmap(n_map, n_map+size);

  //printing the device vector elements with iter3 iter4
  bolt::amp::device_vector<int>::iterator iter3=dinput.begin();
  bolt::amp::device_vector<int>::iterator iter4=dmap.begin();

  //calling the bolt api

  typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                           bolt::amp::device_vector<int>::iterator> intvpi;

  intvpi first = bolt::amp::make_permutation_iterator(dinput.begin(), dmap.begin() );
  intvpi last = bolt::amp::make_permutation_iterator(dinput.end(), dmap.end() );

  int bolt_out =  bolt::amp::reduce( first,last,int(0),bolt::amp::plus<int>( ) );

  int std_out = std::accumulate( boost::make_permutation_iterator( input.begin(), map.begin() ),
                               boost::make_permutation_iterator( input.end(), map.end() ), int(0), std::plus<int>( ) );

  EXPECT_EQ(bolt_out,std_out);
}

TEST(sanity_boost, PermutationIterator_with_boost_amp)
{
  int i = 0;
  static const int element_range_size = 10;
  static const int index_size = 4;

  //boost code

  typedef std::vector< int > element_range_type;
  typedef std::vector< int > index_type;

  element_range_type elements( element_range_size );

  for(element_range_type::iterator el_it = elements.begin() ; el_it != elements.end() ; ++el_it)
  *el_it = static_cast<int>(std::distance(elements.begin(), el_it));

  std::cout << "The original range is : ";
  std::copy( elements.begin(), elements.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  index_type indices( index_size );

  for(index_type::iterator i_it = indices.begin() ; i_it != indices.end() ; ++i_it )
    *i_it = static_cast<int>(element_range_size - index_size + std::distance(indices.begin(), i_it));
  std::reverse( indices.begin(), indices.end() );

  std::cout << "The re-indexing scheme is : ";
  std::copy( indices.begin(), indices.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  typedef boost::permutation_iterator< element_range_type::iterator, index_type::iterator > permutation_type;

  permutation_type begin = boost::make_permutation_iterator( elements.begin(), indices.begin() );
  permutation_type end = boost::make_permutation_iterator( elements.end(), indices.end() );

  //bolt
  typedef bolt::amp::device_vector< int > bolt_element_range_type;
  typedef bolt::amp::device_vector< int > bolt_index_type;

  bolt_element_range_type bolt_elements( element_range_size );
  bolt_element_range_type::iterator bolt_el_it = bolt_elements.begin();

  for(bolt_el_it = bolt_elements.begin(); bolt_el_it != bolt_elements.end() ; ++bolt_el_it)
  *bolt_el_it = static_cast<int>(std::distance(bolt_elements.begin(), bolt_el_it));

  std::cout << "The bolt-original range is : ";
  std::copy( bolt_elements.begin(), bolt_elements.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  bolt_index_type bolt_indices( index_size );
  bolt_index_type::iterator bolt_i_it = bolt_indices.begin();

  for(bolt_i_it = bolt_indices.begin(); bolt_i_it != bolt_indices.end() ; ++bolt_i_it )
    *bolt_i_it = static_cast<int>(element_range_size - index_size + std::distance(bolt_indices.begin(), bolt_i_it));

  std::reverse( bolt_indices.begin(), bolt_indices.end() );

  std::cout << "The bolt re-indexing scheme is : ";
  std::copy( bolt_indices.begin(), bolt_indices.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  typedef bolt::amp::permutation_iterator< bolt_element_range_type::iterator, bolt_index_type::iterator > permutation_type_bolt;

  permutation_type_bolt begin_bolt = bolt::amp::make_permutation_iterator( bolt_elements.begin(), bolt_indices.begin() );
  permutation_type_bolt end_bolt = bolt::amp::make_permutation_iterator( bolt_elements.end(), bolt_indices.end() );


  // bolt- boost comparsion

  std::cout << "The permutated range for boost is : ";
  std::copy( begin, end, std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  std::cout << "The permutated range for bolt amp is : ";
  std::copy( begin_bolt, end_bolt, std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";
}

struct zip_func :
  public std::unary_function<const boost::tuple<const double&, const int&>&, void>
{
  void operator()(const boost::tuple<const int&, const int&>& t) const 
  {
    std::cout << boost::get<0>(t)<< ", " << boost::get<1>(t) << std::endl;
  }

};

//Zip Iterators not yet implemented for AMP!
#if 0
TEST(AMPIterators, ZipForEachTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
	//double n_input2[10] =  {9.8,8.5,7.0,6.2,5.1,4.3,3.1,2.7,1.0,0.0};
	int n_input2[10] = {9,8,7,6,5,4,3,2,1,0};

    std::vector<int> input ( n_input, n_input + 10 );
	std::vector<int> input2 ( n_input2, n_input2 + 10 );

    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
	bolt::amp::device_vector<int> dinput2( input2.begin( ), input2.end( ) );

    std::for_each( boost::make_zip_iterator( boost::make_tuple (input.begin( ), input2.begin( )) ),
               boost::make_zip_iterator( boost::make_tuple (input.end( ), input2.end( ) ) ),
               zip_func() /*[]( boost::tuple<int, int> const& tup ) {
            std::cout 
              << boost::get<0>(tup) 
              << ", " 
              << boost::get<1>(tup) 
              << std::endl;
        }*/
 );

   /* bolt::amp::for_each( bolt::amp::make_zip_iterator( boost::make_tuple ( dinput.begin( ), dinput2.begin( ) )),
                    bolt::amp::make_zip_iterator( boost::make_tuple ( dinput.end( ), dinput2.end( ) ),
                     zip_func());
*/

    cmpArrays(input, dinput);
	cmpArrays(input2, dinput2);
}

TEST(AMPIterators, ZipForEach_nTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
	int n_input2[10] = {9,8,7,6,5,4,3,2,1,0};

	int n = rand()%10;

    std::vector<int> input ( n_input, n_input + 10 );
	std::vector<int> input2 ( n_input2, n_input2 + 10 );

    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
	bolt::amp::device_vector<int> dinput2( input2.begin( ), input2.end( ) );

    std::for_each( boost::make_zip_iterator( boost::make_tuple (input.begin( ), input2.begin( )) ),
               boost::make_zip_iterator( boost::make_tuple (input.begin( ), input2.begin( )) ) + n,
               zip_func() );

   /* bolt::amp::for_each_n( bolt::amp::make_zip_iterator( boost::make_tuple ( dinput.begin( ), dinput2.begin( ) )),
                     n,
                     zip_func());
   */

    cmpArrays(input, dinput);
	cmpArrays(input2, dinput2);
}

#endif

//these 2 test cases dont generate any output for TBB path!
TEST(AMPIterators, PermutationForEachTest)
{
    int n_input[10] =  {0,1,2,3,4,5,6,7,8,9};
	int n_map[10] =  {9,8,7,6,5,4,3,2,1,0};
    std::vector<int> input ( n_input, n_input + 10 );
	std::vector<int> map ( n_map, n_map + 10 );

    bolt::amp::device_vector<int> dinput( input.begin( ), input.end( ) );
	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    std::for_each( boost::make_permutation_iterator( input.begin( ), map.begin( ) ),
               boost::make_permutation_iterator( input.end( ), map.end( )  ),
               std::negate<int>() );

    bolt::amp::for_each( bolt::amp::make_permutation_iterator( dinput.begin( ), dmap.begin( ) ),
                     bolt::amp::make_permutation_iterator( dinput.end( ), dmap.end( ) ),
                     bolt::amp::negate<int>());


    cmpArrays(input, dinput);
}

TEST(AMPIterators,  PermutationForEachUDDTest)
{

    typedef UDD2 etype;
    etype elements[10];

    size_t view_size = 10;

    std::iota(elements, elements+10, 1000);

    bolt::amp::device_vector<UDD2, concurrency::array_view> dve(elements, elements + 10);

    std::vector<UDD2> el(elements, elements+10);

	std::vector<int> map(10);
	for(int i=0;i<10;i++)
		map[i]=i;

	bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

    auto dvebegin = dve.begin( );
    auto dveend = dve.end( );

    std::for_each( boost::make_permutation_iterator(el.begin( ), map.begin()), 
                    boost::make_permutation_iterator(el.end( ), map.end() ),
                    std::negate<UDD2>()
                    );

    bolt::amp::for_each( bolt::amp::make_permutation_iterator(dvebegin, dmap.begin()), 
                          bolt::amp::make_permutation_iterator(dveend, dmap.end()),
                          bolt::amp::negate<UDD2>()
                          );

    cmpArrays(el,dvebegin);

}

TEST( PermutationIterator, ForEachRoutine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );

        /*Generate inputs*/
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::amp::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

		std::vector<int> map(length);
	    for(int i=0;i<length;i++)
		  map[i]= length-1-i;
		bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;


		typedef bolt::amp::permutation_iterator< std::vector<int>::iterator,
                                             std::vector<int>::iterator> sv_trf_itr;
		typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                             bolt::amp::device_vector<int>::iterator> dv_trf_itr;

  
        /*Create Iterators*/

		//sv_trf_itr sv_trf_begin1 = bolt::amp::make_permutation_iterator(svIn1Vec.begin(), map.begin());
		//sv_trf_itr sv_trf_end1 = bolt::amp::make_permutation_iterator(svIn1Vec.end(), map.end());

		dv_trf_itr dv_trf_begin1 = bolt::amp::make_permutation_iterator(dvIn1Vec.begin(), dmap.begin());
		dv_trf_itr dv_trf_end1 = bolt::amp::make_permutation_iterator(dvIn1Vec.end(), dmap.end());


        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

		
        {/*Test case when inputs are Permutation Iterators*/
		    std::vector<int> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each(sv_trf_begin1, sv_trf_end1, bolt::amp::negate<int>());
			//std::vector<int> bolt_output_1(sv_trf_begin1, sv_trf_end1);

            bolt::amp::for_each(dv_trf_begin1, dv_trf_end1, bolt::amp::negate<int>());
			std::vector<int> bolt_output(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.end(), std::negate<int>() );

            /*Check the results*/
            //cmpArrays(bolt_output1, std_input);
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


TEST( PermutationIterator, ForEachUDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );

		std::vector<int> map(length);
	    for(int i=0;i<length;i++)
		  map[i]= length-1-i;
		bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

        /*Generate inputs*/
        gen_input_udd genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::amp::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::amp::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< UDD2 >                                                  constant_itr;
		typedef bolt::amp::permutation_iterator< std::vector<UDD2>::iterator,
                                             std::vector<int>::iterator> sv_trf_itr;
		typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<UDD2>::iterator,
                                             bolt::amp::device_vector<int>::iterator> dv_trf_itr;
     
        /*Create Iterators*/

		//sv_trf_itr sv_trf_begin1 = bolt::amp::make_permutation_iterator(svIn1Vec.begin(), map.begin());
		//sv_trf_itr sv_trf_end1 = bolt::amp::make_permutation_iterator(svIn1Vec.end(), map.end());

		dv_trf_itr dv_trf_begin1 = bolt::amp::make_permutation_iterator(dvIn1Vec.begin(), dmap.begin());
		dv_trf_itr dv_trf_end1 = bolt::amp::make_permutation_iterator(dvIn1Vec.end(), dmap.end());

		UDD2 temp;
		temp.i=1, temp.f=2.5f;

		UDD2 init;
		init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is permutation Iterator*/
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

TEST( PermutationIterator, ForEach_n_Routine)
{
    {
        const int length = 1<<10;
        std::vector< int > svIn1Vec( length );

		int n = rand()%length;

        /*Generate inputs*/
        gen_input gen;

        std::generate(svIn1Vec.begin(), svIn1Vec.end(), gen);
        bolt::amp::device_vector< int > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

		std::vector<int> map(length);
	    for(int i=0;i<length;i++)
		  map[i]= length-1-i;
		bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

        typedef std::vector< int >::const_iterator                                                   sv_itr;
        typedef bolt::amp::device_vector< int >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< int >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< int >                                                  constant_itr;


		typedef bolt::amp::permutation_iterator< std::vector<int>::iterator,
                                             std::vector<int>::iterator> sv_trf_itr;
		typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<int>::iterator,
                                             bolt::amp::device_vector<int>::iterator> dv_trf_itr;

  
        /*Create Iterators*/

		//sv_trf_itr sv_trf_begin1 = bolt::amp::make_permutation_iterator(svIn1Vec.begin(), map.begin());
		//sv_trf_itr sv_trf_end1 = bolt::amp::make_permutation_iterator(svIn1Vec.end(), map.end());

		dv_trf_itr dv_trf_begin1 = bolt::amp::make_permutation_iterator(dvIn1Vec.begin(), dmap.begin());
		dv_trf_itr dv_trf_end1 = bolt::amp::make_permutation_iterator(dvIn1Vec.end(), dmap.end());


        counting_itr count_itr_begin(0);
        counting_itr count_itr_end = count_itr_begin + length;
        constant_itr const_itr_begin(1);
        constant_itr const_itr_end = const_itr_begin + length;

        {/*Test case when input is Permutation Iterators*/
		    std::vector<int> std_input(dv_trf_begin1, dv_trf_end1);

            //bolt::amp::for_each(sv_trf_begin1, sv_trf_end1, bolt::amp::negate<int>());
			//std::vector<int> bolt_output_1(sv_trf_begin1, sv_trf_end1);

            bolt::amp::for_each_n(dv_trf_begin1, n, bolt::amp::negate<int>());
			std::vector<int> bolt_output(dv_trf_begin1, dv_trf_end1);

            /*Compute expected results*/
            std::for_each(std_input.begin(), std_input.begin() + n, std::negate<int>() );

            /*Check the results*/
            //cmpArrays(bolt_output1, std_input);
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


TEST( PermutationIterator, ForEach_n_UDDRoutine)
{
    {
        const int length = 1<<10;
        std::vector< UDD2 > svIn1Vec( length );
        std::vector< UDD2 > svOutVec( length );

		int n = rand()%length;

		std::vector<int> map(length);
	    for(int i=0;i<length;i++)
		  map[i]= length-1-i;
		bolt::amp::device_vector<int> dmap( map.begin( ), map.end( ) );

        /*Generate inputs*/
        gen_input_udd genUDD;
        bolt::amp::generate(svIn1Vec.begin(), svIn1Vec.end(), genUDD);

        bolt::amp::device_vector< UDD2 > dvIn1Vec( svIn1Vec.begin(), svIn1Vec.end() );

        typedef std::vector< UDD2>::const_iterator                                                     sv_itr;
        typedef bolt::amp::device_vector< UDD2 >::iterator                                            dv_itr;
        typedef bolt::amp::counting_iterator< UDD2 >                                                  counting_itr;
        typedef bolt::amp::constant_iterator< UDD2 >                                                  constant_itr;
		typedef bolt::amp::permutation_iterator< std::vector<UDD2>::iterator,
                                             std::vector<int>::iterator> sv_trf_itr;
		typedef bolt::amp::permutation_iterator< bolt::amp::device_vector<UDD2>::iterator,
                                             bolt::amp::device_vector<int>::iterator> dv_trf_itr;
     
        /*Create Iterators*/

		//sv_trf_itr sv_trf_begin1 = bolt::amp::make_permutation_iterator(svIn1Vec.begin(), map.begin());
		//sv_trf_itr sv_trf_end1 = bolt::amp::make_permutation_iterator(svIn1Vec.end(), map.end());

		dv_trf_itr dv_trf_begin1 = bolt::amp::make_permutation_iterator(dvIn1Vec.begin(), dmap.begin());
		dv_trf_itr dv_trf_end1 = bolt::amp::make_permutation_iterator(dvIn1Vec.end(), dmap.end());

		UDD2 temp;
		temp.i=1, temp.f=2.5f;

		UDD2 init;
		init.i=0, init.f=0.0f;

        counting_itr count_itr_begin(init);
        counting_itr count_itr_end = count_itr_begin + length;

        constant_itr const_itr_begin(temp);
        constant_itr const_itr_end = const_itr_begin + length;

      {/*Test case when input is permutation Iterator*/
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


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose amp


    int retVal = RUN_ALL_TESTS( );

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
