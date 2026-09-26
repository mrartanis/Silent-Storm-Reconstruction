#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../Script/StdAfx.h"
#endif
#include "../Script/Script.h"
#include "../Misc/RandomGen.h"

namespace NScript
{
// The authored mission scripts use this original game binding. Keep its
// argument/result behavior identical on both hosts, including invalid args.
int luaRandom( lua_State* state )
{
	Script script(state);
#if defined(_WIN32)
	CRandomGenerator &gameRandom = random;
#else
	CRandomGenerator &gameRandom = s2_game_random;
#endif
	if ( !script.GetObject( 1 ).IsNumber() )
		script.PushNumber( 0 );
	else if ( script.GetTop() == 0 )
		script.PushNumber( gameRandom.Get() );
	else if ( script.GetTop() > 1 )
		script.PushNumber( gameRandom.Get( script.GetObject(1).GetInteger(),
			script.GetObject(2).GetInteger() ) );
	else
		script.PushNumber( gameRandom.Get( script.GetObject(1).GetInteger() ) );
	return 1;
}
}
