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
    const auto loaded = Species::LoadTuningMvp(RepositoryContentRoot());
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
    failsOnly(Species::TerrainCheck::Routes, [](Species::TuningMvp& _t) { _t.wld.travel.valleyPathFactor = 0.5; });
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
