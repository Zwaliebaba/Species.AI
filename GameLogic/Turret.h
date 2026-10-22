#pragma once

#include "Structure.h"
#include "Vec3.h"
#include "Weapons.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// R-BLDG-410 steps 2 to 6 for a built turret (ADR-133), which run before its base advance evaluates its ports, so that
// they read the occupants of the tick before. Its first advance draws its first idle aim point (R-BLDG-412). Its
// ownership evaluation runs on its first advance and every ref.bldg.turret.recount_ticks after (R-BLDG-408); a change of
// team clears its target. While it has a team: its fire timer counts down; its target search runs on the first advance
// with a team and every ref.bldg.turret.recount_ticks after (R-BLDG-411 to R-BLDG-413); its aim follows a live target
// with the wobble (R-BLDG-414); it fires when the target is within the fire cone, and hoses at its last aim point after
// it (R-BLDG-415); and its aim point is clamped and its head and barrels turn (R-BLDG-416).
void AdvanceTurret(RegionSimulation& _region, Structure& _structure);

// R-BLDG-413: the line of sight from _from to _to, sampled every ref.bldg.turret.los_step, meets no structure other
// than _turret in the obstacle-grid cells of its samples.
[[nodiscard]] bool TurretSightClear(const RegionSimulation& _region, const Structure& _turret, const Neuron::Vec3& _from,
                                    const Neuron::Vec3& _to);

// R-BLDG-416 (ADR-133): the barrels' front, the head's front raised to the elevation of the barrels' up vector.
[[nodiscard]] Neuron::Vec3 BarrelFront(const TurretState& _turret) noexcept;
// R-BLDG-418 and DEC-461: barrel _barrel's muzzle in the world, its muzzle socket carried by the barrels' pitch to the
// head's barrel mount and by the head's turn to the turret's position; the position itself without the records.
[[nodiscard]] Neuron::Vec3 MuzzleOf(const Structure& _structure, const StructureRecords* _records, std::uint32_t _barrel);

// R-BLDG-422, R-BLDG-423 and MVP-034: a built turret's health changes by the signed _amount; at 0 or below it is
// destroyed, its head and barrels shatter, and it stays as a construction site of its settlement that is not queued,
// costing mvp.cost.turret_rebuild_fraction of mvp.cost.turret_ru to rebuild (ADR-133). Every other structure, and a
// turret that is a construction site, takes no damage.
void DamageTurret(RegionSimulation& _region, Structure& _structure, std::int32_t _amount);

// R-CRATE-405, fire a turret round: a round of _team from _position with _velocity for ref.weapon.turret_round.life_ticks,
// fired from the barrel _barrel of the turret _source, whose muzzle is at _muzzle (R-BLDG-420).
void FireTurretRound(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity,
                     StableId _source, const Neuron::Vec3& _muzzle, std::uint32_t _barrel);
// R-CRATE-451: one advance of a turret round; true when it asks to be removed.
[[nodiscard]] bool AdvanceTurretRound(RegionSimulation& _region, Effect& _round);

} // namespace Species
