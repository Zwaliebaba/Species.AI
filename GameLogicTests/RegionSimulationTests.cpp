#include "pch.h"

#include "CommandQueue.h"
#include "DrawPurpose.h"
#include "Figure.h"
#include "RegionSimulation.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey FIGURE_TEST_KEY{0x1234567812345678, 0x0badc0de0badc0de};

// A region of its own, at tick 0, on the committed tuning and a level terrain 50 wu above the sea.
Species::RegionSimulation MakeRegion()
{
  return Species::RegionSimulation(0, FIGURE_TEST_KEY, 0, CommittedTuning(), FlatTerrain());
}

std::vector<std::uint32_t> SpawnAt(Species::RegionSimulation& _region, std::uint8_t _team, Species::FigureKind _kind, std::uint32_t _count,
                                   const Neuron::Vec3& _velocity = {})
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.kind = _kind;
  spawn.position = {35000.0, 50.0, 35000.0};
  spawn.velocity = _velocity;
  spawn.count = _count;
  return _region.SpawnFigures(spawn);
}

Species::Figure& FigureAt(Species::RegionSimulation& _region, std::uint8_t _team, std::uint32_t _slot)
{
  return _region.TeamAt(_team).figures.At(_slot);
}

} // namespace

TEST_CLASS(RegionSimulationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit011AFiguresKindSetsItsStats)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit011AFiguresKindSetsItsStats)
  {
    Species::RegionSimulation region = MakeRegion();
    const std::uint32_t worker = SpawnAt(region, 0, Species::FigureKind::Worker, 1).front();
    const std::uint32_t leader = SpawnAt(region, 0, Species::FigureKind::Leader, 1).front();
    const Species::Figure& w = FigureAt(region, 0, worker);
    const Species::Figure& l = FigureAt(region, 0, leader);
    ExpectEqual(50, w.health, L"a Worker's health");
    Assert::IsTrue(w.speed == 20.0, L"a Worker's speed");
    ExpectEqual(30, w.reloadIntervalTicks, L"3 s of reload is 30 ticks");
    Assert::IsTrue(w.reloadTimerTicks >= 0 && w.reloadTimerTicks <= 30, L"R-UNIT-015: an initial reload of 0 to 30 ticks");
    ExpectEqual(255, l.health, L"a Leader's health");
    Assert::IsTrue(l.speed == 30.0, L"a Leader's speed");
    Assert::IsTrue(w.id != 0 && l.id != 0 && w.id != l.id && Species::OwnerOf(w.id) == region.Owner(), L"stable ids of the region");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit015TheInitialReloadIsADrawOfItsOwnPurpose)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-015")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit015TheInitialReloadIsADrawOfItsOwnPurpose)
  {
    // ADR-082: the timer is ⌈NextUnit(rate) ÷ tick period⌉ of the context (ReloadTimer, the figure's id, the tick).
    Species::RegionSimulation region = MakeRegion();
    const std::vector<std::uint32_t> slots = SpawnAt(region, 0, Species::FigureKind::Worker, 50);
    std::map<std::int32_t, int> timers;
    for (const std::uint32_t slot : slots)
    {
      const Species::Figure& figure = FigureAt(region, 0, slot);
      Neuron::SimRng draw(FIGURE_TEST_KEY, Species::PurposeOf(Species::DrawPurpose::ReloadTimer), figure.id, 0);
      const double seconds = draw.NextUnit(3.0);
      ExpectEqual(static_cast<std::int64_t>(std::ceil(seconds / 0.1)), figure.reloadTimerTicks, L"the keyed draw");
      ++timers[figure.reloadTimerTicks];
    }
    Assert::IsTrue(timers.size() > 10, L"fifty Workers do not share a few timers");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit019AFiguresFrontFollowsItsVelocityOrItsKind)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-019")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit019AFiguresFrontFollowsItsVelocityOrItsKind)
  {
    // AC-11 of the containers, R-UNIT-059: a Worker at rest faces a drawn horizontal direction, a Leader at rest the
    // default front, and either one with a velocity faces along it.
    Species::RegionSimulation region = MakeRegion();
    const Species::Figure& worker = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Worker, 1).front());
    Assert::IsTrue(worker.front.y == 0.0 && std::abs(Neuron::Length(worker.front) - 1.0) < 1e-12, L"a horizontal unit front");
    Neuron::SimRng draw(FIGURE_TEST_KEY, Species::PurposeOf(Species::DrawPurpose::SpawnFront), worker.id, 0);
    const double x = draw.NextSigned(1.0);
    const double z = draw.NextSigned(1.0);
    Assert::IsTrue(worker.front == Neuron::NormalizedOr({x, 0.0, z}, {}), L"from x, then z, of its own context");
    const Species::Figure& leader = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Leader, 1).front());
    Assert::IsTrue(leader.front == Neuron::Vec3{1.0, 0.0, 0.0}, L"ref.sim.spawn.default_front");
    const Species::Figure& moving = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Worker, 1, {0.0, 3.0, 4.0}).front());
    Assert::IsTrue(std::abs(moving.front.x) < 1e-15 && std::abs(moving.front.y - 0.6) < 1e-15 && std::abs(moving.front.z - 0.8) < 1e-15,
                   L"the normalised velocity, its vertical part included");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit026AHealthChangeHurtsHealsAndKills)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-026")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit026AHealthChangeHurtsHealsAndKills)
  {
    const Species::TuningMvp& tuning = CommittedTuning();
    Species::RegionSimulation region = MakeRegion();
    Species::Figure& worker = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Worker, 1).front());
    Species::HealthChange change = Species::ApplyHealthChange(worker, -20, Species::DamageKind::Laser, tuning, false);
    Assert::IsTrue(change.registered && !change.killed, L"a hit that does not kill");
    ExpectEqual(30, worker.health, L"50 - 20");
    change = Species::ApplyHealthChange(worker, 1000, Species::DamageKind::Unresistable, tuning, false);
    ExpectEqual(255, worker.health, L"R-UNIT-012: never above ref.unit.health_max");
    change = Species::ApplyHealthChange(worker, -255, Species::DamageKind::Fire, tuning, false);
    Assert::IsTrue(change.registered && change.killed && worker.dead, L"0 or below kills");
    ExpectEqual(100, worker.health, L"the health field becomes the corpse fade");
    change = Species::ApplyHealthChange(worker, -10, Species::DamageKind::Laser, tuning, false);
    Assert::IsTrue(change.registered && !change.killed, L"a dead figure's change registers and does nothing");
    ExpectEqual(100, worker.health, L"the fade is untouched");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit116AWorkerInAFormationTakesLessExplosiveDamage)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-116")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit116AWorkerInAFormationTakesLessExplosiveDamage)
  {
    // R-UNIT-027: × ref.unit.worker.formation_explosive_factor, truncated toward zero; a Leader takes it in full.
    const Species::TuningMvp& tuning = CommittedTuning();
    Species::RegionSimulation region = MakeRegion();
    Species::Figure& worker = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Worker, 1).front());
    static_cast<void>(Species::ApplyHealthChange(worker, -31, Species::DamageKind::Explosive, tuning, true));
    ExpectEqual(50 - 18, worker.health, L"-31 × 0.6 is -18.6, truncated to -18");
    static_cast<void>(Species::ApplyHealthChange(worker, -10, Species::DamageKind::Laser, tuning, true));
    ExpectEqual(32 - 10, worker.health, L"a laser in full");
    Species::Figure& leader = FigureAt(region, 0, SpawnAt(region, 0, Species::FigureKind::Leader, 1).front());
    static_cast<void>(Species::ApplyHealthChange(leader, -31, Species::DamageKind::Explosive, tuning, true));
    ExpectEqual(255 - 31, leader.health, L"a Leader in full");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit034ACorpseFadesThenGoes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit034ACorpseFadesThenGoes)
  {
    // 25 ticks on the ground and 50 in the air; a destroyed figure goes at its next advance, with no corpse (R-SIM-113).
    Species::RegionSimulation region = MakeRegion();
    const std::vector<std::uint32_t> slots = SpawnAt(region, 0, Species::FigureKind::Worker, 3);
    Species::Figure& onGround = FigureAt(region, 0, slots[0]);
    onGround.onGround = true;
    static_cast<void>(Species::ApplyHealthChange(onGround, -100, Species::DamageKind::Laser, CommittedTuning(), false));
    static_cast<void>(
      Species::ApplyHealthChange(FigureAt(region, 0, slots[1]), -100, Species::DamageKind::Laser, CommittedTuning(), false));
    FigureAt(region, 0, slots[2]).destroyed = true;
    region.Step(0);
    Assert::IsFalse(region.TeamAt(0).figures.Used(slots[2]), L"the destroyed figure went at once");
    for (std::uint64_t tick = 1; tick < 25; ++tick)
    {
      region.Step(tick);
    }
    Assert::IsFalse(region.TeamAt(0).figures.Used(slots[0]), L"the corpse on the ground went on its 25th tick");
    Assert::IsTrue(region.TeamAt(0).figures.Used(slots[1]), L"the one in the air fades on");
    for (std::uint64_t tick = 25; tick < 50; ++tick)
    {
      region.Step(tick);
    }
    Assert::IsFalse(region.TeamAt(0).figures.Used(slots[1]), L"and went on its 50th");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim515EveryFigureAdvancesOncePerTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-515")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim515EveryFigureAdvancesOncePerTick)
  {
    // The reload timer counts down once per advance, so it shows one advance per tick in every build.
    Species::RegionSimulation region = MakeRegion();
    const std::vector<std::uint32_t> first = SpawnAt(region, 0, Species::FigureKind::Worker, 250);
    const std::vector<std::uint32_t> second = SpawnAt(region, 1, Species::FigureKind::Worker, 130);
    for (std::uint32_t slot : first)
    {
      FigureAt(region, 0, slot).reloadTimerTicks = 20;
    }
    for (std::uint32_t slot : second)
    {
      FigureAt(region, 1, slot).reloadTimerTicks = 20;
    }
    for (std::uint64_t tick = 0; tick < 7; ++tick)
    {
      region.Step(tick);
    }
    for (std::uint32_t slot : first)
    {
      ExpectEqual(13, FigureAt(region, 0, slot).reloadTimerTicks, L"seven advances in seven ticks");
    }
    for (std::uint32_t slot : second)
    {
      ExpectEqual(13, FigureAt(region, 1, slot).reloadTimerTicks, L"in every team");
    }
    ExpectEqual(7, static_cast<std::int64_t>(region.NextTick()), L"the region's tick clock");
  }

#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(RSim218TeamsAdvanceInIndexOrderWithinEachSlice)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-218")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim218TeamsAdvanceInIndexOrderWithinEachSlice)
  {
    // AC-7 of the containers without groups, which the MVP does not build: in each slice every figure of team 0
    // advances before any of team 1, the slices run 0 to 9, and each figure advances once.
    Species::RegionSimulation region = MakeRegion();
    static_cast<void>(SpawnAt(region, 0, Species::FigureKind::Worker, 120));
    static_cast<void>(SpawnAt(region, 1, Species::FigureKind::Leader, 45));
    struct Seen
    {
      std::uint8_t team;
      std::uint32_t slot;
      std::uint32_t slice;
    };
    std::vector<Seen> seen;
    region.ObserveAdvances([&](std::uint8_t _team, std::uint32_t _slot, std::uint32_t _slice) { seen.push_back({_team, _slot, _slice}); });
    region.Step(0);
    ExpectEqual(165, static_cast<std::int64_t>(seen.size()), L"each figure once");
    for (std::size_t n = 1; n < seen.size(); ++n)
    {
      const Seen& a = seen[n - 1];
      const Seen& b = seen[n];
      Assert::IsTrue(a.slice < b.slice || (a.slice == b.slice && (a.team < b.team || (a.team == b.team && a.slot < b.slot))),
                     L"by slice, then team, then slot");
    }
  }
#endif

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AFigureSavedBeforeT4RegistersWhereItStands)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AFigureSavedBeforeT4RegistersWhereItStands)
  {
    // M1 task T2's layout of a region with one Worker in slot 0 of team 0 (ADR-101): it restores Idle, with its waypoint
    // at its position, and the actor grid registers it in the cell of its position, with no grid history before it.
    constexpr std::uint64_t TICK = 30;
    const Neuron::Vec3 position{33000.0, 50.2, 34000.0};
    const auto writeVec3 = [](Neuron::BitWriter& _writer, const Neuron::Vec3& _value)
    {
      for (const double component : {_value.x, _value.y, _value.z})
      {
        _writer.WriteBits(std::bit_cast<std::uint64_t>(component), 64);
      }
    };
    const Species::StableId id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 1);
    Neuron::BitWriter state;
    state.WriteBits(0, 16);
    state.WriteBits(TICK, 64);
    state.WriteBits(2, 64);
    state.WriteBounded(0, Neuron::COMMAND_QUEUE_STATE_MAX);
    state.WriteBounded(1, Species::RegionSimulation::TEAM_SLOTS);
    state.WriteBits(1, 32); // the pool: capacity, last advanced slot and an empty free list
    state.WriteBits(0, 32);
    state.WriteBits(0, 32);
    state.WriteBool(true);
    state.WriteBits(id, 64); // the figure of version 3
    state.WriteBounded(static_cast<std::uint64_t>(Species::FigureKind::Worker), Species::FIGURE_KIND_MAX);
    writeVec3(state, position);
    writeVec3(state, {});
    writeVec3(state, {1.0, 0.0, 0.0});
    writeVec3(state, position);
    state.WriteBits(std::bit_cast<std::uint64_t>(0.0), 64);
    state.WriteBits(std::bit_cast<std::uint64_t>(20.0), 64);
    state.WriteSigned(50, 32);
    state.WriteSigned(30, 32);
    state.WriteSigned(0, 32);
    for (const bool flag : {true, true, false, false, false})
    {
      state.WriteBool(flag);
    }
    Species::RegionSimulation region(0, FIGURE_TEST_KEY, TICK, CommittedTuning());
    Neuron::BitReader reader(state.Bytes());
    Assert::IsTrue(region.ReadState(reader, 3) && reader.Finish(), L"a version-3 region reads whole");
    const Species::Figure& worker = region.TeamAt(0).figures.At(0);
    Assert::IsTrue(worker.waypoint == position && worker.workerState == Species::WorkerState::Idle && worker.radius == 0.0,
                   L"Idle in place");
    Assert::IsTrue(worker.cells == region.Grid().FootprintOf(position.x, position.z, 0.0), L"registered where it stands");
    Assert::IsTrue(region.Grid().EntriesAt(0, worker.cells.x0, worker.cells.z0) == std::vector<Species::ActorRef>{{id, 0, 0}},
                   L"its entry");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093FiguresAreStateTheHashAndTheSnapshotCover)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093FiguresAreStateTheHashAndTheSnapshotCover)
  {
    // A world with figures gives the same hashes at 0, 1, 2 and 8 workers, a changed figure changes its region's hash,
    // and its region's state reads back to the same bytes.
    const auto run = [](std::size_t _workers)
    {
      Neuron::WorkerPool pool(_workers);
      Species::WorldSimulation world({FIGURE_TEST_KEY, 0, 3, &CommittedTuning()});
      for (std::uint16_t index = 0; index < 3; ++index)
      {
        world.Region(index).AttachTerrain(FlatTerrain());
        static_cast<void>(SpawnAt(world.Region(index), 0, Species::FigureKind::Worker, 40u + index));
        static_cast<void>(SpawnAt(world.Region(index), 1, Species::FigureKind::Leader, 2));
      }
      Neuron::BitWriter chain;
      for (std::uint64_t tick = 0; tick < 60; ++tick)
      {
        world.Step(tick, pool);
        chain.WriteBits(Species::HashWorld(world).world, 64);
      }
      return Species::HashState(chain.Bytes());
    };
    const std::uint64_t serial = run(0);
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      Assert::IsTrue(run(workers) == serial, L"the same hashes at every worker count");
    }

    Species::RegionSimulation region = MakeRegion();
    const std::vector<std::uint32_t> slots = SpawnAt(region, 0, Species::FigureKind::Worker, 5);
    region.Step(0);
    Neuron::BitWriter before;
    region.WriteState(before);
    Species::RegionSimulation copy(0, FIGURE_TEST_KEY, 1, CommittedTuning());
    Neuron::BitReader reader(before.Bytes());
    Assert::IsTrue(copy.ReadState(reader, Species::WORLD_STATE_VERSION), L"the region reads whole");
    Neuron::BitWriter again;
    copy.WriteState(again);
    Assert::IsTrue(std::ranges::equal(before.Bytes(), again.Bytes()), L"the same state");
    FigureAt(region, 0, slots[3]).position.x += 1.0;
    Neuron::BitWriter after;
    region.WriteState(after);
    Assert::IsTrue(Species::HashState(after.Bytes()) != Species::HashState(before.Bytes()), L"a figure's position is hashed");
  }
};

} // namespace GameLogicTests
