#include "pch.h"

#include <ShellScalingApi.h>

#include "AppWindow.h"
#include "FatalPath.h"
#include "PlatformLogSink.h"
#include "ResizeCoalescer.h"
#include "TextEncoding.h"

#include <algorithm>
#include <array>
#include <bit>
#include <mutex>
#include <utility>

#pragma comment(lib, "shcore.lib")

namespace Neuron
{

namespace
{

constexpr wchar_t WINDOW_CLASS[] = L"Neuron.AppWindow";
constexpr DWORD WINDOWED_STYLE = WS_OVERLAPPEDWINDOW;
constexpr DWORD BORDERLESS_STYLE = WS_POPUP;
constexpr DWORD EXTENDED_STYLE = 0;

DWORD StyleOf(WindowMode _mode) noexcept
{
  return _mode == WindowMode::Windowed ? WINDOWED_STYLE : BORDERLESS_STYLE;
}

RectPx ToRect(const RECT& _rect) noexcept
{
  return {static_cast<std::int32_t>(_rect.left), static_cast<std::int32_t>(_rect.top), static_cast<std::int32_t>(_rect.right),
          static_cast<std::int32_t>(_rect.bottom)};
}

FrameInsets InsetsFor(DWORD _style, UINT _dpi) noexcept
{
  RECT rect{0, 0, 0, 0};
  if (AdjustWindowRectExForDpi(&rect, _style, FALSE, EXTENDED_STYLE, _dpi) == FALSE)
  {
    return {};
  }
  return {static_cast<std::int32_t>(-rect.left), static_cast<std::int32_t>(-rect.top), static_cast<std::int32_t>(rect.right),
          static_cast<std::int32_t>(rect.bottom)};
}

MonitorInfo Describe(HMONITOR _monitor)
{
  MonitorInfo info;
  MONITORINFOEXW monitor{};
  monitor.cbSize = sizeof(monitor);
  if (GetMonitorInfoW(_monitor, &monitor) == FALSE)
  {
    return info;
  }
  info.deviceName = ToUtf8(monitor.szDevice);
  info.bounds = ToRect(monitor.rcMonitor);
  info.workArea = ToRect(monitor.rcWork);
  info.primary = (monitor.dwFlags & MONITORINFOF_PRIMARY) != 0;
  UINT dpiX = DPI_BASE;
  UINT dpiY = DPI_BASE;
  if (SUCCEEDED(GetDpiForMonitor(_monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
  {
    info.dpi = dpiX;
  }
  info.dpiScale = ScaleForDpi(info.dpi);
  DEVMODEW mode{};
  mode.dmSize = sizeof(mode);
  if (EnumDisplaySettingsExW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0) != FALSE)
  {
    info.desktop = {mode.dmPelsWidth, mode.dmPelsHeight, mode.dmDisplayFrequency};
  }
  else
  {
    info.desktop = {info.bounds.Width(), info.bounds.Height(), 0};
  }
  return info;
}

const MonitorInfo* FindMonitor(const std::vector<MonitorInfo>& _monitors, std::string_view _deviceName) noexcept
{
  const MonitorInfo* primary = nullptr;
  for (const MonitorInfo& monitor : _monitors)
  {
    if (!_deviceName.empty() && monitor.deviceName == _deviceName)
    {
      return &monitor;
    }
    if (monitor.primary && primary == nullptr)
    {
      primary = &monitor;
    }
  }
  return primary != nullptr ? primary : (_monitors.empty() ? nullptr : &_monitors.front());
}

std::string LayoutTag(HKL _layout)
{
  const LANGID language = LOWORD(reinterpret_cast<std::uintptr_t>(_layout));
  std::array<wchar_t, LOCALE_NAME_MAX_LENGTH> name{};
  if (LCIDToLocaleName(MAKELCID(language, SORT_DEFAULT), name.data(), static_cast<int>(name.size()), 0) == 0)
  {
    return {};
  }
  return ToUtf8(name.data());
}

// Win32 passes pointers and handles in the integer parameters of messages and window data: the bits are the pointer.
template <class T> T FromInteger(LONG_PTR _value) noexcept
{
  return std::bit_cast<T>(_value);
}

bool IsForwardedInput(UINT _message) noexcept
{
  return (_message >= WM_KEYFIRST && _message <= WM_KEYLAST) || (_message >= WM_MOUSEFIRST && _message <= WM_MOUSELAST) ||
         _message == WM_INPUT || (_message >= WM_IME_STARTCOMPOSITION && _message <= WM_IME_KEYLAST) ||
         (_message >= WM_IME_SETCONTEXT && _message <= WM_IME_KEYUP);
}

bool IsButtonDown(UINT _message) noexcept
{
  return _message == WM_LBUTTONDOWN || _message == WM_RBUTTONDOWN || _message == WM_MBUTTONDOWN || _message == WM_XBUTTONDOWN;
}

bool IsButtonUp(UINT _message) noexcept
{
  return _message == WM_LBUTTONUP || _message == WM_RBUTTONUP || _message == WM_MBUTTONUP || _message == WM_XBUTTONUP;
}

LRESULT CALLBACK WindowProcedure(HWND _window, UINT _message, WPARAM _wParam, LPARAM _lParam);

void RegisterWindowClass()
{
  static std::once_flag g_registered;
  std::call_once(g_registered,
                 []
                 {
                   WNDCLASSEXW windowClass{};
                   windowClass.cbSize = sizeof(windowClass);
                   windowClass.lpfnWndProc = &WindowProcedure;
                   windowClass.hInstance = GetModuleHandleW(nullptr);
                   windowClass.lpszClassName = WINDOW_CLASS;
                   // No background brush: the renderer fills the client area. No class pointer: PLT-068 hides it over the client area.
                   if (RegisterClassExW(&windowClass) == 0)
                   {
                     PlatformLog(PlatformLogLevel::Error, "the window class was not registered: error {}", GetLastError());
                   }
                 });
}

} // namespace

/// The window's state, which the window procedure reaches through the window's user data. It lives on the heap so that
/// moving an AppWindow leaves the procedure's pointer valid.
struct WindowState
{
  const PlatformClock* clock = nullptr;
  HWND window = nullptr;
  HMONITOR monitorHandle = nullptr;
  WindowMode mode = WindowMode::BorderlessFullScreen;
  MonitorInfo monitor;
  SizePx renderSize;
  std::optional<RectPx> windowedClientRect;
  ResizeCoalescer sizes{SizePx{}};
  bool hasFocus = false;
  std::string keyboardLayoutTag;
  WindowInputSink* sink = nullptr;
  std::vector<std::pair<WindowListenerId, std::function<void(const WindowEvent&)>>> listeners;
  WindowListenerId nextListenerId = 1;
  std::vector<WindowEvent> pending;
  std::optional<Nanoseconds> retrievedAt;

  LRESULT Handle(UINT _message, WPARAM _wParam, LPARAM _lParam);
  void Forward(UINT _message, WPARAM _wParam, LPARAM _lParam);
  void UpdateFocus();
  void UpdateMonitor();
  void FollowDisplayChange();
  void RememberWindowedRect();
  void Deliver(const std::vector<WindowEvent>& _events);
  [[nodiscard]] RectPx WindowRectFor(WindowMode _mode) const noexcept;
};

namespace
{

LRESULT CALLBACK WindowProcedure(HWND _window, UINT _message, WPARAM _wParam, LPARAM _lParam)
{
  if (_message == WM_NCCREATE)
  {
    const auto* create = FromInteger<const CREATESTRUCTW*>(_lParam);
    SetWindowLongPtrW(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
  }
  auto* state = FromInteger<WindowState*>(GetWindowLongPtrW(_window, GWLP_USERDATA));
  if (state == nullptr)
  {
    return DefWindowProcW(_window, _message, _wParam, _lParam);
  }
  if (_message == WM_NCDESTROY)
  {
    SetWindowLongPtrW(_window, GWLP_USERDATA, 0);
    return DefWindowProcW(_window, _message, _wParam, _lParam);
  }
  // An exception must not unwind through the frames of user32 (PLT-118): it is a defect, and it ends here.
  try
  {
    return state->Handle(_message, _wParam, _lParam);
  }
  catch (...)
  {
    EnterFatalPath("an exception escaped the window procedure");
  }
}

} // namespace

LRESULT WindowState::Handle(UINT _message, WPARAM _wParam, LPARAM _lParam)
{
  switch (_message)
  {
  case WM_ACTIVATEAPP:
  case WM_ACTIVATE:
  case WM_SETFOCUS:
  case WM_KILLFOCUS:
  {
    const LRESULT result = DefWindowProcW(window, _message, _wParam, _lParam);
    UpdateFocus();
    return result;
  }
  case WM_SIZE:
  {
    const SizePx client{LOWORD(_lParam), HIWORD(_lParam)};
    switch (sizes.OnSize(_wParam == SIZE_MINIMIZED, client))
    {
    case ResizeCoalescer::Transition::Minimized:
      pending.emplace_back(Minimized{});
      break;
    case ResizeCoalescer::Transition::Restored:
      pending.emplace_back(Restored{});
      break;
    case ResizeCoalescer::Transition::None:
      break;
    }
    return 0;
  }
  case WM_DPICHANGED:
  {
    const auto* suggested = FromInteger<const RECT*>(_lParam);
    const UINT dpi = HIWORD(_wParam);
    monitor.dpi = dpi;
    monitor.dpiScale = ScaleForDpi(dpi);
    SetWindowPos(window, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    pending.emplace_back(DpiChanged{monitor.dpiScale, ToRect(*suggested)});
    return 0;
  }
  case WM_GETMINMAXINFO:
    if (mode == WindowMode::Windowed)
    {
      const FrameInsets frame = InsetsFor(WINDOWED_STYLE, GetDpiForWindow(window));
      auto* limits = FromInteger<MINMAXINFO*>(_lParam);
      limits->ptMinTrackSize.x = static_cast<LONG>(MIN_RENDER_WIDTH_PIXELS) + frame.left + frame.right;
      limits->ptMinTrackSize.y = static_cast<LONG>(MIN_RENDER_HEIGHT_PIXELS) + frame.top + frame.bottom;
      return 0;
    }
    break;
  case WM_MOVE:
    UpdateMonitor();
    RememberWindowedRect();
    return 0;
  case WM_WINDOWPOSCHANGED:
    UpdateMonitor();
    RememberWindowedRect();
    break; // the default procedure sends WM_SIZE and WM_MOVE
  case WM_SETCURSOR:
    if (LOWORD(_lParam) == HTCLIENT)
    {
      SetCursor(nullptr); // PLT-068 item 1: the game draws its own cursor over the client area
      return TRUE;
    }
    break;
  case WM_SYSCOMMAND:
    if ((_wParam & 0xFFF0) == SC_KEYMENU)
    {
      return 0; // PLT-065: Alt and F10 never open the window menu
    }
    break;
  case WM_CLOSE:
    Forward(_message, _wParam, _lParam); // PLT-009: the close request becomes action.app.quit; the window stays
    return 0;
  case WM_QUERYENDSESSION:
    return TRUE;
  case WM_ENDSESSION:
    if (_wParam != FALSE)
    {
      // PLT-042: no further frame runs, so the quit is requested now rather than at the next pump.
      std::vector<WindowEvent> events = std::exchange(pending, {});
      events.emplace_back(EndOfSession{});
      Deliver(events);
    }
    return 0;
  case WM_INPUTLANGCHANGE:
    keyboardLayoutTag = LayoutTag(FromInteger<HKL>(_lParam));
    PlatformLog(PlatformLogLevel::Info, "the keyboard layout changed to \"{}\"", keyboardLayoutTag);
    pending.emplace_back(KeyboardLayoutChanged{keyboardLayoutTag});
    break;
  case WM_DISPLAYCHANGE:
    FollowDisplayChange();
    pending.emplace_back(DisplayConfigurationChanged{});
    break;
  case WM_CANCELMODE:
    ReleaseCapture();
    break;
  case WM_ERASEBKGND:
    return 1; // the renderer fills the client area
  default:
    if (IsForwardedInput(_message))
    {
      // PLT-068 item 2: a press captures the pointer, so the release of the last held button arrives outside too.
      if (IsButtonDown(_message) && GetCapture() != window)
      {
        SetCapture(window);
      }
      else if (IsButtonUp(_message) && (_wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0)
      {
        ReleaseCapture();
      }
      Forward(_message, _wParam, _lParam);
    }
    break;
  }
  return DefWindowProcW(window, _message, _wParam, _lParam);
}

void WindowState::Forward(UINT _message, WPARAM _wParam, LPARAM _lParam)
{
  if (sink == nullptr)
  {
    return;
  }
  const Nanoseconds time = retrievedAt.has_value() ? *retrievedAt : clock->Now();
  sink->OnWindowMessage(WindowMessage{_message, static_cast<std::uintptr_t>(_wParam), static_cast<std::intptr_t>(_lParam), time});
}

void WindowState::UpdateFocus()
{
  const bool focus = GetForegroundWindow() == window;
  if (focus != hasFocus)
  {
    hasFocus = focus;
    pending.emplace_back(FocusChanged{focus});
  }
}

void WindowState::UpdateMonitor()
{
  // Every move message of a drag arrives here, so the monitor is described again only when it is another one.
  HMONITOR current = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
  if (current != monitorHandle)
  {
    monitorHandle = current;
    MonitorInfo described = Describe(current);
    const bool changed = described.deviceName != monitor.deviceName;
    monitor = std::move(described);
    if (changed)
    {
      pending.emplace_back(MonitorChanged{monitor.deviceName});
    }
  }
  monitor.dpi = GetDpiForWindow(window);
  monitor.dpiScale = ScaleForDpi(monitor.dpi);
}

void WindowState::FollowDisplayChange()
{
  // PLT-072: a window whose monitor is gone moves to the primary monitor in its mode; a borderless window keeps
  // covering its monitor's bounds, which a change of desktop resolution moves.
  const std::vector<MonitorInfo> monitors = EnumerateMonitors();
  const MonitorInfo* target = FindMonitor(monitors, monitor.deviceName);
  if (target == nullptr)
  {
    return;
  }
  const bool moved = target->deviceName != monitor.deviceName;
  monitor = *target;
  monitorHandle = nullptr; // described again at the next move
  if (mode == WindowMode::BorderlessFullScreen || moved)
  {
    if (moved)
    {
      windowedClientRect.reset();
    }
    const RectPx rect = WindowRectFor(mode);
    SetWindowPos(window, nullptr, rect.left, rect.top, static_cast<int>(rect.Width()), static_cast<int>(rect.Height()),
                 SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

void WindowState::RememberWindowedRect()
{
  if (mode != WindowMode::Windowed || IsIconic(window) != FALSE || IsZoomed(window) != FALSE)
  {
    return;
  }
  RECT client{};
  if (GetClientRect(window, &client) == FALSE)
  {
    return;
  }
  POINT corners[2] = {{client.left, client.top}, {client.right, client.bottom}};
  MapWindowPoints(window, nullptr, corners, 2);
  windowedClientRect = RectPx{corners[0].x, corners[0].y, corners[1].x, corners[1].y};
}

RectPx WindowState::WindowRectFor(WindowMode _mode) const noexcept
{
  if (_mode == WindowMode::BorderlessFullScreen)
  {
    return monitor.bounds;
  }
  const FrameInsets frame = InsetsFor(WINDOWED_STYLE, monitor.dpi);
  const RectPx client =
    windowedClientRect.has_value() ? *windowedClientRect : WindowedClientRect(renderSize, monitor.bounds, monitor.workArea, frame);
  return WindowRectFromClient(client, frame);
}

void WindowState::Deliver(const std::vector<WindowEvent>& _events)
{
  for (const WindowEvent& event : _events)
  {
    // A listener may add or remove listeners; it runs on a copy of the list.
    const auto current = listeners;
    for (const auto& entry : current)
    {
      entry.second(event);
    }
  }
}

std::vector<MonitorInfo> EnumerateMonitors()
{
  std::vector<HMONITOR> handles;
  const auto collect = [](HMONITOR _monitor, HDC, LPRECT, LPARAM _context) -> BOOL
  {
    auto* list = FromInteger<std::vector<HMONITOR>*>(_context);
    try
    {
      list->push_back(_monitor);
    }
    catch (...)
    {
      return FALSE;
    }
    return TRUE;
  };
  EnumDisplayMonitors(nullptr, nullptr, collect, reinterpret_cast<LPARAM>(&handles));
  std::vector<MonitorInfo> monitors;
  monitors.reserve(handles.size());
  for (HMONITOR handle : handles)
  {
    monitors.push_back(Describe(handle));
  }
  return monitors;
}

std::expected<AppWindow, PlatformError> AppWindow::Create(const Desc& _desc)
{
  RegisterWindowClass();
  auto state = std::make_unique<WindowState>();
  state->clock = _desc.clock;
  state->mode = _desc.placement.mode;
  state->renderSize = _desc.renderSize;
  state->windowedClientRect = _desc.placement.windowedClientRect;

  const std::vector<MonitorInfo> monitors = EnumerateMonitors();
  const MonitorInfo* monitor = FindMonitor(monitors, _desc.placement.monitorDeviceName);
  if (monitor == nullptr)
  {
    PlatformLog(PlatformLogLevel::Error, "no monitor is attached to the desktop");
    return std::unexpected(PlatformError::Unsupported);
  }
  state->monitor = *monitor;
  // ADR-075: a stored rectangle that no longer fits the work area is clamped to it.
  if (state->windowedClientRect.has_value())
  {
    const FrameInsets frame = InsetsFor(WINDOWED_STYLE, state->monitor.dpi);
    state->windowedClientRect =
      WindowedClientRect(state->windowedClientRect->Size(), state->monitor.bounds, state->monitor.workArea, frame);
  }

  const RectPx rect = state->WindowRectFor(state->mode);
  const std::wstring title = ToUtf16(_desc.title);
  HWND window =
    CreateWindowExW(EXTENDED_STYLE, WINDOW_CLASS, title.c_str(), StyleOf(state->mode), rect.left, rect.top, static_cast<int>(rect.Width()),
                    static_cast<int>(rect.Height()), nullptr, nullptr, GetModuleHandleW(nullptr), state.get());
  if (window == nullptr)
  {
    const DWORD error = GetLastError();
    PlatformLog(PlatformLogLevel::Error, "the window was not created: error {}", error);
    return std::unexpected(error == ERROR_ACCESS_DENIED ? PlatformError::AccessDenied : PlatformError::Other);
  }
  state->window = window;
  RECT client{};
  GetClientRect(window, &client);
  state->sizes = ResizeCoalescer(ToRect(client).Size());
  state->pending.clear();
  state->UpdateMonitor();
  state->pending.clear();
  state->keyboardLayoutTag = LayoutTag(GetKeyboardLayout(0));
  SetPlatformMessageOwner(window);
  PlatformLog(PlatformLogLevel::Info, "window created on {} at {} dpi, client {}x{}, {}", state->monitor.deviceName, state->monitor.dpi,
              state->sizes.ClientSize().width, state->sizes.ClientSize().height,
              state->mode == WindowMode::Windowed ? "windowed" : "borderless full screen");
  return AppWindow(std::move(state));
}

AppWindow::AppWindow(std::unique_ptr<WindowState> _state) noexcept
  : m_state(std::move(_state))
{
}

AppWindow::AppWindow(AppWindow&& _other) noexcept = default;

AppWindow& AppWindow::operator=(AppWindow&& _other) noexcept
{
  if (this != &_other)
  {
    AppWindow discarded(std::move(*this));
    m_state = std::move(_other.m_state);
  }
  return *this;
}

AppWindow::~AppWindow()
{
  if (m_state == nullptr || m_state->window == nullptr)
  {
    return;
  }
  if (PlatformMessageOwner() == m_state->window)
  {
    SetPlatformMessageOwner(nullptr);
  }
  DestroyWindow(m_state->window);
}

NativeWindowHandle AppWindow::Native() const noexcept
{
  return m_state->window;
}

SizePx AppWindow::ClientSize() const noexcept
{
  return m_state->sizes.ClientSize();
}

float AppWindow::DpiScale() const noexcept
{
  return m_state->monitor.dpiScale;
}

bool AppWindow::HasFocus() const noexcept
{
  return m_state->hasFocus;
}

bool AppWindow::IsMinimized() const noexcept
{
  return m_state->sizes.IsMinimized();
}

WindowMode AppWindow::Mode() const noexcept
{
  return m_state->mode;
}

const MonitorInfo& AppWindow::Monitor() const noexcept
{
  return m_state->monitor;
}

WindowPlacement AppWindow::Placement() const
{
  return {m_state->mode, m_state->monitor.deviceName, m_state->windowedClientRect};
}

std::string AppWindow::ActiveKeyboardLayoutTag() const
{
  return m_state->keyboardLayoutTag;
}

void AppWindow::Show()
{
  ShowWindow(m_state->window, SW_SHOW);
  SetForegroundWindow(m_state->window);
  m_state->UpdateFocus();
}

void AppWindow::SetMode(WindowMode _mode, [[maybe_unused]] Permanence _permanence)
{
  WindowState& state = *m_state;
  if (_mode == state.mode)
  {
    return;
  }
  if (IsZoomed(state.window) != FALSE)
  {
    ShowWindow(state.window, SW_RESTORE);
  }
  state.RememberWindowedRect();
  state.mode = _mode;
  const bool visible = IsWindowVisible(state.window) != FALSE;
  SetWindowLongPtrW(state.window, GWL_STYLE, static_cast<LONG_PTR>(StyleOf(_mode) | (visible ? WS_VISIBLE : 0)));
  const RectPx rect = state.WindowRectFor(_mode);
  SetWindowPos(state.window, nullptr, rect.left, rect.top, static_cast<int>(rect.Width()), static_cast<int>(rect.Height()),
               SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
  state.pending.emplace_back(ModeChanged{_mode});
  PlatformLog(PlatformLogLevel::Info, "window mode changed to {}", _mode == WindowMode::Windowed ? "windowed" : "borderless full screen");
}

void AppWindow::PumpMessages()
{
  WindowState& state = *m_state;
  MSG message{};
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE)
  {
    state.retrievedAt = state.clock->Now();
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  state.retrievedAt.reset();
  if (const std::optional<SizePx> size = state.sizes.TakeResize())
  {
    state.pending.emplace_back(Resized{*size});
  }
  state.Deliver(std::exchange(state.pending, {}));
}

void AppWindow::SetInputSink(WindowInputSink* _sink) noexcept
{
  m_state->sink = _sink;
}

WindowListenerId AppWindow::AddListener(std::function<void(const WindowEvent&)> _listener)
{
  const WindowListenerId id = m_state->nextListenerId++;
  m_state->listeners.emplace_back(id, std::move(_listener));
  return id;
}

void AppWindow::RemoveListener(WindowListenerId _id)
{
  std::erase_if(m_state->listeners, [_id](const auto& _entry) { return _entry.first == _id; });
}

} // namespace Neuron
