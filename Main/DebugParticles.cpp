#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif

// AI and animation diagnostics accumulate these spheres; the renderer may
// visualize them, but the storage itself belongs to the shared game state.
vector<SSphere> sphereParticles;
