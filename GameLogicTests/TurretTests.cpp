#include "pch.h"

#include "Construction.h"
#include "Sandbox.h"
#include "StateHash.h"
#include "StructureAdvance.h"
#include "TestSupport.h"
#include "Turret.h"
#include "Weapons.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey TURRET_TEST_KEY{0x7e77e7000000a001, 0x7e77e7000000a002};
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};
constexpr std::uint8_t GUNNERS = 0; // the team that mans the turrets of the flat tests
constexpr std::uint8_t ENEMIES = 1;
constexpr std::uint8_t SETTLED_TEAM = 2; // the settlement of SettledWorld, whose team is its site

// A lone region on flat ground at 50 wu, which builds its structures from the test records.
Species::RegionSimulation FlatRegion()
{
  return Species::RegionSimulation(0, TURRET_TEST_KEY, 0, CommittedTuning(), FlatTerrain(50.0), &TestRecords());
}

// A figure of _team on the ground at _position.
Species::ActorRef PlaceFigure(Species::RegionSimulation& _region, std::uint8_t _team, Species::FigureKind _kind,
                              const Neuron::Vec3& _position)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.kind = _kind;
  spawn.position = _position;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  Species::Figure& figure = _region.TeamAt(_team).figures.At(slot);
  figure.position = _position;
  figure.onGround = true;
  return {figure.id, slot, _team};
}

// Holds a figure through the next step without taking it off the actor grid, as disabling it would (R-SIM-114): on the
// ground at _position, still, with no task search due and its weapon reloading.
void Hold(Species::RegionSimulation& _region, const Species::ActorRef& _ref, const Neuron::Vec3& _position)
{
  Species::Figure& figure = *_region.Resolve(_ref);
  figure.position = _position;
  figure.velocity = {};
  figure.onGround = true;
  figure.waypoint = _position;
  figure.workerState = Species::WorkerState::Idle;
  figure.retargetTimerTicks = 100;
  figure.reloadTimerTicks = 100;
}

// A built turret of the settlement team _owner at (_x, _z), facing +z, and a Worker of _team for each of its first
// _manned ports, standing at the port and sent to it: it mans the port on its first advance and is the port's occupant
// from that tick's structure step.
std::uint32_t MannedTurret(Species::RegionSimulation& _region, std::uint8_t _owner, std::uint8_t _team, double _x, double _z,
                           std::uint32_t _manned)
{
  const std::uint32_t slot = _region.AddStructure(Species::StructureKind::Turret, _owner, std::nullopt, _x, _z, {0.0, 0.0, 1.0});
  for (std::uint32_t port = 0; port < _manned; ++port)
  {
    const Species::Structure& turret = _region.Structures().At(slot);
    const Species::ActorRef gunner = PlaceFigure(_region, _team, Species::FigureKind::Worker, turret.ports[port].position);
    Species::SendToPort(*_region.Resolve(gunner), turret, slot, port);
  }
  return slot;
}

const Species::TurretState& TurretOf(const Species::RegionSimulation& _region, std::uint32_t _slot)
{
  const std::optional<Species::TurretState>& turret = _region.Structures().At(_slot).turret;
  static const Species::TurretState NONE;
  Assert::IsTrue(turret.has_value(), L"a turret's state");
  return turret ? *turret : NONE;
}

// The rounds the last step fired, by barrel.
std::vector<std::uint32_t> BarrelsFired(const Species::RegionSimulation& _region)
{
  std::vector<std::uint32_t> barrels;
  for (const Species::EffectEvent& event : _region.Events())
  {
    if (event.kind == Species::EffectEventKind::TurretFired)
    {
      barrels.push_back(static_cast<std::uint32_t>(event.value));
    }
  }
  return barrels;
}

// The settlement's Workers' slots, in slot order.
std::vector<std::uint32_t> WorkersOf(const Species::RegionSimulation& _region)
{
  std::vector<std::uint32_t> slots;
  const auto& figures = _region.TeamAt(SETTLED_TEAM).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot))
    {
      slots.push_back(slot);
    }
  }
  return slots;
}

} // namespace

TEST_CLASS(TurretTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg408ATurretIsOwnedByTheTeamThatMansIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-408")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-410")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg408ATurretIsOwnedByTheTeamThatMansIt)
  {
    // The settlement's turret is manned by Workers of another team: the evaluation of its first advance reads ports
    // that its base advance has not yet evaluated, so it has no team until the evaluation 11 ticks later, which gives
    // it the gunners' team. A port manned by an enemy of theirs leaves it with no team at the evaluation after.
    Species::RegionSimulation region = FlatRegion();
    const std::uint32_t slot = MannedTurret(region, SETTLED_TEAM, GUNNERS, MIDDLE.x, MIDDLE.z, 2);
    for (std::uint64_t tick = 0; tick < 11; ++tick)
    {
      region.Step(region.NextTick());
      Assert::IsFalse(TurretOf(region, slot).team.has_value(), L"no team before the second evaluation");
    }
    region.Step(region.NextTick());
    Assert::IsTrue(TurretOf(region, slot).team == std::optional<std::uint8_t>(GUNNERS), L"the gunners' team");
    Assert::IsTrue(region.Structures().At(slot).owner == std::optional<std::uint8_t>(SETTLED_TEAM), L"still the settlement's turret");

    // An enemy of theirs as a port's occupant, as the port evaluation of the tick before leaves it: the evaluation due
    // finds an operator who is no friend of the candidate's team.
    Species::Structure& turret = region.Structures().At(slot);
    turret.ports[3].occupant = PlaceFigure(region, ENEMIES, Species::FigureKind::Worker, turret.ports[3].position);
    if (turret.turret)
    {
      turret.turret->recountTicks = 1;
    }
    Species::AdvanceTurret(region, turret);
    Assert::IsFalse(TurretOf(region, slot).team.has_value(), L"an operator who is no friend leaves it with no team");
    Assert::IsFalse(TurretOf(region, slot).target.has_value(), L"and no target");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg419AGunTurretFiresFiveRoundsASecondForEachMannedPort)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-411")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-415")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-418")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-419")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-420")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg419AGunTurretFiresFiveRoundsASecondForEachMannedPort)
  {
    // An enemy held still 150 wu ahead is the turret's target from the tick it gets its team, within the fire cone of
    // its head. With four ports manned, the fire step fires two barrels a tick in cursor order, 20 rounds a second; with
    // one, the cursor passes the three unmanned barrels and it fires every second tick, 5 a second. Each round starts
    // half a tick of travel behind its muzzle and flies at 500 wu/s.
    for (const std::uint32_t manned : {4U, 1U})
    {
      Species::RegionSimulation region = FlatRegion();
      const std::uint32_t slot = MannedTurret(region, GUNNERS, GUNNERS, MIDDLE.x, MIDDLE.z, manned);
      const Neuron::Vec3 ahead = MIDDLE + Neuron::Vec3{0.0, 0.0, 150.0};
      const Species::ActorRef enemy = PlaceFigure(region, ENEMIES, Species::FigureKind::Worker, ahead);
      const std::int32_t health = region.Resolve(enemy)->health;
      const auto step = [&]
      {
        Hold(region, enemy, ahead);
        region.Resolve(enemy)->health = health; // never killed
        region.Step(region.NextTick());
      };
      for (std::uint64_t tick = 0; tick < 11; ++tick)
      {
        step();
        Assert::IsTrue(BarrelsFired(region).empty(), L"no fire without a team");
      }
      step();
      Assert::IsTrue(TurretOf(region, slot).target == std::optional<Species::ActorRef>(enemy), L"the enemy is its target");
      if (manned == 4)
      {
        Assert::IsTrue(BarrelsFired(region) == std::vector<std::uint32_t>{0, 1}, L"barrels 0 and 1 on its first fire step");
        step();
        Assert::IsTrue(BarrelsFired(region) == std::vector<std::uint32_t>{2, 3}, L"then 2 and 3");
      }
      std::size_t rounds = 0;
      for (int tick = 0; tick < 10; ++tick)
      {
        step();
        rounds += BarrelsFired(region).size();
      }
      ExpectEqual(5 * static_cast<std::int64_t>(manned), static_cast<std::int64_t>(rounds), L"5 rounds a second for each manned port");
      for (const Species::EffectEvent& event : region.Events())
      {
        if (event.kind == Species::EffectEventKind::TurretFired)
        {
          Assert::IsTrue(std::abs(Neuron::Length(event.vector) - CommittedTuning().ref.bldg.turret.gun.roundSpeed) < 1e-9,
                         L"the round's speed");
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg413AStructureBlocksATurretsLineOfSight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-411")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-413")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg413AStructureBlocksATurretsLineOfSight)
  {
    // A storehouse 60 wu ahead of the turret hides the enemy 120 wu ahead, so the turret takes the farther enemy in the
    // open as its target; the turret itself never blocks its own line of sight.
    Species::RegionSimulation region = FlatRegion();
    const std::uint32_t slot = MannedTurret(region, GUNNERS, GUNNERS, MIDDLE.x, MIDDLE.z, 4);
    static_cast<void>(
      region.AddStructure(Species::StructureKind::Storehouse, GUNNERS, std::nullopt, MIDDLE.x, MIDDLE.z + 60.0, {0.0, 0.0, 1.0}));
    const Species::Structure& turret = region.Structures().At(slot);
    const Neuron::Vec3 hidden = MIDDLE + Neuron::Vec3{0.0, 0.0, 120.0};
    const Neuron::Vec3 open = MIDDLE + Neuron::Vec3{180.0, 0.0, 0.0};
    Assert::IsFalse(Species::TurretSightClear(region, turret, turret.position, hidden), L"blocked by the storehouse");
    Assert::IsTrue(Species::TurretSightClear(region, turret, turret.position, open), L"clear beside it");
    const Species::ActorRef nearer = PlaceFigure(region, ENEMIES, Species::FigureKind::Worker, hidden);
    const Species::ActorRef farther = PlaceFigure(region, ENEMIES, Species::FigureKind::Worker, open);
    for (std::uint64_t tick = 0; tick < 12; ++tick)
    {
      Hold(region, nearer, hidden);
      Hold(region, farther, open);
      region.Step(region.NextTick());
    }
    Assert::IsTrue(TurretOf(region, slot).target == std::optional<Species::ActorRef>(farther), L"the enemy it can see");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate451ATurretRoundPiercesEveryEnemyItsStepHits)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-450")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-451")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate451ATurretRoundPiercesEveryEnemyItsStepHits)
  {
    // A round of team 0 fired along +z, 2 wu above the ground, passes three enemies and a friend in its path: each enemy
    // takes 15 damage, the friend none, and the round is removed after the sixth of its moves.
    Species::RegionSimulation region = FlatRegion();
    std::vector<std::pair<Species::ActorRef, Neuron::Vec3>> held;
    for (const double z : {40.0, 44.0, 120.0, 80.0})
    {
      const Neuron::Vec3 at = MIDDLE + Neuron::Vec3{0.0, 0.0, z};
      held.emplace_back(PlaceFigure(region, z == 80.0 ? GUNNERS : ENEMIES, Species::FigureKind::Worker, at), at);
    }
    const std::int32_t health = region.Resolve(held.front().first)->health;
    Species::FireTurretRound(region, GUNNERS, MIDDLE + Neuron::Vec3{0.0, 2.0, 0.0}, {0.0, 0.0, 500.0}, 0, MIDDLE, 0);
    for (int move = 0; move < 6; ++move)
    {
      ExpectEqual(1, static_cast<std::int64_t>(region.Effects().Count()), L"the round flies");
      for (const auto& [ref, at] : held)
      {
        Hold(region, ref, at);
      }
      region.Step(region.NextTick());
    }
    ExpectEqual(0, static_cast<std::int64_t>(region.Effects().Count()), L"removed when its life is spent");
    for (std::size_t enemy = 0; enemy < 3; ++enemy)
    {
      ExpectEqual(health - CommittedTuning().ref.weapon.turretRound.damage, region.Resolve(held[enemy].first)->health, L"every enemy hit");
    }
    ExpectEqual(health, region.Resolve(held.back().first)->health, L"never a friend");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp034AnExplosionDestroysATurretWhoseSiteIsRebuiltAtHalfItsCost)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-035")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-422")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-423")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp034AnExplosionDestroysATurretWhoseSiteIsRebuiltAtHalfItsCost)
  {
    // An explosion of range 10 and damage 40 at 10 wu takes 40 × (1 − 10 ÷ 30) = 26 of the turret's 60 health; a
    // second destroys it, leaving a construction site that is not queued and gains no work from its manned ports, and
    // that a third explosion does not harm. The settlement pays half the turret's cost to rebuild it, once; the site,
    // queued, is built by its port's Worker and stands again at full health with no team.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    std::uint64_t tick = 0;
    for (; tick < 60; ++tick)
    {
      world->Step(tick, pool); // the first Workers land
    }
    Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const std::uint32_t slot =
      region.AddStructure(Species::StructureKind::Turret, SETTLED_TEAM, std::nullopt, wellspring.x + 120.0, wellspring.z, {0.0, 0.0, 1.0});
    const Species::StableId id = region.Structures().At(slot).id;
    const Neuron::Vec3 position = region.Structures().At(slot).position;
    static_cast<void>(Species::Explode(region, position + Neuron::Vec3{10.0, 0.0, 0.0}, 10.0, 40.0, 0));
    ExpectEqual(34, TurretOf(region, slot).health, L"60 - 26");
    static_cast<void>(Species::Explode(region, position, 10.0, 100.0, SETTLED_TEAM));
    const Species::Structure& site = region.Structures().At(slot);
    Assert::IsFalse(site.built, L"destroyed, a construction site");
    Assert::IsTrue(std::ranges::any_of(region.Events(), [](const Species::EffectEvent& _event)
                                       { return _event.kind == Species::EffectEventKind::TurretDestroyed; }),
                   L"its head and barrels shatter");
    ExpectEqual(Species::MilliRuOf(200.0), site.costMilliRu, L"half its cost to rebuild");
    Assert::IsFalse(Species::Queued(region, site), L"not queued");
    static_cast<void>(Species::Explode(region, position, 10.0, 100.0, SETTLED_TEAM));
    ExpectEqual(60, TurretOf(region, slot).health, L"a site takes no damage");
    for (int step = 0; step < 20; ++step)
    {
      world->Step(tick++, pool); // the explosions' shockwave rings pass
    }

    const std::uint32_t worker = WorkersOf(region).front();
    Species::Figure& builder = region.TeamAt(SETTLED_TEAM).figures.At(worker);
    builder.position = site.ports[0].position;
    builder.velocity = {};
    builder.onGround = true;
    Species::SendToPort(builder, site, slot, 0);
    for (int step = 0; step < 3; ++step)
    {
      world->Step(tick++, pool);
    }
    Assert::IsTrue(region.Structures().At(slot).ports[0].occupant.has_value(), L"its port is manned");
    ExpectEqual(0, region.Structures().At(slot).workTicks, L"no work while it is not queued");

    const auto unpaid = Species::RebuildTurret(region, settlement.site, slot, id);
    Assert::IsTrue(!unpaid && unpaid.error() == Species::PlacementRefusal::CannotAfford, L"empty stores");
    settlement.storesMilliRu = Species::MilliRuOf(250.0);
    Assert::IsTrue(Species::RebuildTurret(region, settlement.site, slot, id).has_value(), L"rebuilt");
    ExpectEqual(Species::MilliRuOf(50.0), settlement.storesMilliRu, L"200 ru paid");
    const auto again = Species::RebuildTurret(region, settlement.site, slot, id);
    Assert::IsTrue(!again && again.error() == Species::PlacementRefusal::NotBuildable, L"queued once");
    world->Step(tick++, pool);
    ExpectEqual(1, region.Structures().At(slot).workTicks, L"its Worker builds it once queued");
    region.Structures().At(slot).workTicks = Species::RequiredWorkTicks(region.Structures().At(slot), CommittedTuning()) - 1;
    world->Step(tick++, pool);
    const Species::Structure& rebuilt = region.Structures().At(slot);
    Assert::IsTrue(rebuilt.built && !Species::Queued(region, rebuilt), L"built, out of the queue");
    ExpectEqual(60, TurretOf(region, slot).health, L"at full health");
    Assert::IsFalse(TurretOf(region, slot).team.has_value(), L"with no team until its next evaluation");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp034ATurretFiresOnTheSandboxsSideWithAStableHash)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-410")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp034ATurretFiresOnTheSandboxsSideWithAStableHash)
  {
    // The M2 plan's T5: a turret of the steward's side, 180 wu from the AI's side of the battle sandbox towards the
    // steward's and manned by four of its Workers, fires on the AI's side headless, and the world hashes alike on every
    // tick with 1 and 8 worker threads as serially.
    struct Run
    {
      std::uint64_t chain = 0;
      std::size_t rounds = 0;
      std::size_t killed = 0;
    };
    const auto run = [](std::size_t _workers)
    {
      Neuron::WorkerPool pool(_workers);
      Species::WorldSimulation world(Species::WorldSimulation::Desc{TURRET_TEST_KEY, 0, 1, &CommittedTuning(), 0, &TestRecords()});
      Species::RegionSimulation& region = world.Region(0);
      region.AttachTerrain(ValleyTerrain());
      Species::PopulateSandbox(region);
      const auto& sandbox = CommittedTuning().mvp.sandbox;
      const auto& sites = ValleyTerrain()->layout.sites;
      const Neuron::Vec3 ai = sites[static_cast<std::size_t>(sandbox.aiSite) % sites.size()];
      const Neuron::Vec3 steward = sites[static_cast<std::size_t>(sandbox.stewardSite) % sites.size()];
      const Neuron::Vec3 at = ai + Neuron::WithLength(Neuron::Vec3{steward.x - ai.x, 0.0, steward.z - ai.z}, 180.0);
      const std::uint32_t slot = MannedTurret(region, Species::SANDBOX_STEWARD_TEAM, Species::SANDBOX_STEWARD_TEAM, at.x, at.z, 4);
      const std::uint32_t before = region.TeamAt(Species::SANDBOX_AI_TEAM).figures.Count();
      Neuron::BitWriter chain;
      Run result;
      for (std::uint64_t tick = 0; tick < 200; ++tick)
      {
        world.Step(tick, pool);
        result.rounds += BarrelsFired(region).size();
        chain.WriteBits(Species::HashWorld(world).world, 64);
      }
      Assert::IsTrue(region.Structures().Used(slot), L"the turret stands");
      result.chain = Species::HashState(chain.Bytes());
      result.killed = before - region.TeamAt(Species::SANDBOX_AI_TEAM).figures.Count();
      return result;
    };
    std::vector<Run> runs(3);
    const std::vector<std::size_t> workerCounts{0, 1, 8};
    std::vector<std::function<void()>> jobs;
    static_cast<void>(ValleyTerrain());
    for (std::size_t index = 0; index < runs.size(); ++index)
    {
      jobs.emplace_back([&runs, &run, &workerCounts, index] { runs[index] = run(workerCounts[index]); });
    }
    RunConcurrently(jobs);
    Assert::IsTrue(runs[0].rounds > 0, L"the turret fires");
    Assert::IsTrue(runs[0].killed > 0, L"on the AI's side, which loses Workers");
    for (const Run& other : runs)
    {
      Assert::IsTrue(other.chain == runs[0].chain && other.rounds == runs[0].rounds, L"the same hashes on every tick");
    }
  }
};

} // namespace GameLogicTests
