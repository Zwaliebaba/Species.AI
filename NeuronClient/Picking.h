#pragma once

#include "FrameView.h"
#include "Vec3.h"
#include "WindowGeometry.h"

#include <optional>

namespace Neuron
{

/// A ray in world units, in binary64 (CAM-095 amendment, WLD-246): its origin and its unit direction.
struct Ray
{
  Vec3 origin;
  Vec3 direction;
};

/// A world point on the screen (RND-216): pixels of the 2-D overlay space, origin at the top-left and y down, and the
/// clip w, which is the point's distance in front of the camera along the view direction.
struct ScreenPoint
{
  double x = 0.0;
  double y = 0.0;
  double w = 0.0;
};

// CAM-090: the ray from the camera through the pointer at (_x, _y) client pixels of a target of _size, for _frame. The
// pointer maps to x = 2x ÷ W − 1 and y = 1 − 2 (y + _rowOffsetPixels) ÷ H (DEC-956); the origin is that point at the
// near plane (reversed-Z depth 1) and the direction runs to the far plane (depth 0), both through the inverse of the
// frame's view-projection. The frame is camera-relative, so the camera's binary64 position is added after.
[[nodiscard]] Ray PickRayAt(double _x, double _y, const FrameView& _frame, SizePx _size, double _rowOffsetPixels) noexcept;

// RND-216: the world point on the screen through _frame, or none when it lies at or behind the camera (w ≤ 0).
[[nodiscard]] std::optional<ScreenPoint> ProjectToScreen(const Vec3& _world, const FrameView& _frame, SizePx _size) noexcept;

// The ray's first meeting with a sphere at its distance t ≥ 0 along the ray; none when it misses or lies behind.
[[nodiscard]] std::optional<double> RaySphere(const Ray& _ray, const Vec3& _center, double _radius) noexcept;

} // namespace Neuron
