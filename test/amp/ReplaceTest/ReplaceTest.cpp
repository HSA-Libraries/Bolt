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

#include "common/stdafx.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"
#include "bolt/amp/functional.h"
#include <bolt/amp/replace.h>
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
        return ((a+b) == (other.a+other.b));
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
        : a(_in), b(_in +1)  { }
};

template <typename T>
struct is_odd
{

    bool operator()(const T x) const restrict(amp, cpu)
    {
      if (((int)x) % 2)
		  return true;
	  else
		  return false;
    }
};

template <typename T>
struct is_greater_than_5
{
    bool operator()(const T x) const restrict(amp, cpu)
    {
      if (x>5)
		  return true;
	  else
		  return false;
    }
};

struct udd_is_greater_than_5
{
    bool operator()(const UDD x) const restrict(amp, cpu)
    {
      if (x.a>5)
		  return true;
	  else
		  return false;
    }
};

struct is_odd_a
{
    bool operator()(const UDD x) const restrict(amp, cpu)
    {
      if (x.a % 2)
		  return true;
	  else
		  return false;
    }
};

template <typename T>
struct is_less_than_zero
{

    bool operator()(const T x) const restrict(amp, cpu)
    {
      return x < 0;
    }
};


struct is_less_than_zero_udd
{

    bool operator()(const UDD x) const restrict(amp, cpu)
    {
      return (x.a < 0 && x.b < 0);
    }
};






template <typename InputIterator, typename Predicate, typename Stencil, typename T, typename OutputIterator>
void Serial_replace_copy_if_stencil(InputIterator first, InputIterator last,  Stencil s, OutputIterator result, Predicate p, const T &new_val )
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	for(int i=0; i<sz; i++)
	{
		if(p(s[i]))
			result[i] = new_val;
		else
			result[i] = first[i];
	}

}


template <typename InputIterator, typename Predicate, typename Stencil, typename T>
void Serial_replace_if_stencil(InputIterator first, InputIterator last, Stencil s, Predicate p, const T &new_val )
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	for(int i=0; i<sz; i++)
	{
		if(p(s[i]))
			first[i] = new_val;
	}

}

TEST( ReplaceIfStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	const int val = rand()%10;

    std::replace_if(  stdInput.begin( ),  stdInput.end( ), is_odd<int>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), is_odd<int>(), val);
	
    cmpArrays( stdInput, boltInput);
    
}



TEST( ReplaceIfDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	bolt::amp::device_vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	const int val = rand()%10;

    std::replace_if(  stdInput.begin( ),  stdInput.end( ), is_odd<int>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), is_odd<int>(), val);
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( FloatReplaceIfStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	const float val = stdInput[index];

    std::replace_if(  stdInput.begin( ),  stdInput.end( ),  is_odd<float>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), is_odd<float>(), val );
	
    cmpArrays( stdInput, boltInput);
    
}

TEST( FloatReplaceIfDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	const float val = stdInput[index];

    std::replace_if(  stdInput.begin( ),  stdInput.end( ),  is_odd<float>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), is_odd<float>(), val );
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( UDDReplaceIfStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD val;
	val.a = 1, val.b = 2;

    std::replace_if(  stdInput.begin( ),  stdInput.end( ), is_odd_a() , val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ),is_odd_a(), val );
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( UDDReplaceIfDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	bolt::amp::device_vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD val;
	val.a = 1, val.b = 2;

    std::replace_if(  stdInput.begin( ),  stdInput.end( ), is_odd_a() , val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ),is_odd_a(), val );
	
    cmpArrays( stdInput, boltInput);
    
}



TEST( SimpleArrayTest, ReplaceIf )
{
    int input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3  }; 
 
	int val = 100;

	std::replace_if(  input1, input1+11, is_odd<int>(), val );
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11, is_odd<int>(), val );

    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatSimpleArrayTest, ReplaceIf )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
 
	float val = 1.005f;

	std::replace_if(  input1, input1+11, is_odd<float>(), val );
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11, is_odd<float>(), val);

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDSimpleArrayTest, ReplaceIf )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD val;
	val.a = 1, val.b = 2;
 
	std::replace_if(  input1, input1+11, is_odd_a(), val );
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11,is_odd_a(), val);

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}


TEST( StencilReplaceIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltInput( stdInput.begin( ), stdInput.end( ) );
	int val = 23;

	Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), std::identity<int>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), stencil.begin(),bolt::amp::identity<int>(), val);
	
    cmpArrays( stdInput, boltInput);
    
}

TEST( StencilReplaceIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	bolt::amp::device_vector<int> boltInput( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> stencil_dv( stencil.begin(), stencil.end() );
	int val = 23;

	Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), is_greater_than_5<int>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), is_greater_than_5<int>(), val);
	
    cmpArrays( stdInput, boltInput);
     
}


TEST( FloatStencilReplaceIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	std::vector<float> boltInput( stdInput.begin( ), stdInput.end( ) );
	float val = 23.7f;

	Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), std::identity<float>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), stencil.begin(),bolt::amp::identity<float>(), val);
	
    cmpArrays( stdInput, boltInput);
    
}

TEST( FloatStencilReplaceIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> stencil_dv( stencil.begin(), stencil.end() );
	float val = 23.5f;

	Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), is_greater_than_5<float>(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), is_greater_than_5<float>(), val);
	
    cmpArrays( stdInput, boltInput);
     
}


TEST( UDDStencilReplaceIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );
	UDD val;
	val.a = 1, val.b = 2;

	Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), udd_is_greater_than_5(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), udd_is_greater_than_5(), val);
	
    cmpArrays( stdInput, boltInput);
    
}

TEST( UDDStencilReplaceIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	bolt::amp::device_vector<UDD> boltInput(stdInput.begin( ), stdInput.end( ));
	bolt::amp::device_vector<UDD> boltStencil(stencil.begin( ), stencil.end( ));
	
	UDD val;
	val.a = 1, val.b = 2;

    Serial_replace_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), udd_is_greater_than_5(), val);
    bolt::amp::replace_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), udd_is_greater_than_5(), val);

    cmpArrays( stdInput, boltInput);
    
}


TEST( StencilArrayTest, ReplaceIf )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
	int bolt_input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
    int input2[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
    int val = 100; 
 
	Serial_replace_if_stencil(  input1, input1+11, input2,  is_less_than_zero<int>(), val );
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11, input2, is_less_than_zero<int>(), val);

    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatStencilArrayTest, ReplaceIf )
{
    float input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float bolt_input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
    float val = 6.5f; 
 
	Serial_replace_if_stencil(  input1, input1+11, input2, is_less_than_zero<float>(), val );
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11, input2, is_less_than_zero<float>(), val );

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDStencilArrayTest, ReplaceIf )
{
    UDD input1[11], bolt_input1[11], input2[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		input2[i].a = rand()%2;
		input2[i].b = input2[i].a;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

    UDD val;
	val.a = 1, val.b = 2;

	Serial_replace_if_stencil(  input1, input1+11, input2, is_less_than_zero_udd(), val);
    bolt::amp::replace_if(  bolt_input1, bolt_input1+11, input2,  is_less_than_zero_udd(), val);

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}




//Replace Tests...


TEST( ReplaceStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	const int val = rand()%10;
	int index = rand()%length;
	int old_val = stdInput[index];

    std::replace(  stdInput.begin( ),  stdInput.end( ),  old_val, val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val);
	
    cmpArrays( stdInput, boltInput);
    
}



TEST( ReplaceDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	bolt::amp::device_vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	const int val = rand()%10;
	int index = rand()%length;
	int old_val = stdInput[index];

    std::replace(  stdInput.begin( ),  stdInput.end( ),  old_val, val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val);
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( FloatReplaceStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	const float old_val = stdInput[index];
	const float val = 2.5f;

    std::replace(  stdInput.begin( ),  stdInput.end( ),  old_val, val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val );
	
    cmpArrays( stdInput, boltInput);
    
}

TEST( FloatReplaceDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	const float old_val = stdInput[index];
	const float val = 2.5f;

    std::replace(  stdInput.begin( ),  stdInput.end( ),  old_val, val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val );
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( UDDReplaceStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD val;
	val.a = 1, val.b = 2;
	int index = rand()%length;
	UDD old_val = stdInput[index];

    std::replace(  stdInput.begin( ),  stdInput.end( ), old_val , val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val );
	
    cmpArrays( stdInput, boltInput);
    
}


TEST( UDDReplaceDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	bolt::amp::device_vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD val;
	val.a = 1, val.b = 2;
	int index = rand()%length;
	UDD old_val = stdInput[index];

    std::replace(  stdInput.begin( ),  stdInput.end( ), old_val , val);
    bolt::amp::replace( boltInput.begin( ), boltInput.end( ), old_val, val );

    cmpArrays( stdInput, boltInput);
    
}



TEST( SimpleArrayTest, Replace )
{
    int input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3  }; 
 
	int val = 100;
	int old_val =  3;

	std::replace(  input1, input1+11, old_val, val );
    bolt::amp::replace(  bolt_input1, bolt_input1+11, old_val, val );

    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatSimpleArrayTest, Replace )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
 
	float val = 1.005f;
	float old_val = 3.0f;

	std::replace(  input1, input1+11, old_val, val );
    bolt::amp::replace(  bolt_input1, bolt_input1+11, old_val, val );

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDSimpleArrayTest, Replace )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD val;
	val.a = 1, val.b = 2;

	UDD old_val = input1[4];
 
	std::replace(  input1, input1+11, old_val, val );
    bolt::amp::replace(  bolt_input1, bolt_input1+11, old_val, val );

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}



//ReplaceCopyIf tests....

TEST( ReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltOutput(length), stdOutput(length);


	const int val = rand()%10;

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd<int>(), val);
    bolt::amp::replace_copy_if( stdInput.begin( ),  stdInput.end( ), boltOutput.begin(), is_odd<int>(), val);
	
    cmpArrays( stdOutput, boltOutput);
    
}



TEST( ReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	bolt::amp::device_vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );
	bolt::amp::device_vector<int> boltOutput(length);

	std::vector<int> stdOutput(length);

	const int val = rand()%10;

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd<int>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ),  boltInput.end( ), boltOutput.begin(), is_odd<int>(), val);
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( FloatReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltOutput(length), stdOutput(length);

	int index = rand()%length;
	const float val = stdInput[index];

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(),  is_odd<float>(), val);
    bolt::amp::replace_copy_if( stdInput.begin( ),  stdInput.end( ), boltOutput.begin(), is_odd<float>(), val );
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( FloatReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );
	bolt::amp::device_vector<float> boltOutput(length); 
	std::vector<float> stdOutput(length);

	int index = rand()%length;
	const float val = stdInput[index];

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ),  stdOutput.begin(),  is_odd<float>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd<float>(), val );
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( UDDReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<UDD> boltOutput(length), stdOutput(length);

	UDD val;
	val.a = 1, val.b = 2;

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd_a() , val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd_a(), val );
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( UDDReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	bolt::amp::device_vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<UDD>  stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput(length);

	UDD val;
	val.a = 1, val.b = 2;

    std::replace_copy_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd_a() , val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd_a(), val );
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( SimpleArrayTest, ReplaceCopyIf )
{
    int input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3  }; 
 
	int stdOutput[11], boltOutput[11];

	int val = 100;

	std::replace_copy_if(  input1, input1+11, stdOutput, is_odd<int>(), val );
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd<int>(), val );

    // compare results
    cmpArrays<int,11>( stdOutput, boltOutput );

}

TEST( FloatSimpleArrayTest, ReplaceCopyIf )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
 
	float stdOutput[11], boltOutput[11];

	float val = 1.005f;

	std::replace_copy_if(  input1, input1+11, stdOutput, is_odd<float>(), val );
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd<float>(), val);

    // compare results
    cmpArrays<float,11>( stdOutput, boltOutput );

}

TEST( UDDSimpleArrayTest, ReplaceCopyIf )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD val;
	val.a = 1, val.b = 2;
 
	UDD stdOutput[11], boltOutput[11];

	std::replace_copy_if(  input1, input1+11, stdOutput, is_odd_a(), val );
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd_a(), val);

    // compare results
    cmpArrays<UDD,11>( stdOutput, boltOutput );

}




TEST( StencilReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<int> boltOutput(length), stdOutput(length);
	int val = 23;

	Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), is_odd<int>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), is_odd<int>(), val);
	
    cmpArrays( stdOutput, boltOutput);
    
}



TEST( StencilReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	bolt::amp::device_vector<int> boltInput( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> stencil_dv( stencil.begin(), stencil.end() );

	bolt::amp::device_vector<int> boltOutput(length);
	std::vector<int> stdOutput(length);


	int val = 23;

	Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(),  is_greater_than_5<int>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), boltOutput.begin(), is_greater_than_5<int>(), val);
	
    cmpArrays( stdOutput, boltOutput);
     
}


TEST( FloatStencilReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	std::vector<float> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<float> boltOutput(length), stdOutput(length);

	float val = 23.7f;

	Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), std::identity<float>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), bolt::amp::identity<float>(), val);
	
     cmpArrays( stdOutput, boltOutput);
    
}

TEST( FloatStencilReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> stencil_dv( stencil.begin(), stencil.end() );

	bolt::amp::device_vector<float> boltOutput(length);
	std::vector<float> stdOutput(length);

	float val = 23.5f;

	Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), is_greater_than_5<float>(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), boltOutput.begin(), is_greater_than_5<float>(), val);
	
    cmpArrays( stdOutput, boltOutput);
     
}



TEST( UDDStencilReplaceCopyIfStdVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<UDD> boltOutput( length ), stdOutput(length);

	UDD val;
	val.a = 1, val.b = 2;

	Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), udd_is_greater_than_5(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), udd_is_greater_than_5(), val);
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( UDDStencilReplaceCopyIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<UDD> stdInput( length );
	std::vector<UDD> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;

		stencil[i].a = rand()%2;
		stencil[i].b = stencil[i].a;
	}

	bolt::amp::device_vector<UDD> boltInput(stdInput.begin( ), stdInput.end( ));
	bolt::amp::device_vector<UDD> boltStencil(stencil.begin( ), stencil.end( ));
	
	std::vector<UDD> stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput( length );

	UDD val;
	val.a = 1, val.b = 2;

    Serial_replace_copy_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), udd_is_greater_than_5(), val);
    bolt::amp::replace_copy_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), boltOutput.begin(), udd_is_greater_than_5(), val);

    cmpArrays( stdOutput, boltOutput);
    
}


TEST( StencilArrayTest, ReplaceCopyIf )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
	int bolt_input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
    int input2[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
    int val = 100; 
  
	int stdOut[11], boltOut[11];

	Serial_replace_copy_if_stencil(  input1, input1+11, input2, stdOut, is_less_than_zero<int>(), val );
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero<int>(), val);

    // compare results
    cmpArrays<int,11>( stdOut, boltOut );

}

TEST( FloatStencilArrayTest, ReplaceCopyIf )
{
    float input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float bolt_input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
    float val = 6.5f; 
 
	float stdOut[11], boltOut[11];

	Serial_replace_copy_if_stencil(  input1, input1+11, input2, stdOut, is_less_than_zero<float>(), val );
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero<float>(), val );

    // compare results
    cmpArrays<float,11>( stdOut, boltOut );

}

TEST( UDDStencilArrayTest, ReplaceCopyIf )
{
    UDD input1[11], bolt_input1[11], input2[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		input2[i].a = rand()%2;
		input2[i].b = input2[i].a;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

    UDD val;
	val.a = 1, val.b = 2;

	UDD stdOut[11], boltOut[11];

	Serial_replace_copy_if_stencil(  input1, input1+11, input2, stdOut, is_less_than_zero_udd(), val);
    bolt::amp::replace_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero_udd(), val);

    // compare results
    cmpArrays<UDD,11>( stdOut, boltOut );

}




//Replace Tests...


TEST( ReplaceCopyStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );
	std::vector<int> stdOutput(length), boltOutput(length);

	const int val = rand()%10;
	int index = rand()%length;
	int old_val = stdInput[index];

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);
    
}



TEST( ReplaceCopyDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	bolt::amp::device_vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	const int val = rand()%10;
	int index = rand()%length;
	int old_val = stdInput[index];

	std::vector<int> stdOutput(length);
	bolt::amp::device_vector<int> boltOutput(length);

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);
    
}


TEST( FloatReplaceCopyStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	std::vector<float> stdOutput(length), boltOutput(length);

	int index = rand()%length;
	const float old_val = stdInput[index];
	const float val = 2.5f;

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);
    
}

TEST( FloatReplaceCopyDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	bolt::amp::device_vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );
	std::vector<float> stdOutput(length);
	bolt::amp::device_vector<float> boltOutput(length);

	int index = rand()%length;
	const float old_val = stdInput[index];
	const float val = 2.5f;

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);   
    
}


TEST( UDDReplaceCopyStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );
	std::vector<UDD> stdOutput(length), boltOutput(length);

	UDD val;
	val.a = 1, val.b = 2;
	int index = rand()%length;
	UDD old_val = stdInput[index];

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);   
    
}


TEST( UDDReplaceCopyDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	bolt::amp::device_vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD val;
	val.a = 1, val.b = 2;
	int index = rand()%length;
	UDD old_val = stdInput[index];

	std::vector<UDD> stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput(length);

    std::replace_copy(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), old_val, val);
    bolt::amp::replace_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), old_val, val);
	
    cmpArrays( stdOutput, boltOutput);   
    
}


TEST( SimpleArrayTest, ReplaceCopy )
{
    int input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3  }; 
 
	int stdOut[11], boltOut[11];

	int val = 100;
	int old_val =  3;

	std::replace_copy(  input1, input1+11, stdOut, old_val, val );
    bolt::amp::replace_copy(  bolt_input1, bolt_input1+11, boltOut, old_val, val );

    // compare results
    cmpArrays<int,11>( stdOut, boltOut );

}

TEST( FloatSimpleArrayTest, ReplaceCopy )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
 
	float stdOut[11], boltOut[11];

	float val = 1.005f;
	float old_val = 3.0f;

	std::replace_copy(  input1, input1+11, stdOut, old_val, val );
    bolt::amp::replace_copy(  bolt_input1, bolt_input1+11, boltOut, old_val, val );

    // compare results
    cmpArrays<float,11>( stdOut, boltOut );

}

TEST( UDDSimpleArrayTest, ReplaceCopy )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD stdOut[11], boltOut[11];

	UDD val;
	val.a = 1, val.b = 2;

	UDD old_val = input1[4];
 
	std::replace_copy(  input1, input1+11, stdOut, old_val, val );
    bolt::amp::replace_copy(  bolt_input1, bolt_input1+11, boltOut, old_val, val );

    // compare results
    cmpArrays<UDD,11>( stdOut, boltOut );

}



static TestBuffer<1024> test_buffer;

template <typename T>
struct is_even
{
    bool operator()(T x) const restrict (amp, cpu)
    {
        int temp = x;
        return (temp % 2) == 0;
    }
};

template <typename UDD>
struct is_even_a
{
    bool operator()(UDD x) const restrict(amp, cpu)
    {
      if (x.a % 2)
		  return true;
	  else
		  return false;
    }
};

template <typename T>
class Replace_if_raw_test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type src_value_type;
    typedef typename std::tuple_element<1, T>::type stencil_value_type;
    typedef typename std::tuple_element<2, T>::type dst_value_type;
    typedef typename std::tuple_element<3, T>::type Predicate;
    Predicate predicate;

    void test_raw_ptrs()
    {
        int length = 100;
        src_value_type* pInput       = new src_value_type[length];
        test_buffer.init(pInput, length);
        stencil_value_type* pStencil = new stencil_value_type[length];
        test_buffer.init(pStencil, length);
		dst_value_type* pOutput       = new dst_value_type[length];
        test_buffer.init(pOutput, length);

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<stencil_value_type> ref_stencilVec(length);
        test_buffer.init(ref_stencilVec);
		std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);
        
		int n = rand()%length;
		src_value_type val = ref_inputVec[n];

		//Test without Stencil
        std::replace_copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate, val);
        bolt::amp::replace_copy_if(pInput, pInput + length, pOutput, predicate, val);        
        cmpArrays(ref_outputVec, pOutput);

        //Test with Stencil
        Serial_replace_copy_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(), ref_outputVec.begin(), predicate, val);
        bolt::amp::replace_copy_if(pInput, pInput + length, pStencil, pOutput, predicate, val);        
        cmpArrays(ref_outputVec, pOutput);


        //Test without Stencil
        std::replace_if(ref_inputVec.begin(), ref_inputVec.end(), predicate, val);
        bolt::amp::replace_if(pInput, pInput + length, predicate, val);        
        cmpArrays(ref_inputVec, pInput);

        //Test with Stencil
        Serial_replace_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(), predicate, val);
        bolt::amp::replace_if(pInput, pInput + length, pStencil, predicate, val);        
        cmpArrays(ref_inputVec, pInput);


        delete pInput;
        delete pStencil;
		delete pOutput;
    }

};


template <typename T>
class Replace_if_Test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type SrcContainer;
    typedef typename std::tuple_element<1, T>::type StencilContainer;
    typedef typename std::tuple_element<2, T>::type DstContainer;
    typedef typename std::tuple_element<3, T>::type Predicate;
    Predicate predicate;
    
    typedef typename SrcContainer::iterator     SrcIterator;
    typedef typename StencilContainer::iterator StencilIterator;
    typedef typename DstContainer::iterator     DstIterator;

    typedef typename std::iterator_traits<SrcIterator>::value_type      src_value_type;
    typedef typename std::iterator_traits<StencilIterator>::value_type  stencil_value_type;
    typedef typename std::iterator_traits<DstIterator>::value_type      dst_value_type;

    void test_wo_stencil()
    {
        int length = 100;
        std::cout << "Testing Replace_if_Test\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
		std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);
		DstContainer outputVec(length); 
        test_buffer.init(outputVec);

		for(int i=0; i<length; i++)
		{
			ref_inputVec[i]= i;
			inputVec[i] = i;
		}


		int n = rand()%length;
		src_value_type val = ref_inputVec[n];


		std::replace_copy_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate, val);
        //Call Bolt Now   
        bolt::amp::replace_copy_if(inputVec.begin(), inputVec.end(), outputVec.begin(), predicate, val);
        cmpArrays(ref_outputVec, outputVec);


        std::replace_if(ref_inputVec.begin(), ref_inputVec.end(), predicate, val);
        //Call Bolt Now   
        bolt::amp::replace_if(inputVec.begin(), inputVec.end(), predicate, val);
        //for(int i=0; i< length; i++)
            //std::cout << "i = " << i <<"-- ref =" << ref_inputVec[i] << " out = " << inputVec[i] << "\n";
        cmpArrays(ref_inputVec, inputVec);
	
    }

    void test_w_stencil( )
    {
        int length = 100;
        std::cout << "Testing Replace_if_Test\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<stencil_value_type> ref_stencilVec(length);
        test_buffer.init(ref_stencilVec);
		std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);
        StencilContainer stencilVec(length); 
        test_buffer.init(stencilVec);
		DstContainer outputVec(length); 
        test_buffer.init(outputVec);

		for(int i=0; i<length; i++)
		{
			ref_inputVec[i]= i;
			ref_stencilVec[i] = i%2;
			inputVec[i] = i;
			stencilVec[i] = i%2;
		}


		int n = rand()%length;
		src_value_type val = ref_inputVec[n];

		Serial_replace_copy_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(), ref_outputVec.begin(), predicate, val);
        //Call Bolt Now   
        bolt::amp::replace_copy_if(inputVec.begin(), inputVec.end(), stencilVec.begin(), outputVec.begin(), predicate, val);
        cmpArrays(ref_outputVec, outputVec);

        Serial_replace_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_stencilVec.begin(),  predicate, val);
        //Call Bolt Now
        bolt::amp::replace_if(inputVec.begin(), inputVec.end(), stencilVec.begin(), predicate, val);
        cmpArrays(ref_inputVec, inputVec);
    }

};


TYPED_TEST_CASE_P(Replace_if_Test);
TYPED_TEST_CASE_P(Replace_if_raw_test);

TYPED_TEST_P(Replace_if_Test, without_stencil) 
{

    test_wo_stencil();

}

TYPED_TEST_P(Replace_if_Test, with_stencil) 
{

    test_w_stencil();

}

TYPED_TEST_P(Replace_if_raw_test, raw_pointers) 
{

    test_raw_ptrs();

}

//TYPED_TEST_P(Replace_if_raw_test, constant_iteratorr) 
//{
//
//    test_constant_itr();
//
//}

REGISTER_TYPED_TEST_CASE_P(Replace_if_Test, without_stencil, with_stencil);

typedef std::tuple<std::vector<int>,          std::vector<int>,        std::vector<int>,        is_even<int> >       INT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<float>,        std::vector<float>,      std::vector<float>,      is_even<float> >     FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<int>,          std::vector<int>,        std::vector<float>,      is_even<int> >       INT_VEC_2_FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<UDD>,          std::vector<UDD>,        std::vector<UDD>,        is_even_a<UDD> >       UDD_VEC_IS_EVEN_T;
typedef ::testing::Types<
        INT_VEC_IS_EVEN_T,
        FLOAT_VEC_IS_EVEN_T,
        INT_VEC_2_FLOAT_VEC_IS_EVEN_T ,
		UDD_VEC_IS_EVEN_T> STDVectorIsEvenTypes;

typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::device_vector<int>,     is_even<int> >      INT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<float>,     bolt::amp::device_vector<float>,    bolt::amp::device_vector<float>,   is_even<float> >    FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::device_vector<float>,   is_even<int> >      INT_DV_2_FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<UDD>,       bolt::amp::device_vector<UDD>,      bolt::amp::device_vector<UDD>,     is_even_a<UDD>>      UDD_DV_IS_EVEN_T;
typedef ::testing::Types<
        INT_DV_IS_EVEN_T,
        FLOAT_DV_IS_EVEN_T,
        INT_DV_2_FLOAT_DV_IS_EVEN_T ,
        UDD_DV_IS_EVEN_T> DeviceVectorIsEvenTypes;

REGISTER_TYPED_TEST_CASE_P(Replace_if_raw_test, raw_pointers);
typedef std::tuple<int,          int,        int,     is_even<int> >                 INT_2_INT_IS_EVEN_T;
typedef std::tuple<float,        float,      float,   is_even<float> >               FLOAT_2_FLOAT_IS_EVEN_T;
typedef std::tuple<int,          int,        float,   is_even<int> >                 INT_2_FLOAT_IS_EVEN_T;
typedef std::tuple<UDD,          UDD,        UDD,     is_even_a<UDD> >                 UDD_IS_EVEN_T;
typedef ::testing::Types<
        INT_2_INT_IS_EVEN_T,
        FLOAT_2_FLOAT_IS_EVEN_T,
        INT_2_FLOAT_IS_EVEN_T,
        UDD_IS_EVEN_T> POD_RawPtrIsEvenTypes;



INSTANTIATE_TYPED_TEST_CASE_P(STDVectorTests, Replace_if_Test, STDVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(DeviceVectorTests, Replace_if_Test, DeviceVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(RawPtrTests, Replace_if_raw_test, POD_RawPtrIsEvenTypes);


//Add the test team test cases here. 

//Add the EPR test cases here with the EPR number. 

template <typename T>
struct is_even_t
{    
bool operator()(T x) const restrict(amp, cpu)    
	{     
		 if (((int)x) % 2 == 0)      
		   return true;   
		 else     
		   return false;
	}
};


TEST(sanity_replace_if_array, int_bolt_arr_even_DocSample) {
	const int N = 10;
	int in1[N] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
	int in2[N] = {-5,  0,  2,  3,  2,  4, -2,  1,  2,  3};
	int stencil[10] = {0,  0,  1,  1,  0,  1, 1,  1,  0,  1};
	

	bolt::amp::replace_if(in1, in1 + N, is_even_t<int>(),  300);
	std::replace_if(in2, in2 + N, is_even_t<int>(),  300);

	for(int i = 0; i < N;  i++){
        EXPECT_EQ(in1[i], in2[i]); 
	    std::cout << "result = " << in1[i] << "  " << in2[i] <<"\n";
    }
	
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



