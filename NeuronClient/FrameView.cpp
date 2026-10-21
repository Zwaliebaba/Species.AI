#include "pch.h"

#include "FrameView.h"
#include "Geometry.h"

#include <algorithm>
#include <cmath>

namespace Neuron
{

namespace
{

// The largest field of view a perspective projection takes: a half-angle just short of 90°.
constexpr double FOV_LIMIT_RADIANS = 3.14;

} // namespace

Vec3 ViewDirection(const CameraPose& _pose) noexcept
{
  const double cosPitch = std::cos(_pose.pitchRadians);
  return {-std::sin(_pose.yawRadians) * cosPitch, std::sin(_pose.pitchRadians), -std::cos(_pose.yawRadians) * cosPitch};
}

FrameView MakeFrameView(const CameraPose& _pose, double _aspect, const ProjectionPlanes& _planes) noexcept
{
  using namespace DirectX;
  const Vec3 direction = ViewDirection(_pose);
  const XMVECTOR eye = XMVectorZero();
  const XMVECTOR forward =
    XMVectorSet(static_cast<float>(direction.x), static_cast<float>(direction.y), static_cast<float>(direction.z), 0.0f);
  // VFX-232: the roll turns the world's up about the front (Rodrigues' rotation), and the view re-orthogonalises it.
  const Vec3 worldUp{0.0, 1.0, 0.0};
  const double cosRoll = std::cos(_pose.rollRadians);
  const double sinRoll = std::sin(_pose.rollRadians);
  const Vec3 up = worldUp * cosRoll + Cross(direction, worldUp) * sinRoll + direction * (Dot(direction, worldUp) * (1.0 - cosRoll));
  const XMMATRIX view =
    XMMatrixLookToRH(eye, forward, XMVectorSet(static_cast<float>(up.x), static_cast<float>(up.y), static_cast<float>(up.z), 0.0f));
  // RND-212: the field of view is clamped before every projection, and kept within what a projection can take.
  const double fov = std::min(
    std::clamp(_pose.verticalFovRadians, _planes.fovMinRadians, std::max(_planes.fovMinRadians, _planes.fovMaxRadians)), FOV_LIMIT_RADIANS);
  // Reversed Z: the planes are swapped, so the near plane maps to 1 and the far plane to 0.
  const XMMATRIX projection = XMMatrixPerspectiveFovRH(static_cast<float>(fov), static_cast<float>(std::max(_aspect, 1e-3)),
                                                       static_cast<float>(_planes.farPlane), static_cast<float>(_planes.nearPlane));
  FrameView frame;
  XMStoreFloat4x4(&frame.view, view);
  XMStoreFloat4x4(&frame.projection, projection);
  XMStoreFloat4x4(&frame.viewProjection, XMMatrixMultiply(view, projection));
  frame.camera = _pose.position;
  return frame;
}

Float3x4 CameraRelativeTransform(const Vec3& _origin, const Vec3& _camera) noexcept
{
  const auto x = static_cast<float>(_origin.x - _camera.x);
  const auto y = static_cast<float>(_origin.y - _camera.y);
  const auto z = static_cast<float>(_origin.z - _camera.z);
  return Float3x4(1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y, 0.0f, 0.0f, 1.0f, z);
}

std::array<Float4, 6> FrustumPlanes(const FrameView& _frame) noexcept
{
  // Row vectors: clip = p × M, so each plane is a sum of the matrix's columns (Gribb and Hartmann), with depth in [0, w].
  const Float4x4& m = _frame.viewProjection;
  const auto column = [&m](int _c) { return std::array<float, 4>{m.m[0][_c], m.m[1][_c], m.m[2][_c], m.m[3][_c]}; };
  const std::array<float, 4> x = column(0);
  const std::array<float, 4> y = column(1);
  const std::array<float, 4> z = column(2);
  const std::array<float, 4> w = column(3);
  const auto plane = [](const std::array<float, 4>& _a, const std::array<float, 4>& _b, float _sign)
  { return Float4(_a[0] + _sign * _b[0], _a[1] + _sign * _b[1], _a[2] + _sign * _b[2], _a[3] + _sign * _b[3]); };
  return {plane(w, x, 1.0f), plane(w, x, -1.0f), plane(w, y, 1.0f), plane(w, y, -1.0f), Float4(z[0], z[1], z[2], z[3]), plane(w, z, -1.0f)};
}

bool BoxMayBeVisible(const std::array<Float4, 6>& _planes, const Vec3& _min, const Vec3& _max) noexcept
{
  for (const Float4& plane : _planes)
  {
    // The corner furthest along the plane's normal.
    const double x = plane.x >= 0.0f ? _max.x : _min.x;
    const double y = plane.y >= 0.0f ? _max.y : _min.y;
    const double z = plane.z >= 0.0f ? _max.z : _min.z;
    if (static_cast<double>(plane.x) * x + static_cast<double>(plane.y) * y + static_cast<double>(plane.z) * z +
          static_cast<double>(plane.w) <
        0.0)
    {
      return false;
    }
  }
  return true;
}

} // namespace Neuron
