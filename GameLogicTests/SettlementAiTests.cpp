#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "SettlementAi.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "Territory.h"
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
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, the AI settlements at 0, 1 and 3

// The committed tuning with the decisions, the muster and the warning shortened, so that an AI settlement's attack is
// mustered and declared within a few hundred ticks.
const Species::TuningMvp& FastTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.ai.decisionTicks = 10;
    tuning.mvp.ai.musterMinutes = 0.5; // 300 ticks
    tuning.mvp.conflict.warningMinutes = 0.1;
    tuning.pil.battle.maxMinutes = 2.0;
    return tuning;
  }();
  return TUNING;
}

Species::Settlement& SettlementAt(Species::RegionSimulation& _region, std::uint8_t _site)
{
  return *std::ranges::find(_region.Settlements(), _site, &Species::Settlement::site);
}

std::uint8_t SiteOf(const Species::RegionSimulation& _region, Species::Temperament _temperament)
{
  return std::ranges::find(_region.Settlements(), _temperament,
                           [](const Species::Settlement& _settlement) { return _settlement.cladan.temperament; })
    ->site;
}

const Species::SettlementAi& AiOf(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  return *std::ranges::find(_region.SettlementAis(), _site, &Species::SettlementAi::team);
}

// The valley neighbour of _site across the pair of _index in VALLEY_PAIRS that holds it, or none.
std::optional<std::uint8_t> NeighborOf(std::uint8_t _site, std::size_t _index)
{
  const auto& pair = Species::VALLEY_PAIRS[_index];
  return pair[0] == _site ? std::optional(pair[1]) : pair[1] == _site ? std::optional(pair[0]) : std::nullopt;
}

// Each of the region's territories at its largest.
void ReachAll(Species::RegionSimulation& _region)
{
  for (Species::Settlement& settlement : _region.Settlements())
  {
    settlement.territoryRadius = _region.Tuning().wld.territory.maxRadius;
    _region.RecomputeTerritory(settlement.site);
  }
}

// _count more Workers of the settlement at _site at its Wellspring.
void Strengthen(Species::RegionSimulation& _region, std::uint8_t _site, std::uint32_t _count)
{
  const Species::Settlement& settlement = SettlementAt(_region, _site);
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _site;
  spawn.position = _region.ResolveStructure(settlement.wellspringSlot, settlement.wellspringId)->position;
  spawn.count = _count;
  spawn.spread = 30.0;
  static_cast<void>(_region.SpawnFigures(spawn));
}

// The claims of the settlement at _site.
std::vector<Species::Claim> ClaimsOf(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  std::vector<Species::Claim> claims;
  std::ranges::copy_if(_region.Claims(), std::back_inserter(claims),
                       [_site](const Species::Claim& _claim) { return _claim.claimant == _site; });
  return claims;
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

void StepTicks(Species::WorldSimulation& _world, std::uint64_t _ticks)
{
  static_cast<void>(StepUntil(_world, _ticks, [] { return false; }));
}

// A valley on FastTuning whose Bold settlement has 40 Workers to field and the stores of an attack.
struct Armed
{
  std::unique_ptr<Species::WorldSimulation> world;
  std::uint8_t bold = 0;
};

Armed ArmedBold(const Neuron::SipHashKey& _key = SETTLED_TEST_KEY)
{
  Armed armed{ValleyWorld(_key, FastTuning()), 0};
  Species::RegionSimulation& region = armed.world->Region(0);
  armed.bold = SiteOf(region, Species::Temperament::Bold);
  Strengthen(region, armed.bold, 40);
  SettlementAt(region, armed.bold).storesMilliRu = Species::MilliRuOf(10'000.0);
  return armed;
}

} // namespace

TEST_CLASS(SettlementAiTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp057EachTemperamentClaimsAsItsOwnOnACorpusOfKeys)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-057")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp057EachTemperamentClaimsAsItsOwnOnACorpusOfKeys)
  {
    // ADR-162, DEC-1006: whatever settlement each temperament is dealt to, the Bold one claims the central resource site
    // at its first decision, the Territorial one each half of its border areas that a neighbour holds, and the Wary one
    // nothing until it is attacked, then the halves its attacker holds; each through the command path.
    for (std::uint64_t low = 1; low <= 3; ++low)
    {
      const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld({low, 0x5e771e5e771e5e77}, FastTuning());
      Species::RegionSimulation& region = world->Region(0);
      const std::uint8_t bold = SiteOf(region, Species::Temperament::Bold);
      const std::uint8_t territorial = SiteOf(region, Species::Temperament::Territorial);
      const std::uint8_t wary = SiteOf(region, Species::Temperament::Wary);
      Assert::IsTrue(region.SettlementAis().size() == 3, L"an AI's decisions for each AI settlement");
      StepTicks(*world, 2);
      const std::vector<Species::Claim> early = ClaimsOf(region, bold);
      Assert::IsTrue(early.size() == 1 && early.front().area == Species::CLAIM_AREA_CENTRAL &&
                       early.front().state == Species::ClaimState::Waiting,
                     L"the Bold settlement claims the central resource site at once");
      Assert::IsTrue(ClaimsOf(region, territorial).empty() && ClaimsOf(region, wary).empty(), L"the others claim nothing yet");

      ReachAll(region);
      StepTicks(*world, 3 * static_cast<std::uint64_t>(FastTuning().mvp.ai.decisionTicks));
      std::size_t halves = 0;
      for (std::size_t pair = 0; pair < Species::VALLEY_PAIRS.size(); ++pair)
      {
        halves += NeighborOf(territorial, pair).has_value() ? 1 : 0;
      }
      const std::vector<Species::Claim> territorialClaims = ClaimsOf(region, territorial);
      Assert::IsTrue(halves == 2 && territorialClaims.size() == halves,
                     L"the Territorial settlement claims each of its neighbours' halves");
      for (const Species::Claim& claim : territorialClaims)
      {
        Assert::IsTrue(claim.area < Species::CLAIM_AREA_CENTRAL && claim.state == Species::ClaimState::Open &&
                         Species::HolderOf(region, claim.area) != std::optional<std::uint8_t>(territorial),
                       L"each a neighbour's half, open against it");
      }
      Assert::IsTrue(ClaimsOf(region, wary).empty() && ClaimsOf(region, bold).size() == 1, L"the Wary and the Bold claim no half");

      std::optional<std::uint8_t> attacker;
      for (std::size_t pair = 0; pair < Species::VALLEY_PAIRS.size() && !attacker; ++pair)
      {
        attacker = NeighborOf(wary, pair);
      }
      Species::AddGrievance(region, wary, attacker.value_or(0), Species::GrievanceCause::Attack, 4'242);
      StepTicks(*world, 2 * static_cast<std::uint64_t>(FastTuning().mvp.ai.decisionTicks));
      const std::vector<Species::Claim> waryClaims = ClaimsOf(region, wary);
      Assert::IsTrue(waryClaims.size() == 1 && Species::HolderOf(region, waryClaims.front().area) == attacker,
                     L"once attacked, the Wary settlement claims its attacker's half");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp055AnAiAttacksOnlyAboveItsThresholdAndWithEnoughToField)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-055")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-041")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-025")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp055AnAiAttacksOnlyAboveItsThresholdAndWithEnoughToField)
  {
    // ADR-162: below its temperament's threshold an AI settlement musters nothing; at it, it promotes a Leader near its
    // Wellspring, forms it up so that Workers on guard duty join it, and declares a raid when its muster's time is out
    // with at least mvp.ai.attack_min_share of the target's Workers it sees; it musters nothing against a target under a
    // respite, or one whose seen Workers it cannot field enough against.
    const Species::TuningMvp& tuning = FastTuning();
    const auto decision = static_cast<std::uint64_t>(tuning.mvp.ai.decisionTicks);
    {
      Armed armed = ArmedBold();
      Species::RegionSimulation& region = armed.world->Region(0);
      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 1);
      StepTicks(*armed.world, 3 * decision);
      Assert::IsTrue(!AiOf(region, armed.bold).muster && region.Expeditions().empty(), L"below its threshold, no muster");

      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 2);
      Assert::IsTrue(StepUntil(*armed.world, 2 * decision, [&] { return AiOf(region, armed.bold).muster.has_value(); }),
                     L"at its threshold, a muster");
      const Species::Muster muster = AiOf(region, armed.bold).muster.value_or(Species::Muster{});
      Assert::IsTrue(muster.target == STEWARD_SITE && muster.claim == 0 && muster.size == 40 && muster.need >= 1 &&
                       muster.need <= muster.size && muster.leadersWanted == 1,
                     L"a raid by the Bold, of the 40 Workers it can field, with one Leader");
      Assert::IsTrue(StepUntil(*armed.world, 3 * decision,
                               [&] { return AiOf(region, armed.bold).muster.value_or(Species::Muster{}).leaders.size() == 1; }),
                     L"its Leader promoted and adopted");
      const std::vector<Species::StableId> adopted = AiOf(region, armed.bold).muster.value_or(Species::Muster{}).leaders;
      const Species::StableId leader = adopted.empty() ? 0 : adopted.front();
      const std::uint64_t musterTicks =
        static_cast<std::uint64_t>(std::llround(tuning.mvp.ai.musterMinutes * 60.0 / tuning.ref.sim.tickPeriod));
      Assert::IsTrue(StepUntil(*armed.world, musterTicks + 2 * decision, [&] { return !region.Expeditions().empty(); }),
                     L"declared once its muster's time is out");
      const Species::Expedition& expedition = region.Expeditions().front();
      Assert::IsTrue(expedition.attacker == armed.bold && expedition.target == STEWARD_SITE && expedition.claim == 0 &&
                       expedition.leaders.front() == leader && expedition.sent >= muster.need && !expedition.unprovoked,
                     L"its expedition sent, provoked by its anger");
    }
    {
      Armed armed = ArmedBold();
      Species::RegionSimulation& region = armed.world->Region(0);
      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 1);
      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 2);
      region.Respites().push_back({STEWARD_SITE, region.NextTick() + 10'000});
      StepTicks(*armed.world, 3 * decision);
      Assert::IsFalse(AiOf(region, armed.bold).muster.has_value(), L"no muster against a target under a respite");
    }
    {
      Armed armed = ArmedBold();
      Species::RegionSimulation& region = armed.world->Region(0);
      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 1);
      Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 2);
      // 100 of the target's Workers where the Bold settlement sees them, of which it would need 60.
      const Species::Settlement& bold = SettlementAt(region, armed.bold);
      Species::RegionSimulation::Spawn spawn;
      spawn.team = STEWARD_SITE;
      spawn.position = region.ResolveStructure(bold.wellspringSlot, bold.wellspringId)->position;
      spawn.count = 100;
      spawn.spread = 40.0;
      static_cast<void>(region.SpawnFigures(spawn));
      StepTicks(*armed.world, 2);
      Assert::IsTrue(Species::SeenWorkersOf(region, armed.bold, STEWARD_SITE) >= 67, L"the target's Workers it sees");
      StepTicks(*armed.world, 3 * decision);
      Assert::IsFalse(AiOf(region, armed.bold).muster.has_value(), L"no muster it cannot field enough for");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp057TheTemperamentChoosesARaidOrAClaimBattle)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-057")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-055")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp057TheTemperamentChoosesARaidOrAClaimBattle)
  {
    // ADR-162: with both its anger and a claim on what its target holds, the Bold settlement musters a raid and the
    // Territorial one the claim battle; with a claim alone, even the Bold one musters the claim battle (PIL-025).
    const Species::TuningMvp& tuning = FastTuning();
    const auto decision = static_cast<std::uint64_t>(tuning.mvp.ai.decisionTicks);
    for (const Species::Temperament temperament : {Species::Temperament::Bold, Species::Temperament::Territorial})
    {
      for (const bool angry : {true, false})
      {
        const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, FastTuning());
        Species::RegionSimulation& region = world->Region(0);
        const std::uint8_t site = SiteOf(region, temperament);
        std::optional<std::uint8_t> neighbor;
        std::uint8_t half = 0;
        for (std::size_t pair = 0; pair < Species::VALLEY_PAIRS.size() && !neighbor; ++pair)
        {
          neighbor = NeighborOf(site, pair);
          half = static_cast<std::uint8_t>(pair * 2 + (Species::VALLEY_PAIRS[pair][0] == site ? 1 : 0));
        }
        ReachAll(region);
        Strengthen(region, site, 40);
        SettlementAt(region, site).storesMilliRu = Species::MilliRuOf(10'000.0);
        const std::uint8_t target = neighbor.value_or(0);
        const Species::StableId claim = Species::MakeClaim(region, site, half).value_or(0);
        Assert::IsTrue(claim != 0 && Species::HolderOf(region, half) == neighbor, L"a claim on its neighbour's half");
        for (std::uint64_t subject = 1; angry && subject <= 3; ++subject)
        {
          Species::AddGrievance(region, site, target, Species::GrievanceCause::Attack, subject);
        }
        Assert::IsTrue(StepUntil(*world, 3 * decision, [&] { return AiOf(region, site).muster.has_value(); }), L"a muster");
        const Species::Muster muster = AiOf(region, site).muster.value_or(Species::Muster{});
        const bool raid = angry && temperament == Species::Temperament::Bold;
        Assert::IsTrue(muster.target == target && muster.claim == (raid ? 0 : claim),
                       raid ? L"the Bold raids when it is angry" : L"otherwise the claim battle");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp049TheBattleCoresCommandTheSidesAndTheirLeadersGoBackToWork)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-049")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-058")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp049TheBattleCoresCommandTheSidesAndTheirLeadersGoBackToWork)
  {
    // ADR-162: while an AI settlement's expedition fights, the reference bot's battle core commands its Leaders, and
    // while it defends, its core commands its settlement's side; when the battles are over, its Leaders outside a muster
    // are demoted.
    const Species::TuningMvp& tuning = FastTuning();
    Armed armed = ArmedBold();
    Species::RegionSimulation& region = armed.world->Region(0);
    Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 1);
    Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 2);
    const auto battleTicks = static_cast<std::uint64_t>(std::llround(tuning.pil.battle.maxMinutes * 60.0 / tuning.ref.sim.tickPeriod));
    Assert::IsTrue(StepUntil(*armed.world, 20 * battleTicks, [&] { return !region.Battles().empty(); }),
                   L"the Bold settlement's raid starts");
    const Species::StableId expedition = region.Battles().front().expedition;
    Assert::IsTrue(StepUntil(*armed.world, 20, [&] { return !AiOf(region, armed.bold).attack.leaderTimers.empty(); }) &&
                     AiOf(region, armed.bold).attacking && AiOf(region, armed.bold).attack.team == Species::ExpeditionTeamOf(armed.bold),
                   L"its battle core commands the expedition's Leaders");
    Assert::IsTrue(StepUntil(*armed.world, battleTicks + 1, [&] { return region.Battles().empty(); }), L"the battle ends");
    Assert::IsTrue(region.BattleResults().back().expedition == expedition && !AiOf(region, armed.bold).attacking, L"and its core stops");
    // The settlement's Leaders outside a muster: a new muster, which its anger may start at once, keeps its own.
    const auto leaders = [&]
    {
      std::size_t count = 0;
      const std::optional<Species::Muster>& muster = AiOf(region, armed.bold).muster;
      const Species::SlicedPool<Species::Figure>& figures = region.TeamAt(armed.bold).figures;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (!figures.Used(slot))
        {
          continue;
        }
        const Species::Figure& figure = figures.At(slot);
        const bool mustered = muster && std::ranges::find(muster->leaders, figure.id) != muster->leaders.end();
        count += figure.kind == Species::FigureKind::Leader && !figure.dead && !figure.demoted && !mustered ? 1 : 0;
      }
      return count;
    };
    Assert::IsTrue(StepUntil(*armed.world, 3 * static_cast<std::uint64_t>(tuning.mvp.ai.decisionTicks), [&] { return leaders() == 0; }),
                   L"its Leaders home are demoted, but a new muster's");

    // The defense: the steward's raid on the Bold settlement.
    const Species::Settlement& bold = SettlementAt(region, armed.bold);
    const Neuron::Vec3 home = region.ResolveStructure(bold.wellspringSlot, bold.wellspringId)->position;
    Species::RegionSimulation::Spawn spawn;
    spawn.team = STEWARD_SITE;
    spawn.kind = Species::FigureKind::Leader;
    spawn.position = home;
    const std::uint32_t slot = region.SpawnFigures(spawn).front();
    const Species::StableId raider = region.TeamAt(STEWARD_SITE).figures.At(slot).id;
    SettlementAt(region, STEWARD_SITE).storesMilliRu = Species::MilliRuOf(10'000.0);
    region.Respites().clear();
    Assert::IsTrue(Species::DeclareAttack(region, STEWARD_SITE, {raider, 0, 0}, armed.bold, 0).has_value(), L"the steward raids it");
    Assert::IsTrue(StepUntil(*armed.world, 2 * battleTicks, [&] { return AiOf(region, armed.bold).defending; }),
                   L"its battle core defends its side");
    Assert::IsTrue(StepUntil(*armed.world, battleTicks + 1, [&] { return region.Battles().empty(); }) &&
                     StepUntil(*armed.world, 3 * static_cast<std::uint64_t>(tuning.mvp.ai.decisionTicks), [&] { return leaders() == 0; }) &&
                     !AiOf(region, armed.bold).defending,
                   L"and when the battle is over its Leaders are demoted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095TheDecisionsAreSavedAndAVersion13RegionMakesThemAfresh)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095TheDecisionsAreSavedAndAVersion13RegionMakesThemAfresh)
  {
    // ADR-162: the snapshot holds each AI settlement's decisions, a muster under way among them, and the restored world
    // decides as the saved one does; a region of version 13 makes them afresh for each AI settlement's steward.
    Armed armed = ArmedBold();
    Species::RegionSimulation& region = armed.world->Region(0);
    Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 1);
    Species::AddGrievance(region, armed.bold, STEWARD_SITE, Species::GrievanceCause::Claim, 2);
    Assert::IsTrue(StepUntil(*armed.world, 100, [&] { return AiOf(region, armed.bold).muster.has_value(); }), L"a muster under way");
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*armed.world), FastTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(back->Region(0).SettlementAis() == region.SettlementAis(), L"the decisions restored");
    Neuron::WorkerPool pool(0);
    for (std::uint64_t step = 0; step < 400; ++step)
    {
      const std::uint64_t tick = region.NextTick();
      armed.world->Step(tick, pool);
      back->Step(tick, pool);
    }
    Assert::IsTrue(Species::HashWorld(*armed.world).world == Species::HashWorld(*back).world && !region.Expeditions().empty(),
                   L"and decide alike, to the declaration");

    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld();
    const std::vector<std::byte> older = RegionStateOf(plain->Region(0), 13);
    Species::RegionSimulation thirteen(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), CommittedTuning(), ValleyTerrain(),
                                       &TestRecords());
    Neuron::BitReader reader(older);
    Assert::IsTrue(thirteen.ReadState(reader, 13) && reader.Finish(), L"a version-13 region reads whole");
    Assert::IsTrue(thirteen.SettlementAis() == plain->Region(0).SettlementAis(), L"with its decisions made afresh");
  }
};

} // namespace GameLogicTests
