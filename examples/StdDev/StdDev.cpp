#include "stdafx.h"

#include "bolt/cl/reduce.h"
#include "bolt/cl/transform_reduce.h"

#include <math.h>
#include <algorithm>
#include <iomanip>

BOLT_FUNCTOR( Variance< int >,
    template< typename T >
    struct Variance
    {
        T m_mean;

        Variance( T mean ): m_mean( mean ) {};
        T operator( )( const T& elem )
        {
            return (elem - m_mean) * (elem - m_mean);
        };
    };
);

int _tmain( int argc, _TCHAR* argv[ ] )
{
    const cl_uint vecSize = 1024;
    bolt::cl::device_vector< cl_int > boltInput( vecSize );

    //  Initialize random data in device_vector
    std::generate( boltInput.begin( ), boltInput.end( ), rand );

    //  Calculate standard deviation on the Bolt device
    cl_int boltSum = bolt::cl::reduce( boltInput.begin( ), boltInput.end( ), 0 );
    cl_int boltMean = boltSum / vecSize;

    cl_uint boltVariance  = bolt::cl::transform_reduce( boltInput.begin( ), boltInput.end( ), Variance< int >( boltMean ), 0, bolt::cl::plus< cl_int >( ) );
    cl_double boltStdDev = sqrt( boltVariance / vecSize );

    //  Calculate standard deviation with std algorithms (using device_vector!)
    cl_int stdSum = std::accumulate( boltInput.begin( ), boltInput.end( ), 0 );
    cl_int stdMean = stdSum / vecSize;

    std::transform( boltInput.begin( ), boltInput.end( ), boltInput.begin( ), Variance< int >( stdMean ) );
    cl_uint stdVariance = std::accumulate( boltInput.begin( ), boltInput.end( ), 0 );
    cl_double stdStdDev = sqrt( stdVariance / vecSize );

    std::cout << std::setw( 20 ) << std::right << "Bolt StdDev: " << boltStdDev << std::endl;
    std::cout << std::setw( 20 ) << std::right << "Std StdDev: " << stdStdDev << std::endl;

    return 0;
}