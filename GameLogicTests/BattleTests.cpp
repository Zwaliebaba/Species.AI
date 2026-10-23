#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "Construction.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "Relations.h"
#include "Settlement.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "Territory.h"
#include "Upkeep.h"
#include "ValleyAtWar.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, valley neighbour of site 3 (pair 1)
constexpr std::uint8_t TARGET_SITE = 3;
constexpr std::uint8_t EXPEDITION_TEAM = Species::ExpeditionTeamOf(STEWARD_SITE);
constexpr std::uint8_t NEIGHBORS_HALF = 3; // pair 1's half nearer site 3
constexpr std::uint8_t WILD_TEAM = 9;      // a team that is no settlement's nor its expedition's
constexpr Neuron::SenderId STEWARD = 7;
// The Workers the defender is given beyond its first 20, so that its loss cap is not 0 (MVP-046).
constexpr std::uint32_t DEFENDER_EXTRA = 20;

// The committed tuning with the warning, the battle's longest time and an expedition's size shortened, so that the
// tests' battles start and end within a few hundred ticks.
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

// FastTuning with one battle in progress at most (MVP-015).
const Species::TuningMvp& OneBattleTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = FastTuning();
    tuning.wld.scale.battlesPerRegionMax = 1;
    return tuning;
  }();
  return TUNING;
}

std::uint64_t TicksOf(double _seconds)
{
  return static_cast<std::uint64_t>(std::llround(_seconds / CommittedTuning().ref.sim.tickPeriod));
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

// The point _distance from _from towards _toward, on the ground.
Neuron::Vec3 Toward(const Species::RegionSimulation& _region, const Neuron::Vec3& _from, const Neuron::Vec3& _toward, double _distance)
{
  const Neuron::Vec3 across{_toward.x - _from.x, 0.0, _toward.z - _from.z};
  Neuron::Vec3 point = _from + across * (_distance / Neuron::Length(across));
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

// Where the warning holds an expedition of _attacker's that raids _target: the objective radius beyond its territory.
Neuron::Vec3 StagingOf(const Species::RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _target)
{
  return Toward(_region, WellspringAt(_region, _target), WellspringAt(_region, _attacker),
                _region.SettlementAt(_target)->territoryRadius + _region.Tuning().mvp.battle.objectiveRadius);
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

// The living figures of _team, or only its Workers.
std::uint32_t LivingOf(const Species::RegionSimulation& _region, std::uint8_t _team, bool _workersOnly = false)
{
  if (_team >= _region.TeamCount())
  {
    return 0;
  }
  std::uint32_t living = 0;
  const Species::SlicedPool<Species::Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    living +=
      figures.Used(slot) && !figures.At(slot).dead && (!_workersOnly || figures.At(slot).kind == Species::FigureKind::Worker) ? 1 : 0;
  }
  return living;
}

const Species::Grievance* AttackGrievance(const Species::RegionSimulation& _region, Species::StableId _expedition)
{
  const auto found =
    std::ranges::find_if(_region.Grievances(), [&](const Species::Grievance& _grievance)
                         { return _grievance.cause == Species::GrievanceCause::Attack && _grievance.subject == _expedition; });
  return found != _region.Grievances().end() ? &*found : nullptr;
}

// Steps _world from its region's next tick until _until holds after a step, for _ticks at most; true when it held.
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

// A valley on FastTuning whose steward's expedition of one Leader and _followers Workers raids site 3, whose defender has
// DEFENDER_EXTRA more Workers, stepped until its battle starts.
std::unique_ptr<Species::WorldSimulation> RaidUnderway(std::uint32_t _followers = 6, const Species::TuningMvp& _tuning = FastTuning())
{
  std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, _tuning);
  Species::RegionSimulation& region = world->Region(0);
  Strengthen(region, TARGET_SITE, DEFENDER_EXTRA);
  const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, StagingOf(region, STEWARD_SITE, TARGET_SITE), _followers);
  SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
  const auto declared = Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, 0);
  Assert::IsTrue(declared.has_value(), L"declared");
  Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !world->Region(0).Battles().empty(); }), L"its battle starts");
  return world;
}

// The stable ids of the living Workers of _team.
std::vector<Species::StableId> WorkersOf(const Species::RegionSimulation& _region, std::uint8_t _team)
{
  std::vector<Species::StableId> workers;
  const Species::SlicedPool<Species::Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && !figures.At(slot).dead && figures.At(slot).kind == Species::FigureKind::Worker)
    {
      workers.push_back(figures.At(slot).id);
    }
  }
  return workers;
}

// The resource site in the half _area of a valley border area, or null.
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

// The ticks of _minutes of world time.
std::uint64_t MinuteTicks(double _minutes)
{
  return TicksOf(_minutes * 60.0);
}

// One step of the battles alone, at the region's next tick.
void AdvanceBattlesOnce(Species::RegionSimulation& _region)
{
  Species::AdvanceBattles(_region, _region.NextTick());
}

} // namespace

TEST_CLASS(BattleTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp042DeclaringAnAttackPaysItsCostWarnsAndSendsItsExpedition)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-051")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp042DeclaringAnAttackPaysItsCostWarnsAndSendsItsExpedition)
  {
    // ADR-161: a declaration is refused for each of its reasons in order; one that stands pays the attack's cost, twice
    // over with no cause on record, gives the target's Cladan the attack's grievance, moves the Leader and its Workers to
    // the expedition's team under the settlement's commander, and walks them in formation to the edge of the target's
    // territory.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::TuningMvp& tuning = CommittedTuning();
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 6);
    const std::array<Species::StableId, Species::EXPEDITION_LEADERS_MAX> one{leader, 0, 0};
    const auto refusal = [&](std::uint8_t _attacker, const std::array<Species::StableId, Species::EXPEDITION_LEADERS_MAX>& _leaders,
                             std::uint8_t _target, Species::StableId _claim) -> std::optional<Species::AttackRefusal>
    {
      const auto declared = Species::DeclareAttack(region, _attacker, _leaders, _target, _claim);
      return declared ? std::nullopt : std::optional(declared.error());
    };
    Assert::IsTrue(refusal(WILD_TEAM, one, TARGET_SITE, 0) == Species::AttackRefusal::NoSettlement, L"no settlement's");
    Assert::IsTrue(refusal(STEWARD_SITE, one, STEWARD_SITE, 0) == Species::AttackRefusal::NoTarget, L"not on itself");
    Assert::IsTrue(refusal(STEWARD_SITE, one, TARGET_SITE, 12'345) == Species::AttackRefusal::NoTarget, L"nor for a claim it lacks");
    region.Respites().push_back({TARGET_SITE, region.NextTick() + 1});
    Assert::IsTrue(refusal(STEWARD_SITE, one, TARGET_SITE, 0) == Species::AttackRefusal::UnderRespite, L"nor under a respite");
    region.Respites().clear();
    Assert::IsTrue(refusal(STEWARD_SITE, {12'345, 0, 0}, TARGET_SITE, 0) == Species::AttackRefusal::NoLeader,
                   L"nor with no Leader of its own");
    const auto sizeMax = static_cast<std::uint32_t>(tuning.mvp.expedition.sizeMax);
    const Species::StableId crowd = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), sizeMax - 5);
    Assert::IsTrue(refusal(STEWARD_SITE, {leader, crowd, 0}, TARGET_SITE, 0) == Species::AttackRefusal::TooLarge,
                   L"nor with more Workers than an expedition holds, across its Leaders");
    Species::Settlement& home = SettlementAt(region, STEWARD_SITE);
    const std::int64_t unprovokedCost = Species::MilliRuOf(tuning.mvp.conflict.attackCostRu * tuning.pil.conflict.unprovokedCostFactor);
    home.storesMilliRu = unprovokedCost - 1;
    Assert::IsTrue(refusal(STEWARD_SITE, one, TARGET_SITE, 0) == Species::AttackRefusal::CannotAfford, L"nor beyond its stores");
    Assert::IsTrue(region.Expeditions().empty() && region.Grievances().empty() && home.storesMilliRu == unprovokedCost - 1,
                   L"a refused declaration has no effect");

    home.storesMilliRu = Species::MilliRuOf(10'000.0);
    const Species::RegionSimulation::Commander commander = region.CommanderOf(STEWARD_SITE);
    const auto declared = Species::DeclareAttack(region, STEWARD_SITE, one, TARGET_SITE, 0);
    Assert::IsTrue(declared.has_value(), L"declared");
    const Species::StableId id = declared.value_or(0);
    Assert::IsTrue(home.storesMilliRu == Species::MilliRuOf(10'000.0) - unprovokedCost, L"an unprovoked attack costs twice");
    const Species::Expedition& expedition = region.Expeditions().front();
    Assert::IsTrue(expedition.id == id && expedition.attacker == STEWARD_SITE && expedition.target == TARGET_SITE &&
                     expedition.claim == 0 && expedition.sent == 6 && expedition.unprovoked &&
                     expedition.state == Species::ExpeditionState::Held && expedition.declaredTick == region.NextTick() &&
                     expedition.leaders == one,
                   L"the expedition, held by the warning");
    const Species::Grievance* grievance = AttackGrievance(region, id);
    Assert::IsTrue(grievance != nullptr && grievance->holder == TARGET_SITE && grievance->against == STEWARD_SITE &&
                     grievance->unprovoked && grievance->weightMicro == expedition.grievanceMicro && expedition.grievanceMicro > 0,
                   L"the target's grievance, its weight recorded");
    ExpectEqual(7, LivingOf(region, EXPEDITION_TEAM), L"the Leader and its six Workers left for the expedition's team");
    Assert::IsFalse(region.FindFigure(STEWARD_SITE, leader).has_value(), L"and the settlement's");
    const std::optional<Species::ActorRef> moved = region.FindFigure(EXPEDITION_TEAM, leader);
    Assert::IsTrue(moved.has_value(), L"the Leader is the expedition's");
    const Species::SlicedPool<Species::Figure>& members = region.TeamAt(EXPEDITION_TEAM).figures;
    for (std::uint32_t slot = 0; slot < members.Capacity(); ++slot)
    {
      if (members.Used(slot) && members.At(slot).kind == Species::FigureKind::Worker)
      {
        const Species::Figure* followed = region.LeaderOf(members.At(slot), EXPEDITION_TEAM);
        Assert::IsTrue(followed != nullptr && followed->id == leader && members.At(slot).task == Species::WorkerTask::None,
                       L"each Worker still follows it, with no task");
      }
    }
    Assert::IsTrue(region.CommanderOf(EXPEDITION_TEAM).sender == commander.sender && region.CommanderOf(EXPEDITION_TEAM).ai == commander.ai,
                   L"commanded by the settlement's commander");
    const Species::Figure& walking = *region.Resolve(moved.value_or(Species::ActorRef{}));
    Assert::IsTrue(walking.order == Species::LeaderOrder::Follow && walking.formation &&
                     walking.leaderState == Species::LeaderState::ToWaypoint,
                   L"walking in formation");
    Assert::IsTrue(Neuron::HorizontalDistance(walking.waypoint, StagingOf(region, STEWARD_SITE, TARGET_SITE)) < 1e-6 &&
                     !region.TerritoryOf(TARGET_SITE)->Holds(walking.waypoint.x, walking.waypoint.z),
                   L"to the edge of the target's territory");
    Assert::IsTrue(refusal(STEWARD_SITE, {crowd, 0, 0}, TARGET_SITE, 0) == Species::AttackRefusal::Underway, L"one expedition at a time");
    Assert::IsFalse(region.EnemiesOf(EXPEDITION_TEAM).test(TARGET_SITE) || region.EnemiesOf(TARGET_SITE).test(EXPEDITION_TEAM),
                    L"at peace with its target until its battle");
    Assert::IsTrue(region.EnemiesOf(EXPEDITION_TEAM).test(WILD_TEAM), L"and an enemy of a team that is no settlement's");

    // With a cause on record the attack costs mvp.conflict.attack_cost_ru once.
    const std::unique_ptr<Species::WorldSimulation> provoked = ValleyWorld();
    Species::RegionSimulation& other = provoked->Region(0);
    const Species::StableId second = RaiseLeader(other, STEWARD_SITE, WellspringAt(other, STEWARD_SITE), 2);
    Species::AddGrievance(other, STEWARD_SITE, TARGET_SITE, Species::GrievanceCause::Claim, 1);
    SettlementAt(other, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(other, STEWARD_SITE, {second, 0, 0}, TARGET_SITE, 0).has_value() &&
                     !other.Expeditions().front().unprovoked &&
                     SettlementAt(other, STEWARD_SITE).storesMilliRu == Species::MilliRuOf(10'000.0 - tuning.mvp.conflict.attackCostRu),
                   L"a provoked attack costs once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp043ThreeLeadersAreHeldByTheWarningThenMoveAndFightAsOne)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp043ThreeLeadersAreHeldByTheWarningThenMoveAndFightAsOne)
  {
    // O98, ADR-161: an expedition of three Leaders is held at the territory's edge until the warning ends, then each
    // walks to the objective; its battle starts when it enters the territory, the two sides become enemies, and the
    // battle ends in a result within the loss cap's bounds, the expedition home.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
    Species::RegionSimulation& region = world->Region(0);
    const Species::TuningMvp& tuning = FastTuning();
    Strengthen(region, TARGET_SITE, DEFENDER_EXTRA);
    const Neuron::Vec3 staging = StagingOf(region, STEWARD_SITE, TARGET_SITE);
    const Neuron::Vec3 objective = WellspringAt(region, TARGET_SITE);
    std::array<Species::StableId, Species::EXPEDITION_LEADERS_MAX> leaders{};
    for (std::size_t place = 0; place < leaders.size(); ++place)
    {
      leaders[place] = RaiseLeader(region, STEWARD_SITE, Toward(region, staging, objective, -40.0 * static_cast<double>(place)), 3);
    }
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    const std::int64_t storesAtStart = region.SettlementAt(TARGET_SITE)->storesMilliRu;
    const auto declared = Species::DeclareAttack(region, STEWARD_SITE, leaders, TARGET_SITE, 0);
    Assert::IsTrue(declared.has_value() && region.Expeditions().front().sent == 9, L"three Leaders and their nine Workers, as one");
    const std::uint64_t release = region.NextTick() + TicksOf(tuning.mvp.conflict.warningMinutes * 60.0);
    const auto leadersOf = [&]
    {
      std::vector<const Species::Figure*> found;
      for (const Species::StableId id : leaders)
      {
        const std::optional<Species::ActorRef> ref = region.FindFigure(EXPEDITION_TEAM, id);
        found.push_back(ref ? region.Resolve(*ref) : nullptr);
      }
      return found;
    };
    Assert::IsFalse(
      StepUntil(*world, release - region.NextTick(),
                [&] { return region.Expeditions().front().state != Species::ExpeditionState::Held || !region.Battles().empty(); }),
      L"held, with no battle, until the warning ends");
    Assert::IsTrue(StepUntil(*world, 1, [&] { return region.Expeditions().front().state == Species::ExpeditionState::Released; }),
                   L"released when it ends");
    std::vector<double> distances;
    for (const Species::Figure* leader : leadersOf())
    {
      Assert::IsTrue(leader != nullptr && Neuron::HorizontalDistance(leader->waypoint, objective) < 1e-6,
                     L"each Leader walks to the objective");
      distances.push_back(Neuron::HorizontalDistance(leader->position, objective));
    }
    Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !region.Battles().empty(); }), L"its battle starts as it enters");
    const Species::Battle battle = region.Battles().front();
    Assert::IsTrue(battle.attacker == STEWARD_SITE && battle.defender == TARGET_SITE && battle.claim == 0 &&
                     battle.startTick + 1 == region.NextTick() && Neuron::HorizontalDistance(battle.objective, objective) < 1e-6,
                   L"a raid on the target's Wellspring");
    const double territory = region.SettlementAt(TARGET_SITE)->territoryRadius;
    Assert::IsTrue(std::abs(battle.fieldRadius - territory - tuning.wld.view.settlementMargin) < 1e-6 &&
                     battle.defenderStart == Species::Population(region, *region.SettlementAt(TARGET_SITE)) &&
                     battle.defenderStart == 20 + DEFENDER_EXTRA,
                   L"its battlefield and its defender at the start");
    const std::vector<const Species::Figure*> marching = leadersOf();
    for (std::size_t place = 0; place < marching.size(); ++place)
    {
      Assert::IsTrue(marching[place] != nullptr && Neuron::HorizontalDistance(marching[place]->position, objective) < distances[place],
                     L"every Leader has closed on the objective");
    }
    Assert::IsTrue(region.EnemiesOf(EXPEDITION_TEAM).test(TARGET_SITE) && region.EnemiesOf(TARGET_SITE).test(EXPEDITION_TEAM),
                   L"the two sides are enemies in their battle");
    Assert::IsFalse(region.EnemiesOf(STEWARD_SITE).test(TARGET_SITE) || region.EnemiesOf(EXPEDITION_TEAM).test(0) ||
                      region.EnemiesOf(TARGET_SITE).test(1),
                    L"and every other pair is at peace");

    Assert::IsTrue(StepUntil(*world, TicksOf(tuning.pil.battle.maxMinutes * 60.0) + 1, [&] { return region.Battles().empty(); }),
                   L"the battle ends");
    Assert::IsTrue(region.BattleResults().size() == 1 && region.Expeditions().empty(), L"in a result, the expedition over");
    const Species::BattleResult& result = region.BattleResults().front();
    Assert::IsTrue(result.expedition == declared.value_or(0) && result.attacker == STEWARD_SITE && result.defender == TARGET_SITE &&
                     result.startTick == battle.startTick && result.grievanceMicro > 0,
                   L"its sides and its grievance");
    Assert::IsTrue(result.defenderLost <= Species::LossCapOf(region, battle) &&
                     result.storesMovedMilliRu <= static_cast<std::int64_t>(std::floor(tuning.mvp.protection.lossCapStoreShare *
                                                                                       static_cast<double>(storesAtStart))) &&
                     result.essenceMoved <= static_cast<std::uint32_t>(std::floor(tuning.mvp.protection.lossCapWorkerShare * 9.0)),
                   L"every transfer within the loss cap's bounds");
    ExpectEqual(0, LivingOf(region, EXPEDITION_TEAM), L"its survivors home");
  }
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp045ABattleEndsAtTheFirstOfItsEndsAndItsExpeditionGoesHome)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-045")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp045ABattleEndsAtTheFirstOfItsEndsAndItsExpeditionGoesHome)
  {
    // ADR-161: the loss cap, the objective held, the rout and the time end a battle in that order, and a withdrawal by
    // command ends it as the attacker's loss; a won raid moves the capped stores, carried home by its Workers; every end
    // records its result, removes the attack's grievance's cause and returns the expedition to its settlement.
    const Species::TuningMvp& tuning = FastTuning();
    const auto holdTicks = static_cast<std::int32_t>(TicksOf(tuning.mvp.battle.objectiveHoldS));
    {
      // The loss cap, which comes before the objective held, and the capped stores carried home.
      const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      Species::Battle& battle = region.Battles().front();
      const Species::StableId expedition = battle.expedition;
      battle.defenderStoresMilliRu = Species::MilliRuOf(4'000.0);
      SettlementAt(region, TARGET_SITE).storesMilliRu = Species::MilliRuOf(3'500.0);
      battle.defenderLost = Species::LossCapOf(region, battle);
      battle.heldTicks = holdTicks;
      const std::vector<Species::StableId> carriers = WorkersOf(region, EXPEDITION_TEAM);
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.Battles().empty() && region.BattleResults().size() == 1, L"the battle ends");
      const Species::BattleResult& result = region.BattleResults().front();
      Assert::IsTrue(result.end == Species::BattleEnd::LossCap && result.attackerWon && result.endTick == region.NextTick(),
                     L"at the loss cap first, the attacker's win");
      const std::int64_t moved = Species::MilliRuOf(1'000.0); // mvp.protection.loss_cap_store_share of the stores at the start
      Assert::IsTrue(result.storesMovedMilliRu == moved && region.SettlementAt(TARGET_SITE)->storesMilliRu == Species::MilliRuOf(2'500.0),
                     L"the capped share of the stores at its start moves");
      std::int64_t carried = 0;
      for (const Species::StableId id : carriers)
      {
        const std::optional<Species::ActorRef> home = region.FindFigure(STEWARD_SITE, id);
        Assert::IsTrue(home.has_value(), L"each Worker is its settlement's again");
        carried += home ? region.Resolve(*home)->carriedMilliRu : 0;
      }
      Assert::IsTrue(carried == moved, L"carried home by its Workers");
      Assert::IsTrue(region.Expeditions().empty() && LivingOf(region, EXPEDITION_TEAM) == 0 &&
                       AttackGrievance(region, expedition)->causeRemoved,
                     L"the expedition over, its grievance's cause removed");
    }
    {
      // The objective held: the attacker's Workers near the objective with no defender there, for its hold.
      const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      Species::Battle& battle = region.Battles().front();
      const std::optional<Species::ActorRef> worker = region.FindFigure(EXPEDITION_TEAM, WorkersOf(region, EXPEDITION_TEAM).front());
      battle.objective = region.Resolve(worker.value_or(Species::ActorRef{}))->position;
      Species::RegionSimulation::Spawn spawn;
      spawn.team = TARGET_SITE;
      spawn.position = battle.objective;
      const std::uint32_t blocker = region.SpawnFigures(spawn).front();
      battle.heldTicks = holdTicks - 1;
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.Battles().size() == 1 && region.Battles().front().heldTicks == 0, L"a defender at the objective holds it");
      region.TeamAt(TARGET_SITE).figures.At(blocker).dead = true;
      region.Battles().front().heldTicks = holdTicks - 1;
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.Battles().empty() && region.BattleResults().front().end == Species::BattleEnd::ObjectiveHeld &&
                       region.BattleResults().front().attackerWon,
                     L"with none there, the attacker holds it and wins");
    }
    {
      // The rout, which comes before the time.
      const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      const auto workers = static_cast<std::uint32_t>(WorkersOf(region, EXPEDITION_TEAM).size());
      region.Expeditions().front().sent =
        static_cast<std::uint32_t>(std::ceil(static_cast<double>(workers) / tuning.mvp.battle.routShare)) + 1;
      region.Battles().front().startTick = region.NextTick() - MinuteTicks(tuning.pil.battle.maxMinutes);
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.BattleResults().front().end == Species::BattleEnd::Rout && !region.BattleResults().front().attackerWon &&
                       region.BattleResults().front().storesMovedMilliRu == 0,
                     L"too few of the Workers it sent: a rout, the defender's win");
    }
    {
      // The time.
      const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      region.Battles().front().startTick = region.NextTick() - MinuteTicks(tuning.pil.battle.maxMinutes) + 1;
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.Battles().size() == 1, L"not before pil.battle.max_minutes");
      region.Battles().front().startTick -= 1;
      AdvanceBattlesOnce(region);
      Assert::IsTrue(region.BattleResults().front().end == Species::BattleEnd::Time && !region.BattleResults().front().attackerWon,
                     L"then the time ends it, the defender's win");
    }
    {
      // The withdrawal in the battle, and the refusals of a withdrawal.
      const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      Assert::IsTrue(Species::WithdrawAttack(region, WILD_TEAM) == Species::AttackRefusal::NoSettlement &&
                       Species::WithdrawAttack(region, 0) == Species::AttackRefusal::Underway,
                     L"no settlement's, nor one with no expedition");
      Assert::IsFalse(Species::WithdrawAttack(region, STEWARD_SITE).has_value(), L"withdrawn");
      Assert::IsTrue(region.Battles().empty() && region.BattleResults().front().end == Species::BattleEnd::Withdrawn &&
                       !region.BattleResults().front().attackerWon && LivingOf(region, EXPEDITION_TEAM) == 0,
                     L"a withdrawal in the battle is the defender's win, the expedition home");
    }
    {
      // A withdrawal before the battle ends the attack with no battle and no result.
      const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
      Species::RegionSimulation& region = world->Region(0);
      const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 3);
      SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
      const Species::StableId expedition = Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, 0).value_or(0);
      Assert::IsFalse(Species::WithdrawAttack(region, STEWARD_SITE).has_value(), L"withdrawn while held");
      Assert::IsTrue(region.Expeditions().empty() && region.BattleResults().empty() && region.Respites().empty() &&
                       region.FindFigure(STEWARD_SITE, leader).has_value() && LivingOf(region, EXPEDITION_TEAM) == 0 &&
                       AttackGrievance(region, expedition)->causeRemoved,
                     L"no battle, no result and no respite; its Leader home and its grievance's cause removed");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp046TheDefendersAdvantageAndTheLossCapBoundWhatABattleTakes)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-044")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-045")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp046TheDefendersAdvantageAndTheLossCapBoundWhatABattleTakes)
  {
    // ADR-161: a defender inside its own territory takes mvp.defense.damage_taken_factor of the damage, and none once its
    // losses reach the loss cap, which keeps it above the worker floor; an expedition's Essence that the defender's
    // territory takes counts up to the attacker's own share of the Workers it sent, and ascends beyond it.
    const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
    Species::RegionSimulation& region = world->Region(0);
    const Species::TuningMvp& tuning = FastTuning();
    Species::Battle& battle = region.Battles().front();
    const auto workerFloor = static_cast<std::uint32_t>(tuning.mvp.protection.workerFloor);
    Species::Battle sized = battle;
    for (const auto& [start, cap] : {std::pair{workerFloor, 0U}, std::pair{workerFloor + 5, 5U}, std::pair{100U, 30U}})
    {
      sized.defenderStart = start;
      ExpectEqual(cap, Species::LossCapOf(region, sized), L"a share of its Workers, above the worker floor");
    }
    const Neuron::Vec3 inside = WellspringAt(region, TARGET_SITE);
    const Neuron::Vec3 outside = StagingOf(region, STEWARD_SITE, TARGET_SITE);
    const auto taken = static_cast<std::int32_t>(std::llround(-100.0 * tuning.mvp.defense.damageTakenFactor));
    ExpectEqual(taken, Species::BattleDamage(region, TARGET_SITE, inside, -100), L"the defender inside its territory");
    ExpectEqual(-100, Species::BattleDamage(region, TARGET_SITE, outside, -100), L"the defender outside it");
    ExpectEqual(-100, Species::BattleDamage(region, EXPEDITION_TEAM, inside, -100), L"the attacker");
    ExpectEqual(-100, Species::BattleDamage(region, 0, inside, -100), L"a settlement in no battle");

    Assert::IsTrue(Species::BattleDeath(region, TARGET_SITE, inside), L"a defender's death leaves its orb");
    ExpectEqual(1, battle.defenderLost, L"and counts");
    battle.defenderLost = Species::LossCapOf(region, battle);
    ExpectEqual(0, Species::BattleDamage(region, TARGET_SITE, inside, -100), L"none once its losses reach the cap");
    ExpectEqual(0, Species::BattleDamage(region, TARGET_SITE, outside, -100), L"wherever it stands");

    const auto bound = static_cast<std::uint32_t>(
      std::floor(tuning.mvp.protection.lossCapWorkerShare * static_cast<double>(region.Expeditions().front().sent)));
    Assert::IsTrue(bound > 0 && Species::BattleDeath(region, EXPEDITION_TEAM, outside), L"an attacker's death outside the territory");
    Assert::IsTrue(battle.attackerLost == 1 && battle.essenceMoved == 0, L"counts no Essence moved");
    for (std::uint32_t death = 0; death < bound; ++death)
    {
      Assert::IsTrue(Species::BattleDeath(region, EXPEDITION_TEAM, inside), L"inside it, its Essence drifts to the defender");
    }
    Assert::IsFalse(Species::BattleDeath(region, EXPEDITION_TEAM, inside), L"beyond the bound it ascends");
    Assert::IsTrue(battle.essenceMoved == bound && battle.attackerLost == bound + 2, L"every death counted, the Essence bounded");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp047ARespiteFollowsALossOfStrengthInALongEnoughBattle)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-047")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp047ARespiteFollowsALossOfStrengthInALongEnoughBattle)
  {
    // ADR-161: a defender that lost mvp.protection.respite_min_loss_share of its strength in a battle that lasted
    // pil.battle.min_respite_minutes starts a respite, in which no attack may be declared on it; a shorter battle, or a
    // smaller loss, starts none.
    const Species::TuningMvp& tuning = FastTuning();
    const auto battleOf = [&](bool _lost, bool _long)
    {
      std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
      Species::RegionSimulation& region = world->Region(0);
      Species::Battle& battle = region.Battles().front();
      const std::int64_t strength = Species::StrengthMilliRu(region, *region.SettlementAt(TARGET_SITE));
      battle.defenderStrengthMilliRu = _lost ? strength * 2 : strength;
      battle.startTick = region.NextTick() - MinuteTicks(tuning.pil.battle.minRespiteMinutes) + (_long ? 0U : 1U);
      region.Expeditions().front().sent = 1'000; // a rout ends it
      AdvanceBattlesOnce(region);
      return world;
    };
    const std::unique_ptr<Species::WorldSimulation> world = battleOf(true, true);
    Species::RegionSimulation& region = world->Region(0);
    const std::uint64_t until = region.NextTick() + MinuteTicks(tuning.mvp.protection.respiteMinutes);
    Assert::IsTrue(region.BattleResults().front().respiteUntil == until && region.Respites().size() == 1 &&
                     region.Respites().front() == Species::Respite{TARGET_SITE, until},
                   L"a respite after its loss");
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 1);
    Assert::IsTrue(
      Species::UnderRespite(region, TARGET_SITE) &&
        Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, 0).error_or(Species::AttackRefusal::Underway) ==
          Species::AttackRefusal::UnderRespite,
      L"no attack may be declared on it");
    region.Respites().front().until = region.NextTick();
    Assert::IsFalse(Species::UnderRespite(region, TARGET_SITE), L"until it ends");
    Assert::IsTrue(battleOf(true, false)->Region(0).Respites().empty() && battleOf(false, true)->Region(0).Respites().empty(),
                   L"none after a shorter battle or a smaller loss");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp015AnAttackBeyondTheBattleCapWaitsForABattleToEnd)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-015")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-043")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp015AnAttackBeyondTheBattleCapWaitsForABattleToEnd)
  {
    // ADR-161: with wld.scale.battles_per_region_max battles in progress, an expedition that enters its target's
    // territory waits, and starts its battle once one ends.
    const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway(6, OneBattleTuning());
    Species::RegionSimulation& region = world->Region(0);
    constexpr std::uint8_t SECOND = 1;
    const Species::StableId leader = RaiseLeader(region, SECOND, StagingOf(region, SECOND, TARGET_SITE), 4);
    SettlementAt(region, SECOND).storesMilliRu = Species::MilliRuOf(10'000.0);
    const Species::StableId waiting = Species::DeclareAttack(region, SECOND, {leader, 0, 0}, TARGET_SITE, 0).value_or(0);
    const std::uint8_t team = Species::ExpeditionTeamOf(SECOND);
    const auto entered = [&]
    {
      const Species::SlicedPool<Species::Figure>& figures = region.TeamAt(team).figures;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (figures.Used(slot) && !figures.At(slot).dead &&
            region.TerritoryOf(TARGET_SITE)->Holds(figures.At(slot).position.x, figures.At(slot).position.z))
        {
          return true;
        }
      }
      return false;
    };
    Assert::IsTrue(waiting != 0 && StepUntil(*world, 1'200, entered), L"the second expedition enters");
    Assert::IsTrue(region.Battles().size() == 1 && region.Battles().front().expedition != waiting &&
                     region.Expeditions().back().state == Species::ExpeditionState::Released,
                   L"and waits while the first battle is in progress");
    Assert::IsFalse(region.EnemiesOf(team).test(TARGET_SITE), L"at peace with the target while it waits");
    region.Battles().front().startTick = region.NextTick() - MinuteTicks(FastTuning().pil.battle.maxMinutes);
    AdvanceBattlesOnce(region);
    Assert::IsTrue(region.Battles().empty(), L"the first battle ends");
    AdvanceBattlesOnce(region);
    Assert::IsTrue(region.Battles().size() == 1 && region.Battles().front().expedition == waiting, L"and the waiting one starts");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053AClaimBattleIsFoughtWithTheHolderAndMovesTheArea)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-045")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053AClaimBattleIsFoughtWithTheHolderAndMovesTheArea)
  {
    // ADR-161: an attack for an open claim targets the area's holder at the cost of a provoked attack; its battle starts
    // in the claimed half, round its resource site, and a win moves the half. An attack for a waiting claim is refused.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
    Species::RegionSimulation& region = world->Region(0);
    const Species::TuningMvp& tuning = FastTuning();
    for (const std::uint8_t site : {STEWARD_SITE, TARGET_SITE})
    {
      SettlementAt(region, site).territoryRadius = tuning.wld.territory.maxRadius;
      region.RecomputeTerritory(site);
    }
    Strengthen(region, TARGET_SITE, DEFENDER_EXTRA);
    const Species::StableId claim = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    const Species::StableId central = Species::MakeClaim(region, 0, Species::CLAIM_AREA_CENTRAL).value_or(0);
    Assert::IsTrue(claim != 0 && central != 0, L"an open claim and a waiting one");
    Assert::IsTrue(Species::DeclareAttack(region, 0, {1, 0, 0}, 1, central).error_or(Species::AttackRefusal::Underway) ==
                     Species::AttackRefusal::NoTarget,
                   L"no attack for a waiting claim");
    const Species::Structure* resourceSite = SiteInHalf(region, NEIGHBORS_HALF);
    Assert::IsNotNull(resourceSite, L"the half's resource site");
    const Neuron::Vec3 site = resourceSite != nullptr ? resourceSite->position : Neuron::Vec3{};
    const Neuron::Vec3 staging =
      Toward(region, site, WellspringAt(region, STEWARD_SITE), tuning.wld.resource.wildClearance + 2.0 * tuning.mvp.battle.objectiveRadius);
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, staging, 6);
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    Assert::IsTrue(Species::DeclareAttack(region, STEWARD_SITE, {leader, 0, 0}, 0, claim).has_value(), L"declared for the claim");
    const Species::Expedition& expedition = region.Expeditions().front();
    Assert::IsTrue(expedition.target == TARGET_SITE && expedition.claim == claim && !expedition.unprovoked &&
                     region.SettlementAt(STEWARD_SITE)->storesMilliRu == Species::MilliRuOf(10'000.0 - tuning.mvp.conflict.attackCostRu),
                   L"on the half's holder, provoked by its claim");
    Assert::IsTrue(StepUntil(*world, 1'200, [&] { return !region.Battles().empty(); }), L"its battle starts");
    Species::Battle& battle = region.Battles().front();
    Assert::IsTrue(battle.claim == claim && Neuron::HorizontalDistance(battle.objective, site) < 1e-6 &&
                     Neuron::HorizontalDistance(battle.fieldCenter, site) < 1e-6 &&
                     std::abs(battle.fieldRadius - tuning.wld.territory.maxRadius / 2.0 - tuning.wld.view.settlementMargin) < 1e-6,
                   L"round the claimed half's resource site");
    battle.defenderLost = Species::LossCapOf(region, battle);
    AdvanceBattlesOnce(region);
    const Species::BattleResult& result = region.BattleResults().front();
    Assert::IsTrue(result.attackerWon && result.areaMoved && result.claim == claim && result.storesMovedMilliRu == 0 &&
                     Species::HolderOf(region, NEIGHBORS_HALF) == std::optional<std::uint8_t>(STEWARD_SITE) &&
                     std::ranges::none_of(region.Claims(), [&](const Species::Claim& _claim) { return _claim.id == claim; }),
                   L"a win moves the half, and no stores, and settles the claim");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101AnAttackAndItsWithdrawalAreCommandsWithTheirRefusals)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101AnAttackAndItsWithdrawalAreCommandsWithTheirRefusals)
  {
    // ADR-161: the declaration and the withdrawal are commands of the region's one path, permitted for the team's
    // commander, whose refusals name their AttackRefusal.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.SetCommander(STEWARD_SITE, STEWARD, false);
    const Species::StableId leader = RaiseLeader(region, STEWARD_SITE, WellspringAt(region, STEWARD_SITE), 2);
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    std::uint64_t sequence = 0;
    const auto executed = [&](const auto& _command)
    {
      Assert::IsTrue(region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(_command)), L"permitted");
      region.Commands().Execute(region.NextTick());
      const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
      ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"one outcome");
      return outcomes.front();
    };
    Neuron::CommandOutcome outcome = executed(Species::WithdrawAttackCommand{STEWARD_SITE});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::NoEffect &&
                     outcome.reason == static_cast<std::uint8_t>(Species::AttackRefusal::Underway),
                   L"no expedition to withdraw, refused with its reason");
    outcome = executed(Species::DeclareAttackCommand{STEWARD_SITE, {leader, 0, 0}, STEWARD_SITE, 0});
    Assert::IsTrue(outcome.reason == static_cast<std::uint8_t>(Species::AttackRefusal::NoTarget), L"not on itself");
    outcome = executed(Species::DeclareAttackCommand{STEWARD_SITE, {leader, 0, 0}, TARGET_SITE, 0});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::Applied && region.Expeditions().size() == 1, L"declared");
    Assert::IsTrue(region.MayCommand(STEWARD, EXPEDITION_TEAM, false), L"the steward commands its expedition");
    outcome = executed(Species::WithdrawAttackCommand{STEWARD_SITE});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::Applied && region.Expeditions().empty(), L"withdrawn");
    Assert::IsFalse(region.MayCommand(STEWARD, EXPEDITION_TEAM, false), L"and its team has no commander again");
    Assert::IsFalse(region.Commands().Submit(STEWARD, sequence++,
                                             Neuron::EncodeMessage(Species::DeclareAttackCommand{0, {leader, 0, 0}, TARGET_SITE, 0})),
                    L"another settlement's attack is not the steward's to declare");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp015TheValleyAtWarHoldsTwoBattlesInProgress)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-015")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp015TheValleyAtWarHoldsTwoBattlesInProgress)
  {
    // ADR-164: the measurement scene of the region at the MVP's bound fills four settlements to their population and
    // starts the two raids of ValleyRaidsOf at once, one on the steward's settlement, which a scripted steward runs with a
    // Territorial settlement's decisions, and holds both battles in progress; its run never ends.
    Species::ValleyAtWarDesc desc;
    desc.key = SETTLED_TEST_KEY;
    desc.terrain = ValleyTerrain();
    desc.tuning = &FastTuning();
    desc.structures = &TestRecords();
    desc.population = 60;
    desc.expedition = 10;
    desc.settleTicks = 2'000;
    const std::unique_ptr<Species::WorldSimulation> world = Species::StartValleyAtWar(desc);
    const Species::RegionSimulation& region = world->Region(0);
    const auto steward = static_cast<std::uint8_t>(FastTuning().mvp.sandbox.stewardSite);
    const std::array<Species::ValleyRaid, 2> raids = Species::ValleyRaidsOf(steward);
    Assert::IsTrue(raids[0].defender == steward && raids[1].attacker != steward && raids[1].defender != steward &&
                     raids[0].attacker != raids[1].attacker && raids[0].attacker != raids[1].defender,
                   L"the steward's neighbour raids it, and the other pair raids itself");
    ExpectEqual(2, static_cast<std::int64_t>(region.Battles().size()), L"two battles in progress");
    for (const Species::Settlement& settlement : region.Settlements())
    {
      Assert::IsTrue(Species::Population(region, settlement) <= 60 && Species::Population(region, settlement) >= 50,
                     L"each settlement at its population of 60, its expedition among it, less the battles' first losses");
    }
    for (const Species::ValleyRaid& raid : raids)
    {
      Assert::IsTrue(std::ranges::any_of(region.Battles(), [&](const Species::Battle& _battle)
                                         { return _battle.attacker == raid.attacker && _battle.defender == raid.defender; }),
                     L"each raid's battle");
    }
    Assert::IsTrue(
      region.CommanderOf(steward).ai &&
        std::ranges::any_of(region.SettlementAis(), [steward](const Species::SettlementAi& _ai) { return _ai.team == steward; }) &&
        region.SettlementAt(steward)->cladan.temperament == Species::Temperament::Territorial,
      L"the steward's settlement run by its scripted steward with a Territorial settlement's decisions");
    Assert::IsTrue(world->RunState().endTick == std::numeric_limits<std::uint64_t>::max(), L"a run that never ends");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095ExpeditionsAndBattlesAreSavedAndAVersion12RegionHasNone)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095ExpeditionsAndBattlesAreSavedAndAVersion12RegionHasNone)
  {
    // ADR-161: the snapshot holds the expeditions, the battles, the battle results and the respites with the expedition's
    // team and its commander, and the restored world steps as the saved one does; a region of version 12 reads with none
    // of them.
    const std::unique_ptr<Species::WorldSimulation> world = RaidUnderway();
    Species::RegionSimulation& region = world->Region(0);
    Species::BattleResult result;
    result.expedition = 77;
    result.attacker = 0;
    result.defender = 1;
    result.end = Species::BattleEnd::Time;
    result.startTick = 10;
    result.endTick = 20;
    result.storesMovedMilliRu = 5;
    result.respiteUntil = region.NextTick() + 100;
    region.BattleResults().push_back(result);
    region.Respites().push_back({1, result.respiteUntil});
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), FastTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    const Species::RegionSimulation& copy = back->Region(0);
    Assert::IsTrue(copy.Expeditions() == region.Expeditions() && copy.Battles() == region.Battles() &&
                     copy.BattleResults() == region.BattleResults() && copy.Respites() == region.Respites(),
                   L"the expedition, its battle, a result and a respite");
    ExpectEqual(LivingOf(region, EXPEDITION_TEAM), LivingOf(copy, EXPEDITION_TEAM), L"the expedition's team");
    Assert::IsTrue(copy.CommanderOf(EXPEDITION_TEAM).sender == region.CommanderOf(EXPEDITION_TEAM).sender, L"and its commander");
    Neuron::WorkerPool pool(0);
    for (std::uint64_t step = 0; step < 50; ++step)
    {
      const std::uint64_t tick = region.NextTick();
      world->Step(tick, pool);
      back->Step(tick, pool);
    }
    Assert::IsTrue(Species::HashWorld(*world).world == Species::HashWorld(*back).world, L"and steps as the saved world does");

    // Version 12's layout: the current one without the counts of the later sections.
    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld();
    const std::vector<std::byte> older = RegionStateOf(plain->Region(0), 12);
    Species::RegionSimulation twelve(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), CommittedTuning(), ValleyTerrain(), &TestRecords());
    Neuron::BitReader reader(older);
    Assert::IsTrue(twelve.ReadState(reader, 12) && reader.Finish(), L"a version-12 region reads whole");
    Assert::IsTrue(twelve.Expeditions().empty() && twelve.Battles().empty() && twelve.BattleResults().empty() &&
                     twelve.Respites().empty() && twelve.Settlements().size() == 4,
                   L"with no expedition");
  }
};

} // namespace GameLogicTests
