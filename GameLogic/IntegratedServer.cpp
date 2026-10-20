#include "pch.h"

#include "InProcessTransport.h"
#include "IntegratedServer.h"
#include "ServerHost.h"
#include "WorldServer.h"

namespace Species
{

// In the order they are made and the reverse of the order they end: the listener and the world outlive the host.
struct IntegratedServer::Parts
{
  Neuron::InProcessListener listener{Neuron::InProcessTransport::Desc{}};
  WorldServer world{WorldServer::Desc{}};
  std::unique_ptr<Neuron::ServerHost> host;
};

IntegratedServer::IntegratedServer(const Desc& _desc)
  : m_parts(std::make_unique<Parts>())
{
  Neuron::ServerHost::Desc host;
  host.clock = _desc.clock;
  host.tickPeriod = _desc.tickPeriod;
  host.workers = _desc.workers;
  host.listener = &m_parts->listener;
  m_parts->host = std::make_unique<Neuron::ServerHost>(std::move(host), m_parts->world);
}

IntegratedServer::~IntegratedServer() = default;

void IntegratedServer::Start()
{
  m_parts->host->Start();
}

void IntegratedServer::Stop()
{
  m_parts->host->Stop();
}

std::unique_ptr<Neuron::Transport> IntegratedServer::Connect()
{
  return m_parts->listener.Connect();
}

} // namespace Species
