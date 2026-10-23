#include "pch.h"

#include "Battles.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "Figure.h"
#include "Log.h"
#include "RegionSimulation.h"
#include "Settlement.h"
#include "ValleyAtWar.h"
#include "WorkerPool.h"

#include <algorithm>
#include <limits>

namespace Species
{

namespace
{

constexpr double SPAWN_SPREAD = 40.0; // wu: the radius a crowd spawns in about its point
constexpr std::uint32_t LEADERS = 2;

Neuron::Vec3 WellspringOf(const RegionSimulation& _region, std::uint8_t _site)
{
  const Settlement* settlement = _region.SettlementAt(_site);
  const Structure* wellspring = _region.ResolveStructure(settlement->wellspringSlot, settlement->wellspringId);
  return wellspring != nullptr ? wellspring->position : _region.Terrain()->layout.sites[_site];
}

// Where the warning holds a raid of _attacker's on _defender: the objective radius beyond its territory, towards the
// attacker's Wellspring (ADR-161).
Neuron::Vec3 StagingOf(const RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _defender)
{
  const Neuron::Vec3 from = WellspringOf(_region, _defender);
  const Neuron::Vec3 toward = WellspringOf(_region, _attacker);
  const Neuron::Vec3 across{toward.x - from.x, 0.0, toward.z - from.z};
  const double length = Neuron::Length(across);
  const double reach = _region.SettlementAt(_defender)->territoryRadius + _region.Tuning().mvp.battle.objectiveRadius;
  Neuron::Vec3 point = length > 0.0 ? from + across * (reach / length) : from;
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

// A Leader of the settlement at _site at _at, followed by _followers Workers of its own spawned about it.
StableId RaiseLeader(RegionSimulation& _region, std::uint8_t _site, const Neuron::Vec3& _at, std::uint32_t _followers)
{
  RegionSimulation::Spawn spawn;
  spawn.team = _site;
  spawn.kind = FigureKind::Leader;
  spawn.position = _at;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  const StableId id = _region.TeamAt(_site).figures.At(slot).id;
  spawn.kind = FigureKind::Worker;
  spawn.count = _followers;
  spawn.spread = SPAWN_SPREAD;
  for (const std::uint32_t worker : _region.SpawnFigures(spawn))
  {
    Figure& follower = _region.TeamAt(_site).figures.At(worker);
    follower.workerState = WorkerState::FollowingLeader;
    follower.leaderId = id;
    follower.leaderSlot = slot;
  }
  return id;
}

} // namespace

std::array<ValleyRaid, 2> ValleyRaidsOf(std::uint8_t _stewardSite) noexcept
{
  // The first two valley pairs share no site: the steward's pair raids the steward, the other pair raids itself.
  const std::size_t own = VALLEY_PAIRS[0][0] == _stewardSite || VALLEY_PAIRS[0][1] == _stewardSite ? 0 : 1;
  const auto& steward = VALLEY_PAIRS[own];
  const auto& other = VALLEY_PAIRS[1 - own];
  const std::uint8_t neighbor = steward[0] == _stewardSite ? steward[1] : steward[0];
  return {{{neighbor, _stewardSite}, {other[0], other[1]}}};
}

std::unique_ptr<WorldSimulation> StartValleyAtWar(const ValleyAtWarDesc& _desc)
{
  const TuningMvp& tuning = *_desc.tuning;
  WorldStart start;
  start.key = _desc.key;
  start.seed = _desc.terrain->seed;
  start.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(tuning.mvp.sandbox.stewardSite, 0, 3));
  start.aiSettlements = 3;
  std::unique_ptr<WorldSimulation> world = StartWorld(start, _desc.terrain, tuning, _desc.structures);
  world->SetRunEnd(std::numeric_limits<std::uint64_t>::max());
  RegionSimulation& region = world->Region(0);
  const std::uint8_t steward = start.stewardSite.value_or(std::uint8_t{0});
  std::ranges::find(region.Settlements(), steward, &Settlement::site)->cladan.temperament = Temperament::Territorial;
  static_cast<void>(region.AddScriptedSteward(steward));
  for (Settlement& settlement : region.Settlements())
  {
    settlement.storesMilliRu = std::max(settlement.storesMilliRu, MilliRuOf(tuning.mvp.conflict.attackCostRu * 4.0));
  }
  for (const ValleyRaid& raid : ValleyRaidsOf(steward))
  {
    const Neuron::Vec3 staging = StagingOf(region, raid.attacker, raid.defender);
    std::array<StableId, EXPEDITION_LEADERS_MAX> leaders{};
    for (std::uint32_t leader = 0; leader < LEADERS; ++leader)
    {
      leaders[leader] = RaiseLeader(region, raid.attacker, staging, _desc.expedition / LEADERS);
    }
    if (const auto declared = DeclareAttack(region, raid.attacker, leaders, raid.defender, 0); !declared)
    {
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning,
                  "the valley at war: the raid of site {} on site {} is refused ({})", static_cast<int>(raid.attacker),
                  static_cast<int>(raid.defender), static_cast<int>(declared.error()));
    }
  }
  // Each settlement at its population, its expedition's figures among it (ADR-161).
  const std::uint32_t population =
    _desc.population != 0 ? _desc.population : static_cast<std::uint32_t>(std::max<std::int64_t>(0, tuning.mvp.settlement.populationMax));
  for (const Settlement& settlement : region.Settlements())
  {
    const std::uint32_t now = Population(region, settlement);
    if (now < population)
    {
      RegionSimulation::Spawn spawn;
      spawn.team = settlement.team;
      spawn.position = WellspringOf(region, settlement.site);
      spawn.count = population - now;
      spawn.spread = SPAWN_SPREAD * 2.0;
      static_cast<void>(region.SpawnFigures(spawn));
    }
  }
  Neuron::WorkerPool serial(0);
  for (std::uint64_t step = 0; step < _desc.settleTicks && region.Battles().size() < 2; ++step)
  {
    world->Step(world->NextTick(), serial);
  }
  return world;
}

} // namespace Species
