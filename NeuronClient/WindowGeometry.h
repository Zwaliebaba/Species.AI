#pragma once

#include <cstdint>

namespace Neuron
{

struct PointPx
{
  std::int32_t x = 0;
  std::int32_t y = 0;
};

struct SizePx
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;

  bool operator==(const SizePx& _other) const = default;
};

// Left, top, right, bottom: the Windows rectangle order.
struct RectPx
{
  std::int32_t left = 0;
  std::int32_t top = 0;
  std::int32_t right = 0;
  std::int32_t bottom = 0;

  bool operator==(const RectPx& _other) const = default;

  [[nodiscard]] std::uint32_t Width() const noexcept
  {
    return right > left ? static_cast<std::uint32_t>(right - left) : 0;
  }
  [[nodiscard]] std::uint32_t Height() const noexcept
  {
    return bottom > top ? static_cast<std::uint32_t>(bottom - top) : 0;
  }
  [[nodiscard]] SizePx Size() const noexcept
  {
    return {Width(), Height()};
  }
};

// The thickness of a windowed window's frame and caption on each side of its client area, at the window's DPI
// (AdjustWindowRectExForDpi of an empty client rectangle).
struct FrameInsets
{
  std::int32_t left = 0;
  std::int32_t top = 0;
  std::int32_t right = 0;
  std::int32_t bottom = 0;
};

inline constexpr std::uint32_t MIN_RENDER_WIDTH_PIXELS = 640;  // rnd.init.min_render_width
inline constexpr std::uint32_t MIN_RENDER_HEIGHT_PIXELS = 480; // rnd.init.min_render_height
inline constexpr std::uint32_t DPI_BASE = 96;                  // plt.window.dpi_base: the 100 % scale of Windows

// PLT-060: the window's DPI over plt.window.dpi_base.
[[nodiscard]] float ScaleForDpi(std::uint32_t _dpi) noexcept;

// The client rectangle of a windowed window without a stored rectangle (PLT-059, RND-029): the requested client size,
// at least the minimum render size and at most what fits the work area with the frame, centred on the monitor and then
// moved into the work area. When even the minimum does not fit, the minimum wins.
[[nodiscard]] RectPx WindowedClientRect(SizePx _clientSize, const RectPx& _monitorBounds, const RectPx& _workArea,
                                        const FrameInsets& _frame) noexcept;

// A window rectangle from a client rectangle and back, through the frame insets.
[[nodiscard]] RectPx WindowRectFromClient(const RectPx& _client, const FrameInsets& _frame) noexcept;
[[nodiscard]] RectPx ClientRectFromWindow(const RectPx& _window, const FrameInsets& _frame) noexcept;

} // namespace Neuron
