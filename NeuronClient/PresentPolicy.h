#pragma once

#include "AppWindow.h"

#include <cstdint>

namespace Neuron
{

// ADR-012: the CPU records at most two frames ahead of the GPU, with one fence value per frame in flight.
inline constexpr std::uint32_t FRAMES_IN_FLIGHT = 2;

// The present-mode setting (RND-442, rnd.option.present_mode.default: vertical sync).
enum class PresentMode : std::uint8_t
{
  VerticalSync,
  Uncapped
};

// How a frame is presented (ADR-012).
struct PresentPolicy
{
  std::uint32_t syncInterval = 1;
  bool allowTearing = false;

  bool operator==(const PresentPolicy& _other) const = default;
};

// ADR-012: vertical sync is Present(1, 0); uncapped is Present(0, DXGI_PRESENT_ALLOW_TEARING) in a borderless full-screen
// window when the swap chain supports tearing, and Present(0, 0) otherwise.
[[nodiscard]] PresentPolicy SelectPresentPolicy(PresentMode _mode, bool _tearingSupported, WindowMode _windowMode) noexcept;

// ADR-012: at most two frames queued, or one with the low-latency option (rnd.option.low_latency.default: off).
[[nodiscard]] std::uint32_t MaximumFrameLatency(bool _lowLatency) noexcept;

} // namespace Neuron
