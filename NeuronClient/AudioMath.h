#pragma once

#include "Vec3.h"

#include <array>

namespace Neuron
{

// AEN-214: the amplitude of a level in decibels, as XAudio2DecibelsToAmplitudeRatio gives it; 0 at or below _floorDb.
[[nodiscard]] double DecibelsToAmplitude(double _decibels, double _floorDb) noexcept;

// AEN-201, AEN-202 (ADR-027, DEC-566): the distance gain of a positioned sound _distance wu from the listener: 1 within
// _minDistance, _minDistance / _distance beyond it, faded linearly to 0 over the last _fadeFraction of _maxDistance.
[[nodiscard]] double DistanceGain(double _distance, double _minDistance, double _maxDistance, double _fadeFraction) noexcept;

// AEN-204 (ADR-027): the constant-power pan of a positioned sound on a stereo output, the left and right levels, from the
// emitter's direction relative to the listener, whose right is the cross product of its front and up (a right-handed,
// +Y-up frame). An emitter at the listener, or a listener with no right, is centered.
[[nodiscard]] std::array<float, 2> StereoPan(const Vec3& _emitter, const Vec3& _listener, const Vec3& _front, const Vec3& _up) noexcept;

// AEN-204: the centered pan of a two-dimensional mono sound, cos(pi / 4) on both channels.
[[nodiscard]] std::array<float, 2> CenteredPan() noexcept;

// AEN-215: the gain of a stage in decibels for a volume step from 0 to _volumeMax, _stepDb per step below the top, never
// below _floorDb.
[[nodiscard]] double VolumeStepDb(unsigned _step, unsigned _volumeMax, double _stepDb, double _floorDb) noexcept;

} // namespace Neuron
