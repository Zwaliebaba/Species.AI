#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "CommandMessages.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "Relations.h"
#include "Renown.h"
#include "SettlementMessages.h"
#include "Statistics.h"
#include "TestSupport.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, valley neighbour of site 3 (pair 1)
constexpr std::uint8_t TARGET_SITE = 3;
constexpr std::uint8_t NEIGHBORS_HALF = 3; // pair 1's half nearer site 3
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;

// The committed tuning with a run of 3 minutes, its last half minute quiet, and a short warning and battle.
const Species::TuningMvp& ShortRunTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.run.lengthHours = 0.05; // 1,800 ticks
    tuning.mvp.run.quietMinutes = 0.5; // 300 ticks
    tuning.mvp.conflict.warningMinutes = 0.1;
    tuning.pil.battle.maxMinutes = 2.0;
    tuning.mvp.persist.snapshotSeconds = 5.0;
    tuning.mvp.persist.snapshotsKept = 3;
    tuning.mvp.claim.uncontestedMinutes = 0.05; // 30 ticks
    return tuning;
  }();
  return TUNING;
}

Species::Settlement& SettlementAt(Species::RegionSimulation& _region, std::uint8_t _site)
{
  return *std::ranges::find(_region.Settlements(), _site, &Species::Settlement::site);
}

Neuron::Vec3 WellspringAt(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  const Species::Settlement* settlement = _region.SettlementAt(_site);
  return _region.ResolveStructure(settlement->wellspringSlot, settlement->wellspringId)->position;
}

// Where the warning holds an expedition of _attacker's that raids _target: the objective radius beyond its territory.
Neuron::Vec3 StagingOf(const Species::RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _target)
{
  const Neuron::Vec3 from = WellspringAt(_region, _target);
  const Neuron::Vec3 toward = WellspringAt(_region, _attacker);
  const Neuron::Vec3 across{toward.x - from.x, 0.0, toward.z - from.z};
  const double distance = _region.SettlementAt(_target)->territoryRadius + _region.Tuning().mvp.battle.objectiveRadius;
  Neuron::Vec3 point = from + across * (distance / Neuron::Length(across));
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

// A Leader of the settlement at _site at _at, followed by _followers Workers of its own spawned with it.
Species::StableId RaiseLeader(Species::RegionSimulation& _region, std::uint8_t _site, const Neuron::Vec3& _at, std::uint32_t _followers)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _site;
  spawn.kind = Species::FigureKind::Leader;
  spawn.position = _at;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  const Species::StableId id = _region.TeamAt(_site).figures.At(slot).id;
  spawn.kind = Species::FigureKind::Worker;
  spawn.count = _followers;
  spawn.spread = 20.0;
  for (const std::uint32_t worker : _region.SpawnFigures(spawn))
  {
    Species::Figure& follower = _region.TeamAt(_site).figures.At(worker);
    follower.workerState = Species::WorkerState::FollowingLeader;
    follower.leaderId = id;
    follower.leaderSlot = slot;
  }
  return id;
}

// _count more Workers of the settlement at _site at its Wellspring.
void Strengthen(Species::RegionSimulation& _region, std::uint8_t _site, std::uint32_t _count)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _site;
  spawn.position = WellspringAt(_region, _site);
  spawn.count = _count;
  spawn.spread = 30.0;
  static_cast<void>(_region.SpawnFigures(spawn));
}

// Steps _world from its next tick until _until holds after a step, for _ticks at most; true when it held.
bool StepUntil(Species::WorldSimulation& _world, std::uint64_t _ticks, const std::function<bool()>& _until)
{
  Neuron::WorkerPool pool(0);
  for (std::uint64_t step = 0; step < _ticks; ++step)
  {
    _world.Step(_world.NextTick(), pool);
    if (_until())
    {
      return true;
    }
  }
  return false;
}

// The steward's raid on site 3, whose defender has 20 more Workers, stepped until its battle starts; for a claim battle
// when _claim names the steward's claim.
void StartBattle(Species::WorldSimulation& _world, Species::StableId _claim = 0)
{
  Species::RegionSimulation& region = _world.Region(0);
  Strengthen(region, TARGET_SITE, 20);
  const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, StagingOf(region, STEWARD_SITE, TARGET_SITE), 6);
  SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
  Assert::IsTrue(Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, _claim).has_value(), L"declared");
  Assert::IsTrue(StepUntil(_world, 1'500, [&] { return !region.Battles().empty(); }), L"its battle starts");
}

// The region's first battle ended by its time, at the region's next tick; the ticks wrap as the battle's own do.
void EndByTime(Species::RegionSimulation& _region)
{
  const Species::TuningMvp& tuning = _region.Tuning();
  const auto maxTicks = static_cast<std::uint64_t>(std::llround(tuning.pil.battle.maxMinutes * 60.0 / tuning.ref.sim.tickPeriod));
  _region.Battles().front().startTick = _region.NextTick() - maxTicks;
  Species::AdvanceBattles(_region, _region.NextTick());
}

std::vector<Species::StatisticEntry> OfKind(const Species::RegionSimulation& _region, Species::StatisticKind _kind)
{
  std::vector<Species::StatisticEntry> entries;
  std::ranges::copy_if(_region.Statistics(), std::back_inserter(entries),
                       [_kind](const Species::StatisticEntry& _entry) { return _entry.kind == _kind; });
  return entries;
}

// The region's state after its index and its tick, which an ended world's tick still advances.
std::vector<std::byte> RegionBytes(const Species::RegionSimulation& _region)
{
  constexpr std::size_t INDEX_AND_TICK_BITS = 16 + 64;
  Neuron::BitWriter whole;
  _region.WriteState(whole);
  const std::size_t bits = whole.BitCount();
  Neuron::BitReader reader(whole.Bytes());
  static_cast<void>(reader.ReadBits(16));
  static_cast<void>(reader.ReadBits(64));
  Neuron::BitWriter body;
  for (std::size_t done = INDEX_AND_TICK_BITS; done < bits;)
  {
    const auto chunk = static_cast<std::uint32_t>(std::min<std::size_t>(64, bits - done));
    body.WriteBits(reader.ReadBits(chunk), chunk);
    done += chunk;
  }
  return body.Take();
}

// What a connection was sent, the reliable channel's messages kept.
class KeptMessages final : public Neuron::ConnectionSender
{
public:
  [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId, Neuron::Channel _channel,
                                                  std::span<const std::byte> _message) override
  {
    if (_channel == Neuron::Channel::Reliable)
    {
      reliable.emplace_back(_message.begin(), _message.end());
      return 0;
    }
    return ++datagrams;
  }

  std::vector<std::vector<std::byte>> reliable;
  std::uint64_t datagrams = 0;
};

// The command reports among what a server published.
std::vector<Species::CommandReport> ReportsOf(Species::WorldServer& _server, std::uint64_t _tick)
{
  KeptMessages kept;
  _server.Publish(_tick, kept);
  std::vector<Species::CommandReport> reports;
  for (const std::vector<std::byte>& message : kept.reliable)
  {
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::CommandReport>([&reports](const Species::CommandReport& _report) { reports.push_back(_report); });
    static_cast<void>(dispatcher.Dispatch(message));
  }
  return reports;
}

} // namespace

TEST_CLASS(RunTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp060TheRunEndsOnTimeAfterItsQuietMinutes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp060TheRunEndsOnTimeAfterItsQuietMinutes)
  {
    // ADR-163: a run ends mvp.run.length_hours after its world began; in its last mvp.run.quiet_minutes no attack may be
    // declared; at its end, with no battle in progress, it ends, and the world is read-only: its tick advances and
    // nothing else.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, ShortRunTuning());
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsTrue(world->RunState() == Species::WorldSimulation::Run{1'800, false, 0} && world->TicksLeft() == 1'800 &&
                     region.RunEndTick() == 1'800,
                   L"a run of 1,800 ticks from tick 0, which the region knows");
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 1);
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsFalse(StepUntil(*world, 1'499, [&] { return region.InQuietMinutes(); }), L"no quiet before its last 300 ticks");
    Assert::IsTrue(StepUntil(*world, 1, [&] { return region.InQuietMinutes(); }) && world->TicksLeft() == 300, L"then quiet");
    const auto declared = Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, 0);
    Assert::IsTrue(!declared && declared.error() == Species::AttackRefusal::Quiet, L"no attack declared in the quiet minutes");
    Assert::IsFalse(StepUntil(*world, 299, [&] { return world->RunState().ended; }), L"not over before its end");
    Assert::IsTrue(StepUntil(*world, 1, [&] { return world->RunState().ended; }), L"over at its end");
    Assert::IsTrue(world->RunState().endedTick == 1'800 && world->TicksLeft() == 0, L"at tick 1,800");

    const std::vector<std::byte> ended = RegionBytes(region);
    Assert::IsFalse(StepUntil(*world, 50, [] { return false; }), L"fifty more ticks");
    Assert::IsTrue(world->NextTick() == 1'850 && region.NextTick() == 1'850 && RegionBytes(region) == ended,
                   L"the region unchanged but for its tick, which goes on");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp060BattlesInProgressFinishAndUnfoughtExpeditionsEnd)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-045")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp060BattlesInProgressFinishAndUnfoughtExpeditionsEnd)
  {
    // ADR-163: at the run's end an expedition whose battle has not started ends with no battle and no result, and the
    // run waits for the battles in progress, ending at the tick the last of them ends.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, ShortRunTuning());
    world->SetRunEnd(1'000'000);
    Species::RegionSimulation& region = world->Region(0);
    region.SettlementAis().clear(); // the attacks of the test alone
    StartBattle(*world);
    const Species::StableId other = RaiseLeader(region, 0, WellspringAt(region, 0), 2);
    SettlementAt(region, 0).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, 0, {other, 0, 0}, 1, 0).has_value() && region.Expeditions().size() == 2,
                   L"a second attack, held by its warning");
    world->SetRunEnd(world->NextTick() + 1);
    Assert::IsTrue(StepUntil(*world, 1, [] { return true; }) && region.Expeditions().size() == 1 && region.Battles().size() == 1 &&
                     region.BattleResults().empty() && !world->RunState().ended,
                   L"at the end the unfought expedition ends with no result, and the run waits for the battle");
    Assert::IsTrue(StepUntil(*world, 1'500, [&] { return world->RunState().ended; }), L"the run ends");
    Assert::IsTrue(region.Battles().empty() && region.BattleResults().size() == 1 &&
                     world->RunState().endedTick == region.BattleResults().front().endTick + 1,
                   L"at the tick after the battle's last");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp061RenownComesFromItsFourSourcesAndOrdersTheStanding)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-061")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-062")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-125")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp061RenownComesFromItsFourSourcesAndOrdersTheStanding)
  {
    // ADR-163: a battle held as defender, a claim battle won, each 20 weight eased by tribute or a withdrawn claim, and
    // each 100 ru of structures completed earn renown; stores taken earn none; the standing orders the settlements by it.
    const Species::TuningMvp& tuning = ShortRunTuning();
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, tuning);
    world->SetRunEnd(1'000'000);
    Species::RegionSimulation& region = world->Region(0);
    region.SettlementAis().clear();
    Assert::IsTrue(region.Renown().size() == 4 && std::ranges::all_of(region.Renown(), [&](const Species::SettlementRenown& _renown)
                                                                      { return Species::RenownOf(tuning, _renown).Total() == 0; }),
                   L"none at the founding");

    // A structure of 250 ru completed: 2 for its whole hundreds.
    const Neuron::Vec3 home = WellspringAt(region, 0);
    const std::uint32_t dwelling =
      region.AddStructure(Species::StructureKind::Dwelling, std::uint8_t{0}, std::nullopt, home.x + 150.0, home.z, {1.0, 0.0, 0.0});
    region.Structures().At(dwelling).costMilliRu = Species::MilliRuOf(250.0);
    region.Structures().At(dwelling).completedTick = world->NextTick();
    Assert::IsTrue(StepUntil(*world, 1, [] { return true; }), L"a step");
    ExpectEqual(2, Species::RenownOf(tuning, *Species::RenownAt(region, 0)).structures, L"each 100 ru of structures completed");

    // 40 weight eased by tribute and nearly 30 by a claim withdrawn after it stood its wait: 3 for the whole twenties.
    for (Species::Settlement& settlement : region.Settlements())
    {
      settlement.territoryRadius = tuning.wld.territory.maxRadius;
      region.RecomputeTerritory(settlement.site);
    }
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Species::AddGrievance(region, TARGET_SITE, STEWARD_SITE, Species::GrievanceCause::Attack, 77);
    Assert::IsTrue(
      Species::SendTribute(region, STEWARD_SITE, TARGET_SITE, Species::GrievanceCause::Attack, 77, Species::MilliRuOf(5'000.0)).has_value(),
      L"a tribute that eases the attack's whole weight");
    const Species::StableId hasty = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    Assert::IsFalse(Species::WithdrawClaim(region, STEWARD_SITE, hasty).has_value(), L"a claim withdrawn at once");
    ExpectEqual(std::llround(tuning.mvp.grievance.attackWeight * 1e6), Species::RenownAt(region, STEWARD_SITE)->easedMicro,
                L"eases nothing towards renown (DEC-1007)");
    const Species::StableId claim = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    Assert::IsTrue(StepUntil(*world, 30, [] { return false; }) == false, L"the claim stands mvp.claim.uncontested_minutes");
    const std::int64_t stood =
      std::ranges::find_if(region.Grievances(), [&](const Species::Grievance& _grievance) { return _grievance.subject == claim; })
        ->weightMicro;
    Assert::IsFalse(Species::WithdrawClaim(region, STEWARD_SITE, claim).has_value(), L"then withdrawn");
    const Species::SettlementRenown steward = *Species::RenownAt(region, STEWARD_SITE);
    Assert::IsTrue(steward.easedMicro == std::llround(tuning.mvp.grievance.attackWeight * 1e6) + stood &&
                     Species::RenownOf(tuning, steward).easing == 3,
                   L"each 20 weight eased, the withdrawn claim's as its grievance stood");

    // A raid the defender holds, and a claim battle the claimant wins.
    StartBattle(*world);
    EndByTime(region);
    ExpectEqual(1, Species::RenownAt(region, TARGET_SITE)->defensesHeld, L"the defender held");
    const Species::StableId won = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    region.Respites().clear();
    StartBattle(*world, won);
    Species::Battle& battle = region.Battles().front();
    battle.defenderLost = Species::LossCapOf(region, battle);
    Species::AdvanceBattles(region, world->NextTick());
    Assert::IsTrue(region.BattleResults().back().attackerWon && region.BattleResults().back().areaMoved, L"the claim battle won");
    const Species::RenownPoints points = Species::RenownOf(tuning, *Species::RenownAt(region, STEWARD_SITE));
    Assert::IsTrue(points.claims == tuning.mvp.renown.claimWon && points.defense == 0 && points.easing == 3, L"the claimant's renown");
    ExpectEqual(tuning.mvp.renown.defenseHeld, Species::RenownOf(tuning, *Species::RenownAt(region, TARGET_SITE)).defense,
                L"the defender's, and nothing for what it lost");

    // The standing: the steward's 3 + 3, the defender's 3, site 0's 2 and site 1's none.
    const std::vector<std::uint8_t> standing = Species::Standing(region);
    Assert::IsTrue(standing == std::vector<std::uint8_t>{STEWARD_SITE, TARGET_SITE, 0, 1}, L"ordered by renown");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp064TheStatisticsRecordHalfwayLinesGrievancesClaimsAndBattles)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp064TheStatisticsRecordHalfwayLinesGrievancesClaimsAndBattles)
  {
    // ADR-163: each territory's first reach of each of its halfway lines, each new grievance with its cause, each claim
    // made and how it ended, and each battle for both its sides, by its result's place.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, ShortRunTuning());
    world->SetRunEnd(1'000'000);
    Species::RegionSimulation& region = world->Region(0);
    region.SettlementAis().clear();
    Assert::IsTrue(StepUntil(*world, 1, [] { return true; }) && OfKind(region, Species::StatisticKind::HalfwayReached).empty(),
                   L"no halfway line at the founding radius");
    for (Species::Settlement& settlement : region.Settlements())
    {
      settlement.territoryRadius = ShortRunTuning().wld.territory.maxRadius;
      region.RecomputeTerritory(settlement.site);
    }
    Assert::IsTrue(StepUntil(*world, 2, [] { return false; }) == false, L"two steps");
    const std::vector<Species::StatisticEntry> halfway = OfKind(region, Species::StatisticKind::HalfwayReached);
    ExpectEqual(8, static_cast<std::int64_t>(halfway.size()), L"each settlement's two halfway lines, once each");
    for (const Species::StatisticEntry& entry : halfway)
    {
      const auto& pair = Species::VALLEY_PAIRS[entry.detail];
      Assert::IsTrue((pair[0] == entry.site || pair[1] == entry.site) && region.HalfwayReached(entry.site, entry.detail), L"its own pair");
    }

    const std::size_t grievances = OfKind(region, Species::StatisticKind::GrievanceAdded).size();
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 5);
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 5);
    const std::vector<Species::StatisticEntry> added = OfKind(region, Species::StatisticKind::GrievanceAdded);
    Assert::IsTrue(added.size() == grievances + 1 && added.back().site == 0 &&
                     added.back().detail == static_cast<std::uint32_t>(Species::GrievanceCause::Attack),
                   L"a new grievance once, held by its holder, with its cause");

    const Species::StableId withdrawn = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    Assert::IsFalse(Species::WithdrawClaim(region, STEWARD_SITE, withdrawn).has_value(), L"withdrawn");
    Species::SettleClaim(region, Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0), false);
    Species::SettleClaim(region, Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0), true);
    const auto areaOf = [](const Species::StatisticEntry& _entry)
    { return _entry.site == STEWARD_SITE && _entry.detail == NEIGHBORS_HALF; };
    Assert::IsTrue(OfKind(region, Species::StatisticKind::ClaimMade).size() == 3 &&
                     std::ranges::all_of(OfKind(region, Species::StatisticKind::ClaimMade), areaOf),
                   L"three claims made");
    Assert::IsTrue(OfKind(region, Species::StatisticKind::ClaimWithdrawn).size() == 1 &&
                     OfKind(region, Species::StatisticKind::ClaimLost).size() == 1 &&
                     OfKind(region, Species::StatisticKind::ClaimWon).size() == 1,
                   L"one withdrawn, one lost and one won");

    region.Respites().clear();
    StartBattle(*world);
    EndByTime(region);
    const std::vector<Species::StatisticEntry> battles = OfKind(region, Species::StatisticKind::BattleEnded);
    Assert::IsTrue(battles.size() == 2 && battles[0].site == STEWARD_SITE && battles[1].site == TARGET_SITE && battles[0].detail == 0 &&
                     battles[1].detail == 0,
                   L"the battle for each side, by its result's place");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063AWorldThatEndedIsReadOnlyAfterARestart)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063AWorldThatEndedIsReadOnlyAfterARestart)
  {
    // ADR-163: once its run has ended, the world server takes no command into a queue, answering each as not permitted
    // while it still answers a status request; its snapshot holds the ended run, and the world opened from it is
    // read-only as well.
    const TemporaryFolder folder;
    Species::TuningMvp tuning = ShortRunTuning();
    tuning.mvp.run.lengthHours = 0.003; // 108 ticks
    Species::WorldServer::Desc desc;
    desc.tuning = tuning;
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = STEWARD_SITE;
    desc.snapshotFolder = folder.Path();
    Neuron::WorkerPool pool(0);
    std::vector<std::byte> ended;
    {
      Species::WorldServer server(desc);
      std::uint64_t tick = server.Open();
      server.Connect(STEWARD_CONNECTION);
      for (; tick < 120; ++tick)
      {
        server.Step(tick, pool);
        static_cast<void>(ReportsOf(server, tick));
      }
      Assert::IsTrue(server.Simulation()->RunState().ended && server.Simulation()->RunState().endedTick == 108, L"the run ended");
      server.Receive(STEWARD_CONNECTION,
                     Neuron::ReceivedMessage{Neuron::Channel::Reliable,
                                             Neuron::EncodeMessage(Species::SetPrioritiesCommand{STEWARD_SITE, {40, 40, 20}})});
      server.Receive(
        STEWARD_CONNECTION,
        Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::ReadSettlementStatusCommand{STEWARD_SITE})});
      server.Step(tick, pool);
      const std::vector<Species::CommandReport> reports = ReportsOf(server, tick++);
      Assert::IsTrue(reports.size() == 2 && reports[0].result == Species::CommandResult::NotPermitted &&
                       reports[1].result == Species::CommandResult::Applied,
                     L"a command not permitted, a status request answered");
      ended = RegionBytes(server.Simulation()->Region(0));
      server.Freeze(tick);
    }
    Species::WorldServer reopened(desc);
    std::uint64_t tick = reopened.Open();
    Assert::IsTrue(reopened.Simulation()->RunState().ended && RegionBytes(reopened.Simulation()->Region(0)) == ended,
                   L"the world opens ended, as it was");
    for (const std::uint64_t last = tick + 30; tick < last; ++tick)
    {
      reopened.Step(tick, pool);
    }
    Assert::IsTrue(RegionBytes(reopened.Simulation()->Region(0)) == ended, L"and stays read-only");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095TheRunAndRenownAreSavedAndAVersion14RegionStartsWithNone)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095TheRunAndRenownAreSavedAndAVersion14RegionStartsWithNone)
  {
    // ADR-163: the snapshot holds the run, the renown and the statistics of the new kinds, whose halfway lines the
    // restored region knows again; a region of version 14 reads with its settlements' renown at none, and an entry of a
    // version before 15 reads its kind in the narrower bound it was written with.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, ShortRunTuning());
    Species::RegionSimulation& region = world->Region(0);
    for (Species::Settlement& settlement : region.Settlements())
    {
      settlement.territoryRadius = ShortRunTuning().wld.territory.maxRadius;
      region.RecomputeTerritory(settlement.site);
    }
    Species::RenownAt(region, 1)->easedMicro = 21'000'000;
    Assert::IsTrue(StepUntil(*world, 2, [] { return false; }) == false, L"two steps");
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), ShortRunTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    const Species::RegionSimulation& copy = back->Region(0);
    Assert::IsTrue(back->RunState() == world->RunState() && copy.Renown() == region.Renown() && copy.Statistics() == region.Statistics(),
                   L"the run, the renown and the statistics");
    Assert::IsTrue(copy.HalfwayReached(STEWARD_SITE, 1) && copy.HalfwayReached(TARGET_SITE, 1), L"the halfway lines known again");

    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld();
    const std::vector<std::byte> older = RegionStateOf(plain->Region(0), 14);
    Species::RegionSimulation fourteen(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), CommittedTuning(), ValleyTerrain(),
                                       &TestRecords());
    Neuron::BitReader reader(older);
    Assert::IsTrue(fourteen.ReadState(reader, 14) && reader.Finish(), L"a version-14 region reads whole");
    Assert::IsTrue(fourteen.Renown() == plain->Region(0).Renown() && fourteen.Renown().size() == 4, L"its renown at none");

    Neuron::BitWriter entry;
    entry.WriteBits(7, 64);
    entry.WriteBits(2, 8);
    entry.WriteBounded(static_cast<std::uint64_t>(Species::StatisticKind::PopulationStep), Species::STATISTIC_KIND_MAX_BEFORE_RENOWN);
    entry.WriteBits(30, 32);
    entry.WriteBits(30, 32);
    entry.WriteSigned(1'000, 64);
    const std::vector<std::byte> bytes = entry.Take();
    Neuron::BitReader old(bytes);
    Species::StatisticEntry read;
    Assert::IsTrue(Species::ReadStatisticEntry(old, read, 14) && read.kind == Species::StatisticKind::PopulationStep && read.detail == 30 &&
                     read.strengthMilliRu == 1'000,
                   L"an entry of version 14");
  }
};

} // namespace GameLogicTests
