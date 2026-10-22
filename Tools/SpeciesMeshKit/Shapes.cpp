#include "pch.h"

#include "Shapes.h"
#include "UnitCircle.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace MeshKit
{

namespace
{

Point Sub(const Point& _a, const Point& _b) noexcept
{
  return {_a[0] - _b[0], _a[1] - _b[1], _a[2] - _b[2]};
}

Point Cross(const Point& _a, const Point& _b) noexcept
{
  return {_a[1] * _b[2] - _a[2] * _b[1], _a[2] * _b[0] - _a[0] * _b[2], _a[0] * _b[1] - _a[1] * _b[0]};
}

double Dot(const Point& _a, const Point& _b) noexcept
{
  return _a[0] * _b[0] + _a[1] * _b[1] + _a[2] * _b[2];
}

Direction DirectionOf(std::uint32_t _step, std::uint32_t _steps)
{
  const std::optional<Direction> direction = TurnDirection(_step, _steps);
  if (!direction)
  {
    throw std::logic_error("a shape asks for " + std::to_string(_steps) + " steps of a turn, which the kit has no exact directions for");
  }
  return direction.value_or(Direction{});
}

// Adds the triangle wound so that its facet normal points along _outward: counter-clockwise seen from outside.
void AddFacing(ModelNode& _node, const Point& _a, const Point& _b, const Point& _c, const Point& _outward, Color _color, Surface _surface)
{
  const Point normal = Cross(Sub(_b, _a), Sub(_c, _a));
  const double facing = Dot(normal, _outward);
  if (Dot(normal, normal) == 0.0 || facing == 0.0)
  {
    throw std::logic_error("the node " + _node.name + " would hold a triangle with no area or no outside");
  }
  Triangle triangle;
  triangle.corners = facing > 0.0 ? std::array<Point, 3>{_a, _b, _c} : std::array<Point, 3>{_a, _c, _b};
  triangle.color = _color;
  triangle.surface = _surface;
  _node.triangles.push_back(triangle);
}

void AddQuad(ModelNode& _node, const std::array<Point, 4>& _corners, const Point& _outward, Color _color, Surface _surface)
{
  AddFacing(_node, _corners[0], _corners[1], _corners[2], _outward, _color, _surface);
  AddFacing(_node, _corners[0], _corners[2], _corners[3], _outward, _color, _surface);
}

// A cap of a regular polygon, as a fan from its first corner.
void AddCap(ModelNode& _node, const std::vector<Point>& _ring, const Point& _outward, Color _color, Surface _surface)
{
  for (std::size_t i = 1; i + 1 < _ring.size(); ++i)
  {
    AddFacing(_node, _ring[0], _ring[i], _ring[i + 1], _outward, _color, _surface);
  }
}

} // namespace

void AddFrustum(ModelNode& _node, const Frustum& _frustum)
{
  const bool up = _frustum.axis == Axis::Up;
  std::vector<Point> bottom;
  std::vector<Point> top;
  std::vector<Point> radial;
  for (std::uint32_t i = 0; i < _frustum.sides; ++i)
  {
    const Direction d = _frustum.halfStep ? DirectionOf(2 * i + 1, 2 * _frustum.sides) : DirectionOf(i, _frustum.sides);
    const Point& b = _frustum.base;
    if (up)
    {
      bottom.push_back({b[0] + _frustum.baseRadius * d.cosine, b[1], b[2] + _frustum.baseRadius * d.sine});
      top.push_back({b[0] + _frustum.topRadius * d.cosine, b[1] + _frustum.length, b[2] + _frustum.topRadius * d.sine});
      radial.push_back({d.cosine, 0.0, d.sine});
    }
    else
    {
      bottom.push_back({b[0] + _frustum.baseRadius * d.cosine, b[1] + _frustum.baseRadius * d.sine, b[2]});
      top.push_back({b[0] + _frustum.topRadius * d.cosine, b[1] + _frustum.topRadius * d.sine, b[2] + _frustum.length});
      radial.push_back({d.cosine, d.sine, 0.0});
    }
  }
  const Point axis = up ? Point{0.0, 1.0, 0.0} : Point{0.0, 0.0, 1.0};
  for (std::uint32_t i = 0; i < _frustum.sides; ++i)
  {
    const std::uint32_t next = (i + 1) % _frustum.sides;
    const Point outward{radial[i][0] + radial[next][0], radial[i][1] + radial[next][1], radial[i][2] + radial[next][2]};
    if (_frustum.topRadius == 0.0)
    {
      AddFacing(_node, bottom[i], bottom[next], top[i], outward, _frustum.color, _frustum.surface);
    }
    else if (_frustum.baseRadius == 0.0)
    {
      AddFacing(_node, bottom[i], top[next], top[i], outward, _frustum.color, _frustum.surface);
    }
    else
    {
      AddQuad(_node, {bottom[i], bottom[next], top[next], top[i]}, outward, _frustum.color, _frustum.surface);
    }
  }
  if (_frustum.baseCap && _frustum.baseRadius > 0.0)
  {
    AddCap(_node, bottom, {-axis[0], -axis[1], -axis[2]}, _frustum.capColor, _frustum.surface);
  }
  if (_frustum.topCap && _frustum.topRadius > 0.0)
  {
    AddCap(_node, top, axis, _frustum.capColor, _frustum.surface);
  }
}

void AddBox(ModelNode& _node, const Point& _min, const Point& _max, Color _color, Surface _surface, bool _bottom)
{
  const double x0 = _min[0];
  const double y0 = _min[1];
  const double z0 = _min[2];
  const double x1 = _max[0];
  const double y1 = _max[1];
  const double z1 = _max[2];
  AddQuad(_node, {{{x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1}}}, {0.0, 1.0, 0.0}, _color, _surface);
  AddQuad(_node, {{{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}}}, {0.0, 0.0, -1.0}, _color, _surface);
  AddQuad(_node, {{{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}}}, {0.0, 0.0, 1.0}, _color, _surface);
  AddQuad(_node, {{{x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}}}, {-1.0, 0.0, 0.0}, _color, _surface);
  AddQuad(_node, {{{x1, y0, z0}, {x1, y0, z1}, {x1, y1, z1}, {x1, y1, z0}}}, {1.0, 0.0, 0.0}, _color, _surface);
  if (_bottom)
  {
    AddQuad(_node, {{{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}}, {0.0, -1.0, 0.0}, _color, _surface);
  }
}

void AddGable(ModelNode& _node, const Point& _min, const Point& _max, Color _roof, Color _ends, Surface _surface)
{
  const double x0 = _min[0];
  const double x1 = _max[0];
  const double y0 = _min[1];
  const double y1 = _max[1];
  const double z0 = _min[2];
  const double z1 = _max[2];
  const double zm = (z0 + z1) / 2.0;
  AddQuad(_node, {{{x0, y0, z0}, {x1, y0, z0}, {x1, y1, zm}, {x0, y1, zm}}}, {0.0, 1.0, -1.0}, _roof, _surface);
  AddQuad(_node, {{{x0, y0, z1}, {x1, y0, z1}, {x1, y1, zm}, {x0, y1, zm}}}, {0.0, 1.0, 1.0}, _roof, _surface);
  AddFacing(_node, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, zm}, {-1.0, 0.0, 0.0}, _ends, _surface);
  AddFacing(_node, {x1, y0, z0}, {x1, y0, z1}, {x1, y1, zm}, {1.0, 0.0, 0.0}, _ends, _surface);
}

Point Around(const Point& _center, double _radius, std::uint32_t _step, std::uint32_t _steps, double _y)
{
  const Direction d = DirectionOf(_step, _steps);
  return {_center[0] + _radius * d.cosine, _y, _center[2] + _radius * d.sine};
}

} // namespace MeshKit
