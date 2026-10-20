#include "pch.h"

#include "Check.h"
#include "RegionTerrain.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Species
{

namespace
{

// ADR-082 item 4: the domain of world generation.
constexpr std::uint64_t WORLD_GENERATION_DOMAIN = 2;

std::uint64_t Mix64(std::uint64_t _x) noexcept
{
  _x += 0x9E3779B97F4A7C15ULL;
  _x = (_x ^ (_x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  _x = (_x ^ (_x >> 27)) * 0x94D049BB133111EBULL;
  return _x ^ (_x >> 31);
}

std::int32_t Whole(double _value) noexcept
{
  return static_cast<std::int32_t>(_value);
}

// The sample nearest a position, on one axis.
std::int64_t Nearest(double _position, double _origin, double _cell) noexcept
{
  return static_cast<std::int64_t>(std::floor((_position - _origin) / _cell + 0.5));
}

RegionLayout LayoutOf(std::uint64_t _seed, std::uint32_t _attempt, const TuningMvp& _tuning)
{
  const auto& site = _tuning.wld.site;
  RegionLayout layout;
  layout.index = 0;
  layout.side = _tuning.wld.region.side;
  layout.originX = static_cast<double>(_tuning.mvp.world.regionCell) * layout.side;
  layout.originZ = layout.originX;
  // WLD-040: the pattern's offset and rotation, each up to its bound either way.
  const double offsetX = (2.0 * WorldUnit(_seed, WorldDrawPurpose::ValleyOffset, layout.index, _attempt, 0) - 1.0) * site.valleyOffset;
  const double offsetZ = (2.0 * WorldUnit(_seed, WorldDrawPurpose::ValleyOffset, layout.index, _attempt, 1) - 1.0) * site.valleyOffset;
  const double degrees = (2.0 * WorldUnit(_seed, WorldDrawPurpose::ValleyRotation, layout.index, _attempt, 0) - 1.0) * site.valleyRotation;
  layout.rotationRadians = degrees * (std::numbers::pi / 180.0);
  layout.valleyCenter = {layout.originX + layout.side / 2.0 + offsetX, 0.0, layout.originZ + layout.side / 2.0 + offsetZ};
  const double half = site.spacing / 2.0;
  const double cosine = std::cos(layout.rotationRadians);
  const double sine = std::sin(layout.rotationRadians);
  constexpr std::array<std::array<double, 2>, 4> CORNERS{{{-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}}};
  for (std::size_t local = 0; local < CORNERS.size(); ++local)
  {
    const double a = CORNERS[local][0] * half;
    const double b = CORNERS[local][1] * half;
    layout.sites[local] = {layout.valleyCenter.x + a * cosine - b * sine, 0.0, layout.valleyCenter.z + a * sine + b * cosine};
  }
  return layout;
}

// WLD-162: the island base tile inside the channels, and the hills in corners chosen with the seed.
std::vector<Neuron::TerrainTile> TilesOf(const RegionLayout& _layout, std::uint64_t _seed, std::uint32_t _attempt, const TuningMvp& _tuning)
{
  const auto& parameters = _tuning.mvp.terrain;
  const auto& terrain = _tuning.wld.terrain;
  const double channel = _tuning.wld.region.channelWidth;
  std::vector<Neuron::TerrainTile> tiles;

  Neuron::TerrainTile base;
  base.positionX = Whole(_layout.originX + channel);
  base.positionZ = Whole(_layout.originZ + channel);
  base.size = Whole(_layout.side - 2.0 * channel);
  base.method = static_cast<Neuron::TileMethod>(parameters.baseMethod);
  base.fractalDimension = parameters.baseFractalDimension;
  base.heightScale = parameters.baseHeightScale;
  base.desiredHeight = parameters.baseDesiredHeight;
  base.lowlandSmoothing = parameters.baseLowlandSmoothing;
  base.seed = static_cast<std::int32_t>(WorldDraw(_seed, WorldDrawPurpose::TileSeed, _layout.index, _attempt, 0) & 0x7FFFFFFFULL);
  base.guidePower = 2;
  const auto rim = static_cast<std::uint8_t>(parameters.guideRim);
  const auto edge = static_cast<std::uint8_t>(parameters.guideEdge);
  const auto center = static_cast<std::uint8_t>(parameters.guideCenter);
  base.guide = {rim, edge, rim, edge, center, edge, rim, edge, rim};
  tiles.push_back(base);

  std::vector<std::uint32_t> corners{0, 1, 2, 3};
  const auto hills = static_cast<std::uint32_t>(std::min<std::int64_t>(terrain.hillsPerRegion, 4));
  for (std::uint32_t k = 0; k < hills; ++k)
  {
    const double draw = WorldUnit(_seed, WorldDrawPurpose::HillCorner, _layout.index, _attempt, k);
    const auto pick = static_cast<std::size_t>(draw * static_cast<double>(corners.size()));
    const std::uint32_t corner = corners[pick];
    corners.erase(corners.begin() + static_cast<std::ptrdiff_t>(pick));
    const double far = _layout.side - parameters.hillInset - terrain.hillSide;
    Neuron::TerrainTile hill;
    hill.positionX = Whole(_layout.originX + ((corner & 1u) == 0 ? parameters.hillInset : far));
    hill.positionZ = Whole(_layout.originZ + ((corner & 2u) == 0 ? parameters.hillInset : far));
    hill.size = Whole(terrain.hillSide);
    hill.method = static_cast<Neuron::TileMethod>(parameters.hillMethod);
    hill.fractalDimension = parameters.hillFractalDimension;
    hill.heightScale = parameters.hillHeightScale;
    hill.lowlandSmoothing = parameters.hillLowlandSmoothing;
    const double height = WorldUnit(_seed, WorldDrawPurpose::HillHeight, _layout.index, _attempt, k);
    hill.desiredHeight =
      std::min(parameters.hillDesiredMin + (parameters.hillDesiredMax - parameters.hillDesiredMin) * height, terrain.peakHeightMax);
    hill.seed = static_cast<std::int32_t>(WorldDraw(_seed, WorldDrawPurpose::TileSeed, _layout.index, _attempt, 1 + k) & 0x7FFFFFFFULL);
    tiles.push_back(hill);
  }
  ENGINE_CHECK(static_cast<std::int64_t>(tiles.size()) <= terrain.tilesPerRegionMax);
  return tiles;
}

double Clamped(double _height, const TuningMvp& _tuning) noexcept
{
  return std::clamp(_height, _tuning.wld.site.heightMin, _tuning.wld.site.heightMax);
}

double DistanceSquared(double _x, double _z, const Neuron::Vec3& _center) noexcept
{
  const double dx = _x - _center.x;
  const double dz = _z - _center.z;
  return dx * dx + dz * dz;
}

} // namespace

std::uint64_t WorldDraw(std::uint64_t _seed, WorldDrawPurpose _purpose, std::uint64_t _region, std::uint64_t _attempt,
                        std::uint64_t _index) noexcept
{
  std::uint64_t h = Mix64(_seed);
  for (const std::uint64_t key :
       {WORLD_GENERATION_DOMAIN, static_cast<std::uint64_t>(_purpose), _region, static_cast<std::uint64_t>(_attempt), _index})
  {
    h = Mix64(h ^ key);
  }
  return h;
}

double WorldUnit(std::uint64_t _seed, WorldDrawPurpose _purpose, std::uint64_t _region, std::uint64_t _attempt,
                 std::uint64_t _index) noexcept
{
  return static_cast<double>(WorldDraw(_seed, _purpose, _region, _attempt, _index) >> 11) * 0x1.0p-53;
}

RegionTerrain BuildRegionTerrain(std::uint64_t _seed, std::uint32_t _attempt, const TuningMvp& _tuning)
{
  const auto& terrain = _tuning.wld.terrain;
  const auto& site = _tuning.wld.site;
  RegionTerrain region;
  region.seed = _seed;
  region.attempt = _attempt;
  region.layout = LayoutOf(_seed, _attempt, _tuning);
  region.tiles = TilesOf(region.layout, _seed, _attempt, _tuning);
  Neuron::TerrainDefinition definition;
  definition.cellSize = terrain.cellSize;
  definition.originX = region.layout.originX;
  definition.originZ = region.layout.originZ;
  definition.worldSizeX = Whole(region.layout.side);
  definition.worldSizeZ = Whole(region.layout.side);
  definition.outsideHeight = terrain.outsideHeight;

  // WLD-162's first pass: the tiles only, read for each core's rim mean and the valley centre's height.
  const Neuron::TerrainField first = Neuron::TerrainField::Generate(definition, region.tiles, {});
  const auto samples = static_cast<std::int32_t>(terrain.rimSamples);
  for (std::size_t local = 0; local < region.layout.sites.size(); ++local)
  {
    const Neuron::Vec3& center = region.layout.sites[local];
    double sum = 0.0;
    for (std::int32_t k = 0; k < samples; ++k)
    {
      const double angle = 2.0 * std::numbers::pi * k / samples;
      sum += first.Height(center.x + site.coreRadius * std::cos(angle), center.z + site.coreRadius * std::sin(angle));
    }
    region.layout.siteFloors[local] = Clamped(sum / samples, _tuning);
  }
  region.layout.centerFloor = Clamped(first.Height(region.layout.valleyCenter.x, region.layout.valleyCenter.z), _tuning);

  // The second pass: the same tiles and, in the order of WLD-162, the cores' and the valley centre's feathered squares.
  for (std::size_t local = 0; local < region.layout.sites.size(); ++local)
  {
    const Neuron::Vec3& center = region.layout.sites[local];
    region.areas.push_back(
      {center.x, region.layout.siteFloors[local], center.z, site.coreRadius, Neuron::FlattenOp::Feathered, terrain.coreFeather});
  }
  region.areas.push_back({region.layout.valleyCenter.x, region.layout.centerFloor, region.layout.valleyCenter.z, terrain.centerHalfSize,
                          Neuron::FlattenOp::Feathered, terrain.centerFeather});
  ENGINE_CHECK(static_cast<std::int64_t>(region.areas.size()) <= terrain.flattenPerRegionMax);
  region.field = Neuron::TerrainField::Generate(definition, region.tiles, region.areas);

  // WLD-166: the region's chunks, z outer and x inner, and the digest of their digests.
  const auto perChunk = static_cast<std::int64_t>(terrain.chunkSide / terrain.cellSize);
  const auto chunks = static_cast<std::int64_t>(region.layout.side / terrain.chunkSide);
  ENGINE_CHECK(perChunk * chunks == region.field.Heights().Columns(), "the region's chunks tile its grid");
  Neuron::Sha256 all;
  for (std::int64_t cz = 0; cz < chunks; ++cz)
  {
    for (std::int64_t cx = 0; cx < chunks; ++cx)
    {
      const Neuron::Sha256Digest chunk = region.field.DigestOf(cx * perChunk, cz * perChunk, perChunk, perChunk);
      region.chunkDigests.push_back(chunk);
      all.Update(chunk);
    }
  }
  region.digest = all.Finish();
  return region;
}

std::vector<TerrainCheck> CheckRegionTerrain(const RegionTerrain& _terrain, const TuningMvp& _tuning)
{
  const auto& site = _tuning.wld.site;
  const Neuron::TerrainField& field = _terrain.field;
  const Neuron::SurfaceGrid<double>& grid = field.Heights();
  const RegionLayout& layout = _terrain.layout;
  const double cell = grid.CellSize();
  const std::int64_t columns = grid.Columns();
  const std::int64_t rows = grid.Rows();
  const double seaLevel = _tuning.ref.sim.physics.seaLevel;
  std::vector<TerrainCheck> failed;

  // 1: the strip of the channel's width centred on the region border lies at or below sea level.
  const double half = _tuning.wld.region.channelWidth / 2.0;
  bool channels = true;
  for (std::int64_t j = 0; j < rows && channels; ++j)
  {
    const double z = grid.PositionZ(j) - layout.originZ;
    for (std::int64_t i = 0; i < columns; ++i)
    {
      const double x = grid.PositionX(i) - layout.originX;
      const bool inStrip = x <= half || z <= half || x >= layout.side - half || z >= layout.side - half;
      if (inStrip && field.HeightAt(i, j) > seaLevel)
      {
        channels = false;
        break;
      }
    }
  }
  if (!channels)
  {
    failed.push_back(TerrainCheck::Channels);
  }

  // The samples within a radius of a centre, by index.
  const auto within = [&](const Neuron::Vec3& _center, double _radius, auto&& _visit)
  {
    const std::int64_t i0 = std::max<std::int64_t>(0, grid.IndexX(_center.x - _radius) - 1);
    const std::int64_t i1 = std::min<std::int64_t>(columns - 1, grid.IndexX(_center.x + _radius) + 1);
    const std::int64_t j0 = std::max<std::int64_t>(0, grid.IndexZ(_center.z - _radius) - 1);
    const std::int64_t j1 = std::min<std::int64_t>(rows - 1, grid.IndexZ(_center.z + _radius) + 1);
    for (std::int64_t j = j0; j <= j1; ++j)
    {
      for (std::int64_t i = i0; i <= i1; ++i)
      {
        if (DistanceSquared(grid.PositionX(i), grid.PositionZ(j), _center) <= _radius * _radius)
        {
          _visit(i, j);
        }
      }
    }
  };

  // 3: every sample within 0.9 × the core radius is level and in the height band; 5: none within the water clearance is
  // wet; 4: no two neighbouring samples across the core's rim differ by more than the step.
  bool plateaus = true;
  bool dry = true;
  bool rims = true;
  const double coreSquared = site.coreRadius * site.coreRadius;
  for (const Neuron::Vec3& center : layout.sites)
  {
    within(center, 0.9 * site.coreRadius,
           [&](std::int64_t _i, std::int64_t _j)
           {
             const double h = field.HeightAt(_i, _j);
             if (field.NormalAt(_i, _j).y < site.coreMinNormalY || h < site.heightMin || h > site.heightMax)
             {
               plateaus = false;
             }
           });
    within(center, site.waterClearance,
           [&](std::int64_t _i, std::int64_t _j)
           {
             if (field.HeightAt(_i, _j) <= seaLevel)
             {
               dry = false;
             }
           });
    within(center, site.coreRadius + 2.0 * cell,
           [&](std::int64_t _i, std::int64_t _j)
           {
             const bool inside = DistanceSquared(grid.PositionX(_i), grid.PositionZ(_j), center) <= coreSquared;
             for (const auto& [di, dj] : {std::pair{1, 0}, std::pair{0, 1}})
             {
               const bool neighborInside = DistanceSquared(grid.PositionX(_i + di), grid.PositionZ(_j + dj), center) <= coreSquared;
               if (inside != neighborInside && std::abs(field.HeightAt(_i, _j) - field.HeightAt(_i + di, _j + dj)) > site.rimStepMax)
               {
                 rims = false;
               }
             }
           });
  }
  if (!plateaus)
  {
    failed.push_back(TerrainCheck::Plateaus);
  }
  if (!rims)
  {
    failed.push_back(TerrainCheck::Rims);
  }
  if (!dry)
  {
    failed.push_back(TerrainCheck::DrySites);
  }

  // 6: every site centre and the valley centre reached from site 0 over walkable samples, in steps between neighbours.
  const double shore = _tuning.ref.unit.worker.shoreHeight;
  const double minimumNormalY = _tuning.ref.sim.walk.minNormalY;
  const auto walkable = [&](std::int64_t _i, std::int64_t _j) {
    return _i >= 0 && _j >= 0 && _i < columns && _j < rows && field.HeightAt(_i, _j) > shore && field.NormalAt(_i, _j).y >= minimumNormalY;
  };
  const auto indexOf = [&](const Neuron::Vec3& _p)
  { return std::pair{Nearest(_p.x, layout.originX, cell), Nearest(_p.z, layout.originZ, cell)}; };
  std::vector<std::uint8_t> reached(static_cast<std::size_t>(columns * rows), 0);
  const auto [startI, startJ] = indexOf(layout.sites[0]);
  std::vector<std::pair<std::int64_t, std::int64_t>> open;
  if (walkable(startI, startJ))
  {
    reached[static_cast<std::size_t>(startJ * columns + startI)] = 1;
    open.emplace_back(startI, startJ);
  }
  while (!open.empty())
  {
    const auto [i, j] = open.back();
    open.pop_back();
    for (const auto& [di, dj] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}})
    {
      const std::int64_t ni = i + di;
      const std::int64_t nj = j + dj;
      if (walkable(ni, nj) && reached[static_cast<std::size_t>(nj * columns + ni)] == 0)
      {
        reached[static_cast<std::size_t>(nj * columns + ni)] = 1;
        open.emplace_back(ni, nj);
      }
    }
  }
  bool reachable = true;
  std::vector<Neuron::Vec3> targets(layout.sites.begin(), layout.sites.end());
  targets.push_back(layout.valleyCenter);
  for (const Neuron::Vec3& target : targets)
  {
    const auto [i, j] = indexOf(target);
    if (i < 0 || j < 0 || i >= columns || j >= rows || reached[static_cast<std::size_t>(j * columns + i)] == 0)
    {
      reachable = false;
    }
  }
  if (!reachable)
  {
    failed.push_back(TerrainCheck::Reachability);
  }
  return failed;
}

std::expected<RegionTerrain, TerrainFailure> GenerateRegionTerrain(std::uint64_t _seed, const TuningMvp& _tuning)
{
  const auto attempts = static_cast<std::uint32_t>(std::max<std::int64_t>(1, _tuning.wld.generation.maxAttempts));
  TerrainFailure failure{_seed, {}};
  for (std::uint32_t reroll = 0; reroll < attempts; ++reroll)
  {
    const std::uint64_t seed = _seed + reroll;
    for (std::uint32_t attempt = 0; attempt < attempts; ++attempt)
    {
      RegionTerrain region = BuildRegionTerrain(seed, attempt, _tuning);
      failure.checks = CheckRegionTerrain(region, _tuning);
      if (failure.checks.empty())
      {
        return region;
      }
    }
  }
  return std::unexpected(std::move(failure));
}

} // namespace Species
