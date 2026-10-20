#include "pch.h"

#include "Formation.h"
#include "Geometry.h"

#include <algorithm>

namespace Species
{

bool InFormationMode(const Figure& _leader) noexcept
{
  return _leader.order == LeaderOrder::Follow && _leader.formation;
}

bool FormationFull(const Figure& _leader, const TuningMvp& _tuning) noexcept
{
  const auto occupied =
    std::count_if(_leader.slots.begin(), _leader.slots.end(), [](const FormationSlot& _slot) { return _slot.member != 0; });
  return occupied >= _tuning.ref.sim.team.formationSizeUpgrade;
}

std::optional<std::uint32_t> SlotOf(const Figure& _leader, StableId _member) noexcept
{
  if (_member == 0)
  {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < _leader.slots.size(); ++index)
  {
    if (_leader.slots[index].member == _member)
    {
      return static_cast<std::uint32_t>(index);
    }
  }
  return std::nullopt;
}

std::uint32_t RowSizeOf(std::size_t _arraySize, const TuningMvp& _tuning) noexcept
{
  const auto& table = _tuning.ref.unit.leader.rowSizeTable;
  const auto size = static_cast<std::int64_t>(_arraySize);
  std::int64_t row = table.largestRow;
  if (size <= table.smallMax)
  {
    row = table.smallRow;
  }
  else if (size <= table.mediumMax)
  {
    row = table.mediumRow;
  }
  else if (size <= table.largeMax)
  {
    row = table.largeRow;
  }
  else if (size <= table.hugeMax)
  {
    row = table.hugeRow;
  }
  return static_cast<std::uint32_t>(row);
}

Neuron::Vec3 FormationSlotPosition(const Neuron::Vec3& _position, std::uint32_t _index, const Neuron::Vec3& _front, std::size_t _arraySize,
                                   const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto& leader = _tuning.ref.unit.leader;
  const std::uint32_t rowSize = RowSizeOf(_arraySize, _tuning);
  const std::uint32_t rank = _index / rowSize;
  const double column = static_cast<double>(_index % rowSize);
  const double row = static_cast<double>(rank);
  const Neuron::Vec3 right = Neuron::Normalized(Neuron::Cross(_front, {0.0, 1.0, 0.0}));
  Neuron::Vec3 slot = _position + _front * (leader.slotSpacing * row + leader.firstRankOffset) +
                      right * (leader.slotSpacing * column - leader.lateralOffsetFactor * static_cast<double>(rowSize));
  slot.y = _field.Height(slot.x, slot.z);
  return slot;
}

Neuron::Vec3 SlotPositionFor(Figure& _leader, StableId _member, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  std::optional<std::uint32_t> index = SlotOf(_leader, _member);
  if (!index)
  {
    if (FormationFull(_leader, _tuning))
    {
      return {};
    }
    const auto gap = std::find_if(_leader.slots.begin(), _leader.slots.end(), [](const FormationSlot& _slot) { return _slot.member == 0; });
    const FormationSlot slot{_member, static_cast<std::int32_t>(_tuning.ref.unit.leader.slotTimerTicks)};
    if (gap == _leader.slots.end())
    {
      index = static_cast<std::uint32_t>(_leader.slots.size());
      _leader.slots.push_back(slot);
    }
    else
    {
      index = static_cast<std::uint32_t>(gap - _leader.slots.begin());
      *gap = slot;
    }
  }
  return FormationSlotPosition(_leader.position, *index, _leader.front, _leader.slots.size(), _field, _tuning);
}

void RefreshSlot(Figure& _leader, StableId _member) noexcept
{
  if (const std::optional<std::uint32_t> index = SlotOf(_leader, _member))
  {
    ++_leader.slots[*index].timerTicks;
  }
}

void FormationUpkeep(Figure& _leader, const TuningMvp& _tuning) noexcept
{
  for (FormationSlot& slot : _leader.slots)
  {
    if (slot.member == 0)
    {
      continue;
    }
    --slot.timerTicks;
    if (slot.timerTicks <= 0)
    {
      slot = {};
    }
  }
  // The gap count: for every run of consecutive empty slots, its length minus 1.
  std::size_t gaps = 0;
  std::size_t run = 0;
  for (const FormationSlot& slot : _leader.slots)
  {
    if (slot.member == 0)
    {
      ++run;
      continue;
    }
    gaps += run > 0 ? run - 1 : 0;
    run = 0;
  }
  gaps += run > 0 ? run - 1 : 0;
  if (static_cast<double>(gaps) > _tuning.ref.unit.leader.gapRuleFraction * static_cast<double>(_leader.slots.size()))
  {
    std::fill(_leader.slots.begin(), _leader.slots.end(), FormationSlot{});
  }
}

bool LeaderLeads(const Figure& _worker, const Figure* _leader, const TuningMvp& _tuning) noexcept
{
  return _leader != nullptr && !_leader->demoted && _leader->order == LeaderOrder::Follow &&
         !(InFormationMode(*_leader) && FormationFull(*_leader, _tuning) && !SlotOf(*_leader, _worker.id));
}

void StopFollowing(Figure& _worker) noexcept
{
  _worker.workerState = WorkerState::Idle;
  _worker.leaderId = 0;
  _worker.leaderSlot = 0;
}

bool VolleyFires(std::uint64_t _tick, std::uint32_t _index, std::size_t _arraySize, const TuningMvp& _tuning) noexcept
{
  const auto& worker = _tuning.ref.unit.worker;
  const auto cap = static_cast<std::uint64_t>(worker.volleySizeCap);
  const std::uint64_t slots = std::min<std::uint64_t>(cap, _arraySize);
  if (slots == 0)
  {
    return false;
  }
  const auto divisor = static_cast<std::uint64_t>(worker.volleyDivisor);
  return divisor * (_tick + _index) % slots < divisor;
}

} // namespace Species
