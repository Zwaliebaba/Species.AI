#include "pch.h"

#include "PresentPolicy.h"

namespace Neuron
{

PresentPolicy SelectPresentPolicy(PresentMode _mode, bool _tearingSupported, WindowMode _windowMode) noexcept
{
  if (_mode == PresentMode::VerticalSync)
  {
    return {1, false};
  }
  return {0, _tearingSupported && _windowMode == WindowMode::BorderlessFullScreen};
}

std::uint32_t MaximumFrameLatency(bool _lowLatency) noexcept
{
  return _lowLatency ? 1 : 2;
}

} // namespace Neuron
