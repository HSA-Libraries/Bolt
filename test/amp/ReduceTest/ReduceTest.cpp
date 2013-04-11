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

#define ENABLE_GTEST 1


#if !ENABLE_GTEST

    #include "stdafx.h"
    #include <stdio.h>

    #include <numeric>
    #include <limits>
    #include <bolt/AMP/functional.h>
    #include <bolt/AMP/reduce.h>

    #include <list>	// For debugging purposes, to prove that we can reject lists

    template<typename T>
    void printCheckMessage(bool err, std::string msg, T  stlResult, T boltResult)
    {
      if (err) {
        std::cout << "*ERROR ";
      } else {
        std::cout << "PASSED ";
      }

      std::cout << msg << "  STL=" << stlResult << " BOLT=" << boltResult << std::endl;
    };

    template<typename T>
    bool checkResult(std::string msg, T  stlResult, T boltResult)
    {
      bool err =  (stlResult != boltResult);
      printCheckMessage(err, msg, stlResult, boltResult);

      return err;
    };


    // For comparing floating point values:
    template<typename T>
    bool checkResult(std::string msg, T  stlResult, T boltResult, double errorThresh)
    {
      bool err;
      if ((errorThresh != 0.0) && stlResult) {
        double ratio = (double)(boltResult) / (double)(stlResult) - 1.0;
        err = abs(ratio) > errorThresh;
      } else {
        // Avoid div-by-zero, check for exact match.
        err = (stlResult != boltResult);
      }

      printCheckMessage(err, msg, stlResult, boltResult);
      return err;
    };

    // Simple test case for bolt::reduce:
    // Sum together specified numbers, compare against STL::accumulate function.
    // Demonstrates:
    //    * use of bolt with STL::array iterators
    //    * use of bolt with default plus 
    //    * use of bolt with explicit plus argument
    void simpleReduceArray( )
    {
      const unsigned int arraySize = 961;

        std::vector< int > A(arraySize);

      for (int i=0; i < arraySize; i++) {
        A[i(= 1;
      };

      int stlReduce = std::accumulate(A.begin(), A.end(), 0);

        for (int i=0; i < arraySize; i++) {
        A[i] = 1;
      };

      int boltReduce = bolt::amp::reduce(A.begin(), A.end(), 0, bolt::plus<int>());
      //int boltReduce2 = bolt::amp::reduce(A.begin(), A.end(), 0);  // same as above...
      //int boltReduce3 = bolt::amp::reduce(A.rbegin(), A.rend(), 0);  // reverse iterators should not be supported

      printf ("Sum: stl=%d,  bolt=%d\n", stlReduce, boltReduce);
    };



    // Test driver function
    void simpleReduce()
    {
      simpleReduceArray( );
      //simpleReduce1(1024);
      //simpleReduce1(1024000);
      //simpleReduce2();
        //
      //simpleReduce3<float> ("sum", bolt::plus<float>(), 1000000, .0001/*errorThresh*/);
      //simpleReduce3<float> ("min", bolt::minimum<float>(), 1000000);
      //simpleReduce3<float> ("max", bolt::maximum<float>(), 1000000);

      //simpleReduce4();
    };


    int _tmain(int argc, _TCHAR* argv[])
    {
      simpleReduce();
      return 0;
    }
#else

#include "common/stdafx.h"

#include "bolt/amp/reduce.h"
#include "bolt/amp/functional.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <array>
#include "common/test_common.h"
#include <algorithm>
#include <type_traits>
#define TEST_CPU_DEVICE 0
#define TEST_DOUBLE 0
#define TEST_DEVICE_VECTOR 0

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process type parameterized tests

//  This class creates a C++ 'TYPE' out of a size_t value
template< size_t N >
class TypeValue
{
public:
    static const size_t value = N;
};
template <typename T>
T generateRandom()
{
    double value = rand();
    static bool negate = true;
    if (negate)
    {
        negate = false;
        return -(T)fmod(value, 10.0);
    }
    else
    {
        negate = true;
        return (T)fmod(value, 10.0);
    }
}
//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class ReduceArrayTest: public ::testing::Test
{
public:
    ReduceArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<ArrayType>);
        stdOutput = stdInput;
        boltInput = stdInput;
        boltOutput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~ReduceArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput, stdOutput, boltOutput;
    int m_Errors;
};

TYPED_TEST_CASE_P( ReduceArrayTest );

TYPED_TEST_P( ReduceArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    ArrayType init(0);
    //  Calling the actual functions under test
    ArrayType stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    ArrayType boltReduce = bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init,
                                                       bolt::amp::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}

TYPED_TEST_P( ReduceArrayTest, GPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    ArrayType init(0);
    //  Calling the actual functions under test
    ArrayType stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    ArrayType boltReduce = bolt::amp::reduce(boltInput.begin( ), boltInput.end( ), init, bolt::amp::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );

}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( ReduceArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    ArrayType init(0);
    //  Calling the actual functions under test
    ArrayType stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    bolt::amp::control ctl;
    concurrency::accelerator cpuAccelerator = concurrency::accelerator(concurrency::accelerator::cpu_accelerator);
    ctl.setAccelerator(cpuAccelerator);


    ArrayType boltReduce = bolt::cl::reduce( ctl, boltInput.begin( ), boltInput.end( ),
                                                       bolt::amp::square<ArrayType>(), init,
                                                       bolt::amp::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );    
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_P( ReduceArrayTest, MultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    ArrayType init(0);
    //  Calling the actual functions under test
    ArrayType stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    ArrayType boltReduce = bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init,
                                                       bolt::amp::plus<ArrayType>( ));

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );    
    // FIXME - releaseOcl(ocl);
}

TYPED_TEST_P( ReduceArrayTest, GPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
#if OCL_CONTEXT_BUG_WORKAROUND
  ::cl::Context myContext = bolt::cl::control::getDefault( ).context( );
    bolt::cl::control c_gpu( getQueueFromContext(myContext, CL_DEVICE_TYPE_GPU, 0 ));  
#else
    ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
    bolt::amp::control c_gpu(accel);
#endif


    ArrayType init(0);
    //  Calling the actual functions under test
    ArrayType stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    ArrayType boltReduce = bolt::amp::reduce( c_gpu,boltInput.begin( ), boltInput.end( ), init,
                                                       bolt::amp::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );    
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( TransformArrayTest, CPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    ArrayType init(0);
    //  Calling the actual functions under test
    bolt::amp::control c_cpu;
    concurrency::accelerator cpuAccelerator = concurrency::accelerator(concurrency::accelerator::cpu_accelerator);
    ctl.setAccelerator(cpuAccelerator);

    ArrayType stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    ArrayType boltReduce = bolt::cl::reduce( c_cpu,boltInput.begin( ), boltInput.end( ),init,
                                                       bolt::amp::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );    
    // FIXME - releaseOcl(ocl);
}
#endif

#if (TEST_CPU_DEVICE == 1)
REGISTER_TYPED_TEST_CASE_P( TransformArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           CPU_DeviceNormal, CPU_DeviceMultipliesFunction);
#else
REGISTER_TYPED_TEST_CASE_P( ReduceArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction );
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size

class ReduceIntegerVector: public ::testing::TestWithParam< int >
{
public:

    ReduceIntegerVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                               stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<int>);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< int > stdInput, boltInput, stdOutput, boltOutput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceFloatVector: public ::testing::TestWithParam< int >
{
public:
    ReduceFloatVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                             stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<float>);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< float > stdInput, boltInput, stdOutput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceDoubleVector: public ::testing::TestWithParam< int >
{
public:
    ReduceDoubleVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                              stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<double>);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< double > stdInput, boltInput, stdOutput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ReduceIntegerDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ),
                                     stdOutput( GetParam( ) ), boltOutput( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<int>);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< int > stdInput, stdOutput;
    bolt::amp::device_vector< int > boltInput, boltOutput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ReduceFloatDeviceVector( ): stdInput( GetParam( ) ), boltInput( stdInput ), boltOutput( stdInput )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<float>);
        stdOutput = stdInput;

        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< float > stdInput, stdOutput;
    bolt::amp::device_vector< float, concurrency::array_view > boltInput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ReduceDoubleDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ), boltOutput( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), generateRandom<double>);
        stdOutput = stdInput;

        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
        }
    }

protected:
    std::vector< double > stdInput, stdOutput;
    bolt::amp::device_vector< double > boltInput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ReduceIntegerNakedPointer( ): stdInput( new int[ GetParam( ) ] ), boltInput( new int[ GetParam( ) ] ), 
                                     stdOutput( new int[ GetParam( ) ] ), boltOutput( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, generateRandom<int>);
        for (int i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
};

protected:
     int* stdInput;
     int* boltInput;
     int* stdOutput;
     int* boltOutput;

};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ReduceFloatNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ReduceFloatNakedPointer( ): stdInput( new float[ GetParam( ) ] ), boltInput( new float[ GetParam( ) ] ), 
                                   stdOutput( new float[ GetParam( ) ] ), boltOutput( new float[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, generateRandom<float>);
        for (int i = 0; i<size; i++)
        {
            boltInput[i] = stdInput[i];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
    };

protected:
     float* stdInput;
     float* boltInput;
     float* stdOutput;
     float* boltOutput;
};

class ReduceStdVectWithInit :public ::testing::TestWithParam<int>{
protected:
    int mySize;
public:	
    ReduceStdVectWithInit():mySize(GetParam()){
    }
};

TEST_P( ReduceStdVectWithInit, withIntWdInit)
{
    std::vector<int> stdInput( mySize );
    std::vector<int> stdOutput( mySize );
    std::vector<int> boltInput( mySize );

    for (int i = 0; i < mySize; ++i)
    {
        stdInput[i] = i;
        boltInput[i] = stdInput[i];
    }
    
    //  Calling the actual functions under test
    int init = 10;
    int stlTransformReduce = std::accumulate(stdInput.begin( ), stdInput.end( ), init, bolt::amp::plus<int>( ) );
    int boltTransformReduce= bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init, bolt::amp::plus<int>( ) );

    EXPECT_EQ( stlTransformReduce, boltTransformReduce );
}

TEST_P( ReduceStdVectWithInit, withIntWdInitWithStdPlus)
{
    //int mySize = 10;
    int init = 10;

    std::vector<int> stdInput (mySize);
    std::vector<int> stdOutput (mySize);

    std::vector<int> boltInput (mySize);
    //std::vector<int> boltOutput (mySize);

    for (int i = 0; i < mySize; ++i){
        stdInput[i] = i;
        boltInput[i] = stdInput[i];
    }
    
    //  Calling the actual functions under test
    int stlTransformReduce = std::accumulate(stdInput.begin(), stdInput.end(), init, std::plus<int>());
    int boltTransformReduce= bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init, bolt::amp::plus<int>());

    EXPECT_EQ(stlTransformReduce, boltTransformReduce);
}

TEST_P( ReduceStdVectWithInit, withIntWdInitWdAnyFunctor)
{
    //int mySize = 10;
    int init = 10;

    std::vector<int> stdInput (mySize);
    std::vector<int> stdOutput (mySize);

    std::vector<int> boltInput (mySize);
    //std::vector<int> boltOutput (mySize);

    for (int i = 0; i < mySize; ++i){
        stdInput[i] = i;
        boltInput[i] = stdInput[i];
    }
    
    //  Calling the actual functions under test
    int stlTransformReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    int boltTransformReduce= bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init, bolt::amp::plus<int>());

    EXPECT_EQ(stlTransformReduce, boltTransformReduce);
}

INSTANTIATE_TEST_CASE_P( withIntWithInitValue, ReduceStdVectWithInit, ::testing::Range(1, 100, 1) );

class ReduceTestMultFloat: public ::testing::TestWithParam<int>{
protected:
    int arraySize;
public:
    ReduceTestMultFloat( ):arraySize( GetParam( ) )
    {}
};

TEST_P (ReduceTestMultFloat, multiplyWithFloats)
{
    float* myArray = new float[ arraySize ];
    float* myArray2 = new float[ arraySize ];
    float* myBoltArray = new float[ arraySize ];

    myArray[ 0 ] = 1.0f;
    myBoltArray[ 0 ] = 1.0f;
    for( int i=1; i < arraySize; i++ )
    {
        myArray[i] = myArray[i-1] + 0.0625f;
        myBoltArray[i] = myArray[i];
    }

    float stlTransformReduce = std::accumulate(myArray, myArray + arraySize, 1.0f, std::multiplies<float>());
    float boltTransformReduce = bolt::amp::reduce(myBoltArray, myBoltArray + arraySize,
                                                  1.0f, bolt::amp::multiplies<float>());

    EXPECT_FLOAT_EQ(stlTransformReduce , boltTransformReduce )<<"Values does not match\n";

    delete [] myArray;
    delete [] myArray2;
    delete [] myBoltArray;
}

TEST_P( ReduceTestMultFloat, serialFloatValuesWdControl )
{
    std::vector<float> A( arraySize );
    std::vector<float> B( arraySize );
    std::vector<float> boltVect( arraySize );
    
    float myFloatValues = 9.0625f;

    for( int i=0; i < arraySize; ++i )
    {
        A[i] = myFloatValues + float(i);
        boltVect[i] = A[i];
    }

    float stdTransformReduceValue = std::accumulate(A.begin(), A.end(), 0.0f, std::plus<float>());
    float boltClTransformReduce = bolt::amp::reduce(boltVect.begin(), boltVect.end(), 0.0f, bolt::amp::plus<float>());

    //compare these results with each other
    EXPECT_FLOAT_EQ( stdTransformReduceValue, boltClTransformReduce );
}

INSTANTIATE_TEST_CASE_P(serialValues, ReduceTestMultFloat, ::testing::Range(1, 100, 10));
INSTANTIATE_TEST_CASE_P(multiplyWithFloatPredicate, ReduceTestMultFloat, ::testing::Range(1, 20, 1));
//end of new 2

class ReduceTestMultDouble: public ::testing::TestWithParam<int>{
protected:
        int arraySize;
public:
    ReduceTestMultDouble():arraySize(GetParam()){
    }
};

TEST_P (ReduceTestMultDouble, multiplyWithDouble)
{
    double* myArray = new double[ arraySize ];
    double* myArray2 = new double[ arraySize ];
    double* myBoltArray = new double[ arraySize ];
    
    for (int i=0; i < arraySize; i++)
    {
        myArray[i] = (double)i + 1.25;
        myBoltArray[i] = myArray[i];
    }

    double stlTransformReduce = std::accumulate(myArray, myArray + arraySize, 1.0, std::multiplies<double>());

    double boltTransformReduce = bolt::amp::reduce(myBoltArray, myBoltArray + arraySize, 1.0, bolt::amp::multiplies<double>());
    
    EXPECT_DOUBLE_EQ(stlTransformReduce , boltTransformReduce )<<"Values does not match\n";

    delete [] myArray;
    delete [] myArray2;
    delete [] myBoltArray;
}

INSTANTIATE_TEST_CASE_P( multiplyWithDoublePredicate, ReduceTestMultDouble, ::testing::Range(1, 20, 1) );

#if (TEST_DOUBLE ==1 )
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformDoubleNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformDoubleNakedPointer( ): stdInput( new double[ GetParam( ) ] ), boltInput( new double[ GetParam( ) ] ),
                                    stdOutput( new double[ GetParam( ) ] ), boltOutput( new double[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, generateRandom<double>);

        for( int i=0; i < size; i++ )
        {
            boltInput[ i ] = stdInput[ i ];
            boltOutput[i] = stdInput[i];
            stdOutput[i] = stdInput[i];
        }
    };

    virtual void TearDown( )
    {
        delete [] stdInput;
        delete [] boltInput;
        delete [] stdOutput;
        delete [] boltOutput;
    };

protected:
     double* stdInput;
     double* boltInput;
     double* stdOutput;
     double* boltOutput;
};
#endif

TEST_P( ReduceIntegerVector, Normal )
{

    int init(0);
    //  Calling the actual functions under test
    int stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    int boltReduce = bolt::amp::reduce( boltInput.begin( ), boltInput.end( ), init,
                                                       bolt::amp::plus<int>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );

}

TEST_P( ReduceFloatVector, Normal )
{
    float init(0);
    //  Calling the actual functions under test
    float stlReduce = std::accumulate(stdInput.begin(), stdInput.end(), init);

    float boltReduce = bolt::amp::reduce( boltInput.begin( ), boltInput.end( ),init,
                                                       bolt::amp::plus<float>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#if (TEST_DOUBLE == 1)
TEST_P( ReduceDoubleVector, Inplace )
{
    double init(0);
    //  Calling the actual functions under test
    std::transform(stdInput.begin(), stdInput.end(), stdOutput.begin(), bolt::amp::negate<double>());
    double stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    double boltReduce = bolt::cl::transform_reduce( boltInput.begin( ), boltInput.end( ),
                                                       bolt::amp::negate<double>(), init,
                                                       bolt::amp::plus<double>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#endif
#if (TEST_DEVICE_VECTOR == 1)
TEST_P( TransformIntegerDeviceVector, Inplace )
{
    int init(0);
    //  Calling the actual functions under test
    std::transform(stdInput.begin(), stdInput.end(), stdOutput.begin(), bolt::amp::negate<int>());
    int stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    int boltReduce = bolt::cl::transform_reduce( boltInput.begin( ), boltInput.end( ),
                                                       bolt::amp::negate<int>(), init,
                                                       bolt::amp::plus<int>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
TEST_P( TransformFloatDeviceVector, Inplace )
{
    float init(0);
    //  Calling the actual functions under test
    std::transform(stdInput.begin(), stdInput.end(), stdOutput.begin(), bolt::amp::negate<float>());
    float stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    float boltReduce = bolt::cl::transform_reduce( boltInput.begin( ), boltInput.end( ),
                                                       bolt::cl::negate<float>(), init,
                                                       bolt::cl::plus<float>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#if (TEST_DOUBLE == 1)
TEST_P( TransformDoubleDeviceVector, Inplace )
{
    double init(0);
    //  Calling the actual functions under test
    std::transform(stdInput.begin(), stdInput.end(), stdOutput.begin(), bolt::cl::negate<double>());
    double stlReduce = std::accumulate(stdOutput.begin(), stdOutput.end(), init);

    double boltReduce = bolt::cl::transform_reduce( boltInput.begin( ), boltInput.end( ),
                                                       bolt::cl::negate<double>(), init,
                                                       bolt::cl::plus<double>());

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#endif
#endif

TEST_P( ReduceIntegerNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< int* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );

    int init(0);
    //  Calling the actual functions under test
    int stlReduce = std::accumulate(wrapStdInput,wrapStdInput + endIndex, init);

    int boltReduce = bolt::amp::reduce( wrapBoltInput, wrapBoltInput + endIndex, init,
                                                       bolt::amp::plus<int>());

    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}

TEST_P( ReduceFloatNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< float* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );

    float init(0);
    //  Calling the actual functions under test
    float stlReduce = std::accumulate(wrapStdInput,wrapStdInput + endIndex, init);

    float boltReduce = bolt::amp::reduce( wrapBoltInput, wrapBoltInput + endIndex, init,
                                                       bolt::amp::plus<float>());

    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}


#if (TEST_DOUBLE == 1)
TEST_P( TransformDoubleNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< double* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );

    double init(0);
    //  Calling the actual functions under test
    std::transform(wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::cl::negate<double>());
    double stlReduce = std::accumulate(wrapStdOutput,wrapStdOutput + endIndex, init);

    double boltReduce = bolt::cl::transform_reduce( wrapBoltInput, wrapBoltInput + endIndex,
                                                       bolt::cl::negate<double>(), init,
                                                       bolt::cl::plus<double>());

    EXPECT_EQ( stlReduce, boltReduce );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput, endIndex );
}
#endif
std::array<int, 15> TestValues = {2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
//Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceIntegerVector, ::testing::Range( 0, 1024, 7 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceIntegerVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceFloatVector, ::testing::Range( 0, 1024, 3 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceFloatVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceDoubleVector, ::testing::Range( 0, 1024, 21 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceDoubleVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceIntegerDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceIntegerDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceFloatDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceFloatDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceDoubleDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceDoubleDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceIntegerNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceIntegerNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceFloatNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( ReduceValues, ReduceFloatNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( ReduceRange, ReduceDoubleNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( Reduce, ReduceDoubleNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif

typedef ::testing::Types< 
    std::tuple< int, TypeValue< 1 > >,
    std::tuple< int, TypeValue< 31 > >,
    std::tuple< int, TypeValue< 32 > >,
    std::tuple< int, TypeValue< 63 > >,
    std::tuple< int, TypeValue< 64 > >,
    std::tuple< int, TypeValue< 127 > >,
    std::tuple< int, TypeValue< 128 > >,
    std::tuple< int, TypeValue< 129 > >,
    std::tuple< int, TypeValue< 1000 > >,
    std::tuple< int, TypeValue< 1053 > >,
    std::tuple< int, TypeValue< 4096 > >,
    std::tuple< int, TypeValue< 4097 > >,
    std::tuple< int, TypeValue< 65535 > >,
    std::tuple< int, TypeValue< 65536 > >
> IntegerTests;

typedef ::testing::Types< 
    std::tuple< float, TypeValue< 1 > >,
    std::tuple< float, TypeValue< 31 > >,
    std::tuple< float, TypeValue< 32 > >,
    std::tuple< float, TypeValue< 63 > >,
    std::tuple< float, TypeValue< 64 > >,
    std::tuple< float, TypeValue< 127 > >,
    std::tuple< float, TypeValue< 128 > >,
    std::tuple< float, TypeValue< 129 > >,
    std::tuple< float, TypeValue< 1000 > >,
    std::tuple< float, TypeValue< 1053 > >,
    std::tuple< float, TypeValue< 4096 > >,
    std::tuple< float, TypeValue< 4097 > >,
    std::tuple< float, TypeValue< 65535 > >,
    std::tuple< float, TypeValue< 65536 > >
> FloatTests;

#if (TEST_DOUBLE == 1)
typedef ::testing::Types< 
    std::tuple< double, TypeValue< 1 > >,
    std::tuple< double, TypeValue< 31 > >,
    std::tuple< double, TypeValue< 32 > >,
    std::tuple< double, TypeValue< 63 > >,
    std::tuple< double, TypeValue< 64 > >,
    std::tuple< double, TypeValue< 127 > >,
    std::tuple< double, TypeValue< 128 > >,
    std::tuple< double, TypeValue< 129 > >,
    std::tuple< double, TypeValue< 1000 > >,
    std::tuple< double, TypeValue< 1053 > >,
    std::tuple< double, TypeValue< 4096 > >,
    std::tuple< double, TypeValue< 4097 > >,
    std::tuple< double, TypeValue< 65535 > >,
    std::tuple< double, TypeValue< 65536 > >
> DoubleTests;
#endif 

struct UDD { 
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) const restrict (cpu,amp) {
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const restrict (cpu,amp){
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const restrict (cpu,amp){
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const restrict (cpu,amp){
        return ((a+b) == (other.a+other.b));
    }
    UDD operator + (const UDD &rhs) const restrict (amp,cpu){
                UDD tmp = *this;
                tmp.a = tmp.a + rhs.a;
                tmp.b = tmp.b + rhs.b;
                return tmp;
    }
    UDD()   restrict (cpu,amp)
        : a(0),b(0) { }
    UDD(int _in)   restrict (cpu,amp)
        : a(_in), b(_in +1)  { }
}; 

struct UDDplus
{
   UDD operator() (const UDD &lhs, const UDD &rhs) const restrict (cpu,amp)
   {
     UDD _result;
     _result.a = lhs.a + rhs.a;
     _result.b = lhs.b + rhs.b;
     return _result;
   }

};


TEST( ReduceUDD , UDDPlusOperatorInts )
{
    //setup containers
    int length = 1024;
    std::vector< UDD > refInput( length );
    for( int i = 0; i < length ; i++ )
    {
      refInput[i].a = i;
      refInput[i].b = i+1;
    }

    UDD zero;
    zero.a = 0;
    zero.b = 0;
    // call reduce
    UDDplus plusOp;
    UDD boltReduce = bolt::amp::reduce( refInput.begin(), refInput.end(), zero, plusOp );
    UDD stdReduce =  std::accumulate( refInput.begin(), refInput.end(), zero, plusOp ); // out-of-place scan

    EXPECT_EQ(boltReduce,stdReduce);

}

INSTANTIATE_TYPED_TEST_CASE_P( Integer, ReduceArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, ReduceArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, ReduceArrayTest, DoubleTests );
#endif 


void testTBB()
{
    const int aSize = 1<<24;
    std::vector<int> stdInput(aSize);
    std::vector<int> tbbInput(aSize);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = 2;
        tbbInput[i] = 2;
    };

    int hSum = std::accumulate(stdInput.begin(), stdInput.end(), 2);
    bolt::amp::control ctl = bolt::amp::control::getDefault();
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); // serial path also tested
    int sum = bolt::amp::reduce(ctl, tbbInput.begin(), tbbInput.end(), 2);
    if(hSum == sum)
        printf ("\nTBB Test case PASSED %d %d\n", hSum, sum);
    else
        printf ("\nTBB Test case FAILED\n");

};
void testdoubleTBB()
{
  const int aSize = 1<<24;
    std::vector<double> stdInput(aSize);
    std::vector<double> tbbInput(aSize);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = 3.0;
        tbbInput[i] = 3.0;
    };

    double hSum = std::accumulate(stdInput.begin(), stdInput.end(), 1.0);
    bolt::amp::control ctl = bolt::amp::control::getDefault();
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); // serial path also tested
    double sum = bolt::amp::reduce(ctl, tbbInput.begin(), tbbInput.end(), 1.0);
    if(hSum == sum)
        printf ("\nTBB Test case PASSED %lf %lf\n", hSum, sum);
    else
        printf ("\nTBB Test case FAILED\n");
}

void testUDDTBB()
{

    const int aSize = 1<<19;
    std::vector<UDD> stdInput(aSize);
    std::vector<UDD> tbbInput(aSize);

    UDD initial;
    initial.a = 1;
    initial.b = 2;

    for(int i=0; i<aSize; i++) {
        stdInput[i].a = 2;
        stdInput[i].b = 3;
        tbbInput[i].a = 2;
        tbbInput[i].b = 3;

    };
    bolt::amp::plus<UDD> add;
    UDD hSum = std::accumulate(stdInput.begin(), stdInput.end(), initial,add);
    bolt::amp::control ctl = bolt::amp::control::getDefault();
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); // serial path also tested
    UDD sum = bolt::amp::reduce(ctl, tbbInput.begin(), tbbInput.end(), initial, add);
    if(hSum == sum)
        printf ("\nUDDTBB Test case PASSED %d %d %d %d\n", hSum.a, sum.a, hSum.b, sum.b);
    else
        printf ("\nUDDTBB Test case FAILED\n");
        
}


TEST( ReduceFunctor, NormalLambdaFunctor )
{
  
    int init(0);
    //  Calling the actual functions under test
    std::vector<int> stdInput(1024), boltInput(1024);
    std::fill( stdInput.begin(), stdInput.end(), 100 );
    std::copy( stdInput.begin(), stdInput.end(), boltInput.begin() );

    int stlReduce = std::accumulate(stdInput.begin(),
                                    stdInput.end(),
                                    init,
                                    []( int x, int y ) restrict (amp,cpu){return x+y;}
                                   );

    int boltReduce = bolt::amp::reduce( boltInput.begin( ),
                                        boltInput.end( ),
                                        init,
                                        []( int x, int y ) restrict (amp,cpu){return x+y;}
                                      );

    size_t stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    size_t boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );
    //EXPECT_EQ( stlReduce, boltReduce );
}

void testTBBDevicevector()
{
    size_t aSize = 1<<16;
    std::vector<int> stdInput(aSize);
    bolt::amp::device_vector<int> tbbInput(aSize, 0);


    for(int i=0; i<aSize; i++) {
        stdInput[i] = i;
        tbbInput[i] = i;
    };

    int hSum = std::accumulate(stdInput.begin(), stdInput.end(), 0);
    bolt::amp::control ctl = bolt::amp::control::getDefault();
    ctl.setForceRunMode(bolt::amp::control::MultiCoreCpu); // serial path also tested
    int sum = bolt::amp::reduce(ctl, tbbInput.begin(), tbbInput.end(), 0);
    if(hSum == sum)
        printf ("\nTBBDevicevector Test case PASSED %d %d\n", hSum, sum);
    else
        printf ("\nTBBDevicevector Test case FAILED*********\n");


};


int main(int argc, char* argv[])
{

#if defined( ENABLE_TBB )
    testTBB( );
    testdoubleTBB();
    testUDDTBB();
    testTBBDevicevector();
#endif
    
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

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







#endif