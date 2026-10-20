#include "pch.h"

#include "CommandMessages.h"
#include "MarkerTimings.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "TravelScene.h"
#include "Vec3.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace Performance
{

namespace
{

constexpr Neuron::SipHashKey TRAVEL_KEY{0x74726176656c0001, 0x0123456789abcdef};
constexpr Neuron::SenderId STEWARD = 1;
constexpr std::uint8_t CROWD_TEAM = 0;

struct EdgeEnds
{
  std::string name;
  Neuron::Vec3 from;
  Neuron::Vec3 to;
};

// WLD-100's edges inside the valley, in the order of the terrain's check 8.
std::vector<EdgeEnds> EdgesOf(const Species::RegionLayout& _layout)
{
  std::vector<EdgeEnds> edges;
  for (const auto& [a, b] : std::array<std::pair<std::size_t, std::size_t>, 4>{{{0, 1}, {2, 3}, {0, 2}, {1, 3}}})
  {
    edges.push_back({std::format("site {} - site {}", a, b), _layout.sites[a], _layout.sites[b]});
  }
  for (std::size_t site = 0; site < _layout.sites.size(); ++site)
  {
    edges.push_back({std::format("site {} - centre", site), _layout.sites[site], _layout.valleyCenter});
  }
  return edges;
}

// Where each Worker was when the crowd order was given, and the first tick after which it held no waypoint order.
struct Walker
{
  Neuron::Vec3 start;
  double speed = 0.0;
  bool ordered = false;
  std::optional<std::uint64_t> arrivedTick;
  Neuron::Vec3 end;
};

TravelEdge WalkEdge(std::uint64_t _seed, const EdgeEnds& _edge, const std::shared_ptr<const Species::RegionTerrain>& _terrain,
                    const Species::TuningMvp& _tuning, std::uint64_t _maxTicks)
{
  Species::WorldSimulation world({TRAVEL_KEY, 0, 1, &_tuning, _seed});
  Species::RegionSimulation& region = world.Region(0);
  region.AttachTerrain(_terrain);
  region.SetCommander(CROWD_TEAM, STEWARD, false);
  Species::RegionSimulation::Spawn spawn;
  spawn.team = CROWD_TEAM;
  spawn.position = {_edge.from.x, _terrain->field.Height(_edge.from.x, _edge.from.z), _edge.from.z};
  spawn.count = TRAVEL_WORKERS;
  spawn.spread = TRAVEL_SPREAD;
  const std::vector<std::uint32_t> slots = region.SpawnFigures(spawn);
  std::map<std::uint32_t, Walker> walkers;
  const auto& figures = region.TeamAt(CROWD_TEAM).figures;
  for (const std::uint32_t slot : slots)
  {
    walkers[slot] = Walker{figures.At(slot).position, figures.At(slot).speed, false, std::nullopt, {}};
  }
  // The recorded command stream: the selection and the crowd order along the land routes, executed at the first tick.
  const Species::GroundPoint from{_edge.from.x, _edge.from.z};
  const Species::GroundPoint to{_edge.to.x, _edge.to.z};
  if (!region.Commands().Submit(STEWARD, 0, Neuron::EncodeMessage(Species::SelectWorkersCommand{CROWD_TEAM, from, TRAVEL_SELECT_RADIUS})) ||
      !region.Commands().Submit(STEWARD, 1, Neuron::EncodeMessage(Species::OrderWorkersCommand{CROWD_TEAM, false, to})))
  {
    throw std::runtime_error("the travel scene's crowd order was refused");
  }
  Neuron::WorkerPool serial(0);
  const std::uint64_t orderTick = world.NextTick();
  std::size_t pending = walkers.size();
  std::uint64_t figureTicks = 0;
  static_cast<void>(Neuron::TakeMarkerTimings());
  Neuron::SetMarkerTiming(true);
  for (std::uint64_t tick = orderTick; tick < orderTick + _maxTicks && pending > 0; ++tick)
  {
    world.Step(tick, serial);
    figureTicks += figures.Count();
    for (auto& [slot, walker] : walkers)
    {
      if (walker.arrivedTick || !figures.Used(slot))
      {
        continue;
      }
      const Species::Figure& figure = figures.At(slot);
      if (figure.workerState == Species::WorkerState::OnWaypointOrder)
      {
        walker.ordered = true;
      }
      else if (walker.ordered)
      {
        walker.arrivedTick = tick;
        walker.end = figure.position;
        --pending;
      }
    }
  }
  Neuron::SetMarkerTiming(false);
  TravelEdge edge;
  for (const Neuron::MarkerTiming& marker : Neuron::TakeMarkerTimings())
  {
    if (marker.name == "sim.teams.advance_ungrouped")
    {
      edge.advanceNanoseconds = marker.nanoseconds;
    }
  }
  edge.figureTicks = figureTicks;
  edge.seed = _seed;
  edge.name = _edge.name;
  edge.distance = Neuron::HorizontalDistance(_edge.from, _edge.to);
  edge.workers = static_cast<std::uint32_t>(walkers.size());
  const double tickSeconds = _tuning.ref.sim.tickPeriod;
  for (const auto& [slot, walker] : walkers)
  {
    const double covered = Neuron::HorizontalDistance(walker.start, walker.end);
    if (walker.arrivedTick && covered >= TRAVEL_MIN_DISTANCE_SHARE * edge.distance)
    {
      // The tick it arrived in counts whole: the walk began in the order's tick.
      const auto walkingTicks = static_cast<double>(*walker.arrivedTick - orderTick + 1);
      edge.factors.push_back(walkingTicks * tickSeconds * walker.speed / covered);
    }
  }
  std::ranges::sort(edge.factors);
  return edge;
}

} // namespace

std::vector<TravelEdge> MeasureTravel(std::uint64_t _seed, const Species::TuningMvp& _tuning, std::size_t _maxEdges,
                                      std::uint64_t _maxTicks)
{
  auto generated = Species::GenerateRegionTerrain(_seed, _tuning);
  if (!generated)
  {
    throw std::runtime_error(std::format("seed {} gives no valley", _seed));
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  std::vector<TravelEdge> edges;
  for (const EdgeEnds& ends : EdgesOf(terrain->layout))
  {
    if (edges.size() == _maxEdges)
    {
      break;
    }
    edges.push_back(WalkEdge(terrain->seed, ends, terrain, _tuning, _maxTicks));
  }
  return edges;
}

} // namespace Performance
