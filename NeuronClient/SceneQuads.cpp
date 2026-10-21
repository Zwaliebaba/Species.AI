#include "pch.h"

#include "FrameView.h"
#include "SceneQuads.h"

#include <bit>

namespace Neuron
{

namespace
{

Float4 Relative(const Vec3& _point, const Vec3& _camera, float _w) noexcept
{
  return Float4(static_cast<float>(_point.x - _camera.x), static_cast<float>(_point.y - _camera.y),
                static_cast<float>(_point.z - _camera.z), _w);
}

Float4 Direction(const Vec3& _axis, float _w) noexcept
{
  return Float4(static_cast<float>(_axis.x), static_cast<float>(_axis.y), static_cast<float>(_axis.z), _w);
}

Float4 Color(const std::array<float, 4>& _color) noexcept
{
  return Float4(_color[0], _color[1], _color[2], _color[3]);
}

} // namespace

QuadInstance MakeQuadInstance(const SceneQuad& _quad, const Vec3& _camera) noexcept
{
  QuadInstance instance{};
  instance.position = Relative(_quad.position, _camera, _quad.halfWidth);
  instance.endPoint = Relative(_quad.endPoint, _camera, _quad.halfHeight);
  instance.axisRight = Direction(_quad.axisRight, _quad.blendStart);
  instance.axisUp = Direction(_quad.axisUp, _quad.blendLength);
  instance.color = Color(_quad.color);
  instance.endColor = Color(_quad.endColor);
  instance.uv = Color(_quad.uv);
  instance.shape = Uint4{static_cast<std::uint32_t>(_quad.orientation), static_cast<std::uint32_t>(_quad.anchor),
                         std::bit_cast<std::uint32_t>(_quad.rotationRadians), _quad.blendLeft ? QUAD_BLEND_LEFT : 0u};
  return instance;
}

MeshInstance MakeMeshInstance(const SceneMeshInstance& _instance, const Vec3& _camera) noexcept
{
  // The transpose of the row-vector transform, as DrawConstants::world: row r holds the r-th world component of each
  // model axis, then of the origin.
  const Vec3 origin = _instance.position - _camera;
  const auto row = [&](double _x, double _y, double _z, double _t)
  { return Float4(static_cast<float>(_x), static_cast<float>(_y), static_cast<float>(_z), static_cast<float>(_t)); };
  MeshInstance instance{};
  instance.row0 = row(_instance.axisX.x, _instance.axisY.x, _instance.axisZ.x, origin.x);
  instance.row1 = row(_instance.axisX.y, _instance.axisY.y, _instance.axisZ.y, origin.y);
  instance.row2 = row(_instance.axisX.z, _instance.axisY.z, _instance.axisZ.z, origin.z);
  instance.tint = Float4(_instance.tint[0], _instance.tint[1], _instance.tint[2], _instance.tintWeight);
  return instance;
}

TriangleVertex MakeTriangleVertex(const SceneVertex& _vertex, const Vec3& _camera) noexcept
{
  return TriangleVertex{Relative(_vertex.position, _camera, 1.0f), Float4(_vertex.uv[0], _vertex.uv[1], 0.0f, 0.0f), Color(_vertex.color)};
}

bool QuadMayBeVisible(const std::array<Float4, 6>& _planes, const SceneQuad& _quad, const Vec3& _camera, float _radius) noexcept
{
  Vec3 center = _quad.position;
  double radius = _radius;
  if (_quad.orientation == QuadOrientation::Segment)
  {
    center = (_quad.position + _quad.endPoint) * 0.5;
    radius += Length(_quad.endPoint - _quad.position) * 0.5;
  }
  const Vec3 relative = center - _camera;
  const Vec3 extent{radius, radius, radius};
  return BoxMayBeVisible(_planes, relative - extent, relative + extent);
}

} // namespace Neuron
