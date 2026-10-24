#pragma once

#include "AwayMessages.h"
#include "CommandMessages.h"
#include "EffectEvent.h"
#include "JoinMessages.h"
#include "MessageCodec.h"
#include "PlatformClock.h"
#include "RegionTerrain.h"
#include "RelationMessages.h"
#include "ReplicaTiming.h"
#include "ServerStats.h"
#include "SettlementMessages.h"
#include "StateReplica.h"
#include "Transport.h"
#include "TuningMvp.h"
#include "WorldPace.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Species
{

/// A presentation event as the client received it, with its tick (ADR-084 item 5).
struct ReceivedEvent
{
  std::uint64_t tick = 0;
  EffectEvent event;
};

/// The client's session with one server (ADR-083 item 2's client-session scope, ADR-084): it owns the client's end of
/// the transport and decodes what the server sends, and never touches server state (MVP-090). It builds the world's
/// terrain from the seed the server sends with its own library and data, again when a restarted sandbox sends another,
/// and refuses a world whose digest differs (MVP-011, ADR-102, ADR-109). It keeps the replica of the state the server sends, with the arrival time of its latest tick,
/// and the droppable presentation events until they are taken or too old (ADR-111). Until the UI exists it shows the
/// seed, the replica's tick and each side's figures, or the refusal, in the window title (M1 task T12). Used on the
/// main thread.
class ClientSession
{
public:
  struct Desc
  {
    std::unique_ptr<Neuron::Transport> transport;
    std::function<void(std::string_view)> showTitle; // sets the window's title; none in a test
    std::string productName;                         // the title's first part (PLT-057)
    const TuningMvp* tuning = nullptr;               // the client's own record, outliving the session; the terrain is built from it
    const Neuron::PlatformClock* clock = nullptr;    // when each tick arrives, outliving the session
    std::optional<JoinIdentity> identity;            // ADR-189: sent as the connection's first message; none: no join (a test's)
    WorldPace pace = WorldPace::Session;             // MVP-071 (ADR-190): the world's pace, from its description
  };

  explicit ClientSession(Desc _desc);

  // Once per frame: every message that has arrived, then the title when it changed. The frame that receives the world's
  // terrain builds it (ADR-102).
  void Update();
  void Close();

  // The replica's latest tick: the newest state the server sent that has arrived.
  [[nodiscard]] std::optional<std::uint64_t> ServerTick() const noexcept
  {
    return m_replica.LatestTick();
  }
  [[nodiscard]] const StateReplica& Replica() const noexcept
  {
    return m_replica;
  }
  // ADR-084 item 4: where the frame drawn now stands between the replica's two latest ticks, and whether the next
  // state is later than the hold allows.
  [[nodiscard]] double InterpolationFraction() const noexcept;
  [[nodiscard]] bool WaitingForServer() const noexcept;
  // The figures of each team slot in the replica's latest view that are alive, from slot 0 to the highest slot seen.
  [[nodiscard]] const std::vector<std::uint32_t>& SideFigures() const noexcept
  {
    return m_sideFigures;
  }
  // The presentation events received and not yet taken, in the order they arrived, without those older than
  // PRESENTATION_EVENT_LIFE_SECONDS behind the display time.
  [[nodiscard]] std::vector<ReceivedEvent> TakeEvents();
  // The world's terrain once the client has built it and its digest matched the server's.
  [[nodiscard]] const RegionTerrain* Terrain() const noexcept
  {
    return m_terrain.get();
  }
  // ADR-119: the team this client commands, as the server's terrain message names it; none before it arrives.
  [[nodiscard]] std::optional<std::uint8_t> CommandedTeam() const noexcept
  {
    return m_commandedTeam;
  }
  // ADR-120: the latest of the server's tick statistics, which a Debug server sends once a second of ticks; none from a
  // Release server.
  [[nodiscard]] const std::optional<ServerStats>& ServerStatistics() const noexcept
  {
    return m_serverStats;
  }
  // ADR-109, ADR-120: asks the server to restart its sandbox from its seed or a new one; a server that runs no sandbox,
  // or a Release server, ignores it. False when the request was not sent.
  bool RequestSandboxRestart(bool _newSeed);
  // ADR-095, ADR-124: sends a command on the reliable channel and returns the sequence number the server gives it, since
  // the server numbers every reliable message of the connection in order; none when it was not sent.
  template <class T> std::optional<std::uint64_t> SendCommand(const T& _command)
  {
    if (!m_transport || m_refused || m_joinRefused ||
        !m_transport->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(_command)).has_value())
    {
      return std::nullopt;
    }
    return m_commandsSent++;
  }
  // MVP-072 (ADR-184): once per frame, whether the steward gave input in it. A frame with input sends a presence report
  // when none has gone for PRESENCE_REPORT_SECONDS, so that the server knows its steward is present.
  void NoteInput(bool _input);
  // ADR-095: the commands' outcomes received and not yet taken, in the order they arrived.
  [[nodiscard]] std::vector<CommandReport> TakeReports();
  // ADR-139: the guaranteed events received and not yet taken, in the order they arrived, for the HUD's notifications
  // (MVP-102).
  [[nodiscard]] std::vector<SettlementNotice> TakeNotices();
  // ADR-165: the battle results received and not yet taken, in the order they arrived (MVP-048).
  [[nodiscard]] std::vector<BattleResultNotice> TakeBattleResults();
  // ADR-139: the settlement of the team this client commands in the replica's latest view; none in the sandbox, or
  // before it arrives.
  [[nodiscard]] std::optional<ReplicatedSettlement> OwnSettlement() const;
  // MVP-098 (ADR-152): the latest settlement status the server answered a ReadSettlementStatusCommand with; none before
  // the first.
  [[nodiscard]] const std::optional<SettlementStatus>& LatestStatus() const noexcept
  {
    return m_status;
  }
  // MVP-098 (ADR-165): the latest valley status the server answered a ReadValleyStatusCommand with; none before the first.
  [[nodiscard]] const std::optional<ValleyStatus>& LatestValleyStatus() const noexcept
  {
    return m_valleyStatus;
  }
  // MVP-074 (ADR-186): the latest away report the server sent, asked for or sent at a return; none before the first.
  [[nodiscard]] const std::optional<AwayReport>& LatestAwayReport() const noexcept
  {
    return m_awayReport;
  }
  // O110: an away report of a return with news arrived since the last call, so that its page opens first.
  [[nodiscard]] bool TakeAwayReportArrived() noexcept;
  // MVP-071 (ADR-190): the world's pace.
  [[nodiscard]] WorldPace Pace() const noexcept
  {
    return m_pace;
  }
  // MVP-063 (ADR-169): the latest epilogue the server answered a ReadEpilogueCommand with; none before the first.
  [[nodiscard]] const std::optional<Epilogue>& LatestEpilogue() const noexcept
  {
    return m_epilogue;
  }
  // MVP-005: the seed of the world whose terrain the client built; none before it.
  [[nodiscard]] std::optional<std::uint64_t> WorldSeed() const noexcept;
  // MVP-011: the client refused the world, because the terrain it built from the server's seed differs, and closed.
  [[nodiscard]] bool Refused() const noexcept
  {
    return m_refused;
  }
  // ADR-189: the server refused the client's join, with its reason and the server's identity, and closed.
  [[nodiscard]] const std::optional<JoinAnswer>& JoinRefused() const noexcept
  {
    return m_joinRefused;
  }
  [[nodiscard]] bool Connected() const;
  [[nodiscard]] const Neuron::MessageDropCounts& Drops() const noexcept
  {
    return m_dispatcher.Drops();
  }

private:
  void ReceiveTerrain(std::uint64_t _seed, const Neuron::Sha256Digest& _digest);
  void ReceiveState(const StateUpdate& _update);
  void CountSides();
  void DropOldEvents();

  std::unique_ptr<Neuron::Transport> m_transport;
  std::function<void(std::string_view)> m_showTitle;
  std::string m_productName;
  const TuningMvp* m_tuning;
  const Neuron::PlatformClock* m_clock;
  Neuron::MessageDispatcher m_dispatcher;
  StateReplica m_replica;
  ReplicaTiming m_timing;
  std::deque<ReceivedEvent> m_events;
  std::vector<std::uint32_t> m_sideFigures;
  std::shared_ptr<const RegionTerrain> m_terrain;
  std::optional<std::uint8_t> m_commandedTeam;
  std::optional<ServerStats> m_serverStats;
  std::vector<CommandReport> m_reports;
  std::vector<SettlementNotice> m_notices;
  std::optional<SettlementStatus> m_status;
  std::optional<ValleyStatus> m_valleyStatus;
  std::optional<Epilogue> m_epilogue;
  std::vector<BattleResultNotice> m_results;
  std::uint64_t m_commandsSent = 0; // the reliable messages sent after the join, which the server numbers from 0
  bool m_refused = false;
  std::optional<JoinAnswer> m_joinRefused;
  std::optional<Neuron::Nanoseconds> m_lastPresenceReport; // MVP-072: when the latest presence report went
  std::optional<AwayReport> m_awayReport;
  bool m_awayReportArrived = false;
  WorldPace m_pace = WorldPace::Session;
  std::string m_shownTitle;
};

// The interim title of M1 (M1 task T12) with the seed of MVP-005: "<product> — seed <s> — tick <n> — figures <a> : <b>",
// each part once it is known, then, from M2 task T11, the client's own settlement's "— stores <s> of <c> — Workers <p>
// of <l>" in whole resource units, which the session shows in place of the sides (ADR-139), and "— waiting for the
// server" when the next state is late (ADR-084 item 4).
[[nodiscard]] std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _worldSeed,
                                       std::optional<std::uint64_t> _serverTick, std::span<const std::uint32_t> _sideFigures = {},
                                       bool _waiting = false, const std::optional<ReplicatedSettlement>& _settlement = std::nullopt);
// The title of a refused world (MVP-011), until the UI shows the message.
[[nodiscard]] std::string RefusedTitle(std::string_view _productName);
// ADR-189: the title of a refused join, with its reason, until the UI shows the message.
[[nodiscard]] std::string JoinRefusedTitle(std::string_view _productName, JoinRefusal _refusal);

} // namespace Species
