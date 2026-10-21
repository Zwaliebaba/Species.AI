#include "pch.h"

#include "Check.h"
#include "InProcessTransport.h"
#include "IntegratedServer.h"
#include "ServerHost.h"
#include "Tuning.h"
#include "WorldServer.h"

#include <utility>

namespace Species
{

// In the order they are made and the reverse of the order they end: the listener and the world outlive the host.
struct IntegratedServer::Parts
{
  Neuron::InProcessListener listener{Neuron::InProcessTransport::Desc{}};
  std::unique_ptr<WorldServer> world;
  std::unique_ptr<Neuron::ServerHost> host;
};

IntegratedServer::IntegratedServer(const Desc& _desc)
  : m_desc(_desc),
    m_parts(std::make_unique<Parts>())
{
  ENGINE_CHECK(m_desc.clock != nullptr);
}

IntegratedServer::~IntegratedServer() = default;

std::expected<void, Neuron::AssetError> IntegratedServer::Start()
{
  ENGINE_CHECK(m_parts->host == nullptr, "the integrated server starts once");
  ENGINE_CHECK(m_desc.content != nullptr, "the integrated server loads its tuning from mounted content");
  // MVP-099: the tuning values come from data when the server starts, never from code; the content build cooked them
  // (ADR-108).
  std::expected<TuningMvp, Neuron::AssetError> tuning = LoadTuningMvp(*m_desc.content);
  if (!tuning)
  {
    return std::unexpected(std::move(tuning.error()));
  }
  WorldServer::Desc world;
  world.tuning = *tuning;
  world.worldSeed = m_desc.worldSeed;
  world.sandbox = m_desc.sandbox;
  world.watched = m_desc.watched;
  m_parts->world = std::make_unique<WorldServer>(world);
  Neuron::ServerHost::Desc host;
  host.clock = m_desc.clock;
  host.tickPeriod = TickPeriodOf(*tuning);
  host.workers = m_desc.workers;
  host.listener = &m_parts->listener;
  m_parts->host = std::make_unique<Neuron::ServerHost>(std::move(host), *m_parts->world);
  m_parts->host->Start();
  return {};
}

void IntegratedServer::Stop()
{
  if (m_parts->host)
  {
    m_parts->host->Stop();
  }
}

std::unique_ptr<Neuron::Transport> IntegratedServer::Connect()
{
  return m_parts->listener.Connect();
}

} // namespace Species
