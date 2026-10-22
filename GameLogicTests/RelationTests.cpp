#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr double SECONDS_PER_HOUR = 3'600.0;
// The tolerance of a weight after hours of the relations' steps, which round each to a millionth.
constexpr double WEIGHT_TOLERANCE = 0.005;

// The ticks of one second of world time.
std::uint64_t TicksPerSecond()
{
  return static_cast<std::uint64_t>(std::llround(1.0 / CommittedTuning().ref.sim.tickPeriod));
}

// The relations' steps of _seconds from second _first on.
void AdvanceSeconds(Species::RegionSimulation& _region, std::uint64_t _first, std::uint64_t _seconds)
{
  for (std::uint64_t second = _first; second < _first + _seconds; ++second)
  {
    Species::AdvanceRelations(_region, second * TicksPerSecond());
  }
}

// Each of _sites's territories at its largest, so that valley neighbours among them come into contact.
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

// The settlement at _site back at its founding radius.
void Withdraw(Species::RegionSimulation& _region, std::uint8_t _site)
{
  for (Species::Settlement& settlement : _region.Settlements())
  {
    if (settlement.site == _site)
    {
      settlement.territoryRadius = CommittedTuning().wld.territory.foundingRadius;
      _region.RecomputeTerritory(_site);
    }
  }
}

double Weight(std::int64_t _micro)
{
  return static_cast<double>(_micro) / static_cast<double>(Species::WEIGHT_MICRO);
}

// The grievance of the key in _region, or none.
const Species::Grievance* Find(const Species::RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against,
                               Species::GrievanceCause _cause, std::uint64_t _subject = 0)
{
  for (const Species::Grievance& grievance : _region.Grievances())
  {
    if (grievance.holder == _holder && grievance.against == _against && grievance.cause == _cause && grievance.subject == _subject)
    {
      return &grievance;
    }
  }
  return nullptr;
}

// What an hour leaves of a weight at mvp.grievance.fade_share_per_hour, _factor times faster.
double KeptOverAnHour(double _factor = 1.0)
{
  return std::pow(1.0 - CommittedTuning().mvp.grievance.fadeSharePerHour / 100.0, _factor);
}

} // namespace

TEST_CLASS(RelationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp051TerritoryPressureGrowsWhileInContactAndItsCauseEndsWithIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-027")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-050")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-051")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-052")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-122")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp051TerritoryPressureGrowsWhileInContactAndItsCauseEndsWithIt)
  {
    // ADR-159: while two valley neighbours are in contact, each one's Cladan gains mvp.grievance.pressure_per_hour a
    // hour against the other, less the fade; the pressure's cause ends with the contact, after which the grievance fades
    // pil.conflict.settled_fade_factor times faster, and it ends below half a unit. The world's step runs the relations
    // at each whole second.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsFalse(Species::InContact(region, 2, 3), L"no contact at the founding radius");
    Reach(region, {2, 3});
    Assert::IsTrue(Species::InContact(region, 2, 3) && Species::InContact(region, 3, 2), L"sites 2 and 3 in contact");
    Assert::IsFalse(Species::InContact(region, 0, 1) || Species::InContact(region, 2, 0) || Species::InContact(region, 1, 2),
                    L"no other pair, and sites 1 and 2 are no valley neighbours");

    // An hour of seconds: per second w ← (w + g)(1 − f), whose sum from nothing is g(1 − f)(1 − (1 − f)^n) / f.
    AdvanceSeconds(region, 1, 3'600);
    const auto& tuning = CommittedTuning();
    const double gain = tuning.mvp.grievance.pressurePerHour / SECONDS_PER_HOUR;
    const double fade = 1.0 - std::pow(KeptOverAnHour(), 1.0 / SECONDS_PER_HOUR);
    const double expected = gain * (1.0 - fade) * (1.0 - KeptOverAnHour()) / fade;
    for (const auto [holder, against] : {std::pair<std::uint8_t, std::uint8_t>{2, 3}, std::pair<std::uint8_t, std::uint8_t>{3, 2}})
    {
      const Species::Grievance* pressure = Find(region, holder, against, Species::GrievanceCause::Pressure);
      Assert::IsTrue(pressure != nullptr && !pressure->causeRemoved, L"a pressure grievance each way, its cause in place");
      Assert::AreEqual(expected, Weight(pressure != nullptr ? pressure->weightMicro : 0), WEIGHT_TOLERANCE, L"an hour of pressure");
      Assert::AreEqual(expected, Weight(Species::AngerMicro(region, holder, against)), WEIGHT_TOLERANCE, L"the anger is its weight");
    }
    ExpectEqual(2, static_cast<std::int64_t>(region.Grievances().size()), L"no other grievance");
    ExpectEqual(0, Species::AngerMicro(region, 2, 0), L"no anger without a grievance");

    // A tick that starts no second runs no step.
    const std::int64_t before = region.Grievances().front().weightMicro;
    Species::AdvanceRelations(region, 3'601 * TicksPerSecond() + 1);
    ExpectEqual(before, region.Grievances().front().weightMicro, L"no step between seconds");

    // The contact ends: the cause is removed and an hour leaves the weight's share at the settled rate.
    Withdraw(region, 3);
    Assert::IsFalse(Species::InContact(region, 2, 3), L"no longer in contact");
    AdvanceSeconds(region, 3'601, 3'600);
    const Species::Grievance* settled = Find(region, 2, 3, Species::GrievanceCause::Pressure);
    Assert::IsTrue(settled != nullptr && settled->causeRemoved, L"its cause removed");
    Assert::AreEqual(Weight(before) * KeptOverAnHour(tuning.pil.conflict.settledFadeFactor), Weight(settled->weightMicro), WEIGHT_TOLERANCE,
                     L"faded at the settled rate");
    AdvanceSeconds(region, 7'201, std::uint64_t{15} * 3'600);
    Assert::IsTrue(region.Grievances().empty(), L"a grievance whose cause is removed ends below half a unit");

    // The world's own step runs the relations at each whole second.
    const std::unique_ptr<Species::WorldSimulation> stepped = ValleyWorld();
    const std::unique_ptr<Species::WorldSimulation> direct = ValleyWorld();
    Reach(stepped->Region(0), {2, 3});
    Reach(direct->Region(0), {2, 3});
    Neuron::WorkerPool pool(0);
    for (std::uint64_t tick = 0; tick < 2 * TicksPerSecond(); ++tick)
    {
      stepped->Step(tick, pool);
    }
    Species::AdvanceRelations(direct->Region(0), 0);
    Species::AdvanceRelations(direct->Region(0), TicksPerSecond());
    Assert::IsTrue(!direct->Region(0).Grievances().empty() && stepped->Region(0).Grievances() == direct->Region(0).Grievances(),
                   L"two seconds of the world's step are two of the relations' steps");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp051AClaimAndAnAttackGiveTheirWeights)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-050")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-051")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp051AClaimAndAnAttackGiveTheirWeights)
  {
    // ADR-159: a claim gives the holder's Cladan mvp.grievance.claim_weight against the claimant, an attack
    // mvp.grievance.attack_weight against the attacker, mvp.grievance.unprovoked_factor times as much unprovoked; the
    // anger is the sum, and the same cause naming the same subject adds to its grievance.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    const auto& grievance = CommittedTuning().mvp.grievance;
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Claim, 7);
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 11);
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 12, true);
    Species::AddGrievance(region, 1, 0, Species::GrievanceCause::Attack, 13);
    Assert::AreEqual(grievance.claimWeight, Weight(Find(region, 0, 1, Species::GrievanceCause::Claim, 7)->weightMicro), 1e-9, L"a claim");
    Assert::AreEqual(grievance.attackWeight, Weight(Find(region, 0, 1, Species::GrievanceCause::Attack, 11)->weightMicro), 1e-9,
                     L"an attack");
    const Species::Grievance* unprovoked = Find(region, 0, 1, Species::GrievanceCause::Attack, 12);
    Assert::IsTrue(unprovoked != nullptr && unprovoked->unprovoked, L"marked unprovoked");
    Assert::AreEqual(grievance.attackWeight * grievance.unprovokedFactor, Weight(unprovoked->weightMicro), 1e-9, L"an unprovoked attack");
    Assert::AreEqual(grievance.claimWeight + grievance.attackWeight * (1.0 + grievance.unprovokedFactor),
                     Weight(Species::AngerMicro(region, 0, 1)), 1e-9, L"the anger is the sum");
    Assert::AreEqual(grievance.attackWeight, Weight(Species::AngerMicro(region, 1, 0)), 1e-9, L"each Cladan's own");
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Claim, 7);
    Assert::AreEqual(2.0 * grievance.claimWeight, Weight(Find(region, 0, 1, Species::GrievanceCause::Claim, 7)->weightMicro), 1e-9,
                     L"the same claim adds to its grievance");
    ExpectEqual(4, static_cast<std::int64_t>(region.Grievances().size()), L"one grievance for each cause and subject");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp052AGrievanceLosesItsShareEachHourAndFasterOnceItsCauseIsRemoved)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-052")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-122")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp052AGrievanceLosesItsShareEachHourAndFasterOnceItsCauseIsRemoved)
  {
    // ADR-159: an hour leaves (1 − mvp.grievance.fade_share_per_hour) of a grievance's weight while its cause remains,
    // and that share to the pil.conflict.settled_fade_factor power once it is removed; removing one claim's cause leaves
    // another's.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    const auto& tuning = CommittedTuning();
    Species::AddGrievance(region, 3, 1, Species::GrievanceCause::Claim, 21);
    Species::AddGrievance(region, 3, 1, Species::GrievanceCause::Claim, 22);
    AdvanceSeconds(region, 1, 3'600);
    const double held = tuning.mvp.grievance.claimWeight * KeptOverAnHour();
    Assert::AreEqual(held, Weight(Find(region, 3, 1, Species::GrievanceCause::Claim, 21)->weightMicro), WEIGHT_TOLERANCE,
                     L"an hour with its cause");
    Species::RemoveCause(region, Species::GrievanceCause::Claim, 21);
    Assert::IsTrue(Find(region, 3, 1, Species::GrievanceCause::Claim, 21)->causeRemoved &&
                     !Find(region, 3, 1, Species::GrievanceCause::Claim, 22)->causeRemoved,
                   L"only the named claim's cause");
    AdvanceSeconds(region, 3'601, 3'600);
    Assert::AreEqual(held * KeptOverAnHour(tuning.pil.conflict.settledFadeFactor),
                     Weight(Find(region, 3, 1, Species::GrievanceCause::Claim, 21)->weightMicro), WEIGHT_TOLERANCE,
                     L"an hour once its cause is removed");
    Assert::AreEqual(held * KeptOverAnHour(), Weight(Find(region, 3, 1, Species::GrievanceCause::Claim, 22)->weightMicro), WEIGHT_TOLERANCE,
                     L"the other's cause remains");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp052ABattleResultRelievesTheAttackersGrievancesAgainstTheTarget)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-052")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp052ABattleResultRelievesTheAttackersGrievancesAgainstTheTarget)
  {
    // ADR-159: a battle result removes mvp.grievance.battle_relief_won of the attacker's grievances against the target
    // when it won and mvp.grievance.battle_relief_lost when it lost; the target's grievances and the attacker's against
    // another settlement keep their weight.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    const auto& grievance = CommittedTuning().mvp.grievance;
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Claim, 31);
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 32);
    Species::AddGrievance(region, 0, 3, Species::GrievanceCause::Claim, 33);
    Species::AddGrievance(region, 1, 0, Species::GrievanceCause::Attack, 34);
    const double anger = Weight(Species::AngerMicro(region, 0, 1));
    Species::RelieveAfterBattle(region, 0, 1, true);
    const double won = anger * (1.0 - grievance.battleReliefWon / 100.0);
    Assert::AreEqual(won, Weight(Species::AngerMicro(region, 0, 1)), 1e-6, L"a won battle's relief");
    Species::RelieveAfterBattle(region, 0, 1, false);
    Assert::AreEqual(won * (1.0 - grievance.battleReliefLost / 100.0), Weight(Species::AngerMicro(region, 0, 1)), 1e-6,
                     L"a lost battle's relief");
    Assert::AreEqual(grievance.claimWeight, Weight(Species::AngerMicro(region, 0, 3)), 1e-9, L"against another settlement, none");
    Assert::AreEqual(grievance.attackWeight, Weight(Species::AngerMicro(region, 1, 0)), 1e-9, L"the target's own, none");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095GrievancesAreSavedAndAVersion10RegionHasNone)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095GrievancesAreSavedAndAVersion10RegionHasNone)
  {
    // ADR-159: the snapshot holds the grievances and the restored world steps alike; a region of version 10, saved before
    // grievances existed, reads with none, so the owner's M2 world continues (O98).
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    Reach(region, {2, 3});
    Species::AddGrievance(region, 0, 1, Species::GrievanceCause::Attack, 41, true);
    Neuron::WorkerPool pool(0);
    for (std::uint64_t tick = 0; tick < 2 * TicksPerSecond(); ++tick)
    {
      world->Step(tick, pool);
    }
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    if (!restored)
    {
      return;
    }
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(back->Region(0).Grievances() == region.Grievances() && region.Grievances().size() == 3, L"the grievances restored");
    for (std::uint64_t tick = 2 * TicksPerSecond(); tick < 4 * TicksPerSecond(); ++tick)
    {
      world->Step(tick, pool);
      back->Step(tick, pool);
    }
    Assert::IsTrue(Species::HashWorld(*back).world == Species::HashWorld(*world).world, L"and steps alike");

    // Version 10's layout is version 11's without the grievances' count, which ends the region's state.
    const std::unique_ptr<Species::WorldSimulation> plain = ValleyWorld();
    Neuron::BitWriter current;
    plain->Region(0).WriteState(current);
    const std::size_t bits = current.BitCount() - Neuron::BitsFor(65'535);
    Neuron::BitReader whole(current.Bytes());
    Neuron::BitWriter older;
    for (std::size_t done = 0; done < bits;)
    {
      const auto chunk = static_cast<std::uint32_t>(std::min<std::size_t>(64, bits - done));
      older.WriteBits(whole.ReadBits(chunk), chunk);
      done += chunk;
    }
    Species::RegionSimulation copy(0, SETTLED_TEST_KEY, plain->Region(0).NextTick(), CommittedTuning(), ValleyTerrain(), &TestRecords());
    Neuron::BitReader reader(older.Bytes());
    Assert::IsTrue(copy.ReadState(reader, 10) && reader.Finish(), L"a version-10 region reads whole");
    Assert::IsTrue(copy.Grievances().empty() && copy.Settlements().size() == 4, L"with its settlements and no grievance");
  }
};

} // namespace GameLogicTests
