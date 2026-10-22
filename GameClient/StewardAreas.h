#pragma once

#include "RegionTerrain.h"
#include "ReplicatedState.h"
#include "StateReplica.h"
#include "TuningMvp.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

/// The steward's areas of the settlement view (WLD-244; M2 task T13, ADR-142): the steward's own territory as the replica
/// holds it, and its surroundings up to wld.view.settlement_margin beyond it. They are cells of mvp.territory.cell_size
/// over the region's square widened by the margin on each side, so that the surroundings of a territory near the coast
/// reach past the square: a cell is in the areas when its centre lies within the margin of the centre of a territory
/// cell. M2 has no battle, so no battlefield joins them. Until a chunk of the steward's territory arrives they are
/// unknown, and every point is inside. Frame thread only.
class StewardAreas
{
public:
  struct Point
  {
    double x = 0.0;
    double z = 0.0;
  };

  // Reads the territory of the settlement of _team in _latest, and rebuilds the areas when its chunks differ from those
  // they were built from. None for _team, or no settlement of it, leaves them unknown.
  void Update(const StateReplica::View& _latest, std::optional<std::uint8_t> _team, const RegionLayout& _layout, const TuningMvp& _tuning);
  // Forgets the areas, as at a new world.
  void Clear() noexcept;

  [[nodiscard]] bool Known() const noexcept
  {
    return !m_chunks.empty();
  }
  // The local site of the settlement whose territory they hold; none while unknown.
  [[nodiscard]] std::optional<std::uint8_t> Site() const noexcept
  {
    return Known() ? std::optional<std::uint8_t>(m_site) : std::nullopt;
  }
  // Counts the rebuilds, so that a reader can tell when they changed.
  [[nodiscard]] std::uint64_t Revision() const noexcept
  {
    return m_revision;
  }
  // The point lies in a cell of the areas; everywhere while they are unknown.
  [[nodiscard]] bool Contains(double _x, double _z) const noexcept;
  // The point lies in a cell of the steward's territory; nowhere while they are unknown.
  [[nodiscard]] bool InTerritory(double _x, double _z) const noexcept;
  // The centre of the cell of the areas nearest the point, for a point outside them; the point itself when it is inside,
  // and none while they are unknown.
  [[nodiscard]] std::optional<Point> Nearest(double _x, double _z) const;

  // The raster: its corner of least x and z, its cell size, and its cells a side.
  [[nodiscard]] Point Origin() const noexcept
  {
    return {m_originX, m_originZ};
  }
  [[nodiscard]] double CellSize() const noexcept
  {
    return m_cellSize;
  }
  [[nodiscard]] std::int32_t CellsPerAxis() const noexcept
  {
    return m_cellsPerAxis;
  }

private:
  [[nodiscard]] std::int32_t CellOf(double _coordinate, double _origin) const noexcept;
  [[nodiscard]] std::size_t Index(std::int32_t _x, std::int32_t _z) const noexcept;
  void Rebuild(const RegionLayout& _layout, const TuningMvp& _tuning);

  std::vector<ReplicatedCells> m_chunks; // the territory's chunks the areas were built from, in id order
  std::uint8_t m_site = 0;
  std::vector<std::uint8_t> m_territory; // 1: a cell of the territory
  std::vector<std::uint8_t> m_inside;    // 1: a cell of the areas
  double m_originX = 0.0;
  double m_originZ = 0.0;
  double m_cellSize = 1.0;
  std::int32_t m_cellsPerAxis = 0;
  std::int32_t m_padCells = 0; // the cells the raster reaches beyond the region's square on each side
  std::uint64_t m_revision = 0;
};

} // namespace Species
