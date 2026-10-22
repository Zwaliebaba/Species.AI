#include "pch.h"

#include "RegionSimulation.h"
#include "Territory.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <queue>
#include <utility>

namespace Species
{

namespace
{

// The central resource sites' footprints, the discs that a territory stays wld.resource.wild_clearance outside of.
struct Wild
{
  Neuron::Vec3 center;
  double reach = 0.0;
};

std::vector<Wild> WildOf(const RegionSimulation& _region)
{
  std::vector<Wild> wild;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).kind == StructureKind::ResourceSiteCentral)
    {
      wild.push_back({structures.At(slot).center, structures.At(slot).radius + _region.Tuning().wld.resource.wildClearance});
    }
  }
  return wild;
}

bool WithinBounds(const RegionSimulation& _region, const std::vector<Wild>& _wild, std::uint8_t _site, double _x, double _z)
{
  const RegionTerrain& terrain = *_region.Terrain();
  const RegionLayout& layout = terrain.layout;
  const TuningMvp& tuning = _region.Tuning();
  const double band = tuning.wld.region.coastBand;
  if (_x < layout.originX + band || _z < layout.originZ + band || _x > layout.originX + layout.side - band ||
      _z > layout.originZ + layout.side - band)
  {
    return false; // WLD-061: outside the coast bands
  }
  const Neuron::Vec3 point{_x, 0.0, _z};
  const double own = Neuron::HorizontalDistance(point, layout.sites[_site]);
  if (own > tuning.wld.territory.maxRadius)
  {
    return false;
  }
  for (const auto& pair : VALLEY_PAIRS)
  {
    // The halfway line of a valley border pair: a point no nearer its own site than the neighbour's is the neighbour's.
    if ((pair[0] == _site || pair[1] == _site) &&
        Neuron::HorizontalDistance(point, layout.sites[pair[0] == _site ? pair[1] : pair[0]]) <= own)
    {
      return false;
    }
  }
  const double siteReach = tuning.wld.site.coreRadius + tuning.wld.territory.siteClearance;
  for (std::size_t other = 0; other < layout.sites.size(); ++other)
  {
    if (other != _site && Neuron::HorizontalDistance(point, layout.sites[other]) < siteReach)
    {
      return false;
    }
  }
  if (std::ranges::any_of(_wild, [&](const Wild& _disc) { return Neuron::HorizontalDistance(point, _disc.center) < _disc.reach; }))
  {
    return false;
  }
  return terrain.field.Height(_x, _z) > tuning.ref.unit.worker.shoreHeight;
}

} // namespace

TerritoryMap::TerritoryMap(double _originX, double _originZ, double _side, double _cellSize)
  : m_originX(_originX),
    m_originZ(_originZ),
    m_cellSize(_cellSize),
    m_cellsPerAxis(static_cast<std::int32_t>(std::ceil(_side / _cellSize)))
{
  ENGINE_CHECK(_cellSize > 0.0 && m_cellsPerAxis > 0);
  m_cells.assign(static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis), 0);
}

Neuron::Vec3 TerritoryMap::CellCenter(std::int32_t _x, std::int32_t _z) const noexcept
{
  return {m_originX + (_x + 0.5) * m_cellSize, 0.0, m_originZ + (_z + 0.5) * m_cellSize};
}

bool TerritoryMap::HoldsCell(std::int32_t _x, std::int32_t _z) const noexcept
{
  if (_x < 0 || _z < 0 || _x >= m_cellsPerAxis || _z >= m_cellsPerAxis)
  {
    return false;
  }
  return m_cells[static_cast<std::size_t>(_z) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(_x)] != 0;
}

void TerritoryMap::Hold(std::int32_t _x, std::int32_t _z)
{
  ENGINE_CHECK(_x >= 0 && _z >= 0 && _x < m_cellsPerAxis && _z < m_cellsPerAxis);
  std::uint8_t& cell = m_cells[static_cast<std::size_t>(_z) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(_x)];
  m_count += cell == 0 ? 1 : 0;
  cell = 1;
}

std::int32_t TerritoryMap::CellOf(double _coordinate, double _origin) const noexcept
{
  const double cell = std::floor((_coordinate - _origin) / m_cellSize);
  return static_cast<std::int32_t>(std::clamp(cell, -1.0, static_cast<double>(m_cellsPerAxis)));
}

bool TerritoryMap::Holds(double _x, double _z) const noexcept
{
  return m_cellsPerAxis > 0 && HoldsCell(CellOf(_x, m_originX), CellOf(_z, m_originZ));
}

bool TerritoryMap::HoldsDisc(double _x, double _z, double _radius) const noexcept
{
  if (!Holds(_x, _z))
  {
    return false;
  }
  const std::int32_t minX = CellOf(_x - _radius, m_originX);
  const std::int32_t maxX = CellOf(_x + _radius, m_originX);
  const std::int32_t minZ = CellOf(_z - _radius, m_originZ);
  const std::int32_t maxZ = CellOf(_z + _radius, m_originZ);
  for (std::int32_t z = minZ; z <= maxZ; ++z)
  {
    for (std::int32_t x = minX; x <= maxX; ++x)
    {
      const Neuron::Vec3 center = CellCenter(x, z);
      const double dx = center.x - _x;
      const double dz = center.z - _z;
      if (dx * dx + dz * dz <= _radius * _radius && !HoldsCell(x, z))
      {
        return false;
      }
    }
  }
  return true;
}

bool WithinTerritoryBounds(const RegionSimulation& _region, std::uint8_t _site, double _x, double _z)
{
  return WithinBounds(_region, WildOf(_region), _site, _x, _z);
}

TerritoryMap ComputeTerritory(const RegionSimulation& _region, const Settlement& _settlement)
{
  const RegionLayout& layout = _region.Terrain()->layout;
  TerritoryMap map(layout.originX, layout.originZ, layout.side, _region.Tuning().mvp.territory.cellSize);
  const Structure* wellspring = _region.ResolveStructure(_settlement.wellspringSlot, _settlement.wellspringId);
  if (wellspring == nullptr)
  {
    return map;
  }
  const double radius = _settlement.territoryRadius;
  const std::vector<Wild> wild = WildOf(_region);
  const auto inside = [&](std::int32_t _x, std::int32_t _z)
  {
    const Neuron::Vec3 center = map.CellCenter(_x, _z);
    return _x >= 0 && _z >= 0 && _x < map.CellsPerAxis() && _z < map.CellsPerAxis() &&
           Neuron::HorizontalDistance(center, wellspring->position) <= radius &&
           WithinBounds(_region, wild, _settlement.site, center.x, center.z);
  };
  // WLD-060: one connected area, the cells reached from the Wellspring's across their sides, breadth first.
  const auto start = std::pair{static_cast<std::int32_t>(std::floor((wellspring->position.x - layout.originX) / map.CellSize())),
                               static_cast<std::int32_t>(std::floor((wellspring->position.z - layout.originZ) / map.CellSize()))};
  if (!inside(start.first, start.second))
  {
    return map;
  }
  std::queue<std::pair<std::int32_t, std::int32_t>> frontier;
  map.Hold(start.first, start.second);
  frontier.push(start);
  while (!frontier.empty())
  {
    const auto [x, z] = frontier.front();
    frontier.pop();
    for (const auto& [dx, dz] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}})
    {
      const std::int32_t nx = x + dx;
      const std::int32_t nz = z + dz;
      if (!map.HoldsCell(nx, nz) && inside(nx, nz))
      {
        map.Hold(nx, nz);
        frontier.emplace(nx, nz);
      }
    }
  }
  return map;
}

} // namespace Species
