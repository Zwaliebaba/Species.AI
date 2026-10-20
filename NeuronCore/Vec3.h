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

// DEC-241 item 6: a 3-vector normalises when its squared length is above this, and a length is set when its magnitude
// is above this.
inline constexpr double NORMALIZE_MIN_LENGTH_SQUARED = 1e-10;
inline constexpr double SET_LENGTH_MIN_MAGNITUDE = 1e-6;

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

// DEC-241 item 6: the vector scaled to length 1, or (0, 0, 1) when its squared length is not above 10⁻¹⁰.
[[nodiscard]] inline Vec3 Normalized(const Vec3& _a) noexcept
{
  const double squared = LengthSquared(_a);
  if (squared <= NORMALIZE_MIN_LENGTH_SQUARED)
  {
    return {0.0, 0.0, 1.0};
  }
  return _a * (1.0 / std::sqrt(squared));
}

// DEC-241 item 6: the vector scaled to _length, or (_length, 0, 0) when its magnitude is within 10⁻⁶ of 0, so a push
// at a structure's base point still moves (R-SIM-382).
[[nodiscard]] inline Vec3 WithLength(const Vec3& _a, double _length) noexcept
{
  const double magnitude = Length(_a);
  if (magnitude <= SET_LENGTH_MIN_MAGNITUDE)
  {
    return {_length, 0.0, 0.0};
  }
  return _a * (_length / magnitude);
}

// The distance in the horizontal plane (x and z).
[[nodiscard]] inline double HorizontalDistance(const Vec3& _a, const Vec3& _b) noexcept
{
  const double dx = _a.x - _b.x;
  const double dz = _a.z - _b.z;
  return std::sqrt(dx * dx + dz * dz);
}

} // namespace Neuron
