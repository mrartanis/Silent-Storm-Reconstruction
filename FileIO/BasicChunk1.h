#ifndef __BASICCHUNK1_H_
#define __BASICCHUNK1_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Streams.h"
#include "PortableStructureChunks.h"
#include "GeometryWire.h"
#include "../Misc/Basic2.h"
#include "../Misc/BasicFactory.h"
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
////////////////////////////////////////////////////////////////////////////////////////////////////
externA5 CClassFactory<CObjectBase> *pSSClasses;
////////////////////////////////////////////////////////////////////////////////////////////////////
#define REGISTER_SAVELOAD_CLASS( N, name )  \
	BASIC_REGISTER_CLASS( name ) \
	static struct name##Register##N { name##Register##N() {  \
	StartRegisterSaveload(); \
	REGISTER_CLASS( (*pSSClasses), N, name )    \
	} } init##name##N;
#define REGISTER_SAVELOAD_TEMPL_CLASS( N, name, className )  \
	BASIC_REGISTER_CLASS( name ) \
	static struct className##Register##N { className##Register##N() {  \
	StartRegisterSaveload(); \
	REGISTER_TEMPL_CLASS( (*pSSClasses), N, name, className )    \
} } init##className##N;
#define REGISTER_SAVELOAD_CLASS_NM( N, name, nmspace )  \
	BASIC_REGISTER_CLASS( nmspace::name ) \
	static struct name##Register##N { name##Register##N() {  \
	StartRegisterSaveload(); \
	REGISTER_CLASS_NM( (*pSSClasses), N, name, nmspace )    \
	} } init##name##N;
#define START_REGISTER(a) static struct a##Init { a##Init () {
#define FINISH_REGISTER } } init;
void StartRegisterSaveload();
#define ZDATA_(a)
#define ZDATA
#define ZPARENT(a)
#define ZEND
#define ZSKIP
////////////////////////////////////////////////////////////////////////////////////////////////////
// a) chunk structure
// b) ptr/ref storage
// system is able to store ref/ptr only for objectbase ancestors
// final save file structure
// -header section list of object types with pointers
// -object data separated in chunks one chunk per object
// c) can replace CMemoryStream with specialized objects to increase perfomance

// --- save-load load-trace diagnostic (dev harness; off unless the -loadslot path enables it) ---
// When enabled, CStructureSaver::Start tracks the top-level object it is reading and, on any load
// exception, appends "THREW at object #<i> id=0x<id>" to _saveload.log so an unattended harness run
// can name the class that broke the load. SaveLoadDiag() is a no-op when g_bSaveLoadDiag is false.
extern bool g_bSaveLoadDiag;
void SaveLoadDiag( const char *szFmt, ... );
// True only while a CStructureSaver READ is tearing down its temporary object table (Finish). In that
// window a deserialized object graph can still hold a smart-ptr to a sibling that was already freed
// (a dangling ref in the resource/DG layer); releasing through it would touch freed+unmapped memory.
// CObjectBase::ReleaseObj/Ref consult this to skip a release whose target is no longer mapped.
extern bool g_bSaveLoadTeardown;
// [HARNESS] wire audit: while READING a save, aggregate every divergence between what the stream
// carries and what the reachable operator& tables consume, into _wireaudit.log --
//   SIZE    a tag was read with a different byte size than the save carries (silent memcpy today),
//   UNREAD  the save carries a tag the class never consumed (retail field dev doesn't know),
//   MISS    dev asked for a tag (first instance) the save doesn't carry (dev-extra field).
// This mechanically surfaces the Jan03-wire divergence family (swapped/shifted/retyped tags)
// without gameplay repros. Off unless the harness enables it; zero cost in normal play.
extern bool g_bWireAudit;
void RecordRawStructureType( const char *pszType, std::size_t nHostSize, bool bArray, int nTag );

// chunk with index 0 is used for system and should not be used in user code
template<int N> struct SGenericNumberTemplate {};
template<class T> class CArray2D;
typedef char chunk_id;
class CStructureSaver
{
public:
	typedef std::string stdString;
	typedef std::wstring stdWString;
private:
	CDataStream &destStream;

	struct CChunkLevel
	{
		chunk_id idChunk, idLastChunk;
		int nStart, nLength;
		//int nChunkNumber; // ����� ����� �� ������� ��� ���������� - ������������ ��� ������/���������� vector/list
		int nLastPos, nLastNumber;
		
		void ClearCache();
		void Clear();
		CChunkLevel() { Clear(); }
	};
	// objects descriptors
	CMemoryStream obj;
	// objects data
	CMemoryStream data;
	std::list<CChunkLevel> chunks;
	typedef std::list<CChunkLevel>::iterator CChunkLevelIterator;
	typedef std::list<CChunkLevel>::reverse_iterator CChunkLevelReverseIterator;
	bool bIsReading;
	// file format version. Stored as top-level chunk id 4 (value, 4 bytes). Absent in legacy
	// files => v0. The release stamps v1 and packs the payload chunks 0/2/1 with CNetCompressor
	// behind an "A3\0" marker chunk (id 3); the content layout is otherwise identical to v0.
	int nVersion;
	// On disk, object references are 32-bit IDs. Legacy files used the writer's
	// pointer bits as IDs; the reader must treat those bits as opaque values.
	typedef std::unordered_map<std::uint32_t,CPtr<CObjectBase> > CObjectsHash;
	CObjectsHash objects;
	// New writes allocate stable wire IDs instead of truncating host pointers.
	typedef std::unordered_map<CObjectBase*,std::uint32_t> CPObjectsHash;
	CPObjectsHash storedObjects;
	std::uint32_t nextWireID;
	std::list<CObjectBase*> toStore;

	bool ReadShortChunk( CChunkLevel &src, int &nPos, CChunkLevel &res );
	bool WriteShortChunk( CChunkLevel &dst, chunk_id dwID, const unsigned char *pData, int nLength );
	bool GetShortChunk( CChunkLevel &src, chunk_id dwID, CChunkLevel &res, int nNumber );
	int CountShortChunks( CChunkLevel &src, chunk_id dwID );
	//
	bool StartChunk( const chunk_id idChunk, int nChunkNumber );
	void FinishChunk();
	void AlignDataFileSize();
	int CountChunks( const chunk_id idChunk );
	//
	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber );
	template<class T>
	void DataArrayChunk( const chunk_id idChunk, T *p, std::size_t count,
		int nChunkNumber, std::true_type )
	{
		typedef S2FileIO::StructureFieldCodec<T> Codec;
		if ( count > static_cast<std::size_t>((std::numeric_limits<int>::max)()) / Codec::kWireSize )
			throw std::runtime_error( "structure array too large" );
		const int nBytes = static_cast<int>(count * Codec::kWireSize);
		std::vector<std::uint8_t> bytes(static_cast<std::size_t>(nBytes));
		if ( IsReading() )
		{
			DataChunk( idChunk, bytes.data(), nBytes, nChunkNumber );
			for ( std::size_t i = 0; i < count; ++i )
				if ( !Codec::Decode( bytes.data() + i * Codec::kWireSize, Codec::kWireSize, p + i ) )
					throw std::runtime_error( "unsupported structure field array" );
		}
		else
		{
			for ( std::size_t i = 0; i < count; ++i )
				if ( !Codec::Encode( p[i], bytes.data() + i * Codec::kWireSize, Codec::kWireSize ) )
					throw std::runtime_error( "unsupported structure field array" );
			DataChunk( idChunk, bytes.data(), nBytes, nChunkNumber );
		}
	}
	template<class T>
	void DataArrayChunk( const chunk_id idChunk, T *p, std::size_t count,
		int nChunkNumber, std::false_type )
	{
		if ( count > static_cast<std::size_t>((std::numeric_limits<int>::max)()) / sizeof(T) )
			throw std::runtime_error( "structure array too large" );
		if ( g_bWireAudit && IsReading() ) RecordRawStructureType( typeid(T).name(), sizeof(T), true, idChunk );
		DataChunk( idChunk, p, static_cast<int>(count * sizeof(T)), nChunkNumber );
	}
	template<class T>
	void DataScalarChunk( const chunk_id idChunk, T *p, int nChunkNumber, std::true_type )
	{
		typedef S2FileIO::StructureFieldCodec<T> Codec;
		std::uint8_t bytes[Codec::kWireSize] = {};
		if ( IsReading() )
		{
			DataChunk( idChunk, bytes, Codec::kWireSize, nChunkNumber );
			if ( !Codec::Decode( bytes, Codec::kWireSize, p ) )
				throw std::runtime_error( "unsupported structure field" );
		}
		else
		{
			if ( !Codec::Encode( *p, bytes, Codec::kWireSize ) )
				throw std::runtime_error( "unsupported structure field" );
			DataChunk( idChunk, bytes, Codec::kWireSize, nChunkNumber );
		}
	}
	template<class T>
	void DataScalarChunk( const chunk_id idChunk, T *p, int nChunkNumber, std::false_type )
	{
		if ( g_bWireAudit && IsReading() ) RecordRawStructureType( typeid(T).name(), sizeof(T), false, idChunk );
		DataChunk( idChunk, p, sizeof(T), nChunkNumber );
	}
	void RawData( void *pData, int nSize );
	void WriteRawData( const void *pData, int nSize );
	void DataChunkBLOB( CMemoryStream &file );
	void DataChunkString( stdString &data );
	void DataChunkString( stdWString &data );
	// storing/loading pointers to objects
	void StoreObject( CObjectBase *pObject );
	CObjectBase* LoadObject();
	//
	void Start( bool bRead );
	void Finish();

	char __cdecl TestDataPath(...) { return 0; }
	int __cdecl TestDataPath( stdString* ) { return 0; }
	int __cdecl TestDataPath( stdWString* ) { return 0; }
	int __cdecl TestDataPath( CMemoryStream* ) { return 0; }
	template<class T1>
		int __cdecl TestDataPath( CArray2D<T1>* ) { return 0; }
	template<class T1, class T2>
		int __cdecl TestDataPath( std::vector<T1,T2>* ) { return 0; }
	template<class T1, class T2>
		int __cdecl TestDataPath( std::list<T1,T2>* ) { return 0; }
	template<class T1, class T2, class T3, class T4>
		int __cdecl TestDataPath( std::unordered_map<T1,T2,T3,T4>* ) { return 0; }
	//
	template<class T>
		void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, ... )
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			p->T::operator&( *this );
			FinishChunk();
		}
	template<class T>
		void __cdecl CallObjectSerialize( const chunk_id idChunk, int nChunkNumber, T *p, SGenericNumberTemplate<1> *pp )
		{
			DataScalarChunk( idChunk, p, nChunkNumber,
				std::integral_constant<bool, S2FileIO::StructureFieldCodec<T>::kPortable>() );
		}
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, ...) 
		{
			const int N_HAS_SERIALIZE_TEST = sizeof( (*p)&(*this) );
			SGenericNumberTemplate<N_HAS_SERIALIZE_TEST> separator;
			CallObjectSerialize( idChunk, nChunkNumber, p, &separator );
		}
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, stdString *pStr ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DataChunkString( *pStr );
			FinishChunk();
		}
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, stdWString *pStr ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DataChunkString( *pStr );
			FinishChunk();
		}
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CMemoryStream *pStr ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DataChunkBLOB( *pStr );
			FinishChunk();
		}
	template<class T,class T1, class T2>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, std::vector<T1,T2> *pVec ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			if ( sizeof( TestDataPath( &(*pVec)[0] ) ) == 1 && sizeof( (*pVec)[0]&(*this) ) == 1 )
				DoDataVector( *p );
			else
				DoVector( *p );
			FinishChunk();
		}
	// std::vector<bool> is bit-packed in this build (no addressable element storage), so the generic
	// vector path above (&(*pVec)[0]) is ill-formed for it. This overload (more specialized, wins
	// partial ordering) reproduces the release DoDataVector<bool> blob layout EXACTLY -- chunk 1 =
	// int element count, chunk 2 = one raw byte per element (retail sizeof(bool) == 1) -- through a
	// byte staging buffer. Consumer: NRPG::CStore::flagsSet (release tag 4, operator& @0x2b2a90).
	template<class T>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, std::vector<bool> *pVec )
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			std::vector<bool> &data = *pVec;
			if ( data.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) )
				throw std::runtime_error( "structure bool vector too large" );
			int nSize = static_cast<int>(data.size());
			Add( 1, &nSize );
			if ( nSize < 0 )
				throw std::runtime_error( "negative structure bool vector size" );
			std::vector<unsigned char> bytes( static_cast<std::size_t>(nSize) );
			if ( IsReading() )
			{
				if ( nSize > 0 )
					DataChunk( 2, &bytes[0], nSize, 1 );
				data.clear();
				data.resize( nSize );
				for ( int i = 0; i < nSize; ++i )
					data[i] = bytes[i] != 0;
			}
			else
			{
				for ( int i = 0; i < nSize; ++i )
					bytes[i] = data[i] ? 1 : 0;
				if ( nSize > 0 )
					DataChunk( 2, &bytes[0], nSize, 1 );
			}
			FinishChunk();
		}
	template<class T,class T1, class T2, class T3, class T4>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, std::unordered_map<T1,T2,T3,T4> *pHash )
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			DoHashMap( *pHash );
			FinishChunk();
		}
	template<class T,class T1>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, CArray2D<T1> *pArr ) 
		{
			if ( !StartChunk( idChunk, nChunkNumber ) )
				return;
			if ( sizeof( TestDataPath( &(*pArr)[0][0] ) ) == 1 && sizeof( (*pArr)[0][0]&(*this) ) == 1 )
				Do2DArrayData( *pArr );
			else
				Do2DArray( *pArr );
			FinishChunk();
		}
	template<class T,class T1, class T2>
		void __cdecl AddInternal( const chunk_id idChunk, int nChunkNumber, T *p, std::list<T1,T2> *pList ) 
	{
		if ( !StartChunk( idChunk, nChunkNumber ) )
			return;
		list<T1,T2> &data = *pList;
		if ( IsReading() )
		{
			data.clear();
			data.insert( data.begin(), CountChunks( 1 ), T1() );
		}
		int i = 1;
		for ( typename std::list<T1,T2>::iterator k = data.begin(); k != data.end(); ++k, ++i )
			Add( 1, &(*k), i );
		FinishChunk();
	}
	//
	//
	// vector
	template <class T, class T1> void DoVector( std::vector<T, T1> &data )
	{
		int i, nSize;
		if ( IsReading() )
		{
			data.clear();
			data.resize( nSize = CountChunks( 1 ) );
		}
		else
			nSize = data.size();
		for ( i = 0; i < nSize; i++ )
			Add( 1, &data[i], i + 1 );
	}
	template <class T, class T1> void DoDataVector( std::vector<T, T1> &data )
	{
		int nSize = data.size();
		Add( 1, &nSize );
		if ( nSize < 0 ||
			static_cast<std::size_t>(nSize) > static_cast<std::size_t>((std::numeric_limits<int>::max)()) /
				S2FileIO::StructureFieldCodec<T>::kWireSize )
			throw std::runtime_error( "structure vector too large" );
		if ( IsReading() )
		{
			data.clear();
			data.resize( nSize );
		}
		if ( nSize > 0 )
			DataArrayChunk( 2, &data[0], static_cast<std::size_t>(nSize), 1,
				std::integral_constant<bool, S2FileIO::StructureFieldCodec<T>::kPortable>() );
	}
	// unordered_map
	template <class T1,class T2,class T3,class T4> 
		void DoHashMap( std::unordered_map<T1,T2,T3,T4> &data )
	{
		if ( IsReading() )
		{
			data.clear();
			int nSize = CountChunks( 1 ), i;
			std::vector<T1> indices;
			indices.resize( nSize );
			for ( i = 0; i < nSize; ++i )
				Add( 1, &indices[i], i + 1 );
			for ( i = 0; i < nSize; ++i )
				Add( 2, &data[ indices[i] ], i + 1 );
		}
		else
		{
			int i = 1;
			for ( typename std::unordered_map<T1,T2,T3,T4>::iterator pos = data.begin(); pos != data.end(); ++pos, ++i )
			{
				T1 idx = pos->first;
				Add( 1, &idx, i );
				Add( 2, &pos->second, i );
			}
		}
	}
	template<class T> void Do2DArray( CArray2D<T> &a )
	{
		int nXSize = a.GetXSize(), nYSize = a.GetYSize();
		Add( 1, &nXSize );
		Add( 2, &nYSize );
		if ( IsReading() )
			a.SetSizes( nXSize, nYSize );
		for ( int i = 0; i < nXSize * nYSize; i++ )
			Add( 3, &a[i/nXSize][i%nXSize], i + 1 );
	}
	template<class T> void Do2DArrayData( CArray2D<T> &a )
	{
		int nXSize = a.GetXSize(), nYSize = a.GetYSize();
		Add( 1, &nXSize );
		Add( 2, &nYSize );
		if ( nXSize < 0 || nYSize < 0 ||
			(static_cast<std::size_t>(nXSize) * static_cast<std::size_t>(nYSize) >
			 static_cast<std::size_t>((std::numeric_limits<int>::max)()) /
			 S2FileIO::StructureFieldCodec<T>::kWireSize) )
			throw std::runtime_error( "structure 2D array too large" );
		if ( IsReading() )
			a.SetSizes( nXSize, nYSize );
		const std::size_t count = static_cast<std::size_t>(nXSize) * static_cast<std::size_t>(nYSize);
		if ( count > 0 )
			DataArrayChunk( 3, &a[0][0], count, 1,
				std::integral_constant<bool, S2FileIO::StructureFieldCodec<T>::kPortable>() );
	}
public:
	enum EMode
	{
		READ,
		WRITE
	};
	CStructureSaver( CDataStream &res, EMode mode ): destStream(res) 
	{ 
		Start( mode == READ ); 
	}
	~CStructureSaver() { Finish(); }
	bool IsReading() { return bIsReading; }
	// file format version (0 = legacy, 1 = release/shipped). Consulted by version-gated operator&.
	int GetVersion() const { return nVersion; }

	//
	void AddRawData( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber = 1 ) { DataChunk( idChunk, pData, nSize, nChunkNumber ); }
	template<class T>
		void Add( const chunk_id idChunk, T *p, int nChunkNumber = 1 ) { AddInternal( idChunk, nChunkNumber, p, p ); }

	template <class T1, class T2> 
		void DoPtr( CPtrBase<T1,T2> *pData ) 
	{
		if ( IsReading() ) 
			pData->Set( CastToUserObject( LoadObject(), (T1*)0 ) ); 
		else 
			StoreObject( pData->GetBarePtr() );
	}
};
template<class T>
inline char operator&( T&c, CStructureSaver &f ) { return 0; }// f.AddData(); return 0; }
// realisation of forward declared serialisation operator
template< class TUserObj, class TRef>
int CPtrBase<TUserObj,TRef>::operator&( CStructureSaver &ff )
{
	ff.DoPtr( this );
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
