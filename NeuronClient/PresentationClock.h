#pragma once

#include "PlatformClock.h"

namespace Neuron
{

/// The presentation clock (ADR-036, VFX-009, VFX-016): three readings in seconds from the first frame. The real reading
/// follows the platform clock. The unscaled one advances by each frame's step, clamped to `ref.sim.frame.delta_clamp`, and
/// stops while the world is held; the scaled one advances by that step times the game speed. Frame thread only.
class PresentationClock
{
public:
  explicit PresentationClock(double _deltaClampSeconds) noexcept
    : m_deltaClampSeconds(_deltaClampSeconds)
  {
  }

  // VFX-016: once at the start of each frame. The first call starts every reading at 0.
  void Advance(Nanoseconds _realNow, bool _held, double _gameSpeed) noexcept;

  [[nodiscard]] double RealSeconds() const noexcept
  {
    return m_real;
  }
  [[nodiscard]] double UnscaledSeconds() const noexcept
  {
    return m_unscaled;
  }
  [[nodiscard]] double ScaledSeconds() const noexcept
  {
    return m_scaled;
  }
  // The last frame's steps of the unscaled and the scaled readings.
  [[nodiscard]] double UnscaledStepSeconds() const noexcept
  {
    return m_unscaledStep;
  }
  [[nodiscard]] double ScaledStepSeconds() const noexcept
  {
    return m_scaledStep;
  }

private:
  double m_deltaClampSeconds;
  Nanoseconds m_start{};
  Nanoseconds m_previous{};
  bool m_started = false;
  double m_real = 0.0;
  double m_unscaled = 0.0;
  double m_scaled = 0.0;
  double m_unscaledStep = 0.0;
  double m_scaledStep = 0.0;
};

} // namespace Neuron
