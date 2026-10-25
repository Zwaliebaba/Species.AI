#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "RegionSimulation.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldPace.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // the steward's settlement at site 2, whose team is its site

// M3's valley (ADR-157) at _pace, on _tuning, which outlives the world.
std::unique_ptr<Species::WorldSimulation> ValleyAt(Species::WorldPace _pace, const Species::TuningMvp& _tuning = CommittedTuning())
{
  Species::WorldStart start;
  start.key = SETTLED_TEST_KEY;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = SETTLED_TEAM;
  start.aiSettlements = 3;
  start.pace = _pace;
  return Species::StartWorld(start, ValleyTerrain(), _tuning, &TestRecords());
}

// The steward's settlement alone at site 2, at _pace, on _tuning.
std::unique_ptr<Species::WorldSimulation> LoneAt(Species::WorldPace _pace, const Species::TuningMvp& _tuning)
{
  Species::WorldStart start;
  start.key = SETTLED_TEST_KEY;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = SETTLED_TEAM;
  start.pace = _pace;
  return Species::StartWorld(start, ValleyTerrain(), _tuning, &TestRecords());
}

// The tick at which the lone settlement first holds _workers, stepping it at most _limit ticks; none when it does not.
std::optional<std::uint64_t> TickOfPopulation(Species::WorldSimulation& _world, std::uint32_t _workers, std::uint64_t _limit)
{
  Neuron::WorkerPool pool(0);
  const Species::RegionSimulation& region = _world.Region(0);
  for (std::uint64_t tick = _world.NextTick(); tick < _limit; ++tick)
  {
    _world.Step(tick, pool);
    if (Species::Population(region, *region.SettlementAt(SETTLED_TEAM)) >= _workers)
    {
      return tick;
    }
  }
  return std::nullopt;
}

// The world's hash after each of _ticks ticks on _workers threads.
std::vector<std::uint64_t> HashesOver(Species::WorldSimulation& _world, std::uint64_t _ticks, std::size_t _workers)
{
  Neuron::WorkerPool pool(_workers);
  std::vector<std::uint64_t> hashes;
  for (std::uint64_t step = 0; step < _ticks; ++step)
  {
    _world.Step(_world.NextTick(), pool);
    hashes.push_back(Species::HashWorld(_world).world);
  }
  return hashes;
}

} // namespace

TEST_CLASS(PaceTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil104AnAlwaysOnWorldKeepsItsPaceThroughItsSnapshotAndItsJournal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-104")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil104AnAlwaysOnWorldKeepsItsPaceThroughItsSnapshotAndItsJournal)
  {
    // ADR-184: the world reads the record at its pace from its start; its run is 24 times as many ticks, and its Wellspring
    // grows a Worker every 288 s. A snapshot and a journal name the pace, so the world continues and replays at it.
    const std::unique_ptr<Species::WorldSimulation> session = ValleyAt(Species::WorldPace::Session);
    const std::unique_ptr<Species::WorldSimulation> alwaysOn = ValleyAt(Species::WorldPace::AlwaysOn);
    Assert::IsTrue(session->Pace() == Species::WorldPace::Session && alwaysOn->Pace() == Species::WorldPace::AlwaysOn, L"each its pace");
    Assert::IsTrue(alwaysOn->Tuning().mvp.essence.wellspringIntervalS == 288.0, L"the Wellspring's interval at the always-on pace");
    Assert::IsTrue(session->Tuning().mvp.essence.wellspringIntervalS == CommittedTuning().mvp.essence.wellspringIntervalS,
                   L"the record unchanged at the session pace");
    ExpectEqual(static_cast<std::int64_t>(24 * session->RunState().endTick), static_cast<std::int64_t>(alwaysOn->RunState().endTick),
                L"the run's end at 24 times its ticks");
    static_cast<void>(HashesOver(*alwaysOn, 20, 0));

    auto restored = Species::RestoreWorld(Species::SnapshotOf(*alwaysOn), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"the always-on world restores");
    Assert::IsTrue((*restored)->Pace() == Species::WorldPace::AlwaysOn, L"at its pace");
    Assert::IsTrue((*restored)->Tuning().mvp.run.lengthHours == alwaysOn->Tuning().mvp.run.lengthHours, L"reading the record at it");
    (*restored)->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(HashesOver(**restored, 30, 0) == HashesOver(*alwaysOn, 30, 0), L"and runs on as the world it was");

    Species::CommandJournal journal;
    journal.start.key = SETTLED_TEST_KEY;
    journal.start.seed = ValleyTerrain()->seed;
    journal.start.stewardSite = SETTLED_TEAM;
    journal.start.pace = Species::WorldPace::AlwaysOn;
    const std::optional<Species::CommandJournal> read = Species::ReadJournal(Species::WriteJournal(journal));
    Assert::IsTrue(read.has_value() && read->start.pace == Species::WorldPace::AlwaysOn, L"the journal's start names the pace");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AWorldOfVersion17ContinuesAtTheSessionPace)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-104")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AWorldOfVersion17ContinuesAtTheSessionPace)
  {
    // Version 17's layout: today's without the pace after the region count. Every world saved before M4 ran at the session
    // pace, so it continues at it, as the world it was (M4 plan §4).
    const std::unique_ptr<Species::WorldSimulation> world = ValleyAt(Species::WorldPace::Session);
    static_cast<void>(HashesOver(*world, 20, 0));
    // A world saved before version 23 holds no gatherer's home port (ADR-195), so the world it was had none.
    for (std::size_t team = 0; team < world->Region(0).TeamCount(); ++team)
    {
      auto& figures = world->Region(0).TeamAt(team).figures;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (figures.Used(slot))
        {
          figures.At(slot).homeStructureId = 0;
          figures.At(slot).homeStructureSlot = 0;
          figures.At(slot).homePortIndex = 0;
        }
      }
    }
    Neuron::BitWriter state;
    state.WriteBits(world->SimulationKey().low, 64);
    state.WriteBits(world->SimulationKey().high, 64);
    state.WriteBits(world->RegionCount(), 16);
    world->WriteState(state);
    // The region's state without its last sections, version 19's absences and version 20's deputies, both empty: the
    // steward was never away; and version 23's home ports, none.
    Neuron::BitWriter region;
    world->Region(0).WriteState(region);
    Neuron::BitWriter later;
    later.WriteBounded(0, Species::ABSENCES_MAX);
    later.WriteBounded(0, 4);
    for (std::size_t team = 0; team < world->Region(0).TeamCount(); ++team)
    {
      const auto& figures = world->Region(0).TeamAt(team).figures;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (figures.Used(slot))
        {
          Species::WriteHomePort(later, figures.At(slot));
        }
      }
    }
    const std::size_t bits = region.BitCount() - later.BitCount();
    Neuron::BitReader regionBits(region.Bytes());
    for (std::size_t done = 0; done < bits;)
    {
      const auto chunk = static_cast<std::uint32_t>(std::min<std::size_t>(64, bits - done));
      state.WriteBits(regionBits.ReadBits(chunk), chunk);
      done += chunk;
    }
    Species::WriteWorldRecord(state, world->Record());
    auto restored = Species::RestoreWorld(Neuron::SnapshotContents{17, world->NextTick(), state.Take()}, CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"a version-17 world restores");
    Assert::IsTrue((*restored)->Pace() == Species::WorldPace::Session, L"at the session pace");
    (*restored)->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(HashesOver(**restored, 30, 0) == HashesOver(*world, 30, 0), L"as the world it was");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil101AnAlwaysOnSettlementGrowsAtTheSessionFactorsRate)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil101AnAlwaysOnSettlementGrowsAtTheSessionFactorsRate)
  {
    // ADR-184: at the always-on pace a lone settlement's economy runs at 1 / mvp.pace.session_factor of the session's rate,
    // while walking and gathering at a port take the same ticks. Its population therefore reaches 24 Workers at nearly the
    // factor times the session's ticks, below it by the fixed walk of the first loads (3.72 on this valley). A factor of 4
    // keeps the test short; on the committed 24 the ratio at 30 Workers on this valley is 22.5 (ADR-184).
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.pace.sessionFactor = 4.0;
    const std::unique_ptr<Species::WorldSimulation> session = LoneAt(Species::WorldPace::Session, tuning);
    const std::unique_ptr<Species::WorldSimulation> alwaysOn = LoneAt(Species::WorldPace::AlwaysOn, tuning);
    const std::optional<std::uint64_t> sessionTick = TickOfPopulation(*session, 24, 30'000);
    Assert::IsTrue(sessionTick.has_value(), L"24 Workers at the session pace");
    const std::uint64_t sessionTicks = sessionTick.value_or(1);
    const std::optional<std::uint64_t> alwaysOnTick = TickOfPopulation(*alwaysOn, 24, 6 * sessionTicks);
    Assert::IsTrue(alwaysOnTick.has_value(), L"24 Workers at the always-on pace");
    const double ratio = static_cast<double>(alwaysOnTick.value_or(0)) / static_cast<double>(sessionTicks);
    Assert::IsTrue(ratio > 3.6 && ratio <= 4.0, std::format(L"24 Workers at {:.3f} times the session's ticks", ratio).c_str());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094AnAlwaysOnValleyHashesAlikeOnEveryNumberOfWorkers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094AnAlwaysOnValleyHashesAlikeOnEveryNumberOfWorkers)
  {
    std::vector<std::vector<std::uint64_t>> hashes(3);
    const std::size_t workers[]{1, 2, 8};
    for (std::size_t index = 0; index < hashes.size(); ++index)
    {
      const std::unique_ptr<Species::WorldSimulation> world = ValleyAt(Species::WorldPace::AlwaysOn);
      hashes[index] = HashesOver(*world, 300, workers[index]);
    }
    Assert::IsTrue(hashes[0] == hashes[1] && hashes[1] == hashes[2], L"the same hash on every tick at 1, 2 and 8 worker threads");
  }
};

} // namespace GameLogicTests
