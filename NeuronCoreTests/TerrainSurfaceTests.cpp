#include "pch.h"

#include "SeaSurface.h"
#include "TerrainField.h"
#include "TerrainSurface.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

constexpr double ORIGIN = 30'720.0; // a region's origin (ADR-102)
constexpr double CELL = 16.0;
constexpr std::int32_t SIDE = 320; // 20 cells

// A square island of height 50 in the middle of a 320 wu region at sea depth -10 (TER-034's absolute flatten).
Neuron::TerrainField Island()
{
  Neuron::TerrainDefinition definition;
  definition.cellSize = CELL;
  definition.originX = ORIGIN;
  definition.originZ = ORIGIN;
  definition.worldSizeX = SIDE;
  definition.worldSizeZ = SIDE;
  definition.outsideHeight = -10.0;
  Neuron::TerrainFlattenArea area;
  area.centerX = ORIGIN + 160.0;
  area.centerY = 50.0;
  area.centerZ = ORIGIN + 160.0;
  area.size = 48.0;
  area.op = Neuron::FlattenOp::Absolute;
  const std::array areas{area};
  return Neuron::TerrainField::Generate(definition, {}, areas);
}

// A ramp whose texel at column x of memory row r is (x, r, 7), so a colour names the texel it came from.
std::vector<std::uint8_t> NamingRamp()
{
  std::vector<std::uint8_t> texels(std::size_t{64} * 64 * 4);
  for (std::uint32_t row = 0; row < 64; ++row)
  {
    for (std::uint32_t x = 0; x < 64; ++x)
    {
      const std::size_t at = (std::size_t{row} * 64 + x) * 4;
      texels[at] = static_cast<std::uint8_t>(x);
      texels[at + 1] = static_cast<std::uint8_t>(row);
      texels[at + 2] = 7;
      texels[at + 3] = 0;
    }
  }
  return texels;
}

std::uint32_t Column(std::uint32_t _color)
{
  return _color & 0xFFu;
}
std::uint32_t MemoryRow(std::uint32_t _color)
{
  return (_color >> 8) & 0xFFu;
}

} // namespace

TEST_CLASS(TerrainSurfaceTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd132OnlyNeededPairsFormTrianglesWithinARow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-132")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd132OnlyNeededPairsFormTrianglesWithinARow)
  {
    const Neuron::TerrainField field = Island();
    const std::vector<std::uint8_t> ramp = NamingRamp();
    const Neuron::TerrainSurface surface = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, {});
    Assert::IsTrue(surface.triangles > 0);
    std::size_t vertices = 0;
    for (const Neuron::TerrainChunk& chunk : surface.chunks)
    {
      vertices += chunk.vertices.size();
      for (std::size_t t = 0; t < chunk.vertices.size(); t += 3)
      {
        // Every triangle spans one cell: no row-wrap triangle crosses the region (ADR-115).
        const auto [minX, maxX] =
          std::minmax({chunk.vertices[t].position[0], chunk.vertices[t + 1].position[0], chunk.vertices[t + 2].position[0]});
        const auto [minZ, maxZ] =
          std::minmax({chunk.vertices[t].position[2], chunk.vertices[t + 1].position[2], chunk.vertices[t + 2].position[2]});
        Assert::AreEqual(static_cast<float>(CELL), maxX - minX);
        Assert::AreEqual(static_cast<float>(CELL), maxZ - minZ);
      }
    }
    Assert::AreEqual(std::size_t{surface.triangles} * 3, vertices);
    // The island's land samples span columns and rows 8 to 13. A pair is needed within one column of land, in a land
    // row or the row before, so rows 7 to 13 hold needed pairs at columns 7 to 14: 7 rows of 7 steps, 2 triangles each.
    Assert::AreEqual(7u * 7u * 2u, surface.triangles);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd133And134LowVerticesSinkAndFlatFacetsFaceUp)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-133")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-134")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd133And134LowVerticesSinkAndFlatFacetsFaceUp)
  {
    const Neuron::TerrainField field = Island();
    const std::vector<std::uint8_t> ramp = NamingRamp();
    const Neuron::TerrainSurface surface = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, {});
    std::size_t flat = 0;
    for (const Neuron::TerrainChunk& chunk : surface.chunks)
    {
      for (const Neuron::TerrainVertex& vertex : chunk.vertices)
      {
        Assert::IsTrue(vertex.position[1] == -10.0f || vertex.position[1] >= 0.3f, L"a vertex below 0.3 is sunk to -10");
        const float length =
          std::sqrt(vertex.normal[0] * vertex.normal[0] + vertex.normal[1] * vertex.normal[1] + vertex.normal[2] * vertex.normal[2]);
        Assert::AreEqual(1.0f, length, 1e-5f, L"a unit facet normal");
        Assert::IsTrue(vertex.normal[1] >= 0.0f, L"every facet faces up or sideways");
        flat += vertex.normal[1] == 1.0f ? 1u : 0u;
      }
    }
    Assert::IsTrue(flat > 0, L"the plateau's facets are flat");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter181TheSlopePicksTheColumnAndTheHeightTheRow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-181")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-182")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter181TheSlopePicksTheColumnAndTheHeightTheRow)
  {
    const std::vector<std::uint8_t> ramp = NamingRamp();
    const Neuron::CpuImage image{64, 64, ramp};
    Neuron::TerrainLook look;
    look.jitterNumerator = 0.0; // no colour variation
    look.centerFactor = 1.0 / 3.0;
    const auto colorAt = [&](double _degrees, double _height)
    {
      Neuron::TileRandom random;
      const std::array<Neuron::Vec3, 3> corners{Neuron::Vec3{ORIGIN, _height, ORIGIN}, Neuron::Vec3{ORIGIN, _height, ORIGIN + 16.0},
                                                Neuron::Vec3{ORIGIN + 16.0, _height, ORIGIN}};
      return Neuron::FacetColor(corners, static_cast<float>(std::cos(_degrees * std::numbers::pi / 180.0)), 100.0f, image, look, random);
    };
    // TER-182: 10°, 30°, 45°, 60° and vertical facets read columns 11, 28, 39, 48 and 63.
    const std::array<std::pair<double, std::uint32_t>, 6> slopes{{{0.0, 0}, {10.0, 11}, {30.0, 28}, {45.0, 39}, {60.0, 48}, {90.0, 63}}};
    for (const auto& [degrees, column] : slopes)
    {
      Assert::AreEqual(column, Column(colorAt(degrees, 100.0)));
    }
    // TER-040: the highest ground reads row 0 counted from the bottom (memory row 63); sea level reads the top row.
    Assert::AreEqual(63u, MemoryRow(colorAt(0.0, 100.0)));
    Assert::AreEqual(0u, MemoryRow(colorAt(0.0, 0.0)));
    Assert::AreEqual(0xFFu, colorAt(0.0, 0.0) >> 24, L"every facet is opaque");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter185TwoBuildsOfOneFieldGiveTheSameColorsAndShoreNoise)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-185")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-186")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter185TwoBuildsOfOneFieldGiveTheSameColorsAndShoreNoise)
  {
    const Neuron::TerrainField field = Island();
    const std::vector<std::uint8_t> ramp = NamingRamp();
    const Neuron::TerrainSurface first = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, {});
    const Neuron::TerrainSurface second = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, {});
    Assert::AreEqual(first.chunks.size(), second.chunks.size());
    for (std::size_t c = 0; c < first.chunks.size(); ++c)
    {
      Assert::AreEqual(first.chunks[c].vertices.size(), second.chunks[c].vertices.size());
      for (std::size_t v = 0; v < first.chunks[c].vertices.size(); ++v)
      {
        Assert::AreEqual(first.chunks[c].vertices[v].color, second.chunks[c].vertices[v].color);
      }
    }
    Neuron::TileRandom a = first.colorChainEnd;
    Neuron::TileRandom b = second.colorChainEnd;
    Assert::AreEqual(a.Next(), b.Next(), L"the shore noise starts from the same state");
    const Neuron::SeaSurface seaA = Neuron::BuildSeaSurface(field, {}, first.colorChainEnd, {}, 1);
    const Neuron::SeaSurface seaB = Neuron::BuildSeaSurface(field, {}, second.colorChainEnd, {}, 1);
    Assert::AreEqual(seaA.pairs.size(), seaB.pairs.size());
    for (std::size_t p = 0; p < seaA.pairs.size(); ++p)
    {
      Assert::AreEqual(seaA.pairs[p].noise1, seaB.pairs[p].noise1);
      Assert::AreEqual(seaA.pairs[p].noise2, seaB.pairs[p].noise2);
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd138ChunksHoldTheirTrianglesAndBoundThem)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-138")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd138ChunksHoldTheirTrianglesAndBoundThem)
  {
    const Neuron::TerrainField field = Island();
    const std::vector<std::uint8_t> ramp = NamingRamp();
    Neuron::TerrainLook look;
    look.chunkSide = 128.0; // 8 cells: the 20-cell region has 3 × 3 chunks
    const Neuron::TerrainSurface surface = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, look);
    Assert::IsTrue(surface.chunks.size() > 1);
    for (const Neuron::TerrainChunk& chunk : surface.chunks)
    {
      Assert::AreEqual(0.0, std::fmod(chunk.origin.x - ORIGIN, 128.0));
      for (const Neuron::TerrainVertex& vertex : chunk.vertices)
      {
        const double x = chunk.origin.x + vertex.position[0];
        const double z = chunk.origin.z + vertex.position[2];
        Assert::IsTrue(x >= chunk.boundsMin.x && x <= chunk.boundsMax.x && z >= chunk.boundsMin.z && z <= chunk.boundsMax.z);
        Assert::AreEqual(static_cast<float>((x - ORIGIN) / CELL), vertex.grid[0], L"the grid coordinate counts cells from the region");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd135TheGridRepeatsOnceEveryItsCells)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-135")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd135TheGridRepeatsOnceEveryItsCells)
  {
    // O83: with two cells per repeat, the grid coordinates are half the cell coordinates, so fx.terrain_grid spans two
    // cells.
    const Neuron::TerrainField field = Island();
    const std::vector<std::uint8_t> ramp = NamingRamp();
    Neuron::TerrainLook look;
    look.gridCellsPerRepeat = 2.0;
    const Neuron::TerrainSurface surface = Neuron::BuildTerrainSurface(field, {64, 64, ramp}, look);
    Assert::IsTrue(!surface.chunks.empty() && !surface.chunks.front().vertices.empty(), L"a surface");
    for (const Neuron::TerrainChunk& chunk : surface.chunks)
    {
      for (const Neuron::TerrainVertex& vertex : chunk.vertices)
      {
        const double x = chunk.origin.x + vertex.position[0];
        const double z = chunk.origin.z + vertex.position[2];
        Assert::AreEqual(static_cast<float>((x - ORIGIN) / CELL / 2.0), vertex.grid[0], 1e-5f, L"half the cell coordinate in x");
        Assert::AreEqual(static_cast<float>((z - ORIGIN) / CELL / 2.0), vertex.grid[1], 1e-5f, L"and in z");
      }
    }
  }
};

TEST_CLASS(SeaSurfaceTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ter197TheShoreMaskIsSaturatedOnLandAndEmptyFarOutAndStoredFlipped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-196")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-197")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter197TheShoreMaskIsSaturatedOnLandAndEmptyFarOutAndStoredFlipped)
  {
    const Neuron::TerrainField field = Island();
    const Neuron::SeaSurface sea = Neuron::BuildSeaSurface(field, {}, Neuron::TileRandom{}, {}, 1);
    Assert::AreEqual(128u, sea.maskSize);
    Assert::AreEqual(ORIGIN - 160.0, sea.planeOrigin.x, L"the plane begins half a region before the region (ADR-115)");
    Assert::AreEqual(640.0, sea.planeSide);
    // A mask pixel is 5 wu: the island's centre at plane x = 320 is pixel 64, its row 64 at image row 63.
    const auto image = [&](std::uint32_t _x, std::uint32_t _maskRow) { return sea.shoreMask[std::size_t{127 - _maskRow} * 128 + _x]; };
    Assert::AreEqual(std::uint8_t{255}, image(64, 64), L"saturated on land");
    Assert::AreEqual(std::uint8_t{0}, image(10, 10), L"open sea far from the coast");
    Assert::AreEqual(std::uint8_t{0}, sea.shoreMask[0], L"image row 0 is the plane's far edge");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter199OnlyTilesNearLandAreDrawn)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-198")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-199")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-141")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter199OnlyTilesNearLandAreDrawn)
  {
    const Neuron::TerrainField field = Island();
    const Neuron::SeaSurface sea = Neuron::BuildSeaSurface(field, {}, Neuron::TileRandom{}, {}, 1);
    Assert::IsTrue(!sea.flatVertices.empty() && sea.flatVertices.size() % 6 == 0);
    Assert::IsTrue(sea.flatVertices.size() / 6 < std::size_t{32} * 32, L"open-sea tiles are not drawn");
    for (const Neuron::SeaVertex& vertex : sea.flatVertices)
    {
      Assert::AreEqual(-9.0f, vertex.position[1]);
      // Every drawn tile lies within the land's blur reach of the island, which spans plane 272 to 368.
      Assert::IsTrue(vertex.position[0] >= 160.0f && vertex.position[0] <= 480.0f);
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter200To205TheWaveMeshFollowsTheCoastAndMovesWithTime)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-200")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-201")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-204")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-205")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter200To205TheWaveMeshFollowsTheCoastAndMovesWithTime)
  {
    const Neuron::TerrainField field = Island();
    std::vector<std::uint8_t> gradient(std::size_t{400} * 10 * 4, 0);
    for (std::uint32_t x = 0; x < 400; ++x)
    {
      gradient[(std::size_t{8} * 400 + x) * 4] = static_cast<std::uint8_t>(x % 256); // memory row 8 is row 1 from the bottom
    }
    const Neuron::SeaSurface sea = Neuron::BuildSeaSurface(field, {}, Neuron::TileRandom{}, {400, 10, gradient}, 1);
    Assert::AreEqual(3.2, sea.waterCell, 1e-12, L"the cell of level 1 is the side ÷ 100");
    Assert::AreEqual(202u, sea.tableX);
    Assert::AreEqual(std::size_t{400}, sea.waveColors.size());
    Assert::AreEqual(0xFF00'0005u, sea.waveColors[5], L"row 1 from the bottom, opaque");
    Assert::IsTrue(!sea.pairs.empty() && sea.waveTriangles > 0);
    for (const Neuron::WavePair& pair : sea.pairs)
    {
      Assert::IsTrue(pair.depth1 <= 0.999f || pair.depth2 <= 0.999f || pair.xb > 0, L"a pair lies near the coast");
    }
    std::vector<Neuron::SeaVertex> early;
    std::vector<Neuron::SeaVertex> late;
    Neuron::EvaluateWaves(sea, {}, 0.0, early);
    Neuron::EvaluateWaves(sea, {}, 2.0, late);
    Assert::AreEqual(std::size_t{sea.waveTriangles} * 3, early.size());
    bool moved = false;
    for (std::size_t v = 0; v < early.size(); ++v)
    {
      Assert::IsTrue(std::abs(early[v].position[1]) <= 25.6f, L"the waves stay within ±25.55 wu (TER-206)");
      moved |= early[v].position[1] != late[v].position[1];
    }
    Assert::IsTrue(moved, L"the waves move with the presentation time");
    for (std::size_t t = 0; t < early.size(); t += 3)
    {
      Assert::AreEqual(early[t].color, early[t + 1].color, L"one facet colour per triangle (RND-145)");
      Assert::AreEqual(early[t].color, early[t + 2].color);
    }
  }
};

} // namespace NeuronCoreTests
