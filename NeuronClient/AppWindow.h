#pragma once

#include "NativeWindowHandle.h"
#include "PlatformClock.h"
#include "PlatformError.h"
#include "PlatformMessage.h"
#include "WindowGeometry.h"

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Neuron
{

enum class WindowMode : std::uint8_t
{
  Windowed,
  BorderlessFullScreen
};

enum class Permanence : std::uint8_t
{
  Permanent,
  Temporary
};

struct DesktopMode
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t refreshHz = 0;
};

// A monitor record (PLT-027); both rectangles are in desktop coordinates.
struct MonitorInfo
{
  std::string deviceName;
  RectPx bounds;
  RectPx workArea;
  std::uint32_t dpi = DPI_BASE;
  float dpiScale = 1.0f;
  bool primary = false;
  DesktopMode desktop;
  std::vector<SizePx> modeSizes; // PLT-070; empty until the offered render sizes use them (RND-026)
};

// Every monitor of the desktop, in the order Windows reports them (PLT-027, PLT-071).
[[nodiscard]] std::vector<MonitorInfo> EnumerateMonitors();

struct WindowPlacement
{
  WindowMode mode = WindowMode::BorderlessFullScreen; // rnd.option.window_mode.default
  std::string monitorDeviceName;                      // empty, or no such monitor: the primary monitor
  std::optional<RectPx> windowedClientRect;
};

// A keyboard, pointer, button, wheel, raw-input, character, input-method or close message, stamped with the platform
// time at which the pump retrieved it (PLT-008, PLT-009).
struct WindowMessage
{
  std::uint32_t id = 0;
  std::uintptr_t wParam = 0;
  std::intptr_t lParam = 0;
  Nanoseconds time{};
};

// The keyboard and mouse back-end of input, which turns a close message into action.app.quit (PLT-009).
class WindowInputSink
{
public:
  virtual ~WindowInputSink() = default;
  virtual void OnWindowMessage(const WindowMessage& _message) = 0; // main thread
};

struct Resized
{
  SizePx client;
};
struct DpiChanged
{
  float dpiScale = 1.0f;
  RectPx suggestedWindowRect;
};
struct FocusChanged
{
  bool hasFocus = false;
};
struct Minimized
{
};
struct Restored
{
};
struct ModeChanged
{
  WindowMode mode = WindowMode::Windowed;
};
struct MonitorChanged
{
  std::string deviceName;
};
struct DisplayConfigurationChanged
{
};
struct KeyboardLayoutChanged
{
  std::string languageTag; // BCP 47
};
struct EndOfSession
{
};
using WindowEvent = std::variant<Resized, DpiChanged, FocusChanged, Minimized, Restored, ModeChanged, MonitorChanged,
                                 DisplayConfigurationChanged, KeyboardLayoutChanged, EndOfSession>;

using WindowListenerId = std::uint32_t;

struct WindowState;

/// The one application window (ADR-075, PLT-057 to PLT-069): a wide-character Win32 window on the main thread, per-monitor
/// DPI aware v2 through the application manifest, windowed or borderless full screen. Every call is made on the main
/// thread. The window raises its events inside PumpMessages, to the listeners in the order they were added; the work
/// that an event causes runs in its listener's own frame step, never in the window procedure (PLT-109).
class AppWindow
{
public:
  struct Desc
  {
    const PlatformClock* clock = nullptr; // stamps the forwarded messages (PLT-008)
    std::string title;                    // the product name, which the game supplies (PLT-057, AGENTS.md R9)
    WindowPlacement placement;
    SizePx renderSize; // a windowed window's client size when no rectangle is stored (PLT-059)
  };

  // Created hidden (PLT-057); Show follows once the renderer has its swap chain (PLT-038 step 13).
  [[nodiscard]] static std::expected<AppWindow, PlatformError> Create(const Desc& _desc);

  AppWindow(AppWindow&& _other) noexcept;
  AppWindow& operator=(AppWindow&& _other) noexcept;
  AppWindow(const AppWindow&) = delete;
  AppWindow& operator=(const AppWindow&) = delete;
  ~AppWindow();

  [[nodiscard]] NativeWindowHandle Native() const noexcept; // swap-chain creation only
  [[nodiscard]] SizePx ClientSize() const noexcept;
  [[nodiscard]] float DpiScale() const noexcept;
  [[nodiscard]] bool HasFocus() const noexcept;
  [[nodiscard]] bool IsMinimized() const noexcept;
  [[nodiscard]] WindowMode Mode() const noexcept;
  [[nodiscard]] const MonitorInfo& Monitor() const noexcept;
  [[nodiscard]] WindowPlacement Placement() const;
  [[nodiscard]] std::string ActiveKeyboardLayoutTag() const; // BCP 47 (PLT-069)

  void Show();
  // The caption text, UTF-8; the game supplies it as it does the first title (PLT-057, AGENTS.md R9).
  void SetTitle(std::string_view _title);
  // PLT-061. A permanent change is meant to be saved with the window-mode setting; until the settings store exists
  // (PLT-038 step 10) both kinds leave nothing stored.
  void SetMode(WindowMode _mode, Permanence _permanence);
  void PumpMessages(); // once per frame; never blocks (PLT-067)
  void SetInputSink(WindowInputSink* _sink) noexcept;

  // A listener of the window's events; it is removed before what it calls is destroyed.
  WindowListenerId AddListener(std::function<void(const WindowEvent&)> _listener);
  void RemoveListener(WindowListenerId _id);

private:
  explicit AppWindow(std::unique_ptr<WindowState> _state) noexcept;

  std::unique_ptr<WindowState> m_state;
};

} // namespace Neuron
