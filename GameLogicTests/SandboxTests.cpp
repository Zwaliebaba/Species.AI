#include "pch.h"

#include "BitWriter.h"
#include "CommandJournal.h"
#include "CommandMessages.h"
#include "Development.h"
#include "EffectEvent.h"
#include "Files.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "Sandbox.h"
#include "ServerHost.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "TickDraws.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldTerrain.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey SANDBOX_TEST_KEY{0x5a5a5a5a5a5a5a5a, 0x0123456789abcdef};
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;

// A one-region world on the valley of seed 1, populated as the sandbox or as the synthetic battle.
std::unique_ptr<Species::WorldSimulation> SandboxWorld(bool _synthetic)
{
  auto world = std::make_unique<Species::WorldSimulation>(Species::WorldSimulation::Desc{SANDBOX_TEST_KEY, 0, 1, &CommittedTuning()});
  world->Region(0).AttachTerrain(ValleyTerrain());
  if (_synthetic)
  {
    Species::PopulateSyntheticBattle(world->Region(0));
  }
  else
  {
    Species::PopulateSandbox(world->Region(0));
  }
  return world;
}

// The figures of one team that are in use.
std::vector<const Species::Figure*> FiguresOf(const Species::RegionSimulation& _region, std::uint8_t _team)
{
  std::vector<const Species::Figure*> figures;
  const auto& pool = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < pool.Capacity(); ++slot)
  {
    if (pool.Used(slot))
    {
      figures.push_back(&pool.At(slot));
    }
  }
  return figures;
}

double GroundDistance(const Neuron::Vec3& _a, const Neuron::Vec3& _b)
{
  return std::hypot(_a.x - _b.x, _a.z - _b.z);
}

// What the world server sends, with the command outcomes decoded; every send is taken.
class RecordingSender final : public Neuron::ConnectionSender
{
public:
  [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId, Neuron::Channel _channel,
                                                  std::span<const std::byte> _message) override
  {
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::CommandReport>([this](const Species::CommandReport& _report) { reports.push_back(_report); });
    dispatcher.Register<Species::WorldTerrain>([this](const Species::WorldTerrain& _terrain) { terrains.push_back(_terrain); });
    static_cast<void>(dispatcher.Dispatch(_message));
    return _channel == Neuron::Channel::State ? ++datagrams : 0;
  }

  // A sender of a test closes nothing: its server is given no identity, so it refuses no join (ADR-186).
  void Close(Neuron::ConnectionId /*_connection*/) override {}

  std::vector<Species::CommandReport> reports;
  std::vector<Species::WorldTerrain> terrains;
  std::uint64_t datagrams = 0;
};

// A command from the steward, as the transport delivers it.
template <class T> Neuron::ReceivedMessage CommandFrom(const T& _command)
{
  return {Neuron::Channel::Reliable, Neuron::EncodeMessage(_command)};
}

} // namespace

TEST_CLASS(SandboxTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005TheSandboxStartsWithTwoSidesAndTheAi)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005TheSandboxStartsWithTwoSidesAndTheAi)
  {
    // M1 plan §6: mvp.sandbox.side_workers Workers a side with no Leader, each around its site's centre on diagonally
    // opposite sites, the AI commanding the second side and the steward who connects the first.
    const Species::TuningMvp& tuning = CommittedTuning();
    const Species::WorldStart start{SANDBOX_TEST_KEY, 1, 0, 1, true};
    const std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, ValleyTerrain(), tuning);
    const Species::RegionSimulation& region = world->Region(0);
    const auto& sites = ValleyTerrain()->layout.sites;
    const Neuron::Vec3& stewardSite = sites[static_cast<std::size_t>(tuning.mvp.sandbox.stewardSite)];
    const Neuron::Vec3& aiSite = sites[static_cast<std::size_t>(tuning.mvp.sandbox.aiSite)];
    for (const std::uint8_t team : {Species::SANDBOX_STEWARD_TEAM, Species::SANDBOX_AI_TEAM})
    {
      const std::vector<const Species::Figure*> figures = FiguresOf(region, team);
      ExpectEqual(tuning.mvp.sandbox.sideWorkers, static_cast<std::int64_t>(figures.size()), L"a side's Workers");
      const Neuron::Vec3& own = team == Species::SANDBOX_STEWARD_TEAM ? stewardSite : aiSite;
      const Neuron::Vec3& other = team == Species::SANDBOX_STEWARD_TEAM ? aiSite : stewardSite;
      for (const Species::Figure* figure : figures)
      {
        Assert::IsTrue(figure->kind == Species::FigureKind::Worker, L"no Leader");
        Assert::IsTrue(GroundDistance(figure->position, own) < GroundDistance(figure->position, other), L"around its own site");
      }
    }
    Assert::IsTrue(region.Ais().size() == 1 && region.Ais()[0].team == Species::SANDBOX_AI_TEAM, L"the AI commands the second side");
    const Species::StableId steward = Species::ConnectSteward(*world, true);
    Assert::IsTrue(region.MayCommand(steward, Species::SANDBOX_STEWARD_TEAM, false), L"the steward commands the first side");
    Assert::IsFalse(region.MayCommand(steward, Species::SANDBOX_AI_TEAM, false), L"and not the AI's");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005AWatchedSandboxHasAnAiOnEachSide)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005AWatchedSandboxHasAnAiOnEachSide)
  {
    // O77: --watch gives the steward's side the AI too, and the steward commands neither side.
    const Species::WorldStart start{SANDBOX_TEST_KEY, 1, 0, 1, true, true};
    const std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, ValleyTerrain(), CommittedTuning());
    const Species::RegionSimulation& region = world->Region(0);
    Assert::IsTrue(region.Ais().size() == 2, L"an AI on each side");
    Assert::IsTrue(region.Ais()[0].team != region.Ais()[1].team, L"one per side");
    const Species::StableId steward = Species::ConnectSteward(*world, true, true);
    Assert::IsFalse(region.MayCommand(steward, Species::SANDBOX_STEWARD_TEAM, false), L"the steward commands neither side");
    Assert::IsFalse(region.MayCommand(steward, Species::SANDBOX_AI_TEAM, false));
    Assert::IsFalse(region.TeamCommandedBy(steward).has_value(), L"so its terrain message names no team");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093TheSandboxAndTheSyntheticBattleHashAlikeAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093TheSandboxAndTheSyntheticBattleHashAlikeAtAnyWorkerCount)
  {
    // ADR-081's and ADR-082's confirmation at M1: the sandbox and the synthetic battle of 2,000 figures give the same
    // hash every tick with 1, 2 and 8 worker threads as serially.
    const auto run = [](bool _synthetic, std::uint64_t _ticks, std::size_t _workers)
    {
      Neuron::WorkerPool pool(_workers);
      const std::unique_ptr<Species::WorldSimulation> world = SandboxWorld(_synthetic);
      Neuron::BitWriter chain;
      for (std::uint64_t tick = 0; tick < _ticks; ++tick)
      {
        world->Step(tick, pool);
        chain.WriteBits(Species::HashWorld(*world).world, 64);
      }
      return Species::HashState(chain.Bytes());
    };
    const std::unique_ptr<Species::WorldSimulation> synthetic = SandboxWorld(true);
    ExpectEqual(CommittedTuning().mvp.perf.syntheticFigures,
                static_cast<std::int64_t>(synthetic->Region(0).TeamAt(0).figures.Count()) +
                  static_cast<std::int64_t>(synthetic->Region(0).TeamAt(1).figures.Count()),
                L"the synthetic battle's figures");
    ExpectEqual(2, static_cast<std::int64_t>(synthetic->Region(0).Ais().size()), L"an AI on each side");
    // ADR-126: a world start that names the battle's figures, as the frame bench's scene of mvp.client.figures_max does.
    const Species::WorldStart start{
      SANDBOX_TEST_KEY, 1, 0, 1, true, true, static_cast<std::uint32_t>(CommittedTuning().mvp.client.figuresMax)};
    const std::unique_ptr<Species::WorldSimulation> figuresMax = Species::StartWorld(start, ValleyTerrain(), CommittedTuning());
    ExpectEqual(CommittedTuning().mvp.client.figuresMax,
                static_cast<std::int64_t>(figuresMax->Region(0).TeamAt(0).figures.Count()) +
                  static_cast<std::int64_t>(figuresMax->Region(0).TeamAt(1).figures.Count()),
                L"the figures it names");
    ExpectEqual(2, static_cast<std::int64_t>(figuresMax->Region(0).Ais().size()), L"and an AI on each side");
    // The eight runs are independent, so they run at once.
    static_cast<void>(ValleyTerrain());
    const std::vector<std::size_t> workerCounts{0, 1, 2, 8};
    std::vector<std::uint64_t> chains(2 * workerCounts.size());
    std::vector<std::function<void()>> jobs;
    jobs.reserve(chains.size());
    for (std::size_t index = 0; index < chains.size(); ++index)
    {
      const bool isSynthetic = index >= workerCounts.size();
      jobs.emplace_back([&run, &chains, &workerCounts, index, isSynthetic]
                        { chains[index] = run(isSynthetic, isSynthetic ? 100 : 300, workerCounts[index % workerCounts.size()]); });
    }
    RunConcurrently(jobs);
    for (std::size_t index = 0; index < chains.size(); ++index)
    {
      const std::size_t serial = index < workerCounts.size() ? 0 : workerCounts.size();
      Assert::IsTrue(chains[index] == chains[serial], L"the same hashes on every tick");
    }
  }

#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ACosmeticEffectLeavesEveryHashUnchanged)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ACosmeticEffectLeavesEveryHashUnchanged)
  {
    // ADR-081's confirmation: a presentation event emitted in the middle of every tick, between figures' advances, is
    // not state, so the sandbox's hash is the same on every tick with and without it.
    const std::unique_ptr<Species::WorldSimulation> plain = SandboxWorld(false);
    const std::unique_ptr<Species::WorldSimulation> cosmetic = SandboxWorld(false);
    Species::RegionSimulation& region = cosmetic->Region(0);
    std::uint64_t advances = 0;
    region.ObserveAdvances(
      [&region, &advances](std::uint8_t _team, std::uint32_t, std::uint32_t)
      {
        if (++advances % 10 == 0)
        {
          region.Emit({Species::EffectEventKind::LaserFired, _team, 0, {}, {}, 0.0});
        }
      });
    Neuron::WorkerPool pool(2);
    for (std::uint64_t tick = 0; tick < 200; ++tick)
    {
      plain->Step(tick, pool);
      cosmetic->Step(tick, pool);
      Assert::IsTrue(cosmetic->Region(0).Events().size() > plain->Region(0).Events().size(), L"the cosmetic effect happened");
      Assert::IsTrue(Species::HashWorld(*cosmetic).world == Species::HashWorld(*plain).world, L"the same hash");
    }
  }
#endif

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ADrawAddedToOnePurposeChangesNoOtherInATick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ADrawAddedToOnePurposeChangesNoOtherInATick)
  {
    // ADR-082's confirmation through the step's own contexts: for each purpose in turn, extra draws of that purpose
    // interleaved with every other purpose's leave the others' values of the tick as they were.
    constexpr std::uint64_t PURPOSES = Species::PurposeOf(Species::DrawPurpose::AiOffset);
    constexpr Species::StableId SUBJECT = 0x0100000000000007;
    const auto valuesOf = [](std::uint64_t _added)
    {
      Species::TickDraws draws(SANDBOX_TEST_KEY);
      draws.SetTick(600);
      std::vector<std::uint64_t> values;
      for (int round = 0; round < 3; ++round)
      {
        for (std::uint64_t purpose = 1; purpose <= PURPOSES; ++purpose)
        {
          Neuron::SimRng& rng = draws.For(static_cast<Species::DrawPurpose>(purpose), SUBJECT);
          if (purpose == _added)
          {
            static_cast<void>(rng.NextValue());
            static_cast<void>(rng.NextValue());
          }
          else
          {
            values.push_back(rng.NextValue());
          }
        }
      }
      return values;
    };
    for (std::uint64_t added = 1; added <= PURPOSES; ++added)
    {
      const std::vector<std::uint64_t> with = valuesOf(added);
      std::vector<std::uint64_t> without = valuesOf(0);
      // The run without the added purpose has its values too; they are removed to compare the others.
      std::vector<std::uint64_t> others;
      for (std::size_t n = 0; n < without.size(); ++n)
      {
        if (n % PURPOSES != added - 1)
        {
          others.push_back(without[n]);
        }
      }
      Assert::IsTrue(with == others, L"every other purpose's values are unchanged");
    }
  }
};

TEST_CLASS(CommandJournalTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp076AJournalReadsBackAsItWasWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp076AJournalReadsBackAsItWasWritten)
  {
    Species::CommandJournal journal;
    journal.start = {SANDBOX_TEST_KEY, 77, 40, 1, true, true, 1'500};
    journal.entries.push_back({40, true, Species::JOURNAL_WORLD_OWNER, 9, 0, {}});
    journal.entries.push_back({40, false, 1, 9, 0, Neuron::EncodeMessage(Species::PromoteWorkerCommand{0, 12})});
    journal.entries.push_back({52, false, Species::JOURNAL_WORLD_OWNER, 9, 1, Neuron::EncodeMessage(Species::SelectCommand{0, 0})});
    const std::vector<std::byte> bytes = Species::WriteJournal(journal);
    const std::optional<Species::CommandJournal> read = Species::ReadJournal(bytes);
    Assert::IsTrue(read.has_value(), L"the journal reads");
    const Species::CommandJournal back = read.value_or(Species::CommandJournal{});
    Assert::IsTrue(back.start.key.low == SANDBOX_TEST_KEY.low && back.start.key.high == SANDBOX_TEST_KEY.high && back.start.seed == 77 &&
                     back.start.firstTick == 40 && back.start.regions == 1 && back.start.sandbox && back.start.watched &&
                     back.start.syntheticFigures == 1'500,
                   L"its start, watched (O77), with the synthetic battle's figures (ADR-126)");
    Assert::IsTrue(back.entries == journal.entries, L"its entries");
    Assert::IsFalse(back.start.stewardSite.has_value(), L"no steward's site in the sandbox");
    Species::CommandJournal settled = journal;
    settled.start.sandbox = false;
    settled.start.watched = false;
    settled.start.stewardSite = std::uint8_t{2};
    settled.restored = true;
    const std::optional<Species::CommandJournal> settledBack = Species::ReadJournal(Species::WriteJournal(settled));
    Assert::IsTrue(settledBack.has_value() && settledBack->start.stewardSite == std::optional<std::uint8_t>(std::uint8_t{2}),
                   L"a world's steward's site (MVP-012, ADR-130)");
    Assert::IsTrue(settledBack.has_value() && settledBack->restored && !back.restored, L"whether it starts from a snapshot (ADR-137)");

    Assert::IsFalse(Species::ReadJournal(std::span(bytes).first(bytes.size() - 1)).has_value(), L"a short file does not read");
    std::vector<std::byte> otherFormat = bytes;
    otherFormat[0] ^= std::byte{1};
    Assert::IsFalse(Species::ReadJournal(otherFormat).has_value(), L"nor another format");
    Species::CommandJournal backwards = journal;
    backwards.entries[2].tick = 39;
    Assert::IsFalse(Species::ReadJournal(Species::WriteJournal(backwards)).has_value(), L"nor ticks that fall");
    Species::CommandJournal unknownOwner = journal;
    unknownOwner.entries[1].owner = 2;
    Assert::IsFalse(Species::ReadJournal(Species::WriteJournal(unknownOwner)).has_value(), L"nor an owner the world lacks");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp076TheJournalReplaysTheSandboxTickByTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp076TheJournalReplaysTheSandboxTickByTick)
  {
    // The world server runs the sandbox with a steward's commands; its journal, written and read back, replays the same
    // hash on every tick on the same build. A command the queue refuses is not in it.
    Species::WorldServer server({.tuning = CommittedTuning(), .worldSeed = 1, .sandbox = true});
    const std::uint64_t first = server.Open();
    server.Connect(STEWARD_CONNECTION);
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    const Species::StableId worker = FiguresOf(region, Species::SANDBOX_STEWARD_TEAM).front()->id;
    const Neuron::Vec3& aiSite = server.Terrain()->layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.aiSite)];
    const Neuron::Vec3& stewardSite = server.Terrain()->layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
    Neuron::WorkerPool pool(2);
    RecordingSender sender;
    std::vector<std::uint64_t> hashes;
    for (std::uint64_t tick = first; tick < first + 300; ++tick)
    {
      switch (tick - first)
      {
      case 5:
        server.Receive(STEWARD_CONNECTION, CommandFrom(Species::PromoteWorkerCommand{Species::SANDBOX_STEWARD_TEAM, worker}));
        server.Receive(STEWARD_CONNECTION, CommandFrom(Species::PromoteWorkerCommand{Species::SANDBOX_AI_TEAM, worker}));
        break;
      case 6:
        server.Receive(STEWARD_CONNECTION,
                       CommandFrom(Species::OrderUnitCommand{Species::SANDBOX_STEWARD_TEAM, false, false, {aiSite.x, aiSite.z}}));
        break;
      case 20:
        server.Receive(STEWARD_CONNECTION,
                       CommandFrom(Species::SelectWorkersCommand{Species::SANDBOX_STEWARD_TEAM, {stewardSite.x, stewardSite.z}, 150.0}));
        server.Receive(STEWARD_CONNECTION,
                       CommandFrom(Species::OrderWorkersCommand{Species::SANDBOX_STEWARD_TEAM, true, {aiSite.x, aiSite.z}}));
        break;
      default:
        break;
      }
      server.Step(tick, pool);
      server.Publish(tick, sender);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
    }
    // A refusal on arrival is made before the tick executes the commands accepted with it, so it is reported first.
    ExpectEqual(5, static_cast<std::int64_t>(sender.reports.size()), L"an outcome for each command");
    Assert::IsTrue(sender.reports[0].sequence == 1 && sender.reports[0].result == Species::CommandResult::NotPermitted,
                   L"the AI's side is not the steward's");
    Assert::IsTrue(sender.reports[1].sequence == 0 && sender.reports[1].result == Species::CommandResult::Applied,
                   L"the promotion applied");

    Assert::IsTrue(server.Journal().has_value(), L"a new world has its journal");
    const Species::CommandJournal journal = server.Journal().value_or(Species::CommandJournal{});
    ExpectEqual(5, static_cast<std::int64_t>(journal.entries.size()), L"the connection and the four accepted commands");
    Assert::IsTrue(journal.entries[0].connection && journal.entries[0].tick == first, L"the connection first");
    const std::optional<Species::CommandJournal> read = Species::ReadJournal(Species::WriteJournal(journal));
    Assert::IsTrue(read.has_value(), L"the journal reads back");
    const Species::CommandJournal back = read.value_or(Species::CommandJournal{});
    const auto terrain =
      std::make_shared<const Species::RegionTerrain>(*Species::GenerateRegionTerrain(back.start.seed, CommittedTuning()));
    std::vector<std::uint64_t> replayed;
    const std::unique_ptr<Species::WorldSimulation> replay =
      Species::ReplayJournal(back, terrain, CommittedTuning(), nullptr, first + 300, pool,
                             [&replayed](const Species::WorldSimulation& _world) { replayed.push_back(Species::HashWorld(_world).world); });
    Assert::IsTrue(replayed == hashes, L"the same hash on every tick");
    Assert::IsTrue(replay->NextTick() == server.Simulation()->NextTick(), L"to the same tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp076AFreezingWorldWritesItsJournalBesideItsSnapshots)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp076AFreezingWorldWritesItsJournalBesideItsSnapshots)
  {
    // The journal of a world opened new is written when it freezes, under the tick it starts from, beside the
    // snapshots, and reads back whole; a world restored from the final snapshot journals from it (ADR-109, ADR-137).
    const TemporaryFolder folder;
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.worldSeed = 1;
    desc.snapshotFolder = folder.Path() / L"saves";
    desc.sandbox = true;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    server.Connect(STEWARD_CONNECTION);
    const Species::StableId worker = FiguresOf(server.Simulation()->Region(0), Species::SANDBOX_STEWARD_TEAM).front()->id;
    Neuron::WorkerPool pool(0);
    RecordingSender sender;
    for (std::uint64_t tick = first; tick < first + 10; ++tick)
    {
      if (tick == first + 3)
      {
        server.Receive(STEWARD_CONNECTION, CommandFrom(Species::PromoteWorkerCommand{Species::SANDBOX_STEWARD_TEAM, worker}));
      }
      server.Step(tick, pool);
      server.Publish(tick, sender);
    }
    const Species::CommandJournal written = server.Journal().value_or(Species::CommandJournal{});
    server.Freeze(first + 10);
    const auto bytes = Neuron::ReadWholeFile(Species::WorldServer::JournalPath(desc.snapshotFolder, first));
    Assert::IsTrue(bytes.has_value(), L"the journal file is there");
    const std::optional<Species::CommandJournal> read = Species::ReadJournal(bytes.value_or(std::vector<std::byte>{}));
    Assert::IsTrue(read.has_value() && read.value_or(Species::CommandJournal{}).entries == written.entries && written.entries.size() == 2,
                   L"the connection and the command, read back");
    Assert::IsFalse(read.value_or(Species::CommandJournal{}).restored, L"from the world's start");

    Species::WorldServer restored(desc);
    ExpectEqual(static_cast<std::int64_t>(first + 10), static_cast<std::int64_t>(restored.Open()), L"restored where it froze");
    const Species::CommandJournal from = restored.Journal().value_or(Species::CommandJournal{});
    Assert::IsTrue(restored.Journal().has_value() && from.restored && from.start.firstTick == first + 10 && from.entries.empty(),
                   L"a restored world journals from its snapshot");
  }

#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp005ADevelopmentCommandRestartsTheSandbox)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp005ADevelopmentCommandRestartsTheSandbox)
  {
    // M1 plan §6: the restart replaces the battle between ticks with the same seed or a new one, at the next tick, with
    // a new key and journal; the steward is connected to it and commands its side again.
    Species::WorldServer server({.tuning = CommittedTuning(), .worldSeed = 1, .sandbox = true});
    const std::uint64_t first = server.Open();
    server.Connect(STEWARD_CONNECTION);
    Neuron::WorkerPool pool(2);
    RecordingSender sender;
    for (std::uint64_t tick = first; tick < first + 50; ++tick)
    {
      server.Step(tick, pool);
      server.Publish(tick, sender);
    }
    const std::uint64_t seed = server.Simulation()->WorldSeed();
    // ADR-119: the terrain names the side the steward commands.
    Assert::IsTrue(sender.terrains.size() == 1 && sender.terrains[0].commandedTeam == Species::SANDBOX_STEWARD_TEAM,
                   L"the steward's side with the terrain");
    Assert::IsTrue(server.Journal().has_value(), L"a new world has its journal");
    const Neuron::SipHashKey key = server.Journal().value_or(Species::CommandJournal{}).start.key;
    server.Receive(STEWARD_CONNECTION, CommandFrom(Species::SandboxRestartCommand{false}));
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    Assert::IsTrue(server.Simulation()->NextTick() == first + 50 && server.Simulation()->WorldSeed() == seed,
                   L"the same seed, the next tick");
    ExpectEqual(2 * CommittedTuning().mvp.sandbox.sideWorkers,
                static_cast<std::int64_t>(region.TeamAt(0).figures.Count()) + static_cast<std::int64_t>(region.TeamAt(1).figures.Count()),
                L"both sides whole");
    const Species::CommandJournal journal = server.Journal().value_or(Species::CommandJournal{});
    Assert::IsTrue(journal.start.firstTick == first + 50 && (journal.start.key.low != key.low || journal.start.key.high != key.high),
                   L"a new journal with a new key");
    Assert::IsTrue(journal.entries.size() == 1 && journal.entries[0].connection, L"which connects the steward");
    Assert::IsTrue(region.MayCommand(journal.entries[0].sender, Species::SANDBOX_STEWARD_TEAM, false), L"to its side");
    server.Step(first + 50, pool);
    server.Publish(first + 50, sender);
    Assert::IsTrue(!sender.reports.empty() && sender.reports.back().sequence == 0 &&
                     sender.reports.back().result == Species::CommandResult::Applied,
                   L"the restart's outcome");
    Assert::IsTrue(sender.terrains.size() == 2 && sender.terrains[1].commandedTeam == Species::SANDBOX_STEWARD_TEAM,
                   L"the new world's terrain, with the side again");

    server.Receive(STEWARD_CONNECTION, CommandFrom(Species::SandboxRestartCommand{true}));
    Assert::IsTrue(server.Simulation()->NextTick() == first + 51 &&
                     server.Journal().value_or(Species::CommandJournal{}).start.firstTick == first + 51,
                   L"a drawn seed, the next tick");

    // A server that runs no sandbox refuses the command as one it does not know.
    Species::WorldServer world({.tuning = CommittedTuning(), .worldSeed = 1});
    static_cast<void>(world.Open());
    world.Connect(STEWARD_CONNECTION);
    world.Receive(STEWARD_CONNECTION, CommandFrom(Species::SandboxRestartCommand{false}));
    ExpectEqual(1, static_cast<std::int64_t>(world.Simulation()->Commands().Rejections().unknownType), L"refused as unknown");
  }
#endif
};

} // namespace GameLogicTests
