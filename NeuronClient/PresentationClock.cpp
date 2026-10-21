#include "pch.h"

#include "PresentationClock.h"

#include <algorithm>
#include <chrono>

namespace Neuron
{

void PresentationClock::Advance(Nanoseconds _realNow, bool _held, double _gameSpeed) noexcept
{
  if (!m_started)
  {
    m_started = true;
    m_start = _realNow;
    m_previous = _realNow;
  }
  const double real = std::chrono::duration<double>(_realNow - m_previous).count();
  m_previous = _realNow;
  m_real = std::chrono::duration<double>(_realNow - m_start).count();
  const double step = _held ? 0.0 : std::clamp(real, 0.0, m_deltaClampSeconds);
  m_unscaledStep = step;
  m_scaledStep = step * std::max(_gameSpeed, 0.0);
  m_unscaled += m_unscaledStep;
  m_scaled += m_scaledStep;
}

} // namespace Neuron
