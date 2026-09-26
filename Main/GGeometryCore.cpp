#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "GGeometry.h"

namespace NGScene
{
void FilterTrinagles( vector<STriangle> *pRes, const vector<WORD> &filter )
{
	int nTarget = 0;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		const STriangle &src = (*pRes)[k];
		STriangle &res = (*pRes)[nTarget];
		res.i1 = filter[ src.i1 ];
		res.i2 = filter[ src.i2 ];
		res.i3 = filter[ src.i3 ];
		nTarget += (res.i1 != res.i2) & (res.i1 != res.i3) & (res.i2 != res.i3);
	}
	pRes->resize( nTarget );
}

void MergePositions( vector<WORD> *pMatches, vector<CVec3> *pPositions )
{
	vector<CVec3> mergedPositions;
	vector<CVec3> &positions = *pPositions;
	vector<WORD> &posIndices = *pMatches;
	posIndices.resize( positions.size() );
	mergedPositions.reserve( pPositions->size() );
	typedef unordered_map<CVec3,int,SVec3Hash> CPosHash;
	CPosHash posHash;
	for ( int k = 0; k < positions.size(); ++k )
	{
		int nRes;
		CPosHash::iterator i = posHash.find( positions[k] );
		if ( i == posHash.end() )
		{
			nRes = mergedPositions.size();
			mergedPositions.push_back( positions[k] );
			posHash[ positions[k] ] = nRes;
		}
		else
			nRes = i->second;
		posIndices[k] = nRes;
	}
	positions = mergedPositions;
}
}
