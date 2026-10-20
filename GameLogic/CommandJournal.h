#pragma once

#include "CommandQueue.h"
#include "RegionTerrain.h"
#include "SipHash.h"
#include "StableId.h"
#include "TuningMvp.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

/// What a new world is made from (ADR-109): its simulation key, the seed of its terrain, its first tick, its region
/// count, and whether it is the sandbox of M1 (MVP-005).
struct WorldStart
{
  Neuron::SipHashKey key;
  std::uint64_t seed = 0;
  std::uint64_t firstTick = 0;
  std::uint16_t regions = 1;
  bool sandbox = false;
};

// A new world: made, its terrain attached to region 0 (ADR-102), and populated as the sandbox when it is one (M1 plan
// §6). The world server and the journal's replay make every new world through this one function. _tuning outlives it.
[[nodiscard]] std::unique_ptr<WorldSimulation> StartWorld(const WorldStart& _start, std::shared_ptr<const RegionTerrain> _terrain,
                                                          const TuningMvp& _tuning);

// R-SIM's owners as a journal names them: the world owner, or region r as r + 1.
inline constexpr std::uint16_t JOURNAL_WORLD_OWNER = 0;

/// One entry of the command journal (MVP-076): a steward's connection, which takes the next id of the world owner and,
/// in the sandbox, commands the steward's side; or a command that its owner's queue accepted, with the tick it executes
/// in, its sender, its sequence number and its bytes.
struct JournalEntry
{
  std::uint64_t tick = 0;
  bool connection = false;
  std::uint16_t owner = JOURNAL_WORLD_OWNER;
  Neuron::SenderId sender = 0;
  std::uint64_t sequence = 0;
  std::vector<std::byte> message;

  friend bool operator==(const JournalEntry& _a, const JournalEntry& _b) = default;
};

/// The command journal in its first form (MVP-076, ADR-109): a new world's start and every connection and command from
/// outside the simulation since, in the order they arrived. An AI's commands are not in it: the replayed simulation
/// makes them again. Replayed on the same build, it gives the same world, tick by tick.
struct CommandJournal
{
  WorldStart start;
  std::vector<JournalEntry> entries;
};

// The journal's file form: a version, the start and the entries, canonically (ADR-081 item 7); the file holds the key,
// so it is kept as a snapshot is and never shown or sent (ADR-082 item 1).
[[nodiscard]] std::vector<std::byte> WriteJournal(const CommandJournal& _journal);
// The journal a file holds, or none when it does not read whole.
[[nodiscard]] std::optional<CommandJournal> ReadJournal(std::span<const std::byte> _bytes);

// The world of _journal, replayed: started, then for each tick to _untilTick its connections, its commands submitted to
// their owners' queues in their order, and its step on _workers. _afterTick, when given, sees the world after each tick.
[[nodiscard]] std::unique_ptr<WorldSimulation> ReplayJournal(const CommandJournal& _journal, std::shared_ptr<const RegionTerrain> _terrain,
                                                             const TuningMvp& _tuning, std::uint64_t _untilTick,
                                                             Neuron::WorkerPool& _workers,
                                                             const std::function<void(const WorldSimulation&)>& _afterTick = {});

// A steward's connection to _world (ADR-095, ADR-109): the next id of the world owner and, in the sandbox, the
// steward's side to command. Returns the steward's id.
StableId ConnectSteward(WorldSimulation& _world, bool _sandbox);

} // namespace Species
