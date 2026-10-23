#pragma once

#include "RegionTerrain.h"
#include "ReplicatedState.h"
#include "StateReplica.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

/// The steward's areas of the settlement view (WLD-244; M2 task T13, ADR-142): the steward's own territory as the replica
/// holds it, and its surroundings up to wld.view.settlement_margin beyond it. They are cells of mvp.territory.cell_size
/// over the region's square widened by the margin on each side, so that the surroundings of a territory near the coast
/// reach past the square: a cell is in the areas when its centre lies within the margin of the centre of a territory
/// cell. From M3 task T12 (ADR-167) the battlefield of each battle the steward's settlement is a side of joins them for
/// the battle's duration: a cell whose centre lies within its radius of its centre. Until a chunk of the steward's
/// territory arrives they are unknown, and every point is inside. Frame thread only.
class StewardAreas
{
public:
  struct Point
  {
    double x = 0.0;
    double z = 0.0;
  };
  // WLD-244: a battlefield, its centre and radius in wu.
  struct Field
  {
    double x = 0.0;
    double z = 0.0;
    double radius = 0.0;

    friend bool operator==(const Field&, const Field&) = default;
  };

  // Reads the territory of the settlement of _team in _latest, and rebuilds the areas when its chunks, or _fields, the
  // battlefields that join them, differ from those they were built from. None for _team, or no settlement of it, leaves
  // them unknown.
  void Update(const StateReplica::View& _latest, std::optional<std::uint8_t> _team, const RegionLayout& _layout, const TuningMvp& _tuning,
              std::span<const Field> _fields = {});
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
  // The cells of the areas, z outer and x inner: 1 for a cell in them.
  [[nodiscard]] const std::vector<std::uint8_t>& InsideCells() const noexcept
  {
    return m_inside;
  }

private:
  [[nodiscard]] std::int32_t CellOf(double _coordinate, double _origin) const noexcept;
  [[nodiscard]] std::size_t Index(std::int32_t _x, std::int32_t _z) const noexcept;
  void Rebuild(const RegionLayout& _layout, const TuningMvp& _tuning);
  // The cells within _margin of a territory cell, over the territory's box of cells from (_minX, _minZ) to (_maxX, _maxZ)
  // widened by the margin, in the raster's cells.
  void MarkNearTerritory(std::int32_t _minX, std::int32_t _minZ, std::int32_t _maxX, std::int32_t _maxZ, double _margin);

  std::vector<ReplicatedCells> m_chunks; // the territory's chunks the areas were built from, in id order
  std::vector<Field> m_fields;           // the battlefields they were built with
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

// ADR-144: the view mask of the areas for the terrain and the sea, 255 in them and 0 outside, its edge softened over
// mvp.view_fog.edge_width, at the fog's strength mvp.view_fog.strength; none while they are unknown.
[[nodiscard]] std::shared_ptr<const Neuron::ViewMask> BuildViewMask(const StewardAreas& _areas, const TuningPresentation& _presentation);

// WLD-244 (ADR-144): leaves out of _objects each one whose place, _placeOf(object), lies outside the areas, which draw no
// Worker or structure; nothing while the areas are unknown.
template <class T, class PlaceOf> void EraseOutside(const StewardAreas& _areas, std::vector<T>& _objects, PlaceOf _placeOf)
{
  if (_areas.Known())
  {
    std::erase_if(_objects,
                  [&](const T& _object)
                  {
                    const Neuron::Vec3 place = _placeOf(_object);
                    return !_areas.Contains(place.x, place.z);
                  });
  }
}

} // namespace Species
