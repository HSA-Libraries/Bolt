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

#define TEST_DOUBLE 0
#define TEST_DEVICE_VECTOR 1
#define TEST_CPU_DEVICE 1
#define TEST_LARGE_BUFFERS 0

#pragma warning(disable: 4244) // Disabling possible loss of data warning
#if defined (_WIN32)
#include <xutility>
#endif

#include <bolt/amp/remove.h>
#include "common/stdafx.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include "bolt/amp/functional.h"
#include <bolt/amp/replace.h>

#include <bolt/amp/unique.h>


#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
#include <array>
#include "common/test_common.h"


//Add the developer unit test cases here. 
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
		return ((a == other.a) && (b == other.b));
    }

	bool operator !=(const UDD &rhs) const restrict(amp, cpu)
	{
       return ((a != rhs.a) || (b != rhs.b));
	}

    UDD operator + (const UDD &rhs) const restrict(amp,cpu)
    {
      UDD _result;
      _result.a = a + rhs.a;
      _result.b = b + rhs.b;
      return _result;
    }

	UDD operator-() const restrict(amp, cpu)
    {
        UDD r;
        r.a = -a;
        r.b = -b;
        return r;
    }

    UDD() restrict(amp,cpu)
        : a(0),b(0) { }
    UDD(int _in) restrict(amp,cpu)
        : a(_in), b(_in )  { }
};


//Unique Tests...

TEST(stress_unique_bolt_std_pow, int_pow_bolt_std_DocSample) 
{
	const int N = 1 << 20;
    bolt::amp::device_vector<int> in1(N);
    std::vector<int> in2(N);
 
	for(int i = 0; i < N/2; i++) {
		in1[i] = in2[i] = rand() % 2;
	}
	for(int i = N/2; i < N; i++) {
		in1[i] = in2[i] = rand() % 5;
	}

	bolt::amp::unique(in1.begin(), in1.end());
	std::unique(in2.begin(), in2.end());
	
	//std::cout << "Num of elements= " << sizeof(in1) << "  " << sizeof(in2) <<"\n";

    for(int i = 0; i < N/*in1[i] == in2[i]*/;  i++){
    EXPECT_EQ(in1[i], in2[i]); 
    // std::cout << "result = " << in1[i] << "  " << in2[i] <<"\n";
  }
}


TEST( UniqueStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

    std::vector<int>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
	std::vector<int>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, boltInput, sz);
    
}



TEST( UniqueDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

    std::vector<int>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    std::vector<int>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, bolt_out, sz);
    
}


TEST( FloatUniqueStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	stdInput[0] = (float) (rand()%10);
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = (float) (rand()%10);
	}

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

    std::vector<float>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
	std::vector<float>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, boltInput, sz);
    
}

TEST( FloatUniqueDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	stdInput[0] = (float) (rand()%10);
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = (float) (rand()%10);
	}

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

    std::vector<float>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
	std::vector<float>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

	std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, sz);
	
  
    
}


TEST( UDDUniqueStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	stdInput[0].a = rand()%10;
	stdInput[0].b = rand()%12;
	for(int i=1; i<length; i++)
	{
		if(i<length/2)
		{
		   stdInput[i].a = stdInput[i-1].a;
		   stdInput[i].b = stdInput[i-1].b;
		}
		else
		{
		   stdInput[i].a = rand()%10;
		   stdInput[i].b = rand()%12;
		}
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	std::vector<UDD>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
	std::vector<UDD>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, boltInput, sz);
    
}


TEST( UDDUniqueDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	stdInput[0].a = rand()%10;
	stdInput[0].b = rand()%12;
	for(int i=1; i<length; i++)
	{
		if(i<length/2)
		{
		   stdInput[i].a = stdInput[i-1].a;
		   stdInput[i].b = stdInput[i-1].b;
		}
		else
		{
		   stdInput[i].a = rand()%10;
		   stdInput[i].b = rand()%12;
		}
	}

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput(boltInput_t.begin(), boltInput_t.end());

	std::vector<UDD>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ));
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ));
	
	std::vector<UDD>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

	std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, sz);
	
    
}


TEST( UniqueStdVectorPred, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

    std::vector<int>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ), std::equal_to<int>());
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ), bolt::amp::equal_to<int>());
	
	std::vector<int>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, boltInput, sz);
    
}



TEST( UniqueDVVectorPred, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

    std::vector<int>::iterator std_last = std::unique(  stdInput.begin( ),  stdInput.end( ), std::equal_to<int>());
    bolt::amp::unique( boltInput.begin( ), boltInput.end( ), bolt::amp::equal_to<int>());
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    std::vector<int>::iterator std_first = stdInput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdInput, bolt_out, sz);
    
}


//Unique Copy Tests...


TEST( UniqueCopyStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );
	std::vector<int> stdOutput(length);
	std::vector<int> boltOutput(length);


    std::vector<int>::iterator std_last = std::unique_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin());
    bolt::amp::unique_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin());
	
    std::vector<int>::iterator std_first = stdOutput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdOutput, boltOutput, sz);
    
}



TEST( UniqueCopyStdVectorPred, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );
	std::vector<int> stdOutput(length);
	std::vector<int> boltOutput(length);


    std::vector<int>::iterator std_last = std::unique_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::equal_to<int>());
    bolt::amp::unique_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), bolt::amp::equal_to<int>());
	
    std::vector<int>::iterator std_first = stdOutput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdOutput, boltOutput, sz);
    
}



TEST( UniqueCopyDVVectorPred, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	bolt::amp::device_vector<int> boltOutput( length );
	std::vector<int> stdOutput(length);


    std::vector<int>::iterator std_last = std::unique_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), std::equal_to<int>());
    bolt::amp::unique_copy( boltInput.begin( ), boltInput.end( ),  boltOutput.begin(), bolt::amp::equal_to<int>());
	
    std::vector<int> bolt_out(boltOutput.begin( ), boltOutput.end( ));
    std::vector<int>::iterator std_first = stdOutput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdOutput, bolt_out, sz);
    
}


TEST( UniqueCopyDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	stdInput[0] = rand()%10;
	for(int i=1; i<length; i++)
	{
		if(i<(length/2))
		  stdInput[i] = stdInput[i-1];
		else
          stdInput[i] = rand()%10;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	bolt::amp::device_vector<int> boltOutput( length );
	std::vector<int> stdOutput(length);


    std::vector<int>::iterator std_last = std::unique_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin());
    bolt::amp::unique_copy( boltInput.begin( ), boltInput.end( ),  boltOutput.begin());
	
    std::vector<int> bolt_out(boltOutput.begin( ), boltOutput.end( ));
    std::vector<int>::iterator std_first = stdOutput.begin();
	int sz = std::distance(std_first, std_last);

    cmpArrays( stdOutput, bolt_out, sz);
    
}


static TestBuffer<1024> test_buffer;


template <typename T>
class Unique_raw_test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type src_value_type;
    typedef typename std::tuple_element<1, T>::type dst_value_type;
    typedef typename std::tuple_element<2, T>::type Predicate;
    Predicate predicate;

    void test_raw_ptrs()
    {
        int length = 100;
        src_value_type* pInput       = new src_value_type[length];
        test_buffer.init(pInput, length);
        src_value_type* pOutput       = new dst_value_type[length];
        test_buffer.init(pOutput, length);
        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);
        
		src_value_type val = 0;
        std::unique(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        bolt::amp::unique(pInput, pInput + length, predicate);        
        cmpArrays(ref_inputVec, pInput);

        std::unique_copy(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate);
        bolt::amp::unique_copy(pInput, pInput + length, pOutput, predicate);        
        cmpArrays(ref_inputVec, pInput);

        delete pInput;
		delete pOutput;
    }

};


template <typename T>
class Unique_Test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type SrcContainer;
    typedef typename std::tuple_element<1, T>::type DstContainer;
    typedef typename std::tuple_element<2, T>::type Predicate;
    Predicate predicate;
    
    typedef typename SrcContainer::iterator     SrcIterator;
    typedef typename DstContainer::iterator     DstIterator;

    typedef typename std::iterator_traits<SrcIterator>::value_type      src_value_type;
    typedef typename std::iterator_traits<DstIterator>::value_type      dst_value_type;

    void test_unique()
    {
        int length = 100;
        std::cout << "Testing Unique Test\n";

        std::vector<src_value_type> ref_inputVec(length);
	  
        test_buffer.init(ref_inputVec);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);

		ref_inputVec[0]= rand()%10;
		inputVec[0] = ref_inputVec[0];

		for(int i=1; i<length; i++)
		{
			if(i<length/2)
			{
			   ref_inputVec[i] = ref_inputVec[i-1];
			   inputVec[i] = ref_inputVec[i];
			}
			else
			{
				ref_inputVec[i] = rand()%i;
			    inputVec[i] = ref_inputVec[i];
			}
		}

        std::vector<src_value_type>::iterator std_last = std::unique(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        //Call Bolt Now   
        bolt::amp::unique(inputVec.begin(), inputVec.end(), predicate);

		std::vector<src_value_type>::iterator std_first = ref_inputVec.begin();
	    int sz = std::distance(std_first, std_last);

		for(int i = sz; i<length; i++)
		{
			ref_inputVec[i] = 0;
			inputVec[i] = 0;
		}
       
        /*for(int i=0; i< length; i++)
            std::cout << "i = " << i <<"-- ref =" << ref_inputVec[i] << " out = " << inputVec[i] << "\n";*/

        cmpArrays( ref_inputVec, inputVec);
    }

    void test_unique_copy( )
    {
        int length = 100;
        std::cout << "Testing Unique Copy Test\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);
        DstContainer outputVec(length); 
        test_buffer.init(outputVec);

		
		ref_inputVec[0]= rand()%10;
		inputVec[0] = ref_inputVec[0];

		for(int i=1; i<length; i++)
		{
			if(i<length/2)
			{
			   ref_inputVec[i] = ref_inputVec[i-1];
			   inputVec[i] = ref_inputVec[i];
			}
			else
			{
				ref_inputVec[i] = rand()%i;
			    inputVec[i] = ref_inputVec[i];
			}
		}


        std::vector<src_value_type>::iterator std_last = std::unique_copy(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate);
        //Call Bolt Now   
        bolt::amp::unique_copy(inputVec.begin(), inputVec.end(), outputVec.begin(), predicate);

		std::vector<src_value_type>::iterator std_first = ref_outputVec.begin();
	    int sz = std::distance(std_first, std_last);

       
		for(int i = sz; i<length; i++)
		{
			ref_outputVec[i] = 0;
			outputVec[i] = 0;
		}

       /* for(int i=0; i< length; i++)
            std::cout << "i = " << i <<"-- ref =" << ref_outputVec[i] << " out = " << outputVec[i] << "\n";*/

	
        cmpArrays( ref_outputVec, outputVec);
    }

};


TYPED_TEST_CASE_P(Unique_Test);
TYPED_TEST_CASE_P(Unique_raw_test);

TYPED_TEST_P(Unique_Test, unique_only) 
{

    test_unique();

}

TYPED_TEST_P(Unique_Test, unique_copy) 
{

    test_unique_copy();

}

TYPED_TEST_P(Unique_raw_test, raw_pointers) 
{

    test_raw_ptrs();

}

REGISTER_TYPED_TEST_CASE_P(Unique_Test, unique_only, unique_copy);

typedef std::tuple<std::vector<int>,          std::vector<int>,          bolt::amp::equal_to<int> >       INT_VEC_EQUAL_T;
typedef std::tuple<std::vector<float>,        std::vector<float>,        bolt::amp::equal_to<float> >     FLOAT_VEC_EQUAL_T;
typedef std::tuple<std::vector<UDD>,          std::vector<UDD>,          bolt::amp::equal_to<UDD> >       UDD_VEC_EQUAL_T;
typedef ::testing::Types<
        INT_VEC_EQUAL_T,
        FLOAT_VEC_EQUAL_T,
        UDD_VEC_EQUAL_T > STDVectorEqualTypes;

typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::equal_to<int> >      INT_DV_EQUAL_T;
typedef std::tuple<bolt::amp::device_vector<float>,     bolt::amp::device_vector<float>,    bolt::amp::equal_to<float> >    FLOAT_DV_EQUAL_T;
typedef std::tuple<bolt::amp::device_vector<UDD>,       bolt::amp::device_vector<UDD>,      bolt::amp::equal_to<UDD> >      UDD_DV_EQUAL_T;
typedef ::testing::Types<
        INT_DV_EQUAL_T,
        FLOAT_DV_EQUAL_T,
        UDD_DV_EQUAL_T> DeviceVectorEqualTypes;

REGISTER_TYPED_TEST_CASE_P(Unique_raw_test, raw_pointers);
typedef std::tuple<int,          int,         bolt::amp::equal_to<int> >                 INT_EQUAL_T;
typedef std::tuple<float,        float,       bolt::amp::equal_to<float> >               FLOAT_EQUAL_T;
typedef std::tuple<int,          int,         bolt::amp::equal_to<int> >                 INT_2_FLOAT_EQUAL_T;
typedef std::tuple<UDD,          UDD,         bolt::amp::equal_to<UDD> >                 UDD_EQUAL_T;
typedef ::testing::Types<
        INT_EQUAL_T,
        FLOAT_EQUAL_T,
        INT_2_FLOAT_EQUAL_T,
        UDD_EQUAL_T> POD_RawPtrIsEvenTypes;



INSTANTIATE_TYPED_TEST_CASE_P(STDVectorTests, Unique_Test, STDVectorEqualTypes);
INSTANTIATE_TYPED_TEST_CASE_P(DeviceVectorTests, Unique_Test, DeviceVectorEqualTypes);
INSTANTIATE_TYPED_TEST_CASE_P(RawPtrTests, Unique_raw_test, POD_RawPtrIsEvenTypes);



//Add the test team test cases here. 

//Add the EPR test cases here with the EPR number. 

TEST(sanity_unique_copy_ptr_ctl, int_bolt_ptr_ctl_DocSample)
{
//const int N = 100;
//int *in1 = NULL;
//int *in2 = NULL;
//int *out1 = NULL;
//int *out2 = NULL;
//
//in1 = (int *) malloc (N * sizeof(int));
//in2 = (int *) malloc (N * sizeof(int));
//out1 = (int *) malloc (N * sizeof(int));
//out2 = (int *) malloc (N * sizeof(int));
//
//for(int i = 0; i < N; i++) {
//              if(i/2){
//                     in1[i] = in2[i] = 1;
//              }
//              else {
//                     in1[i] = in2[i] = i % 2;
//              }
//       }
//
////Create an AMP Control object using the default accelerator
//       ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
//       bolt::amp::control ctl(accel);
//
//       bolt::amp::unique_copy(in1, in1 + N, out1, bolt::amp::equal_to<int>());
//       std::unique_copy(in2, in2 + N, out2, std::equal_to<int> ());
//       
//  for(int i = 0; i < N;  i++){
//  EXPECT_EQ(out1[i], out2[i]); 
//  //std::cout << "result = " << in1[i] << "  " << in2[i] <<"\n";
//  }

  char *s = "Avinash";
  s = "ACP";
  printf("*******************%s\n", s);


}




int main(int argc, char* argv[])
{
    //  Register our minidump generating logic
#if defined(_WIN32)
    bolt::miniDumpSingleton::enableMiniDumps( );
#endif

    // Define MEMORYREPORT on windows platfroms to enable debug memory heap checking
#if defined( MEMORYREPORT ) && defined( _WIN32 )
    TCHAR logPath[ MAX_PATH ];
    ::GetCurrentDirectory( MAX_PATH, logPath );
    ::_tcscat_s( logPath, _T( "\\MemoryReport.txt") );

    // We leak the handle to this file, on purpose, so that the ::_CrtSetReportFile() can output it's memory 
    // statistics on app shutdown
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

    // By looking at the memory leak report that is generated by this debug heap, there is a number with 
    // {} brackets that indicates the incremental allocation number of that block.  If you wish to set
    // a breakpoint on that allocation number, put it in the _CrtSetBreakAlloc() call below, and the heap
    // will issue a bp on the request, allowing you to look at the call stack
    // ::_CrtSetBreakAlloc( 1833 );

#endif /* MEMORYREPORT */

    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    bolt::amp::control& myControl = bolt::amp::control::getDefault( );
    myControl.setWaitMode( bolt::amp::control::NiceWait );
    myControl.setForceRunMode( bolt::amp::control::Automatic );  // choose AMP

    int retVal = RUN_ALL_TESTS( );

	myControl.setForceRunMode( bolt::amp::control::SerialCpu );  // choose serial
    retVal = RUN_ALL_TESTS( );


    myControl.setForceRunMode( bolt::amp::control::MultiCoreCpu );  // choose tbb
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



