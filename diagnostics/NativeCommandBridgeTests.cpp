#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wInterface.h"
#include "../Main/wUnitCommands.h"

#include <cstdio>
#include <cstring>

static std::vector<unsigned char> SaveCommand(NWorld::CCmd* command)
{
	CMemoryStream stream;
	{
		CStructureSaver saver(stream, CStructureSaver::WRITE);
		saver.Add(2, command);
	}
	const auto* bytes = static_cast<const unsigned char*>(stream.GetBuffer());
	return {bytes, bytes + stream.GetSize()};
}

int main()
{
	// Ownership is runtime state. Saving the same command with another owner
	// must not persist reference counts (or any native vtable bytes).
	CObj<NWorld::CCmdHide> command = new NWorld::CCmdHide;
	const auto before = SaveCommand(command);
	CObj<NWorld::CCmdHide> extraOwner = command;
	const auto after = SaveCommand(command);
	if (before != after) {
		std::fprintf(stderr, "FAIL: command save depends on runtime ownership\n");
		return 10;
	}
	CObj<NWorld::CCmdSetCommand> optional =
		new NWorld::CCmdSetCommand( nullptr, new NWorld::CCmdContinue );
	CObj<NWorld::CCmdSetCommand> required =
		new NWorld::CCmdSetCommand( nullptr, new NWorld::CCmdStartCombat );
	if ( !optional->IsSkippable() || required->IsSkippable() ) return 1;
	std::puts( "native command bridge skip policy: passed" );
	return 0;
}
