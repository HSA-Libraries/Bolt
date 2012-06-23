#pragma once
#if !defined( BOLT_UNICODE_H )
#define BOLT_UNICODE_H

#include <string>
#include <iostream>
#include <sstream>

//	These macros help linux cope with the conventions of windows tchar.h file
#if defined( _WIN32 )
	#include <tchar.h>
#else
	#if defined( __GNUC__ )
		typedef char TCHAR;
		typedef char _TCHAR;
		#define _tmain main

		#if defined( UNICODE )
			#define _T(x)	L ## x
		#else
			#define _T(x)	x
		#endif 
	#endif
#endif

namespace bolt
{
#if defined( _UNICODE )
	typedef std::wstring		tstring;
	typedef std::wstringstream	tstringstream;
	typedef std::wifstream		tifstream;
	typedef std::wofstream		tofstream;
	typedef std::wfstream		tfstream;
	typedef std::wostream		tstream;
	static std::wostream&	tout	= std::wcout;
	static std::wostream&	terr	= std::wcerr;
	static std::wostream&	tlog	= std::wclog;
#else
	typedef std::string tstring;
	typedef std::stringstream tstringstream;
	typedef std::ifstream		tifstream;
	typedef std::ofstream		tofstream;
	typedef std::fstream		tfstream;
	typedef std::ostream		tstream;
	static std::ostream&	tout	= std::cout;
	static std::ostream&	terr	= std::cerr;
	static std::ostream&	tlog	= std::clog;
#endif 
}

#endif