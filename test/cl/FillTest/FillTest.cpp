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
#include "common/myocl.h"
#include <bolt/cl/fill.h>

#define STRUCT 1
#define FILL_GOOGLE_TEST 1

#if FILL_GOOGLE_TEST
#include <gtest/gtest.h>
#include <array>


//////////////////////////////////////////////////////////////////////////////////////////////
//These test cases are used to ensure that fill works with structs
//////////////////////////////////////////////////////////////////////////////////////////////

#if STRUCT
int teststruct( int length );
int teststruct_n( int length );
BOLT_FUNCTOR(samp,struct samp
{
	int x,y;

};
);

#endif



#if STRUCT
/*	
*
*Fill and Fill_N Host/Device Vector Tests
*
*/

 // Fill 
int teststruct( int length )
{
	int errCnt=0;
	static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
	bolt::cl::device_vector<samp>dv(length);
	struct samp s1,temp;			/*temp is used to map back the device_vector to host_vector*/
	s1.x=10;
	s1.y=20;
    std::fill(gold.begin(), gold.end(),s1);
    bolt::cl::fill(hv.begin(), hv.end(),s1);
	bolt::cl::fill(dv.begin(), dv.end(),s1);
    //check results

    for (int i=0; i<length ; i++) {
		temp = dv[i];
		if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
				errCnt++;
				if (errCnt < maxErrCnt) 
				{
				std::cout<<"\tMISMATCH"<<std::endl;
				}
				else if (errCnt == maxErrCnt) 
				{
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
				}
		}
	}
	 if ( errCnt == 0 ) {
		printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
	} else {
		printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", fName.c_str(), length);
	};
    fflush(stdout);
	return errCnt;
}

// Fill_n 
int teststruct_n( int length )
{
	int errCnt=0;
	static const int maxErrCnt = 10;
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<samp> gold(length);
    std::vector<samp> hv(length);
	bolt::cl::device_vector<samp>dv(length);
	struct samp s1,temp;			/*temp is used to map back the device_vector to host_vector*/
	s1.x=10;
	s1.y=20;
    std::fill_n(gold.begin(), length,s1);
    bolt::cl::fill_n(hv.begin(), length,s1);
	bolt::cl::fill_n(dv.begin(), length,s1);
    //check results

    for (int i=0; i<length ; i++) {
		temp = dv[i];
		if((gold[i].x!=hv[i].x)&&(gold[i].y!=hv[i].y)&&(gold[i].x!=temp.x)&&(gold[i].y!=temp.y)){
				errCnt++;
				if (errCnt < maxErrCnt) 
				{
				std::cout<<"\tMISMATCH"<<std::endl;
				}
				else if (errCnt == maxErrCnt) 
				{
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
				}
		}
	}
	 if ( errCnt == 0 ) {
		printf(" PASSED %20s Correct for all %6i elements.\n", fName.c_str(), length);
	} else {
		printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", fName.c_str(), length);
	};
    fflush(stdout);
	return errCnt;
}


#endif

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
template< size_t N >
struct cmpStdArray< float, N >
{
    static ::testing::AssertionResult cmpArrays( const std::array< float, N >& ref, const std::array< float, N >& calc )
    {
        for( size_t i = 0; i < N; ++i )
        {
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
        for( size_t i = 0; i < N; ++i )
        {
            EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
        }

        return ::testing::AssertionSuccess( );
    }
};
template< typename T >
::testing::AssertionResult cmpArrays( const std::vector< T >& ref, const std::vector< T >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
::testing::AssertionResult cmpArrays( const std::vector< float >& ref, const std::vector< float >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_FLOAT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
::testing::AssertionResult cmpArrays( const std::vector< double >& ref, const std::vector< double >& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_DOUBLE_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}
template< typename S, typename B >
::testing::AssertionResult cmpArrays( const S& ref, const B& calc )
{
    for( size_t i = 0; i < ref.size( ); ++i )
    {
        EXPECT_EQ( ref[ i ], calc[ i ] ) << _T( "Where i = " ) << i;
    }

    return ::testing::AssertionSuccess( );
}


class HostIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostIntVector( ): stdInput( GetParam( ), -1 ), boltInput( GetParam( ), -1 )
    {}

protected:
    std::vector< int > stdInput, boltInput;
};

class DevIntVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    DevIntVector( ): stdInput( GetParam( ), -1 ), boltInput( GetParam( ), -1 )
    {}

protected:
    std::vector< int > stdInput;
    bolt::cl::device_vector< int > boltInput;
};

class HostDblVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    HostDblVector( ): stdInput( GetParam( ), -1.0 ), boltInput( GetParam( ), -1.0 )
    {}

protected:
    std::vector< double > stdInput, boltInput;
};

class DevDblVector: public ::testing::TestWithParam< int >
{
public:
    //  Create an std and a bolt vector of requested size, and initialize all the elements to -1
    DevDblVector( ): stdInput( GetParam( ), -1.0 ), boltInput( GetParam( ), -1.0 )
    {}

protected:
    std::vector< double > stdInput;
    bolt::cl::device_vector< double > boltInput;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//Test Cases for Fill
//////////////////////////////////////////////////////////////////////////////////////////////


TEST_P( HostIntVector, Fill )
{
    int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::cl::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( HostDblVector, Fill )
{
    double val = CL_M_E;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::cl::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevIntVector, Fill )
{
    int val = 73;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::cl::fill( boltInput.begin( ), boltInput.end( ), val );

    cmpArrays( stdInput, boltInput );
}

TEST_P( DevDblVector, Fill )
{
    double val = CL_M_E;
    std::fill(  stdInput.begin( ),  stdInput.end( ), val );
    bolt::cl::fill( boltInput.begin( ), boltInput.end( ), val );
    //  Loop through the array and compare all the values with each other
    cmpArrays( stdInput, boltInput );
}

//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
//Test Cases for Fill_N
//////////////////////////////////////////////////////////////////////////////////////////////
TEST_P( HostIntVector, Fill_n )
{
    int val = 73;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin( ),size,val);
    bolt::cl::fill_n(boltInput.begin( ),size,val);

    cmpArrays(stdInput, boltInput);
}


TEST_P( HostDblVector, Fill_n )
{
    double val = CL_M_E;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val );
    bolt::cl::fill_n(boltInput.begin(),size,val );

    cmpArrays(stdInput,boltInput);
}

TEST_P( DevIntVector, Fill_n )
{
    int val = 73;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin(),size,val);
    bolt::cl::fill_n(boltInput.begin(),size,val);

    cmpArrays(stdInput, boltInput);
}

TEST_P( DevDblVector, Fill_n )
{
    double val = CL_M_E;
    size_t size = stdInput.size();
    std::fill_n(stdInput.begin( ),size, val );
    bolt::cl::fill_n(boltInput.begin( ),size,val );
    cmpArrays( stdInput, boltInput );
}

//////////////////////////////////////////////////////////////////////////////////////////////

INSTANTIATE_TEST_CASE_P( FillSmall, HostIntVector, ::testing::Range(1,256,3));
INSTANTIATE_TEST_CASE_P( FillLarge, HostIntVector, ::testing::Range(1023,1050000,350001));
INSTANTIATE_TEST_CASE_P( FillSmall, DevIntVector,  ::testing::Range(2,256,3));
INSTANTIATE_TEST_CASE_P( FillLarge, DevIntVector,  ::testing::Range(1024,1050000,350003));
INSTANTIATE_TEST_CASE_P( FillSmall, HostDblVector, ::testing::Range(3,256,3));
INSTANTIATE_TEST_CASE_P( FillLarge, HostDblVector, ::testing::Range(1025,1050000, 350007 ) );
INSTANTIATE_TEST_CASE_P( FillSmall, DevDblVector,  ::testing::Range(4, 256, 3 ) );
INSTANTIATE_TEST_CASE_P( FillLarge, DevDblVector,  ::testing::Range(1026, 1050000, 350011 ) );

BOLT_FUNCTOR(characters,struct characters
{
    char c;
    int i;

    bool operator == (const characters &rhs) const
    {
        if(c == rhs.c && i == rhs.i)
            return true;
        return false;

    }

};
);

//It fills character pointers properly
TEST( CharPointer, Fill )
{
    int size = 100; 

    std::vector<characters> vs(size);
    std::vector<characters> dvs(size);
    characters c_str;
    c_str.c = 'A';
    c_str.i = 10;
    std::fill(vs.begin(), vs.end(), c_str); 
    bolt::cl::fill(dvs.begin(), dvs.end(),c_str ); 
    for (int i = 0; i < size; ++i)
    { 

        EXPECT_EQ(vs[i], dvs[i]);
    }
}

//Uncomment this to see the string fill bug
//char * testCharP(){ 
//char * str = "Simple String"; 
//return str; 
//} 
//
//
//std::string testString(){ 
//std::string str = "Simple String"; 
//return str; 
//} 
//TEST( String, Fill )
//{
//    int size = 30; 
//
//    std::vector<std::string> vs(size);
//    //std::vector<std::string> dvs(size);
//    bolt::cl::device_vector<std::string> dvs(size);
//
//    std::fill(vs.begin(), vs.end(), testString()); 
//    bolt::cl::fill(dvs.begin(), dvs.end(), testString()); 
//    for (int i = 0; i < size; ++i)
//    { 
//        std::cout<<vs[i]<<std::endl;
//        //EXPECT_STREQ(vs[i],dvs[i]);
//    }
//    //cmpArrays(vs,dvs);
//    
//}



TEST (simpleTest, basicDataBoltClDevVectAutoConvertCheck)
{ 
    size_t size =10; 
    int iValue = 48; 
    union ieeeconvert
    {
        float x;
        int y;
    }converter;
    double dValue = 48.6;
    float fValue = 48.0;
    double dNan = std::numeric_limits<double>::signaling_NaN();
    bolt::cl::device_vector<int> dv(size); 
    std::vector<int> hv(size);
    bolt::cl::device_vector<double> ddv(size); 
    std::vector<double> dhv(size);
    bolt::cl::device_vector<float> fdv(size); 
    std::vector<float> fhv(size);    


    ////////////////////////////////////////////////////
    // No casting needed here!
    ////////////////////////////////////////////////////

    bolt::cl::fill(dv.begin(), dv.end(),iValue);  
    std::fill(hv.begin(), hv.end(),iValue); 
    cmpArrays(hv,dv);

    std::fill(dhv.begin(), dhv.end(), dValue);
    bolt::cl::fill(ddv.begin(), ddv.end(), dValue); 
    cmpArrays(dhv,ddv);

    ////////////////////////////////////////////////////
    // Test cases to verify casting
    ////////////////////////////////////////////////////

    std::fill(hv.begin(), hv.end(), static_cast< int >( dValue ) ); 
    bolt::cl::fill(dv.begin(), dv.end(), dValue);
    cmpArrays(hv,dv);

    std::fill(hv.begin(), hv.end(), static_cast< int >( fValue ) ); 
    bolt::cl::fill(dv.begin(), dv.end(), fValue);
    cmpArrays(hv,dv);

    bolt::cl::fill(ddv.begin(), ddv.end(),iValue);
    std::fill(dhv.begin(), dhv.end(),iValue); 
    cmpArrays(dhv,ddv);

    std::fill(dhv.begin(), dhv.end(), fValue); 
    bolt::cl::fill(ddv.begin(), ddv.end(), fValue); 
    cmpArrays(dhv,ddv);


    converter.y =_FPCLASS_ND;

    ////////////////////////////////////////////////////
    // This verifies that it works with Denormals 
    ////////////////////////////////////////////////////

    //std::fill(dhv.begin(), dhv.end(), converter.x); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), converter.x);
    //cmpArrays(dhv,ddv);

    std::fill(fhv.begin(), fhv.end(), converter.x); 
    bolt::cl::fill(fdv.begin(), fdv.end(), converter.x);
    cmpArrays(fhv,fdv);

    ////////////////////////////////////////////////////
    // Fill some NANs: It fills, but the test fails
    //                 since you can't compare NANs
    ////////////////////////////////////////////////////

    //std::fill(fhv.begin(), fhv.end(), dNan); 
    //bolt::cl::fill(fdv.begin(), fdv.end(), dNan);
    //cmpArrays(fhv,fdv);

    //std::fill(dhv.begin(), dhv.end(), dNan); 
    //bolt::cl::fill(ddv.begin(), ddv.end(), dNan);
    //cmpArrays(dhv,ddv);
    //std::cout<<dhv[0]<<" Hst"<<ddv[0]<<" Device"<<std::endl;


} 


TEST(Fill, AllRunModes)
{
  int length = 1024;
  bolt::cl::control ctlA, ctlCPU, ctlMCPU;

  // Try with Automatic runmode
  ctlA.setForceRunMode(bolt::cl::control::Automatic);

  std::vector<int> hA(length), dA(length);
  bolt::cl::device_vector<int> dVA(length);

  std::fill(hA.begin(), hA.end(), 20);
  bolt::cl::fill(ctlA,dA.begin(), dA.end(), 20);

  cmpArrays(hA,dA);

  // Try with SerialCpu runmode
  ctlCPU.setForceRunMode(bolt::cl::control::SerialCpu);  
  std::fill(hA.begin(), hA.end(), 10);
  bolt::cl::fill(ctlCPU, dA.begin(), dA.end(), 10);

  cmpArrays(hA,dA);

  // Try with MultiCoreCpu runmode
  ctlMCPU.setForceRunMode(bolt::cl::control::MultiCoreCpu);  
  std::fill(hA.begin(), hA.end(), 50);
  bolt::cl::fill(ctlMCPU, dA.begin(), dA.end(), 50);

  cmpArrays(hA,dA);

    // Try DV with MultiCoreCpu runmode
  ctlMCPU.setForceRunMode(bolt::cl::control::MultiCoreCpu);  
  std::fill(hA.begin(), hA.end(), 120);
  bolt::cl::fill(ctlMCPU, dVA.begin(), dVA.end(),120);
  cmpArrays(hA,dVA);





}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

/******************************************************************************
 * checkResults
 *      compare std:: and bolt::cl:: results
 *      returns number of errors
 *		For testing struct types checkResults is embedded in the test function.
 *****************************************************************************/
template<typename InputIterator1, typename InputIterator2>
int checkResults(std::string &msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 10;
	size_t sz = end1-first1 ;
	for (int i=0; i<sz ; i++) {
		if (first1 [i] != *(first2 + i) ) {
			errCnt++;
			if (errCnt < maxErrCnt) {
				std::cout << "\tMISMATCH[" << i << "] " << msg << " STL= " << first1[i] << "  BOLT=" << *(first2 + i) << std::endl;
			} else if (errCnt == maxErrCnt) {
				std::cout << "\tMax error count reached; no more mismatches will be printed...\n";
			}
		};
	};

    if ( errCnt == 0 ) {
		printf(" PASSED %20s Correct for all %6i elements.\n", msg.c_str(), sz);
	} else {
		printf("*FAILED %20s Mismatch for %6i /%6i elements.\n", msg.c_str(), sz);
	};
    fflush(stdout);

	return errCnt;
};

/******************************************************************************
 * Pause Program to see Results
 *****************************************************************************/
void waitForEnter()
{
    std::cout << "Press <ENTER> to continue." << std::endl;
    std::cin.clear();
    std::cin.ignore(1, '\n');
}



/******************************************************************************
 * Tests
 *****************************************************************************/
int testFill1DevVec( int length );
int testFill2DevVec( int length );
int testFillN1DevVec( int length );
int testFillN2DevVec( int length );
int testFill1HostVec( int length );
int testFill2HostVec( int length );
int testFillN1HostVec( int length );
int testFillN2HostVec( int length );


/******************************************************************************
 * Main
 *****************************************************************************/
int main(int argc, char* argv[])
{
    // Test several vector lengths
    std::vector<int> lengths;
    lengths.push_back(0);
    lengths.push_back(1);
    lengths.push_back(63);
    lengths.push_back(64);
    lengths.push_back(65);
    lengths.push_back(1023);
    lengths.push_back(1024);
    lengths.push_back(1025);
    lengths.push_back(16384+1);

    int testsPassed = 0;
    int testsFailed = 0;

    // for each test length
    for (int iter = 0; iter < lengths.size(); iter++)
    {
        int i = iter % lengths.size();
        int errorCount = 0;

        /***********************************************************
         * Test Device Vectors
         **********************************************************/

		//Fill 1
        errorCount = testFill1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2DevVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;


        /***********************************************************
         * Test Host Vectors
         **********************************************************/

        // Fill 1
        errorCount = 0;
        errorCount = testFill1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill 2
        errorCount = testFill2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 1
        errorCount = testFillN1HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

        // Fill_N 2
        errorCount = testFillN2HostVec(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

#if STRUCT
		// Fill struct
		errorCount = teststruct(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;

		errorCount = teststruct_n(lengths[i]);
        if ( errorCount == 0 )
            testsPassed++;
        else
            testsFailed++;
    
#endif
	}
    // Print final results
    printf("Final Results:\n");
    printf("%9i Tests Passed\n", testsPassed);
    printf("%9i Tests Failed\n", testsFailed);

    // Wait to exit
    waitForEnter();
    return 1;
}

/***********************************************************
 * Device Vector Functions
 **********************************************************/

// Fill 1
int testFill1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill
    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::cl::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill
    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::cl::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

 // Fill_N 1
int testFillN1DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill_N
    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::cl::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 2
int testFillN2DevVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    bolt::cl::device_vector<float> dv(length);
	//Call Fill_N
    std::fill_n(gold.begin(), length, 0.f);
    bolt::cl::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}



/***********************************************************
 * Host Vector Functions
 **********************************************************/

// Fill 1
int testFill1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 3.14159f);
    bolt::cl::fill(dv.begin(), dv.end(), 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill 2
int testFill2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill(gold.begin(), gold.end(), 0.f);
    bolt::cl::fill(dv.begin(), dv.end(), 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN1HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 3.14159f);
    bolt::cl::fill_n(dv.begin(), length, 3.14159f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}

// Fill_N 1
int testFillN2HostVec( int length )
{
    // function name for reporting
    std::string fName = __FUNCTION__;
    // containers
    std::vector<float> gold(length);
    std::vector<float> dv(length);

    std::fill_n(gold.begin(), length, 0.f);
    bolt::cl::fill_n(dv.begin(), length, 0.f);
    //check results
    return checkResults(fName, gold.begin(), gold.end(), dv.begin());
}


#endif