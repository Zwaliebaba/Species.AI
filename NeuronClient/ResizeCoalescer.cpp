#include "pch.h"

#include "ResizeCoalescer.h"

namespace Neuron
{

ResizeCoalescer::Transition ResizeCoalescer::OnSize(bool _minimized, SizePx _clientSize) noexcept
{
  if (_minimized)
  {
    const bool wasMinimized = m_minimized;
    m_minimized = true;
    return wasMinimized ? Transition::None : Transition::Minimized;
  }
  const bool wasMinimized = m_minimized;
  m_minimized = false;
  m_latest = _clientSize;
  return wasMinimized ? Transition::Restored : Transition::None;
}

std::optional<SizePx> ResizeCoalescer::TakeResize() noexcept
{
  if (m_minimized || m_latest == m_delivered)
  {
    return std::nullopt;
  }
  m_delivered = m_latest;
  return m_latest;
}

} // namespace Neuron
