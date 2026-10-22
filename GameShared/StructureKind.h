#pragma once

#include <cstdint>

namespace Species
{

// The MVP's structure kinds (MVP-021, MVP-030, MVP-034), a state and wire value that is never renumbered.
enum class StructureKind : std::uint8_t
{
  Wellspring = 0,
  ResourceSite = 1,
  ResourceSiteCentral = 2,
  Dwelling = 3,
  Storehouse = 4,
  Turret = 5 // the reference gun turret (R-BLDG-405, ADR-133)
};

inline constexpr std::uint64_t STRUCTURE_KIND_MAX = 5;

// Why a placement is refused (MVP-032, MVP-101), checked in this order; the reason of a placement's command outcome
// (ADR-136), a wire value that is never renumbered.
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

inline constexpr std::uint8_t PLACEMENT_REFUSAL_MAX = static_cast<std::uint8_t>(PlacementRefusal::CannotAfford);

} // namespace Species
