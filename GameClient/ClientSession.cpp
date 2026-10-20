#include "pch.h"

#include "ClientSession.h"
#include "Log.h"
#include "TickReport.h"

#include <format>
#include <utility>

namespace Species
{

ClientSession::ClientSession(Desc _desc)
  : m_transport(std::move(_desc.transport)),
    m_showTitle(std::move(_desc.showTitle)),
    m_productName(std::move(_desc.productName))
{
  m_dispatcher.Register<TickReport>([this](const TickReport& _report) { m_serverTick = _report.tick; });
}

void ClientSession::Update()
{
  if (!m_transport)
  {
    return;
  }
  while (std::optional<Neuron::ReceivedMessage> message = m_transport->Receive())
  {
    static_cast<void>(m_dispatcher.Dispatch(message->bytes));
  }
  if (m_showTitle && m_serverTick != m_shownTick)
  {
    m_showTitle(SessionTitle(m_productName, m_serverTick));
    m_shownTick = m_serverTick;
  }
}

void ClientSession::Close()
{
  if (m_transport)
  {
    m_transport->Close();
    const Neuron::MessageDropCounts& drops = m_dispatcher.Drops();
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Info, "the session closed at server tick {}, {} messages dropped",
                m_serverTick.value_or(0), drops.Total());
  }
}

bool ClientSession::Connected() const
{
  return m_transport && m_transport->State() == Neuron::ConnectionState::Open;
}

std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _serverTick)
{
  if (!_serverTick)
  {
    return std::string(_productName);
  }
  return std::format("{} — tick {}", _productName, *_serverTick);
}

} // namespace Species
