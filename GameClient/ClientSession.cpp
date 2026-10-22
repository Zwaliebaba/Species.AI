#include "pch.h"

#include "Check.h"
#include "ClientSession.h"
#include "CommandMessages.h"
#include "Log.h"
#include "PresentationEvents.h"
#include "Profile.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <utility>
#include <variant>

namespace Species
{

namespace
{

// The tick period of the client's own record, which the server's equals.
Neuron::Nanoseconds TickPeriodOf(const TuningMvp* _tuning) noexcept
{
  const double seconds = _tuning != nullptr ? _tuning->ref.sim.tickPeriod : 0.0;
  return std::isfinite(seconds) && seconds > 0.0 ? Neuron::Nanoseconds{static_cast<std::int64_t>(std::llround(seconds * 1e9))}
                                                 : Neuron::Nanoseconds{0};
}

} // namespace

ClientSession::ClientSession(Desc _desc)
  : m_transport(std::move(_desc.transport)),
    m_showTitle(std::move(_desc.showTitle)),
    m_productName(std::move(_desc.productName)),
    m_tuning(_desc.tuning),
    m_clock(_desc.clock),
    m_timing(TickPeriodOf(_desc.tuning)),
    m_shownTitle(m_productName)
{
  ENGINE_CHECK(m_tuning != nullptr, "the session builds the terrain from the client's own tuning record");
  ENGINE_CHECK(m_clock != nullptr, "the session times the replica's ticks on the client's clock");
  m_dispatcher.Register<StateUpdate>([this](const StateUpdate& _update) { ReceiveState(_update); });
  m_dispatcher.Register<PresentationEvents>(
    [this](const PresentationEvents& _events)
    {
      for (const EffectEvent& event : _events.events)
      {
        m_events.push_back({_events.tick, event});
      }
    });
  m_dispatcher.Register<WorldTerrain>(
    [this](const WorldTerrain& _terrain)
    {
      m_commandedTeam = _terrain.commandedTeam;
      ReceiveTerrain(_terrain.seed, _terrain.digest);
    });
  m_dispatcher.Register<ServerStats>([this](const ServerStats& _stats) { m_serverStats = _stats; });
  m_dispatcher.Register<SettlementNotice>([this](const SettlementNotice& _notice) { m_notices.push_back(_notice); });
  m_dispatcher.Register<CommandReport>(
    [this](const CommandReport& _report)
    {
      // ADR-124: a refused command is a defect of the client's or a stale request; an executed one with no effect is
      // the ordinary outcome of a command the server found nothing to do for.
      if (_report.result != CommandResult::Applied && _report.result != CommandResult::NoEffect)
      {
        Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Warning, "command {} was refused at tick {}: result {}",
                    _report.sequence, _report.tick, static_cast<int>(_report.result));
      }
      m_reports.push_back(_report);
    });
}

std::vector<CommandReport> ClientSession::TakeReports()
{
  return std::exchange(m_reports, {});
}

std::vector<SettlementNotice> ClientSession::TakeNotices()
{
  return std::exchange(m_notices, {});
}

std::optional<ReplicatedSettlement> ClientSession::OwnSettlement() const
{
  if (!m_commandedTeam)
  {
    return std::nullopt;
  }
  for (const auto& [id, state] : m_replica.Latest())
  {
    if (const auto* settlement = std::get_if<ReplicatedSettlement>(&state); settlement != nullptr && settlement->team == *m_commandedTeam)
    {
      return *settlement;
    }
  }
  return std::nullopt;
}

bool ClientSession::RequestSandboxRestart(bool _newSeed)
{
  if (!m_transport || m_refused)
  {
    return false;
  }
  const bool sent = SendCommand(SandboxRestartCommand{_newSeed}).has_value();
  Neuron::Log(Neuron::LogCategory::Network, sent ? Neuron::LogLevel::Info : Neuron::LogLevel::Warning, "sandbox restart {}: {}",
              _newSeed ? "with a new seed" : "from its seed", sent ? "requested" : "not sent");
  return sent;
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
  CountSides();
  DropOldEvents();
  if (!m_showTitle)
  {
    return;
  }
  // ADR-139: a client with a settlement of its own counts its Workers there, and the sandbox's sides otherwise.
  const std::optional<ReplicatedSettlement> own = OwnSettlement();
  std::string title =
    m_refused
      ? RefusedTitle(m_productName)
      : SessionTitle(m_productName, m_terrain ? std::optional<std::uint64_t>{m_terrain->seed} : std::nullopt, ServerTick(),
                     own ? std::span<const std::uint32_t>{} : std::span<const std::uint32_t>{m_sideFigures}, WaitingForServer(), own);
  if (title != m_shownTitle)
  {
    m_showTitle(title);
    m_shownTitle = std::move(title);
  }
}

void ClientSession::ReceiveState(const StateUpdate& _update)
{
  ENGINE_PROFILE_SCOPE("client.replica.apply");
  m_replica.Apply(_update);
  if (const std::optional<std::uint64_t> latest = m_replica.LatestTick())
  {
    m_timing.Arrived(*latest, m_clock->Now());
  }
}

void ClientSession::CountSides()
{
  std::ranges::fill(m_sideFigures, 0u);
  for (const auto& [id, state] : m_replica.Latest())
  {
    const auto* figure = std::get_if<ReplicatedFigure>(&state);
    if (figure == nullptr)
    {
      continue;
    }
    if (figure->team >= m_sideFigures.size())
    {
      m_sideFigures.resize(std::size_t{figure->team} + 1, 0u); // a side once seen stays, at 0 when it has no figure left
    }
    m_sideFigures[figure->team] += figure->dead ? 0u : 1u;
  }
}

void ClientSession::DropOldEvents()
{
  const std::optional<double> display = m_timing.DisplayTick(m_clock->Now(), m_replica.PreviousTick());
  const double period = std::chrono::duration<double>(m_timing.TickPeriod()).count();
  if (!display || period <= 0.0)
  {
    return;
  }
  // An event of tick t happens between the states of ticks t − 1 and t, so it is due as the display leaves t − 1.
  const double lifeTicks = PRESENTATION_EVENT_LIFE_SECONDS / period;
  std::erase_if(m_events,
                [&](const ReceivedEvent& _received) { return *display - (static_cast<double>(_received.tick) - 1.0) > lifeTicks; });
}

std::vector<ReceivedEvent> ClientSession::TakeEvents()
{
  DropOldEvents();
  std::vector<ReceivedEvent> events(m_events.begin(), m_events.end());
  m_events.clear();
  return events;
}

double ClientSession::InterpolationFraction() const noexcept
{
  return m_timing.Fraction(m_clock->Now());
}

bool ClientSession::WaitingForServer() const noexcept
{
  return m_timing.Waiting(m_clock->Now());
}

void ClientSession::ReceiveTerrain(std::uint64_t _seed, const Neuron::Sha256Digest& _digest)
{
  if (m_refused || (m_terrain && m_terrain->seed == _seed && m_terrain->digest == _digest))
  {
    return; // MVP-011: a world's terrain never changes, so the same terrain again changes nothing
  }
  // A sandbox that restarts from another seed is another world (ADR-109), whose terrain the client builds again; the
  // replica follows by the server's records (ADR-111).
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
    const StateReplica::Counts& replica = m_replica.Totals();
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Info,
                "the session closed at server tick {}, {} messages dropped; {} state updates, {} records, {} without their baseline",
                ServerTick().value_or(0), drops.Total(), replica.updates, replica.records, replica.missingBaselines);
  }
}

bool ClientSession::Connected() const
{
  return m_transport && m_transport->State() == Neuron::ConnectionState::Open;
}

std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _worldSeed, std::optional<std::uint64_t> _serverTick,
                         std::span<const std::uint32_t> _sideFigures, bool _waiting, const std::optional<ReplicatedSettlement>& _settlement)
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
  for (std::size_t side = 0; side < _sideFigures.size(); ++side)
  {
    title += std::format("{}{}", side == 0 ? " — figures " : " : ", _sideFigures[side]);
  }
  if (_settlement)
  {
    title += std::format(" — stores {} of {} — Workers {} of {}", _settlement->storesMilliRu / 1'000, _settlement->capacityMilliRu / 1'000,
                         _settlement->population, _settlement->limit);
  }
  if (_waiting)
  {
    title += " — waiting for the server";
  }
  return title;
}

std::string RefusedTitle(std::string_view _productName)
{
  return std::format("{} — the world is refused: its terrain differs from this game's", _productName);
}

} // namespace Species
