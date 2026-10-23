#include "pch.h"

#include "Claims.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "Relations.h"
#include "Statistics.h"
#include "TestSupport.h"
#include "Territory.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, valley neighbour of site 3 (pair 1)
constexpr std::uint8_t NEIGHBOR_SITE = 3;
constexpr std::uint8_t STEWARDS_HALF = 2;  // pair 1's half nearer site 2
constexpr std::uint8_t NEIGHBORS_HALF = 3; // and its half nearer site 3
constexpr Neuron::SenderId STEWARD = 7;

// Each of _sites's territories at its largest.
void Reach(Species::RegionSimulation& _region, std::initializer_list<std::uint8_t> _sites)
{
  for (Species::Settlement& settlement : _region.Settlements())
  {
    for (const std::uint8_t site : _sites)
    {
      if (settlement.site == site)
      {
        settlement.territoryRadius = CommittedTuning().wld.territory.maxRadius;
        _region.RecomputeTerritory(site);
      }
    }
  }
}

// The cells of the territory of the settlement at _site that lie in the half _area.
std::size_t CellsIn(const Species::RegionSimulation& _region, std::uint8_t _site, std::uint8_t _area)
{
  const Species::CellRaster* territory = _region.TerritoryOf(_site);
  std::size_t cells = 0;
  for (std::int32_t z = 0; territory != nullptr && z < territory->CellsPerAxis(); ++z)
  {
    for (std::int32_t x = 0; x < territory->CellsPerAxis(); ++x)
    {
      const Neuron::Vec3 center = territory->CellCenter(x, z);
      cells += territory->HoldsCell(x, z) && Species::InBorderHalf(_region, _area, center.x, center.z) ? 1 : 0;
    }
  }
  return cells;
}

const Species::Claim* ClaimOf(const Species::RegionSimulation& _region, Species::StableId _id)
{
  const auto found = std::ranges::find(_region.Claims(), _id, &Species::Claim::id);
  return found != _region.Claims().end() ? &*found : nullptr;
}

const Species::Grievance* GrievanceOf(const Species::RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against,
                                      Species::GrievanceCause _cause, std::uint64_t _subject)
{
  const auto found = std::ranges::find_if(_region.Grievances(),
                                          [&](const Species::Grievance& _grievance) {
                                            return _grievance.holder == _holder && _grievance.against == _against &&
                                                   _grievance.cause == _cause && _grievance.subject == _subject;
                                          });
  return found != _region.Grievances().end() ? &*found : nullptr;
}

// The ticks of mvp.claim.uncontested_minutes.
std::uint64_t WaitTicks()
{
  return static_cast<std::uint64_t>(
    std::llround(CommittedTuning().mvp.claim.uncontestedMinutes * 60.0 / CommittedTuning().ref.sim.tickPeriod));
}

// The committed tuning with no claim cooldown, for the tests that claim an area again at once (ADR-160), and with a
// cooldown of 3 seconds, which a test steps through (O100, DEC-1008); both outlive the worlds.
const Species::TuningMvp& NoCooldownTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.claim.cooldownMinutes = 0.0;
    return tuning;
  }();
  return TUNING;
}

const Species::TuningMvp& ShortCooldownTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.claim.cooldownMinutes = 0.05; // 30 ticks
    return tuning;
  }();
  return TUNING;
}

// The ticks of _tuning's mvp.claim.cooldown_minutes.
std::uint64_t CooldownTicks(const Species::TuningMvp& _tuning)
{
  return static_cast<std::uint64_t>(std::llround(_tuning.mvp.claim.cooldownMinutes * 60.0 / _tuning.ref.sim.tickPeriod));
}

std::uint8_t SiteOf(const Species::RegionSimulation& _region, Species::Temperament _temperament)
{
  return std::ranges::find(_region.Settlements(), _temperament,
                           [](const Species::Settlement& _settlement) { return _settlement.cladan.temperament; })
    ->site;
}

} // namespace

TEST_CLASS(ClaimTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld086AHalfIsClaimableOnlyByItsNeighborAndOnlyWhileItHoldsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-063")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-086")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-051")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld086AHalfIsClaimableOnlyByItsNeighborAndOnlyWhileItHoldsIt)
  {
    // ADR-160: a half is held by its own side's settlement once its territory enters it (WLD-063), and only its valley
    // neighbour may claim it then (WLD-086); the claim is Open and gives the holder's Cladan mvp.grievance.claim_weight
    // against the claimant (MVP-051).
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsFalse(Species::HolderOf(region, NEIGHBORS_HALF).has_value(), L"not held at the founding radius");
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::NotHeld, L"so not claimable");
    Reach(region, {NEIGHBOR_SITE});
    Assert::IsTrue(Species::HolderOf(region, NEIGHBORS_HALF) == std::optional<std::uint8_t>(NEIGHBOR_SITE), L"held once entered");
    Assert::IsTrue(Species::CheckClaim(region, 0, NEIGHBORS_HALF) == Species::ClaimRefusal::NotClaimable, L"site 0 is no neighbour of it");
    Assert::IsTrue(Species::CheckClaim(region, NEIGHBOR_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::AlreadyHeld, L"nor its holder");
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, Species::CLAIM_AREA_MAX + 1) == Species::ClaimRefusal::NotClaimable,
                   L"nor an area beyond the region's");
    Assert::IsFalse(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF).has_value(), L"its neighbour may claim it");

    const auto made = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF);
    Assert::IsTrue(made.has_value(), L"claimed");
    const Species::StableId id = made.value_or(0);
    const Species::Claim* claim = ClaimOf(region, id);
    Assert::IsTrue(claim != nullptr && claim->state == Species::ClaimState::Open && claim->claimant == STEWARD_SITE &&
                     claim->madeTick == region.NextTick(),
                   L"an Open claim of the tick");
    const Species::Grievance* grievance = GrievanceOf(region, NEIGHBOR_SITE, STEWARD_SITE, Species::GrievanceCause::Claim, id);
    Assert::IsTrue(grievance != nullptr && grievance->weightMicro == std::llround(CommittedTuning().mvp.grievance.claimWeight * 1e6),
                   L"the holder's grievance against the claimant");
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::AlreadyClaimed, L"once only");
    Assert::IsTrue(Species::WithdrawClaim(region, 0, id) == Species::ClaimRefusal::NoSuchClaim, L"another settlement does not withdraw it");
    Assert::IsFalse(Species::WithdrawClaim(region, STEWARD_SITE, id).has_value(), L"its claimant does");
    Assert::IsTrue(region.Claims().empty() &&
                     GrievanceOf(region, NEIGHBOR_SITE, STEWARD_SITE, Species::GrievanceCause::Claim, id)->causeRemoved,
                   L"which ends it and removes its grievance's cause");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053AClaimBattleWonMovesTheHalfAndAWonBackOneReturnsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-121")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-066")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053AClaimBattleWonMovesTheHalfAndAWonBackOneReturnsIt)
  {
    // ADR-160: a claim battle the claimant loses moves nothing; one it wins moves the half to it: its territory grows
    // into the half and the loser's leaves it, the move is a territory event, and the two are still in contact; the
    // loser may claim it back (here at once, with no cooldown, O100), and winning returns it to its own side.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, NoCooldownTuning());
    Species::RegionSimulation& region = world->Region(0);
    Reach(region, {STEWARD_SITE, NEIGHBOR_SITE});
    const std::size_t neighbors = CellsIn(region, NEIGHBOR_SITE, NEIGHBORS_HALF);
    Assert::IsTrue(neighbors > 0 && CellsIn(region, STEWARD_SITE, NEIGHBORS_HALF) == 0 &&
                     Species::InContact(region, STEWARD_SITE, NEIGHBOR_SITE),
                   L"each in its own half, in contact");

    const Species::StableId lost = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    Species::SettleClaim(region, lost, false);
    Assert::IsTrue(region.Claims().empty() && !Species::MovedTo(region, NEIGHBORS_HALF) &&
                     GrievanceOf(region, NEIGHBOR_SITE, STEWARD_SITE, Species::GrievanceCause::Claim, lost)->causeRemoved,
                   L"a lost battle moves nothing and ends the claim");

    const Species::StableId won = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    const std::size_t events = region.ContactEvents().size();
    Species::SettleClaim(region, won, true);
    Assert::IsTrue(Species::HolderOf(region, NEIGHBORS_HALF) == std::optional<std::uint8_t>(STEWARD_SITE), L"a won battle moves the half");
    Assert::IsTrue(CellsIn(region, STEWARD_SITE, NEIGHBORS_HALF) > neighbors / 2 && CellsIn(region, NEIGHBOR_SITE, NEIGHBORS_HALF) == 0,
                   L"the winner's territory grows into it and the loser's leaves it");
    Assert::IsTrue(region.ContactEvents().size() == events + 1, L"one territory event");
    const Species::ContactEvent expected{region.NextTick(), Species::ContactEventKind::AreaMoved, NEIGHBOR_SITE, STEWARD_SITE, 1,
                                         NEIGHBORS_HALF};
    Assert::IsTrue(region.ContactEvents().back() == expected, L"the half's move, from the loser to the winner");
    Assert::IsTrue(Species::InContact(region, STEWARD_SITE, NEIGHBOR_SITE), L"still in contact at the moved border");
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::AlreadyHeld, L"the winner holds it");

    const Species::StableId back = Species::MakeClaim(region, NEIGHBOR_SITE, NEIGHBORS_HALF).value_or(0);
    Assert::IsTrue(back != 0 && GrievanceOf(region, STEWARD_SITE, NEIGHBOR_SITE, Species::GrievanceCause::Claim, back) != nullptr,
                   L"the loser claims it back, against its holder");
    Species::SettleClaim(region, back, true);
    Assert::IsTrue(region.Holdings().empty() && Species::HolderOf(region, NEIGHBORS_HALF) == std::optional<std::uint8_t>(NEIGHBOR_SITE) &&
                     CellsIn(region, NEIGHBOR_SITE, NEIGHBORS_HALF) == neighbors,
                   L"won back, it is its own side's again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053TheCentralSiteIsHeldUncontestedAfterItsWait)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-086")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053TheCentralSiteIsHeldUncontestedAfterItsWait)
  {
    // ADR-160: a claim on the central resource site that nobody holds waits, with no grievance, and holds the site
    // mvp.claim.uncontested_minutes after it was made; its holder then counts the site among its held resource sites,
    // and a claim on it from another settlement is Open, against the holder (with no cooldown, which the cooldown's own
    // test steps through, O100).
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, NoCooldownTuning());
    Species::RegionSimulation& region = world->Region(0);
    const Species::StableId id = Species::MakeClaim(region, 0, Species::CLAIM_AREA_CENTRAL).value_or(0);
    const Species::Claim* claim = ClaimOf(region, id);
    Assert::IsTrue(claim != nullptr && claim->state == Species::ClaimState::Waiting && region.Grievances().empty(),
                   L"waiting, no grievance");
    const std::uint64_t made = claim->madeTick;
    Species::AdvanceClaims(region, made + WaitTicks() - 1);
    Assert::IsTrue(ClaimOf(region, id) != nullptr && !Species::MovedTo(region, Species::CLAIM_AREA_CENTRAL), L"not before its wait");
    const std::size_t heldBefore = Species::HeldResourceSites(region, *region.SettlementAt(0)).size();
    Species::AdvanceClaims(region, made + WaitTicks());
    Assert::IsTrue(region.Claims().empty() &&
                     Species::HolderOf(region, Species::CLAIM_AREA_CENTRAL) == std::optional<std::uint8_t>(std::uint8_t{0}),
                   L"held after its wait");
    ExpectEqual(static_cast<std::int64_t>(heldBefore) + 1,
                static_cast<std::int64_t>(Species::HeldResourceSites(region, *region.SettlementAt(0)).size()),
                L"one more resource site held");
    Assert::IsTrue(Species::CheckClaim(region, 0, Species::CLAIM_AREA_CENTRAL) == Species::ClaimRefusal::AlreadyHeld, L"its holder's");
    const Species::StableId open = Species::MakeClaim(region, 1, Species::CLAIM_AREA_CENTRAL).value_or(0);
    Assert::IsTrue(ClaimOf(region, open)->state == Species::ClaimState::Open &&
                     GrievanceOf(region, 0, 1, Species::GrievanceCause::Claim, open) != nullptr,
                   L"another's claim on it is Open, against the holder");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053AnAreaIsClaimedAgainOnlyAfterItsCooldown)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053AnAreaIsClaimedAgainOnlyAfterItsCooldown)
  {
    // O100 (DEC-1008): no claim is made on an area until mvp.claim.cooldown_minutes have passed since the last claim on it
    // ended, by a withdrawal, a claim battle or the central resource site's wait; each area cools down alone, the tick
    // of each area's last claim's end is saved, and a region of version 15 reads with none.
    const Species::TuningMvp& tuning = ShortCooldownTuning();
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld(SETTLED_TEST_KEY, tuning);
    Species::RegionSimulation& region = world->Region(0);
    region.SettlementAis().clear(); // the test's claims alone (ADR-162)
    Reach(region, {STEWARD_SITE, NEIGHBOR_SITE});
    Assert::IsFalse(Species::CooldownEndOf(region, NEIGHBORS_HALF).has_value(), L"no claim on it has ended");

    const Species::StableId withdrawn = Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0);
    Assert::IsFalse(Species::WithdrawClaim(region, STEWARD_SITE, withdrawn).has_value(), L"withdrawn");
    const std::uint64_t cooled = region.NextTick() + CooldownTicks(tuning);
    Assert::IsTrue(Species::CooldownEndOf(region, NEIGHBORS_HALF) == std::optional<std::uint64_t>(cooled), L"it cools down from its end");
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::CoolingDown,
                   L"so it takes no claim");
    Assert::IsFalse(Species::CheckClaim(region, NEIGHBOR_SITE, STEWARDS_HALF).has_value(), L"the pair's other half does");
    Neuron::WorkerPool pool(0);
    while (region.NextTick() + 1 < cooled)
    {
      world->Step(region.NextTick(), pool);
    }
    Assert::IsTrue(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::CoolingDown,
                   L"still on the cooldown's last tick");
    world->Step(region.NextTick(), pool);
    Assert::IsFalse(Species::CheckClaim(region, STEWARD_SITE, NEIGHBORS_HALF).has_value(), L"and claimable once it has passed");

    Species::SettleClaim(region, Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0), true);
    Assert::IsTrue(Species::HolderOf(region, NEIGHBORS_HALF) == std::optional<std::uint8_t>(STEWARD_SITE) &&
                     Species::CheckClaim(region, NEIGHBOR_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::CoolingDown,
                   L"a claim battle's end cools the half down too, so its loser does not claim it back at once");
    const Species::StableId central = Species::MakeClaim(region, 0, Species::CLAIM_AREA_CENTRAL).value_or(0);
    Species::AdvanceClaims(region, ClaimOf(region, central)->madeTick + WaitTicks());
    Assert::IsTrue(Species::HolderOf(region, Species::CLAIM_AREA_CENTRAL) == std::optional<std::uint8_t>(std::uint8_t{0}) &&
                     Species::CheckClaim(region, 1, Species::CLAIM_AREA_CENTRAL) == Species::ClaimRefusal::CoolingDown,
                   L"and the central site's wait");

    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), tuning, &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(back->Region(0).ClaimEnds() == region.ClaimEnds() && region.ClaimEnds().size() == 2 &&
                     Species::CheckClaim(back->Region(0), NEIGHBOR_SITE, NEIGHBORS_HALF) == Species::ClaimRefusal::CoolingDown,
                   L"the ends are saved, and the half still cools down");

    // Version 15's layout: the current one without the ends' count.
    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld(SETTLED_TEST_KEY, tuning);
    const std::vector<std::byte> older = RegionStateOf(plain->Region(0), 15);
    Species::RegionSimulation fifteen(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), tuning, ValleyTerrain(), &TestRecords());
    Neuron::BitReader reader(older);
    Assert::IsTrue(fifteen.ReadState(reader, 15) && reader.Finish() && fifteen.ClaimEnds().empty(),
                   L"a version-15 region reads whole, with no area cooling down");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053AContestedCentralClaimWaitsForItsBattleOrAWithdrawal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053AContestedCentralClaimWaitsForItsBattleOrAWithdrawal)
  {
    // ADR-160: a second claim on the central resource site within the first's wait contests both, and no wait holds
    // it; a withdrawal leaves the other waiting again from its own making, and a claim battle settles every claim on it.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::StableId first = Species::MakeClaim(region, 0, Species::CLAIM_AREA_CENTRAL).value_or(0);
    const Species::StableId second = Species::MakeClaim(region, 1, Species::CLAIM_AREA_CENTRAL).value_or(0);
    Assert::IsTrue(ClaimOf(region, first)->state == Species::ClaimState::Contested &&
                     ClaimOf(region, second)->state == Species::ClaimState::Contested,
                   L"both contested");
    Species::AdvanceClaims(region, region.NextTick() + 2 * WaitTicks());
    Assert::IsTrue(region.Claims().size() == 2 && !Species::MovedTo(region, Species::CLAIM_AREA_CENTRAL), L"no wait holds it");
    Assert::IsFalse(Species::WithdrawClaim(region, 1, second).has_value(), L"withdrawn");
    Assert::IsTrue(ClaimOf(region, first)->state == Species::ClaimState::Waiting, L"the other waits again");
    Species::AdvanceClaims(region, region.NextTick() + WaitTicks());
    Assert::IsTrue(Species::HolderOf(region, Species::CLAIM_AREA_CENTRAL) == std::optional<std::uint8_t>(std::uint8_t{0}), L"and holds it");

    const std::unique_ptr<Species::WorldSimulation> battled = ValleyWorld();
    Species::RegionSimulation& other = battled->Region(0);
    const Species::StableId bold = Species::MakeClaim(other, 0, Species::CLAIM_AREA_CENTRAL).value_or(0);
    Assert::IsTrue(Species::MakeClaim(other, 1, Species::CLAIM_AREA_CENTRAL).has_value() &&
                     Species::MakeClaim(other, 3, Species::CLAIM_AREA_CENTRAL).has_value(),
                   L"three claims on it");
    const Species::StableId lost = other.Claims()[1].id;
    Species::SettleClaim(other, lost, false);
    Assert::IsTrue(other.Claims().size() == 2 && other.Claims()[0].state == Species::ClaimState::Contested,
                   L"a lost battle ends its claim");
    Species::SettleClaim(other, bold, true);
    Assert::IsTrue(other.Claims().empty() &&
                     Species::HolderOf(other, Species::CLAIM_AREA_CENTRAL) == std::optional<std::uint8_t>(std::uint8_t{0}),
                   L"a won battle settles every claim on it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp054TributeEasesAGrievanceAtTheRecipientsRate)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-054")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-057")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-064")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp054TributeEasesAGrievanceAtTheRecipientsRate)
  {
    // ADR-160: each mvp.relation.tribute_ru_per_weight eases a grievance by a unit, at the standard rate for a Bold
    // Cladan, half for a Territorial and twice for a Wary; no more is paid than eases it whole, and the recipient keeps
    // what its stores have room for. A tribute names a grievance held against its sender, and the sender pays it.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& steward = *std::ranges::find(region.Settlements(), STEWARD_SITE, &Species::Settlement::site);
    steward.storesMilliRu = 2'000'000;
    const double ruPerWeight = CommittedTuning().mvp.relation.tributeRuPerWeight;
    const std::int64_t attack = std::llround(CommittedTuning().mvp.grievance.attackWeight * 1e6);
    std::uint64_t subject = 100;
    for (const auto& [temperament, rate] : {std::pair{Species::Temperament::Bold, 1.0}, std::pair{Species::Temperament::Territorial, 0.5},
                                            std::pair{Species::Temperament::Wary, 2.0}})
    {
      const std::uint8_t site = SiteOf(region, temperament);
      Species::AddGrievance(region, site, STEWARD_SITE, Species::GrievanceCause::Attack, ++subject);
      const std::int64_t before = region.SettlementAt(site)->storesMilliRu;
      const auto paid = Species::SendTribute(region, STEWARD_SITE, site, Species::GrievanceCause::Attack, subject, 100'000);
      Assert::IsTrue(paid.value_or(0) == 100'000, L"pays the amount");
      Assert::AreEqual(static_cast<double>(attack) - 100.0 / ruPerWeight * rate * 1e6,
                       static_cast<double>(GrievanceOf(region, site, STEWARD_SITE, Species::GrievanceCause::Attack, subject)->weightMicro),
                       1.0, L"eased at the temperament's rate");
      ExpectEqual(before + 100'000, region.SettlementAt(site)->storesMilliRu, L"the recipient keeps it");
    }
    // No more than eases it whole.
    const std::uint8_t wary = SiteOf(region, Species::Temperament::Wary);
    const std::int64_t stores = steward.storesMilliRu;
    const auto rest = Species::SendTribute(region, STEWARD_SITE, wary, Species::GrievanceCause::Attack, subject, 1'000'000);
    Assert::IsTrue(rest.value_or(0) == 100'000 && steward.storesMilliRu == stores - 100'000, L"pays only what eases the rest");
    ExpectEqual(0, GrievanceOf(region, wary, STEWARD_SITE, Species::GrievanceCause::Attack, subject)->weightMicro, L"eased whole");
    // Its refusals, in order.
    const auto refused = [&](std::uint8_t _from, std::uint64_t _subject, std::int64_t _amount)
    {
      return Species::SendTribute(region, _from, wary, Species::GrievanceCause::Attack, _subject, _amount)
        .error_or(Species::TributeRefusal::NoSettlement);
    };
    Assert::IsTrue(refused(STEWARD_SITE, subject + 50, 1'000) == Species::TributeRefusal::NoGrievance, L"no such grievance");
    Species::AddGrievance(region, wary, STEWARD_SITE, Species::GrievanceCause::Attack, 999);
    Assert::IsTrue(refused(STEWARD_SITE, 999, 0) == Species::TributeRefusal::NoAmount, L"no amount");
    Assert::IsTrue(refused(STEWARD_SITE, 999, steward.storesMilliRu + 1) == Species::TributeRefusal::CannotAfford, L"beyond the stores");
    const std::unique_ptr<Species::WorldSimulation> alone = SettledWorld(STEWARD_SITE);
    const auto unsent = Species::SendTribute(alone->Region(0), 0, STEWARD_SITE, Species::GrievanceCause::Attack, 1, 1'000);
    Assert::IsTrue(!unsent.has_value() && unsent.error() == Species::TributeRefusal::NoSettlement, L"from no settlement");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101ClaimsWithdrawalsAndTributeAreCommandsWithTheirRefusals)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101ClaimsWithdrawalsAndTributeAreCommandsWithTheirRefusals)
  {
    // ADR-160: the settlement's claim, its withdrawal and its tribute are commands of the region's one path, permitted
    // for the team's commander, whose refusals name their ClaimRefusal or TributeRefusal.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.SetCommander(STEWARD_SITE, STEWARD, false);
    Reach(region, {NEIGHBOR_SITE});
    std::uint64_t sequence = 0;
    const auto executed = [&](const auto& _command)
    {
      Assert::IsTrue(region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(_command)), L"permitted");
      region.Commands().Execute(region.NextTick());
      const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
      ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"one outcome");
      return outcomes.front();
    };
    Neuron::CommandOutcome outcome = executed(Species::MakeClaimCommand{STEWARD_SITE, 0});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::NoEffect &&
                     outcome.reason == static_cast<std::uint8_t>(Species::ClaimRefusal::NotClaimable),
                   L"a half of another pair, refused with its reason");
    outcome = executed(Species::MakeClaimCommand{STEWARD_SITE, NEIGHBORS_HALF});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::Applied && region.Claims().size() == 1, L"a claim");
    const Species::StableId claim = region.Claims().front().id;
    outcome = executed(
      Species::SendTributeCommand{STEWARD_SITE, NEIGHBOR_SITE, static_cast<std::uint8_t>(Species::GrievanceCause::Claim), claim, 0});
    Assert::IsTrue(outcome.reason == static_cast<std::uint8_t>(Species::TributeRefusal::NoAmount), L"a tribute of nothing");
    outcome = executed(Species::WithdrawClaimCommand{STEWARD_SITE, claim + 1});
    Assert::IsTrue(outcome.reason == static_cast<std::uint8_t>(Species::ClaimRefusal::NoSuchClaim), L"no such claim");
    outcome = executed(Species::WithdrawClaimCommand{STEWARD_SITE, claim});
    Assert::IsTrue(outcome.effect == Neuron::CommandEffect::Applied && region.Claims().empty(), L"withdrawn");
    Assert::IsFalse(region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(Species::MakeClaimCommand{0, NEIGHBORS_HALF})),
                    L"another settlement's claim is not the steward's to make");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095ClaimsAndMovedAreasAreSavedAndAVersion11RegionHasNone)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095ClaimsAndMovedAreasAreSavedAndAVersion11RegionHasNone)
  {
    // ADR-160: the snapshot holds the claims, the moved areas and the move's territory event, and the restored territories
    // keep the moved half; a region of version 11 reads with none of them.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    Reach(region, {STEWARD_SITE, NEIGHBOR_SITE});
    Species::SettleClaim(region, Species::MakeClaim(region, STEWARD_SITE, NEIGHBORS_HALF).value_or(0), true);
    Assert::IsTrue(Species::MakeClaim(region, 0, Species::CLAIM_AREA_CENTRAL).has_value() &&
                     Species::MakeClaim(region, NEIGHBOR_SITE, STEWARDS_HALF).has_value(),
                   L"a waiting claim and an open one");
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    const Species::RegionSimulation& copy = back->Region(0);
    Assert::IsTrue(copy.Claims() == region.Claims() && copy.Claims().size() == 2 && copy.Holdings() == region.Holdings() &&
                     copy.ContactEvents() == region.ContactEvents(),
                   L"the claims, the moved half and its event");
    ExpectEqual(static_cast<std::int64_t>(CellsIn(region, STEWARD_SITE, NEIGHBORS_HALF)),
                static_cast<std::int64_t>(CellsIn(copy, STEWARD_SITE, NEIGHBORS_HALF)), L"the restored territory keeps the moved half");

    // Version 11's layout, with no territory event: the current one without the counts of the later sections.
    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld();
    const std::vector<std::byte> older = RegionStateOf(plain->Region(0), 11);
    Species::RegionSimulation eleven(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), CommittedTuning(), ValleyTerrain(), &TestRecords());
    Neuron::BitReader reader(older);
    Assert::IsTrue(eleven.ReadState(reader, 11) && reader.Finish(), L"a version-11 region reads whole");
    Assert::IsTrue(eleven.Claims().empty() && eleven.Holdings().empty() && eleven.Settlements().size() == 4, L"with no claim");
  }
};

} // namespace GameLogicTests
