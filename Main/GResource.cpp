#if defined(_WIN32)
#include "StdAfx.h"
#include "../Misc/Win32Helper.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
namespace NWin32Helper {
class CCriticalSection {
	std::recursive_mutex mutex;
	friend class CCriticalSectionLock;
};
class CCriticalSectionLock {
	std::unique_lock<std::recursive_mutex> lock;
public:
	explicit CCriticalSectionLock(CCriticalSection &section): lock(section.mutex) {}
};
class CEvent {
	std::mutex mutex;
	std::condition_variable ready;
	bool signaled = false;
public:
	void Set() { std::lock_guard<std::mutex> lock(mutex); signaled = true; ready.notify_all(); }
	void Reset() { std::lock_guard<std::mutex> lock(mutex); signaled = false; }
	void Wait() { std::unique_lock<std::mutex> lock(mutex); ready.wait(lock, [this] { return signaled; }); }
};
}
static void OutputDebugString(const char *message) { std::fputs(message, stderr); }
#endif
#include "GResource.h"
#include "../FileIO/PortablePackageIndex.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGScene
{
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<CPtr<IPrecache> > precacheUpdateList;
void LoadPrecached()
{
	for ( int k = 0; k < precacheUpdateList.size(); ++k )
	{
		IPrecache *p = precacheUpdateList[k];
		if ( IsValid(p) )
			p->Update();
	}
	precacheUpdateList.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static list<CResourceTracker*>& GetTrackers()
{
	static list<CResourceTracker*> trackers;
	return trackers;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Resource dirs. The release keeps an ORDERED LIST of dirs (file-scope szDirs, vector<string>):
// ".\res" first, then one dir per active mod (CModManager::Activate appends them); every lookup
// scans the list LAST-registered-FIRST so mod content overrides the base game. (This dev source
// had collapsed the list to a single string -- restored to the release shape for mod support.)
static vector<string> szDirs;
// release NGScene::AddResourceDir @0x157b30 -- append the dir, enforcing a trailing '\'
void AddResourceDir( const char *pszName )
{
	string szDir = pszName;
#if !defined(_WIN32)
	std::replace(szDir.begin(), szDir.end(), '\\', '/');
	if ( !szDir.empty() && szDir.back() != '/' )
		szDir += "/";
#else
	if ( !szDir.empty() && szDir[ szDir.length() - 1 ] != '\\' )
		szDir += "\\";
#endif
	szDirs.push_back( szDir );
}
// release NGScene::ClearResourceDirs @0x157950 -- drop every registered resource dir
// (CModManager::Activate teardown; it re-registers ".\res" + the chosen mod dirs afterwards)
void ClearResourceDirs()
{
	szDirs.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef unordered_map<string, vector<CPtr<IFilesPackage> > > CPackHash;
static CPackHash packages;
static NWin32Helper::CCriticalSection packageWork;
// release NGScene::GetPackage @0x157ce0 -- collect (once, then cached) the "<dir><name>.res"
// package from EVERY resource dir carrying one, in szDirs order (base first, mod dirs after).
static vector<CPtr<IFilesPackage> >& GetPackages( const char *pszResName )
{
	CPackHash::iterator i = packages.find( pszResName );
	if ( i != packages.end() )
		return i->second;
	vector<CPtr<IFilesPackage> > &res = packages[pszResName];
	for ( int k = 0; k < szDirs.size(); ++k )
	{
		string szFullPath = szDirs[k] + pszResName + ".res";
		IFilesPackage *pRes;
		if ( strcmp( pszResName, "LRTextures" ) == 0 )
			pRes = OpenCachedFilesPackage( szFullPath.c_str() );
		else
			pRes = OpenFilesPackage( szFullPath.c_str() );
		if ( pRes )
			res.push_back( pRes );
	}
#ifndef _MAPEDIT
	ASSERT( !res.empty() );
#endif
	return res;
}
// release ::DoesFileExist( vector<CPtr<IFilesPackage>>&, int ) @0x3f33d0 (FilesPackage.obj) -- scan
// the name's packages LAST-TO-FIRST and return the one carrying nFileID, so an active mod's .res
// overrides the base package PER FILE (retail CResourceFileOpener goes through exactly this pair).
static IFilesPackage* GetPackage( const char *pszResName, int nFileID )
{
	vector<CPtr<IFilesPackage> > &packs = GetPackages( pszResName );
	for ( int k = (int)packs.size() - 1; k >= 0; --k )
	{
		if ( DoesFileExist( packs[k], nFileID ) )
			return packs[k];
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void WaitAllPendingLoad();
void CloseAllResources()
{
	WaitAllPendingLoad();
	NWin32Helper::CCriticalSectionLock l( packageWork );
	packages.clear();
	list<CResourceTracker*> &t = GetTrackers();
	for ( list<CResourceTracker*>::iterator i = t.begin(); i != t.end(); ++i )
		(*i)->Clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static int GetID( const SPartKey &key )
{
	return key.nID + (key.nPart << 16);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline string GetFileResourceName( const char *pszResName, FILE_ID nFileID )
{
#if !defined(_WIN32)
	const std::string suffix = std::string(pszResName) + "/" + std::to_string(nFileID);
	if ( szDirs.empty() )
		return suffix;
	for ( int i = (int)szDirs.size() - 1; i >= 0; --i )
	{
		std::string resolved;
		if ( S2FileIO::ResolveGameResourcePath(szDirs[i] + suffix, &resolved) )
			return resolved;
	}
	return szDirs.front() + suffix;
#else
	// Prefix a resource dir (e.g. ".\res\") so a loose file resolves as "<dir><ResName>\<id>" -
	// matching the release's GetFileResourceName(dir,name,id). With several dirs registered the
	// release probes them LAST-TO-FIRST for the loose file (NGScene::DoesFileExist @0x157780 --
	// mod dirs override ".\res"); mirror that, falling back to the first (base) dir when no dir
	// carries the file. With the usual single dir this compiles down to the old single sprintf.
	char szBuf[1024];
	if ( szDirs.empty() )
	{
		sprintf( szBuf, "%s\\%d", pszResName, nFileID );
		return szBuf;
	}
	for ( int i = (int)szDirs.size() - 1; i > 0; --i )
	{
		sprintf( szBuf, "%s%s\\%d", szDirs[i].c_str(), pszResName, nFileID );
		HANDLE h = CreateFile( szBuf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
		if ( h != INVALID_HANDLE_VALUE )
		{
			CloseHandle( h );
			return szBuf;
		}
	}
	sprintf( szBuf, "%s%s\\%d", szDirs[0].c_str(), pszResName, nFileID );
	return szBuf;
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Does a LOOSE on-disk file exist for this resource? The release checks this BEFORE the package
// (loose overrides package) - that's how the shipped 1.2 patch's updated loose assets take effect.
inline bool DoesLooseFileExist( const char *pszResName, int nID )
{
#if !defined(_WIN32)
	std::error_code error;
	return std::filesystem::is_regular_file(GetFileResourceName(pszResName, nID), error) && !error;
#else
	HANDLE h = CreateFile( GetFileResourceName( pszResName, nID ).c_str(),
		GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	if ( h == INVALID_HANDLE_VALUE )
		return false;
	CloseHandle( h );
	return true;
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CFileResource
////////////////////////////////////////////////////////////////////////////////////////////////////
CFileResource::CFileResource( const char *pszResName, FILE_ID nFileID )
{
	f.OpenRead( GetFileResourceName( pszResName, nFileID ).c_str() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CResourceFileOpener
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool DoesPackageFileExist( const char *pszResName, int nID )
{
	return GetPackage( pszResName, nID ) != 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool DoesPackageFileExist( const char *pszResName, const SPartKey &key )
{
	return GetPackage( pszResName, GetID( key ) ) != 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void TypeReq( const char *pszResName, int nID )
{
	char szBuf[1024];
	sprintf( szBuf, "%s %x\n", pszResName, nID );
	OutputDebugString( szBuf );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CResourceFileOpener::CResourceFileOpener( const char *pszResName, int nID )
{
	//TypeReq( pszResName, nID );
	NWin32Helper::CCriticalSectionLock l( packageWork );
	// loose-overrides-package (matches release CResourceFileOpener)
	if ( DoesLooseFileExist( pszResName, nID ) )
		pf = new CFileResource( pszResName, nID );
	else if ( DoesPackageFileExist( pszResName, nID ) )
		pf = new CPackageResource( GetPackage( pszResName, nID ), nID );
	else
		pf = new CFileResource( pszResName, nID );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CResourceFileOpener::CResourceFileOpener( const char *pszResName, const SPartKey &key )
{
	NWin32Helper::CCriticalSectionLock l( packageWork );
	const int nID = GetID( key );
	//TypeReq( pszResName, nID );
	// loose-overrides-package (matches release CResourceFileOpener)
	if ( DoesLooseFileExist( pszResName, nID ) )
		pf = new CFileResource( pszResName, nID );
	else if ( DoesPackageFileExist( pszResName, nID ) )
		pf = new CPackageResource( GetPackage( pszResName, nID ), nID );
	else
		pf = new CFileResource( pszResName, nID );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CResourceFileOpener::DoesExist( const char *pszResName, int nID )
{
	NWin32Helper::CCriticalSectionLock l( packageWork );
	if ( DoesPackageFileExist( pszResName, nID ) )
		return true;
#ifdef _MAPEDIT
	HANDLE h = CreateFile( GetFileResourceName( pszResName, nID ).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	CloseHandle( h );
	return h != INVALID_HANDLE_VALUE;
//	CFileStream file;
//	return file.TryOpenRead( GetFileResourceName( pszResName, nID ).c_str() );
#else
	return false;
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CResourceFileOpener::DoesExist( const char *pszResName, const SPartKey &key )
{
	NWin32Helper::CCriticalSectionLock l( packageWork );
	if ( DoesPackageFileExist( pszResName, key ) )
		return true;
#ifdef _MAPEDIT
	string szName = GetFileResourceName( pszResName, GetID( key ) ).c_str();
	HANDLE h = CreateFile( szName.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	CloseHandle( h );
	return h != INVALID_HANDLE_VALUE;
	//CFileStream file;
	//return file.TryOpenRead( GetFileResourceName( pszResName, GetID( key ) ).c_str() );
#else
	return false;
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CResourceTracker
////////////////////////////////////////////////////////////////////////////////////////////////////
CResourceTracker::CResourceTracker( const char *pszResourceName ): szResourceName(pszResourceName)
{
	GetTrackers().push_back( this );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CResourceTracker::~CResourceTracker()
{
	GetTrackers().remove( this );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CResourceTracker::DoesExist( const SPartKey &k )
{
	bool bRes;
	CCheckHash::iterator i = check.find( k );
	if ( i == check.end() )
	{
		bRes = CResourceFileOpener::DoesExist( szResourceName.c_str(), k );
		check[k] = bRes;
	}
	else
		bRes = i->second;
	return bRes;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CFileRequest
////////////////////////////////////////////////////////////////////////////////////////////////////
CFileRequest::CFileRequest( const char *_pszResName, int _nID ) 
	: pszResName(_pszResName), nID(_nID), bIsReady(false)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CFileRequest::CFileRequest( const char *_pszResName, const SPartKey &_key )
	: pszResName(_pszResName), nID( GetID(_key) ), bIsReady(false)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static NWin32Helper::CCriticalSection readResource;
static std::atomic<bool> bIsFileReading(false);
void CFileRequest::Read()
{
	ASSERT(!bIsReady.load());
	if ( bIsReady.load() )
		return;
	NWin32Helper::CCriticalSectionLock l( readResource );
	NWin32Helper::CCriticalSectionLock lp( packageWork );
	bIsFileReading.store(true);
	//OutputDebugString( "request " );
	//TypeReq( pszResName, nID );
	try
	{
		// loose-overrides-package (matches release: loose on-disk file wins over the .res package)
		if ( DoesLooseFileExist( pszResName, nID ) )
		{
			CFileStream f;
			f.OpenRead( GetFileResourceName( pszResName, nID ).c_str() );
			f.ReadTo( data, f.GetSize() );
		}
		else if ( DoesPackageFileExist( pszResName, nID ) )
		{
			CPackageStream f( GetPackage( pszResName, nID ), nID );
			f.ReadTo( data, f.GetSize() );
		}
		data.Seek(0);
	}
	catch (...) 
	{
	}
	bIsFileReading.store(false);
	bIsReady.store(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Resource loading thread
////////////////////////////////////////////////////////////////////////////////////////////////////
static NWin32Helper::CCriticalSection reqQueue, pendingCheck;
static NWin32Helper::CEvent newRequest;
#if defined(_WIN32)
static HANDLE hLoaderThread;
#else
static std::thread loaderThread;
#endif
static list<CPtr<CFileRequest> > holdRequests;
static list<CFileRequest*> requests;
////////////////////////////////////////////////////////////////////////////////////////////////////
static void WaitAllPendingLoad()
{
	for(;;)
	{
		{
			NWin32Helper::CCriticalSectionLock lp( pendingCheck );
			NWin32Helper::CCriticalSectionLock l( reqQueue );
			if ( requests.empty() && !bIsFileReading.load() )
				return;
		}
#if defined(_WIN32)
		Sleep(0);
#else
		std::this_thread::yield();
#endif
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
static DWORD WINAPI LoaderThread( void* )
#else
static void LoaderThread()
#endif
{
	for (;;)
	{
		newRequest.Wait();
		newRequest.Reset();
		// process new requests
		CFileRequest *pRes;
		for(;;)
		{
			NWin32Helper::CCriticalSectionLock lp( pendingCheck );
			{
				NWin32Helper::CCriticalSectionLock l( reqQueue );
				if ( requests.empty() )
					break;
				pRes = requests.front();
				requests.pop_front();
				if ( pRes ) bIsFileReading.store(true);
			}
			if ( pRes == 0 )
#if defined(_WIN32)
				return 0;
#else
				return;
#endif
			if ( !IsValid(pRes) )
			{
				bIsFileReading.store(false);
				continue;
			}
			pRes->Read();
		}
	}
#if defined(_WIN32)
	return 0;
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void AddFileRequest( CFileRequest *pReq )
{
	if ( pReq->IsReady() )
		return;
	//pReq->Read();
	//return;
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	holdRequests.push_front( pReq );
	requests.push_front( pReq );
	newRequest.Set();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool HasFileRequestsInFly()
{
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	return bIsFileReading.load() || !requests.empty();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ReleaseFileRequestHolder()
{
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	if ( requests.empty() )
		holdRequests.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void RunResourceLoadingThread()
{
#if defined(_WIN32)
	DWORD dwThread;
	hLoaderThread = CreateThread( 0, 102400, LoaderThread, 0, 0, &dwThread );
#else
	if ( !loaderThread.joinable() )
		loaderThread = std::thread(LoaderThread);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SKillLoaderThread
{
	~SKillLoaderThread()
	{
#if defined(_WIN32)
		if ( hLoaderThread )
#else
		if ( loaderThread.joinable() )
#endif
		{
			{
				NWin32Helper::CCriticalSectionLock l( reqQueue );
				newRequest.Set();
				requests.push_front( 0 );
			}
#if defined(_WIN32)
			WaitForSingleObject( hLoaderThread, INFINITE );
#else
			loaderThread.join();
#endif
		}
	}
} killLoaderThread;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
