#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "Check.h"
#include "CommandJournal.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "Sandbox.h"

#include <algorithm>
#include <array>
#include <utility>

namespace Species
{

namespace
{

// The journal's file version, raised with every change of its layout.
// 2: the watched sandbox (O77); 3: the synthetic battle's figures (ADR-126); 4: the steward's site (ADR-130); 5: a
// journal that starts from the snapshot of its first tick (ADR-137); 6: the AI settlements (ADR-157); 7: the world's
// pace (ADR-184); 8: the steward's presence changes (ADR-185). A journal of format 5 is read as one with no AI
// settlement, one of format 5 or 6 as a world at the session pace, and one of format 7 or older as one whose steward was
// always present (MVP-095).
constexpr std::uint64_t JOURNAL_FORMAT = 8;
constexpr std::uint64_t JOURNAL_FORMAT_WITHOUT_PRESENCE = 7;
constexpr std::uint64_t JOURNAL_FORMAT_WITHOUT_PACE = 6;
constexpr std::uint64_t JOURNAL_FORMAT_WITHOUT_AI = 5;
// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;
constexpr std::uint32_t JOURNAL_FORMAT_BITS = 32;
// The most entries a journal file holds: a bound for the reader.
constexpr std::uint64_t JOURNAL_ENTRIES_MAX = std::uint64_t{1} << 32;

Neuron::CommandQueue& QueueOf(WorldSimulation& _world, std::uint16_t _owner)
{
  return _owner == JOURNAL_WORLD_OWNER ? _world.Commands() : _world.Region(_owner - 1U).Commands();
}

// MVP-013 and MVP-057 (ADR-157): the start's AI settlements at the free sites in site order, the three temperaments dealt
// among them by one keyed draw of the region's first tick, so that no two alike; each run by a scripted steward.
void FoundAiSettlements(RegionSimulation& _region, std::uint8_t _count)
{
  std::array<Temperament, TEMPERAMENT_MAX> deal{Temperament::Bold, Temperament::Territorial, Temperament::Wary};
  _region.Draws().SetTick(_region.NextTick());
  Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::TemperamentDeal, 1);
  for (std::size_t index = deal.size() - 1; index > 0; --index)
  {
    std::swap(deal[index], deal[draw.NextBelow(index + 1)]);
  }
  std::size_t founded = 0;
  const std::size_t sites = _region.Terrain()->layout.sites.size();
  for (std::size_t site = 0; site < sites && founded < std::min<std::size_t>(_count, deal.size()); ++site)
  {
    if (_region.SettlementAt(static_cast<std::uint8_t>(site)) == nullptr)
    {
      const Settlement& settlement = _region.FoundSettlement(static_cast<std::uint8_t>(site), false, deal[founded++]);
      static_cast<void>(_region.AddScriptedSteward(settlement.team));
    }
  }
}

} // namespace

std::unique_ptr<WorldSimulation> StartWorld(const WorldStart& _start, std::shared_ptr<const RegionTerrain> _terrain,
                                            const TuningMvp& _tuning, const StructureRecords* _structures)
{
  WorldSimulation::Desc desc;
  desc.simulationKey = _start.key;
  desc.nextTick = _start.firstTick;
  desc.regionCount = _start.regions;
  desc.tuning = &_tuning;
  desc.worldSeed = _start.seed;
  desc.structures = _structures;
  desc.pace = _start.pace;
  auto world = std::make_unique<WorldSimulation>(desc);
  if (_terrain)
  {
    world->Region(0).AttachTerrain(std::move(_terrain));
    if (_start.sandbox && _start.syntheticFigures > 0)
    {
      PopulateSyntheticBattle(world->Region(0), _start.syntheticFigures);
    }
    else if (_start.sandbox)
    {
      PopulateSandbox(world->Region(0), _start.watched);
    }
    else if (_structures != nullptr)
    {
      world->Region(0).PlaceResourceSites();
      if (_start.stewardSite)
      {
        const Settlement& settlement = world->Region(0).FoundSettlement(_start.stewardSite.value_or(0), true);
        if (_start.watched)
        {
          // ADR-140: a Debug build's --watch hands the steward's settlement to the scripted steward of ADR-138.
          static_cast<void>(world->Region(0).AddScriptedSteward(settlement.team));
        }
      }
      if (_start.aiSettlements > 0)
      {
        FoundAiSettlements(world->Region(0), _start.aiSettlements);
      }
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
  else if (!_sandbox && !_watched)
  {
    // The MVP's world has one steward; each settlement a steward runs is the latest steward's, taken from a scripted
    // steward that ran it (ADR-156).
    RegionSimulation& region = _world.Region(0);
    std::vector<std::uint8_t> teams;
    for (const Settlement& settlement : region.Settlements())
    {
      if (settlement.stewarded)
      {
        teams.push_back(settlement.team);
      }
    }
    for (const std::uint8_t team : teams)
    {
      region.RemoveScriptedStewards(team);
      region.SetCommander(team, steward, false);
      // ADR-161: and its expedition, while one is out.
      if (ExpeditionOf(region, team) != nullptr)
      {
        region.SetCommander(ExpeditionTeamOf(team), steward, false);
      }
    }
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
  writer.WriteBits(_journal.start.syntheticFigures, 32);
  writer.WriteBool(_journal.start.stewardSite.has_value());
  if (_journal.start.stewardSite)
  {
    writer.WriteBounded(_journal.start.stewardSite.value_or(0), LOCAL_SITE_MAX);
  }
  writer.WriteBounded(_journal.start.aiSettlements, LOCAL_SITE_MAX);
  writer.WriteBounded(static_cast<std::uint64_t>(_journal.start.pace), WORLD_PACE_MAX);
  writer.WriteBool(_journal.restored);
  writer.WriteBounded(_journal.entries.size(), JOURNAL_ENTRIES_MAX);
  for (const JournalEntry& entry : _journal.entries)
  {
    writer.WriteBits(entry.tick, 64);
    writer.WriteBool(entry.connection);
    writer.WriteBool(entry.presence);
    if (entry.presence)
    {
      writer.WriteBool(entry.away);
    }
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
  const std::uint64_t format = reader.ReadBits(JOURNAL_FORMAT_BITS);
  if (format < JOURNAL_FORMAT_WITHOUT_AI || format > JOURNAL_FORMAT)
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
  journal.start.syntheticFigures = static_cast<std::uint32_t>(reader.ReadBits(32));
  if (reader.ReadBool())
  {
    journal.start.stewardSite = static_cast<std::uint8_t>(reader.ReadBounded(LOCAL_SITE_MAX));
  }
  if (format >= JOURNAL_FORMAT_WITHOUT_PACE)
  {
    journal.start.aiSettlements = static_cast<std::uint8_t>(reader.ReadBounded(LOCAL_SITE_MAX));
  }
  if (format >= JOURNAL_FORMAT_WITHOUT_PRESENCE)
  {
    journal.start.pace = static_cast<WorldPace>(reader.ReadBounded(WORLD_PACE_MAX));
  }
  journal.restored = reader.ReadBool();
  const std::uint64_t entries = reader.ReadBounded(JOURNAL_ENTRIES_MAX);
  std::uint64_t previousTick = journal.start.firstTick;
  for (std::uint64_t index = 0; index < entries && !reader.Failed(); ++index)
  {
    JournalEntry entry;
    entry.tick = reader.ReadBits(64);
    entry.connection = reader.ReadBool();
    entry.presence = format >= JOURNAL_FORMAT && reader.ReadBool();
    entry.away = entry.presence && reader.ReadBool();
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
                                               const TuningMvp& _tuning, const StructureRecords* _structures, std::uint64_t _untilTick,
                                               Neuron::WorkerPool& _workers, const std::function<void(const WorldSimulation&)>& _afterTick)
{
  return ReplayJournalOnto(StartWorld(_journal.start, std::move(_terrain), _tuning, _structures), _journal, _untilTick, _workers,
                           _afterTick);
}

std::unique_ptr<WorldSimulation> ReplayJournalOnto(std::unique_ptr<WorldSimulation> _world, const CommandJournal& _journal,
                                                   std::uint64_t _untilTick, Neuron::WorkerPool& _workers,
                                                   const std::function<void(const WorldSimulation&)>& _afterTick)
{
  ENGINE_CHECK(_world->NextTick() == _journal.start.firstTick, "a journal from tick {} replays onto a world at tick {}",
               _journal.start.firstTick, _world->NextTick());
  std::unique_ptr<WorldSimulation> world = std::move(_world);
  std::size_t next = 0;
  for (std::uint64_t tick = _journal.start.firstTick; tick < _untilTick; ++tick)
  {
    // Before the step, as the host passes connections and then messages before it (ADR-095).
    for (; next < _journal.entries.size() && _journal.entries[next].tick == tick; ++next)
    {
      const JournalEntry& entry = _journal.entries[next];
      if (entry.presence)
      {
        static_cast<void>(SetStewardPresence(*world, entry.away));
      }
      else if (entry.connection)
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
