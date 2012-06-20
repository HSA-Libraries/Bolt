#include "stdafx.h"

#include <bolt/AMP/functional.h>
#include <bolt/AMP/scan.h>
#include <bolt/unicode.h>
// #include <bolt/sequentialIterator.h>

int _tmain( int argc, _TCHAR* argv[] )
{
	size_t length = 0;
	size_t iDevice = 0;

	try
	{
		// Declare the supported options.
		po::options_description desc( "AMP Scan command line options" );
		desc.add_options()
			( "help,h",			"produces this help message" )
			( "version,v",		"Print queryable version information from the Bolt AMP library" )
			( "ampInfo,i",		"Print queryable information of the AMP runtime" )
			( "length,l",		po::value< size_t >( &length )->default_value( 4096 ), "Specify the length of scan array" )
			( "gpu,g",			po::value< size_t >( &length )->default_value( 0 ), "Choose which AMP device" )
			;

		po::variables_map vm;
		po::store( po::parse_command_line( argc, argv, desc ), vm );
		po::notify( vm );

		if( vm.count( "version" ) )
		{
			//	Todo:  Query Bolt for its version information
			size_t libMajor, libMinor, libPatch;
			libMajor = 0;
			libMinor = 0;
			libPatch = 1;

			const int indent = countOf( "Bolt version: " );
			tout << std::left << std::setw( indent ) << _T( "Bolt version: " )
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
		terr << _T( "Bolt AMP error reported:" ) << std::endl << e.what() << std::endl;
		return 1;
	}

	std::vector< int > input( length );
	std::vector< int > output( length );

	bolt::inclusive_scan( input.begin( ), input.end( ), output.begin( ) );

	return 0;
}