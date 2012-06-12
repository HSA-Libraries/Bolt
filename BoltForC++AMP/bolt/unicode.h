#pragma once
#if !defined( BOLT_UNICODE_H )
#define BOLT_UNICODE_H

#include <string>
#include <iostream>

#if defined( _UNICODE )
	typedef std::wstring		tstring;
	typedef std::wstringstream	tstringstream;
	typedef std::wifstream		tifstream;
	typedef std::wofstream		tofstream;
	typedef std::wfstream		tfstream;
	static std::wostream&	tout	= std::wcout;
	static std::wostream&	terr	= std::wcerr;
#else
	typedef std::string tstring;
	typedef std::stringstream tstringstream;
	typedef std::ifstream		tifstream;
	typedef std::ofstream		tofstream;
	typedef std::fstream		tfstream;
	static std::ostream&	tout	= std::cout;
	static std::ostream&	terr	= std::cerr;
#endif 

#endif