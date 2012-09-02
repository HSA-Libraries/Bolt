#include "stdafx.h"

#include <bolt/boltVersion.h>
#include <bolt/cl/bolt.h>

int _tmain( int argc, _TCHAR* argv[ ] )
{
    cl_uint libMajor = 0, libMinor = 0, libPatch = 0;
    cl_uint appMajor = 0, appMinor = 0, appPatch = 0;
    
    // These version numbers come directly from the Bolt header files, and represent the version of header that the app is compiled against
    appMajor = BoltVersionMajor;
    appMinor = BoltVersionMinor;
    appPatch = BoltVersionPatch;
    
    // These version numbers come from the Bolt library, and represent the version of headers that the lib is compiled against
    bolt::cl::getVersion( libMajor, libMinor, libPatch );
    
    std::cout << std::setw( 35 ) << std::right << "Application compiled with Bolt: " << "v" << appMajor << "." << appMinor << "." << appPatch << std::endl;
    std::cout << std::setw( 35 ) << std::right << "Bolt library compiled with Bolt: " << "v" << libMajor << "." << libMinor << "." << libPatch << std::endl;
    
    return 0;
}