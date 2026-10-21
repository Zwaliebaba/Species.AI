#include "pch.h"

#include "Check.h"
#include "ObstacleGrid.h"

#include <cmath>

namespace Species
{

namespace
{

Neuron::Vec3 CrossProduct(const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

// The point of the triangle nearest _p (Ericson, Real-Time Collision Detection, 5.1.5), in the regions of its vertices,
// its edges and its face.
Neuron::Vec3 NearestOnTriangle(const Neuron::Vec3& _p, const std::array<Neuron::Vec3, 3>& _t) noexcept
{
  const Neuron::Vec3& a = _t[0];
  const Neuron::Vec3& b = _t[1];
  const Neuron::Vec3& c = _t[2];
  const Neuron::Vec3 ab = b - a;
  const Neuron::Vec3 ac = c - a;
  const Neuron::Vec3 ap = _p - a;
  const double d1 = Neuron::Dot(ab, ap);
  const double d2 = Neuron::Dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0)
  {
    return a;
  }
  const Neuron::Vec3 bp = _p - b;
  const double d3 = Neuron::Dot(ab, bp);
  const double d4 = Neuron::Dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3)
  {
    return b;
  }
  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
  {
    return a + ab * (d1 / (d1 - d3));
  }
  const Neuron::Vec3 cp = _p - c;
  const double d5 = Neuron::Dot(ab, cp);
  const double d6 = Neuron::Dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6)
  {
    return c;
  }
  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
  {
    return a + ac * (d2 / (d2 - d6));
  }
  const double va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
  {
    return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
  }
  const double denominator = 1.0 / (va + vb + vc);
  return a + ab * (vb * denominator) + ac * (vc * denominator);
}

// The segment from _origin along the unit _direction for _length meets the triangle (Möller and Trumbore), from either
// side; a segment in the triangle's plane meets nothing.
bool SegmentMeetsTriangle(const Neuron::Vec3& _origin, const Neuron::Vec3& _direction, double _length,
                          const std::array<Neuron::Vec3, 3>& _t) noexcept
{
  const Neuron::Vec3 edge1 = _t[1] - _t[0];
  const Neuron::Vec3 edge2 = _t[2] - _t[0];
  const Neuron::Vec3 p = CrossProduct(_direction, edge2);
  const double determinant = Neuron::Dot(edge1, p);
  if (determinant == 0.0)
  {
    return false;
  }
  const double inverse = 1.0 / determinant;
  const Neuron::Vec3 s = _origin - _t[0];
  const double u = Neuron::Dot(s, p) * inverse;
  if (u < 0.0 || u > 1.0)
  {
    return false;
  }
  const Neuron::Vec3 q = CrossProduct(s, edge1);
  const double v = Neuron::Dot(_direction, q) * inverse;
  if (v < 0.0 || u + v > 1.0)
  {
    return false;
  }
  const double along = Neuron::Dot(edge2, q) * inverse;
  return along >= 0.0 && along <= _length;
}

} // namespace

bool SphereHits(const ObstacleShape& _shape, const Neuron::Vec3& _center, double _radius) noexcept
{
  const double reach = _shape.radius + _radius;
  if (Neuron::LengthSquared(_center - _shape.position) >= reach * reach)
  {
    return false;
  }
  return _shape.triangles.empty() ||
         std::ranges::any_of(_shape.triangles, [&](const std::array<Neuron::Vec3, 3>& _triangle)
                             { return Neuron::LengthSquared(NearestOnTriangle(_center, _triangle) - _center) < _radius * _radius; });
}

bool RayHits(const ObstacleShape& _shape, const Neuron::Vec3& _origin, const Neuron::Vec3& _direction, double _length) noexcept
{
  // The nearest point of the segment to the sphere's centre, the parameter clamped to the segment.
  const Neuron::Vec3 toCenter = _shape.position - _origin;
  const double along = std::clamp(Neuron::Dot(toCenter, _direction), 0.0, std::max(_length, 0.0));
  const Neuron::Vec3 nearest = _origin + _direction * along;
  if (Neuron::LengthSquared(_shape.position - nearest) >= _shape.radius * _shape.radius)
  {
    return false;
  }
  return _shape.triangles.empty() ||
         std::ranges::any_of(_shape.triangles, [&](const std::array<Neuron::Vec3, 3>& _triangle)
                             { return SegmentMeetsTriangle(_origin, _direction, std::max(_length, 0.0), _triangle); });
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
