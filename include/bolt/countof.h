#pragma once
#if !defined( BOLT_COUNTOF_H )
#define BOLT_COUNTOF_H

//	Creating a portable definition of countof macro
#if defined( _WIN32 )
    #define countOf _countof
#else
    #define countOf( arr ) ( sizeof( arr ) / sizeof( arr[ 0 ] ) )
#endif

#endif