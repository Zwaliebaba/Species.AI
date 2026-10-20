#include "pch.h"

#include "InProcessTransport.h"
#include "ServerHost.h"
#include "Snapshot.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey SNAPSHOT_TEST_KEY{0x0f0f0f0f0f0f0f0f, 0x7777777777777777};
constexpr Neuron::Nanoseconds SNAPSHOT_TEST_TICK_PERIOD{100'000'000}; // `ref.sim.tick_period`

// A world of three regions after 25 ticks, with ids issued by the world owner, so its state differs from a new world's.
std::unique_ptr<Species::WorldSimulation> SteppedWorld(Neuron::WorkerPool& _pool)
{
  auto world = std::make_unique<Species::WorldSimulation>(Species::WorldSimulation::Desc{SNAPSHOT_TEST_KEY, 0, 3});
  for (std::uint64_t tick = 0; tick < 25; ++tick)
  {
    world->Step(tick, _pool);
    if (tick % 5 == 0)
    {
      static_cast<void>(world->Ids().Issue());
    }
  }
  return world;
}

// The contents a world's snapshot holds, through the file's encoding.
Neuron::SnapshotContents ThroughTheFile(const Neuron::SnapshotContents& _contents)
{
  const auto decoded = Neuron::DecodeSnapshot(Neuron::EncodeSnapshot(_contents));
  Assert::IsTrue(decoded.has_value(), L"the snapshot decodes");
  return decoded.value_or(Neuron::SnapshotContents{});
}

bool RefusedAs(const Neuron::SnapshotContents& _contents, Neuron::SnapshotError _error)
{
  const auto restored = Species::RestoreWorld(_contents);
  return !restored.has_value() && restored.error() == _error;
}

// Runs a world server on a manual clock until it has completed `_ticks` ticks, then stops it, which freezes the world.
void RunAndFreeze(Species::WorldServer& _world, std::uint64_t _ticks)
{
  auto source = std::make_unique<Neuron::ManualTimeSource>();
  Neuron::ManualTimeSource* manual = source.get();
  const Neuron::PlatformClock clock(std::move(source));
  Neuron::ServerHost::Desc desc;
  desc.clock = &clock;
  desc.tickPeriod = SNAPSHOT_TEST_TICK_PERIOD;
  std::uint64_t waits = 0;
  desc.waitUntil = [&](Neuron::Nanoseconds _deadline)
  {
    // The ticks asked for, then the loop waits on its own clock until the test stops it.
    if (++waits < _ticks)
    {
      manual->Advance(_deadline - clock.Now());
    }
    else
    {
      std::this_thread::yield();
    }
  };
  Neuron::ServerHost host(std::move(desc), _world);
  host.Start();
  while (host.CompletedTicks() < _ticks)
  {
    std::this_thread::yield();
  }
  host.Stop();
}

} // namespace

TEST_CLASS(WorldSnapshotTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075ARestoredWorldIsTheWorldSaved)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075ARestoredWorldIsTheWorldSaved)
  {
    // The same state, the same key and the same tick, and from there the same ticks: the hashes agree after the
    // restore and after ten more ticks of each.
    Neuron::WorkerPool pool(2);
    const std::unique_ptr<Species::WorldSimulation> saved = SteppedWorld(pool);
    const Neuron::SnapshotContents contents = ThroughTheFile(Species::SnapshotOf(*saved));
    ExpectEqual(Species::WORLD_STATE_VERSION, contents.stateVersion, L"the current state version");
    ExpectEqual(25, static_cast<std::int64_t>(contents.tick), L"the first tick the restored world steps");
    auto restored = Species::RestoreWorld(contents);
    Assert::IsTrue(restored.has_value(), L"the world restores");
    Species::WorldSimulation& world = **restored;
    Assert::IsTrue(world.SimulationKey().low == SNAPSHOT_TEST_KEY.low && world.SimulationKey().high == SNAPSHOT_TEST_KEY.high,
                   L"the simulation key is kept");
    ExpectEqual(3, static_cast<std::int64_t>(world.RegionCount()), L"the regions");
    Assert::IsTrue(Species::HashWorld(world).world == Species::HashWorld(*saved).world, L"the same state");
    for (std::uint64_t tick = 25; tick < 35; ++tick)
    {
      saved->Step(tick, pool);
      world.Step(tick, pool);
    }
    Assert::IsTrue(Species::HashWorld(world).world == Species::HashWorld(*saved).world, L"the same ticks after it");
    Assert::IsTrue(world.Ids().Issue() == saved->Ids().Issue(), L"the same next id");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AStateVersionItCannotMigrateIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AStateVersionItCannotMigrateIsRefused)
  {
    Neuron::WorkerPool pool(0);
    Neuron::SnapshotContents contents = Species::SnapshotOf(*SteppedWorld(pool));
    for (const std::uint32_t version : {Species::WORLD_STATE_OLDEST_VERSION - 1, Species::WORLD_STATE_VERSION + 1})
    {
      contents.stateVersion = version;
      Assert::IsTrue(RefusedAs(contents, Neuron::SnapshotError::StateVersion), L"a version below the oldest or after the current");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AStateThatDoesNotDecodeWholeIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AStateThatDoesNotDecodeWholeIsRefused)
  {
    Neuron::WorkerPool pool(0);
    const Neuron::SnapshotContents contents = Species::SnapshotOf(*SteppedWorld(pool));
    for (std::size_t length = 0; length < contents.state.size(); ++length)
    {
      Neuron::SnapshotContents shorter = contents;
      shorter.state.resize(length);
      Assert::IsTrue(RefusedAs(shorter, Neuron::SnapshotError::MalformedState), L"a state cut short");
    }
    Neuron::SnapshotContents longer = contents;
    longer.state.push_back(std::byte{0xff});
    Assert::IsTrue(RefusedAs(longer, Neuron::SnapshotError::MalformedState), L"a state with bytes after it");
    Neuron::SnapshotContents otherTick = contents;
    otherTick.tick += 1;
    Assert::IsTrue(RefusedAs(otherTick, Neuron::SnapshotError::MalformedState), L"a header tick the state does not hold");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075AFrozenWorldOpensWhereItStopped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075AFrozenWorldOpensWhereItStopped)
  {
    // A server that stops saves its world; the next server given the same folder opens it at the tick it froze before,
    // with the same key, and runs on from there.
    const TemporaryFolder folder;
    Species::WorldServer::Desc desc;
    desc.snapshotFolder = folder.Path() / L"saves";
    Species::WorldServer first(desc);
    RunAndFreeze(first, 5);
    Assert::IsTrue(first.Simulation() != nullptr, L"the first server ran");
    const std::uint64_t frozenTick = first.Simulation()->NextTick();
    const Neuron::SipHashKey key = first.Simulation()->SimulationKey();
    const std::uint64_t hash = Species::HashWorld(*first.Simulation()).world;
    ExpectEqual(1, static_cast<std::int64_t>(Neuron::SnapshotStore(desc.snapshotFolder, 1).Snapshots().size()), L"one snapshot");

    Species::WorldServer second(desc);
    ExpectEqual(static_cast<std::int64_t>(frozenTick), static_cast<std::int64_t>(second.Open()),
                L"the next tick is the one it froze before");
    Assert::IsTrue(second.Simulation() != nullptr && second.Simulation()->SimulationKey().low == key.low &&
                     second.Simulation()->SimulationKey().high == key.high,
                   L"the same world's key");
    Assert::IsTrue(Species::HashWorld(*second.Simulation()).world == hash, L"the same state");

    Species::WorldServer third(desc);
    RunAndFreeze(third, 3);
    Assert::IsTrue(third.Simulation() != nullptr, L"the third server ran");
    ExpectEqual(static_cast<std::int64_t>(frozenTick + 3), static_cast<std::int64_t>(third.Simulation()->NextTick()), L"it ran on");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075WithNoSnapshotThatValidatesTheWorldOpensNew)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075WithNoSnapshotThatValidatesTheWorldOpensNew)
  {
    const TemporaryFolder folder;
    Species::WorldServer::Desc desc;
    desc.snapshotFolder = folder.Path() / L"saves";
    WriteText(desc.snapshotFolder / L"world-00000000000000000040.snapshot", "not a snapshot");
    Species::WorldServer world(desc);
    ExpectEqual(0, static_cast<std::int64_t>(world.Open()), L"a new world starts at tick 0");
  }
};

} // namespace GameLogicTests
