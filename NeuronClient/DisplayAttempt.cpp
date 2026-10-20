#include "pch.h"

#include "DisplayAttempt.h"

namespace Neuron
{

std::array<DisplayAttempt, 3> StartupDisplayAttempts(WindowMode _storedMode, SizePx _storedRenderSize, SizePx _desktopSize) noexcept
{
  const SizePx preferred = _storedRenderSize.width == 0 || _storedRenderSize.height == 0 ? _desktopSize : _storedRenderSize;
  return {DisplayAttempt{_storedMode, preferred}, DisplayAttempt{_storedMode, _desktopSize},
          DisplayAttempt{WindowMode::Windowed, FALLBACK_WINDOW_SIZE}};
}

DisplayOutcome RunDisplayAttempts(std::span<const DisplayAttempt> _attempts, const std::function<bool(const DisplayAttempt&)>& _attempt)
{
  DisplayOutcome outcome;
  for (std::size_t index = 0; index < _attempts.size(); ++index)
  {
    if (_attempt(_attempts[index]))
    {
      outcome.succeeded = index;
      return outcome;
    }
    if (index == 0)
    {
      outcome.resolutionRejected = true;
    }
  }
  return outcome;
}

} // namespace Neuron
