#ifndef __AIPOSITION_H_
#define __AIPOSITION_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../Misc/2Darray.h"
#include "../Misc/PortablePathPlace.h"
#include "../FileIO/PortableStructureChunks.h"
namespace NAI
{
class CPathNetwork;
class CLayersGroup;
class IPathNetwork;
class IAIMap;
struct SHeightCalcInfo;
////////////////////////////////////////////////////////////////////////////////////////////////////
// place on NodesNetwork
enum ECheckMove
{
	CM_LAY = 0,
	CM_CROUCH,
	CM_STAND,
	CM_INACTIVE,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// @PDB NAI::EPassable (gen/include/s2_types.h:2367) -- the granular per-place passability verdict that
// retail's IPathNetwork::GetPassability( const SPathPlace& ) returns (path-net vtbl+0x3c). The dev
// engine collapsed this to bool IsPassable; NWorld::CDumbUnitServer::CheckPassable maps it 1:1 to
// ECanMoveRes (AIP_LOCKED->CMR_LOCKED, AIP_DOOR->CMR_DOOR, ...). Ordinal-sensitive (a jump table).
enum EPassable
{
	AIP_YES          = 0,
	AIP_NOT_PASSABLE = 1,
	AIP_CANNOT_LAY   = 2,
	AIP_LOCKED       = 3,
	AIP_DOOR         = 4,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum ETransitionType
{
	TT_NO_WAY,
	TT_SAME,
	TT_MOVE,
	TT_MOVE_DIAGONAL,
	TT_TURN,
	TT_CLIMB_1,
	TT_CLIMB_2,
	TT_CLIMB_3,
	TT_CLIMB_4,
	TT_JUMP,
	TT_JUMP_BACK,	// @PDB ordinal 10 (retail) -- a downward jump where the unit FACES away from the move
					// direction; shifts TT_POSE..TT_LADDER_MOVE to retail ordinals 11..16 (transient, not serialized).
	TT_POSE,
	TT_INTERGRID_SAME,
	TT_INTERGRID,
	TT_LADDER_UP,
	TT_LADDER_DOWN,
	TT_LADDER_MOVE
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EMoveType
{
	MT_MOVE_STAND,
	MT_MOVE_STAND_DIAG,
	MT_MOVE_CROUCH,
	MT_MOVE_CROUCH_DIAG,
	MT_MOVE_CRAWL,
	MT_MOVE_CRAWL_DIAG,
	MT_TURN,
	MT_CLIMB_1,
	MT_CLIMB_2,
	MT_CLIMB_3,
	MT_CLIMB_4,
	MT_JUMP,
	MT_POSE_WALK_CRAWL,
	MT_POSE_WALK_CROUCH,
	MT_POSE_CROUCH_CRAWL,
	MT_LADDER_UP,
	MT_LADDER_DOWN,
	MT_LADDER_MOVE,
	MT_ZERO,
	N_MOVE_TYPES
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EFindPathParams : int
{
	PF_DEFAULT = 0,
	PF_USE_DIR = 1,
	PF_USE_POSE = 2,
	PF_USE_POSEDIR = 3,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
externA5 int nMoveShift[][2];
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SPathPlace
{
private:
	std::uint32_t nData;
public:
	SPathPlace(): nData( UINT32_C(0xffffffff) & ~S2AI::kFinalMask ) {}
	SPathPlace( int _nData ): nData(static_cast<std::uint32_t>(_nData)) {}
	SPathPlace( unsigned _nX, unsigned _nY, unsigned _nLayer )
		: nData(S2AI::MakePathPlace(_nX, _nY, _nLayer, 0, 0, 0)) {}
	SPathPlace( unsigned _nX, unsigned _nY, unsigned _nLayer, unsigned _nDirection, 
		unsigned _nPose, unsigned _nMoving )
		: nData(S2AI::MakePathPlace(_nX, _nY, _nLayer, _nDirection, _nPose, _nMoving)) {}
	static SPathPlace FromBits( std::uint32_t bits ) { SPathPlace p; p.nData = bits; return p; }
	std::uint32_t GetBits() const { return nData; }
	bool IsFinal() const { return (nData & S2AI::kFinalMask) != 0; }
	int GetData() const { return S2AI::PathPlaceSigned(nData); }
	unsigned short GetLayer() const { return static_cast<unsigned short>((nData & S2AI::kLayerMask) >> 17); }
	unsigned short GetDirection() const { return static_cast<unsigned short>((nData & S2AI::kDirectionMask) >> 27); }
	unsigned short IsMoving() const { return static_cast<unsigned short>((nData & S2AI::kMovingMask) >> 26); }
	unsigned short GetPose() const { return static_cast<unsigned short>((nData & S2AI::kPoseMask) >> 30); }
	bool IsIntegral() const { return (nData & S2AI::kIntegralMask) != 0; }
	unsigned short GetX() const { return static_cast<unsigned short>(nData & S2AI::kXMask); }
	unsigned short GetY() const { return static_cast<unsigned short>((nData & S2AI::kYMask) >> 8); }
	unsigned short GetLadderStep() const { ASSERT( !IsIntegral() ); return GetY(); }
	void SetXY( unsigned int _nX, unsigned int _nY ) { nData = S2AI::SetField(nData, S2AI::kXMask, 0, _nX); SetY(_nY); }
	void SetY( unsigned int _nY ) { nData = S2AI::SetField(nData, S2AI::kYMask, 8, _nY); }
	void SetDirection( unsigned short _n ) { nData = S2AI::SetField(nData, S2AI::kDirectionMask, 27, _n); }
	void SetPose( unsigned short _n ) { nData = S2AI::SetField(nData, S2AI::kPoseMask, 30, _n); } // CM_*
	void SetLayer( unsigned _n ) { nData = S2AI::SetField(nData, S2AI::kLayerMask, 17, _n); }
	void SetIntegral( unsigned _n ) { nData = S2AI::SetField(nData, S2AI::kIntegralMask, 16, _n); }
	void SetOnLayer( int _nLayer, int _nX, int _nY, int _nIntegral = 1 ) {
		SetXY(_nX, _nY);
		nData = S2AI::SetField(nData, S2AI::kIntegralMask, 16, _nIntegral);
		nData = S2AI::SetField(nData, S2AI::kLayerMask, 17, _nLayer);
		SetPose(0);
		SetFinal(0);
	}
	void SetFinal( unsigned n ) { nData = S2AI::SetField(nData, S2AI::kFinalMask, 25, n); }
	void SetMoving( unsigned n ) { nData = S2AI::SetField(nData, S2AI::kMovingMask, 26, n); }
	bool operator==( const SPathPlace &a ) const { return nData == a.nData; }
	SPathPlace& operator=( const SPathPlace &a ) { nData = a.nData; return *this; }
};
static_assert( sizeof(SPathPlace) == 4, "game path place must remain one word" );
struct SPathPlaceHash
{
	int operator()( const SPathPlace &a ) const { return a.GetData(); }
};
// @0x00473da0 -- masked place equality: compare the packed SPathPlace bits under nMask. The SHARED 3-arg form
// the release uses everywhere; the a5dll previously had only file-local 2-arg copies (UnitTracker.cpp:90,
// aiDefenceReaction.cpp). CAICombatLogic::DoAction passes 0xc1feffff = tile+layer+POSE (0x1feffff | the pose
// bits 30-31); most other callers use 0x1feffff (tile+layer only). Pass the mask explicitly (retail has no default).
inline bool IsSamePlace( const SPathPlace &a, const SPathPlace &b, unsigned nMask )
{
	return ( ( a.GetData() ^ b.GetData() ) & (int)nMask ) == 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CTPoint<int> SPoint;
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SPosition
{
	SPathPlace p;
	CPtr<CPathNetwork> pNet;
	
	SPosition() {}
	SPosition( const SPathPlace &_p, IPathNetwork *_pNet );
	void SetNetwork( IPathNetwork *_pNet );
	IPathNetwork* GetNetwork() const { return (IPathNetwork*)pNet.GetPtr(); }
	bool IsValid() const;
	CVec2 GetCPNoHeight() const;
	CVec3 GetCP() const;
	float GetDirection() const;
	int GetFloor() const;
	int GetLayer() const { return p.GetLayer(); }
	bool operator==( const SPosition &a ) const { return p == a.p && pNet == a.pNet; }
	int operator&( CStructureSaver &f );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SObjectPosition
{
	SPosition pos;
	int nFloor;

	int operator&( CStructureSaver &f );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EPose : int	// DO NOT REORDER! This is tied to AP calculation!
{
	CRAWL = 0,	// prone (crawling)
	CROUCH,		// crouched
	WALK,		// standing
	RUN			// running
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EDirection : int
{
	RIGHT= 0, UPRIGHT, UP, UPLEFT, LEFT, DOWNLEFT, DOWN, DOWNRIGHT, NONE
};
enum EHitLocation : int
{
	HL_ANY = -1,
	HL_BODY = 0,
	HL_HEAD,
	HL_RHAND,
	HL_LHAND,
	HL_RLEG,
	HL_LLEG,
	N_HL
};
enum ETileHitLocation : int
{
	THL_LOWER = 0,
	THL_MIDDLE,
	THL_UPPER
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitPosition
{
	SPosition pos;
	bool bRun;

	bool IsValid() const;
	CVec2 GetCPNoHeight() const { return pos.GetCPNoHeight(); }
	CVec3 GetCP() const { return pos.GetCP(); }
	float GetDirection() const { return pos.GetDirection(); }
	EDirection GetDir() const { return (EDirection)( pos.p.GetDirection() ); }
	void SetPose( EPose pose );
	EPose GetPose() const;
	float GetHeight() const;
	float GetHLHeight( EHitLocation eHL ) const;
	CVec3 GetCenter() const;
	CVec3 GetEyePosition() const;
	bool operator==( const SUnitPosition &a ) const { return pos == a.pos && bRun == a.bRun; }
	int operator&( CStructureSaver &f );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EBlowHeight // Close combat
{
	BH_TOP,
	BH_MIDDLE,
	BH_BOTTOM,
};
EBlowHeight GetBlowHeight( const SUnitPosition &attackerPos, const CVec3 &ptTarget );
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SMove
{
#if defined(_WIN32)
	union
	{
		struct 
		{
			SPathPlace dest;
			EMoveType type;
		};
		struct 
		{
			SPathPlace first;
			EMoveType second;
		};
	};
#else
	// The MSVC anonymous union contains a non-trivial SPathPlace. GCC/Clang
	// reject that extension; the two aliases have identical storage and only
	// dest/type are used outside this definition.
	SPathPlace dest;
	EMoveType type;
#endif
	SMove() {};
	SMove &operator=(const SMove& src)
	{
		this->dest = src.dest;
		this->type = src.type;
#if defined(_WIN32)
		this->first = src.first;
		this->second = src.second;
#endif
		return *this;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SShowLink
{
	enum EType
	{
		ANY_MOVE,
		DIRECT,
		STAND_MOVE,
		CROUCH_MOVE,
		CRAWL_MOVE,
		HEIGHT_CHANGE,
		NEIGHBOUR_ZONE_STAND_ONLY,
		NEIGHBOUR_ZONE_ANY_MOVE,
		LAY_POSE_SHOW
	};
	CVec3 start, finish;
	EType t;
	
	SShowLink() {}
	SShowLink( const CVec3 &a, const CVec3 &b, EType _t ): start(a), finish(b), t(_t) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SShowPoint
{
	enum EFlags
	{
		CAN_STAND     = 1,
		CAN_CROUCH		= 2,
		CAN_LAY				=	4,
		EVERY_POSE = 15,
		BOW_LEGGED = 8,
		COLOR_CENTER = 16,
		LOCKED = 0x80
	};
	CVec3 pos;
	int nFlags;

	SShowPoint() {}
	SShowPoint( const CVec3 &_pos, int _nFlags ): pos(_pos), nFlags(_nFlags) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EBigLockerType
{
	BL_PANZERKLEINE,
	BL_CAR,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAlternativeGridInfo
{
	int nLayersGroup;
	vector<SAlternativeGridInfo> children;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class IPathNetwork: public CObjectBase
{
public:
	virtual int CreateLayersGroup( int nXSize, int nYSize, const CVec2 &ptOrigin, float fAngle, int nFirstFloor ) = 0;
	virtual int CreateLayer( int nXSize, int nYSize, const CVec2 &ptOrigin, float fAngle, int nFloor, CLayersGroup* _pGroup = 0 ) = 0;
	virtual EDirection GetDir( const SPathPlace &src, const SPathPlace &dst ) = 0;
	virtual EDirection GetClosestDir( const SPathPlace &src, const SPathPlace &dst ) = 0;	
	virtual EDirection GetClosestDir( int nLayer, float fAngle ) = 0;
	virtual bool SetOnLayer( SPosition *pRes, int nLayer, const CVec3 &pos ) = 0;
	virtual void SetOnLayer( SUnitPosition *pRes, int nLayer, const CVec3 &pos ) = 0;
	virtual void SetOnLayer( SObjectPosition *pRes, int nLayer, const CVec3 &pos ) = 0;
	virtual bool SetOnFloor( SPosition *pRes, int nFloor, const CVec3 &pos ) = 0;
	//virtual void GetMoves( const SPathPlace &src, bool bCheckSuicide, bool bMoveOnly, vector<SMove> *pRes ) = 0;
	virtual int GetNumLayers() const = 0;
	virtual void GetPassability( CArray2D<bool> *pRes, int nLayer ) = 0;
	virtual void GetNetworkFragment( const SPosition &pos, bool bOneColorOnly, vector<SShowPoint> *pKnots, vector<SShowLink> *pLinks ) = 0;
	virtual int GetFloor( int nLayer ) const = 0;
	virtual void CreateAlternativeGrids( const SAlternativeGridInfo &root ) = 0;
	virtual void Lock( CObjectBase *pUnit, const SPathPlace &p ) = 0;
	virtual void LockMovingObject( CObjectBase *pUnit, const SPathPlace &p1, const SPathPlace &p2 ) = 0;
	virtual void Unlock( CObjectBase *pUnit ) = 0;
	virtual void LockSelected( const list<CObjectBase*> &selected ) = 0;
	virtual void UnlockSelected() = 0;
	virtual bool IsLocked( const SPathPlace &p, bool bIgnoreBlockedDoors = false ) const = 0;
	virtual bool IsBigLockerLocked( const SPathPlace &p, EBigLockerType type ) const = 0;
	virtual CObjectBase* GetWhoLocksThisPlace( const SPathPlace &p ) const = 0;
	virtual void ClearDynamicLocks( CObjectBase *pUnit ) = 0;
	virtual void RestoreDynamicLocks( CObjectBase *pUnit ) = 0;
	virtual void ChangeDynamicLocks( CObjectBase *pUnit, const vector<SPathPlace> &points ) = 0;
	virtual bool IsValidDestination( const SPathPlace &p ) = 0;
	virtual bool IsPassable( const SPathPlace &p ) = 0;
	// @0x42e30 (retail path-net vtbl+0x3c): the granular per-place verdict. IsPassable( p ) is exactly
	// ( GetPassability( p ) == AIP_YES ). Added AFTER IsPassable rather than at the retail slot index --
	// functional rebuild, no ABI/vtable-offset constraint (CPathNetwork is the only IPathNetwork impl).
	virtual EPassable GetPassability( const SPathPlace &p ) = 0;
	virtual bool IsNativePassable( const SPathPlace &p ) = 0;
	virtual void GetNearPlaces( const SSphere &s, vector<SPathPlace> *pRes, bool bTakeAll = false ) = 0;
	//virtual void ForceLayersRecalc( const SSphere &s ) = 0;
	//virtual ETransitionType GetTransitionType( const SPathPlace &from, const SPathPlace &to ) = 0;
	// retail @0x40290: WORLD-space ladder record (bottom point + a point one grid-step away in the
	// climb direction); the network resolves the owning layers group, tile coords and rotation
	virtual void CreateLadder( const CVec2 &ptPos, const CVec2 &ptUpperPos, int nHeight, int nFloor ) = 0;
	virtual bool UpdateColouring( const vector<SPathPlace> &newLockers ) = 0;
	virtual void FormationMoveTo( vector<SPosition> *pPlaces, const SPosition &to ) = 0;	
	virtual void FlipperOpenClose( CObjectBase* flipper, bool bOpen ) = 0;
	virtual void LockUnlockFlipper( CObjectBase* flipper, bool bLock ) = 0;
	virtual void Freeze( bool bFreeze ) = 0;
	virtual SPathPlace GetDeployPlace( const SPathPlace &start, int nDisplacement ) = 0;
	virtual void GetLockArea( vector<SPathPlace> *pRes, const SPathPlace &p, bool bBigUnit ) const = 0;
	// true while any layer group still has a pending recolour/pass-calc job (PassCalcerIsActive /
	// WaitForPassCalc). Retail CPathNetwork::HasPassCalcerJobs @0x3e4a0.
	virtual bool HasPassCalcerJobs() const { return false; }
	// read-and-clear "grid changed since last query". Retail CPathNetwork::CheckUpdated @0x4c9a0
	// (IAIMap vtbl); retail consumer = CWorld::Segment @0x36bce0, which broadcasts the deferred
	// TBS_GRID_INFO_UPDATED (GridInfoUpdated @0x375ca0) when it returns true. Added like
	// HasPassCalcerJobs: functional rebuild, no vtable-offset constraint.
	virtual bool CheckUpdated() { return false; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class IAIJobManager;
IPathNetwork* CreateNodesNetwork( IAIMap *pMap, IAIJobManager *pJobManager );
// Snap *pOut onto layer 0 near *pSrc's world position, then repurpose its nLayer field to hold the
// altitude above that ground cell (in F_3D_STEP units, clamped 0..255) and flag it as a 3D/fly
// position (nFinal). Used by UnitFlyToWaypoint -> CCmdFly and 3D-waypoint load.
// Retail NAI::MakeFlyPos @0x3d0e0 (worker: raw world point) / @0x3d180 (wrapper: from *pSrc's CP).
bool MakeFlyPos( const CVec3 &pt, IPathNetwork *pNet, SPosition *pOut );
bool MakeFlyPos( SPosition *pSrc, SPosition *pOut );
////////////////////////////////////////////////////////////////////////////////////////////////////
// aiPositionDebug -- locker-validity probes for CPathNetwork::DebugCheck (retail @0x917d0 / @0x91810);
// defined in aiPositionDebug.cpp. Caller absent in dev -> reconstructed but unwired (parity surface).
bool IsLockerUnit( CObjectBase *pUnit );                 // live CUnitServer whose IsLocker() holds
bool IsUnitNear( CObjectBase *pUnit, const CVec3 &pt );  // center within 2 (4 big-locker) grid steps of pt
////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace S2FileIO {
template<>
struct StructureFieldCodec<NAI::SPathPlace, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 4;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NAI::SPathPlace* value) {
    if (!value) return false;
    std::uint32_t bits = 0;
    if (!S2AI::DecodePathPlace(source, length, &bits)) return false;
    *value = NAI::SPathPlace::FromBits(bits);
    return true;
  }
  static bool Encode(const NAI::SPathPlace& value,
                     std::uint8_t* destination, std::size_t length) {
    return S2AI::EncodePathPlace(value.GetBits(), destination, length);
  }
};
} // namespace S2FileIO
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
