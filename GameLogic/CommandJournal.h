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
/// count, whether it is the sandbox of M1 (MVP-005), and whether that sandbox is watched (O77): an AI on each side, and
/// a steward who commands none; or, for a world that is no sandbox, the site the steward's settlement is founded on,
/// with the other three sites free (MVP-012, ADR-130).
struct WorldStart
{
  Neuron::SipHashKey key;
  std::uint64_t seed = 0;
  std::uint64_t firstTick = 0;
  std::uint16_t regions = 1;
  bool sandbox = false;
  bool watched = false;
  std::uint32_t syntheticFigures = 0;      // ADR-126: a sandbox whose sides are a synthetic battle of this many figures; 0: none
  std::optional<std::uint8_t> stewardSite; // MVP-012: the local site of the steward's settlement; none: a world with none
};

// A new world: made, its terrain attached to region 0 (ADR-102), and populated as the sandbox when it is one (M1 plan
// §6), or as a synthetic battle when it names one (ADR-126); a world that is no sandbox has its resource sites placed and,
// when the start names the steward's site, the steward's settlement founded there (MVP-012, ADR-130), run by a scripted
// steward when the start is watched (ADR-140). The world server and the journal's replay make every new world through
// this one function. _tuning and _structures outlive it; a world with
// a settlement needs _structures.
[[nodiscard]] std::unique_ptr<WorldSimulation> StartWorld(const WorldStart& _start, std::shared_ptr<const RegionTerrain> _terrain,
                                                          const TuningMvp& _tuning, const StructureRecords* _structures = nullptr);

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

/// The command journal (MVP-076, ADR-109): a world's start and every connection and command from outside the
/// simulation since, in the order they arrived. An AI's commands are not in it: the replayed simulation makes them
/// again. Replayed on the same build, it gives the same world, tick by tick. From ADR-137 it is cut at each snapshot: a
/// restored journal starts from the snapshot of its start's first tick, beside it, and its start's other values only
/// say how its connections were made.
struct CommandJournal
{
  WorldStart start;
  bool restored = false;
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
                                                             const TuningMvp& _tuning, const StructureRecords* _structures,
                                                             std::uint64_t _untilTick, Neuron::WorkerPool& _workers,
                                                             const std::function<void(const WorldSimulation&)>& _afterTick = {});

// As ReplayJournal, onto _world: the world of a restored journal's snapshot, restored, with its terrain attached, whose
// next tick is the journal's first.
[[nodiscard]] std::unique_ptr<WorldSimulation> ReplayJournalOnto(std::unique_ptr<WorldSimulation> _world, const CommandJournal& _journal,
                                                                 std::uint64_t _untilTick, Neuron::WorkerPool& _workers,
                                                                 const std::function<void(const WorldSimulation&)>& _afterTick = {});

// A steward's connection to _world (ADR-095, ADR-109): the next id of the world owner and, in the sandbox, the
// steward's side to command; in a world that is no sandbox, the team of each settlement that a steward runs (ADR-130).
// A watched world's steward commands nothing (O77, ADR-140). Returns the steward's id.
StableId ConnectSteward(WorldSimulation& _world, bool _sandbox, bool _watched = false);

} // namespace Species
