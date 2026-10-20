#include "pch.h"

#include "Geometry.h"

#include <cmath>

namespace Neuron
{

std::optional<TriangleHit> IntersectTriangle(const Vec3& _origin, const Vec3& _direction, const Vec3& _a, const Vec3& _b, const Vec3& _c,
                                             double _range) noexcept
{
  // The Möller–Trumbore form: the determinant is zero exactly when the ray lies parallel to the triangle's plane.
  const Vec3 edge1 = _b - _a;
  const Vec3 edge2 = _c - _a;
  const Vec3 p = Cross(_direction, edge2);
  const double determinant = Dot(edge1, p);
  if (determinant == 0.0 || !std::isfinite(determinant))
  {
    return std::nullopt;
  }
  const double inverse = 1.0 / determinant;
  const Vec3 s = _origin - _a;
  const double u = Dot(s, p) * inverse;
  if (!(u >= 0.0 && u <= 1.0))
  {
    return std::nullopt;
  }
  const Vec3 q = Cross(s, edge1);
  const double v = Dot(_direction, q) * inverse;
  if (!(v >= 0.0 && u + v <= 1.0))
  {
    return std::nullopt;
  }
  const double t = Dot(edge2, q) * inverse;
  const double lengthSquared = t * t + u * u + v * v;
  if (!std::isfinite(lengthSquared) || lengthSquared > _range * _range)
  {
    return std::nullopt;
  }
  return TriangleHit{t, u, v};
}

} // namespace Neuron
