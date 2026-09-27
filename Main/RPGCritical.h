#ifndef __RPGCRITICAL_H_
#define __RPGCRITICAL_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RPGUnit.h"
#include "RPGUnitInfo.h"
#include "../FileIO/PortableCritical.h"
#include <cstdio>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAI
{
	enum EHitLocation : int;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NRPG
{
////////////////////////////////////////////////////////////////////////////////////////////////////
class CUnit;
class IUnitMission;
class CSkillModifier;
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SCritical
{
	int   nDC; // Difficulty Class
	int   nDuration;
	float fValue;
	NDb::ECritical eCritical;
	NDb::ECriticalLocation eCl;

	SCritical() {}
	SCritical( NDb::ECriticalLocation hl, NDb::ECritical cr, int _nDuration = -1, float _fValue = 0, int _nDC = N_MAX_DC )
		: eCl(hl), eCritical(cr), nDuration(_nDuration), fValue(_fValue), nDC(_nDC) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class CCritical: public ICriticalInfo
{
	//OBJECT_NOCOPY_METHODS(CCritical);

protected:
	void PushModifier( CSkillModifier *pModifier ) { modifiers.push_back( pModifier ); }

private:
	ZDATA
	int nTurn;
	vector<CObj<CSkillModifier> > modifiers;   // retail PDB: OWNING refs (the skill holds the weak CPtr side)
protected:
	SCritical critical;

public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nTurn); f.Add(3,&modifiers); f.Add(4,&critical); return 0; }

	CCritical() {}
	CCritical( const SCritical &crit );

	// false if this critical needs no cancellation (no modifiers were applied)
	virtual bool SetModifiers( CUnit *pRPGUnit, IUnitMission *pRPGMission  )
	{
		#if defined(_WIN32)
		OutputDebugString( "Empty critical\n" );
		#else
		std::fputs( "Empty critical\n", stderr );
		#endif
		return false;
	}
	virtual void RemoveModifiers() { modifiers.clear(); }
	virtual bool CanBeSuspended() = 0;
	virtual bool CanBeMerged() const { return true; }

	enum { WEAKER, MERGED, OTHER }; // results of the Merge function
	int Merge( CCritical *pCritical ) const;

	bool NextTurn();		// true while this critical's duration has not expired

	virtual int  GetRemainingTime() const;
	virtual bool IsTemporarily() const { return critical.nDuration >= 0; }
	float GetModifier() const;
	const SCritical& GetCritical() const { return critical; }

	virtual float GetValue() const { return critical.fValue; } // release @0x695fb0
	virtual int GetDifficultyClass() const { return critical.nDC; }
	virtual NDb::ECritical GetCriticalType() const { return critical.eCritical; }
	virtual NDb::ECriticalLocation GetCriticalLocation() const { return critical.eCl; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
CCritical* CreateCritical( const SCritical &critical );
////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static_assert(sizeof(NRPG::SCritical) == 20, "game critical record must remain 20 bytes");
namespace S2FileIO {
template<>
struct StructureFieldCodec<NRPG::SCritical, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 20;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NRPG::SCritical* value) {
    if (!value) return false;
    CriticalFields fields{};
    if (!DecodeCritical(source, length, &fields) ||
        fields.critical < NDb::C_DEATH || fields.critical > NDb::N_CRIT_TYPES ||
        fields.location < NDb::CL_HEAD || fields.location > NDb::N_CL) return false;
    value->nDC = fields.difficulty;
    value->nDuration = fields.duration;
    value->fValue = fields.value;
    value->eCritical = static_cast<NDb::ECritical>(fields.critical);
    value->eCl = static_cast<NDb::ECriticalLocation>(fields.location);
    return true;
  }
  static bool Encode(const NRPG::SCritical& value,
                     std::uint8_t* destination, std::size_t length) {
    const CriticalFields fields{value.nDC, value.nDuration, value.fValue,
                                static_cast<std::int32_t>(value.eCritical),
                                static_cast<std::int32_t>(value.eCl)};
    return EncodeCritical(fields, destination, length);
  }
};
} // namespace S2FileIO
#endif //__RPGCRITICAL_H_
