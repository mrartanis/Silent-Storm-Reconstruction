#ifndef __HEADRESOURCEDATA_H_
#define __HEADRESOURCEDATA_H_

#include "../FileIO/Streams.h"
#include "../Misc/Geom.h"
#include <cstdint>
#include <vector>

namespace NLSHead
{
// CPU-side fields of the shipped Heads resource, before LifeStudio creates
// the live animator objects. The renderer and the portable tests share this
// exact six-tag read path.
struct SHeadResourceData
{
	std::vector<CMemoryStream> streams;
	std::vector<int> nVertices;
	std::vector<CTPoint<int>> copys;
	std::vector<CVec2> UVs;
	std::vector<std::uint16_t> indices;
	std::vector<int> tris;
};

void LoadHeadResourceData( int id, SHeadResourceData *result );
}

#endif // __HEADRESOURCEDATA_H_
