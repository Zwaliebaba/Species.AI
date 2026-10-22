#include "pch.h"

#include "Construction.h"
#include "RegionSimulation.h"
#include "Upkeep.h"

#include <cmath>
#include <optional>
#include <tuple>

namespace Species
{

namespace
{

constexpr double SECONDS_PER_HOUR = 3600.0;

// The owned structures that pay upkeep and can become inactive: every one of the settlement's other than its Wellspring.
bool PaysUpkeep(const Structure& _structure, std::uint8_t _team) noexcept
{
  return _structure.owner == std::optional<std::uint8_t>(_team) && _structure.kind != StructureKind::Wellspring && _structure.built;
}

// The structure that the next step turns: the most recently completed active one when _deactivate, otherwise the
// inactive one completed earliest; later slots count as later on an equal tick.
std::optional<std::uint32_t> NextToTurn(const RegionSimulation& _region, std::uint8_t _team, bool _deactivate)
{
  std::optional<std::uint32_t> chosen;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot) || !PaysUpkeep(structures.At(slot), _team) || structures.At(slot).active != _deactivate)
    {
      continue;
    }
    const auto key = std::tuple{structures.At(slot).completedTick, slot};
    if (!chosen)
    {
      chosen = slot;
      continue;
    }
    const auto best = std::tuple{structures.At(chosen.value_or(0)).completedTick, chosen.value_or(0)};
    if (_deactivate ? key > best : key < best)
    {
      chosen = slot;
    }
  }
  return chosen;
}

} // namespace

std::int64_t TicksPerHour(const TuningMvp& _tuning) noexcept
{
  return std::llround(SECONDS_PER_HOUR / _tuning.ref.sim.tickPeriod);
}

std::int64_t UpkeepMilliRuPerHour(const RegionSimulation& _region, const Settlement& _settlement)
{
  const auto& upkeep = _region.Tuning().mvp.upkeep;
  std::int64_t structures = 0;
  const SlicedPool<Structure>& pool = _region.Structures();
  for (std::uint32_t slot = 0; slot < pool.Capacity(); ++slot)
  {
    structures += pool.Used(slot) && PaysUpkeep(pool.At(slot), _settlement.team) && pool.At(slot).active ? 1 : 0;
  }
  return std::int64_t{Population(_region, _settlement)} * MilliRuOf(upkeep.workerRuPerHour) +
         structures * MilliRuOf(upkeep.structureRuPerHour);
}

std::uint32_t InactiveStructures(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::uint32_t inactive = 0;
  const SlicedPool<Structure>& pool = _region.Structures();
  for (std::uint32_t slot = 0; slot < pool.Capacity(); ++slot)
  {
    inactive += pool.Used(slot) && PaysUpkeep(pool.At(slot), _settlement.team) && !pool.At(slot).active ? 1 : 0;
  }
  return inactive;
}

void PayUpkeep(RegionSimulation& _region, Settlement& _settlement)
{
  const TuningMvp& tuning = _region.Tuning();
  const std::int64_t ticksPerHour = TicksPerHour(tuning);
  _settlement.upkeepCarry += UpkeepMilliRuPerHour(_region, _settlement);
  const std::int64_t due = _settlement.upkeepCarry / ticksPerHour;
  _settlement.upkeepCarry %= ticksPerHour;
  if (!Withdraw(_settlement, due))
  {
    _settlement.storesMilliRu = 0; // the stores give what they hold; Workers never die of an empty store
  }
  // The steps of inactive structures, one every mvp.upkeep.inactive_step_s: one way while the stores are empty, the other
  // while they hold anything.
  const bool empty = _settlement.storesMilliRu == 0;
  const std::optional<std::uint32_t> next = NextToTurn(_region, _settlement.team, empty);
  if (!next)
  {
    _settlement.inactiveTicks = 0;
    return;
  }
  if (_settlement.inactiveTicks <= 0)
  {
    _settlement.inactiveTicks = static_cast<std::int32_t>(std::llround(tuning.mvp.upkeep.inactiveStepS / tuning.ref.sim.tickPeriod));
  }
  if (--_settlement.inactiveTicks <= 0)
  {
    _region.Structures().At(next.value_or(0)).active = !empty;
  }
}

std::int64_t StrengthMilliRu(const RegionSimulation& _region, const Settlement& _settlement)
{
  const TuningMvp& tuning = _region.Tuning();
  const std::int64_t workers = Population(_region, _settlement);
  std::int64_t structures = 0;
  const SlicedPool<Structure>& pool = _region.Structures();
  for (std::uint32_t slot = 0; slot < pool.Capacity(); ++slot)
  {
    structures += pool.Used(slot) && pool.At(slot).owner == std::optional<std::uint8_t>(_settlement.team) ? pool.At(slot).costMilliRu : 0;
  }
  return workers * MilliRuOf(tuning.mvp.strength.workerRu) + _settlement.storesMilliRu + structures;
}

} // namespace Species
