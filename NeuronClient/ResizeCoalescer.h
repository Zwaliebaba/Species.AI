#pragma once

#include "WindowGeometry.h"

#include <optional>

namespace Neuron
{

/// The size bookkeeping of PLT-062 and PLT-063, apart from the window so that it is tested without one. The window
/// procedure reports every WM_SIZE; the pump takes at most one resize per call, with the latest client size, and none
/// while the window is minimised. A size that ends a pump where the last resize left it raises nothing.
class ResizeCoalescer
{
public:
  enum class Transition : std::uint8_t
  {
    None,
    Minimized,
    Restored
  };

  explicit ResizeCoalescer(SizePx _clientSize) noexcept
    : m_delivered(_clientSize),
      m_latest(_clientSize)
  {
  }

  // One WM_SIZE: whether the window is now minimised and its client size. Returns the minimise or restore that the
  // message makes, which the window raises in order with its other events.
  Transition OnSize(bool _minimized, SizePx _clientSize) noexcept;

  // The end of a pump: the client size to raise as the one resize of this pump, if there is one.
  [[nodiscard]] std::optional<SizePx> TakeResize() noexcept;

  [[nodiscard]] bool IsMinimized() const noexcept
  {
    return m_minimized;
  }
  [[nodiscard]] SizePx ClientSize() const noexcept
  {
    return m_latest;
  }

private:
  SizePx m_delivered;
  SizePx m_latest;
  bool m_minimized = false;
};

} // namespace Neuron
