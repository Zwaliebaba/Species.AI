#include "pch.h"

#include "RegionTerrain.h"
#include "Sha256.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

const Species::TuningMvp& Tuning()
{
  static const Species::TuningMvp COMMITTED = []
  {
    const auto loaded = Species::ReadTuningMvpSource(RepositoryContentRoot());
    Assert::IsTrue(loaded.has_value(), L"the committed tuning record reads whole");
    return loaded.value_or(Species::TuningMvp{});
  }();
  return COMMITTED;
}

// The region of seed 1 that passed, built once for the tests that read it.
const Species::RegionTerrain& SeedOne()
{
  static const Species::RegionTerrain REGION = []
  {
    auto generated = Species::GenerateRegionTerrain(1, Tuning());
    Assert::IsTrue(generated.has_value(), L"seed 1 gives a region that passes");
    return generated ? std::move(*generated) : Species::RegionTerrain{};
  }();
  return REGION;
}

bool Has(const std::vector<Species::TerrainCheck>& _checks, Species::TerrainCheck _check)
{
  return std::ranges::find(_checks, _check) != _checks.end();
}

} // namespace

TEST_CLASS(RegionTerrainTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld161WorldDrawsAreA1sKeyedValues)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-161")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld161WorldDrawsAreA1sKeyedValues)
  {
    // ADR-082 item 4: the chain of Tools/DesignModels/A1WorldDensity.py over (seed; 2, purpose, region, attempt, index).
    Assert::IsTrue(Species::WorldDraw(1, Species::WorldDrawPurpose::TileSeed, 0, 0, 0) == 0x2bbfee56a5171dbfULL, L"the keyed value");
    Assert::IsTrue(Species::WorldUnit(1, Species::WorldDrawPurpose::TileSeed, 0, 0, 0) == 0.17089738478997807, L"as a unit value");
    Assert::IsTrue(Species::WorldUnit(12345, Species::WorldDrawPurpose::ValleyOffset, 0, 0, 1) == 0.08277685548886748, L"another");
    Assert::IsTrue(Species::WorldDraw(1, Species::WorldDrawPurpose::TileSeed, 0, 1, 0) !=
                     Species::WorldDraw(1, Species::WorldDrawPurpose::TileSeed, 0, 0, 0),
                   L"the attempt is a key");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld040TheSitesStandInTheirPattern)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-040")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld040TheSitesStandInTheirPattern)
  {
    // The MVP's region is cell 6 of the extent; valley neighbours stand 2,080 wu apart and diagonals 2,942 wu, around a
    // centre within 64 wu of the region's, in local order.
    const Species::RegionLayout& layout = SeedOne().layout;
    Assert::IsTrue(layout.originX == 30720.0 && layout.originZ == 30720.0 && layout.side == 5120.0, L"region cell (6, 6)");
    Assert::IsTrue(std::abs(layout.valleyCenter.x - 33280.0) <= 64.0 && std::abs(layout.valleyCenter.z - 33280.0) <= 64.0,
                   L"the valley's offset");
    const auto distance = [&](std::size_t _a, std::size_t _b)
    { return std::hypot(layout.sites[_a].x - layout.sites[_b].x, layout.sites[_a].z - layout.sites[_b].z); };
    for (const auto& [a, b] : {std::pair<std::size_t, std::size_t>{0, 1}, {2, 3}, {0, 2}, {1, 3}})
    {
      Assert::IsTrue(std::abs(distance(a, b) - 2080.0) < 1e-9, L"valley neighbours");
    }
    Assert::IsTrue(std::abs(distance(0, 3) - 2080.0 * std::sqrt(2.0)) < 1e-9, L"a diagonal");
    Assert::IsTrue(layout.sites[1].x > layout.sites[0].x && layout.sites[2].z > layout.sites[0].z, L"local order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011OneSeedGivesOneTerrain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011OneSeedGivesOneTerrain)
  {
    // The server's call and the client's are one function of the seed and the tuning: the same heights and digest on
    // every run, and a different seed gives a different valley.
    const auto again = Species::GenerateRegionTerrain(1, Tuning());
    Assert::IsTrue(again.has_value() && again->digest == SeedOne().digest, L"the same digest");
    Assert::IsTrue(again->field.Digest() == SeedOne().field.Digest() && again->seed == SeedOne().seed, L"the same heights and seed");
    const auto other = Species::GenerateRegionTerrain(2, Tuning());
    Assert::IsTrue(other.has_value() && other->digest != SeedOne().digest, L"another seed, another valley");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld162TheCoresAndTheCenterAreLevel)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-162")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld162TheCoresAndTheCenterAreLevel)
  {
    // One base tile and two hills; each core and the valley centre levelled at its first pass's height, clamped to the
    // band, by a feathered square.
    const Species::RegionTerrain& region = SeedOne();
    ExpectEqual(3, static_cast<std::int64_t>(region.tiles.size()), L"the base tile and two hills");
    ExpectEqual(5, static_cast<std::int64_t>(region.areas.size()), L"four cores and the centre");
    for (std::size_t local = 0; local < 4; ++local)
    {
      const Neuron::Vec3& center = region.layout.sites[local];
      const double floor = Neuron::RoundToBinary32(region.layout.siteFloors[local]);
      Assert::IsTrue(floor >= 20.0 && floor <= 120.0, L"in the band");
      const auto& grid = region.field.Heights();
      Assert::IsTrue(region.field.HeightAt(grid.IndexX(center.x), grid.IndexZ(center.z)) == floor, L"the plateau's level");
    }
    Assert::IsTrue(region.areas.back().op == Neuron::FlattenOp::Feathered && region.areas.back().size == 120.0, L"the centre's square");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld163EachCheckRefusesWhatItGuards)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-163")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld163EachCheckRefusesWhatItGuards)
  {
    // The region passes every check; each fixture tightens one check's own threshold, and only that check fails.
    const Species::RegionTerrain& region = SeedOne();
    Assert::IsTrue(Species::CheckRegionTerrain(region, Tuning()).empty(), L"the region passes");
    const auto failsOnly = [&](Species::TerrainCheck _check, auto&& _tighten)
    {
      Species::TuningMvp tuning = Tuning();
      _tighten(tuning);
      const std::vector<Species::TerrainCheck> failed = Species::CheckRegionTerrain(region, tuning);
      Assert::IsTrue(failed.size() == 1 && Has(failed, _check), std::format(L"only check {}", static_cast<int>(_check)).c_str());
    };
    failsOnly(Species::TerrainCheck::Channels, [](Species::TuningMvp& _t) { _t.wld.region.channelWidth = 3000.0; });
    failsOnly(Species::TerrainCheck::Plateaus, [](Species::TuningMvp& _t) { _t.wld.site.coreMinNormalY = 1.5; });
    failsOnly(Species::TerrainCheck::Rims, [](Species::TuningMvp& _t) { _t.wld.site.rimStepMax = -1.0; });
    failsOnly(Species::TerrainCheck::DrySites, [](Species::TuningMvp& _t) { _t.wld.site.waterClearance = 2500.0; });
    failsOnly(Species::TerrainCheck::Reachability, [](Species::TuningMvp& _t) { _t.ref.sim.walk.minNormalY = 2.0; });
    failsOnly(Species::TerrainCheck::HomeYields, [](Species::TuningMvp& _t) { _t.wld.resource.homeYieldTolerance = -1.0; });
    failsOnly(Species::TerrainCheck::Routes, [](Species::TuningMvp& _t) { _t.wld.travel.valleyPathFactor = 0.5; });
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld163Check7RefusesUnequalHomeYields)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-163")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-081")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld163Check7RefusesUnequalHomeYields)
  {
    // Every site's nine home resource sites yield 9 units; a fixture that moves one of site 2's into the sea leaves it 8,
    // which is 11 % short of the others against a tolerance of 5 %, and one that drops a home site does the same. A
    // border resource site is no home resource site, so moving one changes nothing.
    const Species::RegionTerrain& region = SeedOne();
    Assert::IsTrue(Species::HomeYieldsAgree(region.resourceSites, region.field, Tuning()), L"the generated sites agree");
    std::vector<Species::ResourceSitePlace> wet = region.resourceSites;
    wet[2 * 11 + 4].position = {region.layout.originX + 10.0, -10.0, region.layout.originZ + 10.0};
    Assert::IsFalse(Species::HomeYieldsAgree(wet, region.field, Tuning()), L"a home site in the sea yields nothing");
    std::vector<Species::ResourceSitePlace> fewer = region.resourceSites;
    fewer.erase(fewer.begin() + 3);
    Assert::IsFalse(Species::HomeYieldsAgree(fewer, region.field, Tuning()), L"a site with one home site fewer");
    std::vector<Species::ResourceSitePlace> border = region.resourceSites;
    border[9].position = wet[2 * 11 + 4].position;
    Assert::IsTrue(Species::HomeYieldsAgree(border, region.field, Tuning()), L"a border resource site is not counted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld082TheResourceSitesStandInTheirPattern)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-082")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-085")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld082TheResourceSitesStandInTheirPattern)
  {
    // Per site, in order: three home sites at 350 wu at -60, 0 and +60 degrees from the outward diagonal, five at 800 wu
    // at -72 to +72 degrees, the ninth at 1,050 wu and +84 degrees (DEC-1000), and the two border resource sites at the
    // centroid of each half-lens, 894.3 wu towards each valley neighbour; then the central resource site at the valley's
    // centre. Every site holds 11 units of its own (WLD-085) and the central one 3; each faces its site's centre and
    // stands on the terrain.
    const Species::RegionTerrain& region = SeedOne();
    const Species::RegionLayout& layout = region.layout;
    const std::vector<Species::ResourceSitePlace>& places = region.resourceSites;
    ExpectEqual(4 * 11 + 1, static_cast<std::int64_t>(places.size()), L"eleven per site and the central one");
    constexpr std::array<std::pair<double, double>, 9> HOME{{{350.0, -60.0},
                                                             {350.0, 0.0},
                                                             {350.0, 60.0},
                                                             {800.0, -72.0},
                                                             {800.0, -36.0},
                                                             {800.0, 0.0},
                                                             {800.0, 36.0},
                                                             {800.0, 72.0},
                                                             {1050.0, 84.0}}};
    for (std::size_t site = 0; site < 4; ++site)
    {
      const Neuron::Vec3& center = layout.sites[site];
      const double outward = std::atan2(center.z - layout.valleyCenter.z, center.x - layout.valleyCenter.x);
      std::uint32_t units = 0;
      for (std::size_t k = 0; k < 11; ++k)
      {
        const Species::ResourceSitePlace& place = places[site * 11 + k];
        Assert::IsTrue(place.site == static_cast<std::uint8_t>(site), L"the site whose own it is");
        units += place.units;
        const double dx = place.position.x - center.x;
        const double dz = place.position.z - center.z;
        Assert::IsTrue(place.position.y == region.field.Height(place.position.x, place.position.z), L"on the terrain");
        Assert::IsTrue(std::abs(place.front.x * dx + place.front.z * dz + std::hypot(dx, dz)) < 1e-6, L"facing its site's centre");
        if (k < HOME.size())
        {
          Assert::IsTrue(place.siteClass == Species::ResourceSiteClass::Home && place.units == 1, L"a home resource site of one unit");
          Assert::IsTrue(std::abs(std::hypot(dx, dz) - HOME[k].first) < 1e-9, L"at its radius");
          const double bearing = std::remainder(std::atan2(dz, dx) - outward, 2.0 * std::numbers::pi) * 180.0 / std::numbers::pi;
          Assert::IsTrue(std::abs(bearing - HOME[k].second) < 1e-9, L"at its bearing");
        }
        else
        {
          Assert::IsTrue(place.siteClass == Species::ResourceSiteClass::Border && place.units == 1, L"a border resource site");
          Assert::IsTrue(std::abs(std::hypot(dx, dz) - 894.3) < 0.05, L"at the centroid of its half-lens");
          const Neuron::Vec3& neighbor = layout.sites[k == 9 ? site ^ 1U : site ^ 2U];
          const double across = (neighbor.x - center.x) * dz - (neighbor.z - center.z) * dx;
          Assert::IsTrue(std::abs(across) < 1e-6, L"on the line to its valley neighbour");
        }
      }
      ExpectEqual(11, units, L"wld.resource.own_units of its own");
    }
    const Species::ResourceSitePlace& central = places.back();
    Assert::IsTrue(central.siteClass == Species::ResourceSiteClass::Central && !central.site && central.units == 3, L"the central site");
    Assert::IsTrue(central.position.x == layout.valleyCenter.x && central.position.z == layout.valleyCenter.z, L"at the valley's centre");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld082EveryResourceSiteIsWalkableOnACorpusOfSeeds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-082")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-163")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld082EveryResourceSiteIsWalkableOnACorpusOfSeeds)
  {
    // On seeds 1 to 8 every resource site stands above the shore on walkable ground, at least 200 wu from every other, and
    // the region passes check 7 at its first attempt, so the placement never re-rolls a seed of M1 (DEC-1000 measures
    // seeds 1 to 100 the same way).
    const Species::TuningMvp& tuning = Tuning();
    for (std::uint64_t seed = 1; seed <= 8; ++seed)
    {
      const auto region = Species::GenerateRegionTerrain(seed, tuning);
      Assert::IsTrue(region.has_value() && region->seed == seed && region->attempt == 0, L"the seed's first attempt passes");
      const std::vector<Species::ResourceSitePlace>& places = region->resourceSites;
      for (std::size_t a = 0; a < places.size(); ++a)
      {
        const Neuron::Vec3& p = places[a].position;
        Assert::IsTrue(p.y > tuning.ref.unit.worker.shoreHeight, L"above the shore");
        Assert::IsTrue(region->field.Normal(p.x, p.z).y >= tuning.ref.sim.walk.minNormalY, L"on walkable ground");
        for (std::size_t b = a + 1; b < places.size(); ++b)
        {
          Assert::IsTrue(Neuron::HorizontalDistance(p, places[b].position) >= 200.0, L"200 wu from every other");
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld166TheDigestIsTheDigestOfItsChunks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-166")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld166TheDigestIsTheDigestOfItsChunks)
  {
    // 5 × 5 chunks of 64 × 64 samples, each the TER-252 digest of its block; the chunk beyond the region's edge is all sea
    // at the outside height, as every chunk outside a region is; the region's digest is SHA-256 of the chunk digests.
    const Species::RegionTerrain& region = SeedOne();
    ExpectEqual(25, static_cast<std::int64_t>(region.chunkDigests.size()), L"5 × 5 chunks");
    std::vector<std::byte> sea;
    for (int n = 0; n < 64 * 64; ++n)
    {
      const auto bits = std::bit_cast<std::uint64_t>(-10.0);
      for (int k = 0; k < 8; ++k)
      {
        sea.push_back(static_cast<std::byte>(bits >> (8 * k)));
      }
    }
    Assert::IsTrue(region.chunkDigests[0] == region.field.DigestOf(0, 0, 64, 64), L"chunk (0, 0)");
    Assert::IsTrue(region.chunkDigests[7] == region.field.DigestOf(128, 64, 64, 64), L"chunk (2, 1): x inner, z outer");
    Assert::IsTrue(region.field.DigestOf(320, 0, 64, 64) == Neuron::Sha256Of(sea), L"beyond the region, sea");
    Neuron::Sha256 all;
    for (const Neuron::Sha256Digest& chunk : region.chunkDigests)
    {
      all.Update(chunk);
    }
    Assert::IsTrue(all.Finish() == region.digest, L"the digest of the chunk digests");
    Assert::IsTrue(region.chunkDigests[12] != region.chunkDigests[0], L"the middle chunk is land");
  }
};

} // namespace GameSharedTests
