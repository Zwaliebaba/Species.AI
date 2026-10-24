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
#include <vector>

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

bool IsDropOff(const Structure& _structure, std::uint8_t _team) noexcept
{
  return (_structure.kind == StructureKind::Wellspring || _structure.kind == StructureKind::Storehouse) &&
         _structure.owner == std::optional<std::uint8_t>(_team) && _structure.built && _structure.active;
}

// O110 (ADR-184): whether _worker belongs to its home port: a living Worker of a settlement's task that follows no Leader
// and no order, and gathers.
bool AtHomePort(const Figure& _worker) noexcept
{
  return _worker.kind == FigureKind::Worker && !_worker.dead && _worker.leaderId == 0 && !_worker.orderSet &&
         _worker.task == WorkerTask::Gather && _worker.homeStructureId != 0;
}

// A port as one sortable key: its structure's slot × 2³² + its index.
std::uint64_t PortKey(std::uint32_t _slot, std::uint32_t _port) noexcept
{
  return (std::uint64_t{_slot} << 32U) | _port;
}

// O110 (ADR-184): the home ports of _team's gatherers other than _self, as PortKey, sorted.
std::vector<std::uint64_t> HomePortsOf(const RegionSimulation& _region, std::uint8_t _team, StableId _self)
{
  std::vector<std::uint64_t> homes;
  if (_team >= _region.TeamCount())
  {
    return homes;
  }
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& figure = figures.At(slot);
    if (figure.id != _self && AtHomePort(figure) && _region.ResolveStructure(figure.homeStructureSlot, figure.homeStructureId) != nullptr)
    {
      homes.push_back(PortKey(figure.homeStructureSlot, figure.homePortIndex));
    }
  }
  std::ranges::sort(homes);
  return homes;
}

std::uint32_t GatherersAt(const std::vector<std::uint64_t>& _homes, std::uint32_t _slot, std::uint32_t _port)
{
  const auto [first, last] = std::ranges::equal_range(_homes, PortKey(_slot, _port));
  return static_cast<std::uint32_t>(std::distance(first, last));
}

// O110 (ADR-184): how many gatherers each port of a resource site at _position takes: the turns at the port,
// mvp.gather.port_ticks each, that one gatherer's round lasts, rounded up, the round being its turn and its walk to _team's
// nearest drop-off point and back at the Worker's speed; one while _team has no drop-off point.
std::uint32_t PortTurns(const RegionSimulation& _region, const Neuron::Vec3& _position, std::uint8_t _team)
{
  std::optional<double> nearest;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && IsDropOff(structures.At(slot), _team))
    {
      const double distance = Neuron::HorizontalDistance(_position, _region.DropPointAt(slot));
      nearest = !nearest || distance < nearest.value_or(0.0) ? std::optional<double>(distance) : nearest;
    }
  }
  if (!nearest)
  {
    return 1;
  }
  const TuningMvp& tuning = _region.Tuning();
  const double portTicks = static_cast<double>(std::max<std::int64_t>(1, tuning.mvp.gather.portTicks));
  const double walkTicks = 2.0 * nearest.value_or(0.0) / (tuning.ref.unit.worker.speed * tuning.ref.sim.tickPeriod);
  return static_cast<std::uint32_t>(std::ceil((portTicks + walkTicks) / portTicks));
}

// O110 (ADR-184): the port _worker gathers at: its home port while that port still takes it, else, of the ports of the
// structures that _accept takes with room for another gatherer, the one with the fewest, the nearest to the Worker first
// and then in slot and port order.
template <class Accept>
std::optional<PortChoice> GatherPort(const RegionSimulation& _region, std::uint8_t _team, const Figure& _worker, Accept&& _accept)
{
  const std::vector<std::uint64_t> homes = HomePortsOf(_region, _team, _worker.id);
  if (_worker.task == WorkerTask::Gather && _worker.homeStructureId != 0)
  {
    const Structure* home = _region.ResolveStructure(_worker.homeStructureSlot, _worker.homeStructureId);
    if (home != nullptr && _accept(*home, _worker.homeStructureSlot) && _worker.homePortIndex < home->ports.size() &&
        GatherersAt(homes, _worker.homeStructureSlot, _worker.homePortIndex) < PortTurns(_region, home->position, _team))
    {
      return PortChoice{_worker.homeStructureSlot, _worker.homePortIndex,
                        Neuron::HorizontalDistance(_worker.position, home->ports[_worker.homePortIndex].position)};
    }
  }
  std::optional<PortChoice> best;
  std::uint32_t bestGatherers = 0;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot) || !_accept(structures.At(slot), slot))
    {
      continue;
    }
    const Structure& structure = structures.At(slot);
    const std::uint32_t turns = PortTurns(_region, structure.position, _team);
    for (std::uint32_t port = 0; port < structure.ports.size(); ++port)
    {
      const std::uint32_t gatherers = GatherersAt(homes, slot, port);
      const double distance = Neuron::HorizontalDistance(_worker.position, structure.ports[port].position);
      if (gatherers < turns && (!best || gatherers < bestGatherers || (gatherers == bestGatherers && distance < best->distance)))
      {
        best = PortChoice{slot, port, distance};
        bestGatherers = gatherers;
      }
    }
  }
  return best;
}

// O110 (ADR-184): the gatherer's home port, or none.
void SetHomePort(Figure& _worker, StableId _structure, std::uint32_t _slot, std::uint32_t _port) noexcept
{
  _worker.homeStructureId = _structure;
  _worker.homeStructureSlot = _structure != 0 ? _slot : 0;
  _worker.homePortIndex = _structure != 0 ? _port : 0;
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
      const double candidate = Neuron::HorizontalDistance(_worker.position, _region.DropPointAt(slot));
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
  _worker.waypoint = _region.DropPointAt(_worker.portStructureSlot);
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
  // O110 (ADR-184): a gatherer goes back to its home port, where it waits its turn.
  ports[0] = GatherPort(region, team, _worker,
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
      SetHomePort(_worker, 0, 0, 0);
      if (ports[task])
      {
        const PortChoice choice = ports[task].value_or(PortChoice{});
        SendAlongRoute(region, _worker, choice.slot, choice.port);
        if (task == 0)
        {
          SetHomePort(_worker, region.Structures().At(choice.slot).id, choice.slot, choice.port);
        }
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
  SetHomePort(_worker, 0, 0, 0);
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
  // O110 (ADR-184): it comes back to this port; a gatherer sent here before the home ports existed takes it now.
  SetHomePort(_worker, site->id, _worker.portStructureSlot, _worker.portIndex);
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
    _worker.waypoint = region.DropPointAt(_worker.portStructureSlot);
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
