#include "../Main/StdAfx.h"
#include "../Main/A5Script.h"
#include "../Main/wInterface.h"
#include "../Main/aiUnit.h"
#include "../Main/aiRouteLogic.h"
#include "../Main/scriptPtr.h"

#include <cstdio>

extern CPtr<NScript::CScript> pScript;

int main()
{
	CObj<NScript::CScript> script = new NScript::CScript;
	pScript = script.GetPtr();
	CObj<NAI::CAIRouteLogic> route = new NAI::CAIRouteLogic;
	NScript::luaPushCPtr( script->GetState(), route.GetPtr() );
	script->SetGlobal( "route" );
	if ( script->DoString( "before = RouteIsFinished(route)" ) != 0 ) return 1;
	script->ExecuteThreads();
	if ( !script->GetGlobal( "before" ).IsNil() ) return 2;
	script->Pop();
	route->Finish();
	if ( script->DoString( "after = RouteIsFinished(route)" ) != 0 ) return 3;
	script->ExecuteThreads();
	if ( script->GetGlobal( "after" ).GetNumber() != 1.0 ) return 4;
	script->Pop();
	pScript = 0;
	std::printf( "Lua route wait incomplete=nil completed=1\n" );
	return 0;
}
