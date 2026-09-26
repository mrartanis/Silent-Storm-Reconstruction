#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GGeometry.h"

#include <cstdio>

int main()
{
	vector<CVec3> positions;
	positions.push_back( CVec3(0, 0, 0) );
	positions.push_back( CVec3(1, 0, 0) );
	positions.push_back( CVec3(0, 1, 0) );
	positions.push_back( CVec3(0, 0, 0) );
	vector<WORD> matches;
	NGScene::MergePositions( &matches, &positions );
	if ( positions.size() != 3 || matches.size() != 4 ||
		matches[0] != 0 || matches[1] != 1 ||
		matches[2] != 2 || matches[3] != 0 )
		return 1;
	vector<STriangle> triangles;
	triangles.push_back( STriangle(0, 1, 2) );
	triangles.push_back( STriangle(0, 3, 2) );
	triangles.push_back( STriangle(3, 1, 2) );
	NGScene::FilterTrinagles( &triangles, matches );
	if ( triangles.size() != 2 ||
		triangles[0].i1 != 0 || triangles[0].i2 != 1 || triangles[0].i3 != 2 ||
		triangles[1].i1 != 0 || triangles[1].i2 != 1 || triangles[1].i3 != 2 )
		return 2;
	std::printf("positions=%zu mapping=%u,%u,%u,%u triangles=%zu\n",
		positions.size(), matches[0], matches[1], matches[2], matches[3],
		triangles.size());
	return 0;
}
