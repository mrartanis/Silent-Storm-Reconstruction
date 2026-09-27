#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../Misc/Geom.h"
#endif
#include "../ADOImport/BasicDB.h"
#include "../Misc/BasicFactory.h"
#include "../FileIO/BasicChunk1.h"
#include "../FileIO/PortableGameDatabase.h"
#include "../Misc/StrProcCodePage.h"
#include <set>
#include <cstdlib>

#if defined(_WIN32)
#import "C:\Program Files (x86)\Common Files\System\ADO\msado15.dll" no_namespace rename("EOF", "EndOfFile")
#include <ole2.h>
#include <conio.h>

struct SInitBasicDB
{
	SInitBasicDB() { CoInitialize(NULL); }
	~SInitBasicDB() { CoUninitialize(); }
};
static SInitBasicDB init;
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void TESTHR(HRESULT x) {if FAILED(x) _com_issue_error(x);};
////////////////////////////////////////////////////////////////////////////////////////////////////
static _ConnectionPtr pConnection;
static void EstablishConnection( const char *pszSource )
{
	_bstr_t connect(pszSource);
	TESTHR(pConnection.CreateInstance(__uuidof(Connection)));
	//pConnection->Open( connect, "Admin", "", adConnectUnspecified );
	pConnection->CursorLocation = adUseClient;
	pConnection->Open( connect, "sa", "simple", adConnectUnspecified );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void CloseConnection()
{
	pConnection->Close();
	pConnection = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class COLETable
{
	long nIndex, nRecords;
	_variant_t avarRecords, fieldData;
	vector<string> fields;
	vector<int> fieldIndices;
	int nCurrentField;
#ifdef _DEBUG
	vector<string> fieldNames;
#endif
	//
	void ReadField( int nField, VARTYPE fieldType );
	int GetFieldIndex( const char *pszField );
public:
	bool Open( const char *pszTable, int nRecordID = -1 );
	void Close();
	void MoveFirst();
	void MoveNext() { ++nIndex; nCurrentField = 0; }
	bool IsEof() { return nIndex >= nRecords; }
	//
	int GetInt( int nField );
	bool GetBool( int nField );
	float GetFloat( int nField );
	_bstr_t GetString( int nField );
	int GetInt( const char *pszField ) { return GetInt( GetFieldIndex( pszField) ); }
	bool GetBool( const char *pszField ) { return GetBool( GetFieldIndex( pszField) ); }
	float GetFloat( const char *pszField ) { return GetFloat( GetFieldIndex( pszField ) ); }
	_bstr_t GetString( const char *pszField ) { return GetString( GetFieldIndex( pszField ) ); }
	const string& GetFieldName( int n ) { return fields[n]; }
	// v1.2 @0x401900..0x402740: the v1.2 getters report success; this column-presence
	// probe backs that flag. Does NOT touch the GetFieldIndex replay cache (a column's
	// presence is constant per table, so skipping a missing column keeps the per-row
	// cached index sequence consistent).
	bool HasField( const char *pszField ) const { return find( fields.begin(), fields.end(), pszField ) != fields.end(); }
};
#endif // _WIN32: SQL/ADO import is editor-only.
////////////////////////////////////////////////////////////////////////////////////////////////////
// CDBTableDataStorage - the release/Steam runtime DB-load representation.
//
// The shipped game.db (CStructureSaver v1, see CStructureSaver::nVersion) does NOT store typed
// CDBRecord objects the way the dev DataImport does. Instead it stores, per table, one of these:
// a generic COLUMNAR dump of the source SQL table (int/float/wstring column arrays + a field
// schema) wrapped as CObj<CDBTableDataStorage>. The typed records are rebuilt AT LOAD time by
// running each record's existing Import()/ImportField loop against this storage (the same
// machinery the dev uses at DataImport time, but sourced from here instead of the ADO COLETable).
// Layout/serialization recovered from the matched release Game.exe+PDB (CDBTableDataStorage size
// 136, operator& @0x406f70, saveload id 0xa1843130). See reconstruction/parse_storage.py which
// validates this format against the actual Steam game.db (extracts the Fonts table records).
struct SColumnInfo
{
	string szName;
	int eType;            // EColumnType (unused on the read path; kept for byte-exact serialization)
	int operator&( CStructureSaver &f ) { f.Add( 2, &szName ); f.Add( 3, &eType ); return 0; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// Diagnostics for the release columnar database.  Keep this state outside CDBTableDataStorage so
// the reconstructed class retains the 136-byte release layout documented below.  During a table's
// Import() pass every field lookup marks the corresponding column.  Columns left unconsumed are the
// actionable schema gaps requested by reconstruction stage 0.4; malformed row widths and unknown
// table type ids are reported separately by the load loop.
struct SStorageDiagnostics
{
	int nTableID;
	vector<bool> usedInt;
	vector<bool> usedFloat;
	vector<bool> usedString;
	SStorageDiagnostics(): nTableID( 0 ) {}
};
static SStorageDiagnostics *pStorageDiagnostics = 0;

struct SUnresolvedReference
{
	string szField;
	int nSourceTable;
	int nTargetTable;
	int nFirstSourceRecord;
	int nFirstTargetRecord;
	int nCount;
	SUnresolvedReference(): nSourceTable( 0 ), nTargetTable( 0 ), nFirstSourceRecord( 0 ),
		nFirstTargetRecord( 0 ), nCount( 0 ) {}
};
static vector<SUnresolvedReference> unresolvedReferences;

struct SMissingStorageField
{
	string szField;
	string szType;
	int nSourceTable;
	int nFirstSourceRecord;
	int nCount;
	SMissingStorageField(): nSourceTable( 0 ), nFirstSourceRecord( 0 ), nCount( 0 ) {}
};
static vector<SMissingStorageField> missingStorageFields;

static void AddMissingStorageField( const char *pszField, const char *pszType,
	int nSourceTable, int nSourceRecord )
{
	for ( int i = 0; i < (int)missingStorageFields.size(); ++i )
		if ( missingStorageFields[i].nSourceTable == nSourceTable &&
			missingStorageFields[i].szType == pszType && missingStorageFields[i].szField == pszField )
		{
			++missingStorageFields[i].nCount;
			return;
		}
	SMissingStorageField item;
	item.szField = pszField;
	item.szType = pszType;
	item.nSourceTable = nSourceTable;
	item.nFirstSourceRecord = nSourceRecord;
	item.nCount = 1;
	missingStorageFields.push_back( item );
}

static void ReportMissingStorageFields()
{
	for ( int i = 0; i < (int)missingStorageFields.size(); ++i )
	{
		const SMissingStorageField &item = missingStorageFields[i];
		DebugTrace( "DB-SCHEMA MISSING source-table=0x%08X source-record=%d type=%s field=%s count=%d\n",
			item.nSourceTable, item.nFirstSourceRecord, item.szType.c_str(), item.szField.c_str(),
			item.nCount );
	}
}

static void MarkStorageField( const vector<string> &names, vector<bool> *pUsed, const char *pszName )
{
	if ( !pStorageDiagnostics || !pUsed )
		return;
	for ( int i = 0; i < (int)names.size(); ++i )
		if ( names[i] == pszName )
		{
			if ( i < (int)pUsed->size() )
				(*pUsed)[i] = true;
			return;
		}
}

static void AddUnresolvedReference( const char *pszField, int nSourceTable, int nSourceRecord,
	int nTargetTable, int nTargetRecord )
{
	for ( int i = 0; i < (int)unresolvedReferences.size(); ++i )
		if ( unresolvedReferences[i].nSourceTable == nSourceTable &&
			unresolvedReferences[i].nTargetTable == nTargetTable &&
			unresolvedReferences[i].szField == pszField )
		{
			++unresolvedReferences[i].nCount;
			return;
		}
	SUnresolvedReference item;
	item.szField = pszField;
	item.nSourceTable = nSourceTable;
	item.nTargetTable = nTargetTable;
	item.nFirstSourceRecord = nSourceRecord;
	item.nFirstTargetRecord = nTargetRecord;
	item.nCount = 1;
	unresolvedReferences.push_back( item );
}

static void ReportUnresolvedReferences()
{
	for ( int i = 0; i < (int)unresolvedReferences.size(); ++i )
	{
		const SUnresolvedReference &item = unresolvedReferences[i];
		DebugTrace( "DB-SCHEMA UNRESOLVED source-table=0x%08X source-record=%d field=%s "
			"target-table=0x%08X target-record=%d count=%d\n", item.nSourceTable,
			item.nFirstSourceRecord, item.szField.c_str(), item.nTargetTable,
			item.nFirstTargetRecord, item.nCount );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class CDBTableDataStorage: public CObjectBase
{
	OBJECT_BASIC_METHODS( CDBTableDataStorage );
public:
	// serialized columnar data (row-major: records_*[ row ][ colIndex ])
	vector< vector<int> >          records_int;
	vector< vector<float> >        records_float;
	vector< vector<std::wstring> > records_wstring;
	vector< SColumnInfo >          fields;
	vector< string >               intFileds;    // (sic - matches release field names)
	vector< string >               floatFileds;
	vector< string >               stringFileds;
	// transient recordset cursor (rebuilt at load, NOT serialized)
	int nCurrentRecord;

	CDBTableDataStorage(): nCurrentRecord( 0 ) {}

	int operator&( CStructureSaver &f )
	{
		f.Add( 2, &records_int );
		f.Add( 3, &records_float );
		f.Add( 4, &records_wstring );
		f.Add( 5, &fields );
		f.Add( 6, &intFileds );
		f.Add( 7, &floatFileds );
		f.Add( 8, &stringFileds );
		return 0;
	}
	// recordset cursor (read side) - mirrors COLETable's interface
	void MoveFirst() { nCurrentRecord = 0; }
	void MoveNext()  { ++nCurrentRecord; }
	bool IsEof()     { return nCurrentRecord >= (int)records_int.size(); }

	static int FindIndex( const vector<string> &names, const char *psz )
	{
		for ( int i = 0; i < (int)names.size(); ++i )
			if ( names[i] == psz )
				return i;
		return -1;
	}
	int GetInt( const char *psz )
	{
		MarkStorageField( intFileds, pStorageDiagnostics ? &pStorageDiagnostics->usedInt : 0, psz );
		int n = FindIndex( intFileds, psz );
		if ( n < 0 || nCurrentRecord < 0 || nCurrentRecord >= (int)records_int.size() ||
				 n >= (int)records_int[nCurrentRecord].size() )
			return 0;
		return records_int[nCurrentRecord][n];
	}
	bool GetBool( const char *psz ) { return GetInt( psz ) != 0; }
	float GetFloat( const char *psz )
	{
		MarkStorageField( floatFileds, pStorageDiagnostics ? &pStorageDiagnostics->usedFloat : 0, psz );
		int n = FindIndex( floatFileds, psz );
		if ( n < 0 || nCurrentRecord < 0 || nCurrentRecord >= (int)records_float.size() ||
				 n >= (int)records_float[nCurrentRecord].size() )
			return 0;
		return records_float[nCurrentRecord][n];
	}
	std::wstring GetWString( const char *psz )
	{
		MarkStorageField( stringFileds, pStorageDiagnostics ? &pStorageDiagnostics->usedString : 0, psz );
		int n = FindIndex( stringFileds, psz );
		if ( n < 0 || nCurrentRecord < 0 || nCurrentRecord >= (int)records_wstring.size() ||
				 n >= (int)records_wstring[nCurrentRecord].size() )
			return std::wstring();
		return records_wstring[nCurrentRecord][n];
	}
	// v1.2 @0x7ef6d0-family: column-presence probes backing ImportField's success flag
	// (bool columns are stored in the int arrays, so they probe intFileds)
	bool HasIntField( const char *psz )    { return FindIndex( intFileds, psz ) >= 0; }
	bool HasFloatField( const char *psz )  { return FindIndex( floatFileds, psz ) >= 0; }
	bool HasStringField( const char *psz ) { return FindIndex( stringFileds, psz ) >= 0; }
	int GetCurrentRecordID() const
	{
		const int n = FindIndex( intFileds, "ID" );
		if ( n < 0 || nCurrentRecord < 0 || nCurrentRecord >= (int)records_int.size() ||
			n >= (int)records_int[nCurrentRecord].size() )
			return 0;
		return records_int[nCurrentRecord][n];
	}
};
REGISTER_SAVELOAD_CLASS( 0xa1843130, CDBTableDataStorage )
// when set, the Import path reads from this columnar storage instead of the ADO COLETable
static CDBTableDataStorage *pStorageSource = 0;

static void ReportStorageShape( int nTableID, CDBTableDataStorage *pStorage )
{
	if ( !pStorage )
		return;
	const char *parityFlag = std::getenv( "S2_DB_PARITY" );
	if ( parityFlag && parityFlag[0] == '1' && parityFlag[1] == '\0' )
	{
		// Diagnostic only: compare values decoded through the game's CStructureSaver
		// against the independent portable game.db loader, table by table.
		S2FileIO::GameDatabaseTable snapshot;
		snapshot.tableId = nTableID;
		for ( const SColumnInfo &column : pStorage->fields )
			snapshot.columns.push_back( { column.szName, column.eType } );
		snapshot.intNames = pStorage->intFileds;
		snapshot.floatNames = pStorage->floatFileds;
		snapshot.stringNames = pStorage->stringFileds;
		snapshot.intRows = pStorage->records_int;
		snapshot.floatRows = pStorage->records_float;
		snapshot.stringRows = pStorage->records_wstring;
		std::uint64_t hash = 0;
		if ( S2FileIO::HashGameDatabaseTable( snapshot, &hash ) )
			DebugTrace( "DB-PARITY type=0x%08X hash=%016llx\n", nTableID,
				static_cast<unsigned long long>(hash) );
		else
			DebugTrace( "DB-PARITY ERROR type=0x%08X\n", nTableID );
	}
	const int nRows = (int)pStorage->records_int.size(); // every runtime table has the integer ID column
	DebugTrace( "DB-SCHEMA TABLE type=0x%08X rows=%d int-fields=%d float-fields=%d string-fields=%d\n",
		nTableID, nRows, (int)pStorage->intFileds.size(), (int)pStorage->floatFileds.size(),
		(int)pStorage->stringFileds.size() );
	if ( !pStorage->floatFileds.empty() && (int)pStorage->records_float.size() != nRows )
		DebugTrace( "DB-SCHEMA ERROR table=0x%08X rows int=%d float=%d\n", nTableID,
			nRows, (int)pStorage->records_float.size() );
	if ( !pStorage->stringFileds.empty() && (int)pStorage->records_wstring.size() != nRows )
		DebugTrace( "DB-SCHEMA ERROR table=0x%08X rows int=%d string=%d\n", nTableID,
			nRows, (int)pStorage->records_wstring.size() );
	for ( int nRow = 0; nRow < nRows; ++nRow )
	{
		if ( (int)pStorage->records_int[nRow].size() != (int)pStorage->intFileds.size() )
		{
			DebugTrace( "DB-SCHEMA ERROR table=0x%08X row=%d int-width=%d fields=%d\n", nTableID,
				nRow, (int)pStorage->records_int[nRow].size(), (int)pStorage->intFileds.size() );
			break;
		}
	}
	for ( int nRow = 0; nRow < (int)pStorage->records_float.size(); ++nRow )
	{
		if ( (int)pStorage->records_float[nRow].size() != (int)pStorage->floatFileds.size() )
		{
			DebugTrace( "DB-SCHEMA ERROR table=0x%08X row=%d float-width=%d fields=%d\n", nTableID,
				nRow, (int)pStorage->records_float[nRow].size(), (int)pStorage->floatFileds.size() );
			break;
		}
	}
	for ( int nRow = 0; nRow < (int)pStorage->records_wstring.size(); ++nRow )
	{
		if ( (int)pStorage->records_wstring[nRow].size() != (int)pStorage->stringFileds.size() )
		{
			DebugTrace( "DB-SCHEMA ERROR table=0x%08X row=%d string-width=%d fields=%d\n", nTableID,
				nRow, (int)pStorage->records_wstring[nRow].size(), (int)pStorage->stringFileds.size() );
			break;
		}
	}
}

static void BeginStorageDiagnostics( int nTableID, CDBTableDataStorage *pStorage,
	SStorageDiagnostics *pDiagnostics )
{
	pDiagnostics->nTableID = nTableID;
	pDiagnostics->usedInt.assign( pStorage->intFileds.size(), false );
	pDiagnostics->usedFloat.assign( pStorage->floatFileds.size(), false );
	pDiagnostics->usedString.assign( pStorage->stringFileds.size(), false );
	pStorageDiagnostics = pDiagnostics;
	// ID is consumed in the preceding PreCreate pass, outside the instrumented Import pass.
	MarkStorageField( pStorage->intFileds, &pDiagnostics->usedInt, "ID" );
}

static void ReportUnusedStorageFields( CDBTableDataStorage *pStorage,
	const SStorageDiagnostics &diagnostics )
{
	for ( int i = 0; i < (int)diagnostics.usedInt.size(); ++i )
		if ( !diagnostics.usedInt[i] )
			DebugTrace( "DB-SCHEMA UNCONSUMED table=0x%08X type=int field=%s\n",
				diagnostics.nTableID, pStorage->intFileds[i].c_str() );
	for ( int i = 0; i < (int)diagnostics.usedFloat.size(); ++i )
		if ( !diagnostics.usedFloat[i] )
			DebugTrace( "DB-SCHEMA UNCONSUMED table=0x%08X type=float field=%s\n",
				diagnostics.nTableID, pStorage->floatFileds[i].c_str() );
	for ( int i = 0; i < (int)diagnostics.usedString.size(); ++i )
		if ( !diagnostics.usedString[i] )
			DebugTrace( "DB-SCHEMA UNCONSUMED table=0x%08X type=string field=%s\n",
				diagnostics.nTableID, pStorage->stringFileds[i].c_str() );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
static void PrintProviderError(_ConnectionPtr pConnection)
{
	ErrorPtr  pErr  = NULL;

	if( (pConnection->Errors->Count) > 0)
	{
		long nCount = pConnection->Errors->Count;
		// Collection ranges from 0 to nCount -1.
		for(long i = 0; i < nCount; i++)
		{
			char szBuf[1024];
			pErr = pConnection->Errors->GetItem(i);
			sprintf( szBuf, "\t Error number: %x\t%s", pErr->Number, (LPCSTR) pErr->Description );
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void PrintComError(_com_error &e)
{
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	const char *pszSource = (LPCSTR) bstrSource;
	const char *pszDescr = (LPCSTR) bstrDescription;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void COLETable::MoveFirst()
{ 
	nIndex = 0; 
	nCurrentField = 0; 
	fieldIndices.clear(); 
#ifdef _DEBUG
	fieldNames.clear();
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int COLETable::GetFieldIndex( const char *pszField )
{
	if ( nIndex > 0 )
	{
#ifdef _DEBUG
		if ( nCurrentField >= fieldIndices.size() || fieldNames[nCurrentField] != pszField )
		{
			ASSERT(0);
			return 0;
		}
#endif
		return fieldIndices[nCurrentField++];
	}
	vector<string>::iterator r = find( fields.begin(), fields.end(), pszField );
	int nPos = 0;
	if ( r == fields.end() )
	{
		ASSERT( 0 );
	}
	else
	{
		nPos = r - fields.begin();
	}
	fieldIndices.push_back( nPos );
#ifdef _DEBUG
	fieldNames.push_back( pszField );
#endif
	return nPos;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void COLETable::ReadField( int nField, VARTYPE fieldType )
{
	long rgIndices[2];
	rgIndices[0] = nField;
	rgIndices[1] = nIndex;
	//fieldData.vt = fieldType;
	HRESULT hr= SafeArrayGetElement(avarRecords.parray, rgIndices, &fieldData );
	ASSERT( SUCCEEDED(hr) );
	if ( fieldData.vt == VT_NULL )
	{
		switch ( fieldType )
		{
			case VT_I4: fieldData = (long)0; break;
			case VT_R4: fieldData = (float)0; break;
			case VT_BOOL: fieldData = (bool)false; break;
			case VT_BSTR: fieldData = ""; break;
		}
	}
	else
		ASSERT( fieldType == fieldData.vt );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool COLETable::Open( const char *pszTable, int nRecordID )
{
	HRESULT hr = S_OK;
	_RecordsetPtr pTable = 0;
	FieldsPtr pFields = 0;
	FieldPtr pField;
	bool bRet = false;

	try 
	{
		// Open recordset with names and hire dates from Employees table.
		TESTHR(pTable.CreateInstance(__uuidof(Recordset)));

		// Use client cursor to improve performance
		pTable->CursorType = adOpenForwardOnly;//adOpenStatic;
		pTable->CursorLocation = adUseClient;
		pTable->CacheSize = 100;
		_bstr_t source("SELECT * FROM ");
		source += pszTable;
		if ( nRecordID != -1 )
		{
			source += " WHERE ID=";
			char buf[32];
			source += itoa( nRecordID, buf, 10 );
		}

		pTable->Open( source, _variant_t((IDispatch*)pConnection), adOpenForwardOnly, adLockReadOnly, adCmdText);

		if ( pTable->GetRecordCount() > 0 )
		{
			bRet = true;
			avarRecords = pTable->GetRows(-1);
			HRESULT hr = SafeArrayGetUBound(avarRecords.parray, 2, &nRecords );
			nRecords++;
			pFields = pTable->GetFields();
			int nFields = pFields->GetCount();
			fields.resize( nFields );
			for ( int i = 0; i < nFields; ++i )
			{
				_variant_t n( (long) i );
				FieldPtr pField = pFields->GetItem( n );
				fields[i] = (const char*) pField->GetName();
			}
		}
		else
		{
			SAFEARRAYBOUND b[2];
			Zero( b );
			avarRecords = SafeArrayCreate( VT_VARIANT, 2, b );
			nRecords = 0;
			fields.resize(0);
		}
		pTable->Close();
		MoveFirst();
	}
	catch(_com_error &e)
	{
		// Notify the user of errors if any.
		// Pass a connection pointer accessed from the Recordset.
		_variant_t vtConnect = pTable->GetActiveConnection();

		// GetActiveConnection returns connect string if connection
		// is not open, else returns Connection object.
		switch(vtConnect.vt)
		{
			case VT_BSTR:
				PrintComError(e);
				break;
			case VT_DISPATCH:
				PrintProviderError(vtConnect);
				break;
			default:
				printf("Errors occured.");
				break;
		}
	}
	return bRet;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void COLETable::Close()
{
	avarRecords.Clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int COLETable::GetInt( int nField )
{
	ReadField( nField, VT_I4 );
	return (long)fieldData;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool COLETable::GetBool( int nField )
{
	ReadField( nField, VT_BOOL );
	return (bool)fieldData;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
float COLETable::GetFloat( int nField )
{
	ReadField( nField, VT_R4 );
	return (float)fieldData;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
_bstr_t COLETable::GetString( int nField )
{
	ReadField( nField, VT_BSTR );
	if ( fieldData.vt != VT_BSTR || !fieldData.bstrVal )
		return "";
	return fieldData;
}
#endif // _WIN32: SQL/ADO source import is not part of the game runtime.
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDatabase
{
	CClassFactory<CDBRecord>& GetRecordTypes()
	{
		static CClassFactory<CDBRecord> recordTypes;
		return recordTypes;
	}
	typedef unordered_map< int, CDBTableBase > CTablesHash;
	CTablesHash& GetTables() 
	{
		static CTablesHash tables; // maps record type to table
		return tables;
	}
	//////////////////////////////////////////////////////////////////////////////////////
	struct STableDescr
	{
		int nTableID;
		string szTable;
	};
	//////////////////////////////////////////////////////////////////////////////////////
	struct SRelation
	{
		string szTable;               // name of ADO table
		CDBTableBase *pLeft, *pRight;  // appropriate tables (resolved from nTableLeft/nTableRight on v1 load)
		int nTableLeft, nTableRight;   // table ids of pLeft/pRight (release v1 serialized form)
		struct SElement
		{
			int nLeft, nRight;
		};
		vector< SElement > data;
		SRelation(): pLeft(0), pRight(0), nTableLeft(0), nTableRight(0) {}
		// release v1 serialization: szTable(2), nTableLeft(3), nTableRight(4), data(5)
		int operator&( CStructureSaver &f )
		{
			f.Add( 2, &szTable );
			f.Add( 3, &nTableLeft );
			f.Add( 4, &nTableRight );
			f.Add( 5, &data );
			return 0;
		}
	};
	list<STableDescr>& GetTableDescrs()
	{
		static list<STableDescr> tableDescrs;
		return tableDescrs;
	}
	list<SRelation>& GetRelations()
	{
		static list< SRelation > relations;
		return relations;
	}
	static string szDataSource;
	bool bIsDatabaseLoading = false;
#if defined(_WIN32)
	COLETable table;
#endif

	static CDBTableBase* GetTableByName( const char *pszTable );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::SetSource( const char *pszSource )
{
	szDataSource = pszSource;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::AddTable( int nTableID, const char *pszTableName, 
												RecordCreateFunc newf )
{
	CTablesHash &tables = GetTables();
	list<STableDescr> &tableDescrs = GetTableDescrs();
	CTablesHash::iterator i = tables.find( nTableID );
	if ( i != tables.end() )
	{
		ASSERT(0); // already registered
		return;
	}
	ASSERT( pszTableName[ strlen( pszTableName ) - 1 ] == 's' );
	GetRecordTypes().RegisterTypeSafe( nTableID, newf );
	STableDescr &t = *tableDescrs.insert( tableDescrs.end(), STableDescr());
	t.nTableID = nTableID;
	t.szTable = pszTableName;
	tables[nTableID];
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CDBTableBase* NDatabase::GetTable( int nTableID )
{
	CTablesHash &tables = GetTables();
	CTablesHash::iterator i = tables.find( nTableID );
	if ( i != tables.end() )
		return &i->second;
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static int GetTableID( CDBTableBase *pTable )
{
	NDatabase::CTablesHash &tables = NDatabase::GetTables();
	for ( NDatabase::CTablesHash::iterator i = tables.begin(); i != tables.end(); ++i )
		if ( &i->second == pTable )
			return i->first;
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static CDBTableBase* NDatabase::GetTableByName( const char *pszTable )
{
	list<STableDescr> &tableDescrs = GetTableDescrs();
	list< STableDescr >::iterator i;
	for ( i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
	{
		if ( i->szTable == pszTable )
			return GetTable( i->nTableID );
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::AddRelation( const char *pszTableName )
{
	list< SRelation > &relations = GetRelations();
	SRelation &rel = *relations.insert( relations.end(), SRelation());
	rel.szTable = pszTableName;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// ClearDatabaseTables (release BasicDB.obj @0x3570, a release-only free function) -- drop every live
// DB table/record + relation so CModManager::Activate can re-import a fresh game.db (+ mod overlays)
// into a clean database. After clearing, one EMPTY table entry is re-created per registered
// descriptor (the same `tables[id]` idiom AddTable uses), so GetTable() keeps resolving during the
// reload. The relations list is refilled by the next Serialize(READ) (v1 chunk 2).
// NOTE: the release also clears NDatabase::GetStorageTables() here; this dev Serialize keeps the
// columnar storage hash as a Serialize-local and drops it after import, so there is no dev-side
// storage-tables global to clear.
void NDatabase::ClearDatabaseTables()
{
	CTablesHash &tables = GetTables();
	tables.clear();
	GetRelations().clear();
	list<STableDescr> &tableDescrs = GetTableDescrs();
	for ( list<STableDescr>::iterator i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
		tables[ i->nTableID ];
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static string RelationField2TableName( const string &src )
{
	ASSERT( src.substr( src.length() - 2, 2 ) == "ID" );
	return src.substr( 0, src.length() - 2 ) + "s";
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
void NDatabase::Import()
{
	bIsDatabaseLoading = true;
	list<STableDescr> &tableDescrs = GetTableDescrs();
	list< SRelation > &relations = GetRelations();
	EstablishConnection( szDataSource.c_str() );
	list<STableDescr>::iterator i;
	list<SRelation>::iterator k;
	//
	// 1st phase - create all records & load relations
	for ( i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
	{
		STableDescr &t = *i;
		CDBTableBase *pTable = GetTable( t.nTableID );
		ASSERT( pTable );

		table.Open( t.szTable.c_str() );

		pTable->PreCreate( t.nTableID );
	}
	for ( k = relations.begin(); k != relations.end(); ++k )
	{
		SRelation &t = *k;

		table.Open( t.szTable.c_str() );
		// determine left/right sides
		t.pLeft = GetTableByName( RelationField2TableName( table.GetFieldName( 0 ) ).c_str() );
		t.pRight = GetTableByName( RelationField2TableName( table.GetFieldName( 1 ) ).c_str() );
		if ( t.pLeft == 0 || t.pRight == 0 )
		{
			ASSERT(0); // field names in relation does not match table names
			k = relations.erase( k );
		}
    t.data.clear();
		// load data into SRelation
		for ( ; !table.IsEof(); table.MoveNext() )
		{
			SRelation::SElement &res = *t.data.insert( t.data.end(), SRelation::SElement());
			//
			res.nLeft = table.GetInt( 0 );
			res.nRight = table.GetInt( 1 );
		}
	}

	// 2nd phase - read data for each record
	for ( i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
	{
		STableDescr &t = *i;
		CDBTableBase *pTable = GetTable( t.nTableID );
		ASSERT( pTable );

		table.Open( t.szTable.c_str() );

		pTable->Import();
	}
	table.Close();
	CloseConnection();
	bIsDatabaseLoading = false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::Refresh( int nTableID )
{
	list<STableDescr> &tableDescrs = GetTableDescrs();
	EstablishConnection( szDataSource.c_str() );
	// 1st phase - refresh records
	list<STableDescr>::iterator i;
	for ( i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
		if ( nTableID == i->nTableID )
			break;
	if ( i == tableDescrs.end() )
	{
		ASSERT(0);
		return;
	}
	STableDescr &t = *i;
	CDBTableBase *pTable = GetTable( t.nTableID );
	ASSERT( pTable );
	table.Open( t.szTable.c_str() );
	pTable->Refresh( t.nTableID );
	// 2nd phase - read data for each record
	table.Close();
	CloseConnection();
	EstablishConnection( szDataSource.c_str() );
	table.Open( t.szTable.c_str() );
 	pTable->Import();
	table.Close();
	CloseConnection();
}
#endif // _WIN32: legacy SQL import/refresh is not used when loading game.db.
////////////////////////////////////////////////////////////////////////////////////////////////////
// v1.2 @0x7ef6d0-family: the scalar ImportFields now report success -- a missing
// column returns false and leaves *pData untouched (v1.1 zero-filled on the storage
// path / asserted + read column 0 on the ADO path), so a runtime mod db lacking a
// column preserves the record's previous field value.
bool NDatabase::ImportField( const char *pszFieldName, int *pData )
{
	if ( pStorageSource )
	{
		if ( !pStorageSource->HasIntField( pszFieldName ) )
		{
			AddMissingStorageField( pszFieldName, "int", pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
				pStorageSource->GetCurrentRecordID() );
			return false;
		}
		*pData = pStorageSource->GetInt( pszFieldName );
		return true;
	}
#if defined(_WIN32)
	if ( !table.HasField( pszFieldName ) )
		return false;
	*pData = table.GetInt( pszFieldName );
	return true;
#else
	throw std::runtime_error( "ADO field import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NDatabase::ImportField( const char *pszFieldName, bool *pData )
{
	if ( pStorageSource )
	{
		if ( !pStorageSource->HasIntField( pszFieldName ) )   // bools live in the int columns
		{
			AddMissingStorageField( pszFieldName, "bool", pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
				pStorageSource->GetCurrentRecordID() );
			return false;
		}
		*pData = pStorageSource->GetBool( pszFieldName );
		return true;
	}
#if defined(_WIN32)
	if ( !table.HasField( pszFieldName ) )
		return false;
	*pData = table.GetBool( pszFieldName );
	return true;
#else
	throw std::runtime_error( "ADO field import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NDatabase::ImportField( const char *pszFieldName, float *pData )
{
	if ( pStorageSource )
	{
		if ( !pStorageSource->HasFloatField( pszFieldName ) )
		{
			AddMissingStorageField( pszFieldName, "float", pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
				pStorageSource->GetCurrentRecordID() );
			return false;
		}
		*pData = pStorageSource->GetFloat( pszFieldName );
		return true;
	}
#if defined(_WIN32)
	if ( !table.HasField( pszFieldName ) )
		return false;
	*pData = table.GetFloat( pszFieldName );
	return true;
#else
	throw std::runtime_error( "ADO field import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NDatabase::ImportField( const char *pszFieldName, std::string *pData )
{
	if ( pStorageSource )
	{
		if ( !pStorageSource->HasStringField( pszFieldName ) )
		{
			AddMissingStorageField( pszFieldName, "string", pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
				pStorageSource->GetCurrentRecordID() );
			return false;
		}
		// Release game.db stores UTF-16, including Cyrillic in Lua CodeText.
		// The Russian Steam release runs with Windows-1251; keep its narrow
		// string bytes independent of the host's locale or UTF-8 filesystem.
		std::wstring ws = pStorageSource->GetWString( pszFieldName );
		NStr::ToAsciiCodePage( pData, ws, 1251 );
		return true;
	}
#if defined(_WIN32)
	if ( !table.HasField( pszFieldName ) )
		return false;
	*pData = (const char*)table.GetString( pszFieldName );
	return true;
#else
	throw std::runtime_error( "ADO field import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool NDatabase::ImportField( const char *pszFieldName, std::wstring *pData )
{
	if ( pStorageSource )
	{
		if ( !pStorageSource->HasStringField( pszFieldName ) )
		{
			AddMissingStorageField( pszFieldName, "wstring", pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
				pStorageSource->GetCurrentRecordID() );
			return false;
		}
		*pData = pStorageSource->GetWString( pszFieldName );
		return true;
	}
#if defined(_WIN32)
	if ( !table.HasField( pszFieldName ) )
		return false;
	*pData = (const wchar_t*)table.GetString( pszFieldName );
	return true;
#else
	throw std::runtime_error( "ADO field import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::ImportRelation( CDBRecord *pSrc, CDBTableBase *pDestTable, std::vector< CPtr<CDBRecord> > *pRefs )
{
	list< SRelation > &relations = GetRelations();
	ASSERT( pDestTable );
	std::vector< CPtr<CDBRecord> > &refs = *pRefs;
	list<SRelation>::iterator k;
	CDBTableBase *pLeft = GetTableByRecord( pSrc );
	CDBTableBase *pRight = pDestTable;
	refs.clear();
	if ( pLeft == 0 || pRight == 0 || pLeft == pRight )
	{
		ASSERT(0); // desired relation exists
		return;
	}
	bool bDone = false;
	for ( k = relations.begin(); k != relations.end(); ++k )
	{
		if ( k->pLeft == pLeft && k->pRight == pRight ) 
		{ // normal order
			ASSERT( !bDone );
			bDone = true;
			int nLeftID = pSrc->GetRecordID();
			for ( int z = 0; z < k->data.size(); z++ )
			{
				if ( k->data[z].nLeft == nLeftID )
				{
					CDBRecord *pAdd = pRight->GetDBRecord( k->data[z].nRight );
					if ( pAdd != 0 )
						refs.push_back( pAdd );
					else
						ASSERT(0); // relation points to non existing record
				}
			}
		}
		if ( k->pLeft == pRight && k->pRight == pLeft ) 
		{ // reverse order
			ASSERT( !bDone );
			bDone = true;
			int nLeftID = pSrc->GetRecordID();
			for ( int z = 0; z < k->data.size(); z++ )
			{
				if ( k->data[z].nRight == nLeftID )
				{
					CDBRecord *pAdd = pRight->GetDBRecord( k->data[z].nLeft );
					if ( pAdd != 0 )
						refs.push_back( pAdd );
					else
						ASSERT(0); // relation points to non existing record
				}
			}
		}
	}
	ASSERT( bDone );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::ImportField( const char *pszFieldName, CDBRecord **pRef, CDBTableBase *pDestTable )
{
	ASSERT( pDestTable );
	*pRef = 0;
	if ( pStorageSource && !pStorageSource->HasIntField( pszFieldName ) )
	{
		AddMissingStorageField( pszFieldName, "reference",
			pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0,
			pStorageSource->GetCurrentRecordID() );
		return;
	}
	int nID = 0;
	if ( pStorageSource )
		nID = pStorageSource->GetInt( pszFieldName );
#if defined(_WIN32)
	else
		nID = table.GetInt( pszFieldName );
#else
	else
		throw std::runtime_error( "ADO reference import is unavailable on Linux" );
#endif
	if ( pDestTable )
		*pRef = pDestTable->GetDBRecord( nID );
	if ( nID > 0 && !*pRef )
	{
		const int nSourceTable = pStorageDiagnostics ? pStorageDiagnostics->nTableID : 0;
		const int nSourceRecord = pStorageSource ? pStorageSource->GetInt( "ID" ) : 0;
		AddUnresolvedReference( pszFieldName, nSourceTable, nSourceRecord,
			GetTableID( pDestTable ), nID );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Post-load link builder (DBFormat/DataMap.cpp). For the dev-format (v0) game.db these links are
// pre-built by DataImport and serialized into the records, so loading them is enough. The release/
// Steam columnar (v1) game.db stores only raw columns and rebuilds records via Import(), which does
// NOT populate the cross-record links (CSkeleton::pAnimations, CRPGItem::looks, debris, per-pers
// inventory) - the release does that by calling BuildMapLinks from WinMain after every load. We call
// it here, gated on v1, so we don't double-push on a v0 db. (Resolved at Game.exe link time.)
namespace NDb { void BuildMapLinks( bool bTranslate, const std::vector<std::int32_t>* animationRowOrder ); }
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef std::unordered_map< int, CObj<CDBTableDataStorage> > CStorageHash;

static void ImportReleaseStorage( CStorageHash &storageTables,
	list<NDatabase::SRelation> &relations, const char *source,
	std::vector<std::int32_t>* animationRowOrder )
{
	if ( animationRowOrder )
	{
		animationRowOrder->clear();
		const CStorageHash::const_iterator animationTable = storageTables.find( 2 );
		if ( animationTable == storageTables.end() || !IsValid( animationTable->second ) )
			throw SFileIOError( "columnar game.db missing Animations table" );
		const CDBTableDataStorage *storage = animationTable->second;
		const int idColumn = CDBTableDataStorage::FindIndex( storage->intFileds, "ID" );
		if ( idColumn < 0 )
			throw SFileIOError( "columnar Animations table missing ID field" );
		std::set<int> seenIds;
		for ( const std::vector<int>& row : storage->records_int )
		{
			if ( idColumn >= (int)row.size() )
				throw SFileIOError( "columnar Animations row missing ID" );
			if ( !seenIds.insert( row[idColumn] ).second )
				throw SFileIOError( "columnar Animations table has duplicate ID" );
			animationRowOrder->push_back( row[idColumn] );
		}
	}
	for ( NDatabase::SRelation &relation : relations )
	{
		relation.pLeft = NDatabase::GetTable( relation.nTableLeft );
		relation.pRight = NDatabase::GetTable( relation.nTableRight );
		if ( !relation.pLeft || !relation.pRight )
			DebugTrace( "DB-SCHEMA ERROR relation=%s unresolved tables left=0x%08X right=0x%08X\n",
				relation.szTable.c_str(), relation.nTableLeft, relation.nTableRight );
	}
	const char *parityFlag = std::getenv( "S2_DB_PARITY" );
	if ( parityFlag && parityFlag[0] == '1' && parityFlag[1] == '\0' )
	{
		std::vector<S2FileIO::GameDatabaseRelation> relationSnapshot;
		for ( const NDatabase::SRelation &relation : relations )
		{
			S2FileIO::GameDatabaseRelation item;
			item.name = relation.szTable;
			item.leftTableId = relation.nTableLeft;
			item.rightTableId = relation.nTableRight;
			for ( const NDatabase::SRelation::SElement &link : relation.data )
				item.links.emplace_back( link.nLeft, link.nRight );
			relationSnapshot.push_back( item );
		}
		std::uint64_t hash = 0;
		if ( S2FileIO::HashGameDatabaseRelations( relationSnapshot, &hash ) )
			DebugTrace( "DB-PARITY relations=%d hash=%016llx\n",
				(int)relationSnapshot.size(), static_cast<unsigned long long>(hash) );
	}
	int nTables = 0;
	for ( CStorageHash::iterator it = storageTables.begin(); it != storageTables.end(); ++it )
	{
		CDBTableDataStorage *pStorage = it->second;
		ReportStorageShape( it->first, pStorage );
		if ( !NDatabase::GetTable( it->first ) )
			DebugTrace( "DB-SCHEMA ERROR unknown table type=0x%08X rows=%d\n", it->first,
				pStorage ? (int)pStorage->records_int.size() : 0 );
	}
	// Create all shells before importing any fields so cross-table references resolve.
	for ( CStorageHash::iterator it = storageTables.begin(); it != storageTables.end(); ++it )
	{
		CDBTableBase *pTable = NDatabase::GetTable( it->first );
		CDBTableDataStorage *pStorage = it->second;
		if ( !pTable || !pStorage ) continue;
		pStorageSource = pStorage;
		pTable->PreCreate( it->first );
		pStorageSource = 0;
	}
	for ( CStorageHash::iterator it = storageTables.begin(); it != storageTables.end(); ++it )
	{
		CDBTableBase *pTable = NDatabase::GetTable( it->first );
		CDBTableDataStorage *pStorage = it->second;
		if ( !pTable || !pStorage ) continue;
		pStorageSource = pStorage;
		SStorageDiagnostics diagnostics;
		BeginStorageDiagnostics( it->first, pStorage, &diagnostics );
		pTable->Import();
		pStorageDiagnostics = 0;
		ReportUnusedStorageFields( pStorage, diagnostics );
		pStorageSource = 0;
		++nTables;
	}
	DebugTrace( "DB-STORAGE: loaded %d columnar tables via %s\n", nTables, source );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void NDatabase::Serialize( CDataStream &file, CStructureSaver::EMode mode )
{
	CTablesHash &tables = GetTables();
	NDatabase::bIsDatabaseLoading = true;
	unresolvedReferences.clear();
	missingStorageFields.clear();
	bool bDidColumnarLoad = false;
	std::vector<std::int32_t> animationRowOrder;
	// The release v1 database is decoded without CStructureSaver or Windows
	// object-table layout. Historical v0/dev data and writes keep their path.
	bool bPortableRelease = false;
	const int nBaseSeek = file.GetPosition();
	const int nRemaining = file.GetSize() - nBaseSeek;
	if ( mode == CStructureSaver::READ && nRemaining >= 6 )
	{
		std::uint8_t header[6] = {};
		file.Read( header, sizeof(header) );
		file.Seek( nBaseSeek );
		bPortableRelease = header[0] == 4 && header[1] == 8 && header[2] == 1 &&
			header[3] == 0 && header[4] == 0 && header[5] == 0;
	}
	{
		if ( bPortableRelease )
		{
			std::vector<std::uint8_t> bytes( static_cast<std::size_t>(nRemaining) );
			file.Read( bytes.data(), static_cast<unsigned int>(bytes.size()) );
			file.Seek( nBaseSeek );
			S2FileIO::PortableGameDatabase decoded;
			std::string error;
			if ( !S2FileIO::LoadPortableGameDatabaseBytes(
				bytes.data(), bytes.size(), &decoded, &error ) )
				throw SFileIOError( std::string("portable game.db decode: ") + error );
			bDidColumnarLoad = true;
			CStorageHash storageTables;
			for ( const S2FileIO::GameDatabaseTable &table : decoded.tables )
			{
				CDBTableDataStorage *storage = new CDBTableDataStorage;
				for ( const S2FileIO::GameDatabaseColumn &column : table.columns )
					storage->fields.push_back( { column.name, column.type } );
				storage->records_int = table.intRows;
				storage->records_float = table.floatRows;
				storage->records_wstring = table.stringRows;
				storage->intFileds = table.intNames;
				storage->floatFileds = table.floatNames;
				storage->stringFileds = table.stringNames;
				storageTables[table.tableId] = storage;
			}
			list<SRelation> &relations = GetRelations();
			relations.clear();
			for ( const S2FileIO::GameDatabaseRelation &item : decoded.relations )
			{
				relations.push_back( SRelation() );
				SRelation &relation = relations.back();
				relation.szTable = item.name;
				relation.nTableLeft = item.leftTableId;
				relation.nTableRight = item.rightTableId;
				for ( const auto &link : item.links )
					relation.data.push_back( { link.first, link.second } );
			}
			ImportReleaseStorage( storageTables, relations, "portable v1", &animationRowOrder );
		}
		else
		{
			CStructureSaver f( file, mode );
			if ( mode == CStructureSaver::READ && f.GetVersion() >= 1 )
			{
				bDidColumnarLoad = true;
				CStorageHash storageTables;
				f.Add( 1, &storageTables );
				list<SRelation> &relations = GetRelations();
				f.Add( 2, &relations );
				ImportReleaseStorage( storageTables, relations, "legacy v1", &animationRowOrder );
			}
			else
				f.Add( 1, &tables );
		}
	}
	NDatabase::bIsDatabaseLoading = false;
			ReportUnresolvedReferences();
			ReportMissingStorageFields();
	// v1 columnar load rebuilt the records but not the cross-record links - build them now (the v0
	// path loads them already-built, so skip it there to avoid double-pushing into pAnimations etc.)
	if ( bDidColumnarLoad )
		NDb::BuildMapLinks( false, &animationRowOrder );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// CDBTableBase
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NDatabase;
void CDBTableBase::PreCreate( int nTypeID )
{
  records.clear();
	if ( pStorageSource )
	{
		// runtime load from the serialized columnar storage (release/Steam game.db v1)
		for ( pStorageSource->MoveFirst(); !pStorageSource->IsEof(); pStorageSource->MoveNext() )
		{
			CDBRecord *pRes = GetRecordTypes().CreateObject( nTypeID );
			ASSERT( pRes );
			if ( !pRes )
				break;
			pRes->nID = pStorageSource->GetInt( "ID" );
			records[ pRes->nID ] = pRes;
		}
		return;
	}
#if defined(_WIN32)
	// iterate through recordset & create records
	for ( ; !table.IsEof(); table.MoveNext() )
	{
		CDBRecord *pRes = GetRecordTypes().CreateObject( nTypeID );
		ASSERT( pRes );
		if ( !pRes )
			break;
		pRes->nID = table.GetInt( "ID" );
#ifdef _DEBUG
		CRecordHash::iterator i = records.find( pRes->nID );
		if ( i != records.end() )
			ASSERT(0);  // record with this ID already created
#endif
		records[ pRes->nID ] = pRes;
	}
#else
	throw std::runtime_error( "ADO record creation is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
void CDBTableBase::Refresh( int nTypeID )
{
	Sleep(0); // transfer control to the other thread (writing to DB usually happens not in the current thread)
	const CRecordHash copy = records;
  records.clear();
	// iterate through recordset & create records
	for ( ; !table.IsEof(); table.MoveNext() )
	{
		CObj<CDBRecord> pRes = GetRecordTypes().CreateObject( nTypeID );
		ASSERT( pRes );
		if ( !IsValid( pRes ) )
			break;
		pRes->nID = table.GetInt( "ID" );
		CRecordHash::const_iterator it = copy.find( pRes->nID );
		if ( it != copy.end() && IsValid( it->second ) )
			pRes = it->second;
#ifdef _DEBUG
		CRecordHash::iterator i = records.find( pRes->nID );
		if ( i != records.end() )
			ASSERT(0);  // record with this ID already created
#endif
		records[ pRes->nID ] = pRes;
	}
}
#endif // _WIN32: SQL refresh is editor-only.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDBTableBase::Import()
{
	if ( pStorageSource )
	{
		for ( pStorageSource->MoveFirst(); !pStorageSource->IsEof(); pStorageSource->MoveNext() )
		{
			int nID = pStorageSource->GetInt( "ID" );
			CRecordHash::iterator i = records.find( nID );
			if ( i == records.end() )
				continue; // should have been created in PreCreate
			i->second->Import();
		}
		return;
	}
#if defined(_WIN32)
	// iterate through records & Import() them
	for ( ; !table.IsEof(); table.MoveNext() )
	{
		int nID = table.GetInt( "ID" );
		CRecordHash::iterator i = records.find( nID );
		if ( i == records.end() )
		{
			ASSERT(0); // this record should be created on PreCreate stage
			continue;
		}
		i->second->Import();
	}
#else
	throw std::runtime_error( "ADO record import is unavailable on Linux" );
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CDBRecord* CDBTableBase::GetDBRecord( int nID )
{
	CRecordHash::iterator i = records.find( nID );
	if ( i == records.end() ) 
	{
#ifdef _MAPEDIT
		if ( !bIsDatabaseLoading && nID > 0 )
		{
			CTablesHash& tables = GetTables();
			for ( CTablesHash::const_iterator it = tables.begin(); it != tables.end(); ++it )
				if ( &it->second == this )
				{
					list<STableDescr> &tableDescrs = GetTableDescrs();
					for ( list<STableDescr>::iterator i = tableDescrs.begin(); i != tableDescrs.end(); ++i )
						if ( it->first == i->nTableID )
						{
							COLETable tbl;
							bool bClose = false;
							CDBRecord *pRes = 0;

							if ( !pConnection.GetInterfacePtr() )
							{
								EstablishConnection( szDataSource.c_str() );
								bClose = true;
							}
							if ( tbl.Open( i->szTable.c_str(), nID ) )
							{
								pRes = GetRecordTypes().CreateObject( it->first );
								ASSERT( pRes );
								if ( pRes )
								{
									pRes->nID = tbl.GetInt( "ID" );
									records[ pRes->nID ] = pRes;
									COLETable holder;
									holder = table; // CRAP
									table = tbl;
									pRes->Import();
									table = holder;
								}
							}
							if ( bClose )
								CloseConnection();
							return pRes;
						}
					return 0;
				}
		}
#endif
		return 0;
	}
	return i->second;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
string NDatabase::GetDBConnectionStr( const string &szDBName )
{
	string szRet = "DRIVER=SQL Server;SERVER=";
	szRet += szDBName;
	szRet += ";UID=sa;PWD=simple;DATABASE=A5GAME;";
	return szRet;
//	return "DRIVER=SQL Server;SERVER=localhost;UID=sa;DATABASE=A5GAME;";
//	return "DRIVER=SQL Server;SERVER=localhost;UID=sa;PWD=simple;DATABASE=A5GAME;";
//  return string( "DBQ=" ) + szDBName
//	 + ";DRIVER=Microsoft Access Driver (*.mdb);UserCommitSync=Yes;Threads=3;"
//	 + "SafeTransactions=0;PageTimeout=5;MaxScanRows=8;MaxBufferSize=2048;"
//	 + "FIL=MS Access;DriverId=25;";
//   + ";Driver={Driver do Microsoft Access (*.mdb)};DriverId=25;FIL=MS Access;"
//   + "MaxBufferSize=2048;MaxScanRows=8;PageTimeout=5;SafeTransactions=0;Threads=3;";
}
