#include "pch.h"

#include "AwayMessages.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "SettlementMessages.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_SITE = 2; // the steward's settlement, whose team is its site
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;
constexpr std::uint64_t IDLE_TICKS = 150; // mvp.presence.idle_minutes of 0.25 at 10 ticks a second

// A world server of the valley with the steward's settlement at _pace, whose steward is idle after IDLE_TICKS.
Species::WorldServer::Desc PresenceDesc(Species::WorldPace _pace)
{
  Species::WorldServer::Desc desc;
  desc.tuning = CommittedTuning();
  desc.tuning.mvp.presence.idleMinutes = 0.25;
  desc.structures = TestRecords();
  desc.worldSeed = ValleyTerrain()->seed;
  desc.stewardSite = SETTLED_SITE;
  desc.pace = _pace;
  return desc;
}

template <class T> Neuron::ReceivedMessage MessageOf(const T& _message)
{
  return Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(_message)};
}

// Runs the server's ticks from its next to _untilTick, calling _before ahead of each, as the host delivers connections
// and messages before a tick's step.
void RunTo(Species::WorldServer& _server, std::uint64_t _untilTick, Neuron::WorkerPool& _pool,
           const std::function<void(std::uint64_t)>& _before = {})
{
  for (std::uint64_t tick = _server.Simulation()->NextTick(); tick < _untilTick; ++tick)
  {
    if (_before)
    {
      _before(tick);
    }
    _server.Step(tick, _pool);
  }
}

const Species::Absence* OpenOf(const Species::WorldServer& _server)
{
  return Species::OpenAbsence(_server.Simulation()->Region(0), SETTLED_SITE);
}

const Species::Absence* LatestOf(const Species::WorldServer& _server)
{
  return Species::LatestAbsence(_server.Simulation()->Region(0), SETTLED_SITE);
}

} // namespace

TEST_CLASS(PresenceTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp072PresenceFollowsTheConnectionAndTheStewardsInput)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-070")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp072PresenceFollowsTheConnectionAndTheStewardsInput)
  {
    // ADR-185: away with no client connected; present from the connection; away once its input is IDLE_TICKS old; present
    // again on a presence report or a command a queue takes; and not on a request, which is read, not input.
    Species::WorldServer server(PresenceDesc(Species::WorldPace::AlwaysOn));
    static_cast<void>(server.Open());
    Neuron::WorkerPool pool(0);
    RunTo(server, 10, pool);
    Assert::IsTrue(OpenOf(server) != nullptr && OpenOf(server)->fromTick == 0, L"away from the first tick, with no client");
    const std::uint32_t workers = OpenOf(server)->workersFrom;
    ExpectEqual(20, workers, L"the settlement's Workers when the absence opened");

    server.Connect(STEWARD_CONNECTION);
    RunTo(server, 11, pool);
    Assert::IsTrue(OpenOf(server) == nullptr, L"present once a client connects");
    Assert::IsTrue(LatestOf(server) != nullptr && LatestOf(server)->toTick == 10, L"the absence closed at the connection's tick");

    RunTo(server, 10 + IDLE_TICKS, pool);
    Assert::IsTrue(OpenOf(server) == nullptr, L"present while the connection's input is younger than the idle minutes");
    RunTo(server, 11 + IDLE_TICKS, pool);
    Assert::IsTrue(OpenOf(server) != nullptr && OpenOf(server)->fromTick == 10 + IDLE_TICKS, L"away once it is as old");

    constexpr std::uint64_t REPORTED = 250;
    RunTo(server, REPORTED + 1, pool,
          [&](std::uint64_t _tick)
          {
            if (_tick == REPORTED)
            {
              server.Receive(STEWARD_CONNECTION, MessageOf(Species::PresenceReportCommand{}));
            }
          });
    Assert::IsTrue(OpenOf(server) == nullptr && LatestOf(server)->fromTick == 10 + IDLE_TICKS && LatestOf(server)->toTick == REPORTED,
                   L"present again from the tick of a presence report, which replaces the earlier closed absence");

    constexpr std::uint64_t ASKED = REPORTED + IDLE_TICKS + 50;
    constexpr std::uint64_t COMMANDED = ASKED + 50;
    RunTo(server, COMMANDED + 1, pool,
          [&](std::uint64_t _tick)
          {
            if (_tick == ASKED)
            {
              server.Receive(STEWARD_CONNECTION, MessageOf(Species::ReadSettlementStatusCommand{SETTLED_SITE}));
            }
            if (_tick == ASKED + 1)
            {
              Assert::IsTrue(OpenOf(server) != nullptr, L"a status request is no input");
            }
            if (_tick == COMMANDED)
            {
              server.Receive(STEWARD_CONNECTION, MessageOf(Species::SetPrioritiesCommand{SETTLED_SITE, {40, 40, 20}}));
            }
          });
    Assert::IsTrue(OpenOf(server) == nullptr && LatestOf(server)->toTick == COMMANDED, L"a command the queue took is input");

    server.Disconnect(STEWARD_CONNECTION);
    RunTo(server, COMMANDED + 2, pool);
    Assert::IsTrue(OpenOf(server) != nullptr && OpenOf(server)->fromTick == COMMANDED + 1, L"away once the client leaves");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp072ASessionPacedWorldHasNoAwayState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp072ASessionPacedWorldHasNoAwayState)
  {
    Species::WorldServer server(PresenceDesc(Species::WorldPace::Session));
    static_cast<void>(server.Open());
    Neuron::WorkerPool pool(0);
    RunTo(server, 2 * IDLE_TICKS, pool);
    Assert::IsTrue(server.Simulation()->Region(0).Absences().empty(), L"no absence, connected or not");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp076PresenceChangesReplayFromTheJournal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp076PresenceChangesReplayFromTheJournal)
  {
    // ADR-185: the journal holds each presence change at its tick, and its replay gives the server's world tick by tick;
    // the world's absences save and restore with it, and a journal file keeps them.
    Species::WorldServer server(PresenceDesc(Species::WorldPace::AlwaysOn));
    static_cast<void>(server.Open());
    Neuron::WorkerPool pool(0);
    std::vector<std::uint64_t> hashes;
    for (std::uint64_t tick = 0; tick < 3 * IDLE_TICKS; ++tick)
    {
      if (tick == 50)
      {
        server.Connect(STEWARD_CONNECTION);
      }
      if (tick == 120)
      {
        server.Receive(STEWARD_CONNECTION, MessageOf(Species::SetPrioritiesCommand{SETTLED_SITE, {50, 30, 20}}));
      }
      server.Step(tick, pool);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
    }
    const std::optional<Species::CommandJournal> read =
      Species::ReadJournal(Species::WriteJournal(server.Journal().value_or(Species::CommandJournal{})));
    Assert::IsTrue(read.has_value(), L"the journal reads back");
    const Species::CommandJournal journal = read.value_or(Species::CommandJournal{});
    // Each presence change as its tick and whether it went away.
    std::vector<std::pair<std::uint64_t, bool>> presence;
    for (const Species::JournalEntry& entry : journal.entries)
    {
      if (entry.presence)
      {
        presence.emplace_back(entry.tick, entry.away);
      }
    }
    const std::vector<std::pair<std::uint64_t, bool>> expected{{0, true}, {50, false}, {120 + IDLE_TICKS, true}};
    Assert::IsTrue(presence == expected, L"away at 0, back at the connection, away once the command is the idle minutes old");

    std::vector<std::uint64_t> replayed;
    const std::unique_ptr<Species::WorldSimulation> replay =
      Species::ReplayJournal(journal, ValleyTerrain(), server.Tuning(), &server.Structures(), 3 * IDLE_TICKS, pool,
                             [&replayed](const Species::WorldSimulation& _world) { replayed.push_back(Species::HashWorld(_world).world); });
    Assert::IsTrue(replayed == hashes, L"the replay hashes as the server's world on every tick");
    Assert::IsTrue(replay->Region(0).Absences() == server.Simulation()->Region(0).Absences(), L"with the same absences");

    auto restored = Species::RestoreWorld(Species::SnapshotOf(*server.Simulation()), server.Tuning(), &server.Structures());
    Assert::IsTrue(restored.has_value() && (*restored)->Region(0).Absences() == server.Simulation()->Region(0).Absences(),
                   L"the absences save and restore");
    Assert::IsTrue(Species::StewardAway(**restored), L"with the steward away");
  }
};

} // namespace GameLogicTests
