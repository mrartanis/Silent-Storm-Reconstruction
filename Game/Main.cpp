#include "StdAfx.h"
#include "..\Main\GInit.h"
#include "WinFrame.h"
#include "..\Main\iMain.h"
#include "..\Input\Bind.h"
#include "..\ADOImport\BasicDB.h"
#include "..\DBFormat\DataMap.h"		// NDb::BuildMapLinks (post-load DB relation build)
#include "..\DBFormat\DataFaceGen.h" // [HARNESS] resolve game-used expression masks
#include "..\DBFormat\DataFormat.h"  // [HARNESS] CSequence record IDs
#include "..\Misc\StrProc.h"
#include "..\MiscDll\Commands.h"
#include "..\Main\GResource.h" // CRAP for lack of anything better, there should actually be version support
#include "..\Main\iInterMission.h" // CRAP, to start from mission
#include "..\Main\iLoading.h"      // NGame::InitLoadingScreen / TermLoadingScreen -- loading-screen UI built once at boot
#include "..\Misc\HPTimer.h"       // NHPTimer::UpdateHPTimerFrequency -- the per-frame TSC recalibration
#include "..\Misc\RandomGen.h"     // [HARNESS] deterministic ISAAC seed for paired architecture tests
#include "..\Main\iSaveManager.h" // CRAP, to start from mission
#include "..\Main\Sound.h"
#include "..\Main\WinInputConv.h" // Win32->NInput bridge: replays WM_KEYDOWN/WM_CHAR (OS auto-repeat)
#include "..\FileIO\BasicChunk1.h"  // [HARNESS] g_bSaveLoadDiag / SaveLoadDiag
#include "..\MiscDll\LogStream.h"   // [HARNESS] g_bHarnessLog (console-log tee)
#include "..\Main\A5Script.h"       // [HARNESS] ProcessCommand (console/lua entry for the command channel)
#include "..\Main\LSHead.h"         // [HARNESS] export the complete facial-sequence test corpus
#include "..\Main\iMission.h"       // [HARNESS] loaded mission and active player
#include "..\Main\iAdvFaceGen.h"    // [HARNESS] real advanced editor interface command
#include "..\Main\RPGGlobal.h"      // [HARNESS] saved merc list
#include "..\Main\RPGUnit.h"        // [HARNESS] per-merc committed head
#include "..\DBFormat\DataRPG.h"    // [HARNESS] nationality preview template
#include <dbghelp.h>                 // [HARNESS] post-load crash backtrace (SymFromAddr / StackWalk64)
#pragma comment(lib, "dbghelp.lib")
////////////////////////////////////////////////////////////////////////////////////////////////////
// [HARNESS] Unhandled-exception filter: on a post-load AV (the "silent close"), log a symbolic
// backtrace to _saveload.log so the driver can map the fault to a function. Installed only in harness
// runs (needs a PDB next to Game.exe -- CMake emits one for optimized configs). Writes directly to the
// file (no engine calls) so it survives a corrupted heap. Terminates after logging (unattended runs).
static LONG WINAPI HarnessCrashFilter( EXCEPTION_POINTERS *pEP )
{
	FILE *pF = fopen( "_saveload.log", "ab" );
	if ( pF )
	{
		fprintf( pF, "CRASH code=0x%08X addr=0x%p\n",
			(unsigned)pEP->ExceptionRecord->ExceptionCode, pEP->ExceptionRecord->ExceptionAddress );
		if ( pEP->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
			pEP->ExceptionRecord->NumberParameters >= 2 )
			fprintf( pF, "  access=%s data-addr=%p\n",
				pEP->ExceptionRecord->ExceptionInformation[0] ? "WRITE" : "READ",
				reinterpret_cast<void *>(pEP->ExceptionRecord->ExceptionInformation[1]) );
		HANDLE hProc = GetCurrentProcess();
		SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES );
		if ( SymInitialize( hProc, NULL, TRUE ) )
		{
			CONTEXT ctx = *pEP->ContextRecord;
			STACKFRAME64 sf; memset( &sf, 0, sizeof(sf) );
			#if defined(_M_X64)
			const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
			sf.AddrPC.Offset = ctx.Rip;    sf.AddrPC.Mode    = AddrModeFlat;
			sf.AddrFrame.Offset = ctx.Rbp; sf.AddrFrame.Mode = AddrModeFlat;
			sf.AddrStack.Offset = ctx.Rsp; sf.AddrStack.Mode = AddrModeFlat;
			#else
			const DWORD machine = IMAGE_FILE_MACHINE_I386;
			sf.AddrPC.Offset = ctx.Eip;    sf.AddrPC.Mode    = AddrModeFlat;
			sf.AddrFrame.Offset = ctx.Ebp; sf.AddrFrame.Mode = AddrModeFlat;
			sf.AddrStack.Offset = ctx.Esp; sf.AddrStack.Mode = AddrModeFlat;
			#endif
			for ( int n = 0; n < 40; ++n )
			{
				if ( !StackWalk64( machine, hProc, GetCurrentThread(), &sf, &ctx,
						NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL ) )
					break;
				DWORD64 addr = sf.AddrPC.Offset;
				if ( !addr )
					break;
				IMAGEHLP_MODULE64 mod; mod.SizeOfStruct = sizeof(mod);
				const char *szMod = SymGetModuleInfo64( hProc, addr, &mod ) ? mod.ModuleName : "?";
				char symBuf[ sizeof(SYMBOL_INFO) + 512 ];
				SYMBOL_INFO *pSym = (SYMBOL_INFO*)symBuf;
				pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
				pSym->MaxNameLen = 500;
				DWORD64 disp = 0;
				IMAGEHLP_LINE64 line; line.SizeOfStruct = sizeof(line); DWORD lineDisp = 0;
				bool bLine = !!SymGetLineFromAddr64( hProc, addr, &lineDisp, &line );
				if ( SymFromAddr( hProc, addr, &disp, pSym ) )
					fprintf( pF, "  #%02d %s!%s +0x%llX%s%s:%lu\n", n, szMod, pSym->Name,
						(unsigned long long)disp,
						bLine ? "  " : "", bLine ? line.FileName : "", bLine ? line.LineNumber : 0 );
				else
					fprintf( pF, "  #%02d %s +0x%llX  [0x%llX]\n", n, szMod,
						(unsigned long long)( addr - SymGetModuleBase64( hProc, addr ) ),
						(unsigned long long)addr );
			}
		}
		fprintf( pF, "CRASH-END\n" );
		fclose( pF );
	}
	return EXCEPTION_EXECUTE_HANDLER;   // terminate cleanly after logging
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//void DumpMemoryStats() {}

// Install a real DB-backed custom head on the first merc of a loaded mission.
// The slot test saves and reloads this same unit graph; no editor UI is needed.
static NRPG::CUnit* HarnessFaceGenMerc()
{
	NGame::IMission *pMission = dynamic_cast<NGame::IMission*>( NMainLoop::GetCurrentInterfaceForHarness() );
	if ( !pMission || !pMission->GetActivePlayer() )
		return 0;
	NRPG::CGlobalPlayer *pPlayer = pMission->GetActivePlayer()->GetGlobalPlayer();
	if ( !pPlayer )
		return 0;
	for ( size_t i = 0; i < pPlayer->mercs.size(); ++i )
		if ( IsValid( pPlayer->mercs[i] ) )
			return pPlayer->mercs[i];
	return 0;
}

static bool HarnessOpenFaceGenEditor()
{
	NGame::IMission *pMission = dynamic_cast<NGame::IMission*>( NMainLoop::GetCurrentInterfaceForHarness() );
	NRPG::CUnit *pMerc = HarnessFaceGenMerc();
	if ( !pMission || !pMerc || !pMission->GetActivePlayer() )
		return false;
	NRPG::CGlobalPlayer *pPlayer = pMission->GetActivePlayer()->GetGlobalPlayer();
	if ( !pPlayer || !IsValid( pPlayer->pSide ) )
		return false;
	CDBTable<NDb::CNationality> *pTable = NDatabase::GetTable<NDb::CNationality>();
	if ( !pTable )
		return false;
	NDb::CNationality *pNationality = 0;
	CDBIterator<NDb::CNationality> it( *pTable );
	while ( it.MoveNext() )
	{
		NDb::CNationality *p = it.Get();
		if ( IsValid( p ) && p->nFaceGenTemplate > 0 )
		{
			pNationality = p;
			break;
		}
	}
	if ( !pNationality )
		return false;
	NMainLoop::Command( new NGame::CICAdvFaceGen( pPlayer->pSide, pNationality, 0, pMerc ) );
	return true;
}

static bool HarnessFaceGenCommit( int caseIndex )
{
	if ( caseIndex < 0 || caseIndex > 2 )
		return false;
	NRPG::CUnit *pMerc = HarnessFaceGenMerc();
	if ( !pMerc )
		return false;
	CDBTable<NDb::CComplexHead> *pTable = NDatabase::GetTable<NDb::CComplexHead>();
	if ( !pTable )
		return false;
	NDb::CComplexHead *pDbHead = 0;
	CDBIterator<NDb::CComplexHead> it( *pTable );
	while ( it.MoveNext() )
	{
		NDb::CComplexHead *p = it.Get();
		if ( IsValid( p ) && IsValid( p->pHead ) && p->pHead->isTransformable &&
		     IsValid( p->pHead->pTransformableTextures ) )
		{
			pDbHead = p;
			break;
		}
	}
	if ( !pDbHead )
		return false;
	// These editor sliders mutate the shared DB template for live preview.
	// Restore every changed pointer after copying them into the merc's own
	// CHeadInfo; this makes the slot test prove per-unit persistence.
	struct CRestoreTemplateChoices
	{
		NDb::CComplexHead *pHead;
		CPtr<NDb::CRace> body;
		CPtr<NDb::CTRndModel> hair;
		CPtr<NDb::CTRndModel> meshes[4];
		CRestoreTemplateChoices( NDb::CComplexHead *p ): pHead( p ), body( p->pBodyColor ), hair( p->pHair )
		{
			for ( int i = 0; i < 4; ++i ) meshes[i] = p->pMeshes[i];
		}
		~CRestoreTemplateChoices()
		{
			pHead->pBodyColor = body;
			pHead->pHair = hair;
			for ( int i = 0; i < 4; ++i ) pHead->pMeshes[i] = meshes[i];
		}
	} restore( pDbHead );
	CObj<NLSHead::CHeadTransformInfo> pTransform = new NLSHead::CHeadTransformInfo( pDbHead, 0 );
	// Every column is a game-slider position mapped into [-1,+1]. Case 0
	// is the earlier mixed-control fixture with all UI controls explicitly set;
	// case 1 is the editor default;
	// case 2 exercises the five shape sliders and texture channels together.
	struct SSliderCase { const char *name; float values[3]; };
	const SSliderCase sliders[] = {
		{ "Age",         {  0.5f,  0.0f,  1.0f } },
		{ "Gender",      {  0.0f,  0.0f,  1.0f } },
		{ "Nationality", {  0.5f, -1.0f,  1.0f } },
		{ "Lips",        {  0.0f,  0.0f,  0.8f } },
		{ "Chin",        {  0.0f,  0.0f, -0.8f } },
		{ "Nose",        {  0.0f,  0.0f,  0.6f } },
		{ "Brows",      {  0.0f,  0.0f, -0.6f } },
		{ "Cheeks",      {  0.0f,  0.0f,  0.8f } },
		{ "HairColor",   { -1.0f,  1.0f,  1.0f } },
		{ "WomanHair",   {  1.0f,  1.0f, -1.0f } },
		{ "EyesColor",   {  1.0f, -1.0f,  0.5f } },
		{ "EyeGlasses",  { -1.0f, -1.0f,  1.0f } },
		{ "FaceDamage",  { -1.0f, -1.0f,  0.5f } },
		{ "FacialColor", {  0.5f, -1.0f,  0.5f } }
	};
	for ( size_t i = 0; i < sizeof(sliders) / sizeof(sliders[0]); ++i )
		pTransform->SetMMTension( sliders[i].name, sliders[i].values[caseIndex] );
	CObj<NLSHead::CHeadInfo> pHead = pTransform->CreateHeadInfo();
	if ( !IsValid( pHead ) || !pHead->IsStaticHead() || !IsValid( pHead->GetFaceTexture() ) )
		return false;
	pMerc->SetHeadInfo( pHead );
	return true;
}

static void HarnessFaceGenStatus( const char *phase )
{
	NRPG::CUnit *pMerc = HarnessFaceGenMerc();
	NLSHead::CHeadInfo *pHead = pMerc ? pMerc->GetHeadInfo() : 0;
	NLSHead::SFaceGenBakeProbeResult result;
	NLSHead::ProbeCommittedFaceGenHead( pHead, &result );
	int hair = pHead && IsValid( pHead->GetHair() ) ? pHead->GetHair()->GetRecordID() : -1;
	int body = pHead && IsValid( pHead->GetBodyColor() ) ? pHead->GetBodyColor()->GetRecordID() : -1;
	int meshes[4] = { -1, -1, -1, -1 }, ifMeshes[4] = { -1, -1, -1, -1 };
	if ( pHead )
		for ( int i = 0; i < 4; ++i )
		{
			if ( IsValid( pHead->GetMeshes()[i] ) )
				meshes[i] = pHead->GetMeshes()[i]->GetRecordID();
			if ( IsValid( pHead->GetIFMeshes()[i] ) )
				ifMeshes[i] = pHead->GetIFMeshes()[i]->GetRecordID();
		}
	if ( const char *path = getenv( "S2_FACE_SLOT_ANIMATOR_PATH" ) )
	{
		NLSHead::CFaceGenMeshHolder *pMesh = pHead ? dynamic_cast<NLSHead::CFaceGenMeshHolder*>( pHead->GetMesh() ) : 0;
		if ( pMesh && pMesh->GetInfo().animatorStreams.size() == 1 )
		{
			CMemoryStream &stream = pMesh->GetInfo().animatorStreams[0];
			FILE *file = fopen( path, "wb" );
			if ( file )
			{
				size_t wrote = fwrite( stream.GetBuffer(), 1, stream.GetSize(), file );
				fclose( file );
				SaveLoadDiag( "[harness] facegen stream phase=%s bytes=%d exported=%d\n",
					phase, stream.GetSize(), wrote == (size_t)stream.GetSize() ? 1 : 0 );
			}
		}
	}
	SaveLoadDiag( "[harness] facegen status phase=%s merc=%d head=%d static=%d textured=%d animator=%d animator_hash=%016llx pixels=%d hash=%016llx\n",
		phase, pMerc ? 1 : 0, result.headId, result.staticHead ? 1 : 0,
		result.textured ? 1 : 0, result.animatorBytes, result.animatorHash, result.nonzeroPixels,
		result.textureHash );
	SaveLoadDiag( "[harness] facegen models phase=%s hair=%d body=%d mesh=%d,%d,%d,%d ifmesh=%d,%d,%d,%d\n",
		phase, hair, body, meshes[0], meshes[1], meshes[2], meshes[3],
		ifMeshes[0], ifMeshes[1], ifMeshes[2], ifMeshes[3] );
}

// ============================================================================================
// [HARNESS] Frame-polled command channel -- a minimal RTC protocol between an external driver and
// the running game. Once per frame (when g_bHarnessLog is on) the main loop reads ONE command line
// from ".\_harness_cmd.txt" (raw system-ANSI bytes so Cyrillic slot names round-trip), clears the
// file, executes it, and acks into _saveload.log; engine output goes to _console.log (the tee).
// Verbs (extend freely -- this is the protocol foundation):
//   console <text>   run a console command / var / "@lua" (the global ProcessCommand entry)
//   load <slot>      queue a save-slot load (slot name = raw ANSI)
//   save <slot>      queue an ordinary game save slot
//   rng <uint32> [console <text>] reset RNG and optionally run an action in the same frame
//   turnsave <slot> hand the turn to AI and queue an ordinary save in the same frame
//   facefixtures    export DB-backed facial-sequence streams into S2_FACE_FIXTURE_DIR
//   headfixtures    export all DB-backed head animator segments into S2_FACE_FIXTURE_DIR
//   faceexpressions log the DB's game-used expression -> sequence mapping
//   facegenbake     bake three DB-backed FaceGen heads and log mesh/texture evidence
//   facegencommit <0..2> install a DB-backed custom head on the first live merc
//   facegenstatus   inspect that merc's committed head after save/load
//   facegeneditor   push the real Advanced FaceGen UI over a loaded mission
//   facegenedit <name> <0..100> set a live UI scroll and call UpdateHead
//   facegenpreview  bake the current editor preview through CreateLSHeadInfo
//   quit             request a clean shutdown
// The driver (gen/_loadtest.py in s2_scratch) writes _harness_cmd.txt and reads the logs. Sweep the
// whole harness by grepping "[HARNESS]".
// ============================================================================================
static bool HarnessPoll()   // returns false to request main-loop exit
{
	FILE *pF = fopen( "_harness_cmd.txt", "rb" );
	if ( !pF )
		return true;
	char szBuf[2048];
	size_t n = fread( szBuf, 1, sizeof(szBuf) - 1, pF );
	fclose( pF );
	remove( "_harness_cmd.txt" );
	szBuf[n] = 0;
	while ( n && ( szBuf[n-1] == '\n' || szBuf[n-1] == '\r' || szBuf[n-1] == ' ' || szBuf[n-1] == '\t' ) )
		szBuf[--n] = 0;
	if ( n == 0 )
		return true;
	string sCmd( szBuf );
	SaveLoadDiag( "[harness] cmd: %s\n", sCmd.c_str() );
	if ( sCmd == "quit" )
		return false;
	else if ( sCmd.compare( 0, 8, "console " ) == 0 )
		ProcessCommand( NStr::ToUnicode( sCmd.substr( 8 ) ) );
	else if ( sCmd.compare( 0, 5, "load " ) == 0 )
		NMainLoop::Command( new NMainLoop::CICLoad( sCmd.substr( 5 ) ) );
	else if ( sCmd.compare( 0, 5, "save " ) == 0 )
		NMainLoop::Command( new NMainLoop::CICSave( sCmd.substr( 5 ), true ) );
	else if ( sCmd.compare( 0, 9, "turnsave " ) == 0 )
	{
		// Catches the same CICSave path as F5 before a fast AI turn can finish
		// between two external harness commands.
		ProcessCommand( NStr::ToUnicode( "@PlayerGiveTurn(1)" ) );
		NMainLoop::Command( new NMainLoop::CICSave( sCmd.substr( 9 ) ) );
	}
	else if ( sCmd == "facefixtures" )
		SaveLoadDiag( "[harness] face fixtures: %d sequence streams\n", NLSHead::ExportAllFaceSequenceFixtures() );
	else if ( sCmd == "headfixtures" )
		SaveLoadDiag( "[harness] head fixtures: %d animator streams\n", NLSHead::ExportAllFaceHeadFixtures() );
	else if ( sCmd == "faceexpressions" )
	{
		for ( int kind = NDb::FE_NORMAL; kind <= NDb::FE_DISGUST; ++kind )
		{
			NDb::CSequence *pSeq = NDb::GetSequenceByExpression( (NDb::EFaceExpression)kind );
			SaveLoadDiag( "[harness] face expression kind=%d sequence=%d\n",
				kind, IsValid( pSeq ) ? pSeq->GetRecordID() : -1 );
		}
	}
	else if ( sCmd == "facegenbake" )
	{
		for ( int i = 0; i < 3; ++i )
		{
			NLSHead::SFaceGenBakeProbeResult result;
			bool ok = NLSHead::ProbeFaceGenBake( i, &result );
			SaveLoadDiag( "[harness] facegen bake case=%d ok=%d head=%d static=%d textured=%d roundtrip=%d animator=%d pixels=%d hash=%016llx\n",
				i, ok ? 1 : 0, result.headId, result.staticHead ? 1 : 0,
				result.textured ? 1 : 0, result.roundTripped ? 1 : 0,
				result.animatorBytes, result.nonzeroPixels, result.textureHash );
		}
	}
	else if ( sCmd.compare( 0, 13, "facegencommit" ) == 0 &&
	          ( sCmd.size() == 13 || sCmd[13] == ' ' ) )
	{
		int caseIndex = 0;
		if ( sCmd.size() > 13 )
		{
			const char *value = sCmd.c_str() + 14;
			char *end = 0;
			long parsed = strtol( value, &end, 10 );
			caseIndex = value != end && *end == 0 ? (int)parsed : -1;
		}
		bool ok = HarnessFaceGenCommit( caseIndex );
		SaveLoadDiag( "[harness] facegen commit case=%d ok=%d\n", caseIndex, ok ? 1 : 0 );
		HarnessFaceGenStatus( "commit" );
	}
	else if ( sCmd == "facegenstatus" )
		HarnessFaceGenStatus( "query" );
	else if ( sCmd == "facegeneditor" )
		SaveLoadDiag( "[harness] facegen editor queued=%d\n", HarnessOpenFaceGenEditor() ? 1 : 0 );
	else if ( sCmd.compare( 0, 12, "facegenedit " ) == 0 )
	{
		char name[64] = { 0 };
		int value = -1, observed = -1;
		bool parsed = sscanf( sCmd.c_str(), "facegenedit %63s %d", name, &value ) == 2;
		bool ok = parsed && NGame::SetAdvFaceGenSliderForHarness( name, value, &observed );
		SaveLoadDiag( "[harness] facegen edit name=%s requested=%d observed=%d ok=%d\n",
			name, value, observed, ok ? 1 : 0 );
	}
	else if ( sCmd == "facegenpreview" )
	{
		NLSHead::SFaceGenBakeProbeResult result;
		bool ok = NGame::ProbeAdvFaceGenEditorForHarness( &result );
		SaveLoadDiag( "[harness] facegen preview ok=%d head=%d static=%d textured=%d animator=%d animator_hash=%016llx pixels=%d hash=%016llx\n",
			ok ? 1 : 0, result.headId, result.staticHead ? 1 : 0, result.textured ? 1 : 0,
			result.animatorBytes, result.animatorHash, result.nonzeroPixels, result.textureHash );
	}
	else if ( sCmd.compare( 0, 4, "rng " ) == 0 )
	{
		const char *pSeed = sCmd.c_str() + 4;
		char *pEnd = 0;
		unsigned long nSeed = strtoul( pSeed, &pEnd, 0 );
		bool bConsole = strncmp( pEnd, " console ", 9 ) == 0;
		if ( pSeed != pEnd && ( *pEnd == 0 || bConsole ) )
		{
			random.SeedForHarness( static_cast<unsigned int>( nSeed ) );
			srand( static_cast<unsigned int>( nSeed ) );
			SaveLoadDiag( "[harness] rng seeded: %lu\n", nSeed );
			if ( bConsole )
				ProcessCommand( NStr::ToUnicode( pEnd + 9 ) );
		}
		else
			SaveLoadDiag( "[harness] invalid rng seed\n" );
	}
	else
		SaveLoadDiag( "[harness] unknown cmd\n" );
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	//tmpFlag |= _CRTDBG_LEAK_CHECK_DF;// | _CRTDBG_CHECK_ALWAYS_DF;
  tmpFlag = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;// | _CRTDBG_CHECK_ALWAYS_DF;
  _CrtSetDbgFlag( tmpFlag );
	//_CrtSetBreakAlloc( 114 );
#else
	srand( GetTickCount() );
#endif // _DEBUG
	NGScene::AddResourceDir( ".\\res" );
	NGScene::RunResourceLoadingThread();
  // load game database
	try
	{
		CFileStream f;
		f.OpenRead( "game.db" );
		NDatabase::Serialize( f, CStructureSaver::READ );
	}
	catch (...)
	{
		ASSERT( 0 ); // game.db not found
		MessageBox( 0, "File game.db not found", "Error", MB_OK );
		return 0;
	}

	// NOTE: no explicit NDb::BuildMapLinks() here (it is APPEND-ONLY -- calling it twice duplicates
	// skeleton-anim/debris/uniform-look/per-pers-inventory links). Every load path already covers it:
	// a v1/Steam columnar game.db has its links built by NDatabase::Serialize itself (gated internal
	// BuildMapLinks(false), ADOImport\BasicDB.cpp), and a v0 dev-format game.db carries the links
	// serialized in its records (DataImport runs BuildMapLinks before exporting; CSkeleton::pAnimations
	// is a serialized member). The unconditional call that used to sit here double-pushed on both.

	// init subsystems
	if ( !NWinFrame::InitApplication( hInstance, "Silent Storm", "Silent Storm" ) )
		return 0;
	if ( !NGfx::Init3D( NWinFrame::GetWnd() ) )
	{
		ASSERT(0); // DX8 not found
		MessageBox( 0, "Failed to initialize Direct3D8", "Error", MB_OK );
		return 0;
	}
	#if !defined(S2_X64_MEDIA_STUBS) || defined(S2_NATIVE_MUSIC)
	if ( !NSound::InitSound( NWinFrame::GetWnd() ) )
	{
		ASSERT(0); // FMod not found
		MessageBox( 0, "Failed to initialize FMod", "Error", MB_OK );
		return 0;
	}
	#else
	OutputDebugStringA( "x64 core: FMOD unavailable; sound disabled until miniaudio backend\n" );
	#endif
	if ( !NInput::InitInput( NWinFrame::GetWnd() ) )
	{
		ASSERT(0); // DX8input not found
		MessageBox( 0, "Failed to initialize DirectInput8", "Error", MB_OK );
		return 0;
	}

	// Load config & process params
	NGlobal::LoadConfig( ".\\cfg\\autoexec.cfg" );

	vector<string> szParams;
	bool bDoLoad = false;
	bool bHarnessActive = false;
	string szLoadSlot;
	NStr::SplitStringWithMultipleBrackets( lpCmdLine, szParams, ' ' );
	string szCfg( "start.cfg" );
	for ( int i = 0; i < szParams.size(); ++i )
	{
		if ( szParams[i] == "-fullscreen" )
			NGlobal::SetVar( "gfx_fullscreen", 1 );
		else if ( szParams[i] == "-windowed" )
			NGlobal::SetVar( "gfx_fullscreen", 0 );
		else if ( szParams[i] == "-harness" )   // [HARNESS] enable the command channel + console tee WITHOUT auto-loading
		{
			g_bSaveLoadDiag = true;
			g_bHarnessLog = true;
			remove( "_saveload.log" );
			remove( "_console.log" );
			SaveLoadDiag( "BOOT harness (no auto-load)\n" );
		}
		else if ( szParams[i] == "-harness-active" ) // simulate foreground frames in unattended tests
			bHarnessActive = true;
		else if ( szParams[i] == "-320" )
			NGlobal::SetVar( "gfx_resolution", 320 );
		else if ( szParams[i] == "-400" )
			NGlobal::SetVar( "gfx_resolution", 400 );
		else if ( szParams[i] == "-640" )
			NGlobal::SetVar( "gfx_resolution", 640 );
		else if ( szParams[i] == "-800" )
			NGlobal::SetVar( "gfx_resolution", 800 );
		else if ( szParams[i] == "-1024" )
			NGlobal::SetVar( "gfx_resolution", 1024 );
		else if ( szParams[i] == "-1280" )
			NGlobal::SetVar( "gfx_resolution", 1280 );
		else if ( szParams[i] == "-1600" )
			NGlobal::SetVar( "gfx_resolution", 1600 );
		else if ( szParams[i] == "-nops" )
			NGlobal::SetVar( "gfx_nopixelshaders", 1 );
		else if ( szParams[i] == "-novs" )
			NGlobal::SetVar( "gfx_novertexshaders", 1 );
		else if ( szParams[i] == "-gfxvalidate" )
			NGlobal::SetVar( "gfx_validate", 1 );
		else if ( szParams[i] == "-aniso" )
			NGlobal::SetVar( "gfx_anisotropic_filter", 2 );   // retail @0x409f65: level 2 (1 = off)
		else if ( szParams[i] == "-bannp2" )
			NGlobal::SetVar( "gfx_fix_ban_np2", 1 );
		else if ( szParams[i] == "-nvrulez" )
			NGlobal::SetVar( "gfx_fix_nv_np2_hack", 1 );
		else if ( szParams[i] == "-dxtoff" )
			NGlobal::SetVar( "gfx_texture_usedxt", 0 );

		if ( szParams[i] == "-nosound" )
		{
			NGlobal::SetVar( "sound_mode", 0 );   // retail WinMain @0x9810 sets both (sound_mode is what SetModeFromConfig reads)
			NGlobal::SetVar( "sound_init", 0 );
		}

		if ( szParams[i] == "-noai" )
			NGlobal::SetVar( "game_noai", 1 );

		if ( szParams[i] == "-load" )
			bDoLoad = true;
		// -loadslot: unattended save-load test harness. Read the target slot NAME (raw bytes,
		// system-ANSI/CP1251 -- so Cyrillic folder names round-trip) from ".\_loadslot.txt", auto-load
		// it instead of the menu, and enable the SaveLoadDiag object trace (-> ".\_saveload.log").
		if ( szParams[i] == "-loadslot" )
		{
			try
			{
				CFileStream fSlot;
				fSlot.OpenRead( "_loadslot.txt" );
				int nLen = fSlot.GetSize();
				string sSlot;
				sSlot.resize( nLen );
				if ( nLen > 0 )
					fSlot.Read( &sSlot[0], nLen );
				while ( !sSlot.empty() && ( sSlot[sSlot.size()-1] == '\n' || sSlot[sSlot.size()-1] == '\r' || sSlot[sSlot.size()-1] == ' ' || sSlot[sSlot.size()-1] == '\t' ) )
					sSlot.resize( sSlot.size() - 1 );
				if ( !sSlot.empty() )
				{
					szLoadSlot = sSlot;
					bDoLoad = true;
					g_bSaveLoadDiag = true;   // [HARNESS]
					g_bWireAudit = true;      // [HARNESS] per-load wire-divergence audit -> _wireaudit.log
					g_bHarnessLog = true;     // [HARNESS] tee engine console output to _console.log
					remove( "_saveload.log" );   // fresh trace each run
					remove( "_console.log" );
					remove( "_wireaudit.log" );
					SaveLoadDiag( "BOOT loadslot=[%s]\n", szLoadSlot.c_str() );
				}
			}
			catch ( ... ) {}
		}
		if ( szParams[i] == "-cfg" )
		{
			if ( i + 1 < szParams.size() )
				szCfg = szParams[++i];
		}
	}
	//
	#if defined(S2_X64_MEDIA_STUBS) && !defined(S2_NATIVE_MUSIC)
	NGlobal::SetVar( "sound_mode", 0 );
	NGlobal::SetVar( "sound_init", 0 );
	#endif
	if ( !NGScene::SetModeFromConfig() )
	{
		ASSERT(0); // no mode found
		MessageBox( 0, "Failed to set display mode", "Error", MB_OK );
		return 0;
	}
	//
	if ( !NSound::SetModeFromConfig() )
	{
		ASSERT(0);
		MessageBox( 0, "Failed to set sound mode", "Error", MB_OK );
		return 0;
	}
	//
	// Build the loading-screen UI once at boot, BEFORE the first interface command is queued. Mirrors
	// release NMainLoop::InitInterface @0x1f5800, whose first unconditional statement is InitLoadingScreen()
	// (iMain.c:699-700), run before the bLoad?CICLoad:CICInterMission build. Builds the three file-scope
	// iLoading globals (cursor + a SEPARATE CInterface + CLoadingUI) so the load paths paint a splash instead
	// of a black screen. Paired with NGame::TermLoadingScreen() in NMainLoop::DoneInterface (called at shutdown
	// below). Safe: this CInterface is never pushed onto NMainLoop::interfaces; it is only Step+Drawn inside
	// ShowLoadingScreen, so ShowWindow(SHOW) here does not overlay the menu queued just below.
	NGame::InitLoadingScreen();
	if ( bDoLoad )
		NMainLoop::Command( new NMainLoop::CICLoad( szLoadSlot.empty() ? NMainLoop::GetQuickSaveSlot( true ) : szLoadSlot ) );
	else
		NMainLoop::Command( new CICInterMission( szCfg ) );
	if ( g_bHarnessLog )
		SetUnhandledExceptionFilter( HarnessCrashFilter );   // [HARNESS] symbolic backtrace on post-load AV
	SWinToInputMessageConverter sWinInputConv;
	for (;;)
	{
		NWinFrame::PumpMessages();
		bool bActive = NWinFrame::IsAppActive();
		// A hidden debugger leaves the window inactive. The ordinary harness still
		// preserves retail pause-on-background behavior; this explicit test mode
		// advances mission/input frames exactly as a focused window would.
		bool bStepActive = bActive || ( g_bHarnessLog && bHarnessActive );
		NInput::PumpMessages( bStepActive );
		// Re-emit the coalesced Win32 keyboard stream (WM_KEYDOWN/WM_CHAR, OS auto-repeated)
		// as NInput messages, exactly as the retail main loop does (WinMain @0x9810: right
		// after NInput::PumpMessages, before StepApp) -- this is what gives held keys repeat.
		sWinInputConv.Do();
		if ( NWinFrame::IsExit() )
			break;
		if ( !NMainLoop::StepApp( bStepActive, bActive ) )
			break;
		// retail WinMain @0x9810 calls this every frame right here (@0x40a70b, immediately after
		// StepApp): it re-derives the RDTSC->seconds scale against a rolling QPC window. Dev only ever
		// calibrated once, in the HPTimer static ctor, so fProcFreq1 was frozen at whatever clock the
		// CPU happened to be running at during boot -- everything on NHPTimer (the sound mixer's
		// timing, the window-message stamps) then drifts as SpeedStep/turbo move the TSC ratio.
		// Self-throttling: only recalibrates once the 50ms reference window has elapsed.
		NHPTimer::UpdateHPTimerFrequency();
		if ( g_bHarnessLog && !HarnessPoll() )   // [HARNESS] frame-polled command channel
			break;
		if ( !bStepActive )
			Sleep( 40 );
	}
	//
	NGlobal::SaveConfig( ".\\cfg\\config.cfg" );
	NMainLoop::DoneInterface();
	NGfx::Done3D();
	NInput::DoneInput();
	NSound::DoneSound();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
