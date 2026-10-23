#include "pch.h"

#include "Construction.h"
#include "EssenceOrb.h"
#include "Growth.h"
#include "RegionSimulation.h"
#include "Statistics.h"
#include "Upkeep.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

namespace Species
{

namespace
{

// MVP-024: while the settlement's growth is not held (O106) and it is below its population limit and its stores hold an
// orb's cost, its timer counts down, and at 0 its Wellspring pays for an orb at its centre.
void Grow(RegionSimulation& _region, Settlement& _settlement, std::uint32_t _population)
{
  const TuningMvp& tuning = _region.Tuning();
  const std::int64_t cost = MilliRuOf(tuning.mvp.essence.orbCostRu);
  if (_settlement.growthHeld || _population >= PopulationLimitOf(_region, _settlement) || _settlement.storesMilliRu < cost)
  {
    return;
  }
  if (--_settlement.growthTicks > 0)
  {
    return;
  }
  const Structure* wellspring = _region.ResolveStructure(_settlement.wellspringSlot, _settlement.wellspringId);
  if (wellspring != nullptr && Withdraw(_settlement, cost))
  {
    SpawnOrb(_region, wellspring->center, {}, _settlement.team, wellspring->id);
  }
  _settlement.growthTicks = GrowthIntervalTicks(tuning);
}

} // namespace

std::int32_t GrowthIntervalTicks(const TuningMvp& _tuning) noexcept
{
  return static_cast<std::int32_t>(std::llround(_tuning.mvp.essence.wellspringIntervalS / _tuning.ref.sim.tickPeriod));
}

void AdvanceSettlement(RegionSimulation& _region, Settlement& _settlement)
{
  PayUpkeep(_region, _settlement);
  const TuningMvp& tuning = _region.Tuning();
  const std::uint32_t population = Population(_region, _settlement);
  const auto above = std::max<std::int64_t>(0, std::int64_t{population} - tuning.mvp.settlement.startWorkers);
  const double radius = std::min(tuning.wld.territory.foundingRadius + tuning.mvp.territory.radiusPerWorker * static_cast<double>(above),
                                 tuning.wld.territory.maxRadius);
  std::vector<std::uint32_t> taken;
  if (radius > _settlement.territoryRadius)
  {
    const std::vector<std::uint32_t> before = HeldResourceSites(_region, _settlement);
    _settlement.territoryRadius = radius;
    _region.RecomputeTerritory(_settlement.site);
    const std::vector<std::uint32_t> after = HeldResourceSites(_region, _settlement);
    std::ranges::set_difference(after, before, std::back_inserter(taken));
  }
  Grow(_region, _settlement, population);
  RecordStatistics(_region, _settlement, taken);
}

} // namespace Species
