#ifndef MINIDUMP_H_
#define MINIDUMP_H_
#pragma once

#if defined( _WIN32 )
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <dbghelp.h>
#endif
#include <string>
#include "unicode.h"

// Function prototype for dynamically discovering dbghelp.dll!MiniDumpWriteDump( )
typedef BOOL (WINAPI* FNMINIDUMPWRITEDUMP)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

namespace bolt
{
    //	This class is not yet thread safe (static variable creation is not thread safe); 
    //	it is the users responsability that the singleton in a thread safe manner.
    class miniDumpSingleton
    {
    public:
        enum minidumpVerbosity { noVerbose, Verbose };

        //  This sets up the exception filter and enables the generation of minidumps
        static miniDumpSingleton& enableMiniDumps( minidumpVerbosity verbosity = noVerbose )
        {
            #if defined( _WIN32 )
                static miniDumpSingleton singleton( verbosity );
                return singleton;
            #endif
        }

        static LONG WINAPI ExceptionFilter( PEXCEPTION_POINTERS pExceptionInfo );

    private:
        HMODULE hDbgHelp;
        LPTOP_LEVEL_EXCEPTION_FILTER topFilterFunc;
        minidumpVerbosity   m_Verbosity;

        miniDumpSingleton( minidumpVerbosity verbosity );
        miniDumpSingleton( const miniDumpSingleton& );
        miniDumpSingleton& operator=( const miniDumpSingleton& );
        ~miniDumpSingleton( );

        static FNMINIDUMPWRITEDUMP fnMiniDumpWriteDump;
        static bolt::tstring exePath;
        static bolt::tstring exeName;
    };
}

#endif /* CLAMDBLAS_H_ */
