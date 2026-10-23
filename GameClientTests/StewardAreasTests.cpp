#include "pch.h"

#include "ReplicaTiming.h"
#include "StewardAreas.h"
#include "TestSupport.h"

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr std::uint8_t SITE = 2;
constexpr std::uint8_t TEAM = 2;
constexpr double CELL = 16.0;    // mvp.territory.cell_size
constexpr double MARGIN = 800.0; // wld.view.settlement_margin

// A region of 5,120 wu a side from (10,000, 20,000), as WLD-003's land cell has it.
Species::RegionLayout Layout()
{
  Species::RegionLayout layout;
  layout.originX = 10'000.0;
  layout.originZ = 20'000.0;
  layout.side = 5'120.0;
  return layout;
}

// The centre of the region raster's cell (_x, _z).
double CenterX(std::int32_t _x)
{
  return Layout().originX + (_x + 0.5) * CELL;
}
double CenterZ(std::int32_t _z)
{
  return Layout().originZ + (_z + 0.5) * CELL;
}

// A replica holding the settlement of site SITE and team _team, and its territory's cells (_x, _z) of the region raster.
Species::StateReplica::View Replica(std::uint8_t _team, std::initializer_list<std::pair<std::int32_t, std::int32_t>> _cells)
{
  Species::StateReplica::View view;
  Species::ReplicatedSettlement settlement;
  settlement.site = SITE;
  settlement.team = _team;
  view.emplace(Species::SettlementObjectId(SITE), settlement);
  for (const auto& [x, z] : _cells)
  {
    const auto chunkX = static_cast<std::uint8_t>(x / Species::CELL_CHUNK_SIDE);
    const auto chunkZ = static_cast<std::uint8_t>(z / Species::CELL_CHUNK_SIDE);
    const Species::StableId id = Species::CellChunkObjectId(Species::CellLayer::Territory, SITE, chunkX, chunkZ);
    auto& chunk = std::get<Species::ReplicatedCells>(
      view.try_emplace(id, Species::ReplicatedCells{Species::CellLayer::Territory, SITE, chunkX, chunkZ, 0}).first->second);
    chunk.cells |= std::uint64_t{1} << ((z % Species::CELL_CHUNK_SIDE) * Species::CELL_CHUNK_SIDE + x % Species::CELL_CHUNK_SIDE);
  }
  return view;
}

} // namespace

TEST_CLASS(StewardAreasTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244TheAreasAreTheTerritoryAndItsMargin)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244TheAreasAreTheTerritoryAndItsMargin)
  {
    // One territory cell: a cell is in the areas when its centre lies within 800 wu of that cell's centre.
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    Assert::IsTrue(areas.Known());
    Assert::IsTrue(areas.Site() == std::optional<std::uint8_t>(SITE));
    const double x = CenterX(100);
    const double z = CenterZ(120);
    Assert::IsTrue(areas.InTerritory(x, z) && areas.InTerritory(x + 7.0, z - 7.0), L"the territory's cell");
    Assert::IsFalse(areas.InTerritory(x + CELL, z), L"its neighbour is no territory");
    Assert::IsTrue(areas.Contains(x, z), L"the territory is in the areas");
    Assert::IsTrue(areas.Contains(x + MARGIN, z), L"the cell whose centre is 800 wu away");
    Assert::IsFalse(areas.Contains(x + MARGIN + CELL, z), L"and not the next");
    Assert::IsTrue(areas.Contains(x, z - MARGIN));
    // 50 cells along x and 50 along z is 70.7 cells, beyond the margin; 30 and 40 is exactly 50 cells.
    Assert::IsFalse(areas.Contains(x + 50 * CELL, z + 50 * CELL), L"the margin is a disc, not a square");
    Assert::IsTrue(areas.Contains(x + 30 * CELL, z - 40 * CELL), L"30² + 40² = 50²");
    Assert::IsFalse(areas.Contains(x + 31 * CELL, z - 40 * CELL));
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244TheNearestTerritoryCellDecides)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244TheNearestTerritoryCellDecides)
  {
    // Two territory cells 200 cells apart along x: a cell is in the areas when it lies within 50 cells of either.
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{20, 150}, {220, 150}}), TEAM, Layout(), CommittedTuning());
    Assert::IsTrue(areas.Contains(CenterX(265), CenterZ(150)));
    Assert::IsFalse(areas.Contains(CenterX(275), CenterZ(150)));
    Assert::IsFalse(areas.Contains(CenterX(120), CenterZ(150)), L"halfway, 100 cells from each, is outside");
    Assert::IsTrue(areas.Contains(CenterX(70), CenterZ(150)), L"50 cells from the first");
    Assert::IsTrue(areas.Contains(CenterX(170), CenterZ(150)), L"50 cells from the second");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244TheMarginReachesPastTheRegionsSquare)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244TheMarginReachesPastTheRegionsSquare)
  {
    // A territory cell at the square's low corner: its margin lies beyond the square, where the raster still reaches.
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{0, 0}}), TEAM, Layout(), CommittedTuning());
    Assert::IsTrue(areas.Contains(Layout().originX - 700.0, CenterZ(0)), L"700 wu beyond the square");
    Assert::IsFalse(areas.Contains(Layout().originX - 900.0, CenterZ(0)));
    Assert::AreEqual(320 + 2 * 50, areas.CellsPerAxis(), L"the region's cells and 50 a side");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244UnknownAreasHoldEveryPoint)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244UnknownAreasHoldEveryPoint)
  {
    // No team, another team's settlement, or no territory yet: unknown, so nothing bounds the camera but the region.
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{100, 120}}), std::nullopt, Layout(), CommittedTuning());
    Assert::IsFalse(areas.Known());
    areas.Update(Replica(TEAM, {{100, 120}}), std::uint8_t{1}, Layout(), CommittedTuning());
    Assert::IsFalse(areas.Known(), L"the territory of another team's settlement is not the steward's");
    areas.Update(Replica(TEAM, {}), TEAM, Layout(), CommittedTuning());
    Assert::IsFalse(areas.Known(), L"a settlement with no territory received");
    Assert::IsTrue(areas.Contains(-1e6, 1e6));
    Assert::IsFalse(areas.InTerritory(CenterX(100), CenterZ(120)));
    Assert::IsFalse(areas.Nearest(0.0, 0.0).has_value());
    Assert::AreEqual(std::uint64_t{0}, areas.Revision(), L"nothing was built");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr142TheAreasAreRebuiltOnlyWhenTheTerritoryChanges)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr142TheAreasAreRebuiltOnlyWhenTheTerritoryChanges)
  {
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    Assert::AreEqual(std::uint64_t{1}, areas.Revision());
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    Assert::AreEqual(std::uint64_t{1}, areas.Revision(), L"the same chunks");
    areas.Update(Replica(TEAM, {{100, 120}, {101, 120}}), TEAM, Layout(), CommittedTuning());
    Assert::AreEqual(std::uint64_t{2}, areas.Revision(), L"a cell more in the same chunk");
    Assert::IsTrue(areas.Contains(CenterX(101) + MARGIN, CenterZ(120)));
    areas.Clear();
    Assert::AreEqual(std::uint64_t{3}, areas.Revision(), L"forgotten");
    Assert::IsFalse(areas.Known());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244ABattlefieldJoinsTheAreasForItsBattle)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244ABattlefieldJoinsTheAreasForItsBattle)
  {
    // ADR-167: a battlefield far beyond the margin joins the areas while it is given, its cells those whose centres lie
    // within its radius of its centre; the areas are rebuilt when the fields change, and lose it when it ends.
    Species::StewardAreas areas;
    const Species::StateReplica::View replica = Replica(TEAM, {{100, 120}});
    areas.Update(replica, TEAM, Layout(), CommittedTuning());
    const double fieldX = CenterX(200);
    const double fieldZ = CenterZ(200);
    Assert::IsFalse(areas.Contains(fieldX, fieldZ), L"far from the territory");
    const std::vector<Species::StewardAreas::Field> fields{{fieldX, fieldZ, 300.0}};
    areas.Update(replica, TEAM, Layout(), CommittedTuning(), fields);
    Assert::AreEqual(std::uint64_t{2}, areas.Revision(), L"rebuilt for the battlefield");
    Assert::IsTrue(areas.Contains(fieldX, fieldZ), L"its centre");
    Assert::IsTrue(areas.Contains(fieldX + 290.0, fieldZ), L"within its radius");
    Assert::IsFalse(areas.Contains(fieldX + 320.0, fieldZ), L"beyond its radius");
    Assert::IsFalse(areas.InTerritory(fieldX, fieldZ), L"in the areas, not the territory");
    Assert::IsTrue(areas.Contains(CenterX(100) + MARGIN, CenterZ(120)), L"the territory's margin stays");
    areas.Update(replica, TEAM, Layout(), CommittedTuning(), fields);
    Assert::AreEqual(std::uint64_t{2}, areas.Revision(), L"the same fields");
    areas.Update(replica, TEAM, Layout(), CommittedTuning());
    Assert::AreEqual(std::uint64_t{3}, areas.Revision(), L"rebuilt when the battle ends");
    Assert::IsFalse(areas.Contains(fieldX, fieldZ), L"and the battlefield is gone");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr142APointOutsideGoesToTheNearestCellOfTheAreas)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr142APointOutsideGoesToTheNearestCellOfTheAreas)
  {
    Species::StewardAreas areas;
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    const double x = CenterX(100);
    const double z = CenterZ(120);
    const std::optional<Species::StewardAreas::Point> inside = areas.Nearest(x + 10.0, z + 3.0);
    Assert::IsTrue(inside.has_value() && inside->x == x + 10.0 && inside->z == z + 3.0, L"a point inside stays");
    const std::optional<Species::StewardAreas::Point> nearest = areas.Nearest(x + 2'000.0, z);
    Assert::IsTrue(nearest.has_value());
    Assert::AreEqual(x + MARGIN, nearest.value_or(Species::StewardAreas::Point{}).x, 1e-9, L"the cell 50 cells along x");
    Assert::AreEqual(z, nearest.value_or(Species::StewardAreas::Point{}).z, 1e-9);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr144TheViewMaskIsTheAreasWithASoftEdge)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr144TheViewMaskIsTheAreasWithASoftEdge)
  {
    Species::StewardAreas areas;
    Assert::IsTrue(Species::BuildViewMask(areas, CommittedPresentation()) == nullptr, L"no mask while the areas are unknown");
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    const auto mask = Species::BuildViewMask(areas, CommittedPresentation());
    Assert::IsTrue(mask != nullptr);
    if (mask == nullptr)
    {
      return;
    }
    Assert::AreEqual(static_cast<std::uint32_t>(areas.CellsPerAxis()), mask->side);
    Assert::AreEqual(areas.Origin().x, mask->originX);
    Assert::AreEqual(CELL, mask->cellSize);
    Assert::AreEqual(static_cast<float>(CommittedPresentation().mvp.viewFog.strength), mask->fogStrength);
    // The territory cell is at (100 + 50, 120 + 50) of the widened raster; the areas end 50 cells from it along x.
    const auto side = static_cast<std::size_t>(areas.CellsPerAxis());
    const auto at = [&](std::size_t _x, std::size_t _z) { return mask->cells[(120 + 50 + _z) * side + 100 + 50 + _x]; };
    Assert::AreEqual(255, static_cast<int>(at(0, 0)), L"clear inside");
    Assert::AreEqual(0, static_cast<int>(at(55, 0)), L"black outside");
    // mvp.view_fog.edge_width of 64 wu is a box of 5 cells: the areas' last cell along x holds 11 of its 25.
    Assert::AreEqual(112, static_cast<int>(at(50, 0)), L"the edge is a ramp");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244NoWorkerIsDrawnOutsideTheAreas)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244NoWorkerIsDrawnOutsideTheAreas)
  {
    std::vector<Species::InterpolatedFigure> figures(3);
    figures[0].position = {CenterX(100), 0.0, CenterZ(120)};
    figures[1].position = {CenterX(100) + 2 * MARGIN, 0.0, CenterZ(120)};
    figures[2].position = {CenterX(100) - MARGIN / 2.0, 0.0, CenterZ(120)};
    const auto placeOf = [](const Species::InterpolatedFigure& _figure) { return _figure.position; };
    Species::StewardAreas areas;
    std::vector<Species::InterpolatedFigure> all = figures;
    Species::EraseOutside(areas, all, placeOf);
    Assert::AreEqual(std::size_t{3}, all.size(), L"unknown areas, as in the sandbox, keep every figure");
    areas.Update(Replica(TEAM, {{100, 120}}), TEAM, Layout(), CommittedTuning());
    Species::EraseOutside(areas, figures, placeOf);
    Assert::AreEqual(std::size_t{2}, figures.size());
    Assert::AreEqual(CenterX(100) - MARGIN / 2.0, figures.back().position.x, 1e-9, L"the one outside went");
  }
};

} // namespace GameClientTests
