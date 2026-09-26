#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif

#include "wInterface.h"
#include "wUnitCommands.h"

namespace NWorld
{
// The world command inherits the wrapped unit command's skip policy.
bool CCmdSetCommand::IsSkippable() const
{
	return pCmd->IsSkippable();
}
}
