#include "pch.h"

#include "Claims.h"
#include "Construction.h"
#include "DrawPurpose.h"
#include "FigureAdvance.h"
#include "RegionSimulation.h"
#include "SettlementTasks.h"
#include "StructureAdvance.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace Species
{

namespace
{

constexpr double FULL_TURN_RAD = 6.283185307179586;
constexpr std::size_t TASKS = 3; // WorkerTask's tasks, from Gather

struct PortChoice
{
  std::uint32_t slot = 0;
  std::uint32_t port = 0;
  double distance = 0.0;
};

// The free port nearest to _from, horizontally, of the structures that _accept takes, in slot and port order on a tie
// (R-BLDG-023's availability for _team).
template <class Accept>
std::optional<PortChoice> NearestFreePort(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _from, Accept&& _accept)
{
  std::optional<PortChoice> best;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot) || !_accept(structures.At(slot), slot))
    {
      continue;
    }
    const Structure& structure = structures.At(slot);
    for (std::uint32_t port = 0; port < structure.ports.size(); ++port)
    {
      const double distance = Neuron::HorizontalDistance(_from, structure.ports[port].position);
      if (PortAvailable(structure.ports[port], _team, _region.Tuning()) && (!best || distance < best->distance))
      {
        best = PortChoice{slot, port, distance};
      }
    }
  }
  return best;
}

// A Worker sent to a port walks to it along a route of the land route graph (R-SIM-345), as an order's crowd does.
void SendAlongRoute(RegionSimulation& _region, Figure& _worker, std::uint32_t _slot, std::uint32_t _port)
{
  const Structure& structure = _region.Structures().At(_slot);
  SendToPort(_worker, structure, _slot, _port);
  _worker.route = _region.BuildRoute(_worker.position, structure.ports[_port].position, false);
  _worker.routePoint = -1;
}

// MVP-031: where a load is put down, the Wellspring's door or a storehouse's entrance, on the terrain.
Neuron::Vec3 DropPoint(const RegionSimulation& _region, const Structure& _structure)
{
  Neuron::Vec3 point = _structure.position;
  if (_region.Records() != nullptr)
  {
    const Neuron::SimulationRecord& record = RecordOf(*_region.Records(), _structure.kind);
    const auto kind = _structure.kind == StructureKind::Wellspring ? Neuron::SocketKind::Door : Neuron::SocketKind::Entrance;
    const std::vector<const Neuron::SimulationSocket*> sockets = Neuron::SocketsOf(record, kind);
    if (!sockets.empty())
    {
      point = StructurePoint(_structure, Neuron::RestPositionOf(record, *sockets.front()));
    }
  }
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

bool IsDropOff(const Structure& _structure, std::uint8_t _team) noexcept
{
  return (_structure.kind == StructureKind::Wellspring || _structure.kind == StructureKind::Storehouse) &&
         _structure.owner == std::optional<std::uint8_t>(_team) && _structure.built && _structure.active;
}

} // namespace

void StartCarrying(RegionSimulation& _region, Figure& _worker, std::uint8_t _team)
{
  std::optional<std::uint32_t> nearest;
  double distance = 0.0;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && IsDropOff(structures.At(slot), _team))
    {
      const double candidate = Neuron::HorizontalDistance(_worker.position, DropPoint(_region, structures.At(slot)));
      if (!nearest || candidate < distance)
      {
        nearest = slot;
        distance = candidate;
      }
    }
  }
  if (!nearest)
  {
    _worker.workerState = WorkerState::Idle;
    return;
  }
  const Structure& drop = structures.At(nearest.value_or(0));
  _worker.portStructureId = drop.id;
  _worker.portStructureSlot = nearest.value_or(0);
  _worker.portIndex = 0;
  _worker.waypoint = DropPoint(_region, drop);
  _worker.route = _region.BuildRoute(_worker.position, _worker.waypoint, false);
  _worker.routePoint = -1;
  _worker.workerState = WorkerState::Carrying;
}

Settlement* SettlementOfTeam(RegionSimulation& _region, std::uint8_t _team)
{
  const auto found = std::ranges::find(_region.Settlements(), _team, &Settlement::team);
  return found == _region.Settlements().end() ? nullptr : &*found;
}

TaskCounts CountTasks(const RegionSimulation& _region, const Settlement& _settlement, StableId _searching)
{
  TaskCounts counts;
  if (_settlement.team >= _region.TeamCount())
  {
    return counts;
  }
  const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& figure = figures.At(slot);
    if (figure.kind != FigureKind::Worker || figure.dead || figure.leaderId != 0 || figure.id == _searching)
    {
      continue;
    }
    ++counts.workers;
    if (figure.task != WorkerTask::None)
    {
      ++counts.tasks[static_cast<std::size_t>(figure.task) - 1];
    }
  }
  return counts;
}

bool SettlementTaskSearch(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  Settlement* settlement = SettlementOfTeam(region, _context.self.team);
  if (settlement == nullptr || _worker.leaderId != 0)
  {
    return false;
  }
  const TuningMvp& tuning = region.Tuning();
  const std::uint8_t team = settlement->team;
  const CellRaster* territory = region.TerritoryOf(settlement->site);
  const auto inTerritory = [&](const Structure& _structure)
  { return territory != nullptr && territory->Holds(_structure.position.x, _structure.position.z); };
  // The work of each task: its nearest free port, and guard duty's gathering near the Wellspring, which is always open.
  std::array<std::optional<PortChoice>, TASKS> ports;
  // MVP-053 (ADR-160): the central resource site is gathered by the settlement a claim gave it to.
  const bool holdsCentral = MovedTo(region, CLAIM_AREA_CENTRAL) == std::optional<std::uint8_t>(settlement->site);
  ports[0] = NearestFreePort(region, team, _worker.position,
                             [&](const Structure& _structure, std::uint32_t)
                             {
                               return (_structure.kind == StructureKind::ResourceSite && inTerritory(_structure)) ||
                                      (_structure.kind == StructureKind::ResourceSiteCentral && holdsCentral);
                             });
  if (!settlement->queue.empty())
  {
    const StructureRef first = settlement->queue.front();
    ports[1] = NearestFreePort(region, team, _worker.position, [&](const Structure& _structure, std::uint32_t _slot)
                               { return _slot == first.slot && _structure.id == first.id; });
  }
  ports[2] = NearestFreePort(region, team, _worker.position,
                             [&](const Structure& _structure, std::uint32_t)
                             {
                               return _structure.kind == StructureKind::Turret && _structure.owner == std::optional<std::uint8_t>(team) &&
                                      _structure.built && _structure.active && inTerritory(_structure);
                             });
  const std::array<bool, TASKS> work{ports[0].has_value(), ports[1].has_value(), true};
  std::int64_t withWork = 0;
  for (std::size_t task = 0; task < TASKS; ++task)
  {
    withWork += work[task] ? settlement->priorities[task] : 0;
  }
  IdleReason reason = IdleReason::ShareMet;
  if (withWork > 0)
  {
    // The task furthest below its share of the Workers, the shares of tasks with no work given to the others in
    // proportion: share × workers − count × the shares with work, in integers.
    const TaskCounts counts = CountTasks(region, *settlement, _worker.id);
    const std::int64_t workers = std::int64_t{counts.workers} + 1;
    std::optional<std::size_t> best;
    std::int64_t bestDeficit = 0;
    for (std::size_t task = 0; task < TASKS; ++task)
    {
      const std::int64_t deficit = std::int64_t{settlement->priorities[task]} * workers - std::int64_t{counts.tasks[task]} * withWork;
      if (work[task] && settlement->priorities[task] > 0 && deficit > bestDeficit)
      {
        best = task;
        bestDeficit = deficit;
      }
    }
    if (best)
    {
      const std::size_t task = best.value_or(0);
      _worker.task = static_cast<WorkerTask>(task + 1);
      _worker.idleReason = IdleReason::None;
      _worker.taskTicks = 0;
      if (ports[task])
      {
        SendAlongRoute(region, _worker, ports[task].value_or(PortChoice{}).slot, ports[task].value_or(PortChoice{}).port);
        return true;
      }
      // Guard duty with no turret port: towards a point near the Wellspring, or wandering when it is near already.
      const Structure* wellspring = region.ResolveStructure(settlement->wellspringSlot, settlement->wellspringId);
      const double radius = tuning.mvp.guard.gatherRadius;
      if (wellspring == nullptr || Neuron::HorizontalDistance(_worker.position, wellspring->position) <= radius)
      {
        return false;
      }
      Neuron::SimRng& draw = _context.movement->draws->For(DrawPurpose::GuardPoint, _worker.id);
      const double angle = draw.NextUnit(FULL_TURN_RAD);
      const double distance = draw.NextUnit(radius);
      const Neuron::Vec3 point{wellspring->position.x + std::cos(angle) * distance, wellspring->position.y,
                               wellspring->position.z + std::sin(angle) * distance};
      _worker.waypoint = WorkerPush(_worker, point, DrawPurpose::ShoreWander, *_context.movement);
      return true;
    }
  }
  else
  {
    // MVP-037: the reason of the largest share with no work, gathering's first on a tie.
    const auto largest = std::ranges::max_element(settlement->priorities);
    const auto task = static_cast<std::size_t>(largest - settlement->priorities.begin());
    reason = task == 1 && settlement->queue.empty() ? IdleReason::QueueEmpty : IdleReason::NoFreePort;
  }
  _worker.task = WorkerTask::None;
  _worker.idleReason = reason;
  return false;
}

void GatherAtPort(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const Structure* site = region.ResolveStructure(_worker.portStructureSlot, _worker.portStructureId);
  if (_worker.task != WorkerTask::Gather || site == nullptr ||
      (site->kind != StructureKind::ResourceSite && site->kind != StructureKind::ResourceSiteCentral))
  {
    return;
  }
  const auto& gather = region.Tuning().mvp.gather;
  if (++_worker.taskTicks < gather.portTicks)
  {
    return;
  }
  _worker.taskTicks = 0;
  _worker.carriedMilliRu = MilliRuOf(gather.loadRu);
  StartCarrying(region, _worker, _context.self.team);
}

void CarryingStep(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const MovementContext& movement = *_context.movement;
  Settlement* settlement = SettlementOfTeam(region, _context.self.team);
  const Structure* drop = region.ResolveStructure(_worker.portStructureSlot, _worker.portStructureId);
  if (settlement == nullptr)
  {
    _worker.workerState = WorkerState::Idle;
    return;
  }
  if (drop == nullptr || !IsDropOff(*drop, settlement->team))
  {
    StartCarrying(region, _worker, settlement->team);
    drop = region.ResolveStructure(_worker.portStructureSlot, _worker.portStructureId);
    if (_worker.workerState != WorkerState::Carrying || drop == nullptr)
    {
      return;
    }
  }
  if (_worker.route != 0)
  {
    FollowRoute(_worker, movement);
  }
  if (_worker.route == 0)
  {
    _worker.waypoint = DropPoint(region, *drop);
  }
  if (!WorkerMoveToWaypoint(_worker, false, movement))
  {
    return;
  }
  _worker.velocity = {};
  const std::int64_t capacity = StoreCapacityMilliRu(region.Tuning(), ActiveCount(region, settlement->team, StructureKind::Storehouse));
  if (!Deposit(*settlement, _worker.carriedMilliRu, capacity))
  {
    _worker.idleReason = IdleReason::StoreFull; // it waits at the drop-off point
    return;
  }
  _worker.carriedMilliRu = 0;
  _worker.idleReason = IdleReason::None;
  _worker.portStructureId = 0;
  _worker.workerState = WorkerState::Idle;
  _worker.retargetTimerTicks = 0;
}

} // namespace Species
