#pragma once

#include "AppWindow.h"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>

namespace Neuron
{

inline constexpr SizePx FALLBACK_WINDOW_SIZE{800, 600}; // rnd.init.fallback_window_size

// One attempt of the start-up fallback chain (PLT-112, RND-023): a window mode and a render size, which is also the
// client size of a windowed window without a stored rectangle.
struct DisplayAttempt
{
  WindowMode mode = WindowMode::BorderlessFullScreen;
  SizePx renderSize;

  bool operator==(const DisplayAttempt& _other) const = default;
};

// PLT-112: the preferred settings (the stored render size, the desktop size when none is stored), then the desktop
// size of the window's monitor in the stored mode, then windowed at rnd.init.fallback_window_size. The snap to an offered
// render size (RND-022) comes with the offered sizes (RND-026).
[[nodiscard]] std::array<DisplayAttempt, 3> StartupDisplayAttempts(WindowMode _storedMode, SizePx _storedRenderSize,
                                                                   SizePx _desktopSize) noexcept;

struct DisplayOutcome
{
  std::optional<std::size_t> succeeded; // the attempt in use, or none: ui.error.display_mode (FatalExit)
  bool resolutionRejected = false;      // the first attempt failed: the notice ui.error.resolution_rejected
};

// Runs the attempts in order until one succeeds.
[[nodiscard]] DisplayOutcome RunDisplayAttempts(std::span<const DisplayAttempt> _attempts,
                                                const std::function<bool(const DisplayAttempt&)>& _attempt);

} // namespace Neuron
