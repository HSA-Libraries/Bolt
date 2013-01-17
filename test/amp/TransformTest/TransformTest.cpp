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

#include "stdafx.h"

#include <vector>
#include <iostream>
#include <functional>
#include <algorithm>
#include <bolt/miniDump.h>
#include <bolt/amp/functional.h>

#include <gtest/gtest.h>
#include <boost/shared_array.hpp>
#include <array>

#include <bolt/amp/transform.h>
#define ENABLE_TESTS 1
#define GTEST_TESTS 0
#if ENABLE_TESTS
#if !GTEST_TESTS


    template<typename Container>
    static void printA2(const char * msg, const Container &a, const Container &b, int x_size) 
    {
	    std::wcout << msg << std::endl;
	    for (int i = 0; i < x_size; i++)
		    std::wcout << a[i] << "\t" << b[i] << std::endl;
    };

    static void printA(const char * msg, const int *a, int x_size) 
    {
	    std::wcout << msg << std::endl;
	    for (int i = 0; i < x_size; i++)
		    std::wcout << a[i] << std::endl;
    };



    /*
    * Demonstrates:
        * Use of bolt::transform function
        * Bolt delivers same result as stl::transform
        * Bolt syntax is similar to STL transform
        * Works for both integer arrays and STL vectors
        */
    void simpleTransform1()
    {
	    const int aSize = 16;
	    int a[aSize] = {4,0,5,5,0,5,5,1,3,1,0,3,1,1,3,5};
	    int b[aSize] = {1,9,0,8,6,1,7,7,1,0,1,3,5,7,9,8};
	    int out[aSize];
        std::transform(a,a+aSize, out, std::negate<int>());
	    bolt::amp::transform(a, a+aSize, out, bolt::negate<int>());
	    printA2("Transform Neg - From Pointer", a, out, aSize);

	    bolt::amp::transform(a, a+aSize, b, out, bolt::plus<int>());
	    printA("\nTransformVaddOut", out, aSize);

	    static const int vSz=10;
	    std::vector<int> vec(10);
	    std::generate(vec.begin(), vec.end(), rand);
	    std::vector<int> outVec(10);
	    std::transform(vec.begin(),vec.end(), outVec.begin(), std::negate<int>());
	    bolt::amp::transform(vec.begin(),vec.end(), outVec.begin(), bolt::negate<int>());
	    printA2("Transform Neg - from Vector", vec, outVec, vSz);

	

    #if 0
	    // Same as above but using lamda rather than standard "plus" functor:
	    // Doesn't compile in Dev11 Preview due to compiler bug, should be fixed in newer rev.
	    // FIXME- try with new C++AMP compiler.
	    bolt::transform(a, a+aSize, b, out, [&](int x, int y)
	    {
		    return x+y;
	    });
	    printA("\nTransformVaddOut-Lambda", out, aSize);
    #endif
    };


    /* Demostrates:
    * Bolt works for template arguments, ie int, float
    */
    template<typename T>
    void simpleTransform2(const int sz) 
    {
	    std::vector<T> A(sz);
	    std::vector<T> S(sz);
	    std::vector<T> B(sz);

	    for (int i=0; i < sz; i++) {
		    //A[i] = T(i);     // sequential assignment
		    A[i] = T(rand())/137;  // something a little more exciting.
	    };

	    std::transform (A.begin(), A.end(), S.begin(), std::negate<T>());  // single-core CPU version
        //bolt::amp::transform(A.begin(), A.end(), B.begin(), bolt::negate<T>()); // bolt version on GPU or mcore CPU.
	
	    // Check result:
	    const int maxErrCount = 10;
	    int errCount = 0;
	    for (unsigned x=0; x< S.size(); x++) {
		    const T s = S[x];
		    const T b = B[x];
		    //std::cout << s << "," << b << std::endl;
		    if ((s != b) && (++errCount < maxErrCount)) {
			    std::cout << "ERROR#" << errCount << " " << s << "!=" << b << std::endl;
		    };
	    };
    };


    //// Show use of Saxpy Functor object.
    //struct SaxpyFunctor
    //{
	   // float _a;
	   // SaxpyFunctor(float a) : _a(a) {};

	   // float operator() (const float &xx, const float &yy) restrict(cpu,amp)
	   // {
		  //  return _a * xx + yy;
	   // };
	
    //};


    //void transformSaxpy(int aSize)
    //{
	   // std::string fName = __FUNCTION__ ;
	   // fName += ":";

	   // std::vector<float> A(aSize), B(aSize), Z1(aSize), Z0(aSize);

	   // for (int i=0; i<aSize; i++) {
		  //  A[i] = float(i);
		  //  B[i] = 10000.0f + (float)i;
	   // }

	   // SaxpyFunctor sb(10.0);

	   // std::transform(A.begin(), A.end(), B.begin(), Z0.begin(), sb);
	   // bolt::amp::transform(A.begin(), A.end(), B.begin(), Z1.begin(), sb);  

	   // //checkResults(fName, Z0.begin(), Z0.end(), Z1.begin());

    //};

    void simpleTransform()
    {
	    simpleTransform1();
	    simpleTransform2<int>(128);
	    simpleTransform2<float>(1000);
	    simpleTransform2<float>(100000);

	   // transformSaxpy(256);
    };


    int _tmain(int argc, _TCHAR* argv[])
    {
	    simpleTransform();
	    return 0;
    }

#else

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Below are helper routines to compare the results of two arrays for googletest
//  They return an assertion object that googletest knows how to track
//  This is a compare routine for naked pointers.
template< typename T >
::testing::AssertionResult cmpArrays( const T ref, const T calc, size_t N )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

template< typename T, size_t N >
::testing::AssertionResult cmpArrays( const T (&ref)[N], const T (&calc)[N] )
{
    for( size_t i = 0; i < N; ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

//  Primary class template for std::array types
//  The struct wrapper is necessary to partially specialize the member function
template< typename T, size_t N >
struct cmpStdArray
{
    static ::testing::AssertionResult cmpArrays( const std::array< T, N >& ref, const std::array< T, N >& calc )
    {
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename S, typename B >
::testing::AssertionResult cmpArrays( const S& ref, const B& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process type parameterized tests

//  This class creates a C++ 'TYPE' out of a size_t value
template< size_t N >
class TypeValue
{
public:
    static const size_t value = N;
};

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class TransformArrayTest: public ::testing::Test
{
public:
    TransformArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        stdOutput = stdInput;
        boltInput = stdInput;
        boltOutput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~TransformArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput, stdOutput, boltOutput;
    int m_Errors;
};

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class UnaryTransformArrayTest: public ::testing::Test
{
public:
    UnaryTransformArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        boltOutput = stdInput;
        stdOutput = stdInput;
    };

    virtual void TearDown( )
    {};

    virtual ~UnaryTransformArrayTest( )
    {}

protected:
    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;
    typename std::array< ArrayType, ArraySize > stdInput, boltInput, stdOutput, boltOutput;
    int m_Errors;
};

TYPED_TEST_CASE_P( TransformArrayTest );

TYPED_TEST_P( TransformArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    
}

TYPED_TEST_P( TransformArrayTest, GPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( TransformArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_P( TransformArrayTest, MultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::multiplies<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    
}

TYPED_TEST_P( TransformArrayTest, GPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::multiplies<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( TransformArrayTest, CPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::multiplies<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::multiplies<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif
TYPED_TEST_P( TransformArrayTest, MinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::minus<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );

}

TYPED_TEST_P( TransformArrayTest, GPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::minus<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( TransformArrayTest, CPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::amp::minus<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::amp::minus<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_CASE_P( UnaryTransformArrayTest );

TYPED_TEST_P( UnaryTransformArrayTest, Normal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( UnaryTransformArrayTest, GPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ),  boltOutput.begin( ), bolt::amp::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}

#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryTransformArrayTest, CPU_DeviceNormal )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::negate<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

TYPED_TEST_P( UnaryTransformArrayTest, MultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
}

TYPED_TEST_P( UnaryTransformArrayTest, GPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryTransformArrayTest, CPU_DeviceMultipliesFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif
TYPED_TEST_P( UnaryTransformArrayTest, MinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );

}

TYPED_TEST_P( UnaryTransformArrayTest, GPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  The first time our routines get called, we compile the library kernels with a certain context
    //  OpenCL does not allow the context to change without a recompile of the kernel
    // MyOclContext oclgpu = initOcl(CL_DEVICE_TYPE_GPU, 0);
    //bolt::amp::control c_gpu(oclgpu._queue);  // construct control structure from the queue.

    //  Create a new command queue for a different device, but use the same context as was provided
    //  by the default control device
    ::cl::Context myContext = bolt::amp::control::getDefault( ).context( );
    std::vector< cl::Device > devices = myContext.getInfo< CL_CONTEXT_DEVICES >();
    ::cl::CommandQueue myQueue( myContext, devices[ 0 ] );
    bolt::amp::control c_gpu( myQueue );  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( c_gpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#if (TEST_CPU_DEVICE == 1)
TYPED_TEST_P( UnaryTransformArrayTest, CPU_DeviceMinusFunction )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;
    MyOclContext oclcpu = initOcl(CL_DEVICE_TYPE_CPU, 0);
	bolt::amp::control c_cpu(oclcpu._queue);  // construct control structure from the queue.

    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::square<ArrayType>());
    bolt::amp::transform( c_cpu, boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::square<ArrayType>());

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end() );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdOutput, boltOutput );
    // FIXME - releaseOcl(ocl);
}
#endif

#if (TEST_CPU_DEVICE == 1)
REGISTER_TYPED_TEST_CASE_P( TransformArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction, CPU_DeviceNormal, CPU_DeviceMultipliesFunction, CPU_DeviceMinusFunction);
REGISTER_TYPED_TEST_CASE_P( UnaryTransformArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction, CPU_DeviceNormal, CPU_DeviceMultipliesFunction, CPU_DeviceMinusFunction);
#else
REGISTER_TYPED_TEST_CASE_P( TransformArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction );
REGISTER_TYPED_TEST_CASE_P( UnaryTransformArrayTest, Normal, GPU_DeviceNormal, 
                                           MultipliesFunction, GPU_DeviceMultipliesFunction,
                                           MinusFunction, GPU_DeviceMinusFunction );
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size

class TransformIntegerVector: public ::testing::TestWithParam< int >
{
public:

    TransformIntegerVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                               stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);

        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< int > stdInput, boltInput, stdOutput, boltOutput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformFloatVector: public ::testing::TestWithParam< int >
{
public:
    TransformFloatVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                             stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< float > stdInput, boltInput, stdOutput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformDoubleVector: public ::testing::TestWithParam< int >
{
public:
    TransformDoubleVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                              stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        boltInput = stdInput;
        stdOutput = stdInput;
        boltOutput = stdInput;
    }

protected:
    std::vector< double > stdInput, boltInput, stdOutput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformIntegerDeviceVector( ): stdInput( GetParam( ) ), boltInput( GetParam( ) ),
                                     stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
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
class TransformFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformFloatDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) ),
                                   stdOutput( GetParam( ) ), boltOutput( GetParam( ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
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
    std::vector< float > stdInput, stdOutput;
    bolt::amp::device_vector< float > boltInput, boltOutput;
};

#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformDoubleDeviceVector( ): stdInput( GetParam( ) ), boltInput( static_cast<size_t>( GetParam( ) ) )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
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
    std::vector< double > stdInput, stdOutput;
    bolt::amp::device_vector< double > boltInput, boltOutput;
};
#endif

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class TransformIntegerNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformIntegerNakedPointer( ): stdInput( new int[ GetParam( ) ] ), boltInput( new int[ GetParam( ) ] ), 
                                     stdOutput( new int[ GetParam( ) ] ), boltOutput( new int[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, rand);
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
class TransformFloatNakedPointer: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    TransformFloatNakedPointer( ): stdInput( new float[ GetParam( ) ] ), boltInput( new float[ GetParam( ) ] ), 
                                   stdOutput( new float[ GetParam( ) ] ), boltOutput( new float[ GetParam( ) ] )
    {}

    virtual void SetUp( )
    {
        size_t size = GetParam( );

        std::generate(stdInput, stdInput + size, rand);
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

        std::generate(stdInput, stdInput + size, rand);

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

TEST_P( TransformIntegerVector, Normal )
{
    
    //typedef std::iterator_traits<std::vector<int>::iterator>::value_type T;
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<int>());

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::negate<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::negate<int>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}

TEST_P( TransformFloatVector, Normal )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<float>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<float>());

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );

    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::negate<float>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::negate<float>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

}
#if (TEST_DOUBLE == 1)
TEST_P( TransformDoubleVector, Inplace )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<double>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<double>());

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<double>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::negate<double>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

}
#endif
#if (TEST_DEVICE_VECTOR == 1)
TEST_P( TransformIntegerDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<int>());

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY TRANSFORM Test
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::negate<int>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::negate<int>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
TEST_P( TransformFloatDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<float>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<float>());

    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< float >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY TRANSFORM Test
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<float>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::negate<float>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
#if (TEST_DOUBLE == 1)
TEST_P( TransformDoubleDeviceVector, Inplace )
{
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), stdOutput.begin( ), bolt::plus<double>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), boltOutput.begin( ), bolt::plus<double>());

    std::vector< double >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    std::vector< double >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );

    //UNARY TRANSFORM Test
    //  Calling the actual functions under test
    std::transform( stdInput.begin( ), stdInput.end( ), stdOutput.begin( ), bolt::amp::negate<double>());
    bolt::amp::transform( boltInput.begin( ), boltInput.end( ), boltOutput.begin( ), bolt::amp::negate<double>());

    stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput );
}
#endif
#endif

TEST_P( TransformIntegerNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< int* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< int* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< int* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< int* > wrapBoltOutput( boltOutput, endIndex );

    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::plus<int>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::plus<int>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
    
    //UNARY TRANSFORM Test
    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::negate<int>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::negate<int>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}

TEST_P( TransformFloatNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< float* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< float* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< float* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< float* > wrapBoltOutput( boltOutput, endIndex );

    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::plus<float>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::plus<float>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );

    //UNARY TRANSFORM Test
    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::amp::negate<float>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::amp::negate<float>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}

#if (TEST_DOUBLE == 1)
TEST_P( TransformDoubleNakedPointer, Inplace )
{
    size_t endIndex = GetParam( );

    //  Calling the actual functions under test
    stdext::checked_array_iterator< double* > wrapStdInput( stdInput, endIndex );
    stdext::checked_array_iterator< double* > wrapStdOutput( stdOutput, endIndex );
    stdext::checked_array_iterator< double* > wrapBoltInput( boltInput, endIndex );
    stdext::checked_array_iterator< double* > wrapBoltOutput( boltOutput, endIndex );

    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, wrapStdOutput, bolt::plus<double>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, wrapBoltOutput, bolt::plus<double>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );

    //UNARY TRANSFORM Test
    std::transform( wrapStdInput, wrapStdInput + endIndex, wrapStdOutput, bolt::amp::negate<double>());
    bolt::amp::transform( wrapBoltInput, wrapBoltInput+endIndex, wrapBoltOutput, bolt::amp::negate<double>());

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdOutput, boltOutput, endIndex );
}
#endif
std::array<int, 15> TestValues = {2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
//Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( TransformRange, TransformIntegerVector, ::testing::Range( 0, 1024, 7 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformIntegerVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( TransformRange, TransformFloatVector, ::testing::Range( 0, 1024, 3 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformFloatVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( TransformRange, TransformDoubleVector, ::testing::Range( 0, 1024, 21 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformDoubleVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( TransformRange, TransformIntegerDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformIntegerDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( TransformRange, TransformFloatDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformFloatDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( TransformRange, TransformDoubleDeviceVector, ::testing::Range( 0, 1024, 53 ) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformDoubleDeviceVector, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#endif
INSTANTIATE_TEST_CASE_P( TransformRange, TransformIntegerNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformIntegerNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
INSTANTIATE_TEST_CASE_P( TransformRange, TransformFloatNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( TransformValues, TransformFloatNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( TransformRange, TransformDoubleNakedPointer, ::testing::Range( 0, 1024, 13) );
INSTANTIATE_TEST_CASE_P( Transform, TransformDoubleNakedPointer, ::testing::ValuesIn( TestValues.begin(), TestValues.end() ) );
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
BOLT_FUNCTOR(UDD,
struct UDD { 
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) { 
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
    } 
    bool operator < (const UDD& other) const { 
        return ((a+b) < (other.a+other.b));
    }
    bool operator > (const UDD& other) const { 
        return ((a+b) > (other.a+other.b));
    }
    bool operator == (const UDD& other) const { 
        return ((a+b) == (other.a+other.b));
    }
    UDD() 
        : a(0),b(0) { } 
    UDD(int _in) 
        : a(_in), b(_in +1)  { } 
}; 
);
BOLT_CREATE_TYPENAME(bolt::less<UDD>);
BOLT_CREATE_TYPENAME(bolt::greater<UDD>);
BOLT_CREATE_TYPENAME(bolt::plus<UDD>);

typedef ::testing::Types< 
    std::tuple< UDD, TypeValue< 1 > >,
    std::tuple< UDD, TypeValue< 31 > >,
    std::tuple< UDD, TypeValue< 32 > >,
    std::tuple< UDD, TypeValue< 63 > >,
    std::tuple< UDD, TypeValue< 64 > >,
    std::tuple< UDD, TypeValue< 127 > >,
    std::tuple< UDD, TypeValue< 128 > >,
    std::tuple< UDD, TypeValue< 129 > >,
    std::tuple< UDD, TypeValue< 1000 > >,
    std::tuple< UDD, TypeValue< 1053 > >,
    std::tuple< UDD, TypeValue< 4096 > >,
    std::tuple< UDD, TypeValue< 4097 > >,
    std::tuple< UDD, TypeValue< 65535 > >,
    std::tuple< UDD, TypeValue< 65536 > >
> UDDTests;

INSTANTIATE_TYPED_TEST_CASE_P( Integer, TransformArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, TransformArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, TransformArrayTest, DoubleTests );
#endif 
//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortArrayTest, UDDTests );

INSTANTIATE_TYPED_TEST_CASE_P( Integer, UnaryTransformArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, UnaryTransformArrayTest, FloatTests );
#if (TEST_DOUBLE == 1)
INSTANTIATE_TYPED_TEST_CASE_P( Double, UnaryTransformArrayTest, DoubleTests );
#endif 
//INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, SortArrayTest, UDDTests );

int main(int argc, char* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    //	Define MEMORYREPORT on windows platfroms to enable debug memory heap checking
#if defined( MEMORYREPORT ) && defined( _WIN32 )
    TCHAR logPath[ MAX_PATH ];
    ::GetCurrentDirectory( MAX_PATH, logPath );
    ::_tcscat_s( logPath, _T( "\\MemoryReport.txt") );

    //	We leak the handle to this file, on purpose, so that the ::_CrtSetReportFile() can output it's memory 
    //	statistics on app shutdown
    HANDLE hLogFile;
    hLogFile = ::CreateFile( logPath, GENERIC_WRITE, 
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    ::_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG );
    ::_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG );
    ::_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );

    ::_CrtSetReportFile( _CRT_ASSERT, hLogFile );
    ::_CrtSetReportFile( _CRT_ERROR, hLogFile );
    ::_CrtSetReportFile( _CRT_WARN, hLogFile );

    int tmp = ::_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmp |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF;
    ::_CrtSetDbgFlag( tmp );

    //	By looking at the memory leak report that is generated by this debug heap, there is a number with 
    //	{} brackets that indicates the incremental allocation number of that block.  If you wish to set
    //	a breakpoint on that allocation number, put it in the _CrtSetBreakAlloc() call below, and the heap
    //	will issue a bp on the request, allowing you to look at the call stack
    //	::_CrtSetBreakAlloc( 1833 );

#endif /* MEMORYREPORT */

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    ////  Set the standard OpenCL wait behavior to help debugging
    //bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    //myControl.waitMode( bolt::amp::control::NiceWait );

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
#endif

