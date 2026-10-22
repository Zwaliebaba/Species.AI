#pragma once

#include "AssetError.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "Transport.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>

namespace Species
{

/// The in-process server of a session-paced world (O10, MVP-070): the one header through which the client's
/// composition root composes it, offering start, stop and a transport endpoint and nothing else (ADR-083 item 2,
/// ADR-084 item 2, ADR-090 item 4). Behind it run the headless server's library and host, so the integrated and the
/// dedicated server are one program. Its root is a child of the client's: starting and stopping it never tears the
/// client down, and stopping it freezes the world (O4).
class IntegratedServer
{
public:
  struct Desc
  {
    const Neuron::PlatformClock* clock = nullptr;
    const Neuron::LooseContent* content = nullptr; // where the cooked tuning record is loaded from when the server starts (MVP-099)
    std::size_t workers = 2;                       // ADR-080 item 7: the tick thread and at most 2 workers
    std::optional<std::uint64_t> worldSeed;        // MVP-005: the seed the world is built from; none: a seed the server draws
    bool sandbox = false;                          // M1 plan §6: a new world is the battle sandbox
    bool watched = false;                          // O77: the sandbox's AI commands both sides, and the client none
    std::uint32_t syntheticFigures = 0;            // ADR-126: the sandbox's sides are a synthetic battle of this many figures
    std::filesystem::path snapshotFolder;          // MVP-075 (ADR-137): where the world is saved and continued; none: never
  };

  explicit IntegratedServer(const Desc& _desc);
  ~IntegratedServer();

  IntegratedServer(const IntegratedServer&) = delete;
  IntegratedServer& operator=(const IntegratedServer&) = delete;

  // Loads the tuning record, opens the world and starts the tick thread; why the record cannot be loaded when it
  // cannot, and the server stays stopped.
  [[nodiscard]] std::expected<void, Neuron::AssetError> Start();
  void Stop();
  // The client's end of a new in-process connection; the server accepts it at the start of its next tick.
  [[nodiscard]] std::unique_ptr<Neuron::Transport> Connect();

private:
  struct Parts;
  Desc m_desc;
  std::unique_ptr<Parts> m_parts;
};

} // namespace Species
