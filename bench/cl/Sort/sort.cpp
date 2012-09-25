#include "stdafx.h"

#include "bolt/unicode.h"
#include "bolt/statisticalTimer.h"
#include "bolt/countof.h"
#include "bolt/cl/sort.h"


#include <amp.h>
#include <amp_short_vectors.h>

#define cNTiles 64

const std::streamsize colWidth = 26;
using namespace concurrency;
//This function is defined in sort_amp.cpp
extern void Sort(array<unsigned int> &integers,
          array<unsigned int> &tmpIntegers,
          array<unsigned int> &tmpHistograms);

int main( int argc, char* argv[] )
{
    cl_uint userPlatform = 0;
    cl_uint userDevice = 0;
    size_t numLoops = 0;
    size_t length = 0;
    size_t algo = 1;
    bool print_clInfo = false;

    try
    {
        // Declare the supported options.
        po::options_description desc( "OpenCL Sort command line options" );
        desc.add_options()
            ( "help,h",			"produces this help message" )
            ( "version,v",		"Print queryable version information from the Bolt AMP library" )
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( 0 ),	"Specify the platform under test" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( 0 ),	"Specify the device under test" )
            ( "length,l",		po::value< size_t >( &length )->default_value( 4194304 ), "Specify the length of sort array" )
            ( "profile,i",		po::value< size_t >( &numLoops )->default_value( 1 ), "Time and report Sort speed GB/s (default: profiling off)" )
			( "algo,a",		    po::value< size_t >( &algo )->default_value( 1 ), "Algorithm used [1,2]  1:SORT_BOLT, 2:SORT_AMP_SHOC" )
            ;

        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );

        if( vm.count( "version" ) )
        {
            //	TODO:  Query Bolt for its version information
            cl_uint libMajor, libMinor, libPatch;
            libMajor = 0;
            libMinor = 0;
            libPatch = 1;

            bolt::cl::getVersion( libMajor, libMinor, libPatch );
            const int indent = countOf( "Bolt version: " );
            bolt::tout << std::left << std::setw( indent ) << _T( "Bolt version: " )
                << libMajor << _T( "." )
                << libMinor << _T( "." )
                << libPatch << std::endl;
        }

        if( vm.count( "help" ) )
        {
            //	This needs to be 'cout' as program-options does not support wcout yet
            std::cout << desc << std::endl;
            return 0;
        }

    }
    catch( std::exception& e )
    {
        std::cout << _T( "Sort Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    std::vector< int > input( length );
	std::vector< int > backup( length );
	std::generate(backup.begin(), backup.end(),rand);
    
	//AMP sort initializations
	std::vector<unsigned int> integers(length);
	std::generate(integers.begin(), integers.end(),rand);

	// Copy vector to the device.
    array<unsigned int> dIntegers(length, integers.begin());
	array<unsigned int> dTmpIntegers(length);
	array<unsigned int> dTmpHistograms(cNTiles * 16);

    bolt::statTimer& myTimer = bolt::statTimer::getInstance( );
    myTimer.Reserve( 1, numLoops );

    size_t sortId	= myTimer.getUniqueID( _T( "sort" ), 0 );

    if( algo == 1 )
    {
		std::cout<<"Running CL_SORT\n";
		for( unsigned i = 0; i < numLoops; ++i )
        {
			input = backup;
            myTimer.Start( sortId );
            bolt::cl::sort( input.begin( ), input.end( ));
            myTimer.Stop( sortId );
        }
    }
    else if( algo == 2 )
    {
		std::cout<<"Running AMP_SORT\n";
		// Allocate space for temporary integers and histograms.
		for(int run = 0; run < numLoops; ++ run) 
		{
			// Execute and time the kernel.
			myTimer.Start( sortId );			
			Sort(dIntegers, dTmpIntegers, dTmpHistograms);
			myTimer.Stop( sortId );
		}
    }
	else
	{
		std::cout <<"Invalid SORT algorithm specified\n";
	}

    //	Remove all timings that are outside of 2 stddev (keep 65% of samples); we ignore outliers to get a more consistent result
    size_t pruned = myTimer.pruneOutliers( 1.0 );
    double sortTime = myTimer.getAverageTime( sortId );
    double sortGB = ( input.size( ) * sizeof( int ) ) / (1024.0 * 1024.0 * 1024.0);

    bolt::tout << std::left;
    bolt::tout << std::setw( colWidth ) << _T( "Sort profile: " ) << _T( "[" ) << numLoops-pruned << _T( "] samples" ) << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Size (GB): " ) << sortGB << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Time (s): " ) << sortTime << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Speed (GB/s): " ) << sortGB / sortTime << std::endl;
    bolt::tout << std::endl;

//	bolt::tout << myTimer;

    return 0;
//	bolt::tout << myTimer;
	getchar();
	return 0;
}