#include "stdafx.h"

#include <bolt/AMP/functional.h>
#include <bolt/AMP/scan.h>
//#include <bolt/AMP/systemQuery.h>
#include <bolt/unicode.h>

void printAccelerator( unsigned int num, const concurrency::accelerator& dev )
{
	const std::streamsize width = 26;

	std::wcout << std::left << std::boolalpha;
	std::wcout << std::setw( width ) << _T( "Device: " ) << num << std::endl;
	std::wcout << std::setw( width ) << _T( "Description: " ) << dev.get_description( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Device Path: " ) << dev.get_device_path( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Version: " ) << dev.get_version( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Dedicated Memory: " ) << std::showpoint << dev.get_dedicated_memory( ) / 1024 << _T( " MB" ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Double support: " ) << dev.get_supports_double_precision( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Limited Double Support: " ) << dev.get_supports_limited_double_precision( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Emulated: " ) << dev.get_is_emulated( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Debug: " ) << dev.get_is_debug( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "Display: " ) << dev.get_has_display( ) << std::endl;

	concurrency::accelerator_view av = dev.get_default_view( );
	std::wcout << std::setw( width ) << _T( "Default accelerator view: " ) << std::endl;
	std::wcout << std::setw( width ) << _T( "    Version: " ) << av.get_version( ) << std::endl;
	std::wcout << std::setw( width ) << _T( "    Debug: " ) << av.get_is_debug( ) << std::endl;

	concurrency::queuing_mode qm = av.get_queuing_mode( );
	bolt::tout << std::setw( width ) << _T( "    Queueing mode: " );
	switch( qm )
	{
		case concurrency::queuing_mode_immediate:
			std::wcout << _T( "immediate" );
			break;
		case concurrency::queuing_mode_automatic:
			std::wcout << _T( "automatic" );
			break;
		default:
			std::wcout << _T( "unknown" );
			break;
	}
	std::wcout << std::endl;


	std::wcout << std::internal << std::noboolalpha << std::endl;
}

int _tmain( int argc, _TCHAR* argv[] )
{
	size_t length = 0;
	size_t iDevice = 0;
	bool choseDevice = false;

	try
	{
		// Declare the supported options.
		po::options_description desc( "AMP Scan command line options" );
		desc.add_options()
			( "help,h",			"produces this help message" )
			( "version,v",		"Print queryable version information from the Bolt AMP library" )
			( "ampInfo,i",		"Print queryable information of the AMP runtime" )
			( "length,l",		po::value< size_t >( &length )->default_value( 4096 ), "Specify the length of scan array" )
			( "device,d",		po::value< size_t >( &iDevice ), "Choose specific AMP device, otherwise system default (AMP choose)" )
			;

		po::variables_map vm;
		po::store( po::parse_command_line( argc, argv, desc ), vm );
		po::notify( vm );

		if( vm.count( "version" ) )
		{
			//	TODO:  Query Bolt for its version information
			size_t libMajor, libMinor, libPatch;
			libMajor = 0;
			libMinor = 0;
			libPatch = 1;

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

		if( vm.count( "ampInfo" ) )
		{
			std::vector< concurrency::accelerator > allDevices = concurrency::accelerator::get_all( );

			//std::for_each( allDevices.begin( ), allDevices.end( ), printAccelerator );
			for( unsigned int i = 0; i < allDevices.size( ); ++i )
				printAccelerator( i, allDevices.at( i ) );

			return 0;
		}

		if( vm.count( "device" ) )
		{
			choseDevice = true;
		}

	}
	catch( std::exception& e )
	{
		bolt::terr << _T( "Bolt AMP error reported:" ) << std::endl << e.what() << std::endl;
		return 1;
	}

//	bolt::control::getDefault( );
	std::vector< int > input( length, 1 );
	std::vector< int > output( length );

	if( choseDevice )
	{
		std::vector< concurrency::accelerator > allDevices = concurrency::accelerator::get_all( );

		bolt::inclusive_scan( allDevices.at( iDevice ).get_default_view( ), input.begin( ), input.end( ), 
					output.begin( ), bolt::plus< int >( ) );
	}
	else
	{
		bolt::inclusive_scan( input.begin( ), input.end( ), output.begin( ) );
	}

	return 0;
}