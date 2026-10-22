#pragma once

#include "Vec3.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

/// A structure's hit volume as the obstacle grid and the pushes test it (ADR-103, ADR-131): its centre and the radius of
/// its bounding sphere, which the pushes measure from (R-UNIT-114, R-SIM-380), and the triangles of its collision asset
/// in the world, which its sphere and ray tests read (R-SIM-123). A shape with no triangles is the sphere itself, as a
/// structure without a collision asset and the tests' shapes are. The structure's slot, for one that is a structure; and
/// a turret's ray sphere, which its ray test hits besides its triangles (R-BLDG-407), none at a radius of 0.
struct ObstacleShape
{
  Neuron::Vec3 position;
  double radius = 0.0;
  std::vector<std::array<Neuron::Vec3, 3>> triangles;
  std::optional<std::uint32_t> structure;
  Neuron::Vec3 rayCenter;
  double rayRadius = 0.0;
};

// The structure's sphere hit test: a sphere of _radius at _center overlaps the bounding sphere and, when the shape has
// triangles, comes strictly within _radius of one of them.
[[nodiscard]] bool SphereHits(const ObstacleShape& _shape, const Neuron::Vec3& _center, double _radius) noexcept;
// The structure's ray hit test: the segment from _origin along the unit vector _direction for _length passes within
// the bounding sphere and then passes within its ray sphere or meets one of its triangles; a shape with neither is its
// bounding sphere.
[[nodiscard]] bool RayHits(const ObstacleShape& _shape, const Neuron::Vec3& _origin, const Neuron::Vec3& _direction,
                           double _length) noexcept;

/// The obstacle grid of a region (R-SIM-320 to R-SIM-325): a surface grid over the region's square whose cells list the
/// structures whose hit volume overlaps them, in structure order, and one outside cell that every coordinate off the
/// grid reads. It is derived from the structures, so neither the hash nor the snapshot holds it.
class ObstacleGrid
{
public:
  struct Desc
  {
    double originX = 0.0;
    double originZ = 0.0;
    double worldSize = 0.0;
    double cellSize = 32.0;        // ref.sim.obstacle_grid.cell_size
    double cellTestFraction = 0.5; // ref.sim.obstacle_grid.cell_test_fraction
  };

  explicit ObstacleGrid(const Desc& _desc);

  // R-SIM-320: ceil(side ÷ cell size).
  [[nodiscard]] std::int32_t CellsPerAxis() const noexcept
  {
    return m_cellsPerAxis;
  }

  // R-SIM-321 for the next structure in structure order: its index in every cell of the square around its centre's
  // cell whose centre, at the terrain height _height gives, the shape's sphere test hits; both upper bounds inclusive,
  // so a shape that reaches past the far edge also lists itself in the outside cell.
  template <class Height> std::uint32_t Add(const ObstacleShape& _shape, Height&& _height)
  {
    const auto index = static_cast<std::uint32_t>(m_shapes.size());
    m_shapes.push_back(_shape);
    const std::int32_t centerX = CellOf(_shape.position.x, m_desc.originX);
    const std::int32_t centerZ = CellOf(_shape.position.z, m_desc.originZ);
    const std::int32_t range = RangeOf(_shape.radius);
    const double testRadius = m_desc.cellSize * m_desc.cellTestFraction;
    for (std::int32_t x = std::max(centerX - range, 0); x <= std::min(centerX + range, m_cellsPerAxis); ++x)
    {
      for (std::int32_t z = std::max(centerZ - range, 0); z <= std::min(centerZ + range, m_cellsPerAxis); ++z)
      {
        const double cellCenterX = m_desc.originX + (x + 0.5) * m_desc.cellSize;
        const double cellCenterZ = m_desc.originZ + (z + 0.5) * m_desc.cellSize;
        if (SphereHits(_shape, {cellCenterX, _height(cellCenterX, cellCenterZ), cellCenterZ}, testRadius))
        {
          CellAt(x, z).push_back(index);
        }
      }
    }
    return index;
  }

  // R-SIM-324: the list of the cell under (_x, _z), with no interpolation; the outside cell's off the grid.
  [[nodiscard]] const std::vector<std::uint32_t>& StructuresAt(double _x, double _z) const noexcept;
  [[nodiscard]] const ObstacleShape& Shape(std::uint32_t _index) const;
  [[nodiscard]] bool Empty() const noexcept
  {
    return m_shapes.empty();
  }

private:
  [[nodiscard]] std::int32_t CellOf(double _coordinate, double _origin) const noexcept;
  [[nodiscard]] std::int32_t RangeOf(double _radius) const noexcept;
  std::vector<std::uint32_t>& CellAt(std::int32_t _x, std::int32_t _z) noexcept;

  Desc m_desc;
  std::int32_t m_cellsPerAxis;
  std::vector<std::vector<std::uint32_t>> m_cells;
  std::vector<std::uint32_t> m_outside;
  std::vector<ObstacleShape> m_shapes; // by structure index
};

} // namespace Species
