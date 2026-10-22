#include "pch.h"

#include "Construction.h"
#include "EssenceOrb.h"
#include "Growth.h"
#include "RegionSimulation.h"
#include "Upkeep.h"

#include <algorithm>
#include <cmath>

namespace Species
{

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
  if (radius > _settlement.territoryRadius)
  {
    _settlement.territoryRadius = radius;
    _region.RecomputeTerritory(_settlement.site);
  }
  const std::int64_t cost = MilliRuOf(tuning.mvp.essence.orbCostRu);
  if (population >= PopulationLimitOf(_region, _settlement) || _settlement.storesMilliRu < cost)
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

} // namespace Species
