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

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>
#include <array>
#include "common/test_common.h"


template <typename T>
struct is_even{ 
       bool operator()(T x) const restrict(amp, cpu)    
       {   
              if (((int)x) % 2 == 0)   
                     return true; 
              else  
                     return false;
       }
};

template <typename T>
struct is_odd{ 
       bool operator()(T x) const restrict(amp, cpu)    
       {   
              if (((int)x) % 2 == 0)   
                     return true; 
              else  
                     return false;
       }
};


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



template <typename T>
struct is_greater_than_5
{
    bool operator()(T x) const restrict(amp, cpu)
    {
      if (x>5)
		  return true;
	  else
		  return false;
    }
};

struct udd_is_greater_than_5
{
    bool operator()(UDD x) const restrict(amp, cpu)
    {
      if (x.a>5)
		  return true;
	  else
		  return false;
    }
};

struct is_odd_a
{
    bool operator()(UDD x) const restrict(amp, cpu)
    {
      if (x.a % 2)
		  return false;
	  else
		  return true;
    }
};

template <typename T>
struct is_less_than_zero
{

    bool operator()(T x) const restrict(amp, cpu)
    {
      return x < 0;
    }
};


struct is_less_than_zero_udd
{

    bool operator()(UDD x) const restrict(amp, cpu)
    {
      return (x.a < 0 && x.b < 0);
    }
};



template <typename InputIterator, typename T>
int Serial_remove(InputIterator first, InputIterator last, InputIterator result, const T& val, T init)
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	int j = 0;
	for(int i=0; i<sz; i++)
	{
		if(first[i] != val)
			result[j++] = first[i];
	}
	int res = j;
	while(j<sz)
		result[j++] = init;
	return res;
}


template <typename InputIterator, typename Predicate, typename T>
int Serial_remove_if(InputIterator first, InputIterator last, InputIterator result, Predicate pred, T init)
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);

	int j = 0;
	for(int i=0; i<sz; i++)
	{
		if(!(pred(first[i])))
			result[j++] = first[i];
	}
	int res = j;
	while(j<sz)
		result[j++] = init;
	return res;
}


template <typename InputIterator, typename Predicate, typename Stencil, typename T>
int Serial_remove_if_stencil(InputIterator first, InputIterator last, InputIterator result, Stencil stencil, Predicate pred, T init)
{
	typename std::iterator_traits<InputIterator>::difference_type sz = (last - first);
	int j = 0;
	for(int i=0; i<sz; i++)
	{
		if(!(pred(stencil[i])))
			result[j++] = first[i];
	}
	int res = j;
	while(j<sz)
		result[j++] = init;
    return res;
}

TEST( RemoveIfStdVector, SimpleTest )
{
    int length =  1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int init = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdInput.begin(), is_odd<int>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), is_odd<int>());
	
    cmpArrays( stdInput, boltInput, n);
    
}



TEST( RemoveIfDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<int> boltInput_std( stdInput.begin( ),  stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_std.begin( ),  boltInput_std.end( ) );

	int init = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), is_odd<int>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), is_odd<int>());
	
	std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( FloatRemoveIfStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	float init = 0.0f;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),   stdInput.begin(), is_odd<float>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), is_odd<float>());
	
    cmpArrays( stdInput, boltInput, n);
    
}

TEST( FloatRemoveIfDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput_std( stdInput.begin( ),  stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_std.begin( ),  boltInput_std.end( ) );

	float init = 0.0f;
    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), is_odd<float>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), is_odd<float>());
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( UDDRemoveIfStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD init;
	init.a = 0, init.b = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), is_odd_a(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ),is_odd_a());
	
    cmpArrays( stdInput, boltInput, n);
    
}


TEST( UDDRemoveIfDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput_std( stdInput.begin( ),  stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput( boltInput_std.begin( ),  boltInput_std.end( ) );

	UDD init;
	init.a = 0, init.b = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), is_odd_a(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ),is_odd_a());
	
    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, n);
    
}



TEST( SimpleArrayTest, RemoveIf )
{
    int input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3  }; 
 
	int init = 0;

	int n = Serial_remove_if(  input1, input1+11, input1, is_odd<int>(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11, is_odd<int>() );

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;
    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatSimpleArrayTest, RemoveIf )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
 
	float init = 0.0f;

	int n = Serial_remove_if(  input1, input1+11, input1, is_odd<float>(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11, is_odd<float>());

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDSimpleArrayTest, RemoveIf )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD init;
	init.a = 0, init.b = 0;

	int n = Serial_remove_if(  input1, input1+11, input1, is_odd_a(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11,is_odd_a());

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}


TEST( StencilRemoveIfStdVector, SimpleTest )
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

	int init = 0;
	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), std::identity<int>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), stencil.begin(),bolt::amp::identity<int>());
	
    cmpArrays( stdInput, boltInput, n);
    
}

TEST( StencilRemoveIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<int> stencil_dv( stencil.begin(), stencil.end() );

	int init = 0;
	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), is_greater_than_5<int>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), is_greater_than_5<int>());
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));

    cmpArrays( stdInput, bolt_out, n);
     
}


TEST( FloatStencilRemoveIfStdVector, SimpleTest )
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

	float init = 0.0f;
	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), std::identity<float>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), stencil.begin(),bolt::amp::identity<float>());
	
    cmpArrays( stdInput, boltInput, n);
    
}

TEST( FloatStencilRemoveIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<float> stencil_dv( stencil.begin(), stencil.end() );

	float init = 0.0f;
	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), is_greater_than_5<float>(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), is_greater_than_5<float>());
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
     
}


TEST( UDDStencilRemoveIfStdVector, SimpleTest )
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

	UDD init;
	init.a = 0, init.b = 0;

	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), udd_is_greater_than_5(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), udd_is_greater_than_5());
	
    cmpArrays( stdInput, boltInput, n);
    
}

TEST( UDDStencilRemoveIfDVVector, SimpleTest )
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

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<UDD> boltStencil(stencil.begin( ), stencil.end( ));

	UDD init;
	init.a = 0, init.b = 0;

    int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdInput.begin( ), stencil.begin(), udd_is_greater_than_5(), init);
    bolt::amp::remove_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), udd_is_greater_than_5());

    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( StencilArrayTest, RemoveIf )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
	int bolt_input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
    int input2[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
 
	int init = 0;

	int n = Serial_remove_if_stencil(  input1, input1+11, input1, input2,  is_less_than_zero<int>(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11, input2, is_less_than_zero<int>());

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatStencilArrayTest, RemoveIf )
{
    float input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float bolt_input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
 
	float init = 0.0f;
	int n = Serial_remove_if_stencil(  input1, input1+11, input1, input2, is_less_than_zero<float>(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11, input2, is_less_than_zero<float>());

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDStencilArrayTest, RemoveIf )
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

	UDD init;
	init.a = 0, init.b = 0;

	int n = Serial_remove_if_stencil(  input1, input1+11, input1, input2, is_less_than_zero_udd(), init);
    bolt::amp::remove_if(  bolt_input1, bolt_input1+11, input2,  is_less_than_zero_udd());

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}




//Remove Tests...


TEST( RemoveStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	int val = stdInput[index];

	int init = 0;

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val);
	
    cmpArrays( stdInput, boltInput, n);
    
}



TEST( RemoveDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	int index = rand()%length;
	int val = stdInput[index];

	int init = 0;

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val);
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( FloatRemoveStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	int index = rand()%length;
	const float val = stdInput[index];
	float init = 0.0f;

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val );
	
    cmpArrays( stdInput, boltInput, n);
    
}

TEST( FloatRemoveDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	int index = rand()%length;
	const float val = stdInput[index];
	float init = 0.0f;

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ),  stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val );
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( UDDRemoveStdVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput( stdInput.begin( ), stdInput.end( ) );

	UDD init;
	init.a = 0, init.b = 0;
	int index = rand()%length;
	UDD val = stdInput[index];

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val );
	
    cmpArrays( stdInput, boltInput, n);
    
}


TEST( UDDRemoveDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput(boltInput_t.begin(), boltInput_t.end());

	UDD init;
	init.a = 0, init.b = 0;
	int index = rand()%length;
	UDD val = stdInput[index];

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdInput.begin(), val, init);
    bolt::amp::remove( boltInput.begin( ), boltInput.end( ), val );

    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}



TEST( SimpleArrayTest, Remove )
{
    int input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3  }; 
 
	int val = 3;
	int init =  0;

	int n = Serial_remove(  input1, input1+11, input1, val, init );
    bolt::amp::remove(  bolt_input1, bolt_input1+11, val );

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<int,11>( input1, bolt_input1 );

}

TEST( FloatSimpleArrayTest, Remove)
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
 
	float init = 0.0f;
	float val = 3.0f;

	int n = Serial_remove(  input1, input1+11, input1, val, init);
    bolt::amp::remove(  bolt_input1, bolt_input1+11, val );

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<float,11>( input1, bolt_input1 );

}

TEST( UDDSimpleArrayTest, Remove )
{
    UDD input1[11], bolt_input1[11]; 
	for(int i=0; i<11; i++)
	{
		input1[i].a = rand()%10;
		input1[i].b = rand()%12;

		bolt_input1[i].a = input1[i].a;
		bolt_input1[i].b = input1[i].b;
	}

	UDD init;
	init.a = 0, init.b = 0;

	UDD val = input1[4];
 
	int n = Serial_remove(  input1, input1+11, input1, val, init );
    bolt::amp::remove(  bolt_input1, bolt_input1+11, val );

	for(int i = n; i<11; i++)
		bolt_input1[i] = init;

    // compare results
    cmpArrays<UDD,11>( input1, bolt_input1 );

}



//RemoveCopyIf tests....

TEST( RemoveCopyIfStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltOutput(length), stdOutput(length);

	int init = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd<int>(), init);
    bolt::amp::remove_copy_if( stdInput.begin( ),  stdInput.end( ), boltOutput.begin(), is_odd<int>());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}



TEST( RemoveCopyIfDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<int> boltOutput(length);

	std::vector<int> stdOutput(length);

	int init = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd<int>(), init);
    bolt::amp::remove_copy_if( boltInput.begin( ),  boltInput.end( ), boltOutput.begin(), is_odd<int>());
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( FloatRemoveCopyIfStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltOutput(length), stdOutput(length);

	float val = 0.0f;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(),  is_odd<float>(), val);
    bolt::amp::remove_copy_if( stdInput.begin( ),  stdInput.end( ), boltOutput.begin(), is_odd<float>());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}

TEST( FloatRemoveCopyIfDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<float> boltOutput(length); 
	std::vector<float> stdOutput(length);

	float val = 0.0f;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ),  stdOutput.begin(),  is_odd<float>(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd<float>());
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( UDDRemoveCopyIfStdVector, SimpleTest )
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
	val.a = 0, val.b = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd_a() , val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd_a());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}


TEST( UDDRemoveCopyIfDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	std::vector<UDD>  stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput(length);

	UDD val;
	val.a = 0, val.b = 0;

    int n = Serial_remove_if(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), is_odd_a() , val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), is_odd_a() );
	
    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( SimpleArrayTest, RemoveCopyIf )
{
    int input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 30, -5, -50, -20, -5, 3  }; 
 
	int stdOutput[11], boltOutput[11];

	int val = 0;

	int n = Serial_remove_if(  input1, input1+11, stdOutput, is_odd<int>(), val );
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd<int>() );

	for(int i = n; i<11; i++)
		boltOutput[i] = val;

    // compare results
    cmpArrays<int,11>( stdOutput, boltOutput );

}

TEST( FloatSimpleArrayTest, RemoveCopyIf )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.2f }; 
 
	float stdOutput[11], boltOutput[11];

	float val = 0.0f;

	int  n = Serial_remove_if(  input1, input1+11, stdOutput, is_odd<float>(), val );
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd<float>());

	for(int i = n; i<11; i++)
		boltOutput[i] = val;

    // compare results
    cmpArrays<float,11>( stdOutput, boltOutput );

}

TEST( UDDSimpleArrayTest, RemoveCopyIf )
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
	val.a = 0, val.b = 0;
 
	UDD stdOutput[11], boltOutput[11];

	int n = Serial_remove_if(  input1, input1+11, stdOutput, is_odd_a(), val );
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, boltOutput, is_odd_a());

	for(int i = n; i<11; i++)
		boltOutput[i] = val;

    // compare results
    cmpArrays<UDD,11>( stdOutput, boltOutput );

}




TEST( StencilRemoveCopyIfStdVector, SimpleTest )
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
	int val = 0;

	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stencil.begin(), stdOutput.begin(), is_odd<int>(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), is_odd<int>());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}



TEST( StencilRemoveCopyIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<int> stdInput( length );
	std::vector<int> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = rand()%10;
		stencil[i] = rand()%2;
	}

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<int> stencil_dv( stencil.begin(), stencil.end() );

	bolt::amp::device_vector<int> boltOutput(length);
	std::vector<int> stdOutput(length);


	int val = 0;

	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ),  stdOutput.begin(), stencil.begin(), is_greater_than_5<int>(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), boltOutput.begin(), is_greater_than_5<int>());
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
     
}


TEST( FloatStencilRemoveCopyIfStdVector, SimpleTest )
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

	float val = 0.0f;

	int  n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ),  stdOutput.begin(), stencil.begin(), std::identity<float>(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), bolt::amp::identity<float>());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}

TEST( FloatStencilRemoveCopyIfDVVector, SimpleTest )
{
    int length = 1024;

    std::vector<float> stdInput( length );
	std::vector<float> stencil( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i] = (float)(rand()%10);
		stencil[i] = (float) ( rand()%2);
	}

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<float> stencil_dv( stencil.begin(), stencil.end() );

	bolt::amp::device_vector<float> boltOutput(length);
	std::vector<float> stdOutput(length);

	float val = 0.0f;

	int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ),  stdOutput.begin(), stencil.begin(), is_greater_than_5<float>(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), stencil_dv.begin(), boltOutput.begin(), is_greater_than_5<float>());
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
     
}



TEST( UDDStencilRemoveCopyIfStdVector, SimpleTest )
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
	val.a = 0, val.b = 0;

	int  n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdOutput.begin(), stencil.begin(),  udd_is_greater_than_5(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), stencil.begin(), boltOutput.begin(), udd_is_greater_than_5());
	
    cmpArrays( stdOutput, boltOutput, n);
    
}

TEST( UDDStencilRemoveCopyIfDVVector, SimpleTest )
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

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	bolt::amp::device_vector<UDD> boltStencil(stencil.begin( ), stencil.end( ));
	
	std::vector<UDD> stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput( length );

	UDD val;
	val.a = 0, val.b = 0;

    int n = Serial_remove_if_stencil( stdInput.begin( ), stdInput.end( ), stdOutput.begin(), stencil.begin(),  udd_is_greater_than_5(), val);
    bolt::amp::remove_copy_if( boltInput.begin( ), boltInput.end( ), boltStencil.begin(), boltOutput.begin(), udd_is_greater_than_5());

    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( StencilArrayTest, RemoveCopyIf )
{
    int input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 }; 
	int bolt_input1[11] = { 7, 0, 0, 3, 3, 3, -5, -5, -5, -5, 3 };
    int input2[11] = { 0, 1, 1, 0, 0, 0,  1,  0,  1,  1, 0 }; 
    int val = 0; 
  
	int stdOut[11], boltOut[11];

	int  n = Serial_remove_if_stencil(  input1, input1+11, stdOut, input2, is_less_than_zero<int>(), val );
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero<int>());

	for(int i = n; i<11; i++)
		bolt_input1[i] = val;

    // compare results
    cmpArrays<int,11>( stdOut, boltOut );

}

TEST( FloatStencilArrayTest, RemoveCopyIf )
{
    float input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float bolt_input1[11] = { -7.5f, 0, 0.6f, 3.3f, 5.6f, 3.0f, -5.6f, -2.5f, 4.0f, -3.2f, 3.2f }; 
	float input2[11] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f, 0.0f }; 
    float val = 0.0f; 
 
	float stdOut[11], boltOut[11];

	int n = Serial_remove_if_stencil(  input1, input1+11, stdOut, input2, is_less_than_zero<float>(), val );
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero<float>());

	for(int i = n; i<11; i++)
		bolt_input1[i] = val;

    // compare results
    cmpArrays<float,11>( stdOut, boltOut );

}

TEST( UDDStencilArrayTest, RemoveCopyIf )
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
	val.a = 0, val.b = 0;

	UDD stdOut[11], boltOut[11];

	int n = Serial_remove_if_stencil(  input1, input1+11, stdOut, input2, is_less_than_zero_udd(), val);
    bolt::amp::remove_copy_if(  bolt_input1, bolt_input1+11, input2, boltOut, is_less_than_zero_udd());

	for(int i = n; i<11; i++)
		bolt_input1[i] = val;

    // compare results
    cmpArrays<UDD,11>( stdOut, boltOut );

}


//Remove Copy Tests...


TEST( RemoveCopyStdVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;
	std::vector<int> boltInput( stdInput.begin( ),  stdInput.end( ) );
	std::vector<int> stdOutput(length), boltOutput(length);

	int index = rand()%length;
	int val = stdInput[index];
	int init = 0;

    int  n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(),  val);
	
    cmpArrays( stdOutput, boltOutput, n);
    
}



TEST( RemoveCopyDVVector, SimpleTest )
{
    int length = 1 << 18;

    std::vector<int> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = rand()%10;

	std::vector<int> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<int> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	const int init = 0;
	int index = rand()%length;
	int val = stdInput[index];

	std::vector<int> stdOutput(length);
	bolt::amp::device_vector<int> boltOutput(length);

    int  n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), val);
	
    std::vector<int> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( FloatRemoveCopyStdVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput( stdInput.begin( ),  stdInput.end( ) );

	std::vector<float> stdOutput(length), boltOutput(length);

	int index = rand()%length;
	const float val = stdInput[index];
	float init = 0.0f;

    int  n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), val);
	
    cmpArrays( stdOutput, boltOutput, n);
    
}

TEST( FloatRemoveCopyDVVector, SimpleTest )
{
    int length = 1 << 20;

    std::vector<float> stdInput( length );
	for(int i=0; i<length; i++)
		stdInput[i] = (float)(rand()%10);

	std::vector<float> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<float> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );
	std::vector<float> stdOutput(length);
	bolt::amp::device_vector<float> boltOutput(length);

	int index = rand()%length;
	const float val = stdInput[index];
	float init = 0.0f;

    int  n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), val);
	
    std::vector<float> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( UDDRemoveCopyStdVector, SimpleTest )
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

	UDD init;
	init.a = 0, init.b = 0;
	int index = rand()%length;
	UDD val = stdInput[index];

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(),  val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), val);
	
    cmpArrays( stdOutput, boltOutput, n);   
    
}


TEST( UDDRemoveCopyDVVector, SimpleTest )
{
    int length = 1<<20;

    std::vector<UDD> stdInput( length );
	for(int i=0; i<length; i++)
	{
		stdInput[i].a = rand()%10;
		stdInput[i].b = rand()%12;
	}

	std::vector<UDD> boltInput_t( stdInput.begin( ), stdInput.end( ) );
	bolt::amp::device_vector<UDD> boltInput( boltInput_t.begin( ), boltInput_t.end( ) );

	UDD init;
	init.a = 0, init.b = 0;
	int index = rand()%length;
	UDD val = stdInput[index];

	std::vector<UDD> stdOutput(length);
	bolt::amp::device_vector<UDD> boltOutput(length);

    int n = Serial_remove(  stdInput.begin( ),  stdInput.end( ), stdOutput.begin(), val, init);
    bolt::amp::remove_copy( boltInput.begin( ), boltInput.end( ), boltOutput.begin(), val);
	
    std::vector<UDD> bolt_out(boltInput.begin( ), boltInput.end( ));
    cmpArrays( stdInput, bolt_out, n);
    
}


TEST( SimpleArrayTest, RemoveCopy )
{
    int input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3 }; 
	int bolt_input1[11] = { 8, 10, 0, 3, 13, 3, -5, -50, -20, -5, 3  }; 
 
	int stdOut[11], boltOut[11];

	int init = 0;
	int val =  3;

	int n = Serial_remove(  input1, input1+11, stdOut, val, init );
    bolt::amp::remove_copy(  bolt_input1, bolt_input1+11, boltOut, val );

	for(int i = n; i<11; i++)
		boltOut[i] = init;

    // compare results
    cmpArrays<int,11>( stdOut, boltOut );

}

TEST( FloatSimpleArrayTest, RemoveCopy )
{
    float input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
    float bolt_input1[11] = { 7.5f, 0, 0.6f, 3.0f, 5.6f, 3.0f, -5.6f, -2.5f, -4.0f, -3.2f, 3.0f }; 
 
	float stdOut[11], boltOut[11];

	float init = 0.0f;
	float val = 3.0f;

	int n = Serial_remove(  input1, input1+11, stdOut, val, init );
    bolt::amp::remove_copy(  bolt_input1, bolt_input1+11, boltOut, val );

	for(int i = n; i<11; i++)
		boltOut[i] = init;

    // compare results
    cmpArrays<float,11>( stdOut, boltOut );

}

TEST( UDDSimpleArrayTest, RemoveCopy )
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

	UDD init;
	init.a = 0,init.b = 0;

	UDD val = input1[4];
 
	int n = Serial_remove(  input1, input1+11, stdOut, val, init );
    bolt::amp::remove_copy(  bolt_input1, bolt_input1+11, boltOut, val );

	for(int i = n; i<11; i++)
		boltOut[i] = init;

    // compare results
    cmpArrays<UDD,11>( stdOut, boltOut );

}


static TestBuffer<1024> test_buffer;



template <typename UDD>
struct is_even_a
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
class Remove_if_raw_test : public ::testing::Test {
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
        stencil_value_type* pStencil = new src_value_type[length];
        test_buffer.init(pStencil, length);
		dst_value_type* pOutput      = new dst_value_type[length];
        test_buffer.init(pOutput, length);

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        std::vector<stencil_value_type> ref_stencilVec(length);
        test_buffer.init(ref_stencilVec);
		std::vector<dst_value_type> ref_outputVec(length);
        test_buffer.init(ref_outputVec);
        
		src_value_type init = 0;
		int index = rand()%length;
		src_value_type val = ref_inputVec[index];

		 //Test without Stencil
        int n = Serial_remove_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate, init);
        bolt::amp::remove_copy_if(pInput, pInput+length, pOutput, predicate); 
		for(int i=n ;i<length; i++)
			 pOutput[i] = init;
        cmpArrays(ref_outputVec, pOutput);

        //Test with Stencil
        n = Serial_remove_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), ref_stencilVec.begin(), predicate, init);
        bolt::amp::remove_copy_if(pInput, pInput+length, pOutput, predicate);        
        for(int i=n ;i<length; i++)
			 pOutput[i] = init;
        cmpArrays(ref_outputVec, pOutput);

		n = Serial_remove(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), val, init);
        bolt::amp::remove_copy(pInput, pInput+length, pOutput, val);        
        for(int i=n ;i<length; i++)
			 pOutput[i] = init;
        cmpArrays(ref_outputVec, pOutput);;


        //Test without Stencil
        n = Serial_remove_if(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), predicate, init);
        bolt::amp::remove_if(pInput, pInput+length, predicate);     
        for(int i=n;i<length; i++)
			 pInput[i] = init;
        cmpArrays(ref_inputVec, pInput);

        //Test with Stencil
        n = Serial_remove_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), ref_stencilVec.begin(), predicate, init);
        bolt::amp::remove_if(pInput, pInput+length, pStencil, predicate);        
        for(int i=n ;i<length; i++)
			 pInput[i] = init;
        cmpArrays(ref_inputVec, pInput);

		n = Serial_remove(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), val, init);
        bolt::amp::remove(pInput, pInput+length, val);        
        for(int i=n ;i<length; i++)
			 pInput[i] = init;
        cmpArrays(ref_inputVec, pInput);

        delete pInput;
        delete pStencil;
		delete pOutput;
    }

};


template <typename T>
class Remove_if_Test : public ::testing::Test {
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
        std::cout << "Testing Remove_if_Test\n";

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


		src_value_type val = 0;

		int index = rand()%length;
		src_value_type v = ref_inputVec[index];


			
        int n = Serial_remove(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), v, val);
        //Call Bolt Now 
        bolt::amp::remove_copy(inputVec.begin(), inputVec.end(), outputVec.begin(), v);
		for(int i=n; i<length; i++)
			outputVec[i] = val;
        cmpArrays(ref_outputVec, outputVec);


        n = Serial_remove_if(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), predicate, val);
        //Call Bolt Now 
        bolt::amp::remove_copy_if(inputVec.begin(), inputVec.end(), outputVec.begin(), predicate);
		for(int i=n; i<length; i++)
			outputVec[i] = val;
        cmpArrays(ref_outputVec, outputVec);


        n = Serial_remove(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), v, val);
        //Call Bolt Now 
        bolt::amp::remove(inputVec.begin(), inputVec.end(), v);
		for(int i=n; i<length; i++)
			inputVec[i] = val;
        cmpArrays(ref_inputVec, inputVec);


        n = Serial_remove_if(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), predicate, val);
        //Call Bolt Now 
        bolt::amp::remove_if(inputVec.begin(), inputVec.end(), predicate);
		for(int i=n; i<length; i++)
			inputVec[i] = val;
        /*for(int i=0; i< length; i++)
            std::cout << "i = " << i <<"-- ref =" << ref_inputVec[i] << " out = " << inputVec[i] << "\n";*/
        cmpArrays(ref_inputVec, inputVec);

	
    }

    void test_w_stencil( )
    {
        int length = 100;
        std::cout << "Testing Remove_if_Test\n";

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


		src_value_type val = 0;
		int index = rand()%length;
		src_value_type v = ref_inputVec[index];

        int n = Serial_remove_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_outputVec.begin(), ref_stencilVec.begin(),  predicate, val);
        //Call Bolt Now
        bolt::amp::remove_copy_if(inputVec.begin(), inputVec.end(), stencilVec.begin(), outputVec.begin(), predicate);
		for(int i=n; i<length; i++)
			outputVec[i] = val;
        cmpArrays(ref_outputVec, outputVec);

		n = Serial_remove_if_stencil(ref_inputVec.begin(), ref_inputVec.end(), ref_inputVec.begin(), ref_stencilVec.begin(),  predicate, val);
        //Call Bolt Now
        bolt::amp::remove_if(inputVec.begin(), inputVec.end(), stencilVec.begin(), predicate);
		for(int i=n; i<length; i++)
			inputVec[i] = val;
        cmpArrays(ref_inputVec, inputVec);

    }

};


TYPED_TEST_CASE_P(Remove_if_Test);
TYPED_TEST_CASE_P(Remove_if_raw_test);

TYPED_TEST_P(Remove_if_Test, without_stencil) 
{

    test_wo_stencil();

}

TYPED_TEST_P(Remove_if_Test, with_stencil) 
{

    test_w_stencil();

}

TYPED_TEST_P(Remove_if_raw_test, raw_pointers) 
{

    test_raw_ptrs();

}

REGISTER_TYPED_TEST_CASE_P(Remove_if_Test, without_stencil, with_stencil);

typedef std::tuple<std::vector<int>,          std::vector<int>,        std::vector<int>,        is_even<int> >       INT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<float>,        std::vector<float>,      std::vector<float>,      is_even<float> >     FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<UDD>,          std::vector<UDD>,        std::vector<UDD>,        is_even_a<UDD> >     UDD_VEC_IS_EVEN_T;
typedef ::testing::Types<
        INT_VEC_IS_EVEN_T,
        FLOAT_VEC_IS_EVEN_T,
        UDD_VEC_IS_EVEN_T> STDVectorIsEvenTypes;

typedef std::tuple<bolt::amp::device_vector<int>,       bolt::amp::device_vector<int>,      bolt::amp::device_vector<int>,     is_even<int> >      INT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<float>,     bolt::amp::device_vector<float>,    bolt::amp::device_vector<float>,   is_even<float> >    FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<UDD>,       bolt::amp::device_vector<UDD>,      bolt::amp::device_vector<UDD>,     is_even_a<UDD> >    UDD_DV_IS_EVEN_T;
typedef ::testing::Types<
        INT_DV_IS_EVEN_T,
        FLOAT_DV_IS_EVEN_T,
        UDD_DV_IS_EVEN_T> DeviceVectorIsEvenTypes;

REGISTER_TYPED_TEST_CASE_P(Remove_if_raw_test, raw_pointers);
typedef std::tuple<int,          int,        int,     is_even<int> >                 INT_IS_EVEN_T;
typedef std::tuple<float,        float,      float,   is_even<float> >               FLOAT_IS_EVEN_T;
typedef std::tuple<UDD,          UDD,        UDD,     is_even_a<UDD> >                 UDD_IS_EVEN_T;
typedef ::testing::Types<
        INT_IS_EVEN_T,
        FLOAT_IS_EVEN_T,
        UDD_IS_EVEN_T> POD_RawPtrIsEvenTypes;



INSTANTIATE_TYPED_TEST_CASE_P(STDVectorTests, Remove_if_Test, STDVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(DeviceVectorTests, Remove_if_Test, DeviceVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(RawPtrTests, Remove_if_raw_test, POD_RawPtrIsEvenTypes);



//Add the test team test cases here. 

//Add the EPR test cases here with the EPR number. 

TEST(sanity_remove_if_ctl_array, double_bolt_array_ctl_DocSample){
       const int N = 10;
       double in1[N] = {-5.000,  0.000,  2.002,  3.000,  2.001, 4.000, -2.000,  1.000,  2.002,  3.000};
       double in2[N] = {-5.000,  0.000,  2.002,  3.000,  2.001, 4.000, -2.000,  1.000,  2.002,  3.000};
       int stencil[10] = {0, 1, 1, 1, 0, 1, 1, 1, 0, 1};

       ::Concurrency::accelerator accel(::Concurrency::accelerator::default_accelerator);
       bolt::amp::control ctl(accel);
       
       bolt::amp::remove_if(ctl, in1, in1 + 10, is_even<double> ());
       std::remove_if(in2, in2 + 10, is_even<double> ());

       for(int i = 0; i < N; i++ ) {

       //     EXPECT_EQ(in1[i], in2[i]);
              std::cout<<"Values is ----> " << in1[i] << "  " << in2[i] << "\n";

       }
}

TEST(sanity_remove_if_ctl_std_rand, double_bolt_std_rand_DocSample){
       int N = 100;
       std::vector<double> in1(N);
       std::vector<double> in2(N);

       for(int i = 0; i < N; i++) {
              in1[i] = (double) rand();
              in2[i] = in1[i];
       }
       
       bolt::amp::remove_if(in1.begin(), in1.end(), is_odd<double> ());
       std::remove_if(in2.begin(), in2.end(), is_odd<double> ());

       for(int i = 0; i < N; i++ ) {

              EXPECT_EQ(in1[i], in2[i]);

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



