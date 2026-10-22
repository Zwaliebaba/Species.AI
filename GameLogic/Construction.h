#pragma once

#include "Settlement.h"
#include "Structure.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <expected>

namespace Species
{

class RegionSimulation;

// Why a placement is refused (MVP-032, MVP-101), checked in this order; a wire value from M2 task T8 that is never
// renumbered.
enum class PlacementRefusal : std::uint8_t
{
  NotBuildable = 1,     // no settlement at the site, or a kind the steward does not place
  QueueFull = 2,        // mvp.construction.queue_max construction sites are queued
  OutsideTerritory = 3, // the structure does not lie wholly inside the territory
  InCenterRing = 4,     // the structure reaches into the core's centre ring
  NotWalkable = 5,      // its position or a port of it stands on ground that is wet or too steep
  Overlaps = 6,         // its footprint meets another structure's, or a port and a footprint meet
  CannotAfford = 7      // the stores do not hold its cost
};

// MVP-034: a kind's cost in thousandths of a resource unit; a dwelling's grows by mvp.cost.dwelling_escalation for every
// dwelling the settlement already has, built or queued; a storehouse costs mvp.cost.storehouse_ru and a turret
// mvp.cost.turret_ru.
[[nodiscard]] std::int64_t CostMilliRu(const RegionSimulation& _region, const Settlement& _settlement, StructureKind _kind);

// MVP-032 and MVP-033 (ADR-132): the steward's settlement at _site places a dwelling, a storehouse or a turret (ADR-133)
// centred at (_x, _z), facing away from its Wellspring. A placement that every check passes pays the cost from the stores, inserts the
// structure as a construction site and queues it; returns its slot, or the first refusal.
[[nodiscard]] std::expected<std::uint32_t, PlacementRefusal> PlaceStructure(RegionSimulation& _region, std::uint8_t _site,
                                                                            StructureKind _kind, double _x, double _z);

// MVP-034 and ADR-133: the settlement at _site rebuilds the destroyed turret whose site holds _slot with stable id _id:
// it pays the site's cost, mvp.cost.turret_rebuild_fraction of mvp.cost.turret_ru, and queues it. NotBuildable when
// there is no settlement at the site or no turret site of its own there that is not queued, then QueueFull and
// CannotAfford as for a placement.
[[nodiscard]] std::expected<void, PlacementRefusal> RebuildTurret(RegionSimulation& _region, std::uint8_t _site, std::uint32_t _slot,
                                                                  StableId _id);

// A construction site is in its settlement's queue (MVP-032).
[[nodiscard]] bool Queued(const RegionSimulation& _region, const Structure& _structure);

// MVP-033: the Worker-ticks a construction site needs, mvp.construction.work_per_ru Worker-seconds for each resource
// unit of its cost at ref.sim.tick_period, rounded once.
[[nodiscard]] std::int64_t RequiredWorkTicks(const Structure& _structure, const TuningMvp& _tuning) noexcept;

// The built, active structures of _kind that belong to _team: the dwellings that raise the population limit and the
// storehouses that add to the stores' capacity (MVP-022, MVP-034).
[[nodiscard]] std::uint32_t ActiveCount(const RegionSimulation& _region, std::uint8_t _team, StructureKind _kind);

// MVP-032 until M2 task T6 builds the territory's bounds: a disc of _radius at _point lies wholly inside the disc of the
// territory's radius about the settlement's Wellspring (ADR-132).
[[nodiscard]] bool InTerritory(const RegionSimulation& _region, const Settlement& _settlement, const Neuron::Vec3& _point, double _radius);

// A dwelling's and a storehouse's ports are construction ports, which a Worker mans only while it is a construction
// site; the Wellspring's, the resource sites' and the turret's are manned always (MVP-031, MVP-033).
[[nodiscard]] bool PortsMannable(const Structure& _structure) noexcept;

} // namespace Species
