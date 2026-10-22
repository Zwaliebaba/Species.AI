#pragma once

#include "Settlement.h"
#include "Vec3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Species
{

class RegionSimulation;

// WLD-062: the region's pairs of valley neighbours, by local site, in the order of the navigation nodes' border areas
// (MVP-017).
inline constexpr std::array<std::array<std::uint8_t, 2>, 4> VALLEY_PAIRS{{{0, 1}, {2, 3}, {0, 2}, {1, 3}}};

/// A settlement's territory on its region's territory raster (MVP-026, WLD-060, ADR-134): the cells, of side
/// mvp.territory.cell_size over the region's square, whose centres it holds. It is derived from the settlement's radius,
/// the terrain and the structures, so neither the hash nor the snapshot holds it.
class TerritoryMap
{
public:
  TerritoryMap() = default;
  TerritoryMap(double _originX, double _originZ, double _side, double _cellSize);

  [[nodiscard]] std::int32_t CellsPerAxis() const noexcept
  {
    return m_cellsPerAxis;
  }
  [[nodiscard]] double CellSize() const noexcept
  {
    return m_cellSize;
  }
  // The cells held.
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_count;
  }
  // The centre of a cell, at height 0.
  [[nodiscard]] Neuron::Vec3 CellCenter(std::int32_t _x, std::int32_t _z) const noexcept;
  [[nodiscard]] bool HoldsCell(std::int32_t _x, std::int32_t _z) const noexcept;
  void Hold(std::int32_t _x, std::int32_t _z);
  // The cell that holds the point, which is held; false off the raster.
  [[nodiscard]] bool Holds(double _x, double _z) const noexcept;
  // MVP-032: the disc of _radius about (_x, _z) lies in the territory: the cell of its centre and every cell whose
  // centre lies within it are held.
  [[nodiscard]] bool HoldsDisc(double _x, double _z, double _radius) const noexcept;

private:
  [[nodiscard]] std::int32_t CellOf(double _coordinate, double _origin) const noexcept;

  double m_originX = 0.0;
  double m_originZ = 0.0;
  double m_cellSize = 0.0;
  std::int32_t m_cellsPerAxis = 0;
  std::vector<std::uint8_t> m_cells;
  std::size_t m_count = 0;
};

// WLD-060 and WLD-061 for a point of the settlement at _site, before its radius: inside the coast bands, within
// wld.territory.max_radius of its site's centre and nearer to it than to either valley neighbour's, so in its home cell
// or its own valley border areas; at least wld.territory.site_clearance outside the core of every other site; at least
// wld.resource.wild_clearance outside every central resource site's footprint; and on ground above the shore height.
[[nodiscard]] bool WithinTerritoryBounds(const RegionSimulation& _region, std::uint8_t _site, double _x, double _z);

// MVP-026 (ADR-134): the settlement's territory, the cells within its radius of its Wellspring and within the bounds,
// connected to the Wellspring's cell by their sides, so that it is one connected area (WLD-060).
[[nodiscard]] TerritoryMap ComputeTerritory(const RegionSimulation& _region, const Settlement& _settlement);

} // namespace Species
