#pragma once

#include "Settlement.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// MVP-035 (ADR-135): one tick of the settlement's upkeep, before its growth. Each Worker accrues
// mvp.upkeep.worker_ru_per_hour and each built, active structure of its own other than the Wellspring
// mvp.upkeep.structure_ru_per_hour, in exact integers; the whole thousandths due are paid from the stores, or all they
// hold when they fall short. While the stores are then empty, every mvp.upkeep.inactive_step_s the most recently
// completed active structure becomes inactive; while they hold anything, every step the inactive structure completed
// earliest becomes active again.
void PayUpkeep(RegionSimulation& _region, Settlement& _settlement);

// MVP-035: the ticks of an hour of play, 3,600 s at ref.sim.tick_period, rounded once.
[[nodiscard]] std::int64_t TicksPerHour(const TuningMvp& _tuning) noexcept;

// MVP-038: the settlement's strength in thousandths of a resource unit, its population at mvp.strength.worker_ru each, a
// Leader counting as the Worker it was, its stores, and each structure of its own at its cost, built or a site.
[[nodiscard]] std::int64_t StrengthMilliRu(const RegionSimulation& _region, const Settlement& _settlement);

} // namespace Species
