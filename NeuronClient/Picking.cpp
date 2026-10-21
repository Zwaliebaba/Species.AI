#include "pch.h"

#include "Picking.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace Neuron
{

namespace
{

using Matrix = std::array<std::array<double, 4>, 4>;

Matrix ToDouble(const Float4x4& _matrix) noexcept
{
  Matrix result{};
  for (std::size_t r = 0; r < 4; ++r)
  {
    for (std::size_t c = 0; c < 4; ++c)
    {
      result[r][c] = static_cast<double>(_matrix.m[r][c]);
    }
  }
  return result;
}

// Gauss–Jordan elimination with partial pivoting, in binary64; a singular matrix gives the identity.
Matrix Inverse(Matrix _matrix) noexcept
{
  Matrix inverse{};
  for (std::size_t i = 0; i < 4; ++i)
  {
    inverse[i][i] = 1.0;
  }
  for (std::size_t column = 0; column < 4; ++column)
  {
    std::size_t pivot = column;
    for (std::size_t row = column + 1; row < 4; ++row)
    {
      if (std::abs(_matrix[row][column]) > std::abs(_matrix[pivot][column]))
      {
        pivot = row;
      }
    }
    if (_matrix[pivot][column] == 0.0)
    {
      Matrix identity{};
      for (std::size_t i = 0; i < 4; ++i)
      {
        identity[i][i] = 1.0;
      }
      return identity;
    }
    std::swap(_matrix[pivot], _matrix[column]);
    std::swap(inverse[pivot], inverse[column]);
    const double scale = 1.0 / _matrix[column][column];
    for (std::size_t c = 0; c < 4; ++c)
    {
      _matrix[column][c] *= scale;
      inverse[column][c] *= scale;
    }
    for (std::size_t row = 0; row < 4; ++row)
    {
      if (row == column)
      {
        continue;
      }
      const double factor = _matrix[row][column];
      for (std::size_t c = 0; c < 4; ++c)
      {
        _matrix[row][c] -= factor * _matrix[column][c];
        inverse[row][c] -= factor * inverse[column][c];
      }
    }
  }
  return inverse;
}

// A row vector times the matrix (ADR-016).
std::array<double, 4> Transform(const std::array<double, 4>& _row, const Matrix& _matrix) noexcept
{
  std::array<double, 4> result{};
  for (std::size_t c = 0; c < 4; ++c)
  {
    result[c] = _row[0] * _matrix[0][c] + _row[1] * _matrix[1][c] + _row[2] * _matrix[2][c] + _row[3] * _matrix[3][c];
  }
  return result;
}

Vec3 Unproject(double _x, double _y, double _depth, const Matrix& _inverse) noexcept
{
  const std::array<double, 4> point = Transform({_x, _y, _depth, 1.0}, _inverse);
  return {point[0] / point[3], point[1] / point[3], point[2] / point[3]};
}

} // namespace

Ray PickRayAt(double _x, double _y, const FrameView& _frame, SizePx _size, double _rowOffsetPixels) noexcept
{
  const double width = std::max(1.0, static_cast<double>(_size.width));
  const double height = std::max(1.0, static_cast<double>(_size.height));
  const double ndcX = 2.0 * _x / width - 1.0;
  const double ndcY = 1.0 - 2.0 * (_y + _rowOffsetPixels) / height;
  const Matrix inverse = Inverse(ToDouble(_frame.viewProjection));
  const Vec3 nearPoint = Unproject(ndcX, ndcY, 1.0, inverse); // reversed Z: the near plane is depth 1
  const Vec3 farPoint = Unproject(ndcX, ndcY, 0.0, inverse);
  return {_frame.camera + nearPoint, NormalizedOr(farPoint - nearPoint, {0.0, 0.0, -1.0})};
}

std::optional<ScreenPoint> ProjectToScreen(const Vec3& _world, const FrameView& _frame, SizePx _size) noexcept
{
  const Vec3 relative = _world - _frame.camera; // WLD-246: the difference first, in binary64
  const std::array<double, 4> clip = Transform({relative.x, relative.y, relative.z, 1.0}, ToDouble(_frame.viewProjection));
  if (clip[3] <= 0.0)
  {
    return std::nullopt;
  }
  const double ndcX = clip[0] / clip[3];
  const double ndcY = clip[1] / clip[3];
  return ScreenPoint{(ndcX + 1.0) * 0.5 * static_cast<double>(_size.width), (1.0 - ndcY) * 0.5 * static_cast<double>(_size.height),
                     clip[3]};
}

std::optional<double> RaySphere(const Ray& _ray, const Vec3& _center, double _radius) noexcept
{
  const Vec3 offset = _ray.origin - _center;
  const double b = Dot(offset, _ray.direction);
  const double c = Dot(offset, offset) - _radius * _radius;
  const double discriminant = b * b - c;
  if (discriminant < 0.0)
  {
    return std::nullopt;
  }
  const double root = std::sqrt(discriminant);
  const double nearT = -b - root;
  const double farT = -b + root;
  if (farT < 0.0)
  {
    return std::nullopt;
  }
  return nearT >= 0.0 ? nearT : farT;
}

} // namespace Neuron
