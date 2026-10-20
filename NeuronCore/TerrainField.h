#pragma once

#include "Sha256.h"
#include "Vec3.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

// TER-032 field 8 and WLD-268: how a square or diamond step picks its base from its four parents (TER-100).
enum class TileMethod : std::uint8_t
{
  Average = 0,       // the mean of the four
  RandomPair = 1,    // the mean of a random opposite pair
  RandomParent = 2,  // one random parent, with the reference's bias (TER-101)
  UnbiasedParent = 3 // one random parent, each with probability 1/4 (WLD-268)
};

// TER-035 and WLD-268: the operations of a flatten area, by stored value.
enum class FlattenOp : std::uint8_t
{
  Absolute = 0,
  Add = 1,
  Subtract = 2,
  Smooth = 3,
  Subtract2 = 4,
  SmoothUnbiased = 5, // the smooth operation's five terms divided by five
  Feathered = 6       // a level square blended into the terrain across a band of the threshold's width
};

// TER-030 as a world region needs it (TER-002 amendment): the grid starts at its origin rather than at (0, 0).
struct TerrainDefinition
{
  double cellSize = 12.0; // rounded to binary32 by the generator (TER-066)
  double originX = 0.0;
  double originZ = 0.0;
  std::int32_t worldSizeX = 2000;
  std::int32_t worldSizeZ = 2000;
  double outsideHeight = -10.0; // rounded to binary32
};

// TER-032: one fractal tile. Decimal fields are rounded to binary32 by the generator.
struct TerrainTile
{
  std::int32_t positionX = 0;
  double positionY = 0.0;
  std::int32_t positionZ = 0;
  std::int32_t size = 256;
  double fractalDimension = 1.0;
  double heightScale = 1.0;
  double desiredHeight = 100.0;
  TileMethod method = TileMethod::Average;
  std::int32_t seed = 1;
  double lowlandSmoothing = 1.0;
  std::uint32_t guidePower = 0;
  std::vector<std::uint8_t> guide; // (2^p - 1)², z outer and x inner (TER-036)
};

// TER-034: one flatten area; size is the half-width. Decimal fields are rounded to binary32 by the generator.
struct TerrainFlattenArea
{
  double centerX = 0.0;
  double centerY = 0.0; // the operand t
  double centerZ = 0.0;
  double size = 0.0;
  FlattenOp op = FlattenOp::Absolute;
  double threshold = 40.0; // ref.map.terrain.flatten_threshold_default; also the band of the Feathered type
};

/// A tile's private generator (TER-068, TER-080): unsigned 32-bit arithmetic that wraps, reseeded at the start of every
/// generation of its tile.
class TileRandom
{
public:
  void Seed(std::uint32_t _seed) noexcept
  {
    m_state = _seed;
  }
  [[nodiscard]] std::uint32_t Next() noexcept
  {
    m_state = m_state * MULTIPLIER + INCREMENT;
    return (m_state >> OUTPUT_SHIFT) & OUTPUT_MASK;
  }
  // TER-080: uniform over [-_range / 2, _range / 2], with Next() = 0 giving +_range / 2.
  [[nodiscard]] double Signed(double _range) noexcept
  {
    return (0.5 - static_cast<double>(Next()) / static_cast<double>(OUTPUT_MASK)) * _range;
  }

private:
  static constexpr std::uint32_t MULTIPLIER = 214013u;  // ter.generator.lcg.multiplier
  static constexpr std::uint32_t INCREMENT = 2531011u;  // ter.generator.lcg.increment
  static constexpr std::uint32_t OUTPUT_SHIFT = 16;     // ter.generator.lcg.output_shift
  static constexpr std::uint32_t OUTPUT_MASK = 0x7FFFu; // ter.generator.lcg.output_mask

  std::uint32_t m_state = 1;
};

// Round to binary32 and widen again (TER-066).
[[nodiscard]] inline double RoundToBinary32(double _value) noexcept
{
  return static_cast<double>(static_cast<float>(_value));
}

// ceilLog2 through the rounded logarithm (TER-065, TER-067).
[[nodiscard]] std::int32_t CeilLog2(double _value) noexcept;

/// A regular grid of samples with an origin, square cells and an outside value (`SurfaceGrid`, TER-038), with the
/// index rules of TER-072 and the bilinear read of TER-073.
template <class T> class SurfaceGrid
{
public:
  SurfaceGrid() = default;
  SurfaceGrid(std::uint16_t _columns, std::uint16_t _rows, double _originX, double _originZ, double _cellSize, T _outside)
    : m_columns(_columns),
      m_rows(_rows),
      m_originX(_originX),
      m_originZ(_originZ),
      m_cellSize(_cellSize),
      m_inverse(1.0 / _cellSize),
      m_outside(_outside),
      m_values(static_cast<std::size_t>(_columns) * _rows, _outside)
  {
  }

  // TER-072: an index outside the grid, after reduction to 16 bits, reads the outside value.
  [[nodiscard]] const T& At(std::int64_t _i, std::int64_t _j) const noexcept
  {
    const auto i = static_cast<std::uint16_t>(_i);
    const auto j = static_cast<std::uint16_t>(_j);
    if (i >= m_columns || j >= m_rows)
    {
      return m_outside;
    }
    return m_values[static_cast<std::size_t>(j) * m_columns + i];
  }
  void Set(std::int64_t _i, std::int64_t _j, const T& _value) noexcept
  {
    const auto i = static_cast<std::uint16_t>(_i);
    const auto j = static_cast<std::uint16_t>(_j);
    if (i < m_columns && j < m_rows)
    {
      m_values[static_cast<std::size_t>(j) * m_columns + i] = _value;
    }
  }

  // TER-073: the weights-first bilinear sum, left to right, with the far-edge reset.
  [[nodiscard]] T Sample(double _x, double _z) const noexcept
  {
    double fx = (_x - m_originX) * m_inverse;
    double fz = (_z - m_originZ) * m_inverse;
    auto i1 = static_cast<std::uint16_t>(static_cast<std::int64_t>(fx));
    auto i2 = static_cast<std::uint16_t>(i1 + 1);
    auto j1 = static_cast<std::uint16_t>(static_cast<std::int64_t>(fz));
    auto j2 = static_cast<std::uint16_t>(j1 + 1);
    fx -= std::floor(fx);
    fz -= std::floor(fz);
    if (i1 >= m_columns)
    {
      i1 = 0;
    }
    if (i2 >= m_columns)
    {
      i2 = 0;
    }
    if (j1 >= m_rows)
    {
      j1 = 0;
    }
    if (j2 >= m_rows)
    {
      j2 = 0;
    }
    const double w11 = (1 - fx) * (1 - fz);
    const double w12 = (1 - fx) * fz;
    const double w21 = fx * (1 - fz);
    const double w22 = fx * fz;
    // The sum is binary64 whatever T is; a grid of binary32 samples (the sea depth map, TER-198) rounds it once.
    return static_cast<T>(((At(i1, j1) * w11 + At(i1, j2) * w12) + At(i2, j1) * w21) + At(i2, j2) * w22);
  }

  // TER-074: trunc((r - origin) × inverse) as a signed integer.
  [[nodiscard]] std::int64_t IndexX(double _x) const noexcept
  {
    return static_cast<std::int64_t>((_x - m_originX) * m_inverse);
  }
  [[nodiscard]] std::int64_t IndexZ(double _z) const noexcept
  {
    return static_cast<std::int64_t>((_z - m_originZ) * m_inverse);
  }
  // TER-070: the position of a sample.
  [[nodiscard]] double PositionX(std::int64_t _i) const noexcept
  {
    return static_cast<double>(_i) * m_cellSize + m_originX;
  }
  [[nodiscard]] double PositionZ(std::int64_t _j) const noexcept
  {
    return static_cast<double>(_j) * m_cellSize + m_originZ;
  }

  [[nodiscard]] std::uint16_t Columns() const noexcept
  {
    return m_columns;
  }
  [[nodiscard]] std::uint16_t Rows() const noexcept
  {
    return m_rows;
  }
  [[nodiscard]] double OriginX() const noexcept
  {
    return m_originX;
  }
  [[nodiscard]] double OriginZ() const noexcept
  {
    return m_originZ;
  }
  [[nodiscard]] double CellSize() const noexcept
  {
    return m_cellSize;
  }
  [[nodiscard]] const T& Outside() const noexcept
  {
    return m_outside;
  }
  // Re-targets the grid onto world space with another origin and cell size (TER-120).
  void Place(double _originX, double _originZ, double _cellSize) noexcept
  {
    m_originX = _originX;
    m_originZ = _originZ;
    m_cellSize = _cellSize;
    m_inverse = 1.0 / _cellSize;
  }

private:
  std::uint16_t m_columns = 0;
  std::uint16_t m_rows = 0;
  double m_originX = 0.0;
  double m_originZ = 0.0;
  double m_cellSize = 1.0;
  double m_inverse = 1.0;
  T m_outside{};
  std::vector<T> m_values;
};

/// The terrain height field (`TerrainField`, TER-001): its height grid, its simulation normal grid and its extent.
/// `Generate` is a pure function of its records (TER-010); after it the field is immutable, and every query is const,
/// allocates nothing and draws nothing.
class TerrainField
{
public:
  [[nodiscard]] static TerrainField Generate(const TerrainDefinition& _definition, std::span<const TerrainTile> _tiles,
                                             std::span<const TerrainFlattenArea> _areas);

  // TER-011: the bilinear height, with the edge semantics of TER-153.
  [[nodiscard]] double Height(double _x, double _z) const noexcept
  {
    return m_heights.Sample(_x, _z);
  }
  // TER-012: the raw sample; outside the grid, the outside height.
  [[nodiscard]] double HeightAt(std::int64_t _i, std::int64_t _j) const noexcept
  {
    return m_heights.At(_i, _j);
  }
  // TER-013: the blend of four grid normals, not renormalised.
  [[nodiscard]] Vec3 Normal(double _x, double _z) const noexcept;
  [[nodiscard]] Vec3 NormalAt(std::int64_t _i, std::int64_t _j) const noexcept
  {
    return m_normals.At(_i, _j);
  }

  // TER-016 to TER-018: the terrain hit of the ray from _start along _direction, walking the grid's cells from the start
  // cell, or from where the ray enters the grid, each cell's two triangles in order. Structures and actors are not
  // tested. The hit may lie behind _start (DEC-241 item 10).
  [[nodiscard]] std::optional<Vec3> CastRay(const Vec3& _start, const Vec3& _direction) const noexcept;
  // ter.query.ray_tri_range: the bound on the length of a triangle hit's (t, u, v) vector (TER-017).
  static constexpr double RAY_TRIANGLE_RANGE = 1e10;

  [[nodiscard]] const SurfaceGrid<double>& Heights() const noexcept
  {
    return m_heights;
  }
  // TER-150: cell size × columns and × rows.
  [[nodiscard]] double ExtentX() const noexcept
  {
    return m_heights.CellSize() * m_heights.Columns();
  }
  [[nodiscard]] double ExtentZ() const noexcept
  {
    return m_heights.CellSize() * m_heights.Rows();
  }

  // TER-252: SHA-256 of the samples of columns [_i0, _i0 + _columns) and rows [_j0, _j0 + _rows) as little-endian
  // binary64, z outer and x inner; the whole grid when called without arguments.
  [[nodiscard]] Sha256Digest Digest() const noexcept;
  [[nodiscard]] Sha256Digest DigestOf(std::int64_t _i0, std::int64_t _j0, std::int64_t _columns, std::int64_t _rows) const noexcept;

private:
  SurfaceGrid<double> m_heights;
  SurfaceGrid<Vec3> m_normals;
};

} // namespace Neuron
