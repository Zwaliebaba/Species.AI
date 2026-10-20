#include "pch.h"

#include "ActorGrid.h"
#include "ActorGridCache.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "ObstacleGrid.h"
#include "TestSupport.h"
#include "Vec3.h"

#include <cstdint>
#include <map>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

// The reference's grid over a map of 3,000 wu at the origin, as the acceptance criteria state it.
Species::ActorGrid MakeGrid()
{
  Species::ActorGrid::Desc desc;
  desc.worldSize = 3000.0;
  return Species::ActorGrid(desc);
}

// A figure the queries resolve: its position.
struct Placed
{
  Neuron::Vec3 position;
};

// The figures of a test by stable id, which the grid's entries name.
struct World
{
  std::map<Species::StableId, Placed> placed;

  [[nodiscard]] const Placed* operator()(const Species::ActorRef& _ref) const
  {
    const auto found = placed.find(_ref.id);
    return found == placed.end() ? nullptr : &found->second;
  }
};

Species::TeamMask Teams(std::initializer_list<std::size_t> _teams)
{
  Species::TeamMask mask;
  for (const std::size_t team : _teams)
  {
    mask.set(team);
  }
  return mask;
}

bool SameIds(const std::vector<Species::ActorRef>& _refs, std::initializer_list<Species::StableId> _ids)
{
  return std::ranges::equal(_refs, _ids, {}, &Species::ActorRef::id);
}

} // namespace

TEST_CLASS(ActorGridTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim308FootprintsAreTheCellsOfHalfTheRadius)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-308")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim308FootprintsAreTheCellsOfHalfTheRadius)
  {
    // AC-SIM-05-01.
    const Species::ActorGrid grid = MakeGrid();
    ExpectEqual(94, grid.CellsPerAxis(), L"trunc(3000 ÷ 32) + 1");
    Assert::IsTrue(grid.FootprintOf(100, 100, 0) == Species::CellRect{3, 3, 3, 3}, L"radius 0: one cell");
    Assert::IsTrue(grid.FootprintOf(100, 100, 20) == Species::CellRect{2, 2, 3, 3}, L"radius 20: the square [90, 110]");
    Assert::IsTrue(grid.FootprintOf(100, 100, 64) == Species::CellRect{2, 2, 4, 4}, L"radius 64: the square [68, 132]");
    Assert::IsTrue(grid.FootprintOf(0, 0, 20) == Species::CellRect{0, 0, 0, 0}, L"clamped to the grid");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim307SlotsGrowAndEntriesKeepTheirSerialOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-307")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-310")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim307SlotsGrowAndEntriesKeepTheirSerialOrder)
  {
    // AC-SIM-05-02: A, B and C take capacity 4; D after A's removal fits; the cell reports B, C, D by serial, whatever
    // order they registered in.
    Species::ActorGrid grid = MakeGrid();
    const Species::CellRect cell{0, 0, 0, 0};
    grid.Register({3, 0, 0}, cell);
    ExpectEqual(2, grid.CapacityAt(0, 0, 0), L"0 grows to 2");
    grid.Register({1, 1, 0}, cell);
    grid.Register({2, 2, 0}, cell);
    ExpectEqual(4, grid.CapacityAt(0, 0, 0), L"then doubles");
    grid.Unregister({1, 1, 0}, cell);
    grid.Register({4, 3, 0}, cell);
    ExpectEqual(4, grid.CapacityAt(0, 0, 0), L"a freed slot is reused");
    Assert::IsTrue(SameIds(grid.EntriesAt(0, 0, 0), {2, 3, 4}), L"by serial");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim312CountsReachWholeCellsAndPresenceIsPermanent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-312")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim312CountsReachWholeCellsAndPresenceIsPermanent)
  {
    // AC-SIM-05-03 and AC-SIM-05-04: an enemy at (31, 31) is out of the exact query's range of 10 from (16, 16) and in
    // its count; after it leaves the cell, the cell still answers present.
    Species::ActorGrid grid = MakeGrid();
    World world;
    world.placed[7] = {{31.0, 0.0, 31.0}};
    const Species::ActorRef enemy{7, 0, 1};
    grid.Register(enemy, grid.FootprintOf(31, 31, 0));
    Assert::IsTrue(grid.InRange(16, 16, 10, Teams({1}), world).empty(), L"Enemies returns nothing");
    ExpectEqual(1, grid.CountInCells(16, 16, 10, Teams({1})), L"CountEnemiesInCells returns 1");
    ExpectEqual(0, grid.CountInCells(16, 16, 10, Teams({0})), L"no friend");
    grid.Unregister(enemy, grid.FootprintOf(31, 31, 0));
    world.placed[7] = {{500.0, 0.0, 500.0}};
    grid.Register(enemy, grid.FootprintOf(500, 500, 0));
    Assert::IsTrue(grid.AnyInCells(16, 16, 10, Teams({1})), L"EnemiesInCells is still true");
    ExpectEqual(0, grid.CountInCells(16, 16, 10, Teams({1})), L"though nobody is there");
    Assert::IsFalse(grid.AnyInCells(16, 16, 10, Teams({0})), L"never present for a team that never was");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim310QueriesReportCellXThenCellZThenTeamThenSerial)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-310")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-309")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim310QueriesReportCellXThenCellZThenTeamThenSerial)
  {
    // The exact query's strict range and stable order, and a figure with a radius that sits in four cells reported once.
    Species::ActorGrid grid = MakeGrid();
    World world;
    const auto add = [&](Species::StableId _id, std::uint8_t _team, double _x, double _z, double _radius)
    {
      world.placed[_id] = {{_x, 0.0, _z}};
      grid.Register({_id, 0, _team}, grid.FootprintOf(_x, _z, _radius));
    };
    add(9, 0, 70.0, 10.0, 0.0);  // cell (2, 0)
    add(5, 1, 40.0, 40.0, 0.0);  // cell (1, 1)
    add(8, 0, 40.0, 40.0, 0.0);  // cell (1, 1)
    add(6, 0, 10.0, 70.0, 0.0);  // cell (0, 2)
    add(4, 1, 63.0, 63.0, 20.0); // cells (1, 1) to (2, 2)
    add(3, 0, 100.0, 0.0, 0.0);  // cell (3, 0), outside the query's cells
    const std::vector<Species::ActorRef> found = grid.InRange(40.0, 40.0, 50.0, Teams({0, 1}), world);
    Assert::IsTrue(SameIds(found, {6, 8, 4, 5, 9}), L"(0, 2); (1, 1) team 0 then team 1 by serial; (2, 0)");
    Assert::IsTrue(SameIds(grid.InRange(40.0, 40.0, 50.0, Teams({1}), world), {4, 5}), L"one team");
    ExpectEqual(5, grid.CountInCells(40.0, 40.0, 50.0, Teams({0, 1})), L"five entries, the four-cell figure once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095TheGridsHistoryIsState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095TheGridsHistoryIsState)
  {
    // The capacities survive a round trip, so a restored grid answers the presence queries as the saved one.
    Species::ActorGrid grid = MakeGrid();
    grid.Register({1, 0, 2}, {5, 5, 5, 5});
    grid.Unregister({1, 0, 2}, {5, 5, 5, 5});
    Neuron::BitWriter writer;
    grid.WriteState(writer);
    Species::ActorGrid restored = MakeGrid();
    Neuron::BitReader reader(writer.Bytes());
    Assert::IsTrue(restored.ReadState(reader) && reader.Finish(), L"reads whole");
    ExpectEqual(2, restored.CapacityAt(2, 5, 5), L"the capacity");
    Assert::IsTrue(restored.AnyInCells(5 * 32 + 16, 5 * 32 + 16, 1, Teams({2})), L"present");
    Neuron::BitWriter again;
    restored.WriteState(again);
    Assert::IsTrue(std::ranges::equal(writer.Bytes(), again.Bytes()), L"the same state");
  }
};

TEST_CLASS(ActorGridCacheTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim316AHitReturnsTheCachedListAndEntriesAgeOut)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-316")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-318")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim316AHitReturnsTheCachedListAndEntriesAgeOut)
  {
    // AC-SIM-05-05.
    Species::ActorGridCache cache({0.5, 10});
    int queries = 0;
    const auto enemies = [&queries]
    {
      ++queries;
      return std::vector<Species::ActorRef>{{static_cast<Species::StableId>(100 + queries), 0, 2}};
    };
    const std::vector<Species::ActorRef> made = cache.CachedEnemies(0, 0, 100, 1, 10, enemies);
    Assert::IsTrue(cache.CachedEnemies(30, 0, 20, 1, 15, enemies) == made && queries == 1, L"a hit: the cached list, unchanged");
    Assert::IsTrue(cache.CachedEnemies(60, 0, 20, 1, 15, enemies) != made && queries == 2, L"outside the half range: a miss");
    Assert::IsTrue(cache.CachedEnemies(0, 0, 100, 2, 15, enemies) != made && queries == 3, L"another team: a miss");
    cache.Evict(20);
    Assert::IsTrue(cache.CachedEnemies(10, 0, 5, 1, 20, enemies) == made && queries == 3, L"still present after slice 7 of tick 20");
    cache.Evict(21);
    ExpectEqual(2, static_cast<std::int64_t>(cache.EntryCount()), L"gone after slice 7 of tick 21");
  }
};

TEST_CLASS(ObstacleGridTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim321AStructureListsItselfInTheCellsItsSphereReaches)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-321")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-324")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim321AStructureListsItselfInTheCellsItsSphereReaches)
  {
    // The cells whose centre a 16 wu sphere hits, the nearest cell under a point, and the outside cell for a point off
    // the grid and for a structure that reaches past the far edge (R-SIM-320, R-SIM-321's inclusive bounds).
    Species::ObstacleGrid::Desc desc;
    desc.worldSize = 320.0;
    Species::ObstacleGrid grid(desc);
    ExpectEqual(10, grid.CellsPerAxis(), L"ceil(320 ÷ 32)");
    const auto level = [](double, double) { return 0.0; };
    const std::uint32_t middle = grid.Add({{160.0, 0.0, 160.0}, 20.0}, level);
    Assert::IsTrue(grid.StructuresAt(150.0, 150.0) == std::vector<std::uint32_t>{middle}, L"the cell (4, 4), its centre 22.6 wu away");
    Assert::IsTrue(grid.StructuresAt(100.0, 160.0).empty(), L"the cell (3, 5), beyond its square of cells");
    Assert::IsTrue(grid.StructuresAt(210.0, 210.0).empty(), L"the cell (6, 6), its centre 67.9 wu away");
    Assert::IsTrue(grid.StructuresAt(-1.0, 50.0).empty() && grid.StructuresAt(400.0, 50.0).empty(), L"the outside cell holds nothing yet");
    // Past the far edge it reaches the cells (10, 4) and (10, 5), so the outside cell lists it twice: no duplicate check.
    const std::uint32_t edge = grid.Add({{318.0, 0.0, 150.0}, 20.0}, level);
    Assert::IsTrue(grid.StructuresAt(330.0, 150.0) == std::vector<std::uint32_t>{edge, edge}, L"past the far edge: the outside cell");
    Assert::IsTrue(grid.StructuresAt(-5.0, -5.0) == std::vector<std::uint32_t>{edge, edge}, L"every point off the grid reads it");
    Assert::IsTrue(grid.StructuresAt(300.0, 150.0) == std::vector<std::uint32_t>{edge}, L"and the cell (9, 4) once");
  }
};

} // namespace GameLogicTests
