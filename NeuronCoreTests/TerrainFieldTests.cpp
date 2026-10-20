#include "pch.h"

#include "Sha256.h"
#include "TerrainField.h"
#include "TestSupport.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// The vector sets of the terrain component (height_field_generation.md §7.2), and the digests that the conformance
// generator Tools/DesignModels/TerrainGenerator.py computes for them with Python's hashlib (TER-252). Set E extends set A
// with WLD-268's method 3 and flatten types 5 and 6, from an extension of that generator written from WLD-268's text.
constexpr std::string_view SET_A_DIGEST = "bb4397625e5006a1012a348993d5b5e34ffcb246d09e77554ac9fb172de5fb9b";
constexpr std::string_view SET_A_NORMAL_DIGEST = "81a27c238f4f4c3a78401524bfc2e05c1ae357fe6db60777f82d9b5f77f7a69c";
constexpr std::string_view SET_B_DIGEST = "f12074dc1691933ec7e33a380f7d23b8917e9678d9c8ec29d282d97d3886b124";
constexpr std::string_view SET_C_DIGEST = "e7c37b773b3b004ea523bd2158d7f31f89e25fb18619fcbef1153de46c5c3e0c";
constexpr std::string_view SET_E_DIGEST = "bda1fff73240a2f816055a468e408223fda958d996fdf2cdff354ae0a1172a88";

std::string Hex(const Neuron::Sha256Digest& _digest)
{
  std::string text;
  for (const std::byte value : _digest)
  {
    text += std::format("{:02x}", static_cast<unsigned>(value));
  }
  return text;
}

Neuron::TerrainTile Tile(std::int32_t _x, double _y, std::int32_t _z, std::int32_t _size, double _fd, double _hs, double _desired,
                         Neuron::TileMethod _method, std::int32_t _seed, double _ls, std::uint32_t _power,
                         std::vector<std::uint8_t> _guide = {})
{
  Neuron::TerrainTile tile;
  tile.positionX = _x;
  tile.positionY = _y;
  tile.positionZ = _z;
  tile.size = _size;
  tile.fractalDimension = _fd;
  tile.heightScale = _hs;
  tile.desiredHeight = _desired;
  tile.method = _method;
  tile.seed = _seed;
  tile.lowlandSmoothing = _ls;
  tile.guidePower = _power;
  tile.guide = std::move(_guide);
  return tile;
}

Neuron::TerrainDefinition Definition(std::int32_t _size, double _cell, double _outside)
{
  Neuron::TerrainDefinition definition;
  definition.worldSizeX = _size;
  definition.worldSizeZ = _size;
  definition.cellSize = _cell;
  definition.outsideHeight = _outside;
  return definition;
}

// Table 7.2-A.
std::vector<Neuron::TerrainTile> SetATiles()
{
  const std::vector<std::uint8_t> zero(9, 0);
  return {Tile(204, -3.98, 260, 2264, 1.20, 4.00, 220, Neuron::TileMethod::RandomPair, 269491, 1.00, 2, zero),
          Tile(1274, 1.57, 132, 1014, 1.20, 4.00, 220, Neuron::TileMethod::RandomPair, 904945, 1.00, 2, zero),
          Tile(1129, 0.00, 985, 834, 1.20, 1.00, 300, Neuron::TileMethod::Average, 1, 1.00, 0)};
}

Neuron::TerrainField SetA(const std::vector<Neuron::TerrainFlattenArea>& _areas = {})
{
  return Neuron::TerrainField::Generate(Definition(2700, 15.76, -15.39), SetATiles(), _areas);
}

Neuron::TerrainFlattenArea Area(double _x, double _t, double _z, double _size, Neuron::FlattenOp _op, double _threshold = 40.0)
{
  return {_x, _t, _z, _size, _op, _threshold};
}

} // namespace

TEST_CLASS(TerrainFieldTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter080TheTileGeneratorDrawsTheReferenceSequence)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter080TheTileGeneratorDrawsTheReferenceSequence)
  {
    // Test T1.
    Neuron::TileRandom random;
    random.Seed(1);
    for (const std::uint32_t expected : {41u, 18467u, 6334u, 26500u, 19169u, 15724u, 11478u, 29358u})
    {
      ExpectEqual(expected, random.Next(), L"the reference sequence after Seed(1)");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter066DecimalsRoundOnceToBinary32)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-066")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter066DecimalsRoundOnceToBinary32)
  {
    // Test T2.
    Assert::IsTrue(Neuron::RoundToBinary32(15.76) == 15.760000228881836, L"15.76");
    Assert::IsTrue(Neuron::RoundToBinary32(-15.39) == -15.390000343322754, L"-15.39");
    Assert::IsTrue(Neuron::RoundToBinary32(1.2) == 1.2000000476837158, L"1.2");
    Assert::IsTrue(Neuron::RoundToBinary32(-26.13) == -26.1299991607666, L"-26.13");
    Assert::IsTrue(Neuron::RoundToBinary32(18.7) == 18.700000762939453, L"18.7");
    Assert::IsTrue(Neuron::RoundToBinary32(18.3) == 18.299999237060547, L"18.3");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter067CeilLog2IsExactBelowTheLimit)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-067")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter067CeilLog2IsExactBelowTheLimit)
  {
    // Test T3: for every integer from 1 to 69 999, powers of two included.
    for (std::uint32_t x = 1; x < 70000; ++x)
    {
      if (Neuron::CeilLog2(static_cast<double>(x)) != static_cast<std::int32_t>(std::bit_width(x - 1)))
      {
        Assert::Fail(std::format(L"ceilLog2({}) is not exact", x).c_str());
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter060SetAIsTheConformanceGeneratorsField)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter060SetAIsTheConformanceGeneratorsField)
  {
    // Tests T7 and T11: the probe samples of table 7.2-B, then every sample and every normal by digest.
    const Neuron::TerrainField field = SetA();
    ExpectEqual(172, field.Heights().Columns(), L"columns");
    ExpectEqual(172, field.Heights().Rows(), L"rows");
    Assert::IsTrue(field.ExtentX() == 2710.720039367676, L"TER-150: the extent");
    Assert::IsTrue(field.HeightAt(0, 0) == -15.390000343322754, L"h(0, 0)");
    Assert::IsTrue(field.HeightAt(43, 43) == 64.11465376675797, L"h(43, 43)");
    Assert::IsTrue(field.HeightAt(86, 86) == 99.71780778328521, L"h(86, 86)");
    Assert::IsTrue(field.HeightAt(129, 57) == 80.84683847539341, L"h(129, 57)");
    ExpectText(SET_A_DIGEST, Hex(field.Digest()), L"TER-252: every sample");
    Assert::IsTrue(field.Height(700.3, 900.7) == 84.23650515938459, L"TER-073: a bilinear read");
    const Neuron::Vec3 normal = field.NormalAt(43, 43);
    Assert::IsTrue(normal.x == -0.11977730124592213 && normal.y == 0.9264299305221012 && normal.z == -0.3569046812155097,
                   L"TER-170: a simulation normal");
    Neuron::Sha256 normals;
    for (std::int64_t j = 0; j < field.Heights().Rows(); ++j)
    {
      for (std::int64_t i = 0; i < field.Heights().Columns(); ++i)
      {
        const Neuron::Vec3 n = field.NormalAt(i, j);
        for (const double component : {n.x, n.y, n.z})
        {
          const auto bits = std::bit_cast<std::uint64_t>(component);
          std::array<std::byte, 8> bytes{};
          for (std::size_t k = 0; k < 8; ++k)
          {
            bytes[k] = static_cast<std::byte>(bits >> (8 * k));
          }
          normals.Update(bytes);
        }
      }
    }
    ExpectText(SET_A_NORMAL_DIGEST, Hex(normals.Finish()), L"every normal");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter100SetBAndSetCAreTheConformanceGeneratorsFields)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter100SetBAndSetCAreTheConformanceGeneratorsFields)
  {
    // Test T10: method 2 with its guide grid; test T12: the smooth area of set C over set A, two passes in place.
    const std::vector<Neuron::TerrainTile> setB{
      Tile(100, 0.0, 100, 344, 5.0, 1.0, 391.0, Neuron::TileMethod::RandomParent, 1, 1.0, 2, {0, 0, 255, 0, 255, 255, 0, 255, 0})};
    ExpectText(SET_B_DIGEST, Hex(Neuron::TerrainField::Generate(Definition(1000, 20.0, -26.13), setB, {}).Digest()), L"set B");
    const Neuron::TerrainField setC = SetA({Area(1350.0, 45.0, 1350.0, 300.0, Neuron::FlattenOp::Smooth, 18.3)});
    ExpectText(SET_C_DIGEST, Hex(setC.Digest()), L"set C");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter143SetDAppliesEachOperationInOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-143")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter143SetDAppliesEachOperationInOrder)
  {
    // Test T13, table 7.2-H: the samples (4 to 6, 4 to 6) after each of the five areas.
    const std::vector<Neuron::TerrainFlattenArea> areas{
      Area(60, 5, 60, 20, Neuron::FlattenOp::Absolute), Area(60, 18.7, 60, 8, Neuron::FlattenOp::Add),
      Area(60, 4, 60, 20, Neuron::FlattenOp::Subtract, 10), Area(60, 3, 60, 20, Neuron::FlattenOp::Subtract2, 6),
      Area(60, 1, 60, 20, Neuron::FlattenOp::Smooth, 0.5)};
    const std::array<std::array<std::array<double, 3>, 3>, 5> expected{{
      {{{5, 5, 5}, {5, 5, 5}, {5, 5, 5}}},
      {{{5, 5, 5}, {5, 23.700000762939453, 5}, {5, 5, 5}}},
      {{{5, 5, 5}, {5, 19.700000762939453, 5}, {5, 5, 5}}},
      {{{2, 2, 2}, {2, 19.700000762939453, 2}, {2, 2, 2}}},
      {{{-2.5, -2.1083333227369523, -2.5499999788072376},
        {-2.1083333227369523, -0.9888888818246349, -1.358333322736952},
        {-2.549999978807237, -1.358333322736952, -3.619444440912318}}},
    }};
    for (std::size_t count = 1; count <= areas.size(); ++count)
    {
      const Neuron::TerrainField field = Neuron::TerrainField::Generate(Definition(120, 12, -10), {}, std::span(areas).first(count));
      for (std::int64_t j = 4; j <= 6; ++j)
      {
        for (std::int64_t i = 4; i <= 6; ++i)
        {
          Assert::IsTrue(field.HeightAt(i, j) == expected[count - 1][static_cast<std::size_t>(j - 4)][static_cast<std::size_t>(i - 4)],
                         std::format(L"sample ({}, {}) after {} areas", i, j, count).c_str());
        }
      }
      Assert::IsTrue(field.HeightAt(0, 0) == -10.0 && field.HeightAt(9, 9) == -10.0, L"every other sample stays -10");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld268Method3AndTypes5And6AreTheirModelsFields)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-268")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld268Method3AndTypes5And6AreTheirModelsFields)
  {
    // Set A with tile 1 on the unbiased parent, an unbiased smooth area, a feathered square and one with no band.
    std::vector<Neuron::TerrainTile> tiles = SetATiles();
    tiles[1].method = Neuron::TileMethod::UnbiasedParent;
    const std::vector<Neuron::TerrainFlattenArea> areas{Area(1350.0, 45.0, 1350.0, 300.0, Neuron::FlattenOp::SmoothUnbiased, 18.3),
                                                        Area(800.0, 60.0, 1500.0, 200.0, Neuron::FlattenOp::Feathered, 150.0),
                                                        Area(2000.0, 30.0, 700.0, 100.0, Neuron::FlattenOp::Feathered, 0.0)};
    const Neuron::TerrainField field = Neuron::TerrainField::Generate(Definition(2700, 15.76, -15.39), tiles, areas);
    ExpectText(SET_E_DIGEST, Hex(field.Digest()), L"set E");
    // The feathered square is level inside its half-size and blends across its band.
    Assert::IsTrue(field.HeightAt(50, 95) == 60.0 && field.HeightAt(56, 90) == 60.0, L"level inside");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ter002AFieldAtAnOriginIsTheSameFieldMoved)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"TER-002")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ter002AFieldAtAnOriginIsTheSameFieldMoved)
  {
    // A world region's field has its square's origin (TER-002 amendment). With a cell of 16 wu and whole-number positions
    // every coordinate is exact, so the moved records give the same samples, and reads off the grid give row and column 0
    // (TER-153).
    const auto build = [](double _origin)
    {
      Neuron::TerrainDefinition definition = Definition(1600, 16.0, -10.0);
      definition.originX = _origin;
      definition.originZ = _origin;
      const auto at = [_origin](std::int32_t _value) { return static_cast<std::int32_t>(_origin) + _value; };
      const std::vector<Neuron::TerrainTile> tiles{
        Tile(at(160), 0.0, at(160), 1280, 2.0, 0.25, 150, Neuron::TileMethod::RandomPair, 77, 2.0, 2, {70, 80, 70, 80, 90, 80, 70, 80, 70}),
        Tile(at(200), 0.0, at(900), 400, 1.2, 1.0, 300, Neuron::TileMethod::UnbiasedParent, 5, 1.0, 0)};
      const std::vector<Neuron::TerrainFlattenArea> areas{
        Area(_origin + 600, 50, _origin + 600, 160, Neuron::FlattenOp::Feathered, 128),
        Area(_origin + 1100, 40, _origin + 400, 96, Neuron::FlattenOp::SmoothUnbiased, 20)};
      return Neuron::TerrainField::Generate(definition, tiles, areas);
    };
    const Neuron::TerrainField atZero = build(0.0);
    const Neuron::TerrainField moved = build(30720.0);
    Assert::IsTrue(atZero.Digest() == moved.Digest(), L"the same samples");
    Assert::IsTrue(moved.HeightAt(37, 51) == atZero.HeightAt(37, 51) && moved.HeightAt(37, 51) > 0.0, L"land at the same index");
    Assert::IsTrue(moved.Height(30720.0 + 37 * 16.0, 30720.0 + 51 * 16.0) == moved.HeightAt(37, 51), L"a read at a sample");
    Assert::IsTrue(moved.Height(30720.0 - 500.0, 30720.0 + 5 * 16.0) == moved.HeightAt(0, 5), L"west of the grid: column 0");
    Assert::IsTrue(moved.Height(30720.0 + 99999.0, 30720.0 + 5 * 16.0) == moved.HeightAt(0, 5), L"east of it: column 0 too");
    const Neuron::Vec3 up = moved.NormalAt(-1, 3);
    Assert::IsTrue(up.x == 0.0 && up.y == 1.0 && up.z == 0.0, L"TER-013: off the grid, the outside normal");
  }
};

} // namespace NeuronCoreTests
