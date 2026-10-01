#ifndef __WIN32HELPER_H__
#define __WIN32HELPER_H__
#if !defined(_WIN32)
#include <mutex>
#include <condition_variable>
#include <dlfcn.h>
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NWin32Helper
{
#if !defined(_WIN32)
class CEvent {
  std::mutex mutex;
  std::condition_variable ready;
  bool signaled, manual;
public:
  explicit CEvent(bool initial = false, bool manualReset = true): signaled(initial), manual(manualReset) {}
  bool Set() { std::lock_guard<std::mutex> lock(mutex); signaled = true; ready.notify_all(); return true; }
  bool Pulse() { return Set(); }
  bool Reset() { std::lock_guard<std::mutex> lock(mutex); signaled = false; return true; }
  void Wait() { std::unique_lock<std::mutex> lock(mutex); ready.wait(lock, [this] { return signaled; }); if (!manual) signaled = false; }
  bool IsSet() { std::lock_guard<std::mutex> lock(mutex); return signaled; }
};
class CCriticalSection {
  std::recursive_mutex mutex;
  friend class CCriticalSectionLock;
};
class CCriticalSectionLock {
  std::unique_lock<std::recursive_mutex> lock;
public:
  explicit CCriticalSectionLock(CCriticalSection& section): lock(section.mutex) {}
  void Enter() { lock.lock(); }
  void Leave() { if (lock.owns_lock()) lock.unlock(); }
};
#else
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEvent
{
	HANDLE h;
	CEvent( const CEvent& ) {}
	CEvent& operator=( const CEvent& ) {}
public:
	CEvent( bool bInitState = false, bool bManualReset = true ) { h = CreateEvent(0, bManualReset, bInitState, 0 ); }
	~CEvent() { CloseHandle(h); }
	bool Set() { return SetEvent(h) != 0; }
	bool Pulse() { return SetEvent(h) != 0; }
	bool Reset() { return ResetEvent(h) != 0; }
	void Wait() { WaitForSingleObject(h, INFINITE ); }
	bool IsSet() { return WaitForSingleObject( h, 0 ) == WAIT_OBJECT_0; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCriticalSection
{
	CRITICAL_SECTION sect;
	CCriticalSection( const CCriticalSection & ) {}
	CCriticalSection& operator=( const CCriticalSection &) {}
	//
	void Enter() { EnterCriticalSection( &sect ); }
	void Leave() { LeaveCriticalSection( &sect ); }
public:
	CCriticalSection() { InitializeCriticalSection( &sect ); }
	~CCriticalSection() { DeleteCriticalSection( &sect ); }
	friend class CCriticalSectionLock;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCriticalSectionLock
{
	CCriticalSection &lock;
	bool bInsideCriticalSection;
public:
	CCriticalSectionLock( CCriticalSection &_lock ): lock(_lock) { bInsideCriticalSection = true; lock.Enter(); }
	~CCriticalSectionLock() { if ( bInsideCriticalSection ) lock.Leave(); }

	void Enter() { lock.Enter(); bInsideCriticalSection = true; }
	void Leave() { if ( bInsideCriticalSection ) lock.Leave(); bInsideCriticalSection = false; }
};
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class com_ptr
{
	T *pData;
	void Assign( T *_pData ) { if ( _pData ) { _pData->AddRef(); } pData = _pData; }
	void Free() { if ( pData ) pData->Release(); }
public:
	com_ptr( T *_pData = 0 ) { Assign( _pData ); }
	~com_ptr() { Free(); }
	com_ptr( const com_ptr &a ) { Assign( a.pData ); }
	com_ptr& operator=( const com_ptr &a ) { if ( pData == a.pData ) return *this; Free(); Assign( a.pData ); return *this; }
	com_ptr& operator=( T *pObj ) { if ( pData == pObj ) return *this; Free(); Assign( pObj ); return *this; }
	operator T*() const { return pData; }
	T* operator->() const { return pData; }
	// not fair play
	void Create( T *_pData ) { Free(); pData = _pData; }
	T** GetAddr() { Free(); pData = 0; return &pData; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDLLHandle
{
#if defined(_WIN32)
	HMODULE handle;												// DLL handle
	std::string szName;										// file name
	// disable copying...
	CDLLHandle( const CDLLHandle &dll ) {  }
	CDLLHandle& operator=( const CDLLHandle &dll ) { return *this; }
	CDLLHandle() : handle( 0 ) {  }
public:
	CDLLHandle( const char *pszFileName ) : szName( pszFileName ) { handle = ::LoadLibrary( pszFileName ); }
	CDLLHandle( const std::string &szFileName ) : szName( szFileName ) { handle = ::LoadLibrary( szFileName.c_str() ); }
	~CDLLHandle() { if ( handle ) ::FreeLibrary( handle ); }
	// success loading check
	bool IsLoaded() const { return handle != 0; }
	// proc loading. 
	// NOTE: 2nd parameter are a fake - just for return template argument resolving (because of MSVC6.0 can't do it)
	// one can pass just a '(TProc)0' here
	template <class TProc> 
		TProc GetProcAddress( const char *pszProcName, TProc )
	{
		return IsLoaded() ? (TProc)::GetProcAddress( handle, pszProcName ) : (TProc)0;
	}
	template <class TProc> 
		TProc GetProcAddress( int nProcID, TProc )
	{
		return IsLoaded() ? (TProc)::GetProcAddress( handle, (const char *)nProcID ) : (TProc)0;
	}
	// access & casting
	HMODULE GetHMdule() const { return handle; }
	const std::string& GetModuleName() const { return szName; }
	operator HMODULE() const { return handle; }
	operator const char*() const { return szName.c_str(); }
#else
  void* handle;
  std::string szName;
public:
  explicit CDLLHandle(const char* name): handle(dlopen(name, RTLD_NOW)), szName(name) {}
  explicit CDLLHandle(const std::string& name): CDLLHandle(name.c_str()) {}
  ~CDLLHandle() { if (handle) dlclose(handle); }
  bool IsLoaded() const { return handle != nullptr; }
  template<class T> T GetProcAddress(const char* name, T) { return handle ? reinterpret_cast<T>(dlsym(handle, name)) : nullptr; }
  void* GetHMdule() const { return handle; }
  const std::string& GetModuleName() const { return szName; }
#endif
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
