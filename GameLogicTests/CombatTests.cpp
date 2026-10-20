#include "pch.h"

#include "BitWriter.h"
#include "Combat.h"
#include "DrawPurpose.h"
#include "FigureAdvance.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "TestSupport.h"
#include "Weapons.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <numbers>
#include <tuple>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey COMBAT_TEST_KEY{0x0c0b0a7e57c0b0a7, 0x5eed5eed5eed5eed};
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};

Species::RegionSimulation FlatRegion()
{
  return Species::RegionSimulation(0, COMBAT_TEST_KEY, 0, CommittedTuning(), FlatTerrain(50.0));
}

// One Worker of _team at _position, on the ground, and its actor-grid entry.
Species::ActorRef SpawnWorker(Species::RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.position = _position;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  Species::Figure& figure = _region.TeamAt(_team).figures.At(slot);
  figure.position = _position;
  figure.onGround = true;
  return {figure.id, slot, _team};
}

Species::Figure& FigureOf(Species::RegionSimulation& _region, const Species::ActorRef& _ref)
{
  return *_region.Resolve(_ref);
}

// The angle between two horizontal directions, in degrees.
double AngleDegrees(const Neuron::Vec3& _a, const Neuron::Vec3& _b)
{
  const double dot = (_a.x * _b.x + _a.z * _b.z) / (std::hypot(_a.x, _a.z) * std::hypot(_b.x, _b.z));
  return std::acos(std::clamp(dot, -1.0, 1.0)) * 180.0 / std::numbers::pi;
}

std::size_t CountEvents(const Species::RegionSimulation& _region, Species::EffectEventKind _kind)
{
  return static_cast<std::size_t>(std::ranges::count(_region.Events(), _kind, &Species::EffectEvent::kind));
}

std::uint32_t LiveFigures(const Species::RegionSimulation& _region, std::size_t _team)
{
  std::uint32_t live = 0;
  const auto& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    live += figures.Used(slot) && !figures.At(slot).dead ? 1u : 0u;
  }
  return live;
}

} // namespace

TEST_CLASS(CombatTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit098TheThreatSearchTakesTheNearestEnemyOrAGrenade)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-098")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit098TheThreatSearchTakesTheNearestEnemyOrAGrenade)
  {
    // The nearest live enemy becomes the threat and its distance the threat range, a brave fight against two; a grenade
    // of the Worker's own team within 60 wu outranks every enemy, and the Worker flees it (AC-UNIT-W05).
    Species::RegionSimulation region = FlatRegion();
    const Species::ActorRef self = SpawnWorker(region, 0, MIDDLE);
    static_cast<void>(SpawnWorker(region, 1, MIDDLE + Neuron::Vec3{70.0, 0.0, 0.0}));
    const Species::ActorRef nearest = SpawnWorker(region, 1, MIDDLE + Neuron::Vec3{0.0, 0.0, 50.0});
    const Species::MovementContext movement = region.Movement();
    const Species::FigureContext context{&region, &movement, self};
    Species::Figure& worker = FigureOf(region, self);
    Assert::IsTrue(Species::ThreatSearch(worker, context), L"a threat");
    Assert::IsTrue(worker.workerState == Species::WorkerState::Fighting && !worker.scared, L"Fighting, brave");
    Assert::IsTrue(worker.threat == Species::Threat{Species::ThreatKind::Actor, 1, nearest.slot, nearest.id}, L"the nearest enemy");
    Assert::IsTrue(std::abs(worker.threatRange - 50.0) < 1e-9, L"the range is its distance");
    Species::ThrowGrenade(region, 0, MIDDLE + Neuron::Vec3{40.0, 5.0, 0.0}, MIDDLE + Neuron::Vec3{40.0, 0.0, 0.0});
    Assert::IsTrue(Species::ThreatSearch(worker, context), L"the grenade");
    Assert::IsTrue(worker.threat.kind == Species::ThreatKind::Grenade && worker.scared, L"a scared Worker with a grenade threat");
    Assert::IsTrue(CountEvents(region, Species::EffectEventKind::ThreatBark) == 2, L"a bark for each new threat");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit102ASoldierRetreatsLevelAndClosesUphill)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit102ASoldierRetreatsLevelAndClosesUphill)
  {
    // AC-UNIT-W08 over 40 ticks' draws: 80 wu from an enemy at its height, the waypoint lies 30 wu off, away within ±45°;
    // with the enemy 30 wu higher, it lies about 20 to 40 wu short of it, towards it within ±45°.
    Species::RegionSimulation region = FlatRegion();
    const Species::ActorRef self = SpawnWorker(region, 0, MIDDLE);
    const Species::ActorRef enemy = SpawnWorker(region, 1, MIDDLE + Neuron::Vec3{80.0, 0.0, 0.0});
    const Species::MovementContext movement = region.Movement();
    const Species::FigureContext context{&region, &movement, self};
    Species::Figure& worker = FigureOf(region, self);
    for (std::uint64_t tick = 1; tick <= 40; ++tick)
    {
      for (const double rise : {0.0, 30.0})
      {
        region.Draws().SetTick(tick);
        FigureOf(region, enemy).position.y = MIDDLE.y + rise;
        worker.position = MIDDLE;
        worker.scared = false;
        worker.workerState = Species::WorkerState::Fighting;
        worker.threat = {Species::ThreatKind::Actor, 1, enemy.slot, enemy.id};
        Species::FightingStep(worker, context);
        const Neuron::Vec3 offset = worker.waypoint - MIDDLE;
        if (rise == 0.0)
        {
          Assert::IsTrue(std::abs(std::hypot(offset.x, offset.z) - 30.0) < 1e-6, std::format(L"30 wu off; tick {}", tick).c_str());
          Assert::IsTrue(AngleDegrees(offset, {-1.0, 0.0, 0.0}) <= 45.0 + 1e-9, L"away within 45 degrees");
        }
        else
        {
          // Its length falls short of the enemy's 80 wu by the stand-off along the 3-D direction, 20 to 40 × cos(atan(30/80)).
          const double shortOf = 80.0 - std::hypot(offset.x, offset.z);
          Assert::IsTrue(shortOf > 18.0 && shortOf < 40.0, std::format(L"short of the enemy by {}", shortOf).c_str());
          Assert::IsTrue(AngleDegrees(offset, {1.0, 0.0, 0.0}) <= 45.0 + 1e-9, L"towards within 45 degrees");
        }
        Assert::IsTrue(std::abs(worker.waypoint.y - MIDDLE.y) < 1e-9, L"snapped to the ground");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit103OneShotAttemptInTenAndOneShotPerReload)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-103")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-052")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit103OneShotAttemptInTenAndOneShotPerReload)
  {
    // AC-UNIT-W09 over 10,000 Fighting ticks: the fire roll succeeds on about 0.1 of them, and the bolts it fires are at
    // least the reload's 30 ticks apart.
    Species::RegionSimulation region = FlatRegion();
    const Species::ActorRef self = SpawnWorker(region, 0, MIDDLE);
    const Species::ActorRef enemy = SpawnWorker(region, 1, MIDDLE + Neuron::Vec3{150.0, 0.0, 0.0});
    const Species::MovementContext movement = region.Movement();
    const Species::FigureContext context{&region, &movement, self};
    Species::Figure& worker = FigureOf(region, self);
    std::uint32_t attempts = 0;
    std::vector<std::uint64_t> shots;
    for (std::uint64_t tick = 1; tick <= 10000; ++tick)
    {
      region.Draws().SetTick(tick);
      if (worker.reloadTimerTicks > 0)
      {
        --worker.reloadTimerTicks;
      }
      worker.position = MIDDLE;
      worker.workerState = Species::WorkerState::Fighting;
      worker.threat = {Species::ThreatKind::Actor, 1, enemy.slot, enemy.id};
      worker.grenadeTimerTicks = 1000;
      Neuron::SimRng roll(COMBAT_TEST_KEY, Species::PurposeOf(Species::DrawPurpose::FireRoll), worker.id, tick);
      attempts += roll.NextUint() % 10 == 0 ? 1u : 0u;
      const std::size_t before = region.LaserBolts().Count();
      Species::FightingStep(worker, context);
      if (region.LaserBolts().Count() > before)
      {
        shots.push_back(tick);
      }
    }
    const double rate = attempts / 10000.0;
    Assert::IsTrue(rate > 0.09 && rate < 0.11, std::format(L"attempt rate {}", rate).c_str());
    for (std::size_t n = 1; n < shots.size(); ++n)
    {
      Assert::IsTrue(shots[n] - shots[n - 1] >= 30, L"one shot per reload");
    }
    Assert::IsTrue(shots.size() > 200, std::format(L"{} shots", shots.size()).c_str());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit104TheCrowdRuleDecidesTheThrow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-104")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit104TheCrowdRuleDecidesTheThrow)
  {
    // AC-UNIT-W11: a Worker whose grenade timer expires 100 wu from an enemy on flat ground throws when the cells around
    // the enemy hold 6 enemies and 1 friend, and not with 2 friends.
    for (const int friends : {1, 2})
    {
      Species::RegionSimulation region = FlatRegion();
      const Species::ActorRef self = SpawnWorker(region, 0, MIDDLE);
      const Neuron::Vec3 target = MIDDLE + Neuron::Vec3{100.0, 0.0, 0.0};
      const Species::ActorRef enemy = SpawnWorker(region, 1, target);
      for (int n = 1; n < 6; ++n)
      {
        static_cast<void>(SpawnWorker(region, 1, target + Neuron::Vec3{2.0 * n, 0.0, 3.0}));
      }
      for (int n = 0; n < friends; ++n)
      {
        static_cast<void>(SpawnWorker(region, 0, target + Neuron::Vec3{-3.0 - n, 0.0, -2.0}));
      }
      const Species::MovementContext movement = region.Movement();
      const Species::FigureContext context{&region, &movement, self};
      Species::Figure& worker = FigureOf(region, self);
      worker.workerState = Species::WorkerState::Fighting;
      worker.scared = false;
      worker.threat = {Species::ThreatKind::Actor, 1, enemy.slot, enemy.id};
      worker.grenadeTimerTicks = 1;
      Species::FightingStep(worker, context);
      ExpectEqual(friends == 1 ? 1 : 0, region.Effects().Count(), std::format(L"grenades with {} friends", friends));
      Assert::IsTrue(worker.grenadeTimerTicks >= 121 && worker.grenadeTimerTicks <= 200, L"the next interval");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit079ABurningWorkerRunsUntilItDies)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-079")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-121")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-134")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit079ABurningWorkerRunsUntilItDies)
  {
    // AC-UNIT-W15: Workers set on fire never leave Burning, die after about 125 ticks on average, and each releases one
    // Essence orb.
    Species::RegionSimulation region = FlatRegion();
    std::vector<Species::ActorRef> burning;
    burning.reserve(40);
    for (int row = 0; row < 5; ++row)
    {
      for (int column = 0; column < 8; ++column)
      {
        burning.push_back(SpawnWorker(region, 0, MIDDLE + Neuron::Vec3{25.0 * column, 0.0, 25.0 * row}));
      }
    }
    for (const Species::ActorRef& ref : burning)
    {
      region.SetOnFire(ref);
    }
    std::uint64_t ticks = 0;
    std::uint64_t total = 0;
    std::vector<bool> counted(burning.size(), false);
    while (ticks < 2000 && std::ranges::count(counted, false) > 0)
    {
      region.Step(region.NextTick());
      ++ticks;
      for (std::size_t n = 0; n < burning.size(); ++n)
      {
        const Species::Figure* figure = region.Resolve(burning[n]);
        if (figure == nullptr)
        {
          continue; // its corpse has faded
        }
        Assert::IsTrue(figure->workerState == Species::WorkerState::Burning, L"never leaves Burning");
        if (figure->dead && !counted[n])
        {
          counted[n] = true;
          total += ticks;
        }
      }
    }
    const double mean = static_cast<double>(total) / static_cast<double>(burning.size());
    Assert::IsTrue(mean > 95.0 && mean < 160.0, std::format(L"mean time to death {} ticks", mean).c_str());
    ExpectEqual(static_cast<std::int64_t>(burning.size()), region.Orbs().Count(), L"one orb each");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit080ADrowningWorkerDiesOnTheNextTickWithOneOrb)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-038")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit080ADrowningWorkerDiesOnTheNextTickWithOneOrb)
  {
    // AC-UNIT-W16: a Worker falling into the sea is Drowning on the tick its y passes below sea level and dead on the
    // next, with one Essence orb.
    Species::RegionSimulation region(0, COMBAT_TEST_KEY, 0, CommittedTuning(), FlatTerrain(-20.0));
    const Species::ActorRef self = SpawnWorker(region, 0, MIDDLE);
    Species::Figure& worker = FigureOf(region, self);
    worker.position.y = 3.0;
    worker.onGround = false;
    std::uint64_t drowning = 0;
    for (std::uint64_t tick = 1; tick <= 40 && !worker.dead; ++tick)
    {
      region.Step(region.NextTick());
      if (drowning == 0 && worker.workerState == Species::WorkerState::Drowning)
      {
        drowning = tick;
        Assert::IsFalse(worker.dead, L"alive on the tick it sinks");
      }
      if (worker.dead)
      {
        ExpectEqual(static_cast<std::int64_t>(drowning + 1), static_cast<std::int64_t>(tick), L"dead on the next tick");
      }
    }
    Assert::IsTrue(worker.dead && region.Orbs().Count() == 1, L"dead, one orb");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp040TwoCrowdsClashAndStepTheSameTwice)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-040")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-134")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp040TwoCrowdsClashAndStepTheSameTwice)
  {
    // M1 plan T6's done-when as the reference rules allow it (ADR-105): two crowds of 200 mixed at the centre of seed 1's
    // valley, on the threat search alone, clash for 3,000 ticks with heavy losses on both sides, kills credited and orbs
    // released; a second run gives the same state at every 50th tick. Left alone, the survivors stand off about 100 wu
    // apart, beyond the laser's reach, so the fighting thins out rather than ending (R-UNIT-098, R-UNIT-102).
    const auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1's valley");
    const auto terrain = std::make_shared<const Species::RegionTerrain>(*generated);
    const auto battle = [&](std::vector<std::vector<std::byte>>& _states)
    {
      Species::RegionSimulation region(0, COMBAT_TEST_KEY, 0, CommittedTuning(), terrain);
      for (const std::uint8_t team : {std::uint8_t{0}, std::uint8_t{1}})
      {
        Species::RegionSimulation::Spawn spawn;
        spawn.team = team;
        spawn.count = 200;
        spawn.spread = 60.0;
        spawn.position = terrain->layout.valleyCenter + Neuron::Vec3{team == 0 ? -25.0 : 25.0, 0.0, 0.0};
        spawn.position.y = terrain->field.Height(spawn.position.x, spawn.position.z);
        static_cast<void>(region.SpawnFigures(spawn));
      }
      std::size_t deaths = 0;
      std::size_t shots = 0;
      std::size_t throws = 0;
      for (int tick = 1; tick <= 3000; ++tick)
      {
        region.Step(region.NextTick());
        deaths += CountEvents(region, Species::EffectEventKind::FigureDied);
        shots += CountEvents(region, Species::EffectEventKind::LaserFired);
        throws += CountEvents(region, Species::EffectEventKind::GrenadeThrown);
        if (tick % 50 == 0)
        {
          Neuron::BitWriter state;
          region.WriteState(state);
          _states.emplace_back(state.Bytes().begin(), state.Bytes().end());
        }
      }
      return std::tuple{
        LiveFigures(region, 0), LiveFigures(region, 1), region.TeamAt(0).kills + region.TeamAt(1).kills, deaths, shots, throws};
    };
    std::vector<std::vector<std::byte>> first;
    std::vector<std::vector<std::byte>> second;
    const auto [alive0, alive1, kills, deaths, shots, throws] = battle(first);
    Assert::IsTrue(alive0 <= 150 && alive1 <= 150, std::format(L"heavy losses: {} and {} left", alive0, alive1).c_str());
    ExpectEqual(400 - alive0 - alive1, static_cast<std::int64_t>(deaths), L"every death an event, and an orb");
    Assert::IsTrue(kills >= 80 && shots >= 300 && throws >= 10,
                   std::format(L"{} kills, {} shots, {} grenades", kills, shots, throws).c_str());
    static_cast<void>(battle(second));
    Assert::IsTrue(first == second, L"the same states on a second run");
  }
};

} // namespace GameLogicTests
