#include "stdafx.h"

#include <bolt/cl/device_vector.h>

int _tmain( int argc, _TCHAR* argv[ ] )
{
    const size_t vecSize = 10;
    bolt::cl::device_vector< int > dV( vecSize );

    bolt::cl::device_vector< int >::iterator myIter = dV.begin( );

    //  Iterator arithmetic supported
    *myIter = 1;
    ++myIter;
    *myIter = 2;
    myIter++;
    *myIter = 3;
    myIter += 1;
    *(myIter + 0) = 4;
    *(myIter + 1) = 5;
    myIter += 1;

    //  Operator [] on the container suported
    dV[ 5 ] = 6;
    dV[ 6 ] = 7;

    //  The .data() method maps internal GPU buffer to host accessible memory, and keeps the memory mapped
    bolt::cl::device_vector< int >::pointer pdV = dV.data( );

    //  These are fast writes to host accessible memory
    pdV[ 7 ] = 8;
    pdV[ 8 ] = 9;
    pdV[ 9 ] = 10;

    //  Unmaps the GPU buffer, updating the contents of GPU memory
    pdV.reset( );

    std::cout << "Device Vector contents: " << std::endl;
    for( size_t i = 0; i < vecSize; ++i )
    {
        std::cout << dV[ i ] << ", ";
    }

    return 0;
}