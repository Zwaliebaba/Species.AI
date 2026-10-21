#include "pch.h"

#include "TerrainSurface.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Neuron
{

namespace
{

// TER-077 in binary32: ter.math.normalize_min_length_sq and ter.math.normalize_fallback.
constexpr float NORMALIZE_MIN_LENGTH_SQ = 1e-10f;
constexpr std::array<float, 3> NORMALIZE_FALLBACK{0.0f, 0.0f, 1.0f};

using Float3 = std::array<float, 3>;

Float3 Subtract(const Float3& _a, const Float3& _b) noexcept
{
  return {_a[0] - _b[0], _a[1] - _b[1], _a[2] - _b[2]};
}

Float3 Cross(const Float3& _a, const Float3& _b) noexcept
{
  return {_a[1] * _b[2] - _a[2] * _b[1], _a[2] * _b[0] - _a[0] * _b[2], _a[0] * _b[1] - _a[1] * _b[0]};
}

Float3 Normalize(const Float3& _v) noexcept
{
  const float lengthSq = _v[0] * _v[0] + _v[1] * _v[1] + _v[2] * _v[2];
  if (!(lengthSq > NORMALIZE_MIN_LENGTH_SQ))
  {
    return NORMALIZE_FALLBACK;
  }
  const float scale = 1.0f / std::sqrt(lengthSq);
  return {_v[0] * scale, _v[1] * scale, _v[2] * scale};
}

// The ramp texel at (_px, _py), _py counted from the bottom row as viewed (TER-040), with alpha 255 (RND-134).
std::uint32_t RampTexel(const CpuImage& _ramp, std::uint32_t _px, std::uint32_t _py) noexcept
{
  if (_ramp.width == 0 || _ramp.height == 0 || _ramp.texels.size() < std::size_t{_ramp.width} * _ramp.height * 4)
  {
    return 0xFFFF'FFFFu; // no ramp: white, the default of a missing texture (DEC-015)
  }
  const std::uint32_t column = std::min(_px, _ramp.width - 1);
  const std::uint32_t row = _ramp.height - 1 - std::min(_py, _ramp.height - 1);
  const std::size_t at = (std::size_t{row} * _ramp.width + column) * 4;
  return std::uint32_t{_ramp.texels[at]} | (std::uint32_t{_ramp.texels[at + 1]} << 8) | (std::uint32_t{_ramp.texels[at + 2]} << 16) |
         0xFF00'0000u;
}

std::uint32_t RampIndex(float _value, std::uint32_t _size) noexcept
{
  // Clamped as a float before the conversion, so a value far outside the ramp never overflows the integer.
  const float scaled = std::clamp(_value * static_cast<float>(_size), 0.0f, static_cast<float>(_size - 1));
  return static_cast<std::uint32_t>(scaled);
}

} // namespace

double HighestSample(const SurfaceGrid<double>& _grid) noexcept
{
  double highest = _grid.At(0, 0);
  for (std::int64_t z = 0; z < _grid.Rows(); ++z)
  {
    for (std::int64_t x = 0; x < _grid.Rows(); ++x)
    {
      highest = std::max(highest, _grid.At(x, z));
    }
  }
  return highest;
}

std::uint32_t FacetColor(const std::array<Vec3, 3>& _corners, float _normalY, float _highest, const CpuImage& _ramp,
                         const TerrainLook& _look, TileRandom& _colorVariation) noexcept
{
  const Vec3 center = (_corners[0] + _corners[1] + _corners[2]) * _look.centerFactor;
  const float u =
    static_cast<float>(std::pow(std::max(0.0f, 1.0f - _normalY), static_cast<float>(_look.slopeExponent))); // max: a y above 1 is flat
  const auto height = static_cast<float>(center.y);
  auto v = static_cast<float>(1.0f - height / _highest);
  const std::uint32_t r0 = _colorVariation.Next();
  // TER-181: the seed is trunc(x) OR (trunc(z) + r0), the sum modulo 2^32.
  const auto seedX = static_cast<std::uint32_t>(static_cast<std::int64_t>(center.x));
  const auto seedZ = static_cast<std::uint32_t>(static_cast<std::int64_t>(center.z));
  _colorVariation.Seed(seedX | (seedZ + r0));
  const double jitter = _colorVariation.Signed(_look.jitterNumerator / (std::max(static_cast<double>(height), 0.0) + _look.jitterOffset));
  v = static_cast<float>(static_cast<double>(v) + jitter);
  return RampTexel(_ramp, RampIndex(u, _look.rampSize), RampIndex(v, _look.rampSize));
}

TerrainSurface BuildTerrainSurface(const TerrainField& _field, const CpuImage& _ramp, const TerrainLook& _look)
{
  const SurfaceGrid<double>& heights = _field.Heights();
  const std::int64_t columns = heights.Columns();
  const std::int64_t rows = heights.Rows();
  const double cell = heights.CellSize();
  const double originX = heights.OriginX();
  const double originZ = heights.OriginZ();
  // TER-180: a region's terrain states no maximum height, so the normaliser is the grid's highest sample.
  float highest = static_cast<float>(HighestSample(heights));
  if (!(highest > 0.0f))
  {
    highest = 1.0f; // a field with no land: every colour reads the sea-level row
  }

  const std::int64_t chunkCells = std::max<std::int64_t>(1, std::llround(_look.chunkSide / cell));
  const double gridRepeat = _look.gridCellsPerRepeat > 0.0 ? _look.gridCellsPerRepeat : 1.0;
  const std::int64_t chunksX = (columns + chunkCells - 1) / chunkCells;
  const std::int64_t chunksZ = (rows + chunkCells - 1) / chunkCells;
  std::vector<TerrainChunk> chunks(static_cast<std::size_t>(chunksX * chunksZ));
  for (std::int64_t cz = 0; cz < chunksZ; ++cz)
  {
    for (std::int64_t cx = 0; cx < chunksX; ++cx)
    {
      TerrainChunk& chunk = chunks[static_cast<std::size_t>(cz * chunksX + cx)];
      chunk.origin = {originX + static_cast<double>(cx * chunkCells) * cell, 0.0, originZ + static_cast<double>(cz * chunkCells) * cell};
      chunk.boundsMin = {1e300, 1e300, 1e300};
      chunk.boundsMax = {-1e300, -1e300, -1e300};
    }
  }

  const auto sunk = [&](double _height) { return static_cast<float>(_height < _look.sinkThreshold ? _look.sinkHeight : _height); };
  // RND-132: a pair is needed when any of the six heights around it is above 0, before the sink.
  const auto needed = [&](std::int64_t _x, std::int64_t _z)
  {
    for (std::int64_t dx = -1; dx <= 1; ++dx)
    {
      if (_field.HeightAt(_x + dx, _z) > 0.0 || _field.HeightAt(_x + dx, _z + 1) > 0.0)
      {
        return true;
      }
    }
    return false;
  };

  TerrainSurface surface;
  TileRandom colorVariation;
  colorVariation.Seed(_look.presentationSeed);
  std::vector<bool> neededPairs(static_cast<std::size_t>(columns));
  for (std::int64_t z = 0; z < rows; ++z)
  {
    for (std::int64_t x = 0; x < columns; ++x)
    {
      neededPairs[static_cast<std::size_t>(x)] = needed(x, z);
    }
    for (std::int64_t x = 0; x + 1 < columns; ++x)
    {
      if (!neededPairs[static_cast<std::size_t>(x)] || !neededPairs[static_cast<std::size_t>(x + 1)])
      {
        continue;
      }
      TerrainChunk& chunk = chunks[static_cast<std::size_t>((z / chunkCells) * chunksX + x / chunkCells)];
      const auto localX = static_cast<float>(static_cast<double>(x) * cell + originX - chunk.origin.x);
      const auto localZ = static_cast<float>(static_cast<double>(z) * cell + originZ - chunk.origin.z);
      const auto step = static_cast<float>(cell);
      // The pairs P = (p1, p2) at column x and Q = (q1, q2) at x + 1, in chunk coordinates.
      const Float3 p1{localX, sunk(_field.HeightAt(x, z)), localZ};
      const Float3 p2{localX, sunk(_field.HeightAt(x, z + 1)), localZ + step};
      const Float3 q1{localX + step, sunk(_field.HeightAt(x + 1, z)), localZ};
      const Float3 q2{localX + step, sunk(_field.HeightAt(x + 1, z + 1)), localZ + step};
      const auto world = [&](const Float3& _corner)
      {
        return Vec3{chunk.origin.x + static_cast<double>(_corner[0]), static_cast<double>(_corner[1]),
                    chunk.origin.z + static_cast<double>(_corner[2])};
      };
      // RND-135 as O83 amends it: fx.terrain_grid repeats once every gridCellsPerRepeat cells.
      const auto gridOf = [&](const Float3& _corner)
      {
        return std::array<float, 2>{
          static_cast<float>(((chunk.origin.x - originX) / cell + static_cast<double>(_corner[0]) / cell) / gridRepeat),
          static_cast<float>(((chunk.origin.z - originZ) / cell + static_cast<double>(_corner[2]) / cell) / gridRepeat)};
      };

      const Float3 firstNormal = Normalize(Cross(Subtract(q1, p2), Subtract(p1, p2)));
      const Float3 secondNormal = Normalize(Cross(Subtract(q2, p2), Subtract(q1, p2)));
      const std::uint32_t firstColor = FacetColor({world(p1), world(p2), world(q1)}, firstNormal[1], highest, _ramp, _look, colorVariation);
      const std::uint32_t secondColor =
        FacetColor({world(p2), world(q1), world(q2)}, secondNormal[1], highest, _ramp, _look, colorVariation);

      const auto emit = [&](const Float3& _corner, std::uint32_t _color, const Float3& _normal)
      {
        chunk.vertices.push_back(TerrainVertex{_corner, _color, _normal, gridOf(_corner)});
        const Vec3 at = world(_corner);
        chunk.boundsMin = {std::min(chunk.boundsMin.x, at.x), std::min(chunk.boundsMin.y, at.y), std::min(chunk.boundsMin.z, at.z)};
        chunk.boundsMax = {std::max(chunk.boundsMax.x, at.x), std::max(chunk.boundsMax.y, at.y), std::max(chunk.boundsMax.z, at.z)};
      };
      emit(p1, firstColor, firstNormal);
      emit(p2, firstColor, firstNormal);
      emit(q1, firstColor, firstNormal);
      emit(q1, secondColor, secondNormal);
      emit(p2, secondColor, secondNormal);
      emit(q2, secondColor, secondNormal);
      surface.triangles += 2;
    }
  }
  for (TerrainChunk& chunk : chunks)
  {
    if (!chunk.vertices.empty())
    {
      surface.chunks.push_back(std::move(chunk));
    }
  }
  surface.colorChainEnd = colorVariation;
  return surface;
}

} // namespace Neuron
