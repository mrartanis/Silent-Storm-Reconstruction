#pragma once
#include "RPGUnit.h"
#include "../FileIO/PortableStructureChunks.h"

namespace NWorld
{
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Retail-new (wExplosionPerks.obj): per-unit explosive-perk modifiers.
// Filled from the placing/throwing unit's RPG perks, then handed to the mine /
// grenade / explosion code via SetPerkModifiers. The owning unit must be valid
// (alive / not scheduled for deletion); dead or missing units leave the struct
// untouched. Callers seed sensible defaults (e.g. SPerkMineModifiers{1,1,false})
// before Fill, which only overrides on a present perk. Perk ids:
//   0x30 -> structure-damage modifier, 0x35 -> always-human-critical flag,
//   0x5e -> area-effect-damage modifier.
struct SPerkMineModifiers
{
	float fStructureDmgModifier;
	float fAEDmgModifier;
	bool  bAlwaysHumanCritical;

	// neutral defaults: multipliers of 1 (no change), no forced crit. Fill() only overrides on a present perk, so a
	// thrower with no explosive perks (or a thrower-less environmental blast) leaves the explosion damage untouched.
	SPerkMineModifiers() : fStructureDmgModifier( 1.0f ), fAEDmgModifier( 1.0f ), bAlwaysHumanCritical( false ) {}
	void Fill( NRPG::CUnit *pUnit );
};
////////////////////////////////////////////////////////////////////////////////////////////////////
}  // namespace NWorld

namespace S2FileIO {
template<>
struct StructureFieldCodec<NWorld::SPerkMineModifiers, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 12;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     NWorld::SPerkMineModifiers* value) {
    if (!value) return false;
    StructureExplosionPerkFields fields;
    if (!DecodeStructureExplosionPerks(source, length, &fields)) return false;
    value->fStructureDmgModifier = fields.structureDamage;
    value->fAEDmgModifier = fields.areaDamage;
    value->bAlwaysHumanCritical = fields.alwaysHumanCritical;
    return true;
  }
  static bool Encode(const NWorld::SPerkMineModifiers& value,
                     std::uint8_t* destination, std::size_t length) {
    StructureExplosionPerkFields fields;
    fields.structureDamage = value.fStructureDmgModifier;
    fields.areaDamage = value.fAEDmgModifier;
    fields.alwaysHumanCritical = value.bAlwaysHumanCritical;
    return EncodeStructureExplosionPerks(fields, destination, length);
  }
};
}  // namespace S2FileIO
