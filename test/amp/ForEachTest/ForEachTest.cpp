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

#include "common/stdafx.h"
#include "bolt/amp/for_each.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include <gtest/gtest.h>
#include <array>
#include "bolt/amp/functional.h"
#include "common/test_common.h"

#include <bolt/amp/iterator/constant_iterator.h>
#include <bolt/amp/iterator/counting_iterator.h>
#define TEST_DOUBLE 0
#define GTEST_TESTS 1

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
    void simpleForEach1()
    {
        const int aSize = 16;
        int a[aSize] = {4,0,5,5,0,5,5,1,3,1,0,3,1,1,3,5};
        int b[aSize] = {1,9,0,8,6,1,7,7,1,0,1,3,5,7,9,8};
        int out[aSize];
        std::for_each(a,a+aSize, std::negate<int>());
        bolt::amp::for_each(a, a+aSize, bolt::negate<int>());
        printA2("ForEach Neg - From Pointer", a, out, aSize);

        static const int vSz=16;
        std::vector<int> vec(16);
        std::generate(vec.begin(), vec.end(), rand);
        std::vector<int> outVec(16);
        //std::transform(vec.begin(),vec.end(), outVec.begin(), std::negate<int>());
        bolt::amp::for_each(vec.begin(),vec.end(), outVec.begin(), bolt::negate<int>());
        std::cout<<"Printing";
        for(unsigned int i=0; i < 16; i++)
            std::cout<<outVec[i]<<std::endl;
    };


    /* Demostrates:
    * Bolt works for template arguments, ie int, float
    */
    template<typename T>
    void simpleForEach2(const int sz) 
    {
        std::vector<T> A(sz);
        std::vector<T> S(sz);
        std::vector<T> B(sz);

        for (int i=0; i < sz; i++) {
            //A[i] = T(i);     // sequential assignment
            A[i] = T(rand())/137;  // something a little more exciting.
        };

        std::for_each (A.begin(), A.end(), S.begin(), std::negate<T>());  // single-core CPU version
        bolt::amp::for_each(A.begin(), A.end(), B.begin(), bolt::negate<T>()); // bolt version on GPU or mcore CPU.   
    
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


   

    void simpleForEach()
    {
        simpleForEach1();
        simpleForEach2<int>(128);
        simpleForEach2<float>(1000);
        simpleForEach2<float>(100000);
    };


    int _tmain(int argc, _TCHAR* argv[])
    {
        simpleForEach();
        return 0;
    }


#else

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process type parameterized tests

//  This class creates a C++ 'TYPE' out of a size_t value
template< size_t N >
class TypeValue
{
public:
    static const size_t value = N;
};

//  Explicit initialization of the C++ static const
template< size_t N >
const size_t TypeValue< N >::value;

//  Test fixture class, used for the Type-parameterized tests
//  Namely, the tests that use std::array and TYPED_TEST_P macros
template< typename ArrayTuple >
class ForEachArrayTest: public ::testing::Test
{
public:
    ForEachArrayTest( ): m_Errors( 0 )
    {}

    virtual void SetUp( )
    {
        for( int i=0; i < ArraySize; i++ )
        {
            stdInput[ i ] = 1;
            boltInput[ i ] = 1;
        }
    };

    virtual void TearDown( )
    {};

    virtual ~ForEachArrayTest( )
    {}

//protected:

    typedef typename std::tuple_element< 0, ArrayTuple >::type ArrayType;
    static const size_t ArraySize = typename std::tuple_element< 1, ArrayTuple >::type::value;

    typename std::array< ArrayType, ArraySize > stdInput, boltInput;
    int m_Errors;
};



//  Explicit initialization of the C++ static const
template< typename ArrayTuple >
const size_t ForEachArrayTest< ArrayTuple >::ArraySize;


TYPED_TEST_CASE_P( ForEachArrayTest );


TYPED_TEST_P( ForEachArrayTest, NegateForEach )
{
    typedef std::array< ArrayType, ArraySize > ArrayCont;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), std::negate<ArrayType>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), bolt::amp::negate<ArrayType>() );

    ArrayCont::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end( ) );
    ArrayCont::difference_type boltNumElements = std::distance( boltInput.begin( ), boltInput.end( ) );

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpStdArray< ArrayType, ArraySize >::cmpArrays( stdInput, boltInput );
}


struct UDD
{
    int a; 
    int b;

    bool operator() (const UDD& lhs, const UDD& rhs) const restrict(amp,cpu){
        return ((lhs.a+lhs.b) > (rhs.a+rhs.b));
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

	UDD operator -() const restrict(amp,cpu)
    {
        UDD r;
        r.a = -a;
        r.b = -b;
        return r;
    }

	UDD operator = (const UDD &rhs) const restrict(amp,cpu)
    {
      UDD _result;
      _result.a = rhs.a;
      _result.b = rhs.b;
      return _result;
    }

    UDD() restrict(amp,cpu)
        : a(0),b(0) { }
    UDD(int _in) restrict(amp,cpu)
        : a(_in), b(_in +1)  { }
};



REGISTER_TYPED_TEST_CASE_P( ForEachArrayTest, NegateForEach );

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fixture classes are now defined to enable googletest to process value parameterized tests

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ForEachIntegerVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachIntegerVector( ): stdInput( GetParam( ), 1 ), boltInput( GetParam( ), 1 )
    {}

protected:
    std::vector< int > stdInput, boltInput;
};

class ForEachIntegerDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachIntegerDeviceVector( ): stdInput( GetParam( ), 1 ), boltInput(static_cast<size_t>(GetParam( )), 1 )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
             boltInput[i] = stdInput[i];
          
        }
    }

protected:
    std::vector< int > stdInput;
    bolt::amp::device_vector< int > boltInput;
};

//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ForEachFloatVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachFloatVector( ): stdInput( GetParam( ), 1.0f ), boltInput( GetParam( ), 1.0f )
    {}

protected:
    std::vector< float > stdInput, boltInput;
};


class ForEachFloatDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachFloatDeviceVector( ): stdInput( GetParam( ), 1.0f ), boltInput( static_cast<size_t>( GetParam( ) ), 1.0f )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
        }

    }

protected:
    std::vector< float > stdInput;
    bolt::amp::device_vector< float > boltInput;
};
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ForEachDoubleVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachDoubleVector( ): stdInput( GetParam( ), 1.0 ), boltInput( GetParam( ), 1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};


#if (TEST_DOUBLE == 1)
//  ::testing::TestWithParam< int > means that GetParam( ) returns int values, which i use for array size
class ForEachDoubleDeviceVector: public ::testing::TestWithParam< int >
{
public:
    // Create an std and a bolt vector of requested size, and initialize all the elements to 1
    ForEachDoubleDeviceVector( ): stdInput( GetParam( ), 1.0 ), boltInput( static_cast<size_t>( GetParam( ) ), 1.0 )
    {
        std::generate(stdInput.begin(), stdInput.end(), rand);
        //boltInput = stdInput;      
        //FIXME - The above should work but the below loop is used. 
        for (int i=0; i< GetParam( ); i++)
        {
            boltInput[i] = stdInput[i];
        }
    }

protected:
    std::vector< double > stdInput;
    bolt::amp::device_vector< double > boltInput;
};
#endif



template< typename T >
struct negative {
void operator()( T &x ) const restrict(amp, cpu)
{
   x = -x; // actual elements get negated here
}
};

#if 0
template <typename T>
struct mark_present_for_each
{
	    mutable T a[10];

        void operator()(T x) const restrict(amp, cpu)
		{ 
			a[(int) x] = x+3; 
		}
};


TEST( FirstTest_with_Array, For_each  )
{
    int length = 10;

   
	mark_present_for_each<int> std_f;
	mark_present_for_each<int> bolt_f;

	for(int i=0;i<length; i++)
	{
		std_f.a[i] = i;
		bolt_f.a[i] = i;
	}

	for(int i=0; i<length; i++)
	{
		std::cout<< std_f.a[i] <<" ";
	}
	std::cout<<"\n";

    mark_present_for_each<int> std_f_out = std::for_each(  std_f.a,  std_f.a+length, std_f );
    //bolt::amp::for_each( bolt_f.a,  bolt_f.a + length, bolt_f);

	for(int i=0; i<length; i++)
	{
		std::cout<< std_f_out.a[i] <<" ";

	}
	/*std::cout<<"\n";
	for(int i=0; i<length; i++)
	{
		std::cout<< bolt_f.a[i] <<" ";

	}*/

    //cmpArrays( stdInput, boltInput);
    
}
#endif


template< typename T >
struct plus10 {

void operator()( T &x ) const restrict(amp, cpu)
{
   x=x+10; 
}
};

TEST( FirstTest, For_each  )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	for (int i = 0; i < length; ++i)
        stdInput[i] = i;
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());


    std::for_each(  stdInput.begin( ),  stdInput.end( ), negative<int>());
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), negative<int>());

	/*for (int i = 0; i < length; ++i)
        std::cout<<stdInput[i]<<"  "<<boltInput[i]<<" ";*/

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVFirstTest, For_each  )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	for (int i = 0; i < length; ++i)
        stdInput[i] = i;
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

	bolt::amp::device_vector<int> boltDvInput(boltInput.begin( ), boltInput.end( ));

    std::for_each(  stdInput.begin( ),  stdInput.end( ), plus10<int>());
    bolt::amp::for_each( boltDvInput.begin( ), boltDvInput.end( ), plus10<int>());

	/*for (int i = 0; i < length; ++i)
        std::cout<<stdInput[i]<<"  "<<boltDvInput[i]<<" ";
*/
    cmpArrays( stdInput, boltDvInput);
    
}

TEST( FirstTest_n, For_each_n  )
{
    int length = 1024;

	int n = rand()%length;

    std::vector<int> stdInput( length );
	for (int i = 0; i < length; ++i)
        stdInput[i] = rand()%10;
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    std::for_each(  stdInput.begin( ),  stdInput.begin( ) + n,  plus10<int>() );
    bolt::amp::for_each_n( boltInput.begin( ), n,  plus10<int>());

    cmpArrays( stdInput, boltInput);
    
}


TEST( DVFirstTest_n, For_each_n  )
{
    int length = 1024;

	int n = rand()%length;

    std::vector<int> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = rand()%10;
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

	bolt::amp::device_vector<int> boltDvInput(boltInput.begin( ), boltInput.end( ));

    std::for_each(  stdInput.begin( ),  stdInput.begin( ) + n,  plus10<int>() );
    bolt::amp::for_each_n( boltDvInput.begin( ), n,  plus10<int>());

    cmpArrays( stdInput, boltDvInput);
    
}


TEST( HostIntVector, For_each  )
{
    int length = 1024;

    std::vector<int> stdInput( length ,1);
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    std::for_each(  stdInput.begin( ),  stdInput.end( ),  plus10<int>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), plus10<int>() );

    cmpArrays( stdInput, boltInput);
    
}

TEST( HostIntVector, OffsetForEach_n )
{
    int length = 1024;
	int n = rand()%length;

    std::vector<int> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = rand()%10;
    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::for_each(  stdInput.begin( ) + offset,   stdInput.begin( ) + offset + n, negative<int>());
    bolt::amp::for_each_n( boltInput.begin( ) + offset, n,  negative<int>() );

    cmpArrays( stdInput, boltInput);

}

TEST( HostIntVector, OffsetForEach )
{
    int length = 1024;

    std::vector<int> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = i;

    std::vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::for_each(  stdInput.begin( ) + offset,  stdInput.end( ), negative<int>());
    bolt::amp::for_each( boltInput.begin( ) + offset, boltInput.end( ),  negative<int>() );

    cmpArrays( stdInput, boltInput);

}

TEST( DVIntVector, For_each_n  )
{
    int length = 1024;
	int n = rand()%length;

    std::vector<int> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = rand()%(i+1);
    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    std::for_each(  stdInput.begin( ),  stdInput.begin( ) + n, negative<int>() );
    bolt::amp::for_each_n( boltInput.begin( ), n, negative<int>());

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVIntVector, For_each  )
{
    int length = 1024;

    std::vector<int> stdInput( length);
	for(int i=0; i<length; i++)
		stdInput[i] = i;
	std::vector<int> temp(stdInput.begin(),stdInput.end());

    bolt::amp::device_vector<int> boltInput(temp.begin(),temp.end());

    std::for_each(  stdInput.begin( ),  stdInput.end( ), negative<int>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), negative<int>());

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVIntVector, OffsetForEach_n )
{
    int length = 1024;
	int n = 100;


    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = i;

    std::vector<int> temp(stdInput.begin(),stdInput.end());

    bolt::amp::device_vector<int> boltInput(temp.begin(),temp.end());

    int offset = 10;

    std::for_each(  stdInput.begin( ) + offset,  stdInput.begin( ) + n, negative<int>() );
    bolt::amp::for_each_n( boltInput.begin( ) + offset, n-offset, negative<int>() );

	//std::vector<int> temp_out( boltInput.begin( ), boltInput.end());

    cmpArrays( stdInput, boltInput);
    
}

TEST( DVIntVector, OffsetForEach )
{
    int length = 1024;

    std::vector<int> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = rand()%(i+1);

    bolt::amp::device_vector<int> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::for_each(  stdInput.begin( ) + offset,  stdInput.end( ), negative<int>() );
    bolt::amp::for_each( boltInput.begin( ) + offset, boltInput.end( ), negative<int>() );

    cmpArrays( stdInput, boltInput);
    
}

TEST_P( ForEachIntegerVector, for_each_n )
{
	int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin(), stdInput.begin() + n, plus10<int>());
    bolt::amp::for_each_n( boltInput.begin( ), n,  plus10<int>() );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachIntegerVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin(), stdInput.end(), plus10<int>());
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  plus10<int>() );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin( ),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachIntegerDeviceVector, for_each_n )
{
	int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.begin( ) + n, negative<int>() );
    bolt::amp::for_each_n( boltInput.begin( ), n,  negative<int>() );
       
    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachIntegerDeviceVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), negative<int>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  negative<int>() );
       
    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< int >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachFloatVector, for_each_n )
{
	int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.begin( ) + n, negative<float>() );
    bolt::amp::for_each_n( boltInput.begin( ), n, negative<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachFloatVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), negative<float>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), negative<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachFloatDeviceVector, for_each_n )
{
    int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.begin( ) + n, plus10<float>() );
    bolt::amp::for_each_n( boltInput.begin( ), n, plus10<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}


TEST_P( ForEachFloatDeviceVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), negative<float>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  negative<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}



template< typename T >
struct square {

void operator()( T &x ) const restrict(amp, cpu)
{
   x=x*x; 
}
};


TEST_P( ForEachFloatVector, Square_for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), square<float>());
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachFloatDeviceVector, Square_for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), square<float>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), square<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}


template< typename T >
struct cube {

void operator()( T &x ) const restrict(amp, cpu)
{
   x=x*x*x; 
}
};


TEST_P( ForEachFloatVector, Cube_for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), cube<float>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachFloatDeviceVector, Cube_for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), cube<float>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ),  cube<float>() );


    std::vector< float >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end());
    std::vector< float >::iterator::difference_type boltNumElements = std::distance(boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

#if(TEST_DOUBLE == 1)

TEST_P( ForEachDoubleVector, for_each_n )
{
	int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.begin( ) + n, negative<double>() );
    bolt::amp::for_each_n( boltInput.begin( ), n, negative<double>() );

    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachDoubleVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), plus10<double>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), plus10<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachDoubleDeviceVector, for_each_n )
{
    int length = (int) std::distance(stdInput.begin(), stdInput.end());
	int n = rand()%length;

    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.begin( ) + n, negative<double>() );
    bolt::amp::for_each_n( boltInput.begin( ), n, negative<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

TEST_P( ForEachDoubleDeviceVector, for_each )
{
    //  Calling the actual functions under test
    std::for_each( stdInput.begin( ), stdInput.end( ), plus10<double>() );
    bolt::amp::for_each( boltInput.begin( ), boltInput.end( ), plus10<double>() );


    std::vector< int >::iterator::difference_type stdNumElements = std::distance( stdInput.begin( ), stdInput.end() );
    std::vector< int >::iterator::difference_type boltNumElements = std::distance( boltInput.begin(),boltInput.end());

    //  Both collections should have the same number of elements
    EXPECT_EQ( stdNumElements, boltNumElements );

    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}
#endif


TEST( ForEach_nDeviceVector, UDDfor_each)
{
  int length = 1<<8;
  int n = rand()%length;

  std::vector<UDD> hVectorA( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );

  bolt::amp::device_vector<UDD> dVectorA(hVectorA.begin(), hVectorA.end());

  std::for_each( hVectorA.begin(), hVectorA.begin() + n,  negative< UDD >( ) );
  bolt::amp::for_each_n( dVectorA.begin(),
    n,
    negative< UDD >( ) );
  
  cmpArrays(hVectorA, dVectorA);

}

TEST( ForEachDeviceVector, UDDfor_each)
{
  int length = 1<<8;
  std::vector<UDD> hVectorA( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );

  bolt::amp::device_vector<UDD> dVectorA(hVectorA.begin(), hVectorA.end());

  std::for_each( hVectorA.begin(), hVectorA.end(),  negative< UDD >( ) );
  bolt::amp::for_each( dVectorA.begin(),
    dVectorA.end(),
    negative< UDD >( ) );
  
  cmpArrays(hVectorA, dVectorA);

}

TEST( ForEachStdVector, UDDfor_each)
{
  int length = 1<<8;
  std::vector<UDD> hVectorA( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );

  std::vector<UDD> boltVectorA(hVectorA.begin(), hVectorA.end());

  std::for_each( hVectorA.begin(), hVectorA.end(),  negative< UDD >( ) );
  bolt::amp::for_each( boltVectorA.begin(),
    boltVectorA.end(),
    negative< UDD >( ) );
  
  cmpArrays(hVectorA, boltVectorA);

}

TEST( ForEach_nStdVector, UDDfor_each_n)
{
  int length = 1<<8;
  int n = rand()%length;

  std::vector<UDD> hVectorA( length );
  std::fill( hVectorA.begin(), hVectorA.end(), 1024 );

  std::vector<UDD> boltVectorA(hVectorA.begin(), hVectorA.end());

  std::for_each( hVectorA.begin(), hVectorA.begin() + n,  plus10<UDD>() );
  bolt::amp::for_each_n( boltVectorA.begin(),
    n,
    plus10< UDD >( ) );
  
  cmpArrays(hVectorA, boltVectorA);

}


#if TEST_DOUBLE == 1
TEST( DVIntVector, OffsetDoubleTest )
{
    int length = 1024;

    std::vector<double> stdInput( length);
	for (int i = 0; i < length; ++i)
        stdInput[i] = (double) (rand()%(i+1));
    bolt::amp::device_vector<double> boltInput(stdInput.begin(),stdInput.end());

    int offset = 100;

    std::for_each(  stdInput.begin( ) + offset,  stdInput.end( ), negative<double>() );
    bolt::amp::for_each( boltInput.begin( ) + offset, boltInput.end( ), negative<double>() );

    cmpArrays( stdInput, boltInput);
    
}
#endif


//Tests to test the countingiterator. 
//Fancy Iterators are not mutable.
TEST(simple_test,counting_iter)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);

     for(int i=0 ; i< 1024;i++)
     {
         input1[i] = i;
     }
    std::for_each( input1.begin(), input1.end(), std::negate<int>());
    bolt::amp::for_each(iter,iter2, bolt::amp::negate<int>());

	std::vector<int> output(iter,iter2);

    cmpArrays( input1, output);
}

TEST(simple_test_foreach_n,counting_iter)
{
    bolt::amp::counting_iterator<int> iter(0);
    bolt::amp::counting_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);

	int n = rand()%1024;

    for(int i=0 ; i< 1024;i++)
    {
        input1[i] = i;
    }
    std::for_each( input1.begin(), input1.begin() + n, std::negate<int>());
    bolt::amp::for_each_n(iter, n, bolt::amp::negate<int>());

	std::vector<int> output(iter,iter2);

    cmpArrays( input1, output);
}


TEST(simple_test,constant_iter)
{
    bolt::amp::constant_iterator<int> iter(10);
    bolt::amp::constant_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);

    for(int i=0 ; i< 1024;i++)
    {
        input1[i] = 10;
    }
    std::for_each( input1.begin(), input1.end(), std::negate<int>());
    bolt::amp::for_each(iter,iter2, bolt::amp::negate<int>());

	std::vector<int> output(iter,iter2);

    cmpArrays( input1, output);
}


TEST(simple_test_foreach_n,constant_iter)
{
    bolt::amp::constant_iterator<int> iter(10);
    bolt::amp::constant_iterator<int> iter2=iter+1024;
    std::vector<int> input1(1024);

    for(int i=0 ; i< 1024;i++)
    {
        input1[i] = 10;
    }
	int n = rand()%1024;

    std::for_each( input1.begin(), input1.begin() + n, std::negate<int>());
    bolt::amp::for_each_n(iter, n, bolt::amp::negate<int>());

	std::vector<int> output(iter,iter2);

    cmpArrays( input1, output);
}



//  Test lots of consecutive numbers, but small range, suitable for integers because they overflow easier
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachIntegerVector, ::testing::Range( 1, 4096, 54 ) ); //   1 to 2^12
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachIntegerDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

//  Test a huge range, suitable for floating point as they are less prone to overflow (but floating point
// loses granularity at large values)
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachFloatDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

INSTANTIATE_TEST_CASE_P( SquareForEach, ForEachFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( SquareForEach, ForEachFloatDeviceVector, ::testing::Range( 1, 1048576, 4096 ) );

INSTANTIATE_TEST_CASE_P( CubeForEach, ForEachFloatVector, ::testing::Range( 4096, 65536, 555 ) ); //2^12 to 2^16
INSTANTIATE_TEST_CASE_P( CubeForEach, ForEachFloatDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

#if(TEST_DOUBLE == 1)
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachDoubleVector, ::testing::Range(  65536, 2097152, 55555 ) ); //2^16 to 2^21
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachDoubleDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachDoubleVector, ::testing::Range(  65536, 2097152, 55555 ) ); //2^16 to 2^21
INSTANTIATE_TEST_CASE_P( ForEachTest, ForEachDoubleDeviceVector, ::testing::Range( 1, 32768, 3276 ) ); // 1 to 2^15

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


typedef ::testing::Types< 
    std::tuple< UDD, TypeValue< 1 > >,
    std::tuple< UDD, TypeValue< 31 > >,
    std::tuple< UDD, TypeValue< 32 > >,
    std::tuple< UDD, TypeValue< 63 > >,
    std::tuple< UDD, TypeValue< 64 > >,
    std::tuple< UDD, TypeValue< 127 > >,
    std::tuple< UDD, TypeValue< 128 > >,
    std::tuple< UDD, TypeValue< 129 > >,
    std::tuple< UDD, TypeValue< 1000 > >
> UDDTests;
//    std::tuple< UDD, TypeValue< 1053 > >,
//    std::tuple< UDD, TypeValue< 4096 > >,
//    std::tuple< UDD, TypeValue< 4097 > >,
//    std::tuple< UDD, TypeValue< 65535 > >,
//    std::tuple< UDD, TypeValue< 65536 > >


INSTANTIATE_TYPED_TEST_CASE_P( Integer, ForEachArrayTest, IntegerTests );
INSTANTIATE_TYPED_TEST_CASE_P( Float, ForEachArrayTest, FloatTests );
INSTANTIATE_TYPED_TEST_CASE_P( UDDTest, ForEachArrayTest, UDDTests );



int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    //    Set the standard OpenCL wait behavior to help debugging
    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose AMP


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
#endif