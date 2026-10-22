#pragma once

#include "Settlement.h"

namespace Species
{

class RegionSimulation;

// One tick of a settlement's own step, after its region's slices and AI (ADR-134), after its upkeep (PayUpkeep,
// ADR-135):
// - MVP-026: its territory's radius is wld.territory.founding_radius plus mvp.territory.radius_per_worker for each of
//   its population above mvp.settlement.start_workers, up to wld.territory.max_radius, and never shrinks; a radius that
//   grows makes its territory again;
// - MVP-024: while it is below its population limit and its stores hold mvp.essence.orb_cost_ru, its growth timer
//   counts down, and at 0 its Wellspring pays that cost for one orb at its centre and the timer restarts at
//   mvp.essence.wellspring_interval_s;
// - MVP-064: its statistics of the tick (RecordStatistics, ADR-136), with each resource site its territory took in.
void AdvanceSettlement(RegionSimulation& _region, Settlement& _settlement);

// MVP-024: the growth timer's period in ticks, mvp.essence.wellspring_interval_s at ref.sim.tick_period, rounded once.
[[nodiscard]] std::int32_t GrowthIntervalTicks(const TuningMvp& _tuning) noexcept;

} // namespace Species
