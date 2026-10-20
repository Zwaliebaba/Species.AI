#include "pch.h"

#include "WindowGeometry.h"

#include <algorithm>

namespace Neuron
{

namespace
{

// Moves [_start, _start + _length) inside [_low, _high) where it fits, and onto _low where it does not.
std::int32_t ShiftInside(std::int32_t _start, std::int32_t _length, std::int32_t _low, std::int32_t _high) noexcept
{
  return std::max(_low, std::min(_start, _high - _length));
}

} // namespace

float ScaleForDpi(std::uint32_t _dpi) noexcept
{
  return static_cast<float>(_dpi) / static_cast<float>(DPI_BASE);
}

RectPx WindowRectFromClient(const RectPx& _client, const FrameInsets& _frame) noexcept
{
  return {_client.left - _frame.left, _client.top - _frame.top, _client.right + _frame.right, _client.bottom + _frame.bottom};
}

RectPx ClientRectFromWindow(const RectPx& _window, const FrameInsets& _frame) noexcept
{
  return {_window.left + _frame.left, _window.top + _frame.top, _window.right - _frame.right, _window.bottom - _frame.bottom};
}

RectPx WindowedClientRect(SizePx _clientSize, const RectPx& _monitorBounds, const RectPx& _workArea, const FrameInsets& _frame) noexcept
{
  const std::int32_t frameWidth = _frame.left + _frame.right;
  const std::int32_t frameHeight = _frame.top + _frame.bottom;
  const std::int32_t fitWidth = static_cast<std::int32_t>(_workArea.Width()) - frameWidth;
  const std::int32_t fitHeight = static_cast<std::int32_t>(_workArea.Height()) - frameHeight;
  const std::int32_t width =
    std::max(static_cast<std::int32_t>(MIN_RENDER_WIDTH_PIXELS), std::min(static_cast<std::int32_t>(_clientSize.width), fitWidth));
  const std::int32_t height =
    std::max(static_cast<std::int32_t>(MIN_RENDER_HEIGHT_PIXELS), std::min(static_cast<std::int32_t>(_clientSize.height), fitHeight));

  const std::int32_t windowWidth = width + frameWidth;
  const std::int32_t windowHeight = height + frameHeight;
  const std::int32_t centeredLeft = _monitorBounds.left + (static_cast<std::int32_t>(_monitorBounds.Width()) - windowWidth) / 2;
  const std::int32_t centeredTop = _monitorBounds.top + (static_cast<std::int32_t>(_monitorBounds.Height()) - windowHeight) / 2;
  const std::int32_t left = ShiftInside(centeredLeft, windowWidth, _workArea.left, _workArea.right);
  const std::int32_t top = ShiftInside(centeredTop, windowHeight, _workArea.top, _workArea.bottom);
  return ClientRectFromWindow({left, top, left + windowWidth, top + windowHeight}, _frame);
}

} // namespace Neuron
