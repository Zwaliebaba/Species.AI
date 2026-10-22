#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

class RegionSimulation;
struct Settlement;

/// The scripted steward of M2 (the M2 plan §5, ADR-138): the economy half of an AI settlement (MVP-056), which runs one
/// settlement as its team's AI commander. It reads only what the steward's client could see, its own settlement's status,
/// its territory, its own structures, the terrain and the public resource sites, and acts only through the region's
/// command queue, so what it decides executes at the next tick (MVP-094). Region state.
struct ScriptedSteward
{
  StableId id = 0; // its sender id, from the region's counter
  std::uint8_t team = 0;
  std::int32_t thinkTicks = 0;    // to its next look, every mvp.steward.think_s
  std::uint64_t nextSequence = 0; // its commands' sequence numbers (ADR-095)
};

// At the end of the region's tick, after the settlements' own step: every mvp.steward.think_s, a storehouse and then a
// dwelling, each when none of its kind is queued and the stores hold its cost:
// - a storehouse at the first place, the midpoint of two neighbouring outer home resource sites that the territory holds,
//   or the nearest place about it, mvp.steward.storehouse_search_step apart in up to mvp.steward.storehouse_search_rings
//   rings of eight, that no storehouse of its own already serves;
// - a dwelling, once the population is within mvp.steward.dwelling_margin of its limit, at the first place of its
//   layout: rings mvp.steward.ring_step apart from mvp.steward.ring_first, up to mvp.steward.rings_max, with places
//   mvp.steward.slot_spacing apart from the side away from the valley's centre, none within
//   mvp.steward.lane_half_width of the line from the Wellspring to a home resource site of its own.
// A place is the first that every check of a placement passes (CheckPlacement). The priorities stay MVP-036's defaults.
void StepScriptedSteward(RegionSimulation& _region, ScriptedSteward& _steward);

// The places of the dwellings' layout about the settlement's Wellspring, in the order they are tried.
[[nodiscard]] std::vector<Neuron::Vec3> DwellingLayout(const RegionSimulation& _region, const Settlement& _settlement);
// The places the storehouses are sought about: the midpoints of the neighbouring outer home resource sites of the
// settlement's site, each with whether its territory holds both.
struct StorehouseSpot
{
  Neuron::Vec3 position;
  bool held = false;
};
[[nodiscard]] std::vector<StorehouseSpot> StorehouseSpots(const RegionSimulation& _region, const Settlement& _settlement);

// The scripted steward canonically (ADR-081 item 7), and the values the writer wrote; false when they do not read whole.
void WriteScriptedSteward(Neuron::BitWriter& _writer, const ScriptedSteward& _steward);
[[nodiscard]] bool ReadScriptedSteward(Neuron::BitReader& _reader, ScriptedSteward& _steward);

} // namespace Species
