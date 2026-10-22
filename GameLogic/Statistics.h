#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Settlement.h"

#include <cstdint>
#include <vector>

namespace Species
{

class RegionSimulation;

// MVP-064: what an entry of the server's statistics records; a state value that is never renumbered. The last three
// are the growth steps of the M2 plan's §9, question 1.
enum class StatisticKind : std::uint8_t
{
  Sample = 0,             // every mvp.stats.sample_minutes of world time
  StructureCompleted = 1, // a structure of the settlement was built; the detail is its kind
  PopulationStep = 2,     // the population reached a new multiple of mvp.stats.population_step; the detail is that multiple
  ResourceSiteTaken = 3   // the territory took in a resource site it did not hold; the detail is the site's index
};

inline constexpr std::uint64_t STATISTIC_KIND_MAX = 3;

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
  return _kind != StatisticKind::Sample;
}

void WriteStatisticEntry(Neuron::BitWriter& _writer, const StatisticEntry& _entry);
[[nodiscard]] bool ReadStatisticEntry(Neuron::BitReader& _reader, StatisticEntry& _entry);

// MVP-064: the sample period in ticks, mvp.stats.sample_minutes at ref.sim.tick_period, rounded once; 0 samples never.
[[nodiscard]] std::int64_t SampleTicks(const TuningMvp& _tuning) noexcept;

// MVP-064 (ADR-136): the resource sites the settlement's territory holds, by their index, in increasing order. The
// central site, which only a claim holds (WLD-086), is never among them.
[[nodiscard]] std::vector<std::uint32_t> HeldResourceSites(const RegionSimulation& _region, const Settlement& _settlement);

// MVP-064 (ADR-136): the settlement's entries at the end of its own step of the tick: each structure of its own built in
// the tick, in slot order; each resource site in _taken, which its territory took in during the step; each new multiple
// of the population step; and the sample when the tick is a multiple of the sample period.
void RecordStatistics(RegionSimulation& _region, Settlement& _settlement, const std::vector<std::uint32_t>& _taken);

} // namespace Species
