#include "../Misc/StdAfx.h"
#include "../Misc/StrProc.h"
#include "../Misc/StrProcCodePage.h"

#include <cstdio>
#include <string>

int main()
{
	const std::wstring word = L"\u041F\u0440\u0438\u0432\u0435\u0442";
	const std::string cp1251 = "\xCF\xF0\xE8\xE2\xE5\xF2";
	const std::string utf8 = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
	NStr::SetCodePage( 1251 );
	if ( NStr::ToAscii( word ) != cp1251 || NStr::ToUnicode( cp1251 ) != word )
	{
		std::fprintf( stderr, "CP1251 roundtrip failed\n" );
		return 1;
	}
	NStr::SetCodePage( 65001 );
	if ( NStr::ToAscii( word ) != utf8 || NStr::ToUnicode( utf8 ) != word )
	{
		std::fprintf( stderr, "UTF-8 roundtrip failed\n" );
		return 2;
	}
	std::string explicitCp1251;
	NStr::ToAsciiCodePage( &explicitCp1251, word + L"\u2018", 1251 );
	if ( explicitCp1251 != cp1251 + "\x91" || NStr::ToAscii( word ) != utf8 )
	{
		std::fprintf( stderr, "explicit CP1251 conversion changed the global code page\n" );
		return 3;
	}
	std::puts( "cp1251=6 utf8=12" );
	return 0;
}
