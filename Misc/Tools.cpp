#include "StdAfx.h"
#include <cstdarg>
#include <cstdio>
////////////////////////////////////////////////////////////////////////////////////////////////////
void __cdecl DebugTrace( const char *pszFormat, ... )
{
	static char buff[2048];
	va_list va;
	// 
	va_start( va, pszFormat );
	vsnprintf( buff, sizeof(buff), pszFormat, va );
	va_end( va );
	//
#if defined(_WIN32)
	OutputDebugString( buff );
#else
	std::fputs( buff, stderr );
#endif
}
