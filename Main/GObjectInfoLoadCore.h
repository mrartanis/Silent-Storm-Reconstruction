#ifndef S2_GOBJECTINFO_LOAD_CORE_H
#define S2_GOBJECTINFO_LOAD_CORE_H

#include "GGeometry.h"
#include "GObjectInfo.h"
#include "GFileSkin.h"

namespace NGScene
{
inline void ConvertVertices( vector<SVertex> *pRes, const vector<SLoadVertex> &src )
{
	pRes->resize( src.size() );
	for ( size_t k = 0; k < src.size(); ++k )
	{
		SVertex &dst = (*pRes)[k];
		dst.pos = src[k].pos;
		NGfx::CalcCompactVector( &dst.normal, src[k].normal );
		dst.tex = src[k].tex;
		NGfx::CalcCompactVector( &dst.texU, src[k].texU );
		NGfx::CalcCompactVector( &dst.texV, src[k].texV );
	}
}

// Shared by the game's lazy loader and headless resource regression.
inline void AssignLoadedObjectInfo( CObjectInfo *pRes,
	const vector<SLoadVertex> &vertices,
	const vector<SLoadVertexWeight> &weights,
	const SPolygonIndices &geometry )
{
	CObjectInfo::SData data;
	data.geometry = geometry;
	if ( !vertices.empty() )
		ConvertWeights( &data.weights, weights, vertices.size() );
	ConvertVertices( &data.verts, vertices );
	pRes->Assign( data );
}

inline void ReadObjectInfoPieces( CStructureSaver *pSaver,
	CObjectInfoPieces *pRes )
{
	pSaver->Add( 4, &pRes->faces );
}
}

#endif
