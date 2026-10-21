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
