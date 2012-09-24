#include "stdafx.h"

#include "bolt/unicode.h"
#include "bolt/statisticalTimer.h"
#include "bolt/countof.h"
#include "bolt/cl/scan.h"

const std::streamsize colWidth = 26;

int _tmain( int argc, _TCHAR* argv[] )
{
    cl_uint userPlatform = 0;
    cl_uint userDevice = 0;
    size_t numLoops = 0;
    size_t length = 0;
    bool defaultDevice = true;
    bool print_clInfo = false;

    try
    {
        // Declare the supported options.
        po::options_description desc( "OpenCL Scan command line options" );
        desc.add_options()
            ( "help,h",			"produces this help message" )
            ( "version,v",		"Print queryable version information from the Bolt AMP library" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "platform,p",     po::value< cl_uint >( &userPlatform )->default_value( 0 ),	"Specify the platform under test" )
            ( "device,d",       po::value< cl_uint >( &userDevice )->default_value( 0 ),	"Specify the device under test" )
            ( "length,l",		po::value< size_t >( &length )->default_value( 4096 ), "Specify the length of scan array" )
            ( "profile,p",		po::value< size_t >( &numLoops )->default_value( 1 ), "Time and report Scan speed GB/s (default: profiling off)" )
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

        if( vm.count( "queryOpenCL" ) )
        {
            print_clInfo = true;
        }
    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan Benchmark error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    std::vector< int > input( length, 1 );
    std::vector< int > output( length );

    bolt::statTimer& myTimer = bolt::statTimer::getInstance( );
    myTimer.Reserve( 1, numLoops );

    size_t scanId	= myTimer.getUniqueID( _T( "scan" ), 0 );

    if( defaultDevice )
    {
        for( unsigned i = 0; i < numLoops; ++i )
        {
            myTimer.Start( scanId );
            bolt::cl::inclusive_scan( input.begin( ), input.end( ), output.begin( ) );
            myTimer.Stop( scanId );
        }
    }
    else
    {
        for( unsigned i = 0; i < numLoops; ++i )
        {
            myTimer.Start( scanId );
            bolt::cl::inclusive_scan( input.begin( ), input.end( ), output.begin( ), bolt::cl::plus< int >( ) );
            myTimer.Stop( scanId );
        }
    }

    //	Remove all timings that are outside of 2 stddev (keep 65% of samples); we ignore outliers to get a more consistent result
    size_t pruned = myTimer.pruneOutliers( 1.0 );
    double scanTime = myTimer.getAverageTime( scanId );
    double scanGB = ( output.size( ) * sizeof( int ) ) / (1024.0 * 1024.0 * 1024.0);

    bolt::tout << std::left;
    bolt::tout << std::setw( colWidth ) << _T( "Scan profile: " ) << _T( "[" ) << numLoops-pruned << _T( "] samples" ) << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Size (GB): " ) << scanGB << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Time (s): " ) << scanTime << std::endl;
    bolt::tout << std::setw( colWidth ) << _T( "    Speed (GB/s): " ) << scanGB / scanTime << std::endl;
    bolt::tout << std::endl;

//	bolt::tout << myTimer;

    return 0;
}