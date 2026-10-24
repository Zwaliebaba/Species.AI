#pragma once

#include "Sha256.h"
#include "Transport.h"
#include "WorldSaves.h"

#include <cstdint>
#include <expected>
#include <memory>
#include <string>

namespace Species
{

// ADR-187: how long the client waits for a running server to stop, and for the one it started to list itself.
inline constexpr std::uint32_t ALWAYS_ON_STOP_WAIT_MS = 10'000;
inline constexpr std::uint32_t ALWAYS_ON_START_WAIT_MS = 30'000;

/// What reaching an always-on world's server did (ADR-187).
struct AlwaysOnReach
{
  std::unique_ptr<Neuron::Transport> transport; // connecting; its handshake completes on MsQuic's threads
  bool started = false;                         // the client started the world's server
  bool stoppedOther = false;                    // it first stopped a server of another world or another build
};

// MVP-071 (ADR-187): a connection to the server of _world, an always-on world. The server that runs it is reached when it
// is of _build; any other running server is asked to stop, as closing its console does, since one runs at a time
// (O109); then the world's own is started beside the client in a console of its own, which it leaves running when it
// leaves. Waits for each; why it cannot reach it, when it cannot.
[[nodiscard]] std::expected<AlwaysOnReach, std::string> ReachAlwaysOnWorld(const SavedWorld& _world, const Neuron::Sha256Digest& _build);

} // namespace Species
