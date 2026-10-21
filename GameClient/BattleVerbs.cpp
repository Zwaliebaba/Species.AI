#include "pch.h"

#include "BattleVerbs.h"
#include "FigureKind.h"

#include <algorithm>

namespace Species
{

namespace
{

GroundPoint GroundOf(const Neuron::Vec3& _point) noexcept
{
  return {_point.x, _point.z};
}

// The drawn position of a figure of the frame.
std::optional<Neuron::Vec3> DrawnPosition(std::span<const InterpolatedFigure> _figures, StableId _id)
{
  const auto found = std::ranges::lower_bound(_figures, _id, {}, &InterpolatedFigure::id);
  return found != _figures.end() && found->id == _id ? std::optional<Neuron::Vec3>{found->position} : std::nullopt;
}

} // namespace

BattleVerbs::BattleVerbs(const TuningPresentation& _presentation) noexcept
  : m_presentation(&_presentation)
{
}

void BattleVerbs::Reset() noexcept
{
  m_circle.reset();
  m_crowdOrderTick.reset();
  m_controlStateTick.reset();
  m_controlStatePointer.reset();
  m_predictions.clear();
  m_leaderSelected = false;
  m_selectedLeader.reset();
  m_workersSelected = false;
  m_promotable.reset();
  m_released.reset();
}

void BattleVerbs::Sent(std::uint64_t _sequence, const BattleCommand& _command)
{
  // ADR-124: the selection each command leaves, until its tick has arrived; an order to the selected Leader may keep or
  // clear it (R-UNIT-171), so it predicts nothing.
  Prediction prediction{_sequence, std::nullopt, false, 0, false};
  if (const auto* select = std::get_if<SelectCommand>(&_command))
  {
    prediction.leader = select->actor != 0;
    prediction.leaderId = select->actor;
  }
  else if (std::holds_alternative<SelectWorkersCommand>(_command))
  {
    prediction.workers = true;
  }
  else if (std::holds_alternative<PromoteWorkerCommand>(_command))
  {
    prediction.leader = true; // R-UNIT-153: the new Leader is selected
  }
  else if (const auto* toggle = std::get_if<ToggleUnitCommand>(&_command))
  {
    prediction.leader = true;
    prediction.leaderId = toggle->actor;
  }
  else if (!std::holds_alternative<OrderWorkersCommand>(_command) && !std::holds_alternative<DemoteLeaderCommand>(_command))
  {
    return; // OrderUnit and ControlState
  }
  m_predictions.push_back(prediction);
}

void BattleVerbs::Reported(const CommandReport& _report)
{
  const auto found = std::ranges::find(m_predictions, _report.sequence, &Prediction::sequence);
  if (found == m_predictions.end())
  {
    return;
  }
  if (_report.result != CommandResult::Applied)
  {
    m_predictions.erase(found); // it changed nothing, so the replica stands
    return;
  }
  found->tick = _report.tick;
}

void BattleVerbs::ReadSelection(std::uint8_t _team, const StateReplica& _replica)
{
  m_leaderSelected = false;
  m_selectedLeader.reset();
  m_workersSelected = false;
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* figure = std::get_if<ReplicatedFigure>(&state);
    if (figure == nullptr || figure->team != _team || figure->dead)
    {
      continue;
    }
    if (figure->kind == FigureKind::Leader && figure->selected)
    {
      m_leaderSelected = true;
      m_selectedLeader = id;
    }
    m_workersSelected = m_workersSelected || (figure->kind == FigureKind::Worker && figure->underControl);
  }
  // The latest command whose effect the replica does not hold yet decides the selection.
  if (!m_predictions.empty())
  {
    const Prediction& latest = m_predictions.back();
    m_leaderSelected = latest.leader;
    m_selectedLeader = latest.leaderId != 0 ? std::optional<StableId>{latest.leaderId} : std::nullopt;
    m_workersSelected = latest.workers;
  }
}

std::optional<StableId> BattleVerbs::FindPromotable(std::uint8_t _team, const VerbFrame& _frame,
                                                    std::span<const InterpolatedFigure> _figures) const
{
  // R-CTRL-042 row O2: the nearest own selectable Worker within the promote radius of the pointer ground point, while
  // nothing and no Workers are selected and no own Leader is under the pointer.
  if (m_leaderSelected || m_workersSelected || _frame.leaderUnderPointer.has_value() || !_frame.ground.has_value())
  {
    return std::nullopt;
  }
  std::optional<StableId> nearest;
  double best = m_presentation->ref.ctrl.promoteRadius;
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.team != _team || figure.state.kind != FigureKind::Worker || figure.state.dead ||
        figure.state.task == static_cast<std::uint8_t>(WorkerState::Burning))
    {
      continue;
    }
    const double distance = Neuron::Length(figure.position - *_frame.ground);
    if (distance <= best)
    {
      best = distance;
      nearest = figure.id;
    }
  }
  return nearest;
}

void BattleVerbs::Primary(std::uint8_t _team, const VerbFrame& _frame, std::uint64_t _tick, std::vector<BattleCommand>& _commands)
{
  const VerbControls& controls = _frame.controls;
  const auto& select = m_presentation->ref.ctrl.select;
  if (controls.primaryDown)
  {
    if (_frame.leaderUnderPointer.has_value())
    {
      // Row S1: an own Leader under the pointer is selected, unless it is selected already (R-CTRL-062).
      if (m_selectedLeader != _frame.leaderUnderPointer)
      {
        _commands.emplace_back(SelectCommand{_team, *_frame.leaderUnderPointer});
      }
    }
    else if (!m_leaderSelected)
    {
      // Rows S1 and S4 on empty ground with nothing selected: one Select of nothing (R-CTRL-048, edge case 1), and the
      // circle begins when no Workers are selected. With a Leader selected the press is row S2, which has only a sound.
      const bool workers = m_workersSelected;
      _commands.emplace_back(SelectCommand{_team, 0});
      if (!workers && !m_circle.has_value() && _frame.ground.has_value())
      {
        m_circle = SelectionCircle{*_frame.ground, 0.0};
      }
    }
  }
  if (controls.primaryHeld && m_circle.has_value())
  {
    // Row S5 (R-CTRL-156): the circle grows on the real clock, and its selection goes at most once a tick, additive.
    m_circle->radius = std::min(m_circle->radius + select.circleGrowth * _frame.realStepSeconds, select.circleMaxRadius);
    if (!m_crowdOrderTick.has_value() || _tick > *m_crowdOrderTick)
    {
      _commands.emplace_back(SelectWorkersCommand{_team, GroundOf(m_circle->center), m_circle->radius});
      m_crowdOrderTick = _tick;
    }
  }
  if (controls.primaryUp)
  {
    // Row S6 (R-CTRL-157): the circle ends, and the crowd-order time is reset, so an order may follow at once.
    m_circle.reset();
    m_crowdOrderTick.reset();
  }
  if (controls.clear)
  {
    _commands.emplace_back(SelectCommand{_team, 0}); // row S3
  }
}

void BattleVerbs::Secondary(std::uint8_t _team, const VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint64_t _tick,
                            std::vector<BattleCommand>& _commands)
{
  const VerbControls& controls = _frame.controls;
  if (controls.secondaryDown)
  {
    // R-CTRL-057: the order target is the position of an own Leader under the pointer, or the pointer ground point.
    std::optional<Neuron::Vec3> target = _frame.ground;
    if (_frame.leaderUnderPointer.has_value())
    {
      target = DrawnPosition(_figures, *_frame.leaderUnderPointer);
    }
    // R-CTRL-050, tried in order; the MVP places nothing, so step 1 never applies.
    if (_frame.leaderUnderPointer.has_value() && !m_workersSelected && (!m_leaderSelected || m_selectedLeader == _frame.leaderUnderPointer))
    {
      _commands.emplace_back(ToggleUnitCommand{_team, *_frame.leaderUnderPointer}); // step 2, row O3
    }
    else if (m_leaderSelected)
    {
      if (target.has_value())
      {
        _commands.emplace_back(OrderUnitCommand{_team, controls.straight, controls.moveOnly, GroundOf(*target)}); // step 3, O6, O7
      }
    }
    else if (m_workersSelected)
    {
      // Step 4, row O1: no circle, a target on the region, and the crowd-order interval since the last (R-CTRL-160).
      const auto interval = static_cast<std::uint64_t>(std::max<std::int64_t>(m_presentation->ref.ctrl.order.crowdIntervalTicks, 0));
      if (!m_circle.has_value() && target.has_value() && (!m_crowdOrderTick.has_value() || _tick >= *m_crowdOrderTick + interval))
      {
        _commands.emplace_back(OrderWorkersCommand{_team, controls.straight, GroundOf(*target)});
        m_crowdOrderTick = _tick;
      }
    }
    else if (m_promotable.has_value())
    {
      _commands.emplace_back(PromoteWorkerCommand{_team, *m_promotable}); // step 5, row O2
    }
  }
  if (controls.demote && m_selectedLeader.has_value())
  {
    _commands.emplace_back(DemoteLeaderCommand{_team, *m_selectedLeader}); // row T5 (R-CTRL-044)
  }
}

std::vector<BattleCommand> BattleVerbs::Update(const VerbFrame& _frame, std::optional<std::uint8_t> _team, const StateReplica& _replica,
                                               std::span<const InterpolatedFigure> _figures)
{
  std::vector<BattleCommand> commands;
  const std::optional<std::uint64_t> latest = _replica.LatestTick();
  if (!_team.has_value() || !latest.has_value())
  {
    // O77: a client that commands no team, or holds no state yet, has no verbs.
    m_circle.reset();
    m_promotable.reset();
    return commands;
  }
  const std::uint64_t tick = *latest;
  std::erase_if(m_predictions, [tick](const Prediction& _prediction) { return _prediction.tick.has_value() && tick >= *_prediction.tick; });
  ReadSelection(*_team, _replica);
  // R-CTRL-109: a selected Leader that died is released once.
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* figure = std::get_if<ReplicatedFigure>(&state);
    if (figure != nullptr && figure->team == *_team && figure->kind == FigureKind::Leader && figure->selected && figure->dead &&
        m_released != id && m_predictions.empty())
    {
      commands.emplace_back(SelectCommand{*_team, 0});
      m_released = id;
    }
  }
  m_promotable = FindPromotable(*_team, _frame, _figures);
  Primary(*_team, _frame, tick, commands);
  Secondary(*_team, _frame, _figures, tick, commands);
  // R-SIM-438, ADR-124: the pointer, which only a selected Leader reads (R-UNIT-168), at most once a period and when it
  // moved.
  const auto period = static_cast<std::uint64_t>(std::max<std::int64_t>(m_presentation->ref.sim.command.controlStatePeriod, 1));
  if (m_leaderSelected && _frame.ground.has_value() && m_controlStatePointer != _frame.ground &&
      (!m_controlStateTick.has_value() || tick >= *m_controlStateTick + period))
  {
    commands.emplace_back(ControlStateCommand{*_team, GroundOf(*_frame.ground)});
    m_controlStateTick = tick;
    m_controlStatePointer = _frame.ground;
  }
  return commands;
}

} // namespace Species
