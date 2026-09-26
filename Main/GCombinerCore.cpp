#if defined(_WIN32)
#include "StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "DG.H"
#include "GScene.h"
#include "GCombiner.h"
#include "Bound.h"
#if !defined(_WIN32)
#include <thread>
#endif

namespace NGScene
{
IPart::IPart( CPtrFuncBase<CObjectInfo> *pData, CPerMaterialCombiner *_pCombiner, bool _bIsSolid )
	: pObjInfo(pData), pCombiner( _pCombiner ), bIsSolid( _bIsSolid )
{
	if ( IsValid( pCombiner ) )
		pCombiner->AddPart( this );
}

IPart::~IPart()
{
	if ( IsValid( pCombiner ) )
		pCombiner->RemovePart( this );
}

// Retail @0xfe340: wait for the generator node to produce a mesh.
void IPart::RefreshObjectInfo()
{
	pObjInfo.Refresh();
	while ( !pObjInfo->GetValue() )
	{
#if defined(_WIN32)
		Sleep(0);
#else
		std::this_thread::yield();
#endif
	}
}

void IPart::ResetCachedTransform()
{
	xformedPositions.clear();
	gfxData.clear();
}

void IPart::SetCombiner( CPerMaterialCombiner *_pCombiner, bool bForceUpdate, bool bAnimated )
{
	if ( pCombiner == _pCombiner )
	{
		if ( IsValid( pCombiner ) )
		{
			if ( bForceUpdate ) pCombiner->MarkWasted( this );
			if ( bAnimated ) pCombiner->Animated();
			if ( bForceUpdate || bAnimated ) ResetCachedTransform();
		}
		return;
	}
	if ( IsValid( pCombiner ) ) pCombiner->RemovePart( this );
	pCombiner = _pCombiner;
	if ( IsValid( pCombiner ) ) pCombiner->AddPart( this );
	ResetCachedTransform();
}

CPerMaterialCombiner::CPerMaterialCombiner( SStaticTrackers *pTrackers )
{
	if ( pTrackers )
	{
		pSolidTracker = pTrackers->pSolidTracker;
		pTracker = pTrackers->pTracker;
	}
	pAnimation = new CVersioningBase;
}

CPerMaterialCombiner::~CPerMaterialCombiner()
{
	if ( IsValid( pSolidTracker ) ) pSolidTracker->Updated();
	if ( IsValid( pTracker ) ) pTracker->Updated();
}

static bool CmpMaterial( IPart *pA, IPart *pB )
{
	return pA->GetSortValue() > pB->GetSortValue();
}

void CPerMaterialCombiner::AddPart( IPart *pPart )
{
	ASSERT( value.size() < PF_MAX_PARTS_PER_COMBINER );
	value.push_back( pPart );
	sort( value.begin(), value.end(), CmpMaterial );
	Updated();
	if ( IsValid( pSolidTracker ) && pPart->IsSolid() ) pSolidTracker->Updated();
	if ( IsValid( pTracker ) ) pTracker->Updated();
}

void CPerMaterialCombiner::RemovePart( IPart *pPart )
{
	vector< CPtr<IPart> >::iterator i = find( value.begin(), value.end(), pPart );
	if ( i == value.end() ) return;
	value.erase( i );
	Updated();
	if ( IsValid( pSolidTracker ) && pPart->IsSolid() ) pSolidTracker->Updated();
	if ( IsValid( pTracker ) ) pTracker->Updated();
}

void CPerMaterialCombiner::MarkWasted( IPart *pPart )
{
	Updated();
	if ( IsValid( pSolidTracker ) && pPart->IsSolid() ) pSolidTracker->Updated();
	if ( IsValid( pTracker ) ) pTracker->Updated();
}

int CPerMaterialCombiner::operator&( CStructureSaver &f )
{
	f.Add( 1, &value );
	f.Add( 2, &pTracker );
	f.Add( 3, &pAnimation );
	f.Add( 4, &pSolidTracker );
	f.Add( 5, &bHasChanged );
	return 0;
}

bool CAutomaticCombiner::NeedUpdate()
{
	bool changed = false;
	for ( vector< CPtr<IPart> >::iterator i = value.begin(); i != value.end(); )
	{
		if ( IsValid( *i ) ) ++i;
		else
		{
			i = value.erase( i );
			changed = true;
		}
	}
	return changed;
}
} // namespace NGScene
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02741133, CPerMaterialCombiner )
REGISTER_SAVELOAD_CLASS( 0x01091206, CAutomaticCombiner )
