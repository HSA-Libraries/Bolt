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
#if !defined( TEST_COMMON_AMP_H )
#define TEST_COMMON_AMP_H
#pragma once

#include <array>
#include <gtest/gtest.h>

size_t numFailures;
bool resetNumFailures = true;


#define BOLT_TEST_MAX_FAILURES 8

#define BOLT_TEST_RESET_FAILURES \
    size_t numFailures = 0;

#define BOLT_TEST_INCREMENT_FAILURES \
    if ( !(ref[ i ] == calc[ i ]) ) numFailures++; \
    if ( numFailures > BOLT_TEST_MAX_FAILURES ) { \
        break; \
    }

#define BOLT_TEST_INCREMENT_FAILURES_PTR \
    if ( !(refptr[ i ] == calcptr[ i ]) ) numFailures++; \
    if ( numFailures > BOLT_TEST_MAX_FAILURES ) { \
        break; \
    }



template< typename T >
::testing::AssertionResult cmpArrays( const T ref, const T calc, size_t N )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( N ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

template< typename T, size_t N >
::testing::AssertionResult cmpArrays( const T (&ref)[N], const T (&calc)[N] )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( N ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

template< typename T, size_t N >
struct cmpStdArray
{
    static ::testing::AssertionResult cmpArrays( const std::array< T, N >& ref, const std::array< T, N >& calc )
    {
        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( N ); ++i )
        {
            BOLT_TEST_INCREMENT_FAILURES
            EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};


template< size_t N >
struct cmpStdArray< float, N >
{
    static ::testing::AssertionResult cmpArrays( const std::array< float, N >& ref, const std::array< float, N >& calc )
    {
        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( N ); ++i )
        {
            BOLT_TEST_INCREMENT_FAILURES
            EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

template< size_t N >
struct cmpStdArray< double, N >
{
    static ::testing::AssertionResult cmpArrays( const std::array< double, N >& ref, const std::array< double, N >& calc )
    {
        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( N ); ++i )
        {
            BOLT_TEST_INCREMENT_FAILURES
            EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};

//  The following cmpArrays verify the correctness of std::vectors's
template< typename T >
::testing::AssertionResult cmpArrays( const std::vector< T >& ref, const std::vector< T >& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

::testing::AssertionResult cmpArrays( const std::vector< float >& ref, const std::vector< float >& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}

#if TEST_DOUBLE
::testing::AssertionResult cmpArrays( const std::vector< double >& ref, const std::vector< double >& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
#endif

//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename S, typename B >
typename std::enable_if< std::is_same< typename std::iterator_traits<typename S::iterator >::value_type, float >::value,  
                         ::testing::AssertionResult >::type
cmpArrays( const S& ref, const B& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        //the two float values are almost equal 
        //By "almost equal", we mean the two values are within 4 ULP's from each other. 
        //http://code.google.com/p/googletest/wiki/AdvancedGuide#Floating-Point_Comparison
        EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i; 
    }

    return ::testing::AssertionSuccess( );
}

template< typename S, typename B >
typename std::enable_if< std::is_same< typename std::iterator_traits<typename S::iterator >::value_type, double >::value,  
                         ::testing::AssertionResult >::type
cmpArrays( const S& ref, const B& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
        BOLT_TEST_INCREMENT_FAILURES
        //the two float values are almost equal 
        //By "almost equal", we mean the two values are within 4 ULP's from each other. 
        //http://code.google.com/p/googletest/wiki/AdvancedGuide#Floating-Point_Comparison
        EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i; 
    }

    return ::testing::AssertionSuccess( );
}

//  A very generic template that takes two container, and compares their values assuming a vector interface
template< typename S, typename B >
typename std::enable_if< !(std::is_same< typename std::iterator_traits<typename S::iterator >::value_type, float >::value ||
                           std::is_same< typename std::iterator_traits<typename S::iterator >::value_type, double >::value) , 
                         ::testing::AssertionResult >::type
//typename std::enable_if< std::is_same< typename std::iterator_traits<>::value_type >::value,  ::testing::AssertionResult >::type
 cmpArrays( const S& ref, const B& calc )
{
    BOLT_TEST_RESET_FAILURES
    for( int i = 0; i < static_cast<int>( ref.size( ) ); ++i )
    {
       
        //the two float values are almost equal 
        //By "almost equal", we mean the two values are within 4 ULP's from each other. 
        //http://code.google.com/p/googletest/wiki/AdvancedGuide#Floating-Point_Comparison
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i; 
    }

    return ::testing::AssertionSuccess( );
}

template< typename T1,typename T2>
::testing::AssertionResult 
cmpArrays( const T1& ref, typename bolt::amp::device_vector<T2> &calc )
{
		typename bolt::amp::device_vector<T2>::pointer calcptr =  calc.data( );
        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( ref.size() ); ++i )
        {
            EXPECT_EQ( ref[ i ], calcptr[ i ] ) << _T( "Where i = " ) << i ;
        }
        return ::testing::AssertionSuccess( );
}

template< typename T1,typename T2>
::testing::AssertionResult 
cmpArrays( typename bolt::amp::device_vector<T1> &ref, typename bolt::amp::device_vector<T2> &calc )
{

        typename bolt::amp::device_vector<T1>::pointer refptr =  ref.data( );
		typename bolt::amp::device_vector<T2>::pointer calcptr =  calc.data( );

        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( ref.size() ); ++i )
        {
            BOLT_TEST_INCREMENT_FAILURES_PTR
            EXPECT_EQ( refptr[ i ], calcptr[ i ] ) << _T( "Where i = " ) << i;
        }
      return ::testing::AssertionSuccess( );
}

template< typename T1,typename T2>
::testing::AssertionResult 
cmpArrays( typename bolt::amp::device_vector<T1> &ref, typename bolt::amp::device_vector<T2> &calc, size_t N )
{

        typename bolt::amp::device_vector<T1>::pointer refptr =  ref.data( );
		typename bolt::amp::device_vector<T2>::pointer calcptr =  calc.data( );

        BOLT_TEST_RESET_FAILURES
        for( int i = 0; i < static_cast<int>( N ); ++i )
        {
            BOLT_TEST_INCREMENT_FAILURES_PTR
            EXPECT_EQ( refptr[ i ], calcptr[ i ] ) << _T( "Where i = " ) << i;
        }
      return ::testing::AssertionSuccess( );
}



template <typename int N=2<<28>
class TestBuffer
{
    public:
    int *randomValues;
    TestBuffer()
    {
        int large_store_sz = N; 
        randomValues = new int[large_store_sz];
        std::cout << "Initializing Random value Store\n";
        for(int i=0; i<large_store_sz; i++ )
        {
            randomValues[i] = rand()*rand(); 
        }
        std::cout << "Finished initializing Random value Store\n";
    }

    template <typename Container>
    typename std::enable_if< std::is_same<typename std::iterator_traits<typename Container::iterator>::iterator_category,
                                 std::random_access_iterator_tag
                                >::value
                  >::type
    fill_value(Container &cont, typename std::iterator_traits<typename Container::iterator>::value_type value)
    {
        typedef typename Container::iterator iterator;
        iterator begin = cont.begin();
        iterator end   = cont.end();
        while( begin != end )
        {
            *begin = value;
            begin++;
        }
    }

    template <typename Container>
    typename std::enable_if< std::is_same<typename std::iterator_traits<typename Container::iterator>::iterator_category,
                                 bolt::amp::device_vector_tag
                                >::value
                  >::type
    fill_value(Container &cont, typename std::iterator_traits<typename Container::iterator>::value_type value)
    {
        typedef typename Container::iterator iterator;
        iterator begin = cont.begin();
        iterator end   = cont.end();
        size_t sz = cont.size();
        typedef typename std::iterator_traits<iterator>::value_type value_type;
        value_type *mapped_ptr = cont.data();
        std::fill_n(stdext::make_checked_array_iterator( randomValues, sz), sz, value );
    }

    template <typename Container>
    typename std::enable_if< std::is_same<typename std::iterator_traits<typename Container::iterator>::iterator_category,
                                 std::random_access_iterator_tag
                                >::value
                  >::type
    init(Container &cont)
    {
        typedef typename Container::iterator iterator;
        iterator begin = cont.begin();
        iterator end   = cont.end();
        size_t sz = cont.size();
        std::copy(randomValues, randomValues + sz, cont.begin());
    }

    template <typename Container>
    typename std::enable_if< std::is_same<typename std::iterator_traits<typename Container::iterator>::iterator_category,
                                          bolt::amp::device_vector_tag
                                         >::value
                           >::type
    init(Container &cont)
    {
        typedef typename Container::iterator iterator;
        iterator begin = cont.begin();
        iterator end   = cont.end();
        size_t sz = cont.size();
        typedef typename std::iterator_traits<iterator>::value_type value_type;
        value_type *mapped_ptr = cont.data();
        std::copy_n(randomValues, sz, stdext::make_checked_array_iterator( mapped_ptr, sz) );
    }

    template <typename value_type>
    void init (value_type * input, int length)
    {
        std::copy_n(randomValues, length, stdext::make_checked_array_iterator( input, length) );
    }

    ~TestBuffer()
    {
        delete randomValues;
    }
};



#endif