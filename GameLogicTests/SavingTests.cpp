#include "pch.h"

#include "CommandJournal.h"
#include "Files.h"
#include "MessageCodec.h"
#include "SettlementMessages.h"
#include "Snapshot.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_SITE = 2;
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;
constexpr std::uint64_t SAVE_TICKS = 50; // the tests' snapshot period, 5 s at the committed tick period

// The committed tuning, saving every 5 s of world time and keeping three snapshots.
Species::TuningMvp SavingTuning()
{
  Species::TuningMvp tuning = CommittedTuning();
  tuning.mvp.persist.snapshotSeconds = 5.0;
  tuning.mvp.persist.snapshotsKept = 3;
  return tuning;
}

// A world server of the valley with the steward's settlement, saving into _folder (none: never) on a calendar that
// reads *_calendarSeconds when one is given.
Species::WorldServer::Desc SettledDesc(const std::filesystem::path& _folder, const std::int64_t* _calendarSeconds = nullptr)
{
  Species::WorldServer::Desc desc;
  desc.tuning = SavingTuning();
  desc.structures = TestRecords();
  desc.worldSeed = ValleyTerrain()->seed;
  desc.stewardSite = SETTLED_SITE;
  desc.snapshotFolder = _folder;
  if (_calendarSeconds != nullptr)
  {
    desc.calendar = [_calendarSeconds] { return std::chrono::system_clock::time_point(std::chrono::seconds(*_calendarSeconds)); };
  }
  return desc;
}

template <class T> Neuron::ReceivedMessage CommandFrom(const T& _command)
{
  return Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(_command)};
}

// The steward's commands of the tests' runs, before the tick they name: new shares at tick 10, a dwelling placed at
// tick 60 and new shares again at tick 120.
void SendCommands(Species::WorldServer& _server, std::uint64_t _tick)
{
  if (_tick == 10)
  {
    _server.Receive(STEWARD_CONNECTION, CommandFrom(Species::SetPrioritiesCommand{SETTLED_SITE, {40, 40, 20}}));
  }
  if (_tick == 60 || _tick == 120)
  {
    const Neuron::Vec3 site = _server.Terrain()->layout.sites[SETTLED_SITE];
    _server.Receive(STEWARD_CONNECTION,
                    CommandFrom(Species::PlaceStructureCommand{
                      SETTLED_SITE, Species::StructureKind::Dwelling, {site.x + 150.0, site.z + (_tick == 60 ? 0.0 : 90.0)}}));
    _server.Receive(STEWARD_CONNECTION, CommandFrom(Species::SetPrioritiesCommand{SETTLED_SITE, {30, 50, 20}}));
  }
}

// The server stepped from its next tick to _end with the tests' commands, and the hash after each tick.
std::vector<std::uint64_t> Run(Species::WorldServer& _server, std::uint64_t _from, std::uint64_t _end, Neuron::WorkerPool& _pool)
{
  std::vector<std::uint64_t> hashes;
  for (std::uint64_t tick = _from; tick < _end; ++tick)
  {
    SendCommands(_server, tick);
    _server.Step(tick, _pool);
    hashes.push_back(Species::HashWorld(*_server.Simulation()).world);
  }
  return hashes;
}

std::vector<std::uint64_t> TicksOf(const std::vector<std::filesystem::path>& _files)
{
  std::vector<std::uint64_t> ticks;
  for (const std::filesystem::path& file : _files)
  {
    const std::wstring stem = file.stem().wstring();
    ticks.push_back(std::stoull(stem.substr(stem.find(L'-') + 1)));
  }
  std::ranges::sort(ticks);
  return ticks;
}

} // namespace

TEST_CLASS(SavingTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp076ASnapshotAndTheJournalBesideItReplayTheWorld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp076ASnapshotAndTheJournalBesideItReplayTheWorld)
  {
    // A snapshot every period after the tick, the newest three kept; the journal cut at each, written beside the
    // snapshot it starts from, and removed with it. A snapshot and its journal replay the world's hashes to the next.
    const TemporaryFolder folder;
    Species::WorldServer server(SettledDesc(folder.Path()));
    ExpectEqual(0, static_cast<std::int64_t>(server.Open()), L"a new world");
    server.Connect(STEWARD_CONNECTION);
    Neuron::WorkerPool pool(0);
    const std::vector<std::uint64_t> hashes = Run(server, 0, 4 * SAVE_TICKS, pool);
    const Neuron::SnapshotStore store(folder.Path(), 3);
    Assert::IsTrue(TicksOf(store.Snapshots()) == std::vector<std::uint64_t>{100, 150, 200}, L"the newest three snapshots");
    Assert::IsTrue(TicksOf(Neuron::ListFiles(folder.Path(), ".journal")) == std::vector<std::uint64_t>{100, 150},
                   L"the journals from the kept snapshots but the newest");
    const Species::CommandJournal current = server.Journal().value_or(Species::CommandJournal{});
    Assert::IsTrue(current.restored && current.start.firstTick == 200 && current.entries.empty(), L"a new journal from tick 200");

    const auto snapshot = Neuron::SnapshotStore::Load(folder.Path() / L"world-00000000000000000100.snapshot");
    const auto bytes = Neuron::ReadWholeFile(Species::WorldServer::JournalPath(folder.Path(), 100));
    Assert::IsTrue(snapshot.has_value() && bytes.has_value(), L"the snapshot of tick 100 and its journal");
    const std::optional<Species::CommandJournal> journal = Species::ReadJournal(bytes.value_or(std::vector<std::byte>{}));
    Assert::IsTrue(journal.has_value() && journal->restored && journal->start.firstTick == 100, L"a journal from that snapshot");
    ExpectEqual(2, static_cast<std::int64_t>(journal.value_or(Species::CommandJournal{}).entries.size()), L"the two commands of tick 120");
    auto restored = Species::RestoreWorld(snapshot.value_or(Neuron::SnapshotContents{}), server.Tuning(), &server.Structures());
    Assert::IsTrue(restored.has_value(), L"the snapshot restores");
    std::unique_ptr<Species::WorldSimulation> world = std::move(restored.value());
    world->Region(0).AttachTerrain(ValleyTerrain());
    std::vector<std::uint64_t> replayed;
    static_cast<void>(Species::ReplayJournalOnto(std::move(world), journal.value_or(Species::CommandJournal{}), 150, pool,
                                                 [&replayed](const Species::WorldSimulation& _world)
                                                 { replayed.push_back(Species::HashWorld(_world).world); }));
    Assert::IsTrue(std::ranges::equal(replayed, std::vector<std::uint64_t>(hashes.begin() + 100, hashes.begin() + 150)),
                   L"the hashes of ticks 100 to 149");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075ARestoredWorldStepsAsOneNeverStopped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075ARestoredWorldStepsAsOneNeverStopped)
  {
    // ADR-085's confirmation: the world restored from its snapshot of tick 100, its steward connecting again, gives the
    // hashes of the same world never stopped, whose steward reconnected at that tick, at 1, 2 and 8 worker threads. The
    // world is the same one because a new world draws its own key (ADR-082 item 1).
    const TemporaryFolder folder;
    Neuron::WorkerPool serial(0);
    Species::WorldServer unstopped(SettledDesc(folder.Path() / L"saves"));
    static_cast<void>(unstopped.Open());
    unstopped.Connect(STEWARD_CONNECTION);
    static_cast<void>(Run(unstopped, 0, 100, serial));
    for (const wchar_t* copy : {L"1", L"2", L"8"})
    {
      std::filesystem::copy(folder.Path() / L"saves", folder.Path() / copy);
    }
    unstopped.Disconnect(STEWARD_CONNECTION);
    unstopped.Connect(STEWARD_CONNECTION);
    const std::vector<std::uint64_t> expected = Run(unstopped, 100, 160, serial);
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      Neuron::WorkerPool pool(workers);
      Species::WorldServer restored(SettledDesc(folder.Path() / std::to_wstring(workers)));
      ExpectEqual(100, static_cast<std::int64_t>(restored.Open()), L"restored at tick 100");
      restored.Connect(STEWARD_CONNECTION);
      Assert::IsTrue(Run(restored, 100, 160, pool) == expected, L"the hashes of the world never stopped");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil131AnUncleanStopAndAStallAreFreezesOfTheWorldRecord)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-131")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil131AnUncleanStopAndAStallAreFreezesOfTheWorldRecord)
  {
    // A stall that froze the world is recorded with its tick and the seconds the loop gave up, and saved with the world.
    // A server that stops without its final snapshot leaves no marker, so the next records a freeze of cause "unclean
    // stop" from the snapshot it opens, as long as the calendar time since that snapshot. A clean stop records none.
    const TemporaryFolder folder;
    std::int64_t now = 1'800'000'000;
    Neuron::WorkerPool pool(0);
    {
      Species::WorldServer crashed(SettledDesc(folder.Path(), &now));
      static_cast<void>(crashed.Open());
      crashed.Connect(STEWARD_CONNECTION);
      static_cast<void>(Run(crashed, 0, 60, pool));
      crashed.Stalled({60, Neuron::Nanoseconds{30'500'000'000}, true});
      crashed.Stalled({61, Neuron::Nanoseconds{3'000'000'000}, false});
      static_cast<void>(Run(crashed, 60, 110, pool)); // saved at tick 100, then lost
    }
    now += 90;
    Species::WorldServer reopened(SettledDesc(folder.Path(), &now));
    ExpectEqual(100, static_cast<std::int64_t>(reopened.Open()), L"opened from the snapshot of tick 100");
    const std::vector<Species::FreezeRecord> expected{{60, Species::FreezeCause::ServerStall, 30},
                                                      {100, Species::FreezeCause::UncleanStop, 90}};
    Assert::IsTrue(reopened.Simulation()->Record().freezes == expected, L"the stall, saved, and the unclean stop");
    Assert::IsFalse(std::filesystem::exists(folder.Path() / Species::WorldServer::CLEAN_STOP_FILE_NAME), L"no marker while it runs");
    reopened.Freeze(100);
    Assert::IsTrue(std::filesystem::exists(folder.Path() / Species::WorldServer::CLEAN_STOP_FILE_NAME), L"the marker of a clean stop");

    now += 3'600;
    Species::WorldServer clean(SettledDesc(folder.Path(), &now));
    ExpectEqual(100, static_cast<std::int64_t>(clean.Open()), L"opened again at tick 100");
    Assert::IsTrue(clean.Simulation()->Record().freezes == expected, L"no freeze for a clean stop");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075ATornNewestSnapshotFallsBackToTheOneBefore)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075ATornNewestSnapshotFallsBackToTheOneBefore)
  {
    // The final snapshot torn, the world opens from the one before; the marker names the torn one, so the ticks lost
    // between them are a freeze. A snapshot of a state version this build cannot migrate is refused like a torn one,
    // and a folder with nothing else opens a new world.
    const TemporaryFolder folder;
    Neuron::WorkerPool pool(0);
    {
      Species::WorldServer server(SettledDesc(folder.Path()));
      static_cast<void>(server.Open());
      server.Connect(STEWARD_CONNECTION);
      static_cast<void>(Run(server, 0, 130, pool));
      server.Freeze(130);
    }
    const std::filesystem::path newest = folder.Path() / L"world-00000000000000000130.snapshot";
    std::vector<std::byte> bytes = Neuron::ReadWholeFile(newest).value_or(std::vector<std::byte>{});
    bytes.resize(bytes.size() / 2);
    Assert::IsTrue(Neuron::ReplaceFileAtomically(newest, bytes).has_value(), L"the newest snapshot torn");
    Species::WorldServer reopened(SettledDesc(folder.Path()));
    ExpectEqual(100, static_cast<std::int64_t>(reopened.Open()), L"the snapshot of tick 100");
    const std::vector<Species::FreezeRecord> freezes = reopened.Simulation()->Record().freezes;
    Assert::IsTrue(freezes.size() == 1 && freezes[0].tick == 100 && freezes[0].cause == Species::FreezeCause::UncleanStop,
                   L"the ticks after it lost, as an unclean stop");

    // A later version than this build's, beside the snapshot of tick 100, and then alone.
    const auto valid = Neuron::SnapshotStore::Load(folder.Path() / L"world-00000000000000000100.snapshot");
    Assert::IsTrue(valid.has_value(), L"the snapshot of tick 100 loads");
    Neuron::SnapshotContents later = valid.value_or(Neuron::SnapshotContents{});
    later.stateVersion = Species::WORLD_STATE_VERSION + 1;
    later.tick = 500;
    Neuron::SnapshotStore store(folder.Path(), 3);
    Assert::IsTrue(store.Save(later).has_value(), L"saved");
    Species::WorldServer refusing(SettledDesc(folder.Path()));
    ExpectEqual(100, static_cast<std::int64_t>(refusing.Open()), L"the later version refused, the one before opened");
    const TemporaryFolder alone;
    Neuron::SnapshotStore aloneStore(alone.Path(), 3);
    Assert::IsTrue(aloneStore.Save(later).has_value(), L"saved alone");
    Species::WorldServer fresh(SettledDesc(alone.Path()));
    ExpectEqual(0, static_cast<std::int64_t>(fresh.Open()), L"a new world");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075AWorldThatIsOnlyContinuedTouchesNothingItCannotContinue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075AWorldThatIsOnlyContinuedTouchesNothingItCannotContinue)
  {
    // ADR-155: the front end continues a world only from a snapshot that validates. With none, the world opens new
    // without its folder: every snapshot it refused and the marker of its clean stop stay, and the freeze saves nothing.
    const TemporaryFolder folder;
    Neuron::WorkerPool pool(0);
    {
      Species::WorldServer server(SettledDesc(folder.Path()));
      static_cast<void>(server.Open());
      static_cast<void>(Run(server, 0, 60, pool));
      server.Freeze(60);
    }
    Species::WorldServer::Desc continuing = SettledDesc(folder.Path());
    continuing.restoreOnly = true;
    {
      Species::WorldServer server(continuing);
      ExpectEqual(60, static_cast<std::int64_t>(server.Open()), L"a snapshot that validates is continued");
      Assert::IsTrue(server.Opened().restored && server.Opened().refusals.empty(), L"restored, with nothing refused");
      server.Freeze(60);
    }
    // A later version than this build's, alone in its folder with the marker of its clean stop.
    const auto valid = Neuron::SnapshotStore::Load(folder.Path() / L"world-00000000000000000060.snapshot");
    Assert::IsTrue(valid.has_value(), L"the snapshot of tick 60 loads");
    Neuron::SnapshotContents later = valid.value_or(Neuron::SnapshotContents{});
    later.stateVersion = Species::WORLD_STATE_VERSION + 1;
    later.tick = 500;
    const TemporaryFolder alone;
    Neuron::SnapshotStore store(alone.Path(), 3);
    Assert::IsTrue(store.Save(later).has_value(), L"saved alone");
    const std::filesystem::path marker = alone.Path() / Species::WorldServer::CLEAN_STOP_FILE_NAME;
    const std::array<std::byte, 8> tick{};
    Assert::IsTrue(Neuron::ReplaceFileAtomically(marker, tick).has_value(), L"a marker beside it");
    continuing.snapshotFolder = alone.Path();
    {
      Species::WorldServer server(continuing);
      ExpectEqual(0, static_cast<std::int64_t>(server.Open()), L"nothing continues, so a world opens new");
      const Species::WorldServer::OpenOutcome& opened = server.Opened();
      Assert::IsTrue(!opened.restored && opened.refusals.size() == 1 && opened.refusals[0].error == Neuron::SnapshotError::StateVersion,
                     L"the later version refused, with its reason");
      static_cast<void>(Run(server, 0, 60, pool));
      server.Freeze(60);
    }
    const std::vector<std::filesystem::path> files = Neuron::ListFiles(alone.Path(), Neuron::SNAPSHOT_SUFFIX);
    Assert::IsTrue(files.size() == 1 && TicksOf(files) == std::vector<std::uint64_t>{500},
                   L"the refused snapshot stays, and none is added");
    Assert::IsTrue(std::filesystem::exists(marker), L"and so does the marker");
  }
};

} // namespace GameLogicTests
