#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandJournal.h"
#include "MessageCodec.h"
#include "Sandbox.h"

#include <utility>

namespace Species
{

namespace
{

// The journal's file version, raised with every change of its layout.
constexpr std::uint64_t JOURNAL_FORMAT = 2; // 2: the watched sandbox (O77)
constexpr std::uint32_t JOURNAL_FORMAT_BITS = 32;
// The most entries a journal file holds: a bound for the reader.
constexpr std::uint64_t JOURNAL_ENTRIES_MAX = std::uint64_t{1} << 32;

Neuron::CommandQueue& QueueOf(WorldSimulation& _world, std::uint16_t _owner)
{
  return _owner == JOURNAL_WORLD_OWNER ? _world.Commands() : _world.Region(_owner - 1U).Commands();
}

} // namespace

std::unique_ptr<WorldSimulation> StartWorld(const WorldStart& _start, std::shared_ptr<const RegionTerrain> _terrain,
                                            const TuningMvp& _tuning)
{
  WorldSimulation::Desc desc;
  desc.simulationKey = _start.key;
  desc.nextTick = _start.firstTick;
  desc.regionCount = _start.regions;
  desc.tuning = &_tuning;
  desc.worldSeed = _start.seed;
  auto world = std::make_unique<WorldSimulation>(desc);
  if (_terrain)
  {
    world->Region(0).AttachTerrain(std::move(_terrain));
    if (_start.sandbox)
    {
      PopulateSandbox(world->Region(0), _start.watched);
    }
  }
  return world;
}

StableId ConnectSteward(WorldSimulation& _world, bool _sandbox, bool _watched)
{
  const StableId steward = _world.Ids().Issue();
  if (_sandbox && !_watched)
  {
    _world.Region(0).SetCommander(SANDBOX_STEWARD_TEAM, steward, false);
  }
  return steward;
}

std::vector<std::byte> WriteJournal(const CommandJournal& _journal)
{
  Neuron::BitWriter writer;
  writer.WriteBits(JOURNAL_FORMAT, JOURNAL_FORMAT_BITS);
  writer.WriteBits(_journal.start.key.low, 64);
  writer.WriteBits(_journal.start.key.high, 64);
  writer.WriteBits(_journal.start.seed, 64);
  writer.WriteBits(_journal.start.firstTick, 64);
  writer.WriteBits(_journal.start.regions, 16);
  writer.WriteBool(_journal.start.sandbox);
  writer.WriteBool(_journal.start.watched);
  writer.WriteBounded(_journal.entries.size(), JOURNAL_ENTRIES_MAX);
  for (const JournalEntry& entry : _journal.entries)
  {
    writer.WriteBits(entry.tick, 64);
    writer.WriteBool(entry.connection);
    writer.WriteBits(entry.owner, 16);
    writer.WriteBits(entry.sender, 64);
    writer.WriteBits(entry.sequence, 64);
    writer.WriteBytes(entry.message, Neuron::MESSAGE_MAX_BYTES);
  }
  return writer.Take();
}

std::optional<CommandJournal> ReadJournal(std::span<const std::byte> _bytes)
{
  Neuron::BitReader reader(_bytes);
  if (reader.ReadBits(JOURNAL_FORMAT_BITS) != JOURNAL_FORMAT)
  {
    return std::nullopt;
  }
  CommandJournal journal;
  journal.start.key.low = reader.ReadBits(64);
  journal.start.key.high = reader.ReadBits(64);
  journal.start.seed = reader.ReadBits(64);
  journal.start.firstTick = reader.ReadBits(64);
  journal.start.regions = static_cast<std::uint16_t>(reader.ReadBits(16));
  journal.start.sandbox = reader.ReadBool();
  journal.start.watched = reader.ReadBool();
  const std::uint64_t entries = reader.ReadBounded(JOURNAL_ENTRIES_MAX);
  std::uint64_t previousTick = journal.start.firstTick;
  for (std::uint64_t index = 0; index < entries && !reader.Failed(); ++index)
  {
    JournalEntry entry;
    entry.tick = reader.ReadBits(64);
    entry.connection = reader.ReadBool();
    entry.owner = static_cast<std::uint16_t>(reader.ReadBits(16));
    entry.sender = reader.ReadBits(64);
    entry.sequence = reader.ReadBits(64);
    const std::span<const std::byte> message = reader.ReadBytes(Neuron::MESSAGE_MAX_BYTES);
    entry.message.assign(message.begin(), message.end());
    // Entries are in the order they arrived, so their ticks never fall, and each names an owner the world holds.
    if (entry.tick < previousTick || entry.owner > journal.start.regions)
    {
      return std::nullopt;
    }
    previousTick = entry.tick;
    journal.entries.push_back(std::move(entry));
  }
  if (!reader.Finish() || journal.start.regions == 0)
  {
    return std::nullopt;
  }
  return journal;
}

std::unique_ptr<WorldSimulation> ReplayJournal(const CommandJournal& _journal, std::shared_ptr<const RegionTerrain> _terrain,
                                               const TuningMvp& _tuning, std::uint64_t _untilTick, Neuron::WorkerPool& _workers,
                                               const std::function<void(const WorldSimulation&)>& _afterTick)
{
  std::unique_ptr<WorldSimulation> world = StartWorld(_journal.start, std::move(_terrain), _tuning);
  std::size_t next = 0;
  for (std::uint64_t tick = _journal.start.firstTick; tick < _untilTick; ++tick)
  {
    // Before the step, as the host passes connections and then messages before it (ADR-095).
    for (; next < _journal.entries.size() && _journal.entries[next].tick == tick; ++next)
    {
      const JournalEntry& entry = _journal.entries[next];
      if (entry.connection)
      {
        static_cast<void>(ConnectSteward(*world, _journal.start.sandbox, _journal.start.watched));
      }
      else
      {
        static_cast<void>(QueueOf(*world, entry.owner).Submit(entry.sender, entry.sequence, entry.message));
      }
    }
    world->Step(tick, _workers);
    if (_afterTick)
    {
      _afterTick(*world);
    }
  }
  return world;
}

} // namespace Species
