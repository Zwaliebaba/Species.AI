#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "CommandMessages.h"
#include "Construction.h"
#include "Deputy.h"
#include "Formation.h"
#include "LeaderOrders.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "Relations.h"
#include "Settlement.h"
#include "TestSupport.h"
#include "Territory.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, valley neighbour of site 3 (pair 1)
constexpr std::uint8_t NEIGHBOR_SITE = 3;
constexpr std::uint8_t STEWARDS_HALF = 2; // pair 1's half nearer site 2
constexpr std::uint32_t DEFENDER_EXTRA = 20;

// The committed tuning with the warning, the battle's longest time and an expedition's size shortened, as the battle
// tests have it.
const Species::TuningMvp& FastTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.conflict.warningMinutes = 0.1; // 60 ticks
    tuning.pil.battle.maxMinutes = 2.0;       // 1,200 ticks
    tuning.mvp.expedition.sizeMax = 12;
    return tuning;
  }();
  return TUNING;
}

std::int64_t DecisionTicks()
{
  return FastTuning().mvp.ai.decisionTicks;
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

Neuron::Vec3 Toward(const Species::RegionSimulation& _region, const Neuron::Vec3& _from, const Neuron::Vec3& _toward, double _distance)
{
  const Neuron::Vec3 across{_toward.x - _from.x, 0.0, _toward.z - _from.z};
  Neuron::Vec3 point = _from + across * (_distance / Neuron::Length(across));
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

// Where the warning holds an expedition of _attacker's that raids _target.
Neuron::Vec3 StagingOf(const Species::RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _target)
{
  return Toward(_region, WellspringAt(_region, _target), WellspringAt(_region, _attacker),
                _region.SettlementAt(_target)->territoryRadius + _region.Tuning().mvp.battle.objectiveRadius);
}

// A Leader of the settlement at _site's team _team at _at, followed by _followers Workers spawned with it.
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

void Strengthen(Species::RegionSimulation& _region, std::uint8_t _site, std::uint32_t _count)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _site;
  spawn.position = WellspringAt(_region, _site);
  spawn.count = _count;
  spawn.spread = 30.0;
  static_cast<void>(_region.SpawnFigures(spawn));
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

void StepFor(Species::WorldSimulation& _world, std::uint64_t _ticks)
{
  static_cast<void>(StepUntil(_world, _ticks, [] { return false; }));
}

const Species::Figure* FigureById(const Species::RegionSimulation& _region, std::uint8_t _team, Species::StableId _id)
{
  const std::optional<Species::ActorRef> ref = _region.FindFigure(_team, _id);
  return ref ? _region.Resolve(*ref) : nullptr;
}

const std::vector<Species::DeputyAction>& ActionsOf(const Species::RegionSimulation& _region)
{
  static const std::vector<Species::DeputyAction> NONE;
  const Species::Absence* absence = Species::OpenAbsence(_region, STEWARD_SITE);
  return absence != nullptr ? absence->actions : NONE;
}

// The valley on FastTuning, its steward away from now on.
std::unique_ptr<Species::WorldSimulation> AwayValley()
{
  std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
  Assert::IsTrue(Species::SetStewardPresence(*world, true), L"the steward goes away");
  return world;
}

const Species::Structure* SiteInHalf(const Species::RegionSimulation& _region, std::uint8_t _area)
{
  const Species::SlicedPool<Species::Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).kind == Species::StructureKind::ResourceSite &&
        Species::InBorderHalf(_region, _area, structures.At(slot).position.x, structures.At(slot).position.z))
    {
      return &structures.At(slot);
    }
  }
  return nullptr;
}

// The Leader stands in formation on its way to _objective, and the deputy recorded one action for it, of _order against
// the settlement at _other.
void ExpectSentToObjective(const Species::RegionSimulation& _region, std::uint8_t _team, Species::StableId _leader,
                           const Neuron::Vec3& _objective, Species::StandingOrder _order, std::uint8_t _other)
{
  const Species::Figure* leader = FigureById(_region, _team, _leader);
  Assert::IsNotNull(leader, L"the Leader lives");
  Assert::IsTrue(Species::InFormationMode(*leader) &&
                   std::hypot(leader->waypoint.x - _objective.x, leader->waypoint.z - _objective.z) < 1.0,
                 L"in formation, walking to the objective");
  const std::vector<Species::DeputyAction>& actions = ActionsOf(_region);
  const auto count = std::ranges::count_if(actions, [_leader](const Species::DeputyAction& _action) { return _action.leader == _leader; });
  ExpectEqual(1, count, L"one deputy action for the Leader, not one each look");
  const auto action = std::ranges::find(actions, _leader, &Species::DeputyAction::leader);
  Assert::IsTrue(action->kind == Species::DeputyActionKind::LeaderToObjective && action->order == _order && action->detail == _other,
                 L"with the standing order that permits it and the other side");
}

} // namespace

TEST_CLASS(DeputyTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp073TheDeputyDefendsTheSettlementOnceItsBattleStarts)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-026")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-043")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp073TheDeputyDefendsTheSettlementOnceItsBattleStarts)
  {
    // Playtest 2's first situation (the M4 plan's table 4): an attack declared on the settlement. During the warning the
    // deputy does nothing; once the battle starts it sends the settlement's Leader to the objective, the Wellspring, once,
    // under "defend the settlement" against the attacker.
    const std::unique_ptr<Species::WorldSimulation> world = AwayValley();
    Species::RegionSimulation& region = world->Region(0);
    Strengthen(region, STEWARD_SITE, DEFENDER_EXTRA);
    const Neuron::Vec3 wellspring = WellspringAt(region, STEWARD_SITE);
    const Species::StableId defender = RaiseLeader(region, STEWARD_SITE, Toward(region, wellspring, WellspringAt(region, 0), 90.0), 6);
    const Species::StableId attacker = RaiseLeader(region, NEIGHBOR_SITE, StagingOf(region, NEIGHBOR_SITE, STEWARD_SITE), 6);
    SettlementAt(region, NEIGHBOR_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, NEIGHBOR_SITE, {attacker, 0, 0}, STEWARD_SITE, 0).has_value(), L"declared");
    StepFor(*world, 2 * DecisionTicks());
    Assert::IsTrue(region.Battles().empty() && ActionsOf(region).empty(), L"no action during the warning");
    Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !region.Battles().empty(); }), L"its battle starts");
    StepFor(*world, 3 * DecisionTicks());
    ExpectSentToObjective(region, STEWARD_SITE, defender, region.Battles().empty() ? wellspring : region.Battles().front().objective,
                          Species::StandingOrder::DefendTheSettlement, NEIGHBOR_SITE);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp073TheDeputyDefendsABorderHalfItsSettlementHolds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp073TheDeputyDefendsABorderHalfItsSettlementHolds)
  {
    // Playtest 2's second situation: a claim battle at a border half the settlement holds (O110). The deputy sends the
    // settlement's Leader to the claimed half's resource site, the battle's objective.
    const std::unique_ptr<Species::WorldSimulation> world = AwayValley();
    Species::RegionSimulation& region = world->Region(0);
    const Species::TuningMvp& tuning = FastTuning();
    for (const std::uint8_t site : {STEWARD_SITE, NEIGHBOR_SITE})
    {
      SettlementAt(region, site).territoryRadius = tuning.wld.territory.maxRadius;
      region.RecomputeTerritory(site);
    }
    Strengthen(region, STEWARD_SITE, DEFENDER_EXTRA);
    const Species::StableId claim = Species::MakeClaim(region, NEIGHBOR_SITE, STEWARDS_HALF).value_or(0);
    Assert::IsTrue(claim != 0, L"the neighbour claims the steward's half");
    const Species::Structure* resourceSite = SiteInHalf(region, STEWARDS_HALF);
    Assert::IsNotNull(resourceSite, L"the half's resource site");
    const Neuron::Vec3 site = resourceSite->position;
    const Species::StableId defender = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 6);
    const Neuron::Vec3 staging = Toward(region, site, WellspringAt(region, NEIGHBOR_SITE),
                                        tuning.wld.resource.wildClearance + 2.0 * tuning.mvp.battle.objectiveRadius);
    const Species::StableId attacker = RaiseLeader(region, NEIGHBOR_SITE, staging, 6);
    SettlementAt(region, NEIGHBOR_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, NEIGHBOR_SITE, {attacker, 0, 0}, 0, claim).has_value(), L"its claim battle declared");
    Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !region.Battles().empty(); }), L"its battle starts");
    const Neuron::Vec3 objective = region.Battles().front().objective;
    Assert::IsTrue(Neuron::HorizontalDistance(objective, site) < 1e-6, L"at the half's resource site");
    StepFor(*world, 3 * DecisionTicks());
    ExpectSentToObjective(region, STEWARD_SITE, defender, objective, Species::StandingOrder::DefendTheSettlement, NEIGHBOR_SITE);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp073TheDeputyFightsTheStewardsOwnExpeditionAndNeverWithdrawsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp073TheDeputyFightsTheStewardsOwnExpeditionAndNeverWithdrawsIt)
  {
    // O110: the steward's raid on site 3 is out when the steward goes away. The deputy sends its Leader to the target's
    // Wellspring under "the steward's attack", and the expedition stays out until its battle ends.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
    Species::RegionSimulation& region = world->Region(0);
    Strengthen(region, NEIGHBOR_SITE, DEFENDER_EXTRA);
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, StagingOf(region, STEWARD_SITE, NEIGHBOR_SITE), 6);
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, NEIGHBOR_SITE, 0).has_value(), L"the steward's raid");
    Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !region.Battles().empty(); }), L"its battle starts");
    // The battle walks the expedition to its objective; the steward turns its Leader back before going away.
    const Neuron::Vec3 objective = region.Battles().front().objective;
    const std::optional<Species::ActorRef> ref = region.FindFigure(Species::ExpeditionTeamOf(STEWARD_SITE), leader);
    Species::Figure* turned = ref ? region.Resolve(*ref) : nullptr;
    Assert::IsNotNull(turned, L"the expedition's Leader");
    if (turned != nullptr)
    {
      Species::AssignWaypoint(*turned, StagingOf(region, STEWARD_SITE, NEIGHBOR_SITE));
    }
    Assert::IsTrue(Species::SetStewardPresence(*world, true), L"the steward goes away");
    StepFor(*world, 3 * DecisionTicks());
    ExpectSentToObjective(region, Species::ExpeditionTeamOf(STEWARD_SITE), leader, objective, Species::StandingOrder::TheStewardsAttack,
                          NEIGHBOR_SITE);
    Assert::IsTrue(Species::ExpeditionOf(region, STEWARD_SITE) != nullptr || !region.BattleResults().empty(),
                   L"the expedition is out until its battle ends");
    Assert::IsTrue(std::ranges::none_of(region.BattleResults(),
                                        [](const Species::BattleResult& _result) { return _result.end == Species::BattleEnd::Withdrawn; }),
                   L"and never withdrawn");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp073TheDeputyLeavesTheQueueAndThePrioritiesAsTheStewardLeftThem)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-079")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp073TheDeputyLeavesTheQueueAndThePrioritiesAsTheStewardLeftThem)
  {
    // Playtest 2's third and fourth situations: the stores below a structure's cost, and the population at its limit with
    // a dwelling queued. The deputy takes no action: the dwelling, paid when it was placed, is built in its order by the
    // Workers whose priority is construction (finish the queued construction), and the priorities and the growth hold
    // stay as the steward left them.
    const std::unique_ptr<Species::WorldSimulation> world = AwayValley();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = SettlementAt(region, STEWARD_SITE);
    Strengthen(region, STEWARD_SITE, Species::PopulationLimitOf(region, settlement) - Species::Population(region, settlement));
    ExpectEqual(Species::PopulationLimitOf(region, settlement), Species::Population(region, settlement), L"at the population limit");
    settlement.storesMilliRu = Species::MilliRuOf(1'000.0);
    const Neuron::Vec3 place = Toward(region, WellspringAt(region, STEWARD_SITE), WellspringAt(region, 0), 120.0);
    const auto placed = Species::PlaceStructure(region, STEWARD_SITE, Species::StructureKind::Dwelling, place.x, place.z);
    Assert::IsTrue(placed.has_value(), L"a dwelling queued");
    settlement.storesMilliRu = 0;
    settlement.priorities = {50, 30, 20};
    settlement.growthHeld = true;
    const std::int64_t work = region.Structures().At(*placed).workTicks;
    StepFor(*world, 6 * DecisionTicks());
    Assert::IsTrue(ActionsOf(region).empty(), L"no deputy action");
    Assert::IsTrue(region.Structures().At(*placed).workTicks > work || !Species::Queued(region, region.Structures().At(*placed)),
                   L"the dwelling is built by the Workers");
    Assert::IsTrue(settlement.priorities == std::array<std::uint8_t, 3>{50, 30, 20} && settlement.growthHeld,
                   L"the priorities and the hold as the steward left them");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp073TheDeputyNeverAttacks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-079")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp073TheDeputyNeverAttacks)
  {
    // Playtest 2's fifth situation: a neighbour's respite ends while the steward's Cladan is angry with it. The deputy, with
    // a Leader at hand and stores to pay, declares nothing and takes no action (never attack).
    const std::unique_ptr<Species::WorldSimulation> world = AwayValley();
    Species::RegionSimulation& region = world->Region(0);
    for (std::uint64_t subject = 1; subject <= 5; ++subject)
    {
      Species::AddGrievance(region, STEWARD_SITE, NEIGHBOR_SITE, Species::GrievanceCause::Attack, subject, true);
    }
    region.Respites().push_back({NEIGHBOR_SITE, region.NextTick() + 10});
    static_cast<void>(RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 6));
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    StepFor(*world, 6 * DecisionTicks());
    Assert::IsTrue(Species::ExpeditionOf(region, STEWARD_SITE) == nullptr && ActionsOf(region).empty(), L"no attack and no action");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil071TheDeputyCommandsOnlyWhileTheStewardIsAway)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil071TheDeputyCommandsOnlyWhileTheStewardIsAway)
  {
    // ADR-186: the deputy is made at the first absence and kept; it may command its settlement's team and its
    // expedition's only while the steward is away, and no other team; it saves and restores, and a region of version 19
    // reads with none.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsTrue(region.Deputies().empty(), L"no deputy before the first absence");
    Assert::IsTrue(Species::SetStewardPresence(*world, true), L"away");
    const Species::Deputy* deputy = Species::DeputyOf(region, STEWARD_SITE);
    Assert::IsNotNull(deputy, L"a deputy");
    const Species::StableId id = deputy->id;
    Assert::IsTrue(region.MayCommand(id, STEWARD_SITE, true) && region.MayCommand(id, Species::ExpeditionTeamOf(STEWARD_SITE), true),
                   L"its settlement's team and its expedition's, as an AI, while away");
    Assert::IsFalse(region.MayCommand(id, NEIGHBOR_SITE, false), L"no other team");
    Assert::IsTrue(Species::SetStewardPresence(*world, false), L"present again");
    Assert::IsFalse(region.MayCommand(id, STEWARD_SITE, true), L"nothing while the steward is present");
    Assert::IsTrue(Species::SetStewardPresence(*world, true) && Species::DeputyOf(region, STEWARD_SITE)->id == id,
                   L"the same deputy at the next absence");

    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), FastTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value() && (*restored)->Region(0).Deputies() == region.Deputies(), L"the deputies save and restore");
    Species::RegionSimulation older(0, SETTLED_TEST_KEY, region.NextTick(), FastTuning(), ValleyTerrain(), &TestRecords());
    const std::vector<std::byte> state = RegionStateOf(region, 19);
    Neuron::BitReader reader(state);
    Assert::IsTrue(older.ReadState(reader, 19) && older.Deputies().empty(), L"a version-19 region reads with none");
  }
};

} // namespace GameLogicTests
