#include "stdafx.h"

#include <bolt/cl/scan.h>

#include <vector>
#include <numeric>

int _tmain( int argc, _TCHAR* argv[ ] )
{
    bolt::cl::device_vector< int > boltInput( 1024ul, 1 );
    bolt::cl::device_vector< int >::iterator boltEnd = bolt::cl::inclusive_scan( boltInput.begin( ), boltInput.end( ), boltInput.begin( ) );

    std::vector< int > stdInput( 1024ul, 1 );
    std::vector< int >::iterator stdEnd  = bolt::cl::inclusive_scan( stdInput.begin( ), stdInput.end( ), stdInput.begin( ) );
    
	return 0;
}