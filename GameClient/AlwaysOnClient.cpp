#include "pch.h"

#include "AlwaysOnClient.h"
#include "AlwaysOnServer.h"
#include "Files.h"
#include "InstanceLock.h"
#include "Log.h"
#include "ProgramStart.h"
#include "QuicTransport.h"
#include "TextEncoding.h"

#include <array>
#include <chrono>
#include <optional>
#include <thread>
#include <utility>

namespace Species
{

namespace
{

constexpr std::uint32_t POLL_MS = 50;

// Polls _done every POLL_MS until it holds or _milliseconds have passed; whether it held.
template <class Fn> bool WaitUntil(std::uint32_t _milliseconds, Fn&& _done)
{
  for (std::uint32_t waited = 0; !_done(); waited += POLL_MS)
  {
    if (waited >= _milliseconds)
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MS));
  }
  return true;
}

// The listing of the server that runs the world of _folder, when its process runs.
std::optional<ServerListing> RunningListing(const std::filesystem::path& _folder)
{
  std::optional<ServerListing> listing = ReadServerListing(_folder);
  return listing.has_value() && Neuron::ProcessRunning(listing->processId) ? listing : std::nullopt;
}

} // namespace

std::expected<AlwaysOnReach, std::string> ReachAlwaysOnWorld(const SavedWorld& _world, const Neuron::Sha256Digest& _build,
                                                             std::uint16_t _linkPort)
{
  AlwaysOnReach reach;
  std::optional<ServerListing> listing;
  if (Neuron::InstanceLock::Held(ALWAYS_ON_LOCK))
  {
    listing = RunningListing(_world.folder);
    if (!listing.has_value() || listing->build != _build)
    {
      // O109: another world's server, or this world's of another build (ADR-186), stops as closing its console stops it.
      Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Info, "the running always-on server is {}; it is asked to stop",
                  listing.has_value() ? "of another build" : "another world's");
      if (!Neuron::StopSignal::Raise(ALWAYS_ON_STOP) ||
          !WaitUntil(ALWAYS_ON_STOP_WAIT_MS, [] { return !Neuron::InstanceLock::Held(ALWAYS_ON_LOCK); }))
      {
        return std::unexpected(std::string("the running always-on world does not stop"));
      }
      reach.stoppedOther = true;
      listing.reset();
    }
  }
  if (!listing.has_value())
  {
    const std::filesystem::path program = Neuron::ExecutableFolder() / SERVER_PROGRAM;
    const std::array<std::wstring, 2> arguments{std::wstring(ALWAYS_ON_WORLD_OPTION), _world.folder.wstring()};
    const std::expected<std::uint32_t, std::string> started = Neuron::StartInOwnConsole(program, arguments);
    if (!started.has_value())
    {
      return std::unexpected(std::format("{} does not start: {}", Neuron::ToUtf8(program.wstring()), started.error()));
    }
    reach.started = true;
    const std::uint32_t process = *started;
    Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Info, "the always-on server of {} started as process {}",
                Neuron::ToUtf8(_world.folder.wstring()), process);
    // Its listing, which it writes once it listens; a server that ends first has said why in its console and log.
    const bool listed = WaitUntil(ALWAYS_ON_START_WAIT_MS,
                                  [&]
                                  {
                                    listing = ReadServerListing(_world.folder);
                                    return (listing.has_value() && listing->processId == process) || !Neuron::ProcessRunning(process);
                                  });
    if (!listed || !listing.has_value() || listing->processId != process)
    {
      return std::unexpected(std::string("the world's server did not start; its console and log say why"));
    }
  }
  std::expected<std::unique_ptr<Neuron::QuicTransport>, std::string> transport =
    Neuron::QuicTransport::Connect({_linkPort != 0 ? _linkPort : listing->port, listing->fingerprint});
  if (!transport.has_value())
  {
    return std::unexpected(std::format("the connection to the world's server does not start: {}", transport.error()));
  }
  reach.transport = std::move(*transport);
  return reach;
}

} // namespace Species
