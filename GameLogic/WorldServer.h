#pragma once

#include "ServerHost.h"
#include "Snapshot.h"
#include "StableId.h"
#include "StateHash.h"
#include "TuningMvp.h"
#include "WorldSimulation.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>

namespace Species
{

/// The game's world as the server host runs it (ADR-083, ADR-095): it opens and freezes the world, turns each
/// connection into a steward, passes their messages to the command queue of the owner they address, and publishes each
/// tick to every connection. With a snapshot folder it opens the newest snapshot that validates and saves one when it
/// freezes (MVP-075, ADR-099); without one, or with none that validates, the world opens new and empty. At M0 every
/// connection is a steward, and the tick report is all it publishes.
class WorldServer final : public Neuron::ServerWorld
{
public:
  struct Desc
  {
    std::uint16_t regionCount = 1;
    TuningMvp tuning;                     // read when the server started (MVP-099); the world reads it while it runs
    std::filesystem::path snapshotFolder; // where the world is saved when it freezes and restored when it opens; none: a new
                                          // world each run (ADR-099)
  };

  explicit WorldServer(const Desc& _desc);

  [[nodiscard]] std::uint64_t Open() override;
  void Connect(Neuron::ConnectionId _connection) override;
  void Disconnect(Neuron::ConnectionId _connection) override;
  void Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message) override;
  void DatagramFates(Neuron::ConnectionId _connection, std::span<const Neuron::DatagramOutcome> _outcomes) override;
  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers) override;
  void Publish(std::uint64_t _tick, Neuron::ConnectionSender& _sender) override;
  void Stalled(const Neuron::ScheduleStall& _stall) override;
  void Freeze(std::uint64_t _nextTick) override;

  // The tick thread's, while the host runs; any thread once it has stopped.
  [[nodiscard]] const WorldSimulation* Simulation() const noexcept
  {
    return m_world.get();
  }
  // The hashes at the end of the last tick, which development builds compute every tick (ADR-081 item 8, DBG-147).
  [[nodiscard]] const std::optional<WorldHashes>& LastHashes() const noexcept
  {
    return m_lastHashes;
  }
  [[nodiscard]] const TuningMvp& Tuning() const noexcept
  {
    return m_desc.tuning;
  }

private:
  [[nodiscard]] std::size_t SnapshotsKept() const noexcept;

  struct Steward
  {
    StableId id = 0;
    std::uint64_t commandsReceived = 0; // the sequence number of the next command (ADR-095)
  };

  Desc m_desc;
  std::unique_ptr<WorldSimulation> m_world;
  std::map<Neuron::ConnectionId, Steward> m_stewards;
  std::optional<WorldHashes> m_lastHashes;
};

} // namespace Species
