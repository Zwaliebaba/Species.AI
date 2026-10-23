#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "Geometry.h"
#include "RegionTerrain.h"
#include "TerritoryLook.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr std::uint8_t SITE = 1;

const Species::RegionTerrain& SeedOne()
{
  static const std::shared_ptr<const Species::RegionTerrain> TERRAIN = []
  {
    auto terrain = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(terrain.has_value(), L"seed 1 gives a region");
    return std::make_shared<const Species::RegionTerrain>(std::move(*terrain));
  }();
  return *TERRAIN;
}

double Cell()
{
  return CommittedTuning().mvp.territory.cellSize;
}

// The cell of the region raster under the site SITE's centre.
std::pair<std::int32_t, std::int32_t> SiteCell()
{
  const Species::RegionLayout& layout = SeedOne().layout;
  return {static_cast<std::int32_t>(std::floor((layout.sites[SITE].x - layout.originX) / Cell())),
          static_cast<std::int32_t>(std::floor((layout.sites[SITE].z - layout.originZ) / Cell()))};
}

// A replica holding the settlement of site SITE in the Cladan colour _color, and its territory's cells (_x, _z) of the
// region raster, in chunks as ADR-139 sends them.
Species::StateReplica::View Replica(std::uint8_t _color, const std::vector<std::pair<std::int32_t, std::int32_t>>& _cells)
{
  Species::StateReplica::View view;
  Species::ReplicatedSettlement settlement;
  settlement.site = SITE;
  settlement.team = SITE;
  settlement.color = _color;
  view.emplace(Species::SettlementObjectId(SITE), settlement);
  for (const auto& [x, z] : _cells)
  {
    const auto chunkX = static_cast<std::uint8_t>(x / Species::CELL_CHUNK_SIDE);
    const auto chunkZ = static_cast<std::uint8_t>(z / Species::CELL_CHUNK_SIDE);
    const auto entry = view.try_emplace(Species::CellChunkObjectId(Species::CellLayer::Territory, SITE, chunkX, chunkZ),
                                        Species::ReplicatedCells{Species::CellLayer::Territory, SITE, chunkX, chunkZ, 0});
    std::get<Species::ReplicatedCells>(entry.first->second).cells |=
      std::uint64_t{1} << ((z % Species::CELL_CHUNK_SIDE) * Species::CELL_CHUNK_SIDE + x % Species::CELL_CHUNK_SIDE);
  }
  return view;
}

// The cells of a square of _side cells from the site's cell.
std::vector<std::pair<std::int32_t, std::int32_t>> Square(std::int32_t _side)
{
  const auto [x0, z0] = SiteCell();
  std::vector<std::pair<std::int32_t, std::int32_t>> cells;
  for (std::int32_t z = 0; z < _side; ++z)
  {
    for (std::int32_t x = 0; x < _side; ++x)
    {
      cells.emplace_back(x0 + x, z0 + z);
    }
  }
  return cells;
}

const Neuron::QuadBatch* BatchOf(const std::vector<Neuron::QuadBatch>& _batches, Species::SceneTexture _texture)
{
  const auto it =
    std::ranges::find_if(_batches, [&](const Neuron::QuadBatch& _batch) { return _batch.texture == static_cast<std::uint16_t>(_texture); });
  return it == _batches.end() ? nullptr : &*it;
}

} // namespace

TEST_CLASS(TerritoryLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp084ACellsLineRunsHalfACellAroundIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp084ACellsLineRunsHalfACellAroundIt)
  {
    // One territory cell: four pieces between the midpoints to its four neighbours, at the line's lift over the terrain.
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(SITE, Square(1)), SeedOne());
    Assert::AreEqual(std::size_t{1}, look.Lines().size());
    const Species::TerritoryLook::Line& line = look.Lines().front();
    Assert::IsTrue(line.site == SITE, L"the settlement's site");
    Assert::AreEqual(std::size_t{4}, line.pieces.size());
    const Species::RegionLayout& layout = SeedOne().layout;
    const auto [x, z] = SiteCell();
    const Neuron::Vec3 center{layout.originX + (x + 0.5) * Cell(), 0.0, layout.originZ + (z + 0.5) * Cell()};
    const double lift = CommittedPresentation().mvp.territoryLine.lift;
    for (const auto& piece : line.pieces)
    {
      for (const Neuron::Vec3& end : piece)
      {
        Assert::AreEqual(Cell() / 2.0, Neuron::HorizontalDistance(end, center), 1e-9, L"half a cell from the cell's centre");
        Assert::AreEqual(SeedOne().field.Height(end.x, end.z) + lift, end.y, 1e-9, L"at the lift over the terrain");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp084ATerritorysLineIsClosed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp084ATerritorysLineIsClosed)
  {
    // A square of 5 by 5 cells with its centre cell free: the outer line and the inner one are closed, so every end is
    // the end of exactly two pieces, and their pieces run 4 × 5 cells outside and 4 × 1 inside, with a chamfer at each
    // corner.
    std::vector<std::pair<std::int32_t, std::int32_t>> cells = Square(5);
    const auto [x0, z0] = SiteCell();
    std::erase(cells, std::pair{x0 + 2, z0 + 2});
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(SITE, cells), SeedOne());
    const Species::TerritoryLook::Line& line = look.Lines().front();
    std::map<std::pair<double, double>, int> ends;
    double length = 0.0;
    for (const auto& piece : line.pieces)
    {
      for (const Neuron::Vec3& end : piece)
      {
        ++ends[{end.x, end.z}];
      }
      length += Neuron::HorizontalDistance(piece[0], piece[1]);
    }
    Assert::IsTrue(std::ranges::all_of(ends, [](const auto& _end) { return _end.second == 2; }), L"closed");
    // Outside: 4 sides of 4 cells and 4 chamfers of half a diagonal; inside: a diamond of 4 chamfers.
    const double chamfer = Cell() * std::sqrt(0.5);
    Assert::AreEqual(4.0 * 4.0 * Cell() + 4.0 * chamfer + 4.0 * chamfer, length, 1e-6);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp084TheLineIsTheLongBeamInTheCladansColor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-013")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp084TheLineIsTheLongBeamInTheCladansColor)
  {
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(3, Square(2)), SeedOne());
    std::vector<Neuron::QuadBatch> batches;
    look.AppendBatches(batches);
    const Neuron::QuadBatch* lines = BatchOf(batches, Species::SceneTexture::BeamLong);
    Assert::IsTrue(lines != nullptr && lines->pass == Neuron::ScenePass::Effects && lines->entry == "effect_additive", L"pass 10a");
    if (lines == nullptr)
    {
      return;
    }
    Assert::AreEqual(look.Lines().front().pieces.size(), lines->quads.size());
    const auto& territory = CommittedPresentation().mvp.territoryLine;
    const std::array<float, 4> team = Species::TeamColor(3);
    for (const Neuron::SceneQuad& quad : lines->quads)
    {
      Assert::IsTrue(quad.orientation == Neuron::QuadOrientation::Segment);
      Assert::AreEqual(static_cast<float>(territory.halfWidth), quad.halfWidth);
      Assert::IsTrue(quad.color[0] == team[0] && quad.color[1] == team[1] && quad.color[2] == team[2], L"the Cladan's colour");
      Assert::AreEqual(static_cast<float>(territory.alpha), quad.color[3]);
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr143ALineFollowsItsSettlementsChunks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr143ALineFollowsItsSettlementsChunks)
  {
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(SITE, Square(1)), SeedOne());
    Assert::AreEqual(std::size_t{4}, look.Lines().front().pieces.size());
    look.Update(Replica(SITE, Square(3)), SeedOne());
    Assert::AreEqual(std::size_t{12}, look.Lines().front().pieces.size(), L"a grown territory, built again");
    look.Update(Replica(2, Square(3)), SeedOne());
    Assert::IsTrue(look.Lines().front().color == 2, L"the colour follows the settlement");
    look.Update({}, SeedOne());
    Assert::IsTrue(look.Lines().empty(), L"a settlement gone takes its line");
    Assert::IsFalse(look.BorderSpots().empty(), L"the border areas stay");
    std::vector<Neuron::QuadBatch> batches;
    look.AppendBatches(batches);
    Assert::IsTrue(batches.empty(), L"with no settlement, as in the sandbox, nothing is drawn");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld062TheBorderAreasSpotsMarkTheirLensesAndHalfwayLines)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-062")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld062TheBorderAreasSpotsMarkTheirLensesAndHalfwayLines)
  {
    // Every spot lies outside the coast bands, at the spot's size over the terrain, and on a valley pair's halfway line
    // inside both discs, or on one disc's circle inside the other.
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(SITE, Square(1)), SeedOne());
    const Species::RegionLayout& layout = SeedOne().layout;
    const double radius = CommittedTuning().wld.territory.maxRadius;
    const double band = CommittedTuning().wld.region.coastBand;
    const auto& mark = CommittedPresentation().mvp.borderMark;
    Assert::IsTrue(look.BorderSpots().size() > 100, L"four lenses of spots");
    std::size_t onLines = 0;
    for (const Species::TerritoryLook::BorderSpot& border : look.BorderSpots())
    {
      const Neuron::Vec3& spot = border.position;
      Assert::IsTrue(spot.x >= layout.originX + band && spot.x <= layout.originX + layout.side - band && spot.z >= layout.originZ + band &&
                       spot.z <= layout.originZ + layout.side - band,
                     L"outside the coast bands");
      Assert::AreEqual(SeedOne().field.Height(spot.x, spot.z) + mark.spotSize, spot.y, 1e-9);
      bool placed = false;
      for (std::size_t index = 0; index < Species::VALLEY_PAIRS.size(); ++index)
      {
        const auto& pair = Species::VALLEY_PAIRS[index];
        const double first = Neuron::HorizontalDistance(spot, layout.sites[pair[0]]);
        const double second = Neuron::HorizontalDistance(spot, layout.sites[pair[1]]);
        const bool halfway = std::abs(first - second) < 1e-6 && first <= radius + 1e-6;
        // ADR-166: the arc about the first site bounds the half nearer the second, area index × 2 + 1, and the other way.
        const bool nearSecond = std::abs(first - radius) < 1e-6 && second <= radius + 1e-6;
        const bool nearFirst = std::abs(second - radius) < 1e-6 && first <= radius + 1e-6;
        onLines += halfway ? 1 : 0;
        placed = placed || (halfway && !border.area.has_value()) ||
                 (nearSecond && border.area == std::optional<std::uint8_t>(static_cast<std::uint8_t>(index * 2 + 1))) ||
                 (nearFirst && border.area == std::optional<std::uint8_t>(static_cast<std::uint8_t>(index * 2)));
      }
      Assert::IsTrue(placed, L"on a lens's edge, naming its half, or its halfway line");
    }
    Assert::IsTrue(onLines > 0, L"the halfway lines are marked");
    std::vector<Neuron::QuadBatch> batches;
    look.AppendBatches(batches);
    const Neuron::QuadBatch* spots = BatchOf(batches, Species::SceneTexture::ParticleDot);
    Assert::IsTrue(spots != nullptr && spots->pass == Neuron::ScenePass::Effects && spots->entry == "effect_additive" &&
                     spots->quads.size() == look.BorderSpots().size(),
                   L"pass 10a, depth-tested, a spot each");
    if (spots != nullptr && !spots->quads.empty())
    {
      const Neuron::SceneQuad& quad = spots->quads.front();
      Assert::IsTrue(quad.orientation == Neuron::QuadOrientation::CameraFacing);
      Assert::AreEqual(static_cast<float>(mark.spotSize), quad.halfWidth);
      Assert::AreEqual(static_cast<float>(mark.color.r), quad.color[0]);
      Assert::AreEqual(static_cast<float>(mark.alpha), quad.color[3]);
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp053AHalfsEdgeTakesItsHoldersOrItsClaimantsColor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-053")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp053AHalfsEdgeTakesItsHoldersOrItsClaimantsColor)
  {
    // ADR-166: a half's edge spots take its holder's colour from the valley's status, its claimant's while a claim on it
    // stands, and the neutral mark's with no status or no holder; the halfway line stays neutral.
    Species::TerritoryLook look(CommittedPresentation(), CommittedTuning());
    look.Update(Replica(SITE, Square(1)), SeedOne());
    const auto& mark = CommittedPresentation().mvp.borderMark;
    const std::array<float, 4> neutral{static_cast<float>(mark.color.r), static_cast<float>(mark.color.g), static_cast<float>(mark.color.b),
                                       static_cast<float>(mark.alpha)};
    const auto spotOf = [&](std::optional<std::uint8_t> _area)
    { return *std::ranges::find(look.BorderSpots(), _area, &Species::TerritoryLook::BorderSpot::area); };
    Assert::IsTrue(look.SpotColor(spotOf(std::uint8_t{2})) == neutral, L"no status: neutral");
    Species::ValleyStatus valley;
    valley.areas[2].holder = std::uint8_t{2};
    valley.areas[3].holder = std::uint8_t{3};
    valley.claims.push_back({Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 5), 2, 3, 0, Species::ClaimState::Open});
    look.SetValley(valley);
    const auto colored = [&](std::uint8_t _team)
    {
      std::array<float, 4> color = Species::TeamColor(_team);
      color[3] *= static_cast<float>(mark.alpha);
      return color;
    };
    Assert::IsTrue(look.SpotColor(spotOf(std::uint8_t{2})) == colored(2), L"a held half in its holder's colour");
    Assert::IsTrue(look.SpotColor(spotOf(std::uint8_t{3})) == colored(2), L"a claimed half in its claimant's");
    Assert::IsTrue(look.SpotColor(spotOf(std::uint8_t{0})) == neutral, L"an unheld half neutral");
    Assert::IsTrue(look.SpotColor(spotOf(std::nullopt)) == neutral, L"and the halfway line");
    look.SetValley(std::nullopt);
    Assert::IsTrue(look.SpotColor(spotOf(std::uint8_t{2})) == neutral, L"neutral again with no status");
  }
};

} // namespace GameClientTests
