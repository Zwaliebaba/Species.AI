#pragma once

#include "Vec3.h"

#include <optional>

namespace Neuron
{

[[nodiscard]] constexpr Vec3 Cross(const Vec3& _a, const Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

// PLT-138: a ray's hit inside a triangle, as the ray parameter and the barycentric coordinates of the hit.
struct TriangleHit
{
  double t = 0.0;
  double u = 0.0;
  double v = 0.0;
};

// PLT-138, DEC-241 item 10: the ray–triangle intersection. No hit when the ray is parallel to the triangle's plane,
// misses the triangle, or when the (t, u, v) vector is longer than _range or not finite. The hit may lie behind the
// ray's start (t < 0), which terrain picking relies on (TER-017).
[[nodiscard]] std::optional<TriangleHit> IntersectTriangle(const Vec3& _origin, const Vec3& _direction, const Vec3& _a, const Vec3& _b,
                                                           const Vec3& _c, double _range) noexcept;

} // namespace Neuron
