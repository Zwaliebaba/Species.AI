#include "pch.h"

#include "Check.h"
#include "TerrainField.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace Neuron
{

namespace
{

// The terrain component's constants (its tuning table, §4.17), each named for its key. They define the
// generator rather than tune a game, so they are compiled in: a change to one changes every terrain and its digest
// (ADR-102).
constexpr double GAIN_FD_COEFFICIENT = 30.7;                                          // ter.generator.gain.fd_coefficient
constexpr double GAIN_FD_EXPONENT = -6.5;                                             // ter.generator.gain.fd_exponent
constexpr double GAIN_LS_COEFFICIENT = 15.353;                                        // ter.generator.gain.ls_coefficient
constexpr double GAIN_LS_EXPONENT = -3.1;                                             // ter.generator.gain.ls_exponent
constexpr std::uint32_t METHOD2_CHOICE_MASK = 0xFFFFu;                                // ter.generator.method2.choice_mask
constexpr std::array<std::uint32_t, 3> METHOD2_THRESHOLDS{0x3FFFu, 0x7FFFu, 0xBFFFu}; // ter.generator.method2.thresholds
constexpr std::uint32_t METHOD3_PARENT_DIVISOR = 8192u;                               // WLD-268: ⌊Next() ÷ 8,192⌋
constexpr double NOISE_LENGTH_SCALE = 256.0;                                          // ter.generator.noise.length_scale
constexpr double NOISE_LENGTH_MULTIPLIER = 10.0;                                      // ter.generator.noise.length_multiplier
constexpr double NOISE_ENVELOPE_FLOOR = 0.1;                                          // ter.generator.noise.envelope_floor
constexpr double NOISE_ENVELOPE_GAIN = 0.15;                                          // ter.generator.noise.envelope_gain
constexpr double MERGE_INDEX_ROUNDING = 0.5;                                          // ter.merge.index_rounding
constexpr double MERGE_RANGE_EPSILON = 0.001;                                         // ter.merge.range_epsilon
constexpr double SMOOTH_DIVISOR = 6.0;                                                // ter.flatten.smooth_divisor
constexpr double SMOOTH_UNBIASED_DIVISOR = 5.0;                                       // WLD-268: the five terms divided by five
constexpr double NORMALIZE_MIN_LENGTH_SQUARED = 1e-10;                                // ter.math.normalize_min_length_sq

// TER-065: the maths library's binary64 result rounded to binary32.
double Pow32(double _x, double _y) noexcept
{
  if (_x == 0.0 && _y == 0.0)
  {
    return 1.0;
  }
  return RoundToBinary32(std::pow(_x, _y));
}

double Exp32(double _x) noexcept
{
  return RoundToBinary32(std::exp(_x));
}

double Sqrt32(double _x) noexcept
{
  return RoundToBinary32(std::sqrt(_x));
}

// TER-091: one tile in tile space, n × n samples of cell 1 at the terrain's outside height.
struct TileGrid
{
  SurfaceGrid<double> grid;
  std::int32_t samples = 0;
};

TileGrid GenerateTile(const TerrainTile& _tile, double _cellSize, double _outside)
{
  const double positionY = RoundToBinary32(_tile.positionY);
  const double fractal = RoundToBinary32(_tile.fractalDimension);
  const double heightScale = RoundToBinary32(_tile.heightScale);
  const double lowland = RoundToBinary32(_tile.lowlandSmoothing);
  const auto requested = static_cast<std::int32_t>(_tile.size / _cellSize);
  const std::int32_t k = CeilLog2(static_cast<double>(requested - 1));
  const std::int32_t n = static_cast<std::int32_t>(Pow32(2.0, static_cast<double>(k))) + 1;
  TileGrid result{SurfaceGrid<double>(static_cast<std::uint16_t>(n), static_cast<std::uint16_t>(n), 0.0, 0.0, 1.0, _outside), n};
  SurfaceGrid<double>& grid = result.grid;

  // Step 3: the guide points (TER-036, TER-037).
  const std::uint32_t power = _tile.guidePower;
  if (power > 0)
  {
    const std::int64_t pointsPerSide = (std::int64_t{1} << power) - 1;
    const std::int64_t scale = (n - 1) / (pointsPerSide + 1);
    for (std::int64_t z = 0; z < pointsPerSide; ++z)
    {
      for (std::int64_t x = 0; x < pointsPerSide; ++x)
      {
        grid.Set((x + 1) * scale, (z + 1) * scale, static_cast<double>(_tile.guide[static_cast<std::size_t>(z * pointsPerSide + x)]));
      }
    }
  }

  // Step 4: the compensated gain, in this order.
  const double fractalModifier = GAIN_FD_COEFFICIENT * Exp32(GAIN_FD_EXPONENT * fractal);
  const double lowlandModifier = GAIN_LS_COEFFICIENT * Exp32(GAIN_LS_EXPONENT * lowland);
  double gain = heightScale;
  gain *= fractalModifier;
  gain *= lowlandModifier;
  const std::int32_t iterations = CeilLog2(static_cast<double>(n)) - static_cast<std::int32_t>(power);
  std::int32_t step = (n - 1) >> power;
  TileRandom random;
  random.Seed(static_cast<std::uint32_t>(_tile.seed));
  const TileMethod method = _tile.method;

  // TER-105: the noise of one call.
  const auto noise = [&](std::int32_t _half, double _base)
  {
    const double length = NOISE_LENGTH_SCALE * _half / n;
    const double amplitude = Pow32(length * NOISE_LENGTH_MULTIPLIER, fractal);
    const std::uint32_t draw = random.Next();
    double value = (0.5 - draw / 32767.0) * amplitude;
    value *= gain;
    value *= NOISE_ENVELOPE_FLOOR + Pow32(std::abs(RoundToBinary32(_base)), lowland) * NOISE_ENVELOPE_GAIN;
    return value;
  };

  // TER-100 to TER-102: a step's base from parents in the order A, B, C, D (square) or W, E, N, S (diamond).
  const auto choose = [&](const std::array<double, 4>& _parents, double _average)
  {
    switch (method)
    {
    case TileMethod::Average:
      return _average;
    case TileMethod::RandomPair:
    {
      const std::uint32_t choice = random.Next();
      return (choice & 1u) != 0 ? (_parents[0] + _parents[3]) * 0.5 : (_parents[1] + _parents[2]) * 0.5;
    }
    case TileMethod::RandomParent:
    {
      const std::uint32_t choice = random.Next() & METHOD2_CHOICE_MASK;
      return choice < METHOD2_THRESHOLDS[0]   ? _parents[0]
             : choice < METHOD2_THRESHOLDS[1] ? _parents[1]
             : choice < METHOD2_THRESHOLDS[2] ? _parents[2]
                                              : _parents[3];
    }
    case TileMethod::UnbiasedParent:
    {
      const std::uint32_t choice = random.Next();
      return _parents[choice / METHOD3_PARENT_DIVISOR];
    }
    }
    return _average;
  };

  const auto square = [&](std::int32_t _x, std::int32_t _z, std::int32_t _half)
  {
    const double a = grid.At(_x - _half, _z - _half);
    const double b = grid.At(_x - _half, _z + _half);
    const double c = grid.At(_x + _half, _z - _half);
    const double d = grid.At(_x + _half, _z + _half);
    const double base = method == TileMethod::Average ? (a + b + c + d) * 0.25 : choose({a, b, c, d}, 0.0);
    grid.Set(_x, _z, base + noise(_half, base));
  };

  // The diamond's pair for method 1 is (W + E) or (N + S): its parents are passed as W, N, S, E so that the pair of
  // indices 0 and 3 is W and E; methods 2 and 3 pick in the order W, E, N, S.
  const auto diamond = [&](std::int32_t _x, std::int32_t _z, std::int32_t _half)
  {
    const double w = grid.At(_x - _half, _z);
    const double e = grid.At(_x + _half, _z);
    const double nn = grid.At(_x, _z + _half);
    const double s = grid.At(_x, _z - _half);
    double base = 0.0;
    if (method == TileMethod::Average)
    {
      base = (w + e + s + nn) * 0.25;
    }
    else if (method == TileMethod::RandomPair)
    {
      base = choose({w, nn, s, e}, 0.0);
    }
    else
    {
      base = choose({w, e, nn, s}, 0.0);
    }
    grid.Set(_x, _z, base + noise(_half, base));
  };

  // TER-107: the write order of a pass.
  for (std::int32_t pass = 0; pass < iterations; ++pass)
  {
    for (std::int32_t x = 0; x < n - 1; x += step)
    {
      for (std::int32_t z = 0; z < n - 1; z += step)
      {
        const std::int32_t half = step >> 1;
        const std::int32_t midX = x + half;
        const std::int32_t midZ = z + half;
        square(midX, midZ, half);
        if (z != 0)
        {
          diamond(midX, z, half);
        }
        if (x != 0)
        {
          diamond(x, midZ, half);
        }
      }
    }
    step >>= 1;
  }

  // Step 7: every sample shifted by the tile's position y.
  for (std::int32_t x = 0; x < n; ++x)
  {
    for (std::int32_t z = 0; z < n; ++z)
    {
      grid.Set(x, z, grid.At(x, z) + positionY);
    }
  }
  return result;
}

// TER-076: the highest sample, both loops bounded by the row count.
double Highest(const SurfaceGrid<double>& _grid) noexcept
{
  double best = _grid.At(0, 0);
  for (std::int64_t z = 0; z < _grid.Rows(); ++z)
  {
    for (std::int64_t x = 0; x < _grid.Rows(); ++x)
    {
      const double h = _grid.At(x, z);
      if (h > best)
      {
        best = h;
      }
    }
  }
  return best;
}

// TER-120 to TER-122: the tile's peak rescaled about the outside height, resampled onto the world, the higher kept.
void Merge(SurfaceGrid<double>& _world, const TerrainTile& _tile, TileGrid& _generated, double _outside)
{
  const double cell = _world.CellSize();
  const double desired = RoundToBinary32(_tile.desiredHeight);
  // TER-124: the nearest sample by a conversion that truncates toward zero, as the reference's does, and wraps to 16 bits.
  const double startCellsX = (_tile.positionX - _world.OriginX()) / cell + MERGE_INDEX_ROUNDING;
  const double startCellsZ = (_tile.positionZ - _world.OriginZ()) / cell + MERGE_INDEX_ROUNDING;
  const auto startX = static_cast<std::uint16_t>(static_cast<std::int64_t>(startCellsX));
  const auto startZ = static_cast<std::uint16_t>(static_cast<std::int64_t>(startCellsZ));
  const auto cells = static_cast<std::int32_t>(_tile.size / cell);
  const double span = Highest(_generated.grid) - _outside + MERGE_RANGE_EPSILON;
  const double factor = (desired - _outside) / span;
  SurfaceGrid<double>& tile = _generated.grid;
  tile.Place(static_cast<double>(_tile.positionX), static_cast<double>(_tile.positionZ),
             _tile.size / static_cast<double>(_generated.samples));
  for (std::int32_t z = 0; z < cells; ++z)
  {
    const double worldZ = (z + startZ) * cell + _world.OriginZ();
    for (std::int32_t x = 0; x < cells; ++x)
    {
      const double worldX = (x + startX) * cell + _world.OriginX();
      double h = tile.Sample(worldX, worldZ);
      h = h - _outside;
      h = h * factor;
      h = h + _outside;
      if (h > _world.At(x + startX, z + startZ))
      {
        _world.Set(x + startX, z + startZ, h);
      }
    }
  }
}

// TER-143 to TER-145: one smooth evaluation, the additions in read order; WLD-268's type 5 divides by five.
double Smooth(double _current, double _target, double _threshold, const std::array<double, 4>& _neighbors, double _divisor) noexcept
{
  double value = _target;
  if (_current > _threshold)
  {
    value = _current * (_target / _current);
    for (const double neighbor : _neighbors)
    {
      value += neighbor;
    }
    value /= _divisor;
  }
  return value > _current ? _current : value;
}

// TER-140 to TER-145 and WLD-268: one area, in place, z outer and x inner.
void Flatten(SurfaceGrid<double>& _world, const TerrainFlattenArea& _area)
{
  const double cx = RoundToBinary32(_area.centerX);
  const double t = RoundToBinary32(_area.centerY);
  const double cz = RoundToBinary32(_area.centerZ);
  const double size = RoundToBinary32(_area.size);
  const double threshold = RoundToBinary32(_area.threshold);
  if (_area.op == FlattenOp::Feathered)
  {
    // WLD-268: every sample whose Chebyshev distance d from the centre is below size + band; level within size, and a
    // linear blend to the existing height across the band.
    const double reach = size + threshold;
    for (std::int64_t z = _world.IndexZ(cz - reach); z <= _world.IndexZ(cz + reach) + 1; ++z)
    {
      for (std::int64_t x = _world.IndexX(cx - reach); x <= _world.IndexX(cx + reach) + 1; ++x)
      {
        const double d = std::max(std::abs(_world.PositionX(x) - cx), std::abs(_world.PositionZ(z) - cz));
        if (d <= size)
        {
          _world.Set(x, z, t);
        }
        else if (d < reach)
        {
          const double current = _world.At(x, z);
          const double blend = 1.0 - (d - size) / threshold;
          _world.Set(x, z, current + (t - current) * blend);
        }
      }
    }
    return;
  }
  const std::int64_t x1 = _world.IndexX(cx - size) + 1;
  const std::int64_t x2 = _world.IndexX(cx + size) + 1;
  const std::int64_t z1 = _world.IndexZ(cz - size) + 1;
  const std::int64_t z2 = _world.IndexZ(cz + size) + 1;
  const bool smooth = _area.op == FlattenOp::Smooth || _area.op == FlattenOp::SmoothUnbiased;
  const double divisor = _area.op == FlattenOp::SmoothUnbiased ? SMOOTH_UNBIASED_DIVISOR : SMOOTH_DIVISOR;
  for (int pass = 0; pass < (smooth ? 2 : 1); ++pass)
  {
    for (std::int64_t z = z1; z < z2; ++z)
    {
      for (std::int64_t x = x1; x < x2; ++x)
      {
        const double current = _world.At(x, z);
        double value = t;
        switch (_area.op)
        {
        case FlattenOp::Add:
          value = t + current;
          break;
        case FlattenOp::Subtract:
          value = current > threshold ? current - t : current;
          break;
        case FlattenOp::Subtract2:
          value = current < threshold ? current - t : current;
          break;
        case FlattenOp::Smooth:
        case FlattenOp::SmoothUnbiased:
          value =
            Smooth(current, t, threshold, {_world.At(x - 1, z), _world.At(x + 1, z), _world.At(x, z + 1), _world.At(x, z - 1)}, divisor);
          break;
        case FlattenOp::Absolute:
        case FlattenOp::Feathered:
          break;
        }
        _world.Set(x, z, value);
      }
    }
  }
}

// TER-077 and TER-078: length above 1e-10, else the reference's fallback.
Vec3 NormalizeTerrain(const Vec3& _v) noexcept
{
  const double squared = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
  if (squared > NORMALIZE_MIN_LENGTH_SQUARED)
  {
    const double inverse = 1.0 / Sqrt32(squared);
    return {_v.x * inverse, _v.y * inverse, _v.z * inverse};
  }
  return {0.0, 0.0, 1.0};
}

Vec3 Cross(const Vec3& _a, const Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

} // namespace

std::int32_t CeilLog2(double _value) noexcept
{
  static const double INVERSE_LOG2 = 1.0 / RoundToBinary32(std::log(2.0));
  return static_cast<std::int32_t>(std::ceil(RoundToBinary32(std::log(_value)) * INVERSE_LOG2));
}

TerrainField TerrainField::Generate(const TerrainDefinition& _definition, std::span<const TerrainTile> _tiles,
                                    std::span<const TerrainFlattenArea> _areas)
{
  const double cell = RoundToBinary32(_definition.cellSize);
  const double outside = RoundToBinary32(_definition.outsideHeight);
  ENGINE_CHECK(std::isfinite(cell) && cell > 0.0 && _definition.worldSizeX >= 1 && _definition.worldSizeZ >= 1);
  const auto columns = static_cast<std::uint16_t>(static_cast<std::int64_t>(std::ceil(_definition.worldSizeX / cell)));
  const auto rows = static_cast<std::uint16_t>(static_cast<std::int64_t>(std::ceil(_definition.worldSizeZ / cell)));
  TerrainField field;
  field.m_heights = SurfaceGrid<double>(columns, rows, _definition.originX, _definition.originZ, cell, outside);
  for (const TerrainTile& tile : _tiles)
  {
    // TER-112, WLD-268: a tile of fewer than two cells, an unknown method or a guide of the wrong size is undefined.
    ENGINE_CHECK(static_cast<std::int32_t>(tile.size / cell) > 1 && tile.method <= TileMethod::UnbiasedParent);
    ENGINE_CHECK(tile.guidePower == 0 ||
                 tile.guide.size() == static_cast<std::size_t>(((1 << tile.guidePower) - 1) * ((1 << tile.guidePower) - 1)));
    TileGrid generated = GenerateTile(tile, cell, outside);
    Merge(field.m_heights, tile, generated, outside);
  }
  for (const TerrainFlattenArea& area : _areas)
  {
    ENGINE_CHECK(area.op <= FlattenOp::Feathered);
    Flatten(field.m_heights, area);
  }

  // TER-170: the simulation normals, z outer and x inner.
  const SurfaceGrid<double>& h = field.m_heights;
  field.m_normals = SurfaceGrid<Vec3>(columns, rows, _definition.originX, _definition.originZ, cell, Vec3{0.0, 1.0, 0.0});
  for (std::int64_t j = 0; j < rows; ++j)
  {
    for (std::int64_t i = 0; i < columns; ++i)
    {
      const double hN = h.At(i, j - 1);
      const double hW = h.At(i + 1, j);
      const double hE = h.At(i - 1, j);
      const double hS = h.At(i, j + 1);
      if (hN == hW && hE == hS && hN == hE)
      {
        continue;
      }
      const double center = h.At(i, j);
      const Vec3 vN{0.0, center - hN, cell};
      const Vec3 vW{-cell, center - hW, 0.0};
      const Vec3 vS{0.0, center - hS, -cell};
      const Vec3 vE{cell, center - hE, 0.0};
      const Vec3 nA = NormalizeTerrain(Cross(vW, vN));
      const Vec3 nB = NormalizeTerrain(Cross(vE, vS));
      field.m_normals.Set(i, j, NormalizeTerrain({nA.x + nB.x, nA.y + nB.y, nA.z + nB.z}));
    }
  }
  return field;
}

Vec3 TerrainField::Normal(double _x, double _z) const noexcept
{
  return m_normals.Sample(_x, _z);
}

Sha256Digest TerrainField::Digest() const noexcept
{
  return DigestOf(0, 0, m_heights.Columns(), m_heights.Rows());
}

Sha256Digest TerrainField::DigestOf(std::int64_t _i0, std::int64_t _j0, std::int64_t _columns, std::int64_t _rows) const noexcept
{
  Sha256 hasher;
  for (std::int64_t j = _j0; j < _j0 + _rows; ++j)
  {
    for (std::int64_t i = _i0; i < _i0 + _columns; ++i)
    {
      const auto bits = std::bit_cast<std::uint64_t>(m_heights.At(i, j));
      std::array<std::byte, 8> bytes{};
      for (std::size_t n = 0; n < 8; ++n)
      {
        bytes[n] = static_cast<std::byte>(bits >> (8 * n));
      }
      hasher.Update(bytes);
    }
  }
  return hasher.Finish();
}

} // namespace Neuron
