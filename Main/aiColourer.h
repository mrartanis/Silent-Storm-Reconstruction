#ifndef _AI_MAP_COLOURER_H
#define _AI_MAP_COLOURER_H

#include "..\Misc\2DArray.h"
#include "aiMapProxy.h"
#include "aiWaveSearch.h"
#include "aiSimpleWaveSearch.h"
#include "../Misc/PortableZone.h"
#include "../FileIO/PortableStructureChunks.h"
#include "../FileIO/PortableAIColourWire.h"
#include "../FileIO/PortableAIDistanceWire.h"

namespace NAI
{
enum ESpecialColor
{
	EC_LADDER_COLOR = 32000,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
struct SRectColorConstraint
{
	T x1, x2, y1, y2;
	WORD wColor;
	CArray2D<WORD> *pColors;
	typedef CTPoint<T> SPosition;
	SRectColorConstraint( T xmin, T ymin, T xmax, T ymax, CArray2D<WORD> *_pColors = 0, WORD _wColor = 65535 ): 
		x1( xmin ), x2( xmax ), y1( ymin ), y2( ymax ), pColors(_pColors), wColor( _wColor ) {}
	bool operator()( const SPosition &dst ) const
	{ 
		bool bInRect = (dst.x >= x1) && (dst.x < x2) && (dst.y >= y1) && (dst.y < y2); 
		if ( !bInRect )
			return false;
		if ( wColor == 65535 ) 
			return true;
		return (*pColors)[ dst.y ][ dst.x ] == wColor;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef	CWaveCountCosts<CNodesLayerProxy, CTPoint<unsigned char>, WORD, 
	SRectColorConstraint<unsigned char>, CSquareMapCosts, 250, 16>
	CWaysCounter;
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SLocalColorInfo
{
	WORD wAverageX;
	WORD wAverageY;
	SLocalColorInfo(): wAverageX(0), wAverageY(0) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SNeighbour
{
	WORD wNodeNumber;
	WORD wDistance;
	int nLayer;  
	SNeighbour() {} 
	SNeighbour( WORD _wNode, int _nDistance, int _nLayer ): 
		wNodeNumber(_wNode), wDistance(_nDistance), nLayer(_nLayer) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SNetNode
{
	ZDATA
	list<SNeighbour> neighbours;
	WORD wColor;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&neighbours); f.Add(3,&wColor); return 0; }
	void AddNeighbour( WORD wNode, int wDistance, int nLayer );
	void Clear() { neighbours.clear(); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SNet 
{
	ZDATA
	vector<SNetNode> nodes;
	vector<char> ladderConsistent;
	vector<SNeighbour> ladderUpperPoints;
	vector<SNeighbour> ladderLowerPoints;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nodes); f.Add(3,&ladderConsistent); f.Add(4,&ladderUpperPoints); f.Add(5,&ladderLowerPoints); return 0; }
	SNet( int nNodes ): nodes(nNodes) {}
	SNet() {}
	void Clear();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SZone
{
	WORD wColor;
	int nLayer;
	SZone(): wColor(65535), nLayer(0) {}
	SZone(WORD _wColor, int _nLayer): wColor(_wColor), nLayer(_nLayer) {}
	SZone(const CPathNetwork* pNet, const SPathPlace& point);
	void MakeNull() { wColor = 65535; }
	CVec3 GetCP( const CPathNetwork *pNet  ) const;
	CTPoint<unsigned char> GetCenter( const CPathNetwork *pNet ) const;
	bool IsNull() const { return wColor == 65535; }
	bool operator==(const SZone& zone) const { return ( zone.nLayer == nLayer)&&( zone.wColor == wColor ); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SZoneHash
{
	int operator()( const SZone &a ) const { return a.wColor + ( a.nLayer << 8 ); }
	bool operator()( const SZone &a, const SZone& b ) const 
	{ 
		return ( a.wColor == b.wColor )&&( a.nLayer == b.nLayer ); 
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
//  , ,   CUpperNetWay ,   
typedef list<SZone> CUpperNetWay;
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDistanceInfo
{
	WORD distance;
	SZone parent;
	// list info
	bool isProceeded;
	bool isInList;
	SZone prev;
	SZone next;

	SDistanceInfo(): isInList(false), isProceeded(false), distance(65535) {}
};
typedef unordered_map<SZone, SDistanceInfo, SZoneHash> CZonesToDistInfo;
////////////////////////////////////////////////////////////////////////////////////////////////////
class CMapColourer : public CObjectBase
{
	OBJECT_BASIC_METHODS(CMapColourer);
	ZDATA
	CArray2D<WORD> pointColors;
	CSquareMapCosts pointDistancesStandOnly;
	CSquareMapCosts pointDistancesAnyMove;
	vector<WORD> localColorRedirects; // parent colour of given local
	vector<SLocalColorInfo> localColorInfos; // some info about given color
	vector<int> localColorCounts; // areas (sizes) of given color
	unsigned nLocalColors; // counter
	SNet zonesStandOnly, zonesAnyMove; // global maps
	int nLayer; // number of this layer
	CObj<CNodesLayerProxy> pMap;
public:
	vector< CTRect<unsigned char> > zonesToRecalc;	
private:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pointColors); f.Add(3,&pointDistancesStandOnly); f.Add(4,&pointDistancesAnyMove); f.Add(5,&localColorRedirects); f.Add(6,&localColorInfos); f.Add(7,&localColorCounts); f.Add(8,&nLocalColors); f.Add(9,&zonesStandOnly); f.Add(10,&zonesAnyMove); f.Add(11,&nLayer); f.Add(12,&pMap); f.Add(13,&zonesToRecalc); f.Add(14,&bAlreadyConstructed); f.Add(15,&bTransitionsAttached); return 0; }
	bool bAlreadyConstructed = false;
	bool bTransitionsAttached = false;

	void CreateZonesMap( SNet *pZones, vector<CNodesLayer::SLadder> *pLadders );
	void Get8x8SquareOfColor( WORD wColor, unsigned char *pX, unsigned char *pY );
	bool IsColorInArea( WORD wColor, const vector< CTRect<unsigned char> > &areas );
	bool IsColorTouchingArea( WORD wColor, const vector< CTRect<unsigned char> > &areas );

	int CountDistance(int X1, int Y1, int X2, int Y2) const;
	void ClearColor( EPathfinderMode mode, CPathNetwork* pNet, WORD wColor, int nLayer ); 
	WORD GetNextEmptyColor( WORD wCurrent ); 
	void InvestigateSquareMap( int nMinX, int nMinY, int nMaxX, int nMaxY );
	void InvestigateSquareMap( const vector< CTRect<unsigned char> > &areas );
	
	// returns the way length from point to its color center
	WORD GetDistanceToCenter( unsigned char cX, unsigned char cY, EPathfinderMode mode );
	void AttachLadderTransition( CPathNetwork* pNet, int nLadder, const CNodesLayer::SLadder &ladder );
	void AttachColourers( unsigned char cX, unsigned char cY, CMapColourer* pOther,
		unsigned char cOtherX, unsigned char cOtherY, unsigned char cTransitionCost, EPathfinderMode mode );
	void AttachTransitions( CPathNetwork* pNet,
		const SPathPlace &p, const CNodesLayer::STransitionSet &trSet, EPathfinderMode mode );
	void AttachSameTransitions( CPathNetwork* pNet,
		const SPathPlace &search, EPathfinderMode mode );
	void CalcDistanceTableForColor( WORD wCurrColor, EPathfinderMode mode, CWaysCounter *counter );
	void FindAdjacentColours( unsigned char cMinX,  unsigned char cMinY, unsigned char cMaxX, unsigned char cMaxY, 
		EPathfinderMode mode, vector<CNodesLayer::SLadder> *pLadders  );
	void RecalcTransitions( CPathNetwork* pNet, EPathfinderMode mode );

	friend class CLayerColorConstraints;
	friend class CPathNetwork;
	friend bool CalcWay( CPathNetwork *pPathNet, CUpperNetWay *pWay, const SZone &src, 
		const SZone &dst, bool bStandOnly, SZone *pStart, CZonesToDistInfo *pDistances );

public:
	
	CMapColourer() : nLocalColors(0) {}
	void SetLayer(CNodesLayer* _pLayer, IAIMap* _pMap, int _nLayer) 
	{ 
		pMap = new CNodesLayerProxy(_pLayer);
		nLayer = _nLayer; 
	}
	void ConstructColouring( CPathNetwork* pNet );
	void AttachTransitions(	CPathNetwork* pNet );
	void AddZoneToRecalc( const CTRect<unsigned char> &rect ) 
	{ 
		zonesToRecalc.push_back( rect ); 
	}
	void RecalcColouring( CPathNetwork *pNet, int nCurrentLayer );
	void RecalcTransitions( CPathNetwork* pNet );
	bool IsReady() { return nLocalColors!=0; }  // Map was already investigated
	WORD GetPointColor( unsigned char cX, unsigned char cY ) { return pointColors[ cY ][ cX ]; }
	void GetColorCenter( WORD wColor, unsigned char *cX, unsigned char *cY )
	{
		*cX = (unsigned char)localColorInfos[ wColor ].wAverageX;
		*cY = (unsigned char)localColorInfos[ wColor ].wAverageY;
	}
	void GetColorNeighbourCenters( CPathNetwork *pNet, WORD wColor, vector<unsigned char> *pXs,
	  vector<unsigned char> *pYs, vector<int> *pLayers, EPathfinderMode mode );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CColouredWaysCalcer : public CObjectBase
{
	OBJECT_BASIC_METHODS(CColouredWaysCalcer);
	ZDATA
	// are cached values correct?
	bool bMustChange;
	SZone prevSrc;
	bool bPrevStandOnly;
	CPtr<CPathNetwork> pPrevNet;
	// cached values
	CZonesToDistInfo distances;
	SZone start;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&bMustChange); f.Add(3,&prevSrc); f.Add(4,&bPrevStandOnly); f.Add(5,&pPrevNet); f.Add(6,&distances); f.Add(7,&start); return 0; }
public:
	void ForceUpdate() { bMustChange = true; }
	void CalcBestWays( CPathNetwork *pPathNet, CLayerColorConstraints *pWays, const SZone &src, 
		const vector<SZone> &dst, const vector<CVec3> &dstCP, bool bStandOnly );
	CColouredWaysCalcer::CColouredWaysCalcer() : bMustChange( true ) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}
static_assert(sizeof(NAI::SZone) == 8, "game zone must keep its 8-byte wire layout");
static_assert(sizeof(NAI::SDistanceInfo) == 32 &&
              offsetof(NAI::SDistanceInfo, distance) == 0 &&
              offsetof(NAI::SDistanceInfo, parent) == 4 &&
              offsetof(NAI::SDistanceInfo, isProceeded) == 12 &&
              offsetof(NAI::SDistanceInfo, isInList) == 13 &&
              offsetof(NAI::SDistanceInfo, prev) == 16 &&
              offsetof(NAI::SDistanceInfo, next) == 24,
              "AI zone distance wire layout");
static_assert(sizeof(NAI::SNeighbour) == 8 && sizeof(NAI::SLocalColorInfo) == 4,
              "game colour network wire sizes");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NAI::SNeighbour, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAI::SNeighbour* value) {
    if (!value) return false;
    NeighbourFields fields{};
    if (!DecodeNeighbour(source, length, &fields)) return false;
    value->wNodeNumber = fields.node;
    value->wDistance = fields.distance;
    value->nLayer = fields.layer;
    return true;
  }
  static bool Encode(const NAI::SNeighbour& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeNeighbour(NeighbourFields{
        value.wNodeNumber, value.wDistance, value.nLayer}, destination, length);
  }
};
template<>
struct StructureFieldCodec<NAI::SLocalColorInfo, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 4;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAI::SLocalColorInfo* value) {
    if (!value) return false;
    LocalColorFields fields{};
    if (!DecodeLocalColor(source, length, &fields)) return false;
    value->wAverageX = fields.averageX;
    value->wAverageY = fields.averageY;
    return true;
  }
  static bool Encode(const NAI::SLocalColorInfo& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeLocalColor(LocalColorFields{
        value.wAverageX, value.wAverageY}, destination, length);
  }
};
template<>
struct StructureFieldCodec<NAI::SZone, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 8;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAI::SZone* value) {
    if (!value) return false;
    std::uint16_t color = 0;
    std::int32_t layer = 0;
    if (!S2AI::DecodeZone(source, length, &color, &layer)) return false;
    value->wColor = color;
    value->nLayer = layer;
    return true;
  }
  static bool Encode(const NAI::SZone& value,
                     std::uint8_t* destination, std::size_t length) {
    return S2AI::EncodeZone(value.wColor, value.nLayer, destination, length);
  }
};
template<>
struct StructureFieldCodec<NAI::SDistanceInfo, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 32;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAI::SDistanceInfo* value) {
    if (!value) return false;
    AIDistanceFields fields{};
    if (!DecodeAIDistance(source, length, &fields)) return false;
    value->distance = fields.distance;
    value->parent.wColor = fields.parent.color;
    value->parent.nLayer = fields.parent.layer;
    value->isProceeded = fields.proceeded != 0;
    value->isInList = fields.inList != 0;
    value->prev.wColor = fields.previous.color;
    value->prev.nLayer = fields.previous.layer;
    value->next.wColor = fields.next.color;
    value->next.nLayer = fields.next.layer;
    return true;
  }
  static bool Encode(const NAI::SDistanceInfo& value,
                     std::uint8_t* destination, std::size_t length) {
    return EncodeAIDistance(AIDistanceFields{
        value.distance,
        {value.parent.wColor, value.parent.nLayer},
        static_cast<std::uint8_t>(value.isProceeded ? 1 : 0),
        static_cast<std::uint8_t>(value.isInList ? 1 : 0),
        {value.prev.wColor, value.prev.nLayer},
        {value.next.wColor, value.next.nLayer}}, destination, length);
  }
};
} // namespace S2FileIO
#endif
