#pragma once

#include <cmath>

namespace Neuron
{

/// A 3-vector of the simulation and of the terrain, in world units or world units per second (R-SIM-107). Its
/// components are binary64 (ADR-081 item 1), so a sum is the same sum in every build of one compiler; nothing here
/// contracts a multiply and an add, because no project sets /fp:contract (ADR-081 item 2).
struct Vec3
{
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  friend constexpr Vec3 operator+(const Vec3& _a, const Vec3& _b) noexcept
  {
    return {_a.x + _b.x, _a.y + _b.y, _a.z + _b.z};
  }
  friend constexpr Vec3 operator-(const Vec3& _a, const Vec3& _b) noexcept
  {
    return {_a.x - _b.x, _a.y - _b.y, _a.z - _b.z};
  }
  friend constexpr Vec3 operator*(const Vec3& _a, double _scale) noexcept
  {
    return {_a.x * _scale, _a.y * _scale, _a.z * _scale};
  }
  friend constexpr bool operator==(const Vec3& _a, const Vec3& _b) noexcept = default;
};

[[nodiscard]] constexpr double Dot(const Vec3& _a, const Vec3& _b) noexcept
{
  return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z;
}

[[nodiscard]] constexpr double LengthSquared(const Vec3& _a) noexcept
{
  return Dot(_a, _a);
}

[[nodiscard]] inline double Length(const Vec3& _a) noexcept
{
  return std::sqrt(LengthSquared(_a));
}

// The vector scaled to length 1, or _fallback when its squared length is 0, so no division by zero ever reaches
// simulation state (DEC-241, DEC-653).
[[nodiscard]] inline Vec3 NormalizedOr(const Vec3& _a, const Vec3& _fallback) noexcept
{
  const double squared = LengthSquared(_a);
  if (squared <= 0.0)
  {
    return _fallback;
  }
  return _a * (1.0 / std::sqrt(squared));
}

} // namespace Neuron
