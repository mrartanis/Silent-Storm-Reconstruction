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
#include "../Misc/Win32Helper.h"
#if !defined(S2_FULL_GAME)
static void OutputDebugString(const char *message) { std::fputs(message, stderr); }
#endif
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
static string hdDirectory;
static std::atomic<bool> hdTexturesEnabled{true};
static std::atomic<unsigned> textureResourceRevision{1};
static void WaitAllPendingLoad();
bool HDTexturesEnabled() { return hdTexturesEnabled.load(); }
unsigned GetTextureResourceRevision() { return textureResourceRevision.load(); }
vector<string> GetNetworkResourceDirectories()
{
	vector<string> result;
	for ( const string &dir : szDirs )
		if ( dir != hdDirectory ) result.push_back(dir);
	return result;
}
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
	hdDirectory.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Cache per layer rather than collapsing packages from all layers into one list.
typedef unordered_map<string, CPtr<IFilesPackage> > CPackHash;
static CPackHash packages;
static unordered_map<string, vector<string>> hdTexturePackages;
static NWin32Helper::CCriticalSection packageWork;
void SetHDTexturesEnabled( bool enabled )
{
    if ( HDTexturesEnabled() == enabled ) return;
    WaitAllPendingLoad();
    NWin32Helper::CCriticalSectionLock lock(packageWork);
    hdTexturesEnabled = enabled;
    ++textureResourceRevision;
    for(CResourceTracker* tracker:GetTrackers()) tracker->Clear();
}
static bool IsResourcePath( const string &path, bool directory )
{
#if defined(_WIN32)
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        bool(attributes & FILE_ATTRIBUTE_DIRECTORY) == directory;
#else
    std::error_code error;
    return (directory ? std::filesystem::is_directory(path, error) :
        std::filesystem::is_regular_file(path, error)) && !error;
#endif
}
void AddBaseResourceDirs()
{
    AddResourceDir( "./res" );
    string resolved;
    if ( S2FileIO::ResolveGameResourcePath("./res-hd", &resolved) )
    {
        if ( IsResourcePath(resolved, true) )
        {
            AddResourceDir( resolved.c_str() );
            hdDirectory = szDirs.back();
        }
    }
}
static IFilesPackage* GetLayerPackage( const string &dir, const char *name )
{
    const string path = dir + name + ".res";
    CPackHash::iterator i = packages.find(path);
    if ( i != packages.end() ) return i->second;
    IFilesPackage *pack = strcmp(name, "LRTextures") == 0 ?
        OpenCachedFilesPackage(path.c_str()) : OpenFilesPackage(path.c_str());
    packages[path] = pack;
    return pack;
}
static const vector<string>& GetHDTexturePackages(const string& dir)
{
    auto found=hdTexturePackages.find(dir);
    if(found!=hdTexturePackages.end()) return found->second;
    vector<string> names;
    auto addName=[&](const string& name) {
        if(name.compare(0,9,"Textures-")!=0 || name.size()==9)return;
        for(size_t n=9;n<name.size();++n)if(name[n]<'0' || name[n]>'9')return;
        names.push_back(name);
    };
#if defined(_WIN32)
    WIN32_FIND_DATAA data;
    HANDLE search=FindFirstFileA((dir+"Textures-*.res").c_str(),&data);
    if(search!=INVALID_HANDLE_VALUE)
    {
        do {
            if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)continue;
            const string file=data.cFileName;
            if(file.size()>4 && file.compare(file.size()-4,4,".res")==0)
                addName(file.substr(0,file.size()-4));
        } while(FindNextFileA(search,&data));
        FindClose(search);
    }
#else
    std::error_code error;
    for(std::filesystem::directory_iterator it(dir,error),end;!error && it!=end;it.increment(error))
    {
        const string name=it->path().stem().string();
        const string extension=it->path().extension().string();
        if(extension==".res")addName(name);
    }
#endif
    std::sort(names.begin(),names.end());
    return hdTexturePackages.emplace(dir,std::move(names)).first->second;
}
struct SResolvedResource
{
    string path;
    IFilesPackage *package = 0;
    bool found = false;
};
// A higher layer always wins, whether its asset is loose or packed. Within a
// layer loose patch files still override that layer's archive. Both loading
// paths and existence checks share this resolver (caller holds packageWork).
static SResolvedResource ResolveResource( const char *name, int id )
{
    SResolvedResource result;
    const string suffix = string(name) + "/" + std::to_string(id);
    for ( int k = int(szDirs.size()) - 1; k >= 0; --k )
    {
        if ( szDirs[k] == hdDirectory && !HDTexturesEnabled() ) continue;
        if ( szDirs[k] == hdDirectory && strcmp(name, "Textures") != 0 && strcmp(name, "LRTextures") != 0 )
            continue;
        string resolved;
        if ( S2FileIO::ResolveGameResourcePath(szDirs[k] + suffix, &resolved) )
        {
            if ( IsResourcePath(resolved, false) )
            {
                result.path = resolved; result.found = true; return result;
            }
        }
        IFilesPackage *pack = GetLayerPackage(szDirs[k], name);
        if ( pack && DoesFileExist(pack, id) )
        {
            result.package = pack; result.found = true; return result;
        }
        if(szDirs[k]==hdDirectory && strcmp(name,"Textures")==0)
            for(const string& shard:GetHDTexturePackages(szDirs[k]))
            {
                IFilesPackage* extra=GetLayerPackage(szDirs[k],shard.c_str());
                if(extra && DoesFileExist(extra,id))
                {
                    result.package=extra;result.found=true;return result;
                }
            }
    }
    result.path = (szDirs.empty() ? string() : szDirs.front()) + suffix;
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void WaitAllPendingLoad();
void CloseAllResources()
{
	WaitAllPendingLoad();
	NWin32Helper::CCriticalSectionLock l( packageWork );
	packages.clear();
	hdTexturePackages.clear();
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
CFileResource::CFileResource( const char *name, FILE_ID id )
{
    NWin32Helper::CCriticalSectionLock lock(packageWork);
    f.OpenRead( ResolveResource(name, id).path.c_str() );
}
static void TypeReq( const char *pszResName, int nID )
{
	char szBuf[1024];
	sprintf( szBuf, "%s %x\n", pszResName, nID );
	OutputDebugString( szBuf );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CResourceFileOpener::CResourceFileOpener( const char *name, int id )
{
    NWin32Helper::CCriticalSectionLock lock(packageWork);
    const SResolvedResource resource = ResolveResource(name, id);
    if ( resource.package ) pf = new CPackageResource(resource.package, id);
    else pf = new CFileResource(resource.path);
}
CResourceFileOpener::CResourceFileOpener( const char *name, const SPartKey &key )
    : CResourceFileOpener(name, GetID(key)) {}
bool CResourceFileOpener::DoesExist( const char *name, int id )
{
    NWin32Helper::CCriticalSectionLock lock(packageWork);
    return ResolveResource(name, id).found;
}
bool CResourceFileOpener::DoesExist( const char *name, const SPartKey &key )
{
    return DoesExist(name, GetID(key));
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
        const SResolvedResource resource = ResolveResource(pszResName, nID);
        if ( resource.package )
        {
            CPackageStream f(resource.package, nID);
            f.ReadTo(data, f.GetSize());
        }
        else if ( resource.found )
        {
            CFileStream f;
            f.OpenRead(resource.path.c_str());
            f.ReadTo(data, f.GetSize());
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
	if ( hLoaderThread ) return;
	DWORD dwThread;
	hLoaderThread = CreateThread( 0, 102400, LoaderThread, 0, 0, &dwThread );
#else
	if ( !loaderThread.joinable() )
		loaderThread = std::thread(LoaderThread);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void StopResourceLoadingThread()
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
			CloseHandle( hLoaderThread );
			hLoaderThread = 0;
#else
			loaderThread.join();
#endif
			NWin32Helper::CCriticalSectionLock l( reqQueue );
			requests.clear();
			holdRequests.clear();
			bIsFileReading.store(false);
		}
}
struct SKillLoaderThread
{
	~SKillLoaderThread() { StopResourceLoadingThread(); }
} killLoaderThread;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
