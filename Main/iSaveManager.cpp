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
const char S_SAVE_TEMPLATE[] = "save\\";
namespace {
string EnvironmentValue( const char *name )
{
	const DWORD length = GetEnvironmentVariableA( name, 0, 0 );
	if ( !length ) return string();
	vector<char> value( length );
	if ( GetEnvironmentVariableA( name, &value[0], length ) >= length ) return string();
	return string( &value[0] );
}

bool IsDirectory( const string &path )
{
	const DWORD attr = GetFileAttributesA( path.c_str() );
	return attr != INVALID_FILE_ATTRIBUTES && ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0 &&
		( attr & FILE_ATTRIBUTE_REPARSE_POINT ) == 0;
}

bool IsReparsePoint( const string &path )
{
	const DWORD attr = GetFileAttributesA( path.c_str() );
	return attr != INVALID_FILE_ATTRIBUTES && ( attr & FILE_ATTRIBUTE_REPARSE_POINT ) != 0;
}

bool LegacyCopyTree( const string &source, const string &target, int depth )
{
	if ( depth > 3 || !IsDirectory( source ) ) return false;
	CreateDir( target );
	if ( !IsDirectory( target ) ) return false;
	_finddata_t entry;
	const std::intptr_t handle = _findfirst( ( source + "*.*" ).c_str(), &entry );
	if ( handle == -1 ) return true;
	bool ok = true;
	for ( int found = 0; found != -1; found = _findnext( handle, &entry ) )
	{
		const string name( entry.name );
		if ( name == "." || name == ".." ) continue;
		if ( !S2FileIO::IsSafeSaveComponent( name ) ) { ok = false; continue; }
		const string from = source + name;
		const string to = target + name;
		const DWORD attr = GetFileAttributesA( from.c_str() );
		if ( attr == INVALID_FILE_ATTRIBUTES || ( attr & FILE_ATTRIBUTE_REPARSE_POINT ) ) { ok = false; continue; }
		if ( attr & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( !LegacyCopyTree( from + "\\", to + "\\", depth + 1 ) ) ok = false;
		}
		else if ( !CopyFileA( from.c_str(), to.c_str(), TRUE ) && GetLastError() != ERROR_FILE_EXISTS )
			ok = false;
	}
	_findclose( handle );
	return ok;
}

// The resource cwd remains untouched. Old saves are copied once, never moved or
// overwritten; a marker prevents deleted slots from returning on later runs.
const string &SaveRoot()
{
	static const string root = []() -> string {
		S2FileIO::UserPathEnvironment env;
		env.overrideRoot = EnvironmentValue( "S2_USER_DATA_DIR" );
		env.localAppData = EnvironmentValue( "LOCALAPPDATA" );
		string base, error;
		if ( !S2FileIO::ResolveUserDataRoot( S2FileIO::HostPlatform::Windows, env, &base, &error ) )
		{
			OutputDebugStringA( ( "Save path error: " + error + "\n" ).c_str() );
			return string();
		}
		const string target = base + "\\save\\";
		CreateDir( target );
		if ( !IsDirectory( target ) ) return string();
		const string marker = base + "\\.legacy-save-imported";
		if ( GetFileAttributesA( marker.c_str() ) == INVALID_FILE_ATTRIBUTES )
		{
			const bool imported = GetFileAttributesA( S_SAVE_TEMPLATE ) == INVALID_FILE_ATTRIBUTES ||
				LegacyCopyTree( S_SAVE_TEMPLATE, target, 0 );
			if ( imported )
			{
				HANDLE file = CreateFileA( marker.c_str(), GENERIC_WRITE, 0, 0,
					CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
				if ( file != INVALID_HANDLE_VALUE ) CloseHandle( file );
			}
			else OutputDebugStringA( "Legacy save import incomplete; will retry next launch\n" );
		}
		return target;
	}();
	return root;
}

string ProfileDir( const string &profile )
{
	if ( !S2FileIO::IsSafeSaveComponent( profile ) || SaveRoot().empty() ) return string();
	const string path = SaveRoot() + profile + "\\";
	return IsReparsePoint( path ) ? string() : path;
}

string SlotDir( const string &profile, const string &slot )
{
	const string parent = ProfileDir( profile );
	if ( parent.empty() || !S2FileIO::IsSafeSaveComponent( slot ) ) return string();
	const string path = parent + slot + "\\";
	return IsReparsePoint( path ) ? string() : path;
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
	const string path = ProfileDir( szProfile );
	if ( !path.empty() ) CreateDir( path );
	return;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::DeleteProfile( const string &szProfile ) const
{
	const string path = ProfileDir( szProfile );
	if ( !path.empty() ) RemoveDir( path );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetProfilesList( list<string> *pList ) const
{
	if ( SaveRoot().empty() ) return;
	string szProfilesDir( SaveRoot() + "*.*" );

	_finddata_t sFindData;
	std::intptr_t nHandle = _findfirst( szProfilesDir.c_str(), &sFindData );
	int nRet = nHandle == -1 ? -1 : 0;
	while ( nRet != -1 )
	{
		if ( ( sFindData.attrib & _A_SUBDIR ) && S2FileIO::IsSafeSaveComponent( sFindData.name ) &&
			!IsReparsePoint( SaveRoot() + sFindData.name ) )
			pList->push_back( sFindData.name );

		nRet = _findnext( nHandle, &sFindData );
	}

	if ( nHandle != -1 ) _findclose( nHandle );
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
	string szSource( SlotDir( szActiveProfile, S_SLOT_ACTIVE ) );
	string szTarget( SlotDir( szActiveProfile, szName ) );
	if ( szSource.empty() || szTarget.empty() ) return;

	if ( szSource == szTarget )
	{
		CreateDir( szTarget );
		return;
	}

	RemoveDir( szTarget );
	////
	CreateDir( szTarget );
	CreateDir( szSource );

	CopyFiles( szSource, szTarget, "*.*" );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::LoadSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	string szSource( SlotDir( szActiveProfile, szName ) );
	string szTarget( SlotDir( szActiveProfile, S_SLOT_ACTIVE ) );
	if ( szSource.empty() || szTarget.empty() ) return;

	if ( szSource == szTarget )
	{
		CreateDir( szTarget );
		return;
	}

	RemoveDir( szTarget );
	////
	CreateDir( szSource );
	CreateDir( szTarget );

	CopyFiles( szSource, szTarget, "*.*" );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::ClearSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	string szSource( SlotDir( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	RemoveDir( szSource );
	CreateDir( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::DeleteSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	string szSource( SlotDir( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	RemoveDir( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::PrepareSlot( const string &szName )
{
	const string szActiveProfile = GetActiveProfile();
	string szSource( SlotDir( szActiveProfile, szName ) );
	if ( szSource.empty() ) return;
	CreateDir( szSource );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CSaveManager::GetSlotsList( list<string> *pList ) const
{
	const string szActiveProfile = GetActiveProfile();
	string szSource( ProfileDir( szActiveProfile ) );
	if ( szSource.empty() ) return;
	string szSourceMask( szSource + "*.*" );

	_finddata_t sFindData;
	std::intptr_t nHandle = _findfirst( szSourceMask.c_str(), &sFindData );
	int nRet = nHandle == -1 ? -1 : 0;
	while ( nRet != -1 )
	{
		string szName( sFindData.name );
		if ( ( sFindData.attrib & _A_SUBDIR ) && S2FileIO::IsSafeSaveComponent( szName ) &&
			!IsReparsePoint( szSource + szName ) && ( szName.compare( "temp" ) != 0 ) )
			pList->push_back( sFindData.name );

		nRet = _findnext( nHandle, &sFindData );
	}

	if ( nHandle != -1 ) _findclose( nHandle );
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
	int nRet = _stat( GetSaveManager()->GetSlotFilePath( szName, S_SAVE_FILENAME ).c_str(), &sStat );
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
		sFile.OpenRead( GetSlotFilePath( szName, S_SAVE_FILENAME ).c_str() );

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
string CSaveManager::GetSlotFilePath( const string &szName, const string &szFileName ) const
{
	// Retail resolves game_profile for each operation, including the first one after startup.
	const string szActiveProfile = GetActiveProfile();
	const string path = SlotDir( szActiveProfile, szName );
	return !path.empty() && S2FileIO::IsSafeSaveComponent( szFileName ) ? path + szFileName : string();
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
void RemoveDir( const string &szDir )
{
	string szSourcePath( szDir + "*.*" );

	_finddata_t sFindData;
	std::intptr_t nHandle = _findfirst( szSourcePath.c_str(), &sFindData );
	int nRet = nHandle == -1 ? -1 : 0;
	while ( nRet != -1 )
	{
		string szName( sFindData.name );
		if ( ( szName.compare( "." ) != 0 ) && ( szName.compare( ".." ) != 0 ) )
		{
			const DWORD attr = GetFileAttributesA( ( szDir + szName ).c_str() );
			if ( attr == INVALID_FILE_ATTRIBUTES || ( attr & FILE_ATTRIBUTE_REPARSE_POINT ) )
			{
				csSystem << "Can't remove unsafe save entry " << szName << endl;
				nRet = _findnext( nHandle, &sFindData );
				continue;
			}
			if ( sFindData.attrib & _A_SUBDIR )
				RemoveDir( szDir + sFindData.name + "\\" );
			else
			{
				if ( !DeleteFile( string( szDir + sFindData.name ).c_str() ) )
					csSystem << "Can't delete file " << sFindData.name << endl;
			}
		}

		nRet = _findnext( nHandle, &sFindData );
	}

	if ( nHandle != -1 ) _findclose( nHandle );

	const string directory = !szDir.empty() && szDir[szDir.size() - 1] == '\\' ?
		szDir.substr( 0, szDir.size() - 1 ) : szDir;
	if ( !RemoveDirectory( directory.c_str() ) )
		csSystem << "Can't delete directory " << szDir << endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CopyFiles( const string &szSource, const string &szTarget, const string &szMask )
{
	string szSourcePath( szSource + szMask );

	_finddata_t sFindData;
	std::intptr_t nHandle = _findfirst( szSourcePath.c_str(), &sFindData );
	int nRet = nHandle == -1 ? -1 : 0;
	while ( nRet != -1 )
	{
		string sSourceFile( szSource + sFindData.name );
		string sTargetFile( szTarget + sFindData.name );
		CopyFile( sSourceFile.c_str(), sTargetFile.c_str(), FALSE );

		nRet = _findnext( nHandle, &sFindData );
	}

	if ( nHandle != -1 ) _findclose( nHandle );
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
