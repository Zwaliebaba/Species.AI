#pragma once

#include "TuningMvp.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Performance
{

// The travel scene of the M1 plan (T10, WLD-100, O71): a crowd walks each edge of the valley's travel network.
inline constexpr std::uint32_t TRAVEL_WORKERS = 200;     // the crowd on each edge
inline constexpr double TRAVEL_SPREAD = 60.0;            // wu: the radius it spawns in around the edge's first end
inline constexpr double TRAVEL_SELECT_RADIUS = 400.0;    // wu: the selection circle that takes the whole crowd
inline constexpr double TRAVEL_MIN_DISTANCE_SHARE = 0.5; // a walk shorter than this share of the edge is not one

/// One edge of the travel network inside the valley, walked by a crowd (ADR-110): its ends' straight distance, and for
/// each Worker that arrived the factor of its walking time × its speed over the straight distance it covered, sorted.
struct TravelEdge
{
  std::uint64_t seed = 0;
  std::string name;      // "site 0 - site 1", "site 2 - centre"
  double distance = 0.0; // wu, horizontal, between the edge's ends
  std::uint32_t workers = 0;
  std::vector<double> factors;
  std::uint64_t figureTicks = 0;       // the figures the steps advanced, summed over the ticks walked
  std::int64_t advanceNanoseconds = 0; // their advances' time, read from the marker sim.teams.advance_ungrouped
};

// The edges of WLD-100 inside the valley of _seed, as the terrain's check 8 lists them (the four valley neighbours, then
// each site with the central resource site), at most _maxEdges of them. Each crowd takes the steward's selection and
// crowd order at the first tick, its recorded command stream, and walks for at most _maxTicks ticks.
[[nodiscard]] std::vector<TravelEdge> MeasureTravel(std::uint64_t _seed, const Species::TuningMvp& _tuning, std::size_t _maxEdges,
                                                    std::uint64_t _maxTicks);

} // namespace Performance
