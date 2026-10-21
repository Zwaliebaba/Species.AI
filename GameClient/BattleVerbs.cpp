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

} // namespace

BattleVerbs::BattleVerbs(const TuningPresentation& _presentation) noexcept
  : m_presentation(&_presentation)
{
}

void BattleVerbs::Reset() noexcept
{
  m_circle.reset();
  m_fadingCircle.reset();
  m_circleGrown = 0.0;
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
    // O82 (ADR-127): the primary press resolves to one meaning, tried in this order.
    if (_frame.leaderUnderPointer.has_value())
    {
      // Row S1: an own Leader under the pointer is selected, unless it is selected already (R-CTRL-062); it is no
      // order's target.
      if (m_selectedLeader != _frame.leaderUnderPointer)
      {
        _commands.emplace_back(SelectCommand{_team, *_frame.leaderUnderPointer});
      }
    }
    else if (m_leaderSelected)
    {
      // Rows O6 and O7: the selected Leader's order to the pointer ground point, move-only with Shift and straight with
      // Ctrl (R-CTRL-043); none off the region or while the route cannot be walked (ADR-125).
      if (_frame.ground.has_value() && !_frame.routeBlocked)
      {
        _commands.emplace_back(OrderUnitCommand{_team, controls.straight, controls.moveOnly, GroundOf(*_frame.ground)});
      }
    }
    else if (m_workersSelected)
    {
      // Row O1: no circle, a target on the region, a route that can be walked, and the crowd-order interval since the
      // last (R-CTRL-160).
      const auto interval = static_cast<std::uint64_t>(std::max<std::int64_t>(m_presentation->ref.ctrl.order.crowdIntervalTicks, 0));
      if (!m_circle.has_value() && _frame.ground.has_value() && !_frame.routeBlocked &&
          (!m_crowdOrderTick.has_value() || _tick >= *m_crowdOrderTick + interval))
      {
        _commands.emplace_back(OrderWorkersCommand{_team, controls.straight, GroundOf(*_frame.ground)});
        m_crowdOrderTick = _tick;
      }
    }
    else
    {
      // Rows S1 and S4 on empty ground with nothing selected: one Select of nothing (R-CTRL-048, edge case 1), and the
      // circle begins.
      _commands.emplace_back(SelectCommand{_team, 0});
      if (!m_circle.has_value() && _frame.ground.has_value())
      {
        m_circle = SelectionCircle{*_frame.ground, 0.0, 1.0};
        m_fadingCircle.reset();
        m_circleGrown = 0.0;
      }
    }
  }
  if (controls.primaryHeld && m_circle.has_value())
  {
    // Row S5 (R-CTRL-156) as O82 amends it: the radius is the larger of its growth on the real clock and the farthest
    // the pointer ground point has been dragged from the centre, up to the limit, and it never shrinks. Its selection
    // goes at most once a tick, additive.
    m_circleGrown = std::min(m_circleGrown + select.circleGrowth * _frame.realStepSeconds, select.circleMaxRadius);
    double radius = std::max(m_circle->radius, m_circleGrown);
    if (_frame.ground.has_value())
    {
      radius = std::max(radius, Neuron::HorizontalDistance(*_frame.ground, m_circle->center));
    }
    m_circle->radius = std::min(radius, select.circleMaxRadius);
    if (!m_crowdOrderTick.has_value() || _tick > *m_crowdOrderTick)
    {
      _commands.emplace_back(SelectWorkersCommand{_team, GroundOf(m_circle->center), m_circle->radius});
      m_crowdOrderTick = _tick;
    }
  }
  if (controls.primaryUp && m_circle.has_value())
  {
    // Row S6 (R-CTRL-157): the circle ends, and the crowd-order time is reset, so an order may follow at once. ADR-127:
    // only a release that ends a circle resets it, since the primary press also orders; the circle released fades.
    if (m_circle->radius > 0.0)
    {
      m_fadingCircle = m_circle;
    }
    m_circle.reset();
    m_crowdOrderTick.reset();
  }
  if (controls.clear)
  {
    _commands.emplace_back(SelectCommand{_team, 0}); // row S3
  }
}

void BattleVerbs::Secondary(std::uint8_t _team, const VerbFrame& _frame, std::vector<BattleCommand>& _commands)
{
  const VerbControls& controls = _frame.controls;
  if (controls.secondaryDown)
  {
    // O82 (ADR-127): the secondary press resolves to one meaning, tried in this order.
    if (_frame.leaderUnderPointer.has_value() && !m_workersSelected && (!m_leaderSelected || m_selectedLeader == _frame.leaderUnderPointer))
    {
      _commands.emplace_back(ToggleUnitCommand{_team, *_frame.leaderUnderPointer}); // row O3
    }
    else if (m_leaderSelected || m_workersSelected)
    {
      _commands.emplace_back(SelectCommand{_team, 0}); // row S3's effect: the selection cleared, the group released
    }
    else if (m_promotable.has_value())
    {
      _commands.emplace_back(PromoteWorkerCommand{_team, *m_promotable}); // row O2
    }
  }
  if (controls.demote && m_selectedLeader.has_value())
  {
    _commands.emplace_back(DemoteLeaderCommand{_team, *m_selectedLeader}); // row T5 (R-CTRL-044)
  }
}

void BattleVerbs::FadeCircle(double _realStepSeconds) noexcept
{
  // ADR-127: the released circle's strength falls to 0 over mvp.selection.circle_fade_seconds of real time.
  if (!m_fadingCircle.has_value())
  {
    return;
  }
  const double fadeSeconds = m_presentation->mvp.selection.circleFadeSeconds;
  m_fadingCircle->strength = fadeSeconds > 0.0 ? m_fadingCircle->strength - _realStepSeconds / fadeSeconds : 0.0;
  if (m_fadingCircle->strength <= 0.0)
  {
    m_fadingCircle.reset();
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
    m_fadingCircle.reset();
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
  FadeCircle(_frame.realStepSeconds);
  Primary(*_team, _frame, tick, commands);
  Secondary(*_team, _frame, commands);
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
