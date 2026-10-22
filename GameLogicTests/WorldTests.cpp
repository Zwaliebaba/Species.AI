#include "pch.h"

#include "CommandMessages.h"
#include "Development.h"
#include "InProcessTransport.h"
#include "IntegratedServer.h"
#include "LooseContent.h"
#include "PresentationEvents.h"
#include "RegionTerrain.h"
#include "ServerHost.h"
#include "ServerStats.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "Tuning.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldTerrain.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey TEST_KEY{0x1111111111111111, 0x2222222222222222};
constexpr Neuron::Nanoseconds TEST_TICK_PERIOD{100'000'000}; // `ref.sim.tick_period`

// The repository's tuning record with one value's text replaced, under a temporary content root.
std::filesystem::path ContentRootWith(const TemporaryFolder& _folder, std::string_view _from, std::string_view _to)
{
  std::string text = ReadText(RepositoryContentRoot() / Species::TUNING_MVP_FILE);
  const std::size_t at = text.find(_from);
  Assert::IsTrue(at != std::string::npos, L"the committed record holds the text to replace");
  text.replace(at, _from.size(), _to);
  WriteText(_folder.Path() / Species::TUNING_MVP_FILE, text);
  return _folder.Path();
}

// That record cooked as the content build cooks it, in a loose-content folder of its own, mounted (ADR-108), with the
// structure records the tests write unless _structures is false (ADR-129).
Neuron::LooseContent CookedContentWith(const TemporaryFolder& _folder, std::string_view _from, std::string_view _to,
                                       bool _structures = true)
{
  const auto tuning = Species::ReadTuningMvpSource(ContentRootWith(_folder, _from, _to));
  Assert::IsTrue(tuning.has_value(), L"the changed record reads whole");
  Neuron::CookedEntry cooked;
  cooked.entry.key = std::string(Species::TUNING_MVP_KEY);
  cooked.entry.type = Neuron::ContentType::Record;
  cooked.entry.schemaId = std::string(Species::TUNING_MVP_SCHEMA_ID);
  cooked.entry.schemaVersion = Species::TUNING_MVP_VERSION;
  cooked.entry.simulationRelevant = Species::TUNING_MVP_SIMULATION_RELEVANT;
  cooked.bytes = Species::CookTuningMvp(tuning.value_or(Species::TuningMvp{}));
  std::vector<Neuron::CookedEntry> entries = _structures ? StructureRecordEntries() : std::vector<Neuron::CookedEntry>{};
  entries.push_back(std::move(cooked));
  Assert::IsTrue(Neuron::WriteLooseContent(_folder.Path() / L"content", std::move(entries)).has_value(), L"the folder is written");
  std::expected<Neuron::LooseContent, Neuron::AssetError> content = Neuron::LooseContent::Mount(_folder.Path() / L"content");
  Assert::IsTrue(content.has_value(), L"the folder mounts");
  return std::move(*content);
}

// What a client end has received, in order: the world's terrain, the ticks of the state updates and its commands'
// outcomes.
struct Arrivals
{
  std::vector<Species::WorldTerrain> terrains;
  std::vector<std::uint64_t> ticks;
  std::vector<Species::CommandReport> reports;
  std::vector<Species::ServerStats> stats;
};

Arrivals TakeArrivals(Neuron::Transport& _client)
{
  Arrivals arrivals;
  Neuron::MessageDispatcher dispatcher;
  dispatcher.Register<Species::WorldTerrain>([&arrivals](const Species::WorldTerrain& _terrain) { arrivals.terrains.push_back(_terrain); });
  dispatcher.Register<Species::StateUpdate>(
    [&arrivals](const Species::StateUpdate& _update)
    {
      if (arrivals.ticks.empty() || arrivals.ticks.back() != _update.tick)
      {
        arrivals.ticks.push_back(_update.tick);
      }
    });
  dispatcher.Register<Species::PresentationEvents>([](const Species::PresentationEvents&) {});
  dispatcher.Register<Species::CommandReport>([&arrivals](const Species::CommandReport& _report) { arrivals.reports.push_back(_report); });
  dispatcher.Register<Species::ServerStats>([&arrivals](const Species::ServerStats& _stats) { arrivals.stats.push_back(_stats); });
  while (std::optional<Neuron::ReceivedMessage> message = _client.Receive())
  {
    Assert::IsTrue(dispatcher.Dispatch(message->bytes), L"only the terrain, state updates, events, reports and statistics arrive");
  }
  return arrivals;
}

} // namespace

TEST_CLASS(WorldSimulationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp096EachRegionStepsAsAJobEveryTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp096EachRegionStepsAsAJobEveryTick)
  {
    for (const std::size_t workers : {std::size_t{0}, std::size_t{2}})
    {
      Neuron::WorkerPool pool(workers);
      Species::WorldSimulation world({TEST_KEY, 50, 3, &CommittedTuning()});
      ExpectEqual(3, static_cast<std::int64_t>(world.RegionCount()), L"regions");
      for (std::uint64_t tick = 50; tick < 60; ++tick)
      {
        world.Step(tick, pool);
      }
      ExpectEqual(60, static_cast<std::int64_t>(world.NextTick()), L"the world tick");
      for (std::size_t r = 0; r < world.RegionCount(); ++r)
      {
        ExpectEqual(60, static_cast<std::int64_t>(world.Region(r).NextTick()), L"every region stepped every tick");
        ExpectEqual(Species::REGION_OWNER_ID_FIRST + static_cast<std::int64_t>(r), world.Region(r).Owner(), L"the region's owner id");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093OwnersIssueStableIdsInOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093OwnersIssueStableIdsInOrder)
  {
    Species::WorldSimulation world({TEST_KEY, 0, 1, &CommittedTuning()});
    const Species::StableId first = world.Ids().Issue();
    const Species::StableId second = world.Ids().Issue();
    Assert::IsTrue(first == Species::MakeStableId(Species::WORLD_OWNER_ID, 1), L"the world's first id");
    Assert::IsTrue(second == first + 1, L"then the next");
    ExpectEqual(3, static_cast<std::int64_t>(world.Ids().Next()), L"the counter is owner state");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094WorldCommandsExecuteAfterTheRegions)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094WorldCommandsExecuteAfterTheRegions)
  {
    // A command for an unknown type is refused on arrival with its reason, in the world owner's queue.
    Species::WorldSimulation world({TEST_KEY, 0, 1, &CommittedTuning()});
    Assert::IsFalse(world.Commands().Submit(world.Ids().Issue(), 0, Neuron::EncodeMessage(Species::WorldTerrain{})));
    ExpectEqual(1, static_cast<std::int64_t>(world.Commands().Rejections().unknownType), L"a message that is no command");
  }
};

TEST_CLASS(WorldServerTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090EveryConnectionReceivesEachTicksState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090EveryConnectionReceivesEachTicksState)
  {
    // The headless library and host with the game's world, on a manual clock (PLT-048): two clients, each sent the
    // world's terrain once and the state of every tick after it connected, which names its tick when the world is empty.
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource* manual = source.get();
    const Neuron::PlatformClock clock(std::move(source));
    Neuron::InProcessListener listener({});
    std::unique_ptr<Neuron::Transport> first = listener.Connect();
    std::unique_ptr<Neuron::Transport> second = listener.Connect();
    Species::WorldServer world({.tuning = CommittedTuning()});
    Neuron::ServerHost::Desc desc;
    desc.clock = &clock;
    desc.tickPeriod = TEST_TICK_PERIOD;
    desc.listener = &listener;
    std::uint64_t waits = 0;
    desc.waitUntil = [&](Neuron::Nanoseconds _deadline)
    {
      // Ten ticks, then the loop waits on its own clock until the test has read the updates.
      if (++waits <= 10)
      {
        manual->Advance(_deadline - clock.Now());
      }
      else
      {
        std::this_thread::yield();
      }
    };
    Neuron::ServerHost host(std::move(desc), world);
    host.Start();
    while (host.CompletedTicks() < 11)
    {
      std::this_thread::yield();
    }
    host.Stop();

    Assert::IsTrue(world.Terrain() != nullptr, L"the world opened with its terrain");
    for (Neuron::Transport* client : {first.get(), second.get()})
    {
      const Arrivals arrivals = TakeArrivals(*client);
      Assert::IsTrue(arrivals.terrains.size() == 1 && arrivals.terrains[0].seed == world.Terrain()->seed &&
                       arrivals.terrains[0].digest == world.Terrain()->digest,
                     L"the terrain's seed and digest, once");
      Assert::IsFalse(arrivals.terrains[0].commandedTeam.has_value(), L"a world that is no sandbox gives a steward no team yet");
#if ENGINE_DEVELOPMENT
      // ADR-120: a second of ticks, ten at 100 ms, gives one statistics message in a Debug build.
      Assert::IsTrue(arrivals.stats.size() == 1 && arrivals.stats[0].ticks == 10 && arrivals.stats[0].tick == 9,
                     L"the statistics of the first ten ticks");
#endif
      const std::vector<std::uint64_t>& ticks = arrivals.ticks;
      ExpectEqual(11, static_cast<std::int64_t>(ticks.size()), L"an update a tick, from the first");
      for (std::size_t n = 0; n < ticks.size(); ++n)
      {
        ExpectEqual(static_cast<std::int64_t>(n), static_cast<std::int64_t>(ticks[n]), L"in tick order");
      }
    }
    Assert::IsTrue(world.Simulation() != nullptr && world.Simulation()->NextTick() == 11, L"the world froze before tick 11");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005AWorldIsBuiltFromItsSeed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005AWorldIsBuiltFromItsSeed)
  {
    // A given seed builds its terrain when the world opens, outside every tick, and the world and its region hold it;
    // with none, the server draws a seed of 32 bits, which a re-roll raises by at most seven.
    Species::WorldServer given({.tuning = CommittedTuning(), .worldSeed = 1});
    static_cast<void>(given.Open());
    const auto expected = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(expected.has_value() && given.Terrain() != nullptr && given.Terrain()->digest == expected->digest, L"seed 1's terrain");
    Assert::IsTrue(given.Simulation()->WorldSeed() == 1 && given.Simulation()->Region(0).Terrain() == given.Terrain(),
                   L"the world holds the seed, and its region the terrain");
    Species::WorldServer drawn({.tuning = CommittedTuning()});
    static_cast<void>(drawn.Open());
    Assert::IsTrue(drawn.Terrain() != nullptr && drawn.Simulation()->WorldSeed() == drawn.Terrain()->seed, L"a drawn seed's terrain");
    Assert::IsTrue(drawn.Terrain()->seed < (std::uint64_t{1} << 32) + 8, L"a seed that can be typed again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094AClientsCommandEntersTheValidatedPath)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094AClientsCommandEntersTheValidatedPath)
  {
    // A message that is no command is refused as unknown by the world owner's queue, and a battle command goes to the
    // region's queue, which refuses it because the steward commands no team: each took the one path, and nothing
    // reached the simulation. The client receives each outcome with its sequence number and tick (ADR-095, ADR-106).
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource* manual = source.get();
    const Neuron::PlatformClock clock(std::move(source));
    Neuron::InProcessListener listener({});
    std::unique_ptr<Neuron::Transport> client = listener.Connect();
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::WorldTerrain{})).has_value());
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::SelectCommand{0, 0})).has_value());
    Species::WorldServer world({.tuning = CommittedTuning()});
    Neuron::ServerHost::Desc desc;
    desc.clock = &clock;
    desc.tickPeriod = TEST_TICK_PERIOD;
    desc.listener = &listener;
    desc.waitUntil = [&](Neuron::Nanoseconds _deadline) { manual->Advance(_deadline - clock.Now()); };
    Neuron::ServerHost host(std::move(desc), world);
    host.Start();
    while (host.CompletedTicks() < 3)
    {
      std::this_thread::yield();
    }
    host.Stop();
    Assert::IsTrue(world.Simulation() != nullptr);
    ExpectEqual(1, static_cast<std::int64_t>(world.Simulation()->Commands().Rejections().unknownType), L"refused as unknown");
    ExpectEqual(1, static_cast<std::int64_t>(world.Simulation()->Region(0).Commands().Rejections().notPermitted), L"refused by the region");
    const Arrivals arrivals = TakeArrivals(*client);
    Assert::IsTrue(arrivals.reports.size() == 2, L"two outcomes");
    Assert::IsTrue(arrivals.reports[0].sequence == 0 && arrivals.reports[0].result == Species::CommandResult::UnknownType,
                   L"the first command's outcome");
    Assert::IsTrue(arrivals.reports[1].sequence == 1 && arrivals.reports[1].result == Species::CommandResult::NotPermitted,
                   L"the second command's outcome");
    Assert::IsTrue(arrivals.reports[0].tick == arrivals.reports[1].tick && arrivals.reports[0].tick < 3, L"each with its tick");
  }
};

TEST_CLASS(IntegratedServerTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090TheClientSeesTheTickRiseThroughTheTransport)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090TheClientSeesTheTickRiseThroughTheTransport)
  {
    // M0's exit criterion in a test: the client's end of the integrated server, on the real clock at a period of 10 ms
    // that the cooked tuning record states, receives rising ticks until the server stops.
    const TemporaryFolder folder;
    const Neuron::LooseContent content = CookedContentWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.01");
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.content = &content;
    Species::IntegratedServer server(desc);
    std::unique_ptr<Neuron::Transport> client = server.Connect();
    Assert::IsTrue(server.Start().has_value(), L"the server starts on the record");
    std::vector<std::uint64_t> ticks;
    while (ticks.size() < 20)
    {
      const Arrivals arrived = TakeArrivals(*client);
      ticks.insert(ticks.end(), arrived.ticks.begin(), arrived.ticks.end());
      clock.WaitUntil(clock.Now() + Neuron::Nanoseconds{2'000'000});
    }
    server.Stop();
    for (std::size_t n = 1; n < ticks.size(); ++n)
    {
      Assert::IsTrue(ticks[n] == ticks[n - 1] + 1, L"one update a tick, in order");
    }
    Assert::IsTrue(client->State() == Neuron::ConnectionState::Closed, L"stopping the server closes the connection");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075TheIntegratedServerContinuesItsWorldFromItsFolder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075TheIntegratedServerContinuesItsWorldFromItsFolder)
  {
    // ADR-137: stopping the integrated server freezes its world into its folder with the marker of a clean stop, and a
    // server started on the folder continues the world after the tick it froze before, where a new world starts at 0.
    const TemporaryFolder folder;
    const Neuron::LooseContent content = CookedContentWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.01");
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.content = &content;
    desc.snapshotFolder = folder.Path() / L"world";
    const auto firstTicks = [&](std::uint64_t _atLeast)
    {
      Species::IntegratedServer server(desc);
      std::unique_ptr<Neuron::Transport> client = server.Connect();
      Assert::IsTrue(server.Start().has_value(), L"the server starts");
      std::vector<std::uint64_t> ticks;
      while (ticks.size() < _atLeast)
      {
        const Arrivals arrived = TakeArrivals(*client);
        ticks.insert(ticks.end(), arrived.ticks.begin(), arrived.ticks.end());
        clock.WaitUntil(clock.Now() + Neuron::Nanoseconds{2'000'000});
      }
      server.Stop();
      return ticks;
    };
    const std::vector<std::uint64_t> before = firstTicks(5);
    ExpectEqual(0, static_cast<std::int64_t>(before.front()), L"a new world starts at tick 0");
    Assert::IsTrue(std::filesystem::exists(desc.snapshotFolder / Species::WorldServer::CLEAN_STOP_FILE_NAME), L"stopped cleanly");
    const std::vector<std::uint64_t> after = firstTicks(1);
    Assert::IsTrue(after.front() > before.back(), L"the world continues after the ticks it ran");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099AServerWithoutItsTuningDoesNotStart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099AServerWithoutItsTuningDoesNotStart)
  {
    const TemporaryFolder folder;
    Assert::IsTrue(Neuron::WriteLooseContent(folder.Path(), {}).has_value(), L"a folder of no entry is written");
    const auto content = Neuron::LooseContent::Mount(folder.Path());
    Assert::IsTrue(content.has_value(), L"it mounts");
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.content = &*content;
    Species::IntegratedServer server(desc);
    std::unique_ptr<Neuron::Transport> client = server.Connect();
    const auto started = server.Start();
    Assert::IsFalse(started.has_value(), L"no record, no server");
    Assert::IsTrue(started.error().fault == Neuron::AssetFault::Missing, L"a missing record");
    server.Stop();
    Assert::IsFalse(client->Receive().has_value(), L"no tick ran");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast046AServerWithoutItsStructureRecordsDoesNotStart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-043")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast046AServerWithoutItsStructureRecordsDoesNotStart)
  {
    // M2 task T3's done-when (ADR-129): the server starts from the cooked simulation data records, and content that
    // lacks them starts no server.
    const TemporaryFolder folder;
    const Neuron::LooseContent content = CookedContentWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.1", false);
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.content = &content;
    Species::IntegratedServer server(desc);
    std::unique_ptr<Neuron::Transport> client = server.Connect();
    const auto started = server.Start();
    Assert::IsFalse(started.has_value(), L"no structure records, no server");
    Assert::IsTrue(started.error().fault == Neuron::AssetFault::Missing && started.error().key == "meshes/wellspring@sim",
                   L"the first missing record");
    server.Stop();
    Assert::IsFalse(client->Receive().has_value(), L"no tick ran");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp030AServerWhoseResourceSiteMeshDisagreesWithItsTuningDoesNotStart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp030AServerWhoseResourceSiteMeshDisagreesWithItsTuningDoesNotStart)
  {
    // ADR-130: a resource site's gathering ports are the port sockets of its mesh, so a tuning record that gives it 5 per
    // unit against a mesh of 4 starts no server, and names the resource site's record.
    const TemporaryFolder folder;
    const Neuron::LooseContent content = CookedContentWith(folder, "\"ports_per_unit\": 4", "\"ports_per_unit\": 5");
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.content = &content;
    Species::IntegratedServer server(desc);
    const auto started = server.Start();
    Assert::IsFalse(started.has_value(), L"no server");
    Assert::IsTrue(started.error().fault == Neuron::AssetFault::Damaged && started.error().key == "meshes/resourcesite@sim",
                   L"the resource site's record");
    server.Stop();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099ATickPeriodChangedInDataChangesTheServersTicks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099ATickPeriodChangedInDataChangesTheServersTicks)
  {
    // M0 task T9's done-when (owner decision O67): the same build, given 0.2 s in place of 0.1 s in the record, runs
    // its ticks 200 ms apart. The clock is manual, so the test measures the schedule exactly.
    const TemporaryFolder folder;
    const auto tuning = Species::LoadTuningMvp(CookedContentWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.2"));
    Assert::IsTrue(tuning.has_value(), L"the changed record, cooked, loads");
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource* manual = source.get();
    const Neuron::PlatformClock clock(std::move(source));
    Species::WorldServer::Desc worldDesc;
    worldDesc.tuning = tuning.value_or(Species::TuningMvp{});
    Species::WorldServer world(worldDesc);
    Neuron::ServerHost::Desc desc;
    desc.clock = &clock;
    desc.tickPeriod = Species::TickPeriodOf(world.Tuning());
    std::vector<Neuron::Nanoseconds> waits;
    desc.waitUntil = [&](Neuron::Nanoseconds _deadline)
    {
      waits.push_back(_deadline);
      manual->Advance(_deadline - clock.Now());
    };
    Neuron::ServerHost host(std::move(desc), world);
    host.Start();
    while (host.CompletedTicks() < 10)
    {
      std::this_thread::yield();
    }
    host.Stop();
    Assert::IsTrue(waits.size() >= 9, L"the loop slept to each deadline");
    for (std::size_t n = 1; n < 9; ++n)
    {
      ExpectEqual(200'000'000, (waits[n] - waits[n - 1]).count(), L"the ticks are 0.2 s apart");
    }
  }
};

} // namespace GameLogicTests
