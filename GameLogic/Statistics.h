#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Settlement.h"

#include <cstdint>
#include <vector>

namespace Species
{

class RegionSimulation;

// MVP-064: what an entry of the server's statistics records; a state value that is never renumbered. Values 1 to 3 are
// the growth steps of the M2 plan's §9, question 1; from M3 task T8 (ADR-163) the halfway lines, grievances, claims and
// battles follow.
enum class StatisticKind : std::uint8_t
{
  Sample = 0,             // every mvp.stats.sample_minutes of world time
  StructureCompleted = 1, // a structure of the settlement was built; the detail is its kind
  PopulationStep = 2,     // the population reached a new multiple of mvp.stats.population_step; the detail is that multiple
  ResourceSiteTaken = 3,  // the territory took in a resource site it did not hold; the detail is the site's index
  HalfwayReached = 4,     // the territory first reached a valley border pair's halfway line; the detail is the pair
  GrievanceAdded = 5,     // the settlement's Cladan took a new grievance; the detail is its cause
  ClaimMade = 6,          // the settlement claimed an area; the detail is the area
  ClaimWithdrawn = 7,     // the settlement withdrew its claim; the detail is the area
  ClaimLost = 8,          // the settlement lost its claim's battle; the detail is the area
  ClaimWon = 9,           // the settlement won its claim's battle, or its claim held the central site; the detail is the area
  BattleEnded = 10        // a battle the settlement was a side of ended; the detail is its result's place in the results
};

inline constexpr std::uint64_t STATISTIC_KIND_MAX = 10;
// The kinds a region of a state version before 15 holds, and so the bound its entries were written with.
inline constexpr std::uint64_t STATISTIC_KIND_MAX_BEFORE_RENOWN = 3;

/// One entry of the server's statistics (MVP-064, ADR-136): its tick, its settlement by site, what it records, and the
/// settlement's Worker count, a Leader counting as the Worker it was, and strength (MVP-038) at the end of its step.
struct StatisticEntry
{
  std::uint64_t tick = 0;
  std::uint8_t site = 0;
  StatisticKind kind = StatisticKind::Sample;
  std::uint32_t detail = 0;
  std::uint32_t workers = 0;
  std::int64_t strengthMilliRu = 0;

  friend constexpr bool operator==(const StatisticEntry& _a, const StatisticEntry& _b) noexcept = default;
};

[[nodiscard]] constexpr bool IsGrowthStep(StatisticKind _kind) noexcept
{
  return _kind == StatisticKind::StructureCompleted || _kind == StatisticKind::PopulationStep || _kind == StatisticKind::ResourceSiteTaken;
}

// The entry canonically (ADR-081 item 7), and as a region of _stateVersion wrote it; false when it does not read whole.
void WriteStatisticEntry(Neuron::BitWriter& _writer, const StatisticEntry& _entry);
[[nodiscard]] bool ReadStatisticEntry(Neuron::BitReader& _reader, StatisticEntry& _entry, std::uint32_t _stateVersion);

// MVP-064 (ADR-163): an entry of _kind for the settlement at _site, of the region's next tick, with its Worker count and
// strength now; nothing for no settlement.
void RecordEvent(RegionSimulation& _region, std::uint8_t _site, StatisticKind _kind, std::uint32_t _detail);

// MVP-064: the sample period in ticks, mvp.stats.sample_minutes at ref.sim.tick_period, rounded once; 0 samples never.
[[nodiscard]] std::int64_t SampleTicks(const TuningMvp& _tuning) noexcept;

// MVP-064 (ADR-136): the resource sites the settlement's territory holds, by their index, in increasing order. The
// central site, which only a claim holds (WLD-086), is never among them.
[[nodiscard]] std::vector<std::uint32_t> HeldResourceSites(const RegionSimulation& _region, const Settlement& _settlement);

// MVP-064 (ADR-136): the settlement's entries at the end of its own step of the tick: each structure of its own built in
// the tick, in slot order, whose cost counts towards its renown (MVP-061, ADR-163); each resource site in _taken, which
// its territory took in during the step; each valley border pair whose halfway line its territory reaches for the first
// time (ADR-163); each new multiple of the population step; and the sample when the tick is a multiple of the sample
// period.
void RecordStatistics(RegionSimulation& _region, Settlement& _settlement, const std::vector<std::uint32_t>& _taken);

} // namespace Species
