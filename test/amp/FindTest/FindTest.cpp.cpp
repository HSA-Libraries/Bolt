#define TEST_DOUBLE 1
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
#include <bolt/amp/find.h>
#include <gtest/gtest.h>
#include <array>
#include "common/test_common.h"

class IsOdd
{
public:
	bool operator()(int i) const restrict(cpu, amp)
	{
	  return (i%7 == 0)?1:0;
	}
};


TEST( find_if, test)
{
    int aSize = 1<<16;
    std::vector<int> stdInput(aSize);
	std::vector<int> stdInput2(aSize);

	for (int i = 0; i<aSize; i++) {
        stdInput[i] = (int)i+1;
		stdInput2[i] = (int)i+1;
    };
    bolt::amp::device_vector<int> boltInput(stdInput2.begin(), stdInput2.end());


    std::vector<int>::iterator itr = std::find_if(stdInput.begin(), stdInput.end(), IsOdd());

    bolt::amp::device_vector<int>::iterator dv_itr = bolt::amp::find_if(boltInput.begin(), boltInput.end(), IsOdd());


	if(itr != stdInput.end() && dv_itr!= boltInput.end() )
		EXPECT_EQ( *itr, *dv_itr );
	else
        std::cout << "Element not found\n";

};


TEST( find_if_not, test)
{
    int aSize = 1<<16;
    std::vector<int> stdInput(aSize);
	std::vector<int> stdInput2(aSize);

	for (int i = 0; i<aSize; i++) {
        stdInput[i] = (int)i;
		stdInput2[i] = (int)i;
    };
    bolt::amp::device_vector<int> boltInput(stdInput2.begin(), stdInput2.end());


    std::vector<int>::iterator itr = std::find_if_not(stdInput.begin(), stdInput.end(), IsOdd());
   
    bolt::amp::device_vector<int>::iterator dv_itr = bolt::amp::find_if_not(boltInput.begin(), boltInput.end(), IsOdd());
    
	if(itr != stdInput.end() && dv_itr!= boltInput.end() )
		EXPECT_EQ( *itr, *dv_itr );
	else
        std::cout << "Element not found\n";

};



TEST( find, test)
{
    int aSize = 1<<16;
    std::vector<int> stdInput(aSize);
	std::vector<int> stdInput2(aSize);

	for (int i = 0; i<aSize; i++) {
        stdInput[i] = (int)i;
		stdInput2[i] = (int)i;
    };
    bolt::amp::device_vector<int> boltInput(stdInput2.begin(), stdInput2.end());

	int index = rand()%aSize;
	int val = stdInput[index];

    std::vector<int>::iterator itr = std::find(stdInput.begin(), stdInput.end(), val);

    bolt::amp::device_vector<int>::iterator dv_itr = bolt::amp::find(boltInput.begin(), boltInput.end(), val);


    if(itr != stdInput.end() && dv_itr!= boltInput.end() )
		EXPECT_EQ( *itr, *dv_itr );
	else
        std::cout << "Element not found\n";


};


TEST( find_2, test)
{
    int aSize = 10;
    std::vector<int> stdInput(aSize);
	std::vector<int> stdInput2(aSize);

	for (int i = 0; i<aSize; i++) {
        stdInput[i] = (int)i;
		stdInput2[i] = (int)i;
    };
    bolt::amp::device_vector<int> boltInput(stdInput2.begin(), stdInput2.end());

	int val = aSize;

    std::vector<int>::iterator itr = std::find(stdInput.begin(), stdInput.end(), val);

    bolt::amp::device_vector<int>::iterator dv_itr = bolt::amp::find(boltInput.begin(), boltInput.end(), val);

	if(itr != stdInput.end() && dv_itr!= boltInput.end() )
		std::cout << "Element found in input: " << *itr << '\n';
    else
        std::cout << "Element not found\n";


};


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

template <typename UDD>
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

template <typename T>
struct is_less_than_zero
{

    bool operator()(T x) const restrict(amp, cpu)
    {
      return x < 0;
    }
};

template <typename UDD>
struct is_less_than_zero_udd
{

    bool operator()(UDD x) const restrict(amp, cpu)
    {
      return (x.a < 0 && x.b < 0);
    }
};


template <typename T>
class Find_if_raw_test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type src_value_type;
    typedef typename std::tuple_element<1, T>::type Predicate;
    Predicate predicate;


    void test_raw_ptrs()
    {
        int length = 1<<16;
        src_value_type* pInput       = new src_value_type[length];
        test_buffer.init(pInput, length);

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);
        
		int n = rand()%length;
		src_value_type val = ref_inputVec[n];


		src_value_type val2 = ref_inputVec[0];
		src_value_type val3 = ref_inputVec[length-1];
		src_value_type val4 = rand();

		//Hit at any random position
        std::vector<src_value_type>::iterator itr1 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val);
        src_value_type * bolt_itr1 = bolt::amp::find(pInput, pInput + length, val);        
        EXPECT_EQ( *itr1, *bolt_itr1 );

		//Hit at the first position
		std::vector<src_value_type>::iterator itr4 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val2);
        src_value_type * bolt_itr4 = bolt::amp::find(pInput, pInput + length, val2);        
        EXPECT_EQ( *itr4, *bolt_itr4 );

		//hit at the last position
		std::vector<src_value_type>::iterator itr5 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val3);
        src_value_type * bolt_itr5 = bolt::amp::find(pInput, pInput + length, val3); 
        EXPECT_EQ( *itr5, *bolt_itr5 );

		//May be A Hit or Miss
		std::vector<src_value_type>::iterator itr6 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val4);
        src_value_type * bolt_itr6 = bolt::amp::find(pInput, pInput + length, val4);        
        if(itr6 != ref_inputVec.end() && (bolt_itr6 != (pInput + length)))
		    EXPECT_EQ( *itr6, *bolt_itr6 );
	    else
            std::cout << "Element not found\n";


        std::vector<src_value_type>::iterator itr2 = std::find_if(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        src_value_type * bolt_itr2 = bolt::amp::find_if(pInput, pInput + length, predicate);        
        if(itr2 != ref_inputVec.end() && (bolt_itr2 != (pInput + length)))
		    EXPECT_EQ( *itr2, *bolt_itr2 );
	    else
            std::cout << "Element not found\n";


        std::vector<src_value_type>::iterator itr3 = std::find_if_not(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        src_value_type * bolt_itr3 = bolt::amp::find_if_not(pInput, pInput + length, predicate);        
        if(itr3 != ref_inputVec.end() && (bolt_itr3 != (pInput + length)))
		    EXPECT_EQ( *itr3, *bolt_itr3 );
	    else
            std::cout << "Element not found\n";


        delete pInput;
    }

};


template <typename T>
class Find_if_Test : public ::testing::Test {
    public: 
    typedef typename std::tuple_element<0, T>::type SrcContainer;
    typedef typename std::tuple_element<1, T>::type Predicate;
    Predicate predicate;
    
    typedef typename SrcContainer::iterator     SrcIterator;

    typedef typename std::iterator_traits<SrcIterator>::value_type      src_value_type;

    void test_find()
    {
        int length = 1<<16;
        std::cout << "Testing Find and Find_if...\n";

        std::vector<src_value_type> ref_inputVec(length);
        test_buffer.init(ref_inputVec);

        SrcContainer inputVec(length); 
        test_buffer.init(inputVec);

		for(int i=0; i<length; i++)
		{
			ref_inputVec[i]= rand() % (i+1);
			inputVec[i] = ref_inputVec[i];
		}


		int n = rand()%length;
		src_value_type val = ref_inputVec[n];

		src_value_type val1 = ref_inputVec[0];
		src_value_type valn = ref_inputVec[length-1];
		src_value_type val_rand = rand();

		std::vector<src_value_type>::iterator itr1 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val);
        //Call Bolt Now   
        SrcIterator bolt_itr1 = bolt::amp::find(inputVec.begin(), inputVec.end(), val);
        EXPECT_EQ( *itr1, *bolt_itr1 );

		std::vector<src_value_type>::iterator itr4 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val1);
        //Call Bolt Now   
        SrcIterator bolt_itr4 = bolt::amp::find(inputVec.begin(), inputVec.end(), val1);
        EXPECT_EQ( *itr4, *bolt_itr4 );


		std::vector<src_value_type>::iterator itr5 = std::find(ref_inputVec.begin(), ref_inputVec.end(), valn);
        //Call Bolt Now   
        SrcIterator bolt_itr5 = bolt::amp::find(inputVec.begin(), inputVec.end(), valn);
        EXPECT_EQ( *itr5, *bolt_itr5 );

		std::vector<src_value_type>::iterator itr6 = std::find(ref_inputVec.begin(), ref_inputVec.end(), val_rand);
        //Call Bolt Now   
        SrcIterator bolt_itr6 = bolt::amp::find(inputVec.begin(), inputVec.end(), val_rand);
		if(itr6 != ref_inputVec.end() && (bolt_itr6 != inputVec.end()))
		    EXPECT_EQ( *itr6, *bolt_itr6);
	    else
            std::cout << "Element not found\n"; 
  

        std::vector<src_value_type>::iterator itr2 = std::find_if(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        //Call Bolt Now   
        SrcIterator bolt_itr2 = bolt::amp::find_if(inputVec.begin(), inputVec.end(), predicate);
        if(itr2 != ref_inputVec.end() && (bolt_itr2 != inputVec.end()))
		    EXPECT_EQ( *itr2, *bolt_itr2 );
	    else
            std::cout << "Element not found\n"; 

		std::vector<src_value_type>::iterator itr3 = std::find_if_not(ref_inputVec.begin(), ref_inputVec.end(), predicate);
        //Call Bolt Now   
        SrcIterator bolt_itr3 = bolt::amp::find_if_not(inputVec.begin(), inputVec.end(), predicate);
        if(itr3 != ref_inputVec.end() && (bolt_itr3 != inputVec.end()))
		    EXPECT_EQ( *itr3, *bolt_itr3 );
	    else
            std::cout << "Element not found\n"; 
    }

    

};


TYPED_TEST_CASE_P(Find_if_Test);
TYPED_TEST_CASE_P(Find_if_raw_test);

TYPED_TEST_P(Find_if_Test, test) 
{

    test_find();

}


TYPED_TEST_P(Find_if_raw_test, raw_pointers) 
{

    test_raw_ptrs();

}


REGISTER_TYPED_TEST_CASE_P(Find_if_Test, test);

typedef std::tuple<std::vector<int>,         is_even<int> >                 INT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<int>,         is_greater_than_5<int> >       INT_VEC_IS_GREATER_T;
typedef std::tuple<std::vector<int>,         is_less_than_zero<int> >       INT_VEC_IS_LESS_T;

typedef std::tuple<std::vector<float>,       is_even<float> >                 FLOAT_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<float>,       is_greater_than_5<float> >       FLOAT_VEC_IS_GREATER_T;
typedef std::tuple<std::vector<float>,       is_less_than_zero<float> >       FLOAT_VEC_IS_LESS_T;

typedef std::tuple<std::vector<UDD>,         is_even_a<UDD> >                 UDD_VEC_IS_EVEN_T;
typedef std::tuple<std::vector<UDD>,         udd_is_greater_than_5<UDD> >     UDD_VEC_IS_GREATER_T;
typedef std::tuple<std::vector<UDD>,         is_less_than_zero_udd<UDD> >     UDD_VEC_IS_LESS_T;

typedef ::testing::Types<
        INT_VEC_IS_EVEN_T,
		INT_VEC_IS_GREATER_T,
		INT_VEC_IS_LESS_T,
        FLOAT_VEC_IS_EVEN_T,
		FLOAT_VEC_IS_GREATER_T,
		FLOAT_VEC_IS_LESS_T,
		UDD_VEC_IS_EVEN_T,
		UDD_VEC_IS_GREATER_T,
		UDD_VEC_IS_LESS_T > STDVectorIsEvenTypes;

typedef std::tuple<bolt::amp::device_vector<int>,         is_even<int> >                 INT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<int>,         is_greater_than_5<int> >       INT_DV_IS_GREATER_T;
typedef std::tuple<bolt::amp::device_vector<int>,         is_less_than_zero<int> >       INT_DV_IS_LESS_T;


typedef std::tuple<bolt::amp::device_vector<float>,       is_even<float> >                 FLOAT_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<float>,       is_greater_than_5<float> >       FLOAT_DV_IS_GREATER_T;
typedef std::tuple<bolt::amp::device_vector<float>,       is_less_than_zero<float> >       FLOAT_DV_IS_LESS_T;


typedef std::tuple<bolt::amp::device_vector<UDD>,         is_even_a<UDD>>                  UDD_DV_IS_EVEN_T;
typedef std::tuple<bolt::amp::device_vector<UDD>,         udd_is_greater_than_5<UDD> >     UDD_DV_IS_GREATER_T;
typedef std::tuple<bolt::amp::device_vector<UDD>,         is_less_than_zero_udd<UDD> >     UDD_DV_IS_LESS_T;

typedef ::testing::Types<
        INT_DV_IS_EVEN_T,
		INT_DV_IS_GREATER_T,
		INT_DV_IS_LESS_T,
        FLOAT_DV_IS_EVEN_T,
		FLOAT_DV_IS_GREATER_T,
		FLOAT_DV_IS_LESS_T,
        UDD_DV_IS_EVEN_T,
        UDD_DV_IS_GREATER_T,
		UDD_DV_IS_LESS_T> DeviceVectorIsEvenTypes;

REGISTER_TYPED_TEST_CASE_P(Find_if_raw_test, raw_pointers);
typedef std::tuple<int,     is_even<int> >                           INT_IS_EVEN_T;
typedef std::tuple<int,     is_greater_than_5<int> >                 INT_IS_GREATER_T;
typedef std::tuple<int,     is_less_than_zero<int> >                 INT_IS_LESS_T;

typedef std::tuple<float,   is_even<float> >                         FLOAT_IS_EVEN_T;
typedef std::tuple<int,     is_greater_than_5<int> >                 FLOAT_IS_GREATER_T;
typedef std::tuple<int,     is_less_than_zero<int> >                 FLOAT_IS_LESS_T;


typedef std::tuple<UDD,     is_even_a<UDD> >                         UDD_IS_EVEN_T;
typedef std::tuple<UDD,     udd_is_greater_than_5<UDD> >             UDD_IS_GREATER_T;
typedef std::tuple<UDD,     is_less_than_zero_udd<UDD> >             UDD_IS_LESS_T;

typedef ::testing::Types<
        INT_IS_EVEN_T,
		INT_IS_GREATER_T,
		INT_IS_LESS_T,
        FLOAT_IS_EVEN_T,
		FLOAT_IS_GREATER_T,
		FLOAT_IS_LESS_T,
        UDD_IS_EVEN_T,
        UDD_IS_GREATER_T,
		UDD_IS_LESS_T> POD_RawPtrIsEvenTypes;



INSTANTIATE_TYPED_TEST_CASE_P(STDVectorTests, Find_if_Test, STDVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(DeviceVectorTests, Find_if_Test, DeviceVectorIsEvenTypes);
INSTANTIATE_TYPED_TEST_CASE_P(RawPtrTests, Find_if_raw_test, POD_RawPtrIsEvenTypes);


//INSTANTIATE_TEST_CASE_P(  Find_if_Test, STDVectorIsEvenTypes, ::testing::Range( 1, 4096, 54 ) ); 

//Add the test team test cases here. 

//Add the EPR test cases here with the EPR number. 


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
