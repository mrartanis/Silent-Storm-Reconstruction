#include "StdAfx.h"
#include "iMain.h"
#include "..\Misc\StrProc.h"
#include "..\MiscDll\LogStream.h"
#include "..\MiscDll\Commands.h"
#include "..\FileIO\Streams.h"
#include "Interface.h"     // NUI umbrella -- GetDBString (IsValidCustomName's reserved-name lookups)
#include "iSaveManager.h"
#include "..\FileIO\PortableUserPaths.h"
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <cstdint>
#include <vector>
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NMainLoop
{
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace {
wstring WideEnvironmentValue( const wchar_t *name )
{
	const DWORD length = GetEnvironmentVariableW( name, 0, 0 );
	if ( !length ) return wstring();
	vector<wchar_t> value( length );
	if ( GetEnvironmentVariableW( name, &value[0], length ) >= length ) return wstring();
	return wstring( &value[0] );
}

bool IsReparsePointW( const wstring &path )
{
	const DWORD attr = GetFileAttributesW( path.c_str() );
	return attr != INVALID_FILE_ATTRIBUTES && ( attr & FILE_ATTRIBUTE_REPARSE_POINT ) != 0;
}

bool IsDirectoryW( const wstring &path )
{
	const DWORD attr = GetFileAttributesW( path.c_str() );
	return attr != INVALID_FILE_ATTRIBUTES && ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0 &&
		( attr & FILE_ATTRIBUTE_REPARSE_POINT ) == 0;
}

void CreateDirW( const wstring &path )
{
	if ( path.size() < 3 ) return;
	wstring directory = path;
	while ( directory.size() > 3 && directory.back() == L'\\' ) directory.pop_back();
	for ( size_t pos = 3; pos <= directory.size(); ++pos )
		if ( pos == directory.size() || directory[pos] == L'\\' )
			CreateDirectoryW( directory.substr( 0, pos ).c_str(), 0 );
}

bool LegacyCopyTreeW( const wstring &source, const wstring &target, int depth )
{
	if ( depth > 3 || !IsDirectoryW( source ) ) return false;
	CreateDirW( target );
	if ( !IsDirectoryW( target ) ) return false;
	WIN32_FIND_DATAW entry;
	HANDLE handle = FindFirstFileW( ( source + L"*" ).c_str(), &entry );
	if ( handle == INVALID_HANDLE_VALUE ) return GetLastError() == ERROR_FILE_NOT_FOUND;
	bool ok = true;
	do
	{
		const wstring name( entry.cFileName );
		if ( name == L"." || name == L".." ) continue;
		if ( !S2FileIO::IsSafeSaveComponent( name ) || ( entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) )
		{
			ok = false;
			continue;
		}
		const wstring from = source + name;
		const wstring to = target + name;
		if ( entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( !LegacyCopyTreeW( from + L"\\", to + L"\\", depth + 1 ) ) ok = false;
		}
		else if ( !CopyFileW( from.c_str(), to.c_str(), TRUE ) && GetLastError() != ERROR_FILE_EXISTS )
			ok = false;
	} while ( FindNextFileW( handle, &entry ) );
	FindClose( handle );
	return ok;
}

// The resource cwd remains untouched. Old saves are copied once, never moved or
// overwritten; a marker prevents deleted slots from returning on later runs.
const wstring &SaveRootW()
{
	static const wstring root = []() -> wstring {
		S2FileIO::WideUserPathEnvironment env;
		env.overrideRoot = WideEnvironmentValue( L"S2_USER_DATA_DIR" );
		env.localAppData = WideEnvironmentValue( L"LOCALAPPDATA" );
		wstring base;
		string error;
		if ( !S2FileIO::ResolveUserDataRoot( S2FileIO::HostPlatform::Windows, env, &base, &error ) )
		{
			OutputDebugStringA( ( "Save path error: " + error + "\n" ).c_str() );
			return wstring();
		}
		const wstring target = base + L"\\save\\";
		CreateDirW( target );
		if ( !IsDirectoryW( target ) ) return wstring();
		const wstring marker = base + L"\\.legacy-save-imported";
		if ( GetFileAttributesW( marker.c_str() ) == INVALID_FILE_ATTRIBUTES )
		{
			const bool imported = GetFileAttributesW( L"save\\" ) == INVALID_FILE_ATTRIBUTES ||
				LegacyCopyTreeW( L"save\\", target, 0 );
			if ( imported )
			{
				HANDLE file = CreateFileW( marker.c_str(), GENERIC_WRITE, 0, 0,
					CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
				if ( file != INVALID_HANDLE_VALUE ) CloseHandle( file );
			}
			else OutputDebugStringA( "Legacy save import incomplete; will retry next launch\n" );
		}
		return target;
	}();
	return root;
}

wstring ProfileDirW( const string &profile )
{
	const wstring name = NStr::ToUnicode( profile );
	if ( !S2FileIO::IsSafeSaveComponent( name ) || SaveRootW().empty() ) return wstring();
	const wstring path = SaveRootW() + name + L"\\";
	return IsReparsePointW( path ) ? wstring() : path;
}

wstring SlotDirW( const string &profile, const string &slot )
{
	const wstring parent = ProfileDirW( profile );
	const wstring name = NStr::ToUnicode( slot );
	if ( parent.empty() || !S2FileIO::IsSafeSaveComponent( name ) ) return wstring();
	const wstring path = parent + name + L"\\";
	return IsReparsePointW( path ) ? wstring() : path;
}

void RemoveDirW( const wstring &path )
{
	if ( !IsDirectoryW( path ) ) return;
	WIN32_FIND_DATAW entry;
	HANDLE handle = FindFirstFileW( ( path + L"*" ).c_str(), &entry );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		do
		{
			const wstring name( entry.cFileName );
			if ( name == L"." || name == L".." ) continue;
			if ( !S2FileIO::IsSafeSaveComponent( name ) || ( entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) )
				continue;
			const wstring child = path + name;
			if ( entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) RemoveDirW( child + L"\\" );
			else DeleteFileW( child.c_str() );
		} while ( FindNextFileW( handle, &entry ) );
		FindClose( handle );
	}
	wstring directory = path;
	if ( !directory.empty() && directory.back() == L'\\' ) directory.pop_back();
	RemoveDirectoryW( directory.c_str() );
}

void CopyFilesW( const wstring &source, const wstring &target )
{
	WIN32_FIND_DATAW entry;
	HANDLE handle = FindFirstFileW( ( source + L"*" ).c_str(), &entry );
	if ( handle == INVALID_HANDLE_VALUE ) return;
	do
	{
		const wstring name( entry.cFileName );
		if ( !S2FileIO::IsSafeSaveComponent( name ) ||
			( entry.dwFileAttributes & ( FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY ) ) )
			continue;
		CopyFileW( ( source + name ).c_str(), ( target + name ).c_str(), FALSE );
	} while ( FindNextFileW( handle, &entry ) );
	FindClose( handle );
}
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
CSaveManager* GetSaveManager()
{
	static CSaveManager sSaveManager;
	return &sSaveManager;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CSaveManager::CSaveManager(): 
	nActiveSlotID( 0 )
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::CreateProfile( const string &szProfile ) const
{
	const wstring path = ProfileDirW( szProfile );
	if ( !path.empty() ) CreateDirW( path );
	return;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::DeleteProfile( const string &szProfile ) const
{
	const wstring path = ProfileDirW( szProfile );
	if ( !path.empty() ) RemoveDirW( path );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetProfilesList( list<string> *pList ) const
{
	if ( SaveRootW().empty() ) return;
	WIN32_FIND_DATAW entry;
	HANDLE handle = FindFirstFileW( ( SaveRootW() + L"*" ).c_str(), &entry );
	if ( handle == INVALID_HANDLE_VALUE ) return;
	do
	{
		const wstring name( entry.cFileName );
		if ( !( entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ||
			( entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ||
			!S2FileIO::IsSafeSaveComponent( name ) ) continue;
		const string encoded = NStr::ToAscii( name );
		if ( NStr::ToUnicode( encoded ) == name ) pList->push_back( encoded );
	} while ( FindNextFileW( handle, &entry ) );
	FindClose( handle );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
string CSaveManager::GetActiveProfile() const
{
	return NMainLoop::GetActiveProfile();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::SetActiveProfile( const string &szProfile )
{
	NMainLoop::SetActiveProfile( szProfile );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::SaveSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( SlotDirW( szActiveProfile, S_SLOT_ACTIVE ) );
	wstring szTarget( SlotDirW( szActiveProfile, szName ) );
	if ( szSource.empty() || szTarget.empty() ) return;

	if ( szSource == szTarget )
	{
		CreateDirW( szTarget );
		return;
	}

	RemoveDirW( szTarget );
	////
	CreateDirW( szTarget );
	CreateDirW( szSource );

	CopyFilesW( szSource, szTarget );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::LoadSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( SlotDirW( szActiveProfile, szName ) );
	wstring szTarget( SlotDirW( szActiveProfile, S_SLOT_ACTIVE ) );
	if ( szSource.empty() || szTarget.empty() ) return;

	if ( szSource == szTarget )
	{
		CreateDirW( szTarget );
		return;
	}

	RemoveDirW( szTarget );
	////
	CreateDirW( szSource );
	CreateDirW( szTarget );

	CopyFilesW( szSource, szTarget );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::ClearSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( SlotDirW( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	RemoveDirW( szSource );
	CreateDirW( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::DeleteSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( SlotDirW( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	RemoveDirW( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::PrepareSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( SlotDirW( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	CreateDirW( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetSlotsList( list<string> *pList ) const
{
	const string szActiveProfile = GetActiveProfile();
	wstring szSource( ProfileDirW( szActiveProfile ) );
	if ( szSource.empty() ) return;
	WIN32_FIND_DATAW entry;
	HANDLE handle = FindFirstFileW( ( szSource + L"*" ).c_str(), &entry );
	if ( handle == INVALID_HANDLE_VALUE ) return;
	do
	{
		const wstring name( entry.cFileName );
		if ( !( entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ||
			( entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ||
			!S2FileIO::IsSafeSaveComponent( name ) || name == L"temp" ) continue;
		const string encoded = NStr::ToAscii( name );
		if ( NStr::ToUnicode( encoded ) == name ) pList->push_back( encoded );
	} while ( FindNextFileW( handle, &entry ) );
	FindClose( handle );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetSlotTime( const string &szName, wstring *pTime )
{
	int nDate = 0, nTime = 0;
	NMainLoop::GetSlotTime( szName, pTime, &nDate, &nTime );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Retail v1.2 0x637640: modification time, locale date order, and two sort keys.
void GetSlotTime( const string &szName, wstring *pTime, int *pDateKey, int *pTimeKey )
{
	struct _stat sStat;
	int nRet = _wstat( GetSaveManager()->GetSlotFilePathW( szName, S_SAVE_FILENAME ).c_str(), &sStat );
	if ( nRet == -1 )
		return;

	struct tm *pLocalTime = localtime( &sStat.st_mtime );
	*pDateKey = ( ( pLocalTime->tm_year * 12 + pLocalTime->tm_mon ) * 31 + pLocalTime->tm_mday ) * 24 + pLocalTime->tm_hour;
	// Preserve retail's unusual multiplier (raw 0x6376fb..0x637712).
	*pTimeKey = pLocalTime->tm_min * 3660 + pLocalTime->tm_sec;
	char szDateOrder[4] = { 0 };
	GetLocaleInfoA( LOCALE_USER_DEFAULT, LOCALE_IDATE, szDateOrder, sizeof(szDateOrder) );
	const wchar_t *pFormat = L"%d/%m/%y";
	if ( szDateOrder[0] == '0' ) pFormat = L"%m/%d/%y";
	else if ( szDateOrder[0] == '2' ) pFormat = L"%y/%m/%d";
	wchar_t date[MAX_PATH], time[MAX_PATH];
	wcsftime( date, MAX_PATH, pFormat, pLocalTime );
	wcsftime( time, MAX_PATH, L"%H:%M:%S", pLocalTime );
	*pTime = wstring( date ) + L"<tab>" + time;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
string GetQuickSaveSlot( bool bLoad )
{
	// Retail v1.2 0x637a70: save to the older slot, load the newer one.
	string first = NStr::ToAscii( NUI::GetDBString( 20241 ) );
	string second = NStr::ToAscii( NUI::GetDBString( 20242 ) );
	int firstDate = 0, firstTime = 0, secondDate = 0, secondTime = 0;
	wstring time;
	GetSlotTime( first, &time, &firstDate, &firstTime );
	GetSlotTime( second, &time, &secondDate, &secondTime );
	bool bFirstNewer = firstDate != secondDate ? firstDate > secondDate : firstTime > secondTime;
	return ( bLoad ? bFirstNewer : !bFirstNewer ) ? first : second;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetSlotScreenShot( const string &szName, CArray2D<NGfx::SPixel8888> *pScreenShot )
{
#ifndef _DEBUG
	try
#endif
	{
		CFileStream sFile;
		sFile.OpenRead( GetSlotFilePathW( szName, S_SAVE_FILENAME ).c_str() );

		SSaveFileHeader sHeader;
		sFile.Read( &sHeader, sizeof(SSaveFileHeader) );

		// Retail GetSlotScreenShot @0x232720 accepts the same two wire-compatible magics as
		// CICLoad::Exec. Old autosaves therefore remain visible/selectable in the save menu.
		if ( sHeader.nMagic != N_SAVE_MAGIC_NUMBER && sHeader.nMagic != N_SAVE_MAGIC_NUMBER_V0 )
			throw L"Invalid save file";

		pScreenShot->SetSizes( N_SAVE_SCREENSHOT_X, N_SAVE_SCREENSHOT_Y );
		for ( int nTempY = 0; nTempY < N_SAVE_SCREENSHOT_Y; nTempY++ )
			for ( int nTempX = 0; nTempX < N_SAVE_SCREENSHOT_X; nTempX++ )
				(*pScreenShot)[nTempY][nTempX] = sHeader.sScreenShot[nTempY][nTempX];
	}
#ifndef _DEBUG
	catch(...)
	{
		ASSERT( 0 && "Loading failed!" );
		return;
	}
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
wstring CSaveManager::GetSlotFilePathW( const string &szName, const string &szFileName ) const
{
	const wstring path = SlotDirW( GetActiveProfile(), szName );
	const wstring file = NStr::ToUnicode( szFileName );
	return !path.empty() && S2FileIO::IsSafeSaveComponent( file ) ? path + file : wstring();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateDir( const string &szDir )
{
	int nPos = 0, nLastPos = 0;

	do
	{
		nPos = szDir.find_first_of( '\\', nLastPos );
		CreateDirectory( szDir.substr( 0, nPos ).c_str(), NULL );

		nLastPos = nPos + 1;
	} while( nPos != string::npos );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile free-fns (release iSaveManager.obj).  The profile UI talks to these.  Dir ops delegate to
// the CSaveManager; the ACTIVE profile lives in the NGlobal "game_profile" var (validated against the
// on-disk list). Slot operations use the same getter, without a separate cached profile.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateProfile( const string &szProfile )
{
	GetSaveManager()->CreateProfile( szProfile );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void DeleteProfile( const string &szProfile )
{
	GetSaveManager()->DeleteProfile( szProfile );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetProfilesList( list<string> *pList )
{
	GetSaveManager()->GetProfilesList( pList );
	// CSaveManager::GetProfilesList returns every save\* subdir incl. the "." / ".." dir entries that
	// _findfirst yields -- drop them so they don't show up as bogus profiles.
	for ( list<string>::iterator iProfile = pList->begin(); iProfile != pList->end(); )
	{
		if ( *iProfile == "." || *iProfile == ".." )
			iProfile = pList->erase( iProfile );
		else
			iProfile++;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void MakeDefaultProfile()
{
	list<string> profilesList;
	GetProfilesList( &profilesList );
	if ( profilesList.empty() )
	{
		NGlobal::ResetVar( "game_profile" );
		CreateProfile( NStr::ToAscii( NGlobal::GetVar( "game_profile" ).GetString() ) );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
string GetActiveProfile()
{
	string szActive = NStr::ToAscii( NGlobal::GetVar( "game_profile" ).GetString() );

	list<string> profilesList;
	GetProfilesList( &profilesList );

	bool bFound = false;
	for ( list<string>::const_iterator iProfile = profilesList.begin(); iProfile != profilesList.end(); iProfile++ )
		if ( *iProfile == szActive )
			bFound = true;

	if ( !bFound )
	{
		if ( profilesList.empty() )
			MakeDefaultProfile();
		else
			NGlobal::SetVar( "game_profile", NGlobal::CValue( NStr::ToUnicode( profilesList.front() ) ) );
	}

	return NStr::ToAscii( NGlobal::GetVar( "game_profile" ).GetString() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetActiveProfile( const string &szProfile )
{
	NGlobal::SetVar( "game_profile", NGlobal::CValue( NStr::ToUnicode( szProfile ) ) );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// retail NMainLoop::IsValidCustomName @0x235d00 (iSaveManager.obj): a typed save-slot name is valid
// unless it equals one of the four RESERVED localized slot names, DB strings 20241..20244 (each
// compared as ToAscii(GetDBString(id)) with plain string equality -- disasm 0x635d1c..0x635e4b).
// The empty-name check is the caller's job (CSaveView::SaveSlot tests empty separately).
////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsValidCustomName( const string &szName )
{
	if ( szName == NStr::ToAscii( NUI::GetDBString( 20241 ) ) )
		return false;
	if ( szName == NStr::ToAscii( NUI::GetDBString( 20242 ) ) )
		return false;
	if ( szName == NStr::ToAscii( NUI::GetDBString( 20243 ) ) )
		return false;
	if ( szName == NStr::ToAscii( NUI::GetDBString( 20244 ) ) )
		return false;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
