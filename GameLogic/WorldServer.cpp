#include "pch.h"

#include "AwayMessages.h"
#include "Check.h"
#include "Development.h"
#include "Files.h"
#include "Log.h"
#include "MarkerTimings.h"
#include "Profile.h"
#include "SecureRandom.h"
#include "Presence.h"
#include "PresentationEvents.h"
#include "ServerStats.h"
#include "SettlementCommands.h"
#include "WorldServer.h"
#include "WorldSnapshot.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

// MVP-005: a seed the server draws; it has 32 bits, so that it can be typed again.
std::uint64_t DrawWorldSeed()
{
  std::array<std::byte, 4> drawn{};
  Neuron::FillSecureRandom(drawn);
  return std::bit_cast<std::uint32_t>(drawn);
}

// ADR-106: the outcome as the client reads it.
CommandResult ResultOf(const Neuron::CommandOutcome& _outcome) noexcept
{
  if (!_outcome.rejection)
  {
    return _outcome.effect == Neuron::CommandEffect::Applied ? CommandResult::Applied : CommandResult::NoEffect;
  }
  switch (*_outcome.rejection)
  {
  case Neuron::CommandRejection::UnknownType:
    return CommandResult::UnknownType;
  case Neuron::CommandRejection::WrongVersion:
    return CommandResult::WrongVersion;
  case Neuron::CommandRejection::Oversized:
    return CommandResult::Oversized;
  case Neuron::CommandRejection::Malformed:
    return CommandResult::Malformed;
  case Neuron::CommandRejection::NotPermitted:
    return CommandResult::NotPermitted;
  case Neuron::CommandRejection::Duplicate:
    break;
  }
  return CommandResult::Duplicate;
}

// ADR-139: the notice of a structure completed, with its kind, which the pool still holds in the tick that raised it.
SettlementNotice NoticeOf(const RegionSimulation& _region, const EffectEvent& _event, std::uint64_t _tick)
{
  SettlementNotice notice{_tick,          NoticeKind::StructureCompleted, _event.team,
                          _event.subject, StructureKind::Dwelling,        QuantizePosition(_event.position)};
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).id == _event.subject)
    {
      notice.structure = structures.At(slot).kind;
      break;
    }
  }
  return notice;
}

// ADR-137: the tick of a file named world-<tick>.<suffix>, as the snapshots and journals are; none for another name.
std::optional<std::uint64_t> TickOfFile(const std::filesystem::path& _path)
{
  const std::wstring stem = _path.stem().wstring();
  constexpr std::wstring_view PREFIX = L"world-";
  if (!stem.starts_with(PREFIX) || stem.size() == PREFIX.size() ||
      !std::ranges::all_of(stem.substr(PREFIX.size()), [](wchar_t _c) { return _c >= L'0' && _c <= L'9'; }))
  {
    return std::nullopt;
  }
  std::uint64_t tick = 0;
  for (const wchar_t digit : stem.substr(PREFIX.size()))
  {
    tick = tick * 10 + static_cast<std::uint64_t>(digit - L'0');
  }
  return tick;
}

// PIL-131: the tick that the marker of a clean stop names, or none when there is no marker or it names nothing.
std::optional<std::uint64_t> CleanStopTick(const std::filesystem::path& _marker)
{
  const auto bytes = Neuron::ReadWholeFile(_marker);
  if (!bytes || bytes->empty() || bytes->size() > 20 ||
      !std::ranges::all_of(*bytes, [](std::byte _b) { return _b >= std::byte{'0'} && _b <= std::byte{'9'}; }))
  {
    return std::nullopt;
  }
  std::uint64_t tick = 0;
  for (const std::byte digit : *bytes)
  {
    tick = tick * 10 + (std::to_integer<std::uint64_t>(digit) - '0');
  }
  return tick;
}

// A message the server answers itself rather than queueing (ADR-109, ADR-136): the checks of ADR-087 item 2 that need
// no state, and the message decoded when they pass.
template <Neuron::MessageCodec T>
CommandResult CheckAnswered(const Neuron::MessageHeader& _header, Neuron::BitReader& _reader, std::size_t _bytes, T& _message)
{
  if (_header.version != T::VERSION)
  {
    return CommandResult::WrongVersion;
  }
  if (_bytes > T::MAX_BYTES)
  {
    return CommandResult::Oversized;
  }
  _message.Decode(_reader);
  return _reader.Finish() ? CommandResult::Applied : CommandResult::Malformed;
}

// ADR-165: the settlement at _site, when the steward commands it.
const Settlement* CommandedAt(const RegionSimulation& _region, StableId _steward, std::optional<std::uint8_t> _site)
{
  const Settlement* settlement = _site ? _region.SettlementAt(*_site) : nullptr;
  return settlement != nullptr && _region.CommanderOf(settlement->team).sender == _steward ? settlement : nullptr;
}

bool IsClaimNotice(NoticeKind _kind) noexcept
{
  return _kind == NoticeKind::ClaimMade || _kind == NoticeKind::ClaimWithdrawn || _kind == NoticeKind::ClaimLost ||
         _kind == NoticeKind::ClaimWon;
}

// MVP-102 (ADR-165): the notice of a valley event for the steward, when it concerns a settlement the steward commands:
// the one that acted, else the other one concerned, else, for a claim on the central resource site, one whose own claim
// on it stands.
std::optional<SettlementNotice> NoticeOf(const RegionSimulation& _region, const ValleyEvent& _event, StableId _steward, std::uint64_t _tick)
{
  const Settlement* ours = CommandedAt(_region, _steward, _event.site);
  const Settlement* concerned = ours != nullptr ? ours : CommandedAt(_region, _steward, _event.other);
  if (concerned == nullptr && IsClaimNotice(_event.kind) && _event.detail == CLAIM_AREA_CENTRAL)
  {
    for (const Claim& claim : _region.Claims())
    {
      concerned = claim.area == CLAIM_AREA_CENTRAL ? CommandedAt(_region, _steward, claim.claimant) : nullptr;
      if (concerned != nullptr)
      {
        break;
      }
    }
  }
  if (concerned == nullptr)
  {
    return std::nullopt;
  }
  SettlementNotice notice;
  notice.tick = _tick;
  notice.kind = _event.kind;
  notice.team = concerned->team;
  notice.subject = _event.subject;
  notice.position = QuantizePosition(_event.position);
  notice.other = ours != nullptr ? _event.other : std::optional<std::uint8_t>(_event.site);
  notice.ours = ours != nullptr;
  notice.detail = _event.detail;
  return notice;
}

// MVP-048 (ADR-165): the result of the battle a BattleEnded event names, with the area its claim's end named in the
// same tick, as the steward sees it; none when the region holds no such result.
std::optional<BattleResultNotice> ResultFor(const RegionSimulation& _region, const ValleyEvent& _ended, StableId _steward)
{
  const auto result =
    std::ranges::find(_region.BattleResults().rbegin(), _region.BattleResults().rend(), _ended.subject, &BattleResult::expedition);
  if (result == _region.BattleResults().rend())
  {
    return std::nullopt;
  }
  std::optional<std::uint8_t> area;
  for (const ValleyEvent& event : _region.ValleyEvents())
  {
    if ((event.kind == NoticeKind::ClaimWon || event.kind == NoticeKind::ClaimLost) && event.subject == result->claim && result->claim != 0)
    {
      area = event.detail;
    }
  }
  const bool side =
    CommandedAt(_region, _steward, result->attacker) != nullptr || CommandedAt(_region, _steward, result->defender) != nullptr;
  return ResultNoticeOf(_region, *result, area, side);
}

} // namespace

WorldServer::WorldServer(const Desc& _desc)
  : m_desc(_desc)
{
  // ADR-126: a synthetic battle is a sandbox whose two sides the AI commands.
  if (m_desc.syntheticFigures > 0)
  {
    m_desc.sandbox = true;
    m_desc.watched = true;
  }
}

std::uint64_t WorldServer::Open()
{
  if (!m_desc.snapshotFolder.empty())
  {
    // PIL-131 (ADR-137): a world that froze cleanly left a marker naming its final snapshot. It goes as the world opens,
    // so that a stop that leaves none is unclean; ADR-155: a world that is only continued keeps it until it continues.
    const std::filesystem::path marker = m_desc.snapshotFolder / CLEAN_STOP_FILE_NAME;
    const std::optional<std::uint64_t> cleanTick = CleanStopTick(marker);
    const auto removeMarker = [&marker]
    {
      if (!Neuron::RemoveFile(marker))
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the marker of a clean stop could not be removed");
      }
    };
    // MVP-075: the newest snapshot that validates; a refused one is logged, kept with its reason, and the next older one
    // tried.
    const Neuron::SnapshotStore store(m_desc.snapshotFolder, SnapshotsKept());
    for (const std::filesystem::path& snapshot : store.Snapshots())
    {
      auto restored = Neuron::SnapshotStore::Load(snapshot).and_then(
        [this](const Neuron::SnapshotContents& _contents) { return RestoreWorld(_contents, m_desc.tuning, &m_desc.structures); });
      if (!restored)
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "the snapshot {} is refused: {}",
                    snapshot.filename().string(), Neuron::SnapshotErrorText(restored.error()));
        m_opened.refusals.push_back({snapshot, restored.error()});
        continue;
      }
      // The terrain is not state: the restored world builds it again from its seed (ADR-102).
      std::shared_ptr<const RegionTerrain> terrain = BuildTerrain((*restored)->WorldSeed());
      if (!terrain)
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "the snapshot {} is refused: its seed gives no terrain",
                    snapshot.filename().string());
        m_opened.refusals.push_back({snapshot, std::nullopt});
        continue;
      }
      removeMarker();
      m_opened.restored = true;
      Adopt(std::move(*restored), std::move(terrain));
      // ADR-165: a world that continues in its quiet minutes, or ended, tells no steward again.
      m_runEnded = m_world->RunState().ended;
      m_runQuiet = m_runEnded || m_world->Region(0).InQuietMinutes();
      // MVP-075, PIL-131: a world that did not freeze cleanly at this snapshot lost what it ran after it, a freeze of
      // cause "unclean stop" from the snapshot's tick.
      if (cleanTick != m_world->NextTick())
      {
        const std::optional<std::int64_t> now = CalendarSeconds();
        const std::int64_t saved = m_world->Record().savedAtSeconds;
        const std::uint64_t seconds = now && saved > 0 && *now > saved ? static_cast<std::uint64_t>(*now - saved) : 0;
        m_world->Record().freezes.push_back({m_world->NextTick(), FreezeCause::UncleanStop, seconds});
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning,
                    "the world did not stop cleanly: a freeze of {} s from tick {}, cause unclean stop", seconds, m_world->NextTick());
      }
      m_journal = RestoredJournal(); // MVP-076: the journal from the snapshot it opened (ADR-137)
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world opens from {} at tick {}",
                  snapshot.filename().string(), m_world->NextTick());
      return m_world->NextTick();
    }
    if (m_desc.restoreOnly)
    {
      // ADR-155: nothing to continue; the world opens new without its folder, which keeps every snapshot it refused.
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning,
                  "no snapshot of {} can be continued; the world opens new and saves nothing there", m_desc.snapshotFolder.string());
      m_desc.snapshotFolder.clear();
    }
    else
    {
      removeMarker();
    }
  }
  // MVP-005: a seed the steward gave, or one the server draws.
  if (!StartNew(m_desc.worldSeed ? *m_desc.worldSeed : DrawWorldSeed(), 0))
  {
    throw std::runtime_error("no seed that the re-roll reaches gives a region that passes every check");
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "a new {} opens with {} region(s) from seed {}",
              m_desc.sandbox ? "sandbox" : "world", m_desc.regionCount, m_world->WorldSeed());
  return m_world->NextTick();
}

bool WorldServer::StartNew(std::uint64_t _seed, std::uint64_t _firstTick)
{
  std::shared_ptr<const RegionTerrain> terrain = BuildTerrain(_seed);
  if (!terrain)
  {
    return false;
  }
  // A new world draws its simulation key from the system's cryptographic generator (ADR-082 item 1), and holds the
  // seed that passed (MVP-011), which gives the same terrain at its first attempt. Its journal starts with it.
  CommandJournal journal;
  journal.start = WorldStart{Neuron::DrawSecretKey(), terrain->seed,           _firstTick,         m_desc.regionCount,   m_desc.sandbox,
                             m_desc.watched,          m_desc.syntheticFigures, m_desc.stewardSite, m_desc.aiSettlements, m_desc.pace};
  std::unique_ptr<WorldSimulation> world = StartWorld(journal.start, terrain, m_desc.tuning, &m_desc.structures);
  Adopt(std::move(world), std::move(terrain));
  m_journal = std::move(journal);
  return true;
}

bool WorldServer::RestartSandbox(bool _newSeed)
{
  const std::uint64_t seed = _newSeed ? DrawWorldSeed() : m_world->WorldSeed();
  const std::uint64_t tick = m_world->NextTick();
  // The refusals the replaced world's queues made on arrival go to their senders; a command they accepted for the next
  // tick goes with that world, without an outcome.
  CollectAllReports();
  if (!StartNew(seed, tick))
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the sandbox cannot restart from seed {}", seed);
    return false;
  }
  // Each steward is connected to the new world, in connection order, and receives its terrain.
  for (auto& [connection, steward] : m_stewards)
  {
    steward.id = ConnectSteward(*m_world, m_desc.sandbox, m_desc.watched);
    steward.terrainSent = false;
    steward.state.Forget(); // the new world's ids start again (ADR-111)
    if (m_journal)
    {
      m_journal->entries.push_back({tick, true, JOURNAL_WORLD_OWNER, steward.id, 0, {}});
    }
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the sandbox restarts before tick {} from seed {}", tick,
              m_world->WorldSeed());
  return true;
}

std::shared_ptr<const RegionTerrain> WorldServer::BuildTerrain(std::uint64_t _seed) const
{
  ENGINE_PROFILE_SCOPE("world.terrain.build");
  auto built = GenerateRegionTerrain(_seed, m_desc.tuning);
  if (!built)
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "no terrain from seed {}: seed {} failed {} check(s) last", _seed,
                built.error().seed, built.error().checks.size());
    return nullptr;
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the terrain of seed {} passed at attempt {}, digest {}",
              built->seed, built->attempt, Neuron::ShortHex(built->digest));
  return std::make_shared<const RegionTerrain>(std::move(*built));
}

void WorldServer::Adopt(std::unique_ptr<WorldSimulation> _world, std::shared_ptr<const RegionTerrain> _terrain)
{
  // The MVP's generator builds the one region of MVP-010; a world of more regions, which only tests make, leaves the
  // others without terrain.
  _world->Region(0).AttachTerrain(_terrain);
  m_world = std::move(_world);
  m_terrain = std::move(_terrain);
  m_runQuiet = false;
  m_runEnded = false;
}

std::filesystem::path WorldServer::JournalPath(const std::filesystem::path& _folder, std::uint64_t _tick)
{
  return _folder / std::format(L"world-{:020}.journal", _tick);
}

std::uint64_t WorldServer::SnapshotTicks() const noexcept
{
  const double ticks = m_desc.tuning.mvp.persist.snapshotSeconds / m_desc.tuning.ref.sim.tickPeriod;
  return ticks >= 1.0 ? static_cast<std::uint64_t>(std::llround(ticks)) : 0;
}

std::optional<std::int64_t> WorldServer::CalendarSeconds() const
{
  if (!m_desc.calendar)
  {
    return std::nullopt;
  }
  return std::chrono::duration_cast<std::chrono::seconds>(m_desc.calendar().time_since_epoch()).count();
}

CommandJournal WorldServer::RestoredJournal() const
{
  CommandJournal journal;
  journal.start =
    WorldStart{m_world->SimulationKey(), m_world->WorldSeed(), m_world->NextTick(),     static_cast<std::uint16_t>(m_world->RegionCount()),
               m_desc.sandbox,           m_desc.watched,       m_desc.syntheticFigures, m_desc.stewardSite,
               m_desc.aiSettlements,     m_world->Pace()};
  journal.restored = true;
  return journal;
}

bool WorldServer::SaveCut()
{
  ENGINE_PROFILE_SCOPE("world.snapshot.save");
  if (const std::optional<std::int64_t> now = CalendarSeconds())
  {
    m_world->Record().savedAtSeconds = *now;
  }
  Neuron::SnapshotStore store(m_desc.snapshotFolder, SnapshotsKept());
  const auto saved = store.Save(SnapshotOf(*m_world));
  if (!saved)
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the world could not be saved in {}",
                m_desc.snapshotFolder.string());
    return false;
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Debug, "the world is saved in {}", saved->filename().string());
  // MVP-076: the journal since the last cut goes beside the snapshot or the start it replays from, and a new one starts
  // from this snapshot.
  std::optional<std::uint64_t> written;
  if (m_journal)
  {
    written = m_journal->start.firstTick;
    if (!Neuron::ReplaceFileAtomically(JournalPath(m_desc.snapshotFolder, m_journal->start.firstTick), WriteJournal(*m_journal)))
    {
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the command journal from tick {} could not be written",
                  m_journal->start.firstTick);
    }
  }
  m_journal = RestoredJournal();
  // The journals that start before the oldest snapshot kept go with the snapshots, but the one just written, which may
  // start from the world's start.
  const std::vector<std::filesystem::path> kept = store.Snapshots();
  const std::optional<std::uint64_t> oldest = kept.empty() ? std::nullopt : TickOfFile(kept.back());
  for (const std::filesystem::path& journal : Neuron::ListFiles(m_desc.snapshotFolder, ".journal"))
  {
    const std::optional<std::uint64_t> tick = TickOfFile(journal);
    if (tick && oldest && *tick < *oldest && tick != written)
    {
      if (!Neuron::RemoveFile(journal))
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "the journal {} could not be removed",
                    journal.filename().string());
      }
    }
  }
  return true;
}

std::size_t WorldServer::SnapshotsKept() const noexcept
{
  // mvp.persist.snapshots_kept; a value below 1 keeps the one snapshot a restart needs.
  return static_cast<std::size_t>(std::max<std::int64_t>(1, m_desc.tuning.mvp.persist.snapshotsKept));
}

void WorldServer::Connect(Neuron::ConnectionId _connection)
{
  // ADR-109: the next id of the world owner and, in the sandbox, the steward's side.
  const StableId steward = ConnectSteward(*m_world, m_desc.sandbox, m_desc.watched);
  // MVP-072 (ADR-181): connecting is the steward's input.
  m_stewards.emplace(_connection, Steward{steward, 0, false, {}, {}, {}, {}, {}, {}, m_world->NextTick()});
  if (m_journal)
  {
    m_journal->entries.push_back({m_world->NextTick(), true, JOURNAL_WORLD_OWNER, steward, 0, {}});
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "connection {} is steward {:#x}", _connection, steward);
}

void WorldServer::Disconnect(Neuron::ConnectionId _connection)
{
  m_stewards.erase(_connection);
}

void WorldServer::Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message)
{
  const auto found = m_stewards.find(_connection);
  if (found == m_stewards.end() || _message.channel != Neuron::Channel::Reliable)
  {
    return; // the control states of the state channel arrive with M1's input (ADR-087 item 5)
  }
  // A command enters the queue of the owner it addresses (ADR-087 item 1): a battle command the queue of the MVP's one
  // region (MVP-001, ADR-106), any other the world owner's, which refuses a type it does not know with its counter.
  Neuron::BitReader reader(_message.bytes);
  const Neuron::MessageHeader header = Neuron::ReadMessageHeader(reader);
  Steward& steward = found->second;
  const std::uint64_t sequence = steward.commandsReceived++;
#if ENGINE_DEVELOPMENT
  // ADR-109: the sandbox's restart replaces the world at once, between ticks, rather than entering a queue; what arrives
  // after it enters the new world.
  if (!reader.Failed() && header.type == SandboxRestartCommand::TYPE && m_desc.sandbox)
  {
    SandboxRestartCommand restart;
    CommandResult result = CheckAnswered(header, reader, _message.bytes.size(), restart);
    if (result == CommandResult::Applied)
    {
      result = RestartSandbox(restart.newSeed) ? CommandResult::Applied : CommandResult::NoEffect;
    }
    steward.reports.push_back({sequence, m_world->NextTick(), result, 0});
    return;
  }
#endif
  // MVP-072 (ADR-181): a presence report is read, not queued: it notes the steward's input for the next tick.
  if (!reader.Failed() && header.type == PresenceReportCommand::TYPE)
  {
    PresenceReportCommand report;
    const CommandResult result = CheckAnswered(header, reader, _message.bytes.size(), report);
    if (result == CommandResult::Applied)
    {
      steward.inputTick = m_world->NextTick();
    }
    steward.reports.push_back({sequence, m_world->NextTick(), result, 0});
    return;
  }
  // MVP-098 (ADR-136, ADR-165): a request for a settlement's status or its valley's is read, not queued, by a steward who
  // commands the settlement's team; the status goes after the next tick, which the outcome names.
  // From M3 task T14 (ADR-169) the epilogue is asked for the same way.
  if (!reader.Failed() && (header.type == ReadSettlementStatusCommand::TYPE || header.type == ReadValleyStatusCommand::TYPE ||
                           header.type == ReadEpilogueCommand::TYPE))
  {
    const bool valley = header.type == ReadValleyStatusCommand::TYPE;
    const bool epilogue = header.type == ReadEpilogueCommand::TYPE;
    std::uint8_t team = 0;
    CommandResult result = CommandResult::Applied;
    if (valley)
    {
      ReadValleyStatusCommand request;
      result = CheckAnswered(header, reader, _message.bytes.size(), request);
      team = request.team;
    }
    else if (epilogue)
    {
      ReadEpilogueCommand request;
      result = CheckAnswered(header, reader, _message.bytes.size(), request);
      team = request.team;
    }
    else
    {
      ReadSettlementStatusCommand request;
      result = CheckAnswered(header, reader, _message.bytes.size(), request);
      team = request.team;
    }
    const RegionSimulation& region = m_world->Region(0);
    if (result == CommandResult::Applied && !region.MayCommand(steward.id, team, false))
    {
      result = CommandResult::NotPermitted;
    }
    else if (result == CommandResult::Applied &&
             std::ranges::find(region.Settlements(), team, &Settlement::team) == region.Settlements().end())
    {
      result = CommandResult::NoEffect;
    }
    if (result == CommandResult::Applied)
    {
      (epilogue ? steward.epilogueTeams : valley ? steward.valleyTeams : steward.statusTeams).push_back(team);
    }
    steward.reports.push_back({sequence, m_world->NextTick(), result, 0});
    return;
  }
  // MVP-063 (ADR-163): a world whose run has ended is read-only, and takes no command into a queue.
  if (m_world->RunState().ended)
  {
    steward.reports.push_back({sequence, m_world->NextTick(), CommandResult::NotPermitted, 0});
    return;
  }
  Neuron::CommandQueue& region = m_world->Region(0).Commands();
  const bool regional = !reader.Failed() && region.Handles(header.type);
  Neuron::CommandQueue& queue = regional ? region : m_world->Commands();
  if (queue.Submit(steward.id, sequence, _message.bytes))
  {
    steward.inputTick = m_world->NextTick(); // MVP-072: a command the queue took is the steward's input (ADR-181)
    if (m_journal)
    {
      // MVP-076: the command with the tick it executes in, which is the world's next.
      m_journal->entries.push_back({m_world->NextTick(), false, regional ? std::uint16_t{1} : JOURNAL_WORLD_OWNER, steward.id, sequence,
                                    std::vector<std::byte>(_message.bytes.begin(), _message.bytes.end())});
    }
  }
}

void WorldServer::UpdatePresence(std::uint64_t _tick)
{
  // MVP-072: a world at the session pace has no away state, and a watched world's settlement is the scripted steward's
  // (ADR-140), which no deputy relieves.
  if (m_world->Pace() != WorldPace::AlwaysOn || m_desc.sandbox || m_desc.watched)
  {
    return;
  }
  const auto idleTicks = static_cast<std::uint64_t>(
    std::max<std::int64_t>(0, std::llround(m_desc.tuning.mvp.presence.idleMinutes * 60.0 / m_desc.tuning.ref.sim.tickPeriod)));
  const bool present =
    std::ranges::any_of(m_stewards, [_tick, idleTicks](const auto& _entry) { return _tick < _entry.second.inputTick + idleTicks; });
  if (!SetStewardPresence(*m_world, !present))
  {
    return;
  }
  if (m_journal)
  {
    m_journal->entries.push_back({_tick, false, JOURNAL_WORLD_OWNER, 0, 0, {}, true, !present});
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the steward is {} from tick {}", present ? "present" : "away",
              _tick);
}

void WorldServer::CollectAllReports()
{
  CollectReports(m_world->Commands());
  for (std::size_t region = 0; region < m_world->RegionCount(); ++region)
  {
    CollectReports(m_world->Region(region).Commands());
  }
}

void WorldServer::CollectReports(Neuron::CommandQueue& _queue)
{
  for (const Neuron::CommandOutcome& outcome : _queue.TakeOutcomes())
  {
    for (auto& [connection, steward] : m_stewards)
    {
      if (steward.id == outcome.sender)
      {
        steward.reports.push_back({outcome.sequence, outcome.tick, ResultOf(outcome), outcome.reason});
      }
    }
  }
}

void WorldServer::DatagramFates(Neuron::ConnectionId _connection, std::span<const Neuron::DatagramOutcome> _outcomes)
{
  // ADR-086 item 3: an acknowledged state datagram makes its records the client's baselines.
  if (const auto found = m_stewards.find(_connection); found != m_stewards.end())
  {
    found->second.state.Fates(_outcomes, m_world->NextTick());
  }
}

void WorldServer::Step(std::uint64_t _tick, Neuron::WorkerPool& _workers)
{
#if ENGINE_DEVELOPMENT
  // ADR-120: the tick's wall time starts here, for the overlay; a measurement the simulated state never reads.
  m_statsTickStart = Neuron::MarkerClockNanoseconds();
#endif
  UpdatePresence(_tick);
  m_world->Step(_tick, _workers);
  // MVP-075: a snapshot after every mvp.persist.snapshot_seconds of world time.
  if (const std::uint64_t period = SnapshotTicks(); !m_desc.snapshotFolder.empty() && period > 0 && m_world->NextTick() % period == 0)
  {
    static_cast<void>(SaveCut());
  }
#if ENGINE_DEVELOPMENT
  // Debug and test runs hash every tick (ADR-081 item 8); a Release server hashes at its saves.
  WorldHashes hashes = HashWorld(*m_world);
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Trace, "tick {} hash {:016x}", _tick, hashes.world);
  m_lastHashes = std::move(hashes);
#endif
}

void WorldServer::Publish(std::uint64_t _tick, Neuron::ConnectionSender& _sender)
{
  CollectAllReports();
  // ADR-111: the MVP's one region, which ViewForSteward narrows to each steward's seen layers (MVP-097, ADR-158).
  const RegionSimulation& region = m_world->Region(0);
  const std::vector<ReplicatedObject> objects = CollectReplicatedObjects(region);
  // MVP-060 and MVP-063 (ADR-165): the run's quiet minutes and its end, each told once to every steward as it begins.
  const bool ended = m_world->RunState().ended;
  const bool quiet = !ended && region.InQuietMinutes();
  std::vector<NoticeKind> runNotices;
  if (quiet && !m_runQuiet)
  {
    runNotices.push_back(NoticeKind::RunQuiet);
  }
  if (ended && !m_runEnded)
  {
    runNotices.push_back(NoticeKind::RunEnded);
  }
  m_runQuiet = m_runQuiet || quiet;
  m_runEnded = ended;
  for (auto& [connection, steward] : m_stewards)
  {
    // ADR-095: each outcome on the reliable channel, in order; one the backlog refuses waits with those after it.
    std::size_t sent = 0;
    while (sent < steward.reports.size() &&
           _sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(steward.reports[sent])).has_value())
    {
      ++sent;
    }
    steward.reports.erase(steward.reports.begin(), steward.reports.begin() + static_cast<std::ptrdiff_t>(sent));
    // ADR-084 item 5, MVP-102 (ADR-139): the tick's guaranteed events of the teams it commands, after the outcomes and
    // in the order the region raised them; one the backlog refuses waits with those after it.
    for (const EffectEvent& event : region.Events())
    {
      if (event.kind == EffectEventKind::StructureCompleted && region.CommanderOf(event.team).sender == steward.id)
      {
        steward.notices.push_back(Neuron::EncodeMessage(NoticeOf(region, event, _tick)));
      }
    }
    // ADR-165: the valley's events of the settlements it commands, each battle's result to every steward after its end,
    // and the run's.
    for (const ValleyEvent& event : region.ValleyEvents())
    {
      if (const std::optional<SettlementNotice> notice = NoticeOf(region, event, steward.id, _tick))
      {
        steward.notices.push_back(Neuron::EncodeMessage(*notice));
      }
      if (event.kind == NoticeKind::BattleEnded)
      {
        if (const std::optional<BattleResultNotice> result = ResultFor(region, event, steward.id))
        {
          steward.notices.push_back(Neuron::EncodeMessage(*result));
        }
      }
    }
    for (const NoticeKind kind : runNotices)
    {
      const auto commanded = std::ranges::find_if(region.Settlements(), [&](const Settlement& _settlement)
                                                  { return region.CommanderOf(_settlement.team).sender == steward.id; });
      SettlementNotice notice;
      notice.tick = _tick;
      notice.kind = kind;
      notice.team = commanded != region.Settlements().end() ? commanded->team : std::uint8_t{0};
      notice.ours = true;
      steward.notices.push_back(Neuron::EncodeMessage(notice));
    }
    std::size_t noticed = 0;
    while (noticed < steward.notices.size() && _sender.Send(connection, Neuron::Channel::Reliable, steward.notices[noticed]).has_value())
    {
      ++noticed;
    }
    steward.notices.erase(steward.notices.begin(), steward.notices.begin() + static_cast<std::ptrdiff_t>(noticed));
    // MVP-098: each status requested, after the reports; one the backlog refuses goes after a later tick.
    std::size_t answered = 0;
    while (answered < steward.statusTeams.size())
    {
      const auto settlement = std::ranges::find(region.Settlements(), steward.statusTeams[answered], &Settlement::team);
      if (settlement != region.Settlements().end() &&
          !_sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(StatusOf(region, *settlement, _tick))).has_value())
      {
        break;
      }
      ++answered;
    }
    steward.statusTeams.erase(steward.statusTeams.begin(), steward.statusTeams.begin() + static_cast<std::ptrdiff_t>(answered));
    // ADR-165: each valley status requested, after the settlement's.
    std::size_t valleys = 0;
    while (valleys < steward.valleyTeams.size())
    {
      const auto settlement = std::ranges::find(region.Settlements(), steward.valleyTeams[valleys], &Settlement::team);
      if (settlement != region.Settlements().end() &&
          !_sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(ValleyStatusOf(region, *settlement, _tick, ended)))
             .has_value())
      {
        break;
      }
      ++valleys;
    }
    steward.valleyTeams.erase(steward.valleyTeams.begin(), steward.valleyTeams.begin() + static_cast<std::ptrdiff_t>(valleys));
    // ADR-169: each epilogue requested, after the valley's.
    std::size_t epilogues = 0;
    while (epilogues < steward.epilogueTeams.size())
    {
      const auto settlement = std::ranges::find(region.Settlements(), steward.epilogueTeams[epilogues], &Settlement::team);
      if (settlement != region.Settlements().end() &&
          !_sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(EpilogueOf(region, *settlement, _tick, ended)))
             .has_value())
      {
        break;
      }
      ++epilogues;
    }
    steward.epilogueTeams.erase(steward.epilogueTeams.begin(), steward.epilogueTeams.begin() + static_cast<std::ptrdiff_t>(epilogues));
    if (!steward.terrainSent && m_terrain)
    {
      // MVP-011: the seed and digest go once on the reliable channel, with the team the steward commands (ADR-119); a
      // send the backlog refuses is tried next tick.
      const WorldTerrain terrain{m_terrain->seed, m_terrain->digest, region.TeamCommandedBy(steward.id)};
      steward.terrainSent = _sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(terrain)).has_value();
    }
    // ADR-086 item 3: the tick's state datagrams against what the client has acknowledged; one the transport refuses
    // is lost, and its records go again next tick.
    for (StateSender::Outgoing& outgoing : steward.state.Build(_tick, ViewForSteward(objects, region, steward.id)))
    {
      if (const std::optional<std::uint64_t> datagram =
            _sender.Send(connection, Neuron::Channel::State, Neuron::EncodeMessage(outgoing.update)))
      {
        steward.state.Sent(*datagram, std::move(outgoing));
      }
    }
    // ADR-084 item 5: the tick's droppable events, a team's selections and orders to its commander only (ADR-106), and
    // another team's only where the steward's settlements see (ADR-158).
    for (const PresentationEvents& message : PackPresentationEvents(_tick, EventsForSteward(region.Events(), region, steward.id)))
    {
      static_cast<void>(_sender.Send(connection, Neuron::Channel::State, Neuron::EncodeMessage(message)));
    }
  }
#if ENGINE_DEVELOPMENT
  // ADR-120: the tick's wall time from its step to the end of its replication, and once a second of ticks their
  // statistics to every steward.
  if (m_statsTickStart != 0)
  {
    const std::int64_t tickNanoseconds = Neuron::MarkerClockNanoseconds() - std::exchange(m_statsTickStart, 0);
    ++m_statsTicks;
    m_statsTickNanoseconds += tickNanoseconds;
    m_statsTickMaxNanoseconds = std::max(m_statsTickMaxNanoseconds, tickNanoseconds);
  }
  const auto ticksPerSecond = static_cast<std::uint32_t>(std::max(1.0, std::round(1.0 / m_desc.tuning.ref.sim.tickPeriod)));
  if (m_statsTicks >= ticksPerSecond)
  {
    ServerStats stats;
    stats.worldSeed = m_world->WorldSeed();
    stats.tick = _tick;
    stats.ticks = m_statsTicks;
    stats.tickMeanMicroseconds = static_cast<std::uint32_t>(m_statsTickNanoseconds / m_statsTicks / 1'000);
    stats.tickMaxMicroseconds = static_cast<std::uint32_t>(m_statsTickMaxNanoseconds / 1'000);
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(stats);
    for (const auto& [connection, steward] : m_stewards)
    {
      static_cast<void>(_sender.Send(connection, Neuron::Channel::Reliable, bytes));
    }
    m_statsTicks = 0;
    m_statsTickNanoseconds = 0;
    m_statsTickMaxNanoseconds = 0;
  }
#endif
}

void WorldServer::Stalled(const Neuron::ScheduleStall& _stall)
{
  if (_stall.freeze)
  {
    // PIL-131 (ADR-137): the freeze in the world record, with the wall time the loop gave up.
    const auto seconds = static_cast<std::uint64_t>(std::max<std::int64_t>(0, _stall.lost.count() / 1'000'000'000));
    if (m_world != nullptr)
    {
      m_world->Record().freezes.push_back({_stall.tick, FreezeCause::ServerStall, seconds});
    }
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "freeze of {} s before tick {}, cause server stall", seconds,
                _stall.tick);
  }
}

void WorldServer::Freeze(std::uint64_t _nextTick)
{
  ENGINE_ASSERT(m_world == nullptr || m_world->NextTick() == _nextTick);
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world freezes before tick {}", _nextTick);
  if (m_world == nullptr || m_desc.snapshotFolder.empty())
  {
    return;
  }
  // Freezing writes a final snapshot and loses nothing (ADR-083 item 9; MVP-075's snapshot stands in for the cut), cuts
  // the journal there, and leaves the marker of a clean stop that names it (ADR-137).
  if (!SaveCut())
  {
    return;
  }
  const std::string tick = std::to_string(m_world->NextTick());
  if (!Neuron::ReplaceFileAtomically(m_desc.snapshotFolder / CLEAN_STOP_FILE_NAME, std::as_bytes(std::span(tick))))
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the marker of a clean stop could not be written");
    return;
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world is saved in {} at tick {}",
              m_desc.snapshotFolder.string(), m_world->NextTick());
}

} // namespace Species
