#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiLogic.h"

#include <cstdio>

int main()
{
	CObj<NAI::CAILogic> logic = new NAI::CAILogic;
	if ( logic->IsFinished() || logic->IsEndOfTurn() || !logic->IsActive() )
		return 1;
	logic->Pause();
	logic->Pause();
	if ( logic->IsActive() ) return 2;
	logic->Resume();
	if ( logic->IsActive() ) return 3;
	logic->Resume();
	if ( !logic->IsActive() ) return 4;
	logic->Finish();
	if ( !logic->IsFinished() || !logic->IsEndOfTurn() ) return 5;
	std::puts( "native AI logic lifecycle: passed" );
	return 0;
}
