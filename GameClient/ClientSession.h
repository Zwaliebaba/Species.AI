#pragma once

#include "MessageCodec.h"
#include "RegionTerrain.h"
#include "Transport.h"
#include "TuningMvp.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Species
{

/// The client's session with one server (ADR-083 item 2's client-session scope, ADR-084): it owns the client's end of
/// the transport and decodes what the server sends, and never touches server state (MVP-090). It builds the world's
/// terrain from the seed the server sends with its own library and data, and refuses a world whose digest differs
/// (MVP-011, ADR-102). Until the UI exists it shows the seed and the server's tick, or the refusal, in the window title
/// (M0 task T7). Used on the main thread.
class ClientSession
{
public:
  struct Desc
  {
    std::unique_ptr<Neuron::Transport> transport;
    std::function<void(std::string_view)> showTitle; // sets the window's title; none in a test
    std::string productName;                         // the title's first part (PLT-057)
    const TuningMvp* tuning = nullptr;               // the client's own record, outliving the session; the terrain is built from it
  };

  explicit ClientSession(Desc _desc);

  // Once per frame: every message that has arrived, then the title when it changed. The frame that receives the world's
  // terrain builds it (ADR-102).
  void Update();
  void Close();

  [[nodiscard]] std::optional<std::uint64_t> ServerTick() const noexcept
  {
    return m_serverTick;
  }
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

  std::unique_ptr<Neuron::Transport> m_transport;
  std::function<void(std::string_view)> m_showTitle;
  std::string m_productName;
  const TuningMvp* m_tuning;
  Neuron::MessageDispatcher m_dispatcher;
  std::optional<std::uint64_t> m_serverTick;
  std::shared_ptr<const RegionTerrain> m_terrain;
  bool m_refused = false;
  std::string m_shownTitle;
};

// The interim title of M0 (ADR-096) with the seed of MVP-005: "<product> — seed <s> — tick <n>", each part once it is
// known, or the product alone.
[[nodiscard]] std::string SessionTitle(std::string_view _productName, std::optional<std::uint64_t> _worldSeed,
                                       std::optional<std::uint64_t> _serverTick);
// The title of a refused world (MVP-011), until the UI shows the message.
[[nodiscard]] std::string RefusedTitle(std::string_view _productName);

} // namespace Species
