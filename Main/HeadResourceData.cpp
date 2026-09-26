#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "HeadResourceData.h"
#include "GResource.h"

namespace NLSHead
{
void LoadHeadResourceData( int id, SHeadResourceData *result )
{
	NGScene::CResourceOpener file( "Heads", id );
	file->Add(1, &result->streams);
	file->Add(2, &result->nVertices);
	file->Add(3, &result->copys);
	file->Add(4, &result->UVs);
	file->Add(5, &result->indices);
	file->Add(6, &result->tris);
}
}
