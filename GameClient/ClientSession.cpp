#include "pch.h"

#include "Check.h"
#include "ClientSession.h"
#include "Log.h"
#include "Profile.h"
#include "TickReport.h"
#include "WorldTerrain.h"

#include <format>
#include <utility>

namespace Species
{

ClientSession::ClientSession(Desc _desc)
  : m_transport(std::move(_desc.transport)),
    m_showTitle(std::move(_desc.showTitle)),
    m_productName(std::move(_desc.productName)),
    m_tuning(_desc.tuning),
    m_shownTitle(m_productName)
{
  ENGINE_CHECK(m_tuning != nullptr, "the session builds the terrain from the client's own tuning record");
  m_dispatcher.Register<TickReport>([this](const TickReport& _report) { m_serverTick = _report.tick; });
  m_dispatcher.Register<WorldTerrain>([this](const WorldTerrain& _terrain) { ReceiveTerrain(_terrain.seed, _terrain.digest); });
}

void ClientSession::Update()
{
  if (!m_transport)
  {
    return;
  }
  while (!m_refused)
  {
    std::optional<Neuron::ReceivedMessage> message = m_transport->Receive();
    if (!message)
    {
      break;
    }
    static_cast<void>(m_dispatcher.Dispatch(message->bytes));
  }
  if (!m_showTitle)
  {
    return;
  }
  std::string title =
    m_refused ? RefusedTitle(m_productName)
              : SessionTitle(m_productName, m_terrain ? std::optional<std::uint64_t>{m_terrain->seed} : std::nullopt, m_serverTick);
  if (title != m_shownTitle)
  {
    m_showTitle(title);
    m_shownTitle = std::move(title);
  }
}

void ClientSession::ReceiveTerrain(std::uint64_t _seed, const Neuron::Sha256Digest& _digest)
{
  if (m_terrain || m_refused)
  {
    return; // MVP-011: a world's terrain never changes, so a second message changes nothing
  }
  // The client's own library and data, from the seed the server shows; the seed that passed gives its terrain at its
  // first passing attempt, so a client that agrees with the server agrees on the seed too.
  ENGINE_PROFILE_SCOPE("client.terrain.build");
  auto built = GenerateRegionTerrain(_seed, *m_tuning);
  if (built && built->seed == _seed && built->digest == _digest)
  {
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Info, "the terrain of seed {} matches the server's, digest {}", _seed,
                Neuron::ShortHex(_digest));
    m_terrain = std::make_shared<const RegionTerrain>(std::move(*built));
    return;
  }
  Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Error,
              "the world is refused: the server's terrain of seed {} has digest {}, and this client's {}", _seed, Neuron::ShortHex(_digest),
              built ? Neuron::ShortHex(built->digest) : std::string("fails its checks"));
  m_refused = true;
  m_transport->Close();
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

std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _worldSeed, std::optional<std::uint64_t> _serverTick)
{
  std::string title(_productName);
  if (_worldSeed)
  {
    title += std::format(" — seed {}", *_worldSeed);
  }
  if (_serverTick)
  {
    title += std::format(" — tick {}", *_serverTick);
  }
  return title;
}

std::string RefusedTitle(std::string_view _productName)
{
  return std::format("{} — the world is refused: its terrain differs from this game's", _productName);
}

} // namespace Species
