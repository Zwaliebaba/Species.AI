#include "pch.h"

#include "CommandMessages.h"
#include "FigureAdvance.h"
#include "Formation.h"
#include "LeaderOrders.h"
#include "RegionCommands.h"
#include "RegionSimulation.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

using Neuron::CommandEffect;

// A command's point on the ground: clamped into the region's square (R-SIM-366), at the terrain's height.
Neuron::Vec3 OnGround(RegionSimulation& _region, const GroundPoint& _point)
{
  const MovementContext movement = _region.Movement();
  const double x = std::clamp(_point.x, movement.minX, movement.maxX);
  const double z = std::clamp(_point.z, movement.minZ, movement.maxZ);
  return {x, movement.terrain != nullptr ? movement.Height(x, z) : 0.0, z};
}

// R-SIM-453: the figure of _kind that _id names in _team, while it qualifies: it exists, is alive and enabled, and is
// not on its way out after a promotion or a demotion.
std::optional<ActorRef> Qualifying(const RegionSimulation& _region, std::uint8_t _team, StableId _id, FigureKind _kind)
{
  const std::optional<ActorRef> ref = _region.FindFigure(_team, _id);
  const Figure* figure = ref ? _region.Resolve(*ref) : nullptr;
  if (figure == nullptr || figure->kind != _kind || figure->dead || figure->destroyed || !figure->enabled || figure->promoted ||
      figure->demoted)
  {
    return std::nullopt;
  }
  return ref;
}

// A Worker that an order reaches: Burning and Drowning end only in death (R-UNIT-079, R-UNIT-080; ADR-106).
bool Orderable(const Figure& _worker) noexcept
{
  return _worker.workerState != WorkerState::Burning && _worker.workerState != WorkerState::Drowning;
}

CommandEffect Select(RegionSimulation& _region, const SelectCommand& _command)
{
  if (_command.actor == 0)
  {
    _region.Select(_command.team, std::nullopt);
    return CommandEffect::Applied;
  }
  const std::optional<ActorRef> leader = Qualifying(_region, _command.team, _command.actor, FigureKind::Leader);
  if (!leader)
  {
    return CommandEffect::NoEffect;
  }
  _region.Select(_command.team, leader);
  return CommandEffect::Applied;
}

// R-UNIT-153: a Leader at the Worker's position with its velocity, selected; the Worker goes at its next advance.
CommandEffect PromoteWorker(RegionSimulation& _region, const PromoteWorkerCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.actor, FigureKind::Worker);
  Figure* worker = ref ? _region.Resolve(*ref) : nullptr;
  if (worker == nullptr || worker->workerState == WorkerState::Burning)
  {
    return CommandEffect::NoEffect;
  }
  worker->promoted = true;
  RegionSimulation::Spawn spawn;
  spawn.team = _command.team;
  spawn.kind = FigureKind::Leader;
  spawn.position = worker->position;
  spawn.velocity = worker->velocity;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  const Figure& leader = _region.TeamAt(_command.team).figures.At(slot);
  const ActorRef leaderRef{leader.id, slot, _command.team};
  _region.Emit({EffectEventKind::WorkerPromoted, _command.team, leader.id, leader.position, {}, 0.0});
  _region.Select(_command.team, leaderRef);
  return CommandEffect::Applied;
}

// R-UNIT-154: a Worker at the Leader's position with its velocity; the Leader goes at its next advance.
CommandEffect DemoteLeader(RegionSimulation& _region, const DemoteLeaderCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.actor, FigureKind::Leader);
  Figure* leader = ref ? _region.Resolve(*ref) : nullptr;
  if (leader == nullptr)
  {
    return CommandEffect::NoEffect;
  }
  leader->demoted = true;
  RegionSimulation::Spawn spawn;
  spawn.team = _command.team;
  spawn.kind = FigureKind::Worker;
  spawn.position = leader->position;
  spawn.velocity = leader->velocity;
  static_cast<void>(_region.SpawnFigures(spawn));
  _region.Select(_command.team, std::nullopt);
  return CommandEffect::Applied;
}

// R-UNIT-175: the selected Leader's waypoint, or its orders.
CommandEffect OrderUnit(RegionSimulation& _region, const OrderUnitCommand& _command)
{
  const std::optional<ActorRef> ref = _region.SelectedLeaderRef(_command.team);
  if (!ref)
  {
    return CommandEffect::NoEffect;
  }
  Figure* leader = _region.Resolve(*ref);
  if (leader == nullptr || leader->demoted)
  {
    return CommandEffect::NoEffect;
  }
  const Neuron::Vec3 target = OnGround(_region, _command.position);
  if (_command.moveOnly)
  {
    AssignWaypoint(*leader, target);
  }
  else
  {
    AssignOrders(_region, *ref, target, _command.straight);
  }
  return CommandEffect::Applied;
}

// R-UNIT-175: a Leader in formation or with a waypoint order cancels its orders; any other enters formation facing
// forward. Either way it is selected in the same step (R-SIM-454).
CommandEffect ToggleUnit(RegionSimulation& _region, const ToggleUnitCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.actor, FigureKind::Leader);
  Figure* leader = ref ? _region.Resolve(*ref) : nullptr;
  if (leader == nullptr)
  {
    return CommandEffect::NoEffect;
  }
  if (InFormationMode(*leader) || leader->order == LeaderOrder::Waypoint)
  {
    ClearOrders(*leader);
  }
  else
  {
    AssignOrders(_region, *ref, leader->position + leader->front * _region.Tuning().ref.sim.command.leaderToggleOffset, false);
  }
  _region.Select(_command.team, ref);
  return CommandEffect::Applied;
}

// R-UNIT-136: the team's Workers within the circle that are alive and not Burning come under control; the selection
// before it is cleared first (R-UNIT-137).
CommandEffect SelectWorkers(RegionSimulation& _region, const SelectWorkersCommand& _command)
{
  _region.Select(_command.team, std::nullopt);
  const Neuron::Vec3 center = OnGround(_region, _command.center);
  std::uint32_t selected = 0;
  for (const ActorRef& ref : _region.InRange(center.x, center.z, _command.radius, RegionSimulation::FriendsOf(_command.team)))
  {
    Figure* worker = _region.Resolve(ref);
    if (worker != nullptr && worker->kind == FigureKind::Worker && !worker->dead && !worker->promoted &&
        worker->workerState != WorkerState::Burning)
    {
      worker->underControl = true;
      ++selected;
    }
  }
  _region.Emit({EffectEventKind::WorkersSelected, _command.team, 0, center, {}, static_cast<double>(selected)});
  return CommandEffect::Applied;
}

// R-UNIT-140, R-SIM-455: the controlled Workers at this tick take a waypoint order along one route from their mean
// position, and the selection is released; with none left the order does nothing.
CommandEffect OrderWorkers(RegionSimulation& _region, const OrderWorkersCommand& _command)
{
  if (_command.team >= _region.TeamCount())
  {
    return CommandEffect::NoEffect;
  }
  SlicedPool<Figure>& figures = _region.TeamAt(_command.team).figures;
  std::vector<std::uint32_t> controlled;
  Neuron::Vec3 sum;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& worker = figures.At(slot);
    if (worker.kind == FigureKind::Worker && worker.underControl && !worker.dead && !worker.promoted && Orderable(worker))
    {
      controlled.push_back(slot);
      sum = sum + worker.position;
    }
  }
  if (controlled.empty())
  {
    return CommandEffect::NoEffect;
  }
  const Neuron::Vec3 start = sum * (1.0 / static_cast<double>(controlled.size()));
  const Neuron::Vec3 target = OnGround(_region, _command.position);
  const RouteHandle route = _region.BuildRoute(start, target, _command.straight);
  const MovementContext movement = _region.Movement();
  for (const std::uint32_t slot : controlled)
  {
    Figure& worker = figures.At(slot);
    GiveWaypointOrder(worker, target, route, movement);
    worker.playerOrders = true;
  }
  _region.Emit({EffectEventKind::WorkersOrderedByPlayer, _command.team, 0, target, {}, static_cast<double>(controlled.size())});
  _region.Select(_command.team, std::nullopt);
  return CommandEffect::Applied;
}

// R-SIM-442: the team's pointer position; the latest of a tick holds, because each replaces the last (R-SIM-443).
CommandEffect ControlState(RegionSimulation& _region, const ControlStateCommand& _command)
{
  const Neuron::Vec3 pointer = OnGround(_region, _command.pointer);
  _region.EnsureTeam(_command.team).pointer = pointer;
  return CommandEffect::Applied;
}

// R-UNIT-111 for the AI, with no route (R-BOT-086).
CommandEffect AiOrderWorker(RegionSimulation& _region, const AiOrderWorkerCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.worker, FigureKind::Worker);
  Figure* worker = ref ? _region.Resolve(*ref) : nullptr;
  if (worker == nullptr || !Orderable(*worker))
  {
    return CommandEffect::NoEffect;
  }
  const Neuron::Vec3 target = OnGround(_region, _command.position);
  GiveWaypointOrder(*worker, target, 0, _region.Movement());
  return CommandEffect::Applied;
}

// R-UNIT-188's Leader operations for the AI.
CommandEffect AiOrderLeader(RegionSimulation& _region, const AiOrderLeaderCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.leader, FigureKind::Leader);
  if (!ref)
  {
    return CommandEffect::NoEffect;
  }
  const Neuron::Vec3 target = OnGround(_region, _command.position);
  switch (_command.operation)
  {
  case LeaderOperation::AssignOrders:
    AssignOrders(_region, *ref, target, _command.straight);
    break;
  case LeaderOperation::AssignWaypoint:
    AssignWaypoint(*_region.Resolve(*ref), target);
    break;
  case LeaderOperation::FormUp:
    FormUp(_region, *ref, target);
    break;
  }
  return CommandEffect::Applied;
}

// Table 2.3-A: the AI's fields of a Leader. A front of no horizontal length leaves the bot target front as it was.
CommandEffect AiLeaderFields(RegionSimulation& _region, const AiLeaderFieldsCommand& _command)
{
  const std::optional<ActorRef> ref = Qualifying(_region, _command.team, _command.leader, FigureKind::Leader);
  if (!ref)
  {
    return CommandEffect::NoEffect;
  }
  Figure& leader = *_region.Resolve(*ref);
  leader.noFormations = _command.noFormations;
  const Neuron::Vec3 front{_command.front.x, 0.0, _command.front.z};
  if (_command.hasFront && Neuron::LengthSquared(front) > Neuron::NORMALIZE_MIN_LENGTH_SQUARED)
  {
    leader.botFront = Neuron::Normalized(front);
    leader.botTurn = true;
  }
  return CommandEffect::Applied;
}

// One command type: permitted when its sender commands the team it names, and is an AI when _aiOnly.
template <class T> void Add(RegionSimulation& _region, bool _aiOnly, CommandEffect (*_execute)(RegionSimulation&, const T&))
{
  _region.Commands().Register<T>(
    [&_region, _aiOnly](Neuron::SenderId _sender, const T& _command) { return _region.MayCommand(_sender, _command.team, _aiOnly); },
    [&_region, _execute](Neuron::SenderId, const T& _command, std::uint64_t) { return _execute(_region, _command); });
}

} // namespace

void RegisterRegionCommands(RegionSimulation& _region)
{
  Add<SelectCommand>(_region, false, &Select);
  Add<PromoteWorkerCommand>(_region, false, &PromoteWorker);
  Add<DemoteLeaderCommand>(_region, false, &DemoteLeader);
  Add<OrderUnitCommand>(_region, false, &OrderUnit);
  Add<ToggleUnitCommand>(_region, false, &ToggleUnit);
  Add<SelectWorkersCommand>(_region, false, &SelectWorkers);
  Add<OrderWorkersCommand>(_region, false, &OrderWorkers);
  Add<ControlStateCommand>(_region, false, &ControlState);
  Add<AiOrderWorkerCommand>(_region, true, &AiOrderWorker);
  Add<AiOrderLeaderCommand>(_region, true, &AiOrderLeader);
  Add<AiLeaderFieldsCommand>(_region, true, &AiLeaderFields);
}

} // namespace Species
