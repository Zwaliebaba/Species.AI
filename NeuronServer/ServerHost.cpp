#include "pch.h"

#include "Check.h"
#include "Log.h"
#include "Profile.h"
#include "ServerHost.h"

#include <exception>
#include <expected>
#include <optional>
#include <utility>
#include <vector>

namespace Neuron
{

ServerHost::ServerHost(Desc _desc, ServerWorld& _world)
  : m_desc(std::move(_desc)),
    m_world(_world)
{
  ENGINE_CHECK(m_desc.clock != nullptr && m_desc.tickPeriod.count() > 0);
}

ServerHost::~ServerHost()
{
  try
  {
    Stop();
  }
  catch (...)
  {
    // A world that fails to freeze in a destructor has nowhere to report it; the fatal path takes it.
    std::terminate();
  }
}

void ServerHost::Start()
{
  ENGINE_CHECK(State() == ServerState::Stopped);
  m_state.store(ServerState::Opening, std::memory_order_release);
  try
  {
    m_firstTick = m_world.Open();
  }
  catch (...)
  {
    m_state.store(ServerState::Stopped, std::memory_order_release);
    throw;
  }
  m_workers = std::make_unique<WorkerPool>(m_desc.workers);
  TickLoop::Desc loop;
  loop.clock = m_desc.clock;
  loop.period = m_desc.tickPeriod;
  loop.firstTick = m_firstTick;
  loop.waitUntil = m_desc.waitUntil;
  loop.onStall = [this](const ScheduleStall& _stall) { m_world.Stalled(_stall); };
  m_loop = std::make_unique<TickLoop>(std::move(loop));
  m_state.store(ServerState::Running, std::memory_order_release);
  Log(LogCategory::Simulation, LogLevel::Info, "the server runs from tick {} at {} ms per tick with {} workers", m_firstTick,
      m_desc.tickPeriod.count() / 1'000'000, m_desc.workers);
  m_tickThread =
    PlatformThread::Start("server.tick", [this](const std::stop_token& _stop) { static_cast<void>(m_loop->Run(*this, _stop)); });
}

void ServerHost::Stop()
{
  if (State() != ServerState::Running)
  {
    return;
  }
  m_state.store(ServerState::Freezing, std::memory_order_release);
  m_tickThread.RequestStop();
  m_tickThread.Join();
  const std::uint64_t nextTick = m_firstTick + m_loop->CompletedTicks();
  for (auto& [id, connection] : m_connections)
  {
    connection->Close();
    m_world.Disconnect(id);
  }
  m_connections.clear();
  m_world.Freeze(nextTick);
  m_loop.reset();
  m_workers.reset();
  Log(LogCategory::Simulation, LogLevel::Info, "the server froze before tick {}", nextTick);
  m_state.store(ServerState::Stopped, std::memory_order_release);
}

std::uint64_t ServerHost::CompletedTicks() const noexcept
{
  return m_loop ? m_loop->CompletedTicks() : 0;
}

void ServerHost::RunTick(std::uint64_t _tick)
{
  {
    ENGINE_PROFILE_SCOPE("server.receive");
    AcceptConnections();
    DeliverMessages();
  }
  {
    ENGINE_PROFILE_SCOPE("server.step");
    m_world.Step(_tick, *m_workers);
  }
  {
    ENGINE_PROFILE_SCOPE("server.publish");
    m_world.Publish(_tick, *this);
  }
}

std::optional<std::uint64_t> ServerHost::Send(ConnectionId _connection, Channel _channel, std::span<const std::byte> _message)
{
  const auto found = m_connections.find(_connection);
  if (found == m_connections.end())
  {
    return std::nullopt;
  }
  const std::expected<std::uint64_t, SendError> sent = found->second->Send(_channel, _message);
  return sent ? std::optional<std::uint64_t>{*sent} : std::nullopt;
}

void ServerHost::AcceptConnections()
{
  if (m_desc.listener == nullptr)
  {
    return;
  }
  while (std::unique_ptr<Transport> transport = m_desc.listener->Accept())
  {
    const ConnectionId id = m_nextConnection++;
    m_connections.emplace(id, std::move(transport));
    Log(LogCategory::Network, LogLevel::Info, "connection {} accepted", id);
    m_world.Connect(id);
  }
}

void ServerHost::DeliverMessages()
{
  std::vector<ConnectionId> closed;
  for (auto& [id, connection] : m_connections)
  {
    // Everything that has arrived is delivered at the start of the tick, so a command executes in the first tick that
    // starts after it arrived (ADR-087 item 4). The transport's own bound limits how much that is; the per-client rate
    // limits of ADR-087 item 6 come with the server API and the remote clients.
    while (std::optional<ReceivedMessage> message = connection->Receive())
    {
      m_world.Receive(id, *message);
    }
    const std::vector<DatagramOutcome> outcomes = connection->TakeDatagramOutcomes();
    if (!outcomes.empty())
    {
      m_world.DatagramFates(id, outcomes);
    }
    if (connection->State() == ConnectionState::Closed)
    {
      closed.push_back(id);
    }
  }
  for (const ConnectionId id : closed)
  {
    Log(LogCategory::Network, LogLevel::Info, "connection {} closed", id);
    m_world.Disconnect(id);
    m_connections.erase(id);
  }
}

} // namespace Neuron
