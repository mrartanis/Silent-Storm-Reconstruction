#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "GDecal.h"

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x003c2142, CDecalTarget )
