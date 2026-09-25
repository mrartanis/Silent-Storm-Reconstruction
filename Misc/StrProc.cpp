#include "StdAfx.h"

#include "StrProc.h"

#include <unordered_map>
#include <stack>
#include <math.h>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#if !defined(_WIN32)
#include <iconv.h>
#include <codecvt>
#include <locale>
#include <stdexcept>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NStr
{
	static std::unordered_map<char, char> brackets;   // map with open bracket <=> close bracket respection
	static char cBracketTypes[8] = "({[\" ";     // all available brackets (open)
	static const int NUM_BRACKET_TYPES = 4;      // number of available brackets
	static int nCodePage =
#if defined(_WIN32)
		CP_ACP;
#else
		65001; // Portable default: UTF-8; SetCodePage(1251) remains supported.
#endif
	//
	void InitStringProcessor();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// проинициализировать внутренние структуры string processor'а
void NStr::InitStringProcessor()
{
	brackets['('] = ')';
	brackets['['] = ']';
	brackets['{'] = '}';
	brackets['\"'] = '\"';
}
// это вспомогательная структура для автоматической инициализации string processor'а
struct SStrProcInit
{
	SStrProcInit() { NStr::InitStringProcessor(); }
};
static SStrProcInit spInit;
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NStr::IsOpenBracket( const char cSymbol )
{
	return brackets.find( cSymbol ) != brackets.end();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// добавить новую пару скобок
void NStr::AddBrackets( const char cOpenBracket, const char cCloseBracket )
{
	brackets[cOpenBracket] = cCloseBracket;
}
// удалить пару скобок
void NStr::RemoveBrackets( const char cOpenBracket, const char cCloseBracket )
{
	brackets.erase( cOpenBracket );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// получить закрывающую скобку по открывающей
const char NStr::GetCloseBracket( const char cOpenBracket )
{
	return brackets[cOpenBracket];
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// разделить строку на массив строк по заданному разделителю
void NStr::SplitString( const std::string &szString, std::vector<std::string> &szVector, const char cSeparator )
{
	int nPos = 0, nLastPos = 0;
	//
	do
	{
		nPos = szString.find( cSeparator, nLastPos );
		// add string
		szVector.push_back( szString.substr( nLastPos, nPos - nLastPos ) );
		nLastPos = nPos + 1;//szString.find_first_not_of( cSeparator, nPos );
		//
	} while( nPos != std::string::npos );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// разделить строку на массив строк по заданному разделителю с учётом скобок одной вложенности
void NStr::SplitStringWithBrackets( const std::string &szString, std::vector<std::string> &szVector, const char cSeparator )
{
	int nPos = 0, nLastPos = 0;
	//
	cBracketTypes[NUM_BRACKET_TYPES] = cSeparator;
	//
	do
	{
		nPos = szString.find_first_of( cBracketTypes, nLastPos );
		if ( nPos != std::string::npos )
		{
			if ( szString[nPos] != cSeparator )      // this is a bracket
			{
				nPos = szString.find( brackets[szString[nPos]], nPos + 1 );
				continue;
			}
		}
		// add string
		szVector.push_back( szString.substr( nLastPos, nPos - nLastPos ) );
		nLastPos = nPos + 1;//szString.find_first_not_of( cSeparator, nPos );
		//
	} while( nPos != std::string::npos );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// разделить строку на массив строк по заданному разделителю с учётом скобок любой вложенности
void NStr::SplitStringWithMultipleBrackets( const std::string &szString, std::vector<std::string> &szVector, const char cSeparator )
{
	std::stack<char> stackBrackets;
	int nLastPos = 0;
  int i;
	//
	for ( i=0; i<szString.size(); ++i )
	{
		char c = szString[i];
		if ( IsOpenBracket(c) )
			stackBrackets.push( brackets[c] );
		else if ( stackBrackets.empty() )
		{
			if ( c == cSeparator )
			{
				szVector.push_back( szString.substr( nLastPos, i - nLastPos ) );
				nLastPos = i + 1; // szString.find_first_not_of( cSeparator, i );
			}
		}
		else if ( c == stackBrackets.top() )
			stackBrackets.pop();
	}
	// last substring
	if ( nLastPos + 1 <= int( i ) )
		szVector.push_back( szString.substr( nLastPos ) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// найти закрывающую скобку без учёта внутренних скобок
int NStr::FindCloseBracket( const std::string &szString, int nPos, const char cOpenBracket )
{
	return szString.find( brackets[cOpenBracket], nPos );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// найти закрывающую скобку с учётом внутренних скобок
int NStr::FindMultipleCloseBracket( const std::string &szString, int nPos, const char cOpenBracket )
{
	std::stack<char> stackBrackets;
	//
	stackBrackets.push( brackets[cOpenBracket] );
	for ( int i=nPos; i<szString.size(); ++i )
	{
		char c = szString[i];
		if ( IsOpenBracket(c) )
			stackBrackets.push( brackets[c] );
		else if ( c == stackBrackets.top() )
		{
			stackBrackets.pop();
			// check bracket stack for empty in the case of the bracket pop
			if ( stackBrackets.empty() )
				return i;
		}
	}
	return std::string::npos;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// отрезать все символы 'cTrim' справа
void NStr::TrimRight( std::string &szString, const char cTrim )
{
	int nPos = szString.find_last_not_of( cTrim );
	if ( nPos == std::string::npos )
	{
		if ( szString.find_first_of( cTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, std::string::npos );
}
void NStr::TrimRight( std::string &szString, const char *pszTrim )
{
	int nPos = szString.find_last_not_of( pszTrim );
	if ( nPos == std::string::npos )
	{
		if ( szString.find_first_of( pszTrim ) == 0 )
			szString.clear();
	}
	else
		szString.erase( nPos + 1, std::string::npos );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// вырезать все символы 'cTrim' из строки
class CSymbolCheckFunctional
{
private:
  const char *pszSymbols;
public:
  explicit CSymbolCheckFunctional( const char *pszNewSymbols ) : pszSymbols( pszNewSymbols ) {  }
  bool operator()( const char cSymbol )
  {
    for ( const char *p = pszSymbols; *p != 0; ++p )
    {
      if ( *p == cSymbol )
        return true;
    }
    return false;
  }
};
void NStr::TrimInside( std::string &szString, const char *pszTrim )
{
  szString.erase( std::remove_if(szString.begin(), szString.end(), CSymbolCheckFunctional(pszTrim)), szString.end() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// преобразовать целое в строку, разделяя каждые три знака (три порядка) специальным разделителем (.)
void NStr::ToDotString( std::wstring *pDst, int nVal, const wchar_t cSeparator )
{
	wchar_t buff[32], buff2[32];
	buff[0] = buff2[0] = 0;
  int nOrder = static_cast<int>( log10( nVal ) );
  int nOrderVal = static_cast<int>( pow( 10, nOrder - (nOrder % 3) ) );
  while ( nOrderVal > 1 )
  {
    int nVal1 = nVal / nOrderVal;
#if defined(_WIN32)
		swprintf( buff2, L"%d%c", nVal1, cSeparator );
#else
		swprintf( buff2, sizeof(buff2) / sizeof(buff2[0]), L"%d%lc", nVal1, cSeparator );
#endif
		wcscat( buff, buff2 );
    nVal -= nVal1 * nOrderVal;
    nOrderVal /= 1000;
  }
#if defined(_WIN32)
	wcscat( buff, _itow(nVal, buff2, 10) );
#else
	swprintf( buff2, sizeof(buff2) / sizeof(buff2[0]), L"%d", nVal );
	wcscat( buff, buff2 );
#endif
  *pDst = buff;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// форматирование строки
const char* __cdecl NStr::Format( const char *pszFormat, ... )
{
  static char buff[2048];
  va_list va;
	// 
  va_start( va, pszFormat );
  vsnprintf( buff, sizeof(buff), pszFormat, va );
  va_end( va );
	//
	return buff;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const wchar_t* __cdecl NStr::Format( const wchar_t *pszFormat, ... )
{
  static wchar_t buff[2048];
  va_list va;
	// 
  va_start( va, pszFormat );
#if defined(_WIN32)
  vswprintf( buff, pszFormat, va );
#else
  vswprintf( buff, sizeof(buff) / sizeof(buff[0]), pszFormat, va );
#endif
  va_end( va );
	//
	return buff;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
const char* NStr::BinToString( const void *pData, int nSize, char *pszBuffer )
{
	for ( int i=0; i<nSize; ++i )
		sprintf( pszBuffer + i*2, "%.2X", int( ((unsigned char*)pData)[i] ) );
	return pszBuffer;
}
void* NStr::StringToBin( const char *pszData, void *pBuffer, int *pnSize )
{
	int nStrSize = strlen( pszData );
	char buffer[8];
	buffer[0] = '0';
	buffer[1] = 'x';
	buffer[4] = '\0';
	int nData;
	for ( int i=0; i<nStrSize/2; ++i )
	{
		buffer[2] = pszData[i*2 + 0];
		buffer[3] = pszData[i*2 + 1];
		sscanf( buffer, "%i", &nData );
		((unsigned char*)pBuffer)[i] = (unsigned char)nData;
	}
	if ( pnSize )
		*pnSize = nStrSize / 2;
	return pBuffer;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NStr::CBracketCharSeparator::operator()( const char cSymbol )
{
	if ( IsOpenBracket(cSymbol) )
		stackBrackets.push( brackets[cSymbol] );
	else if ( stackBrackets.empty() )
		return cSymbol == cSeparator;
	else if ( cSymbol == stackBrackets.top() )
		stackBrackets.pop();
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// <[+/-]>[dec digit]*
bool NStr::IsDecNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits == 0 )
		return false;												// this is not a number at all => zero length digits
	if ( (nNumDigits > 1) && (szString[nFirstDigit] == '0') )
		return false;												// hex number
	for ( i=nFirstDigit; (i < szString.size()) && IsDecDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}
// <[+/-]>[0][oct digit]*
bool NStr::IsOctNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits == 0 )
		return false;
	if ( szString[nFirstDigit] != '0' )
		return false;
	if ( nNumDigits < 2 )
		return false;

	for ( i=nFirstDigit; (i < szString.size()) && IsOctDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}
// <[+/-]>[0x][hex digit]*
bool NStr::IsHexNumber( const std::string &szString )
{
	if ( szString.empty() )
		return false;
	int i, nFirstDigit = IsSign( szString[0] ) ? 1 : 0;
	int nNumDigits = szString.size() - nFirstDigit;
	if ( nNumDigits < 3 )
		return false;
	if ( (szString[nFirstDigit] != '0') || (szString[nFirstDigit + 1] != 'x') )
		return false;
	for ( i=nFirstDigit + 2; (i < szString.size()) && IsHexDigit(szString[i]); ++i ) { ; }
	return ( (i > nFirstDigit) && (i == szString.size()) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NStr::ToInt( const char *pszString )
{
	int nNumber = 0;
	sscanf( pszString, "%i", &nNumber );
	return nNumber;
}
float NStr::ToFloat( const char *pszString )
{
	float fNumber = 0;
	sscanf( pszString, "%f", &fNumber );
	return fNumber;
}
double NStr::ToDouble( const char *pszString )
{
	double fNumber = 0;
	sscanf( pszString, "%lf", &fNumber );
	return fNumber;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NStr::SetCodePage( int _nCodePage )
{
	nCodePage = _nCodePage;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_WIN32)
// Windows-1251 high half, from the platform code-page mapping. Keep this
// game-used legacy encoding available even when an ARM runtime has no gconv
// modules installed (notably the QEMU test sysroot).
static const wchar_t kCp1251High[128] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
	0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
	0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
	0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
	0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
};
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
void NStr::ToAscii( std::string *pRes, const std::wstring &szSrc )
{
#if defined(_WIN32)
	const int N_STACK_BUFF_SIZE = 1024;
	char static_buff[N_STACK_BUFF_SIZE];
	int nBufLeng = szSrc.length() * 2 + 10;
	char *pszBuf;
	if ( nBufLeng < N_STACK_BUFF_SIZE )
		pszBuf = static_buff;
	else
		pszBuf = new char[ nBufLeng ];
	int nRes = WideCharToMultiByte( nCodePage, 0, szSrc.c_str(), szSrc.length(), pszBuf, nBufLeng, 0, 0 );
	pszBuf[nRes] = 0;
	*pRes = pszBuf;
	if ( nBufLeng >= N_STACK_BUFF_SIZE )
		delete[] pszBuf;
#else
	if ( nCodePage == 65001 )
	{
		*pRes = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes( szSrc );
		return;
	}
	if ( nCodePage == 1251 )
	{
		pRes->clear();
		pRes->reserve( szSrc.size() );
		for ( wchar_t c : szSrc )
		{
			if ( c <= 0x7f )
			{
				pRes->push_back( static_cast<char>(c) );
				continue;
			}
			const wchar_t *end = kCp1251High + 128;
			const wchar_t *found = std::find( kCp1251High, end, c );
			if ( found == end )
				throw std::runtime_error( "character is not representable in CP1251" );
			pRes->push_back( static_cast<char>(0x80 + (found - kCp1251High)) );
		}
		return;
	}
	const std::string encoding = nCodePage == 65001 ? "UTF-8" : "CP" + std::to_string(nCodePage);
	iconv_t converter = iconv_open( encoding.c_str(), "WCHAR_T" );
	if ( converter == (iconv_t)-1 )
		throw std::runtime_error( "unsupported game text code page" );
	std::string result( szSrc.size() * 4 + 16, '\0' );
	char *input = reinterpret_cast<char*>( const_cast<wchar_t*>(szSrc.data()) );
	size_t inputLeft = szSrc.size() * sizeof(wchar_t);
	char *output = result.data();
	size_t outputLeft = result.size();
	const size_t status = iconv( converter, &input, &inputLeft, &output, &outputLeft );
	iconv_close( converter );
	if ( status == (size_t)-1 )
		throw std::runtime_error( "invalid game text conversion to code page" );
	result.resize( result.size() - outputLeft );
	*pRes = std::move(result);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NStr::ToUnicode( std::wstring *pRes, const std::string &szSrc )
{
#if defined(_WIN32)
	const int N_STACK_BUFF_SIZE = 1024;
	WCHAR static_buff[N_STACK_BUFF_SIZE];
	int nBufLeng = szSrc.length() + 3;
	WCHAR *pszBuf;
	if ( nBufLeng < N_STACK_BUFF_SIZE )
		pszBuf = static_buff;
	else
		pszBuf = new WCHAR[ nBufLeng ];
	int nRes = MultiByteToWideChar( nCodePage, 0, szSrc.c_str(), szSrc.length(), pszBuf, nBufLeng );
	pszBuf[nRes] = 0;
	*pRes = pszBuf;
	if ( nBufLeng >= N_STACK_BUFF_SIZE )
		delete[] pszBuf;
#else
	if ( nCodePage == 65001 )
	{
		*pRes = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes( szSrc );
		return;
	}
	if ( nCodePage == 1251 )
	{
		pRes->clear();
		pRes->reserve( szSrc.size() );
		for ( unsigned char c : szSrc )
			pRes->push_back( c < 0x80 ? static_cast<wchar_t>(c) : kCp1251High[c - 0x80] );
		return;
	}
	const std::string encoding = nCodePage == 65001 ? "UTF-8" : "CP" + std::to_string(nCodePage);
	iconv_t converter = iconv_open( "WCHAR_T", encoding.c_str() );
	if ( converter == (iconv_t)-1 )
		throw std::runtime_error( "unsupported game text code page" );
	std::string bytes( szSrc.size() * sizeof(wchar_t) + 16, '\0' );
	char *input = const_cast<char*>( szSrc.data() );
	size_t inputLeft = szSrc.size();
	char *output = bytes.data();
	size_t outputLeft = bytes.size();
	const size_t status = iconv( converter, &input, &inputLeft, &output, &outputLeft );
	iconv_close( converter );
	if ( status == (size_t)-1 || (bytes.size() - outputLeft) % sizeof(wchar_t) != 0 )
		throw std::runtime_error( "invalid game text conversion from code page" );
	pRes->resize( (bytes.size() - outputLeft) / sizeof(wchar_t) );
	std::memcpy( pRes->data(), bytes.data(), bytes.size() - outputLeft );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
