#include "pch.h"

#include "Construction.h"
#include "RegionSimulation.h"
#include "Statistics.h"
#include "Upkeep.h"

#include <algorithm>
#include <cmath>

namespace Species
{

namespace
{

constexpr double SECONDS_PER_MINUTE = 60.0;

} // namespace

void WriteStatisticEntry(Neuron::BitWriter& _writer, const StatisticEntry& _entry)
{
  _writer.WriteBits(_entry.tick, 64);
  _writer.WriteBits(_entry.site, 8);
  _writer.WriteBounded(static_cast<std::uint64_t>(_entry.kind), STATISTIC_KIND_MAX);
  _writer.WriteBits(_entry.detail, 32);
  _writer.WriteBits(_entry.workers, 32);
  _writer.WriteSigned(_entry.strengthMilliRu, 64);
}

bool ReadStatisticEntry(Neuron::BitReader& _reader, StatisticEntry& _entry)
{
  _entry.tick = _reader.ReadBits(64);
  _entry.site = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _entry.kind = static_cast<StatisticKind>(_reader.ReadBounded(STATISTIC_KIND_MAX));
  _entry.detail = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _entry.workers = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _entry.strengthMilliRu = _reader.ReadSigned(64);
  return !_reader.Failed() && _entry.strengthMilliRu >= 0;
}

std::int64_t SampleTicks(const TuningMvp& _tuning) noexcept
{
  return std::llround(_tuning.mvp.stats.sampleMinutes * SECONDS_PER_MINUTE / _tuning.ref.sim.tickPeriod);
}

std::vector<std::uint32_t> HeldResourceSites(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::vector<std::uint32_t> held;
  const CellRaster* territory = _region.TerritoryOf(_settlement.site);
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; territory != nullptr && slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot))
    {
      continue;
    }
    const Structure& site = structures.At(slot);
    if (site.kind == StructureKind::ResourceSite && site.resourceSite && territory->Holds(site.position.x, site.position.z))
    {
      held.push_back(site.resourceSite.value_or(0));
    }
  }
  std::ranges::sort(held);
  return held;
}

void RecordStatistics(RegionSimulation& _region, Settlement& _settlement, const std::vector<std::uint32_t>& _taken)
{
  const TuningMvp& tuning = _region.Tuning();
  const std::uint64_t tick = _region.Draws().Tick();
  const auto workers = Population(_region, _settlement);
  const std::int64_t strength = StrengthMilliRu(_region, _settlement);
  const auto record = [&](StatisticKind _kind, std::uint32_t _detail)
  { _region.RecordStatistic({tick, _settlement.site, _kind, _detail, workers, strength}); };
  // A structure is built in its structure's step of the tick, before its settlement's own (ADR-134); the Wellspring,
  // which the world places, is no step.
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot))
    {
      continue;
    }
    const Structure& structure = structures.At(slot);
    if (structure.owner == std::optional<std::uint8_t>(_settlement.team) && structure.built && structure.completedTick == tick &&
        structure.kind != StructureKind::Wellspring)
    {
      record(StatisticKind::StructureCompleted, static_cast<std::uint32_t>(structure.kind));
    }
  }
  for (const std::uint32_t site : _taken)
  {
    record(StatisticKind::ResourceSiteTaken, site);
  }
  const std::int64_t step = tuning.mvp.stats.populationStep;
  while (step > 0 && std::int64_t{workers} >= std::int64_t{_settlement.populationMark} + step)
  {
    _settlement.populationMark = static_cast<std::uint32_t>(_settlement.populationMark + step);
    record(StatisticKind::PopulationStep, _settlement.populationMark);
  }
  const std::int64_t period = SampleTicks(tuning);
  if (period > 0 && tick % static_cast<std::uint64_t>(period) == 0)
  {
    record(StatisticKind::Sample, 0);
  }
}

} // namespace Species
