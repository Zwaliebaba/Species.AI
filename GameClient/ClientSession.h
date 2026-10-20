#pragma once

#include "EffectEvent.h"
#include "MessageCodec.h"
#include "PlatformClock.h"
#include "RegionTerrain.h"
#include "ReplicaTiming.h"
#include "StateReplica.h"
#include "Transport.h"
#include "TuningMvp.h"

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
  // MVP-011: the client refused the world, because the terrain it built from the server's seed differs, and closed.
  [[nodiscard]] bool Refused() const noexcept
  {
    return m_refused;
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
  bool m_refused = false;
  std::string m_shownTitle;
};

// The interim title of M1 (M1 task T12) with the seed of MVP-005: "<product> — seed <s> — tick <n> — figures <a> : <b>",
// each part once it is known, and "— waiting for the server" when the next state is late (ADR-084 item 4).
[[nodiscard]] std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _worldSeed,
                                       std::optional<std::uint64_t> _serverTick, std::span<const std::uint32_t> _sideFigures = {},
                                       bool _waiting = false);
// The title of a refused world (MVP-011), until the UI shows the message.
[[nodiscard]] std::string RefusedTitle(std::string_view _productName);

} // namespace Species
