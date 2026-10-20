#pragma once

#include "PlatformClock.h"
#include "PlatformThread.h"
#include "TickLoop.h"
#include "Transport.h"
#include "WorkerPool.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <span>

namespace Neuron
{

// A connection of a client to this server, numbered by the host from 1 in the order they were accepted.
using ConnectionId = std::uint64_t;

// ADR-083 item 3: a world server's states. Every start and end of the server run's scope is one of its transitions.
enum class ServerState : std::uint8_t
{
  Stopped,
  Opening,
  Running,
  Freezing
};

/// What the world sees of the host's connections when it publishes a tick.
class ConnectionSender
{
public:
  virtual ~ConnectionSender() = default;

  // Queues a message to one connection; false when it cannot take it (closed, too large or backlogged).
  [[nodiscard]] virtual bool Send(ConnectionId _connection, Channel _channel, std::span<const std::byte> _message) = 0;
};

/// The world a server host runs: the game's state owners behind one interface, so that the engine knows no game
/// concept (R9). Open and Freeze come from the thread that starts and stops the host; every other call comes from the
/// tick thread, in the order of one tick (ADR-083 item 6): the connections, the messages, the step, the publication.
class ServerWorld
{
public:
  virtual ~ServerWorld() = default;

  // Opening: the world is loaded and validated; returns the first tick to run.
  [[nodiscard]] virtual std::uint64_t Open() = 0;
  virtual void Connect(ConnectionId _connection) = 0;
  virtual void Disconnect(ConnectionId _connection) = 0;
  // A message that arrived before this tick began, delivered at its start.
  virtual void Receive(ConnectionId _connection, const ReceivedMessage& _message) = 0;
  // The fates of the state datagrams sent to a connection, decided before this tick began (ADR-086).
  virtual void DatagramFates(ConnectionId _connection, std::span<const DatagramOutcome> _outcomes) = 0;
  // One whole tick: its commands, then every owner's step, the regions as jobs on the workers.
  virtual void Step(std::uint64_t _tick, WorkerPool& _workers) = 0;
  // After the step: every connection's part of the tick.
  virtual void Publish(std::uint64_t _tick, ConnectionSender& _sender) = 0;
  // The loop gave up real time, which is not world time; a freeze goes in the world record (PIL-131).
  virtual void Stalled(const ScheduleStall& _stall) = 0;
  // Freezing: the tick thread has stopped and every connection is closed; _nextTick is the first tick not run.
  virtual void Freeze(std::uint64_t _nextTick) = 0;
};

/// NeuronServer's composition root (ADR-083 item 2): the server run of one world. It creates the worker pool, the tick
/// loop and the tick thread in that order and ends them in the reverse order, and it holds the connections. The same
/// host runs integrated in the client's process and stand-alone in the headless server; it reaches the system only
/// through the platform layer and the network only through the transport (ADR-083 item 1).
class ServerHost final : private TickTarget, private ConnectionSender
{
public:
  struct Desc
  {
    const PlatformClock* clock = nullptr;
    Nanoseconds tickPeriod{0};                  // `ref.sim.tick_period`
    std::size_t workers = 0;                    // the integrated server has at most 2 (ADR-080 item 7)
    TransportListener* listener = nullptr;      // where clients connect; none for a world nobody joins
    std::function<void(Nanoseconds)> waitUntil; // tests only: see TickLoop::Desc
  };

  ServerHost(Desc _desc, ServerWorld& _world);
  ~ServerHost() override;

  ServerHost(const ServerHost&) = delete;
  ServerHost& operator=(const ServerHost&) = delete;

  // Stopped → Opening → Running: opens the world and starts the tick thread. A world that fails to open throws, and the
  // host stays stopped.
  void Start();
  // Running → Freezing → Stopped: stops the tick thread after its tick, closes every connection and freezes the world.
  void Stop();

  // Any thread.
  [[nodiscard]] ServerState State() const noexcept
  {
    return m_state.load(std::memory_order_acquire);
  }
  [[nodiscard]] std::uint64_t CompletedTicks() const noexcept;

private:
  void RunTick(std::uint64_t _tick) override;
  [[nodiscard]] bool Send(ConnectionId _connection, Channel _channel, std::span<const std::byte> _message) override;

  void AcceptConnections();
  void DeliverMessages();

  Desc m_desc;
  ServerWorld& m_world;
  std::atomic<ServerState> m_state{ServerState::Stopped};
  std::uint64_t m_firstTick = 0;
  std::unique_ptr<WorkerPool> m_workers;
  std::unique_ptr<TickLoop> m_loop;
  PlatformThread m_tickThread;
  std::map<ConnectionId, std::unique_ptr<Transport>> m_connections; // the tick thread's, while it runs
  ConnectionId m_nextConnection = 1;
};

} // namespace Neuron
