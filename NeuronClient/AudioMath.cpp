#include "pch.h"

#include "AudioMath.h"
#include "Geometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Neuron
{

double DecibelsToAmplitude(double _decibels, double _floorDb) noexcept
{
  return _decibels <= _floorDb ? 0.0 : std::pow(10.0, _decibels / 20.0);
}

double DistanceGain(double _distance, double _minDistance, double _maxDistance, double _fadeFraction) noexcept
{
  if (_distance >= _maxDistance)
  {
    return 0.0;
  }
  const double gain = _distance <= _minDistance ? 1.0 : _minDistance / _distance;
  const double fadeStart = _maxDistance * (1.0 - _fadeFraction);
  if (_fadeFraction > 0.0 && _distance > fadeStart)
  {
    return gain * (_maxDistance - _distance) / (_maxDistance - fadeStart);
  }
  return gain;
}

std::array<float, 2> StereoPan(const Vec3& _emitter, const Vec3& _listener, const Vec3& _front, const Vec3& _up) noexcept
{
  const Vec3 right = Cross(_front, _up);
  const Vec3 toward = _emitter - _listener;
  if (LengthSquared(right) < NORMALIZE_MIN_LENGTH_SQUARED || LengthSquared(toward) < NORMALIZE_MIN_LENGTH_SQUARED)
  {
    return CenteredPan();
  }
  // x = u · r, from -1 (left) to 1 (right); p = (x + 1) / 2; L = cos(p π / 2), R = sin(p π / 2).
  const double across = std::clamp(Dot(NormalizedOr(toward, Vec3{}), NormalizedOr(right, Vec3{})), -1.0, 1.0);
  const double place = (across + 1.0) * 0.5;
  return {static_cast<float>(std::cos(place * std::numbers::pi * 0.5)), static_cast<float>(std::sin(place * std::numbers::pi * 0.5))};
}

std::array<float, 2> CenteredPan() noexcept
{
  const auto level = static_cast<float>(std::numbers::sqrt2 * 0.5);
  return {level, level};
}

double VolumeStepDb(unsigned _step, unsigned _volumeMax, double _stepDb, double _floorDb) noexcept
{
  const unsigned step = std::min(_step, _volumeMax);
  return std::max(-_stepDb * static_cast<double>(_volumeMax - step), _floorDb);
}

} // namespace Neuron
