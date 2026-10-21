#pragma once

#include <cstdint>

namespace Neuron
{

// RND-220: the DepthBias of a bias factor k, round((k − 1) × _unitScale), with _unitScale rnd.depth_bias.unit_scale (2²⁴
// for D32_FLOAT); the slope and the clamp stay 0. Under reversed Z a positive bias draws nearer the camera (RND-219).
[[nodiscard]] constexpr std::int32_t DepthBiasOf(double _factor, double _unitScale) noexcept
{
  const double bias = (_factor - 1.0) * _unitScale;
  return static_cast<std::int32_t>(bias >= 0.0 ? bias + 0.5 : bias - 0.5);
}

// RND-219, ADR-123: the `bias_laser` variant, of rnd.depth_bias.laser (1.2). The catalogue is created before the client
// reads its records, so the variant is compiled in, and a test holds it to the record's factor.
inline constexpr std::int32_t DEPTH_BIAS_LASER = DepthBiasOf(1.2, 16'777'216.0);

// VFX-014, ADR-125: the `bias_flag_pointer` variant, vfx.cloth.pointer_constant_bias (5) depth-bias units and
// vfx.cloth.pointer_slope_bias (1.0) towards the camera; compiled in for the same reason, and held to the record.
inline constexpr std::int32_t DEPTH_BIAS_FLAG_POINTER = 5;
inline constexpr float SLOPE_DEPTH_BIAS_FLAG_POINTER = 1.0f;

} // namespace Neuron
