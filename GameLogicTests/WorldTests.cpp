#include "pch.h"

#include "InProcessTransport.h"
#include "IntegratedServer.h"
#include "ServerHost.h"
#include "TestSupport.h"
#include "TickReport.h"
#include "Tuning.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
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

// The tick reports a client end has received, in order.
std::vector<std::uint64_t> TakeReports(Neuron::Transport& _client)
{
  std::vector<std::uint64_t> ticks;
  Neuron::MessageDispatcher dispatcher;
  dispatcher.Register<Species::TickReport>([&ticks](const Species::TickReport& _report) { ticks.push_back(_report.tick); });
  while (std::optional<Neuron::ReceivedMessage> message = _client.Receive())
  {
    Assert::IsTrue(dispatcher.Dispatch(message->bytes), L"only tick reports arrive");
  }
  return ticks;
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
      Species::WorldSimulation world({TEST_KEY, 50, 3});
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
    Species::WorldSimulation world({TEST_KEY, 0, 1});
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
    Species::WorldSimulation world({TEST_KEY, 0, 1});
    Assert::IsFalse(world.Commands().Submit(world.Ids().Issue(), 0, Neuron::EncodeMessage(Species::TickReport{1})));
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090EveryConnectionReceivesEachTicksReport)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090EveryConnectionReceivesEachTicksReport)
  {
    // The headless library and host with the game's world, on a manual clock (PLT-048): two clients, each sent the
    // report of every tick after it connected.
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
      // Ten ticks, then the loop waits on its own clock until the test has read the reports.
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

    for (Neuron::Transport* client : {first.get(), second.get()})
    {
      const std::vector<std::uint64_t> ticks = TakeReports(*client);
      ExpectEqual(11, static_cast<std::int64_t>(ticks.size()), L"a report a tick, from the first");
      for (std::size_t n = 0; n < ticks.size(); ++n)
      {
        ExpectEqual(static_cast<std::int64_t>(n), static_cast<std::int64_t>(ticks[n]), L"in tick order");
      }
    }
    Assert::IsTrue(world.Simulation() != nullptr && world.Simulation()->NextTick() == 11, L"the world froze before tick 11");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094AClientsCommandEntersTheValidatedPath)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094AClientsCommandEntersTheValidatedPath)
  {
    // M0 has no command types, so a client's message is refused as unknown by the world owner's queue: it took the
    // one path, and nothing reached the simulation.
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource* manual = source.get();
    const Neuron::PlatformClock clock(std::move(source));
    Neuron::InProcessListener listener({});
    std::unique_ptr<Neuron::Transport> client = listener.Connect();
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::TickReport{7})).has_value());
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
    // that the tuning record states, receives rising ticks until the server stops.
    const TemporaryFolder folder;
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.contentRoot = ContentRootWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.01");
    Species::IntegratedServer server(desc);
    std::unique_ptr<Neuron::Transport> client = server.Connect();
    Assert::IsTrue(server.Start().has_value(), L"the server starts on the record");
    std::vector<std::uint64_t> ticks;
    while (ticks.size() < 20)
    {
      const std::vector<std::uint64_t> arrived = TakeReports(*client);
      ticks.insert(ticks.end(), arrived.begin(), arrived.end());
      clock.WaitUntil(clock.Now() + Neuron::Nanoseconds{2'000'000});
    }
    server.Stop();
    for (std::size_t n = 1; n < ticks.size(); ++n)
    {
      Assert::IsTrue(ticks[n] == ticks[n - 1] + 1, L"one report a tick, in order");
    }
    Assert::IsTrue(client->State() == Neuron::ConnectionState::Closed, L"stopping the server closes the connection");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099AServerWithoutItsTuningDoesNotStart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099AServerWithoutItsTuningDoesNotStart)
  {
    const TemporaryFolder folder;
    const Neuron::PlatformClock clock;
    Species::IntegratedServer::Desc desc;
    desc.clock = &clock;
    desc.contentRoot = folder.Path();
    Species::IntegratedServer server(desc);
    std::unique_ptr<Neuron::Transport> client = server.Connect();
    const auto started = server.Start();
    Assert::IsFalse(started.has_value(), L"no record, no server");
    Assert::IsTrue(started.error().front().diagnosticClass == Neuron::DiagnosticClass::MissingRecord, L"a missing record");
    server.Stop();
    Assert::IsFalse(client->Receive().has_value(), L"no tick ran");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp099ATickPeriodChangedInDataChangesTheServersTicks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp099ATickPeriodChangedInDataChangesTheServersTicks)
  {
    // M0 task T9's done-when (owner decision O67): the same build, given 0.2 s in place of 0.1 s in the record, runs
    // its ticks 200 ms apart. The clock is manual, so the test measures the schedule exactly.
    const TemporaryFolder folder;
    const auto tuning = Species::LoadTuningMvp(ContentRootWith(folder, "\"tick_period\": 0.1", "\"tick_period\": 0.2"));
    Assert::IsTrue(tuning.has_value(), L"the changed record reads whole");
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
