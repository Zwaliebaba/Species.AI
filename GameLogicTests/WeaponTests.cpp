#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "EssenceOrb.h"
#include "RegionSimulation.h"
#include "TestSupport.h"
#include "Weapons.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey WEAPON_TEST_KEY{0x3ea9017e57000001, 0x3ea9017e57000002};
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};

Species::RegionSimulation FlatRegion()
{
  return Species::RegionSimulation(0, WEAPON_TEST_KEY, 0, CommittedTuning(), FlatTerrain(50.0));
}

Species::ActorRef SpawnAt(Species::RegionSimulation& _region, std::uint8_t _team, Species::FigureKind _kind, const Neuron::Vec3& _position)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.kind = _kind;
  spawn.position = _position;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  Species::Figure& figure = _region.TeamAt(_team).figures.At(slot);
  figure.position = _position;
  figure.onGround = true;
  return {figure.id, slot, _team};
}

Species::Effect& OnlyEffect(Species::RegionSimulation& _region)
{
  auto& effects = _region.Effects();
  for (std::uint32_t slot = 0; slot < effects.Capacity(); ++slot)
  {
    if (effects.Used(slot))
    {
      return effects.At(slot);
    }
  }
  Assert::Fail(L"no effect");
}

Species::LaserBolt& OnlyBolt(Species::RegionSimulation& _region)
{
  auto& bolts = _region.LaserBolts();
  for (std::uint32_t slot = 0; slot < bolts.Capacity(); ++slot)
  {
    if (bolts.Used(slot))
    {
      return bolts.At(slot);
    }
  }
  Assert::Fail(L"no bolt");
}

} // namespace

TEST_CLASS(WeaponTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate406AGrenadesExplosionFallsOffToItsReach)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-406")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-388")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-032")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate406AGrenadesExplosionFallsOffToItsReach)
  {
    // AC-W-01: a grenade's explosion of range 12.5 and damage 50 takes −50 at 10 and 18.75 wu, −40 at 20, −8 at 24 and
    // nothing at 25 wu, which is not a candidate; the hits knock their Workers into the air, and the two enemy kills
    // count for the exploding team.
    Species::RegionSimulation region = FlatRegion();
    static_cast<void>(region.EnsureTeam(1));
    // Each on an axis, so that every distance is exact.
    const std::vector<double> distances{10.0, 18.75, 20.0, 24.0, 25.0};
    const std::vector<Neuron::Vec3> offsets{{10.0, 0.0, 0.0}, {0.0, 0.0, 18.75}, {-20.0, 0.0, 0.0}, {0.0, 0.0, -24.0}, {25.0, 0.0, 0.0}};
    std::vector<Species::ActorRef> victims;
    victims.reserve(offsets.size());
    for (const Neuron::Vec3& offset : offsets)
    {
      victims.push_back(SpawnAt(region, 1, Species::FigureKind::Worker, MIDDLE + offset));
    }
    static_cast<void>(region.EnsureTeam(0));
    ExpectEqual(2, Species::Explode(region, MIDDLE, 12.5, 50.0, 0), L"enemy Workers killed");
    ExpectEqual(2, region.TeamAt(0).kills, L"the exploding team's kills");
    const std::vector<std::int32_t> health{-1, -1, 10, 42, 50};
    for (std::size_t n = 0; n < victims.size(); ++n)
    {
      const Species::Figure& victim = *region.Resolve(victims[n]);
      if (health[n] < 0)
      {
        Assert::IsTrue(victim.dead, std::format(L"dead at {} wu", distances[n]).c_str());
      }
      else
      {
        ExpectEqual(health[n], victim.health, std::format(L"health at {} wu", distances[n]));
      }
      Assert::IsTrue(victim.onGround == (n == 4), L"knocked into the air, but not the one out of reach");
    }
    ExpectEqual(2, region.Orbs().Count(), L"an orb for each death");
    ExpectEqual(1, region.Effects().Count(), L"its shockwave ring");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate411AThrownWeaponRestsAfterItsNinthBounce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-411")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-385")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate411AThrownWeaponRestsAfterItsNinthBounce)
  {
    // AC-W-03: released at rest above flat ground, each bounce leaves 0.75 of the force, and after the ninth it rests.
    Species::RegionSimulation region = FlatRegion();
    Species::ThrowGrenade(region, 0, MIDDLE + Neuron::Vec3{0.0, 20.0, 0.0}, MIDDLE + Neuron::Vec3{0.0, 20.0, 0.0});
    Species::Effect& grenade = OnlyEffect(region);
    grenade.velocity = {};
    grenade.fuseTicks = 100000;
    int bounces = 0;
    double force = grenade.force;
    for (int advance = 0; advance < 2000; ++advance)
    {
      Assert::IsFalse(Species::AdvanceEffect(region, grenade), L"no explosion");
      if (grenade.force != force)
      {
        ++bounces;
        Assert::IsTrue(std::abs(grenade.force - force * 0.75) < 1e-12, L"0.75 of the force");
        force = grenade.force;
      }
    }
    ExpectEqual(9, bounces, L"bounces");
    const Neuron::Vec3 rest = grenade.position;
    Assert::IsFalse(Species::AdvanceEffect(region, grenade), L"still no explosion");
    Assert::IsTrue(grenade.position == rest && rest.y >= MIDDLE.y, L"at rest above the ground");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate415AGrenadeExplodesOnItsThirtiethAdvance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-415")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate415AGrenadeExplodesOnItsThirtiethAdvance)
  {
    // AC-W-04, counted in advances: the fuse counts down before the throw physics and ends the grenade on its 30th.
    Species::RegionSimulation region = FlatRegion();
    static_cast<void>(region.EnsureTeam(0));
    Species::ThrowGrenade(region, 0, MIDDLE + Neuron::Vec3{0.0, 2.0, 0.0}, MIDDLE + Neuron::Vec3{60.0, 0.0, 0.0});
    Species::Effect& grenade = OnlyEffect(region);
    int advances = 1;
    while (!Species::AdvanceEffect(region, grenade))
    {
      ++advances;
    }
    ExpectEqual(30, advances, L"advances");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate440ABoltReachesEightyButNotAHundred)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-440")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-443")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-444")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate440ABoltReachesEightyButNotAHundred)
  {
    // AC-W-06: a bolt at 150 wu/s straight at an enemy 80 wu away on flat ground hits it for 20 to 39, pushes it and is
    // harmless after; at 100 wu it does not hit.
    for (const double distance : {80.0, 100.0})
    {
      Species::RegionSimulation region = FlatRegion();
      static_cast<void>(region.EnsureTeam(0));
      const Species::ActorRef enemy = SpawnAt(region, 1, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{distance, 0.0, 0.0});
      Species::FireLaser(region, 0, MIDDLE + Neuron::Vec3{0.0, 3.0, 0.0}, {150.0, 0.0, 0.0});
      Species::LaserBolt& bolt = OnlyBolt(region);
      int advances = 1;
      while (!Species::AdvanceLaserBolt(region, bolt))
      {
        ++advances;
      }
      ExpectEqual(7, advances, L"six moves, removed on the seventh advance");
      const Species::Figure& victim = *region.Resolve(enemy);
      if (distance == 80.0)
      {
        Assert::IsTrue(victim.health >= 11 && victim.health <= 30, std::format(L"20 to 39 damage: health {}", victim.health).c_str());
        Assert::IsTrue(bolt.harmless && victim.velocity.y >= 7.0 && victim.velocity.x >= 0.0, L"harmless, and the victim pushed");
      }
      else
      {
        ExpectEqual(50, victim.health, L"untouched at 100 wu");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate441ABoltRicochetsOffTheGround)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-441")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate441ABoltRicochetsOffTheGround)
  {
    // R-CRATE-441 step 4: a bolt that meets the ground reflects about its normal and flies the rest of its step from the
    // contact, above the ground.
    Species::RegionSimulation region = FlatRegion();
    Species::FireLaser(region, 0, MIDDLE + Neuron::Vec3{0.0, 3.0, 0.0}, {120.0, -90.0, 0.0});
    Species::LaserBolt& bolt = OnlyBolt(region);
    Assert::IsFalse(Species::AdvanceLaserBolt(region, bolt), L"still flying");
    Assert::IsTrue(bolt.ricocheted && bolt.velocity.y == 90.0 && bolt.velocity.x == 120.0, L"reflected");
    // The contact a third of the way along the 15 wu step, then 10 wu along the reflection: 12 wu on in x, 6 above.
    Assert::IsTrue(std::abs(bolt.position.x - (MIDDLE.x + 12.0)) < 1e-9 && std::abs(bolt.position.y - (MIDDLE.y + 6.0)) < 1e-9,
                   L"above the ground, a full step on");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Crate470ARingHitsAThinBandForItsActiveTicks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-470")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Crate470ARingHitsAThinBandForItsActiveTicks)
  {
    // AC-W-10 for a ring of 2.5 s: 24 active ticks, removed on the 25th advance, and a figure 60 wu away is hit at the
    // radii 63 and 67 wu only, for 40 × 0.72 and 40 × 0.68; a Leader is not pushed.
    Species::RegionSimulation region = FlatRegion();
    static_cast<void>(region.EnsureTeam(0));
    const Species::ActorRef leader = SpawnAt(region, 1, Species::FigureKind::Leader, MIDDLE + Neuron::Vec3{60.0, 0.0, 0.0});
    Species::CreateShockwaveRing(region, MIDDLE, 2.5, 0);
    Species::Effect& ring = OnlyEffect(region);
    std::vector<std::int32_t> health;
    int advances = 1;
    while (!Species::AdvanceEffect(region, ring))
    {
      health.push_back(region.Resolve(leader)->health);
      ++advances;
    }
    ExpectEqual(25, advances, L"advances");
    ExpectEqual(255, health[5], L"not yet at radius 59");
    ExpectEqual(255 - 28, health[6], L"hit at radius 63");
    ExpectEqual(255 - 28 - 27, health[7], L"hit at radius 67");
    ExpectEqual(255 - 28 - 27, health.back(), L"and never again");
    Assert::IsTrue(region.Resolve(leader)->onGround, L"a Leader is not pushed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Unit318AnOrbBirthsFloatsAndAscends)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-318")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-319")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-023")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Unit318AnOrbBirthsFloatsAndAscends)
  {
    // An orb nothing takes is in Birth for 61 ticks, Floating for 301 to 500 and in Ascent for 901, above its floor until
    // it ascends, and is removed at the end (MVP-023: outside every territory).
    Species::RegionSimulation region = FlatRegion();
    Species::SpawnOrb(region, MIDDLE, {}, 0, 7);
    Species::EssenceOrb& orb = region.Orbs().At(0);
    Assert::IsTrue(orb.position.y == MIDDLE.y + 1.0 && orb.state == Species::OrbState::Birth && orb.timerTicks == 61, L"its start");
    int birth = 0;
    int floating = 0;
    int ascent = 0;
    int advances = 0;
    bool removed = false;
    while (!removed && advances < 3000)
    {
      const Species::OrbState state = orb.state;
      removed = Species::AdvanceOrb(region, orb);
      ++advances;
      (state == Species::OrbState::Birth ? birth : state == Species::OrbState::Floating ? floating : ascent) += 1;
      if (orb.state != Species::OrbState::Ascent)
      {
        Assert::IsTrue(orb.position.y >= MIDDLE.y + 2.0, L"above its floor");
      }
    }
    Assert::IsTrue(removed, L"removed");
    ExpectEqual(61, birth, L"Birth");
    Assert::IsTrue(floating >= 301 && floating <= 500, std::format(L"Floating {}", floating).c_str());
    ExpectEqual(901, ascent, L"Ascent");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095BoltsEffectsAndOrbsAreState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-218")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095BoltsEffectsAndOrbsAreState)
  {
    // A region with a bolt, a grenade, a ring, an orb and a kill reads back to the same state, and both step on alike.
    Species::RegionSimulation region = FlatRegion();
    static_cast<void>(SpawnAt(region, 0, Species::FigureKind::Worker, MIDDLE));
    static_cast<void>(SpawnAt(region, 1, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{0.0, 0.0, 200.0}));
    Species::FireLaser(region, 0, MIDDLE + Neuron::Vec3{0.0, 3.0, 0.0}, {0.0, 0.0, 150.0});
    Species::ThrowGrenade(region, 1, MIDDLE + Neuron::Vec3{0.0, 2.0, 200.0}, MIDDLE + Neuron::Vec3{0.0, 0.0, 120.0});
    Species::CreateShockwaveRing(region, MIDDLE + Neuron::Vec3{300.0, 0.0, 0.0}, 0.625, 1);
    Species::SpawnOrb(region, MIDDLE + Neuron::Vec3{-100.0, 0.0, 0.0}, {3.0, 0.0, 0.0}, 0, 9);
    region.TeamAt(1).kills = 4;
    region.Step(region.NextTick());
    Neuron::BitWriter before;
    region.WriteState(before);
    Species::RegionSimulation copy(0, WEAPON_TEST_KEY, region.NextTick(), CommittedTuning(), FlatTerrain(50.0));
    Neuron::BitReader reader(before.Bytes());
    Assert::IsTrue(copy.ReadState(reader, Species::WORLD_STATE_VERSION) && reader.Finish(), L"reads whole");
    for (int tick = 0; tick < 40; ++tick)
    {
      region.Step(region.NextTick());
      copy.Step(copy.NextTick());
    }
    Neuron::BitWriter left;
    Neuron::BitWriter right;
    region.WriteState(left);
    copy.WriteState(right);
    Assert::IsTrue(std::ranges::equal(left.Bytes(), right.Bytes()), L"the same state 40 ticks on");
    ExpectEqual(4, copy.TeamAt(1).kills, L"the kills");
  }
};

} // namespace GameLogicTests
