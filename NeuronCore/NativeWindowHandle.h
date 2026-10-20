#pragma once

namespace Neuron
{

// The window's handle, without <windows.h> (ADR-075). HWND__ is the SDK's own name for the type (R4), so only the code
// that holds a window includes this header: the client's window and the platform's pre-UI messages, both of which see
// <windows.h> first. The headless server never includes it (ADR-100).
using NativeWindowHandle = struct HWND__*;

// The window that owns pre-UI messages while it exists (PLT-039); nullptr when there is none.
void SetPlatformMessageOwner(NativeWindowHandle _window) noexcept;
[[nodiscard]] NativeWindowHandle PlatformMessageOwner() noexcept;

} // namespace Neuron
