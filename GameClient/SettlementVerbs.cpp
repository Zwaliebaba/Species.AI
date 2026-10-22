#include "pch.h"

#include "SettlementVerbs.h"

#include "StructureCost.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace Species
{

namespace
{

constexpr std::size_t TASKS = 3;
constexpr std::int64_t SHARE_TOTAL = 100; // MVP-036: the shares sum to 100 %

bool SumsToTotal(const std::array<std::uint8_t, 3>& _shares) noexcept
{
  return std::int64_t{_shares[0]} + _shares[1] + _shares[2] == SHARE_TOTAL;
}

} // namespace

std::array<std::uint8_t, 3> ShiftPriority(std::array<std::uint8_t, 3> _shares, std::size_t _task, std::int64_t _deltaPercent) noexcept
{
  if (_task >= TASKS || _deltaPercent == 0)
  {
    return _shares;
  }
  const std::size_t low = std::min((_task + 1) % TASKS, (_task + 2) % TASKS);
  const std::size_t high = std::max((_task + 1) % TASKS, (_task + 2) % TASKS);
  std::array<std::int64_t, 3> share{_shares[0], _shares[1], _shares[2]};
  if (_deltaPercent > 0)
  {
    // A raise takes half from each other task, the larger giving the odd point, and the rest from one that runs out.
    const std::int64_t amount = std::min(_deltaPercent, share[low] + share[high]);
    const std::size_t first = share[high] > share[low] ? high : low;
    const std::size_t second = first == low ? high : low;
    std::int64_t fromFirst = (amount + 1) / 2;
    std::int64_t fromSecond = amount / 2;
    if (fromSecond > share[second])
    {
      fromFirst += fromSecond - share[second];
      fromSecond = share[second];
    }
    if (fromFirst > share[first])
    {
      fromSecond += fromFirst - share[first];
      fromFirst = share[first];
    }
    share[first] -= fromFirst;
    share[second] -= fromSecond;
    share[_task] += amount;
  }
  else
  {
    // A lower gives half to each other task, the smaller taking the odd point.
    const std::int64_t amount = std::min(_deltaPercent < -SHARE_TOTAL ? SHARE_TOTAL : -_deltaPercent, share[_task]);
    const std::size_t first = share[high] < share[low] ? high : low;
    const std::size_t second = first == low ? high : low;
    share[first] += (amount + 1) / 2;
    share[second] += amount / 2;
    share[_task] -= amount;
  }
  return {static_cast<std::uint8_t>(share[0]), static_cast<std::uint8_t>(share[1]), static_cast<std::uint8_t>(share[2])};
}

PlacementView PlacementViewOf(const StateReplica::View& _latest, std::optional<std::uint8_t> _team,
                              const std::optional<SettlementStatus>& _status)
{
  PlacementView view;
  if (!_team.has_value())
  {
    return view;
  }
  for (const auto& [id, state] : _latest)
  {
    if (const auto* settlement = std::get_if<ReplicatedSettlement>(&state); settlement != nullptr && settlement->team == *_team)
    {
      view.settlement = *settlement;
    }
    else if (const auto* structure = std::get_if<ReplicatedStructure>(&state);
             structure != nullptr && structure->kind == StructureKind::Wellspring && structure->owner == _team)
    {
      view.wellspring = PositionOf(structure->position);
    }
  }
  view.dwellings = TeamDwellings(_latest, *_team);
  if (view.settlement.has_value() && _status.has_value() && _status->team == *_team)
  {
    view.status = _status;
  }
  return view;
}

std::optional<PlacementRefusal> PredictPlacement(const PlacementView& _view, const StewardAreas& _areas, StructureKind _kind,
                                                 const Neuron::Vec3& _point, const TuningMvp& _tuning)
{
  if (!_view.settlement.has_value())
  {
    return PlacementRefusal::NotBuildable;
  }
  if (_view.status.has_value() && std::cmp_greater_equal(_view.status->queue.size(), _tuning.mvp.construction.queueMax))
  {
    return PlacementRefusal::QueueFull;
  }
  if (!_areas.InTerritory(_point.x, _point.z))
  {
    return PlacementRefusal::OutsideTerritory;
  }
  // The server measures from the footprint's edge, so a point inside the ring is refused whatever the footprint.
  if (_view.wellspring.has_value() && Neuron::HorizontalDistance(_point, *_view.wellspring) < _tuning.mvp.construction.centerRingRadius)
  {
    return PlacementRefusal::InCenterRing;
  }
  if (std::cmp_less(_view.settlement->storesMilliRu, StructureCostMilliRu(_tuning, _kind, _view.dwellings)))
  {
    return PlacementRefusal::CannotAfford;
  }
  return std::nullopt;
}

SettlementVerbs::SettlementVerbs(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept
  : m_tuning(_tuning),
    m_presentation(_presentation)
{
}

void SettlementVerbs::Reset() noexcept
{
  m_placing.reset();
  m_foreseen.reset();
  m_ghost.reset();
  m_placements.clear();
  m_refusal.reset();
  m_refusalAgeSeconds = 0.0;
  m_shares.reset();
}

SettlementVerbResult SettlementVerbs::Update(const SettlementFrame& _frame, std::optional<std::uint8_t> _team, const PlacementView& _view,
                                             const StewardAreas& _areas)
{
  SettlementVerbResult result;
  // The server's refusal ages on the real clock, and the shares sent show until a status of their tick holds them.
  if (m_refusal.has_value())
  {
    m_refusalAgeSeconds += std::max(_frame.realStepSeconds, 0.0);
    if (m_refusalAgeSeconds >= m_presentation.mvp.hud.noticeSeconds)
    {
      m_refusal.reset();
    }
  }
  if (m_shares.has_value() && m_shares->tick.has_value() && _view.status.has_value() && _view.status->tick >= *m_shares->tick)
  {
    m_shares.reset();
  }
  m_foreseen.reset();
  m_ghost.reset();
  if (!_team.has_value() || !_view.settlement.has_value())
  {
    m_placing.reset();
    return result;
  }
  const std::uint8_t team = *_team;
  // UI-461: a press over the HUD reaches no world verb; the secondary press leaves placing there too.
  if (_frame.hudZone.has_value())
  {
    result.tookPrimary = _frame.primaryDown;
    result.tookSecondary = _frame.secondaryDown;
    if (_frame.primaryDown)
    {
      PressHud(*_frame.hudZone, team, _view, result);
    }
    else if (_frame.secondaryDown)
    {
      m_placing.reset();
    }
    return result;
  }
  if (!m_placing.has_value())
  {
    return result;
  }
  // O82: placing takes both presses before the battle verbs, and the secondary press leaves it.
  result.tookPrimary = _frame.primaryDown;
  result.tookSecondary = _frame.secondaryDown;
  if (_frame.secondaryDown)
  {
    m_placing.reset();
    return result;
  }
  if (!_frame.ground.has_value())
  {
    return result;
  }
  const Neuron::Vec3& ground = *_frame.ground;
  m_foreseen = PredictPlacement(_view, _areas, *m_placing, ground, m_tuning);
  // MVP-032 (ADR-132): the structure faces away from its Wellspring, +x where it stands on it (R-BLDG-014).
  StructureGhost ghost{*m_placing, ground, std::numbers::pi * 0.5, team};
  if (_view.wellspring.has_value() && Neuron::HorizontalDistance(ground, *_view.wellspring) > 0.0)
  {
    ghost.yawRadians = std::atan2(ground.x - _view.wellspring->x, ground.z - _view.wellspring->z);
  }
  m_ghost = ghost;
  // ADR-153: the press sends what the client foresees refused too, so that the reason shown is the server's.
  if (_frame.primaryDown)
  {
    PlaceStructureCommand command;
    command.team = team;
    command.kind = *m_placing;
    command.position = {ground.x, ground.z};
    result.commands.emplace_back(command);
  }
  return result;
}

void SettlementVerbs::PressHud(const HudZoneRect& _zone, std::uint8_t _team, const PlacementView& _view, SettlementVerbResult& _result)
{
  switch (_zone.zone)
  {
  case HudZone::Build:
    // MVP-101: an entry enters placing its structure, and leaves it when it is the one being placed.
    if (_zone.index < BUILD_KINDS.size())
    {
      const StructureKind kind = BUILD_KINDS[_zone.index];
      m_placing = m_placing == kind ? std::nullopt : std::optional<StructureKind>(kind);
    }
    break;
  case HudZone::PriorityRaise:
  case HudZone::PriorityLower:
  {
    // MVP-036: the shares move from those last sent, or the status's.
    if (!_view.status.has_value() || _zone.index >= TASKS)
    {
      break;
    }
    const std::array<std::uint8_t, 3> from = m_shares.has_value() ? m_shares->shares : _view.status->priorities;
    const std::int64_t step = m_presentation.mvp.hud.priorityStep;
    const std::array<std::uint8_t, 3> to = ShiftPriority(from, _zone.index, _zone.zone == HudZone::PriorityRaise ? step : -step);
    if (to != from && SumsToTotal(to))
    {
      SetPrioritiesCommand command;
      command.team = _team;
      command.shares = to;
      _result.commands.emplace_back(command);
    }
    break;
  }
  case HudZone::Settlement:
  case HudZone::Queue:
  case HudZone::Notices:
  case HudZone::World:
    break;
  }
}

void SettlementVerbs::Sent(std::uint64_t _sequence, const SettlementCommand& _command)
{
  if (std::holds_alternative<PlaceStructureCommand>(_command))
  {
    m_placements.push_back(_sequence);
  }
  else if (const auto* priorities = std::get_if<SetPrioritiesCommand>(&_command); priorities != nullptr)
  {
    m_shares = SentShares{priorities->shares, _sequence, std::nullopt};
  }
}

void SettlementVerbs::Reported(const CommandReport& _report)
{
  if (const auto placement = std::ranges::find(m_placements, _report.sequence); placement != m_placements.end())
  {
    m_placements.erase(placement);
    // MVP-101 (ADR-136): a refusal's reason travels in its outcome; one with no placement reason shows as not buildable.
    if (_report.result == CommandResult::Applied)
    {
      m_refusal.reset();
    }
    else
    {
      const bool known =
        _report.reason >= static_cast<std::uint8_t>(PlacementRefusal::NotBuildable) && _report.reason <= PLACEMENT_REFUSAL_MAX;
      m_refusal = known ? static_cast<PlacementRefusal>(_report.reason) : PlacementRefusal::NotBuildable;
      m_refusalAgeSeconds = 0.0;
    }
    return;
  }
  if (m_shares.has_value() && m_shares->sequence == _report.sequence)
  {
    if (_report.result == CommandResult::Applied)
    {
      m_shares->tick = _report.tick;
    }
    else
    {
      m_shares.reset();
    }
  }
}

HudVerbs SettlementVerbs::ForHud() const
{
  HudVerbs verbs;
  verbs.placing = m_placing;
  verbs.foreseen = m_foreseen;
  verbs.refusal = m_refusal;
  verbs.refusalAgeSeconds = m_refusalAgeSeconds;
  if (m_shares.has_value())
  {
    verbs.shares = m_shares->shares;
  }
  return verbs;
}

} // namespace Species
