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

//#include "common/stdafx.h"
#include <vector>
//#include <array>
#include "bolt/cl/bolt.h"
//#include "bolt/cl/scan.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/scan_by_key.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>


#if 1


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track

/******************************************************************************
 *  Double x4
 *****************************************************************************/
BOLT_FUNCTOR(uddtD4,
struct uddtD4
{
    double a;
    double b;
    double c;
    double d;

    bool operator==(const uddtD4& rhs) const
    {
        bool equal = true;
        double th = 0.0000000001;
        if (rhs.a < th && rhs.a > -th)
            equal = ( (1.0*a - rhs.a) < th && (1.0*a - rhs.a) > -th) ? equal : false;
        else
            equal = ( (1.0*a - rhs.a)/rhs.a < th && (1.0*a - rhs.a)/rhs.a > -th) ? equal : false;
        if (rhs.b < th && rhs.b > -th)
            equal = ( (1.0*b - rhs.b) < th && (1.0*b - rhs.b) > -th) ? equal : false;
        else
            equal = ( (1.0*b - rhs.b)/rhs.b < th && (1.0*b - rhs.b)/rhs.b > -th) ? equal : false;
        if (rhs.c < th && rhs.c > -th)
            equal = ( (1.0*c - rhs.c) < th && (1.0*c - rhs.c) > -th) ? equal : false;
        else
            equal = ( (1.0*c - rhs.c)/rhs.c < th && (1.0*c - rhs.c)/rhs.c > -th) ? equal : false;
        if (rhs.d < th && rhs.d > -th)
            equal = ( (1.0*d - rhs.d) < th && (1.0*d - rhs.d) > -th) ? equal : false;
        else
            equal = ( (1.0*d - rhs.d)/rhs.d < th && (1.0*d - rhs.d)/rhs.d > -th) ? equal : false;
        return equal;
    }
    uddtD4 operator-() const
    {
        uddtD4 r;
        r.a = -a;
        r.b = -b;
        r.c = -c;
        r.d = -d;
        return r;
    }
    uddtD4 operator*(const uddtD4& rhs)
    {
        uddtD4 r;
        r.a = a*a;
        r.b = b*b;
        r.c = c*c;
        r.d = d*d;
        return r;
    }
};
);
BOLT_FUNCTOR(MultD4,
struct MultD4
{
    uddtD4 operator()(const uddtD4 &lhs, const uddtD4 &rhs) const
    {
        uddtD4 _result;
        _result.a = lhs.a*rhs.a;
        _result.b = lhs.b*rhs.b;
        _result.c = lhs.c*rhs.c;
        _result.d = lhs.d*rhs.d;
        return _result;
    };
}; 
);
uddtD4 identityMultD4 = { 1.0, 1.0, 1.0, 1.0 };
uddtD4 initialMultD4  = { 1.00001, 1.000003, 1.0000005, 1.00000007 };


/******************************************************************************
 *  Integer x2
 *****************************************************************************/
BOLT_FUNCTOR(uddtI2,
struct uddtI2
{
    int a;
    int b;

    bool operator==(const uddtI2& rhs) const
    {
        bool equal = true;
        equal = ( a == rhs.a ) ? equal : false;
        equal = ( b == rhs.b ) ? equal : false;
        return equal;
    }
    uddtI2 operator-() const
    {
        uddtI2 r;
        r.a = -a;
        r.b = -b;
        return r;
    }
    uddtI2 operator*(const uddtI2& rhs)
    {
        uddtI2 r;
        r.a = a*a;
        r.b = b*b;
        return r;
    }
};
);
BOLT_FUNCTOR(AddI2,
struct AddI2
{
    uddtI2 operator()(const uddtI2 &lhs, const uddtI2 &rhs) const
    {
        uddtI2 _result;
        _result.a = lhs.a+rhs.a;
        _result.b = lhs.b+rhs.b;
        return _result;
    };
}; 
);
uddtI2 identityAddI2 = {  0, 0 };
uddtI2 initialAddI2  = { -1, 2 };


/******************************************************************************
 *  Mixed float and int
 *****************************************************************************/
BOLT_FUNCTOR(uddtM3,
struct uddtM3
{
    int a;
    int        b;
    double       c;

    bool operator==(const uddtM3& rhs) const
    {
        bool equal = true;
        double ths = 0.0001;
        double thd = 0.000000001;
        equal = ( a == rhs.a ) ? equal : false;
        equal = ( b == rhs.b ) ? equal : false;
        //if (rhs.b < ths && rhs.b > -ths)
        //    equal = ( (1.0*b - rhs.b) < ths && (1.0*b - rhs.b) > -ths) ? equal : false;
        //else
        //    equal = ( (1.0*b - rhs.b)/rhs.b < ths && (1.0*b - rhs.b)/rhs.b > -ths) ? equal : false;
        if (rhs.c < thd && rhs.c > -thd)
            equal = ( (1.0*c - rhs.c) < thd && (1.0*c - rhs.c) > -thd) ? equal : false;
        else
            equal = ( (1.0*c - rhs.c)/rhs.c < thd && (1.0*c - rhs.c)/rhs.c > -thd) ? equal : false;
        return equal;
    }
    uddtM3 operator-() const
    {
        uddtM3 r;
        r.a = -a;
        r.b = -b;
        r.c = -c;
        return r;
    }
    uddtM3 operator*(const uddtM3& rhs)
    {
        uddtM3 r;
        r.a = a*a;
        r.b = b*b;
        r.c = c*c;
        return r;
    }
};
);
BOLT_FUNCTOR(MixM3,
struct MixM3
{
    uddtM3 operator()(const uddtM3 &lhs, const uddtM3 &rhs) const
    {
        uddtM3 _result;
        _result.a = lhs.a^rhs.a;
        _result.b = lhs.b+rhs.b;
        _result.c = lhs.c*rhs.c;
        return _result;
    };
}; 
);
uddtM3 identityMixM3 = { 0, 0, 1.0 };
uddtM3 initialMixM3  = { 2, 1, 1.000001 };

BOLT_FUNCTOR(uddtM2,
struct uddtM2
{
    int a;
    float b;

    bool operator==(const uddtM2& rhs) const
    {
        bool equal = true;
        float ths = 0.00001; // thresh hold single(float)
        equal = ( a == rhs.a ) ? equal : false;
        if (rhs.b < ths && rhs.b > -ths)
            equal = ( (1.0*b - rhs.b) < ths && (1.0*b - rhs.b) > -ths) ? equal : false;
        else
            equal = ( (1.0*b - rhs.b)/rhs.b < ths && (1.0*b - rhs.b)/rhs.b > -ths) ? equal : false;
        return equal;
    }
    uddtM2 operator-() const
    {
        uddtM2 r;
        r.a = -a;
        r.b = -b;
        return r;
    }
    uddtM2 operator*(const uddtM2& rhs)
    {
        uddtM2 r;
        r.a = a*a;
        r.b = b*b;
        return r;
    }
    void operator++()
    {
        a += 1;
        b += 1.234567f;
    }
};
);

BOLT_FUNCTOR(MixM2,
struct MixM2
{
    uddtM2 operator()(const uddtM2 &lhs, const uddtM2 &rhs) const
    {
        uddtM2 _result;
        _result.a = lhs.a^rhs.a;
        _result.b = lhs.b+rhs.b;
        return _result;
    };
}; 
);


uddtM2 identityMixM2 = { 0, 3.141596f };
uddtM2 initialMixM2  = { 2, 1.000001f };


BOLT_FUNCTOR(uddtM2_equal_to,
struct uddtM2_equal_to
{
    bool operator()(const uddtM2& lhs, const uddtM2& rhs) const
    {
        return lhs == rhs;
    }
};
);


#include "test_common.h"


/******************************************************************************
 *  Scan with User Defined Data Types and Operators
 *****************************************************************************/


template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator,
    typename BinaryFunction>
OutputIterator
gold_scan_by_key(
    InputIterator1 firstKey,
    InputIterator1 lastKey,
    InputIterator2 values,
    OutputIterator result,
    BinaryFunction binary_op)
{
    typedef std::iterator_traits< InputIterator1 >::value_type kType;
    typedef std::iterator_traits< InputIterator2 >::value_type vType;
    typedef std::iterator_traits< OutputIterator >::value_type oType;

    static_assert( std::is_convertible< vType, oType >::value,
        "InputIterator2 and OutputIterator's value types are not convertible." );

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
    typename OutputIterator,
    typename BinaryFunction,
    typename T>
OutputIterator
gold_scan_by_key_exclusive(
    InputIterator1 firstKey,
    InputIterator1 lastKey,
    InputIterator2 values,
    OutputIterator result,
    BinaryFunction binary_op,
    T init)
{
    typedef std::iterator_traits< InputIterator1 >::value_type kType;
    typedef std::iterator_traits< InputIterator2 >::value_type vType;
    typedef std::iterator_traits< OutputIterator >::value_type oType;

    static_assert( std::is_convertible< vType, oType >::value,
        "InputIterator2 and OutputIterator's value types are not convertible." );

    // do zeroeth element
    //*result = *values; // assign value
    *result = (vType)init;


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
           // std::cout << "new segment" << std::endl;
            *result = (vType)init;
        }
    }

    return result;
}



TEST(InclusiveScanByKey, IncMixedM3increment)
{
    //setup keys
    int length = (1<<24);
    std::vector< uddtM2 > keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        
        keys[i] = key;
        segmentIndex++;
    }

    // input and output vectors for device and reference
    std::vector< uddtM3 > input(  length, initialMixM3 );
    std::vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );

    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    bolt::cl::inclusive_scan_by_key( keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);

#if 0
    // print Bolt scan_by_key
    for (int i = 0; i < length; i++)
    {
        if ( !(output[i] == refOutput[i]) ) {
        std::cout << "BOLT: i=" << i << ", ";
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << input[i].a << ", " << input[i].b << ", " << input[i].c << "}; ";
        std::cout << "out={" << output[i].a << ", " << output[i].b << ", " << output[i].c << "};" << std::endl;
    
        std::cout << "GOLD: i=" << i << ", ";
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << refInput[i].a << ", " << refInput[i].b << ", " << refInput[i].c << "}; ";
        std::cout << "out={" << refOutput[i].a << ", " << refOutput[i].b << ", " << refOutput[i].c << "};" << std::endl;
        }
    }
#endif

    // compare results
    cmpArrays(refOutput, output);
}

TEST(InclusiveScanByKey, IncMixedM3same)
{
    //setup keys
    int length = (1<<24);
    uddtM2 key = {1, 2.3f};
    std::vector< uddtM2 > keys( length, key);

    // input and output vectors for device and reference
    std::vector< uddtM3 > input(  length, initialMixM3 );
    std::vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );

    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    bolt::cl::inclusive_scan_by_key( keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);

#if 0
    std::cout.setf(std::ios_base::scientific);
    std::cout.precision(15);
    // print Bolt scan_by_key
    for (int i = 0; i < length; i++)
    {
        if ( !(output[i] == refOutput[i]) ) {
        std::cout.precision(3);
        std::cout << "BOLT: i=" << i << ", ";
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << input[i].a << ", " << input[i].b << ", " << input[i].c << "}; ";
        std::cout.precision(15);
        std::cout << "out={" << output[i].a << ", " << output[i].b << ", " << output[i].c << "};" << std::endl;
    
        std::cout << "GOLD: i=" << i << ", ";
        std::cout.precision(3);
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << refInput[i].a << ", " << refInput[i].b << ", " << refInput[i].c << "}; ";
        std::cout.precision(15);
        std::cout << "out={" << refOutput[i].a << ", " << refOutput[i].b << ", " << refOutput[i].c << "};" << std::endl;
        }
    }
#endif

    // compare results
    cmpArrays(refOutput, output);
}

TEST(InclusiveScanByKey, IncMixedM3each)
{
    //setup keys
    int length = (1<<24);
    std::vector< uddtM2 > keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        ++key;
        keys[i] = key;
    }

    // input and output vectors for device and reference
    std::vector< uddtM3 > input(  length, initialMixM3 );
    std::vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );

    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    bolt::cl::inclusive_scan_by_key( keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);

#if 0
    // print Bolt scan_by_key
    for (int i = 0; i < length; i++)
    {
        if ( !(output[i] == refOutput[i]) ) {
        std::cout << "BOLT: i=" << i << ", ";
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << input[i].a << ", " << input[i].b << ", " << input[i].c << "}; ";
        std::cout << "out={" << output[i].a << ", " << output[i].b << ", " << output[i].c << "};" << std::endl;
    
        std::cout << "GOLD: i=" << i << ", ";
        std::cout << "key={" << keys[i].a << ", " << keys[i].b << "}; ";
        std::cout << "val={" << refInput[i].a << ", " << refInput[i].b << ", " << refInput[i].c << "}; ";
        std::cout << "out={" << refOutput[i].a << ", " << refOutput[i].b << ", " << refOutput[i].c << "};" << std::endl;
        }
    }
#endif

    // compare results
    cmpArrays(refOutput, output);
}

TEST( equalValMult, iValues )
{
    int keys[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
    int vals[11] = { 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 2 }; 
    int out[11]; 
   
    bolt::cl::equal_to<int> eq; 
    bolt::cl::multiplies<int> mult; 
   
    bolt::cl::inclusive_scan_by_key( keys, keys+11, vals, out, eq, mult ); 
   
    int arrToMatch[11] = { 2, 2, 4, 2, 4, 8, 2, 4, 8, 16, 2 };

    // compare results
    cmpArrays( arrToMatch, out );
}

/////////////////////////Inclusive//////////////////////////////////////////////////
TEST(InclusiveScanByKey, DeviceVectorInclUdd)
{
    //setup keys
    int length = 1<<16;

    std::vector< uddtM2 > keys( length, identityMixM2);
    bolt::cl::device_vector< uddtM2 > device_keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;

    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;
    }


    // input and output vectors for device and reference
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3 );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );
    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also

    bolt::cl::inclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), eq, mM3);

    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}



TEST(InclusiveScanByKey, DeviceVectorInclFloat)
{
    //setup keys
    int length = 1<<16;
    std::vector< int > keys( length);
    bolt::cl::device_vector< int > device_keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
           	segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector< float > input( length);
    bolt::cl::device_vector< float > output( length);
    std::vector< float > refInput( length);
    std::vector< float > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 2.0f;
        refInput[i] = 2.0f;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<float> mM3; 
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::inclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(InclusiveScanByKey, DeviceVectorInclDouble)
{
    //setup keys
    int length = 1<<16;
    std::vector< int > keys( length);
    bolt::cl::device_vector< int > device_keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
           	segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector< double > input( length);
    bolt::cl::device_vector< double > output( length);
    std::vector< double > refInput( length);
    std::vector< double > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 2.0;
        refInput[i] = 2.0;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<double> mM3; 
    // MixM3 mM3;
    // uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::inclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}




/////////////////////////Exclusive//////////////////////////////////////////////////
TEST(ExclusiveScanByKey, DeviceVectorExclFloat)
{
    //setup keys
    int length = 1<<16;
    std::vector< int > keys( length);
    bolt::cl::device_vector< int > device_keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is actually scan
        device_keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector< float > input( length);
    bolt::cl::device_vector< float > output( length);
    std::vector< float > refInput( length);
    std::vector< float > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0f;
        refInput[i] = 1.0f;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<float> mM3; 
  
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), 4.0f,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0f);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, DeviceVectorExclDouble)
{
    //setup keys
    int length = 1<<16;
    std::vector< int > keys( length);
    bolt::cl::device_vector< int > device_keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is just scan
        device_keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector< double > input( length);
    bolt::cl::device_vector< double > output( length);
    std::vector< double> refInput( length);
    std::vector< double > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0;
        refInput[i] = 1.0;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<double> mM3; 
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), 4.0,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, DeviceVectorExclUdd)
{
    //setup keys
    int length = 1<<16;
    std::vector< uddtM2 > keys( length, identityMixM2);
    bolt::cl::device_vector< uddtM2 > device_keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key ;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector< uddtM3 > input(  length, initialMixM3 );
    bolt::cl::device_vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );
    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, device_keys.begin(), device_keys.end(), input.begin(), output.begin(), initialMixM3,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, initialMixM3);
    // compare results
    cmpArrays(refOutput, output);
}



/////////////////////////////////////////////////////////////////////////////////////////////////









TEST(InclusiveScanByKey, MulticoreInclUdd)
{
    //setup keys
    int length = 1<<24;
    std::vector< uddtM2 > keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< uddtM3 > input(  length, initialMixM3 );
    std::vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );
    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    bolt::cl::inclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}


TEST(InclusiveScanByKey, MulticoreInclFloat)
{
    //setup keys
    int length = 1<<24;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
           	segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< float > input( length);
    std::vector< float > output( length);
    std::vector< float > refInput( length);
    std::vector< float > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0f;
        refInput[i] = 1.0f;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<float> mM3; 
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::inclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}


TEST(InclusiveScanByKey, MulticoreInclDouble)
{
    //setup keys
    int length = 1<<24;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
           	segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< double > input( length);
    std::vector< double > output( length);
    std::vector< double > refInput( length);
    std::vector< double > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0;
        refInput[i] = 1.0;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<double> mM3; 
    // MixM3 mM3;
    // uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::inclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), eq, mM3);
    gold_scan_by_key(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3);
    // compare results
    cmpArrays(refOutput, output);
}



TEST(ExclusiveScanByKey, MulticoreExclFloat)
{
    //setup keys
    int length = 1<<24;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is actually scan
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< float > input( length);
    std::vector< float > output( length);
    std::vector< float > refInput( length);
    std::vector< float > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0f;
        refInput[i] = 1.0f;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<float> mM3; 
  
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), 4.0f,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0f);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, MulticoreExclDouble)
{
    //setup keys
    int length = 1<<24;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is just scan
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< double > input( length);
    std::vector< double > output( length);
    std::vector< double> refInput( length);
    std::vector< double > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0;
        refInput[i] = 1.0;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<double> mM3; 
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), 4.0,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, MulticoreExclUdd)
{
    //setup keys
    int length = 1<<24;
    std::vector< uddtM2 > keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< uddtM3 > input(  length, initialMixM3 );
    std::vector< uddtM3 > output( length, identityMixM3 );
    std::vector< uddtM3 > refInput( length, initialMixM3 );
    std::vector< uddtM3 > refOutput( length );
    // call scan
    MixM3 mM3;
    uddtM2_equal_to eq;
    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu); // tested for serial cpu also
    bolt::cl::exclusive_scan_by_key(ctl, keys.begin(), keys.end(), input.begin(), output.begin(), initialMixM3,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, initialMixM3);
    // compare results
    cmpArrays(refOutput, output);
}

/////////////////////////////////////////////////CL Exclusive test Cases after fix ///////////////////////////

TEST(ExclusiveScanByKey, CLscanbykeyExclFloat)
{
    //setup keys
    int length = 1<<18;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is actually scan
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< float > input( length);
    std::vector< float > output( length);
    std::vector< float > refInput( length);
    std::vector< float > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0f;
        refInput[i] = 1.0f;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<float> mM3; 
  
    bolt::cl::exclusive_scan_by_key(keys.begin(), keys.end(), input.begin(), output.begin(), 4.0f,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0f);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, CLscanbykeyExclDouble)
{
    //setup keys
    int length = 1<<18;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    for (int i = 0; i < length; i++)
    {
        if (segmentIndex == segmentLength)
        {
    	      segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key; // tested with key = 1 also which is just scan
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< double > input( length);
    std::vector< double > output( length);
    std::vector< double> refInput( length);
    std::vector< double > refOutput( length);
    for(int i=0; i<length; i++) {
        input[i] = 1.0;
        refInput[i] = 1.0;
    }
    // call scan
    bolt::cl::equal_to<int> eq; 
    bolt::cl::plus<double> mM3; 
    bolt::cl::exclusive_scan_by_key(keys.begin(), keys.end(), input.begin(), output.begin(), 4.0,eq, mM3);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM3, 4.0);
    // compare results
    cmpArrays(refOutput, output);
}

TEST(ExclusiveScanByKey, CLscanbykeyExclUDD)
{
    //setup keys
    int length = 1<<18;
    std::vector< uddtM2 > keys( length, identityMixM2);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    uddtM2 key = identityMixM2;
    for (int i = 0; i < length; i++)
    {
        // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        segmentIndex++;
    }
    // input and output vectors for device and reference
    std::vector< uddtM2 > input(  length, initialMixM2 );
    std::vector< uddtM2 > output( length, identityMixM2 );
    std::vector< uddtM2 > refInput( length, initialMixM2 );
    std::vector< uddtM2 > refOutput( length );
    // call scan
    MixM2 mM2;
    uddtM2_equal_to eq;
    bolt::cl::exclusive_scan_by_key( keys.begin(), keys.end(), input.begin(), output.begin(), initialMixM2,eq, mM2);
    gold_scan_by_key_exclusive(keys.begin(), keys.end(), refInput.begin(), refOutput.begin(), mM2, initialMixM2);
    // compare results
    cmpArrays(refOutput, output);
}







// paste from above
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    //  Initialize googletest; this removes googletest specific flags from command line
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    bool print_clInfo = false;
    cl_uint userPlatform = 0;
    cl_uint userDevice = 0;
    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;

    try
    {
        // Declare supported options below, describe what they do
        boost::program_options::options_description desc( "Scan GoogleTest command line options" );
        desc.add_options()
            ( "help,h",         "produces this help message" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "platform,p",     boost::program_options::value< cl_uint >( &userPlatform )->default_value( 0 ),	"Specify the platform under test" )
            ( "device,d",       boost::program_options::value< cl_uint >( &userDevice )->default_value( 0 ),	"Specify the device under test" )
            ;


        boost::program_options::variables_map vm;
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc ), vm );
        boost::program_options::notify( vm );

        if( vm.count( "help" ) )
        {
            //	This needs to be 'cout' as program-options does not support wcout yet
            std::cout << desc << std::endl;
            return 0;
        }

        if( vm.count( "queryOpenCL" ) )
        {
            print_clInfo = true;
        }

        //  The following 3 options are not implemented yet; they are meant to be used with ::clCreateContextFromType()
        if( vm.count( "gpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_GPU;
        }
        
        if( vm.count( "cpu" ) )
        {
            deviceType	= CL_DEVICE_TYPE_CPU;
        }

        if( vm.count( "all" ) )
        {
            deviceType	= CL_DEVICE_TYPE_ALL;
        }

    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan GoogleTest error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    //  Query OpenCL for available platforms
    cl_int err = CL_SUCCESS;

    // Platform vector contains all available platforms on system
    std::vector< cl::Platform > platforms;
    //std::cout << "HelloCL!\nGetting Platform Information\n";
    bolt::cl::V_OPENCL( cl::Platform::get( &platforms ), "Platform::get() failed" );

    if( print_clInfo )
    {
        bolt::cl::control::printPlatforms( );
        return 0;
    }

    //  Do stuff with the platforms
    std::vector<cl::Platform>::iterator i;
    if(platforms.size() > 0)
    {
        for(i = platforms.begin(); i != platforms.end(); ++i)
        {
            if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str(), "Advanced Micro Devices, Inc."))
            {
                break;
            }
        }
    }
    bolt::cl::V_OPENCL( err, "Platform::getInfo() failed" );

    // Device info
    std::vector< cl::Device > devices;
    bolt::cl::V_OPENCL( platforms.front( ).getDevices( CL_DEVICE_TYPE_ALL, &devices ), "Platform::getDevices() failed" );

    cl::Context myContext( devices.at( userDevice ) );
    cl::CommandQueue myQueue( myContext, devices.at( userDevice ) );
    bolt::cl::control::getDefault( ).setCommandQueue( myQueue );

    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    std::cout << "Device under test : " << strDeviceName << std::endl;

    int retVal = RUN_ALL_TESTS( );

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