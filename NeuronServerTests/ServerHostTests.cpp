#include "pch.h"

#include "InProcessTransport.h"
#include "ServerHost.h"
#include "TestSupport.h"
#include "WorkerPool.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronServerTests
{

namespace
{

using Neuron::Nanoseconds;

// A world that records what the host asked of it, and echoes each received message back on the state channel.
class RecordingWorld final : public Neuron::ServerWorld
{
public:
  RecordingWorld(const ManualClock& _clock, std::uint64_t _firstTick)
    : m_clock(_clock),
      m_firstTick(_firstTick)
  {
  }

  std::uint64_t Open() override
  {
    if (failToOpen)
    {
      throw std::runtime_error("the world does not validate");
    }
    events.emplace_back("open");
    return m_firstTick;
  }
  void Connect(Neuron::ConnectionId _connection) override
  {
    events.push_back("connect " + std::to_string(_connection));
  }
  void Disconnect(Neuron::ConnectionId _connection) override
  {
    events.push_back("disconnect " + std::to_string(_connection));
  }
  void Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message) override
  {
    received.push_back({_connection, ticks.empty() ? 0 : ticks.back() + 1, _message.bytes});
  }
  void DatagramFates(Neuron::ConnectionId, std::span<const Neuron::DatagramOutcome> _outcomes) override
  {
    fates.fetch_add(_outcomes.size());
  }
  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers) override
  {
    std::vector<std::uint64_t> regions(3, 0);
    _workers.Run(regions.size(), [&regions, _tick](std::size_t _region) { regions[_region] = _tick * 10 + _region; });
    regionSum += regions[0] + regions[1] + regions[2];
    ticks.push_back(_tick);
    starts.push_back(m_clock.Now());
    if (ticks.size() == stopAfter)
    {
      const std::scoped_lock lock(mutex);
      reached = true;
      wake.notify_all();
    }
  }
  void Publish(std::uint64_t, Neuron::ConnectionSender& _sender) override
  {
    for (const Received& message : received)
    {
      if (!message.echoed)
      {
        Assert::IsTrue(_sender.Send(message.connection, Neuron::Channel::State, message.bytes).has_value());
      }
    }
    for (Received& message : received)
    {
      message.echoed = true;
    }
  }
  void Stalled(const Neuron::ScheduleStall&) override {}
  void Freeze(std::uint64_t _nextTick) override
  {
    events.push_back("freeze " + std::to_string(_nextTick));
  }

  void WaitUntilReached()
  {
    std::unique_lock lock(mutex);
    wake.wait(lock, [this] { return reached; });
  }

  struct Received
  {
    Neuron::ConnectionId connection = 0;
    std::uint64_t tick = 0;
    std::vector<std::byte> bytes;
    bool echoed = false;
  };

  bool failToOpen = false;
  std::size_t stopAfter = 0;
  std::vector<std::string> events;
  std::vector<std::uint64_t> ticks;
  std::vector<Nanoseconds> starts;
  std::vector<Received> received;
  std::atomic<std::size_t> fates{0}; // read by the test thread while the server runs
  std::uint64_t regionSum = 0;
  std::mutex mutex;
  std::condition_variable wake;
  bool reached = false;

private:
  const ManualClock& m_clock;
  std::uint64_t m_firstTick;
};

Neuron::ServerHost::Desc HostOn(ManualClock& _clock, Neuron::TransportListener* _listener, std::size_t _workers)
{
  Neuron::ServerHost::Desc desc;
  desc.clock = &_clock.Clock();
  desc.tickPeriod = TEST_TICK_PERIOD;
  desc.workers = _workers;
  desc.listener = _listener;
  desc.waitUntil = [&_clock](Nanoseconds _deadline) { _clock.AdvanceTo(_deadline); };
  return desc;
}

} // namespace

TEST_CLASS(ServerHostTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092AHeadlessServerRuns600TicksOnTheDeadline)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092AHeadlessServerRuns600TicksOnTheDeadline)
  {
    // M0 task T6's done-when: the host with no client steps its world 600 times, each tick on its deadline. The test
    // clock is manual and every sleep moves it to its deadline, so the minute of world time takes no minute.
    ManualClock clock;
    clock.Advance(Nanoseconds{5'000});
    const Nanoseconds start = clock.Now();
    RecordingWorld world(clock, 1200);
    world.stopAfter = 600;
    Neuron::ServerHost host(HostOn(clock, nullptr, 2), world);
    host.Start();
    Assert::IsTrue(host.State() == Neuron::ServerState::Running);
    world.WaitUntilReached();
    host.Stop();
    Assert::IsTrue(host.State() == Neuron::ServerState::Stopped);

    Assert::IsTrue(world.ticks.size() >= 600, L"600 ticks ran");
    for (std::size_t n = 0; n < 600; ++n)
    {
      ExpectEqual(static_cast<std::int64_t>(1200 + n), static_cast<std::int64_t>(world.ticks[n]), L"the world's tick");
      ExpectEqual((start + TEST_TICK_PERIOD * static_cast<std::int64_t>(n)).count(), world.starts[n].count(), L"each tick on its deadline");
    }
    Assert::IsTrue(world.events.front() == "open", L"the world opens first");
    Assert::IsTrue(world.events.back() == "freeze " + std::to_string(1200 + world.ticks.size()), L"and freezes before the next tick");
    // Three region jobs a tick on two workers and the tick thread: each wrote its own part.
    std::uint64_t expectedSum = 0;
    for (const std::uint64_t tick : world.ticks)
    {
      expectedSum += tick * 30 + 3;
    }
    ExpectEqual(static_cast<std::int64_t>(expectedSum), static_cast<std::int64_t>(world.regionSum), L"every region job ran once a tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090AClientConnectsSendsReceivesAndLeaves)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090AClientConnectsSendsReceivesAndLeaves)
  {
    ManualClock clock;
    RecordingWorld world(clock, 0);
    Neuron::InProcessListener listener({});
    std::unique_ptr<Neuron::Transport> client = listener.Connect();
    const std::vector<std::byte> hello{std::byte{1}, std::byte{2}, std::byte{3}};
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, hello).has_value());
    Neuron::ServerHost host(HostOn(clock, &listener, 0), world);
    host.Start();
    // The client reads the world's reply while the server runs, and the server learns the reply's fate at the start of
    // a later tick.
    std::optional<Neuron::ReceivedMessage> echo;
    while (!(echo = client->Receive()))
    {
      std::this_thread::yield();
    }
    while (world.fates.load() == 0)
    {
      std::this_thread::yield();
    }
    host.Stop();

    Assert::IsTrue(world.events.size() >= 3 && world.events[1] == "connect 1", L"the client's connection is accepted");
    ExpectEqual(1, static_cast<std::int64_t>(world.received.size()), L"messages delivered");
    ExpectEqual(0, static_cast<std::int64_t>(world.received[0].tick), L"delivered at the start of the first tick");
    Assert::IsTrue(echo->channel == Neuron::Channel::State && echo->bytes == hello, L"the world's reply");
    Assert::IsTrue(world.events[world.events.size() - 2] == "disconnect 1", L"a stopping host closes the connection");
    Assert::IsTrue(client->State() == Neuron::ConnectionState::Closed && client->Reason() == Neuron::CloseReason::ClosedByPeer);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp091AWorldThatFailsToOpenLeavesTheHostStopped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp091AWorldThatFailsToOpenLeavesTheHostStopped)
  {
    ManualClock clock;
    RecordingWorld world(clock, 0);
    world.failToOpen = true;
    Neuron::ServerHost host(HostOn(clock, nullptr, 0), world);
    bool threw = false;
    try
    {
      host.Start();
    }
    catch (const std::runtime_error&)
    {
      threw = true;
    }
    Assert::IsTrue(threw, L"the failure reaches the caller");
    Assert::IsTrue(host.State() == Neuron::ServerState::Stopped);
    Assert::IsTrue(world.ticks.empty(), L"no tick ran");
  }
};

TEST_CLASS(WorkerPoolTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp096EveryRegionJobRunsOnceAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp096EveryRegionJobRunsOnceAtAnyWorkerCount)
  {
    for (const std::size_t workers : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      Neuron::WorkerPool pool(workers);
      ExpectEqual(static_cast<std::int64_t>(workers), static_cast<std::int64_t>(pool.Workers()), L"workers");
      for (std::size_t round = 0; round < 50; ++round)
      {
        std::vector<std::atomic<int>> runs(17);
        pool.Run(runs.size(), [&runs](std::size_t _job) { runs[_job].fetch_add(1); });
        for (const std::atomic<int>& count : runs)
        {
          ExpectEqual(1, count.load(), L"each job ran once");
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp096AJobThatThrowsReachesTheTickThread)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp096AJobThatThrowsReachesTheTickThread)
  {
    Neuron::WorkerPool pool(2);
    std::atomic<int> ran{0};
    bool threw = false;
    try
    {
      pool.Run(10,
               [&ran](std::size_t _job)
               {
                 ran.fetch_add(1);
                 if (_job == 4)
                 {
                   throw std::runtime_error("a region step failed");
                 }
               });
    }
    catch (const std::runtime_error&)
    {
      threw = true;
    }
    Assert::IsTrue(threw);
    ExpectEqual(10, ran.load(), L"the other jobs still ran");
  }
};

} // namespace NeuronServerTests
