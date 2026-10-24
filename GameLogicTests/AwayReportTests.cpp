#include "pch.h"

#include "AwayMessages.h"
#include "AwayReport.h"
#include "Battles.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "ServerHost.h"
#include "Settlement.h"
#include "TestSupport.h"
#include "TickSchedule.h"
#include "WorldServer.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2;
constexpr std::uint8_t NEIGHBOR_SITE = 3;
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;
constexpr std::uint64_t IDLE_TICKS = 300; // mvp.presence.idle_minutes of 0.5

const Species::TuningMvp& FastTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.conflict.warningMinutes = 0.1;
    tuning.pil.battle.maxMinutes = 2.0;
    tuning.mvp.expedition.sizeMax = 12;
    return tuning;
  }();
  return TUNING;
}

Neuron::Vec3 WellspringAt(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  const Species::Settlement* settlement = _region.SettlementAt(_site);
  return _region.ResolveStructure(settlement->wellspringSlot, settlement->wellspringId)->position;
}

Neuron::Vec3 Toward(const Species::RegionSimulation& _region, const Neuron::Vec3& _from, const Neuron::Vec3& _toward, double _distance)
{
  const Neuron::Vec3 across{_toward.x - _from.x, 0.0, _toward.z - _from.z};
  Neuron::Vec3 point = _from + across * (_distance / Neuron::Length(across));
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

Species::StableId RaiseLeader(Species::RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _at, std::uint32_t _followers)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.kind = Species::FigureKind::Leader;
  spawn.position = _at;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  const Species::StableId id = _region.TeamAt(_team).figures.At(slot).id;
  spawn.kind = Species::FigureKind::Worker;
  spawn.count = _followers;
  spawn.spread = 20.0;
  for (const std::uint32_t worker : _region.SpawnFigures(spawn))
  {
    Species::Figure& follower = _region.TeamAt(_team).figures.At(worker);
    follower.workerState = Species::WorkerState::FollowingLeader;
    follower.leaderId = id;
    follower.leaderSlot = slot;
  }
  return id;
}

bool StepUntil(Species::WorldSimulation& _world, std::uint64_t _ticks, const std::function<bool()>& _until)
{
  Neuron::WorkerPool pool(0);
  for (std::uint64_t step = 0; step < _ticks; ++step)
  {
    _world.Step(_world.Region(0).NextTick(), pool);
    if (_until())
    {
      return true;
    }
  }
  return false;
}

// What a world server sends, by connection: each reliable message's bytes, in order.
class RecordingSender final : public Neuron::ConnectionSender
{
public:
  std::optional<std::uint64_t> Send(Neuron::ConnectionId _connection, Neuron::Channel _channel,
                                    std::span<const std::byte> _message) override
  {
    if (_channel == Neuron::Channel::Reliable)
    {
      m_sent[_connection].emplace_back(_message.begin(), _message.end());
    }
    return 0;
  }

  // A sender of a test closes nothing: its server is given no identity, so it refuses no join (ADR-189).
  void Close(Neuron::ConnectionId /*_connection*/) override {}

  // The away reports sent to _connection since the last call.
  std::vector<Species::AwayReport> TakeReports(Neuron::ConnectionId _connection)
  {
    std::vector<Species::AwayReport> reports;
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::AwayReport>([&reports](const Species::AwayReport& _report) { reports.push_back(_report); });
    for (const std::vector<std::byte>& message : m_sent[_connection])
    {
      static_cast<void>(dispatcher.Dispatch(message));
    }
    m_sent[_connection].clear();
    return reports;
  }

private:
  std::map<Neuron::ConnectionId, std::vector<std::vector<std::byte>>> m_sent;
};

template <class T> Neuron::ReceivedMessage MessageOf(const T& _message)
{
  return Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(_message)};
}

} // namespace

TEST_CLASS(AwayReportTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp074TheReportSummarizesTheAbsenceFromTheRegionsRecords)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-074")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-131")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-026")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp074TheReportSummarizesTheAbsenceFromTheRegionsRecords)
  {
    // ADR-186: an absence in which site 3 raids the steward's settlement, its battle ends, the deputy orders the
    // settlement's Leader and the world freezes once. The report agrees with the battle's result, the absence's Workers,
    // stores and grievances, the world record and the deputy's actions.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsFalse(Species::AwayReportOf(*world, STEWARD_SITE, region.NextTick(), false).absence, L"no absence yet");
    Assert::IsTrue(Species::SetStewardPresence(*world, true), L"away");
    const std::uint64_t from = region.NextTick();
    const Species::StableId defender =
      RaiseLeader(region, STEWARD_SITE, Toward(region, WellspringAt(region, STEWARD_SITE), WellspringAt(region, 0), 90.0), 6);
    const Neuron::Vec3 staging = Toward(region, WellspringAt(region, STEWARD_SITE), WellspringAt(region, NEIGHBOR_SITE),
                                        region.SettlementAt(STEWARD_SITE)->territoryRadius + FastTuning().mvp.battle.objectiveRadius);
    const Species::StableId attacker = RaiseLeader(region, NEIGHBOR_SITE, staging, 6);
    std::ranges::find(region.Settlements(), NEIGHBOR_SITE, &Species::Settlement::site)->storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, NEIGHBOR_SITE, {attacker, 0, 0}, STEWARD_SITE, 0).has_value(), L"declared");
    Assert::IsTrue(StepUntil(*world, 2'000, [&] { return !region.BattleResults().empty(); }), L"its battle ends");
    world->Record().freezes.push_back({region.NextTick(), Species::FreezeCause::ServerStall, 42});
    StepUntil(*world, 5, [] { return false; });
    Assert::IsTrue(Species::SetStewardPresence(*world, false), L"back");
    const std::uint64_t to = region.NextTick();

    const Species::AwayReport report = Species::AwayReportOf(*world, STEWARD_SITE, to, true);
    const Species::Absence& absence = *Species::LatestAbsence(region, STEWARD_SITE);
    Assert::IsTrue(report.absence && report.onReturn && report.fromTick == from && report.toTick == to, L"the absence's ticks");
    Assert::IsTrue(report.workersFrom == absence.workersFrom && report.workersTo == absence.workersTo &&
                     report.storesFromMilliRu == absence.storesFromMilliRu && report.storesToMilliRu == absence.storesToMilliRu,
                   L"its Workers and stores at either end");
    const Species::BattleResult& result = region.BattleResults().front();
    Assert::IsTrue(report.battles.size() == 1 && report.battles[0].other == NEIGHBOR_SITE && report.battles[0].defended &&
                     report.battles[0].won == !result.attackerWon && report.battles[0].workersLost == result.defenderLost &&
                     report.battles[0].storesMilliRu == -result.storesMovedMilliRu && report.battles[0].end == result.end,
                   L"the battle as its result has it, from the settlement's side");
    Assert::IsTrue(std::ranges::any_of(report.grievances,
                                       [](const Species::AwayGrievanceLine& _line) {
                                         return _line.holder == STEWARD_SITE && _line.against == NEIGHBOR_SITE &&
                                                _line.cause == Species::GrievanceCause::Attack;
                                       }),
                   L"the attack's grievance against the raider");
    Assert::IsTrue(report.freezes.size() == 1 && report.freezes[0].seconds == 42 &&
                     report.freezes[0].cause == Species::FreezeCause::ServerStall,
                   L"the freeze");
    Assert::IsTrue(std::ranges::any_of(report.actions,
                                       [defender](const Species::AwayActionLine& _line) {
                                         return _line.leader == defender && _line.order == Species::StandingOrder::DefendTheSettlement &&
                                                _line.other == NEIGHBOR_SITE;
                                       }),
                   L"the deputy's action with its order");
    ExpectEqual(static_cast<std::int64_t>(absence.actions.size()), report.actionsTotal, L"every action counted");
    Assert::IsTrue(report.HasNews(), L"news");

    // The report goes on the wire whole.
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::AwayReport> read;
    dispatcher.Register<Species::AwayReport>([&read](const Species::AwayReport& _report) { read = _report; });
    Assert::IsTrue(dispatcher.Dispatch(Neuron::EncodeMessage(report)) && read == report, L"and reads back whole");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp074AnAbsenceWithNothingInItHasNoNews)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-074")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp074AnAbsenceWithNothingInItHasNoNews)
  {
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Assert::IsTrue(Species::SetStewardPresence(*world, true), L"away");
    StepUntil(*world, 20, [] { return false; });
    Assert::IsTrue(Species::SetStewardPresence(*world, false), L"back");
    const Species::AwayReport report = Species::AwayReportOf(*world, STEWARD_SITE, world->NextTick(), false);
    Assert::IsTrue(report.absence && !report.HasNews(), L"an absence without news");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp074TheServerSendsTheReportOnAReturnWithNewsAndWhenAsked)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-074")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp074TheServerSendsTheReportOnAReturnWithNewsAndWhenAsked)
  {
    // O110 (ADR-186): a return whose absence holds nothing sends no report; one whose absence holds a freeze sends it after
    // its tick, marked as a return's; a request is answered after the next tick, unmarked, with the latest absence.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.tuning.mvp.presence.idleMinutes = 0.5;
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = STEWARD_SITE;
    desc.pace = Species::WorldPace::AlwaysOn;
    Species::WorldServer server(desc);
    static_cast<void>(server.Open());
    Neuron::WorkerPool pool(0);
    RecordingSender sender;
    const auto run = [&](std::uint64_t _until, const std::function<void(std::uint64_t)>& _before)
    {
      for (std::uint64_t tick = server.Simulation()->NextTick(); tick < _until; ++tick)
      {
        _before(tick);
        server.Step(tick, pool);
        server.Publish(tick, sender);
      }
    };
    run(5,
        [&](std::uint64_t _tick)
        {
          if (_tick == 3)
          {
            server.Connect(STEWARD_CONNECTION);
          }
        });
    Assert::IsTrue(sender.TakeReports(STEWARD_CONNECTION).empty(), L"the first absence, of the ticks before the client, has no news");

    const std::uint64_t stalled = 3 + IDLE_TICKS + 20;
    const std::uint64_t back = stalled + 30;
    run(back + 1,
        [&](std::uint64_t _tick)
        {
          if (_tick == stalled)
          {
            server.Stalled(Neuron::ScheduleStall{_tick, Neuron::Nanoseconds{12'000'000'000}, true});
          }
          if (_tick == back)
          {
            server.Receive(STEWARD_CONNECTION, MessageOf(Species::PresenceReportCommand{}));
          }
        });
    const std::vector<Species::AwayReport> returned = sender.TakeReports(STEWARD_CONNECTION);
    Assert::IsTrue(returned.size() == 1 && returned[0].onReturn && returned[0].fromTick == 3 + IDLE_TICKS && returned[0].toTick == back &&
                     returned[0].freezes.size() == 1 && returned[0].freezes[0].seconds == 12,
                   L"the return's report, with the stall's freeze");

    run(back + 3,
        [&](std::uint64_t _tick)
        {
          if (_tick == back + 1)
          {
            server.Receive(STEWARD_CONNECTION, MessageOf(Species::ReadAwayReportCommand{STEWARD_SITE}));
          }
        });
    const std::vector<Species::AwayReport> asked = sender.TakeReports(STEWARD_CONNECTION);
    Assert::IsTrue(asked.size() == 1 && !asked[0].onReturn && asked[0].fromTick == returned[0].fromTick,
                   L"the request's, the same absence");
  }
};

} // namespace GameLogicTests
