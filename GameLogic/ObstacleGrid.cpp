#include "pch.h"

#include "Check.h"
#include "ObstacleGrid.h"

#include <cmath>

namespace Species
{

bool SphereHits(const ObstacleShape& _shape, const Neuron::Vec3& _center, double _radius) noexcept
{
  const double reach = _shape.radius + _radius;
  return Neuron::LengthSquared(_center - _shape.position) < reach * reach;
}

bool RayHits(const ObstacleShape& _shape, const Neuron::Vec3& _origin, const Neuron::Vec3& _direction, double _length) noexcept
{
  // The nearest point of the segment to the sphere's centre, the parameter clamped to the segment.
  const Neuron::Vec3 toCenter = _shape.position - _origin;
  const double along = std::clamp(Neuron::Dot(toCenter, _direction), 0.0, std::max(_length, 0.0));
  const Neuron::Vec3 nearest = _origin + _direction * along;
  return Neuron::LengthSquared(_shape.position - nearest) < _shape.radius * _shape.radius;
}

ObstacleGrid::ObstacleGrid(const Desc& _desc)
  : m_desc(_desc),
    m_cellsPerAxis(static_cast<std::int32_t>(std::ceil(_desc.worldSize / _desc.cellSize)))
{
  ENGINE_CHECK(_desc.cellSize > 0.0 && m_cellsPerAxis > 0);
  m_cells.resize(static_cast<std::size_t>(m_cellsPerAxis) * static_cast<std::size_t>(m_cellsPerAxis));
}

std::int32_t ObstacleGrid::CellOf(double _coordinate, double _origin) const noexcept
{
  // R-SIM-324: floor, so a negative coordinate is a negative cell, which is the outside cell's. The clamp keeps a
  // coordinate far off the grid representable; any value past the grid is outside.
  const double cell = std::floor((_coordinate - _origin) / m_desc.cellSize);
  return static_cast<std::int32_t>(std::clamp(cell, -1.0, static_cast<double>(m_cellsPerAxis)));
}

std::int32_t ObstacleGrid::RangeOf(double _radius) const noexcept
{
  return static_cast<std::int32_t>(std::ceil(_radius / m_desc.cellSize));
}

std::vector<std::uint32_t>& ObstacleGrid::CellAt(std::int32_t _x, std::int32_t _z) noexcept
{
  if (_x < 0 || _z < 0 || _x >= m_cellsPerAxis || _z >= m_cellsPerAxis)
  {
    return m_outside;
  }
  return m_cells[static_cast<std::size_t>(_z) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(_x)];
}

const std::vector<std::uint32_t>& ObstacleGrid::StructuresAt(double _x, double _z) const noexcept
{
  const std::int32_t x = CellOf(_x, m_desc.originX);
  const std::int32_t z = CellOf(_z, m_desc.originZ);
  if (x < 0 || z < 0 || x >= m_cellsPerAxis || z >= m_cellsPerAxis)
  {
    return m_outside;
  }
  return m_cells[static_cast<std::size_t>(z) * static_cast<std::size_t>(m_cellsPerAxis) + static_cast<std::size_t>(x)];
}

const ObstacleShape& ObstacleGrid::Shape(std::uint32_t _index) const
{
  ENGINE_CHECK_INDEX(_index, m_shapes.size());
  return m_shapes[_index];
}

} // namespace Species
