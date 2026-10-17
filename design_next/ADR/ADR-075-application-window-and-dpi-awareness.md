---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Application window: a platform-owned Win32 window, per-monitor DPI awareness v2, windowed and borderless full-screen modes

## Context and Problem Statement

The Species client shows one window in which the renderer presents through a flip-model swap chain and the UI lays out its screens. Players use monitors of different sizes and DPI scales, move windows between them, switch between windowed and full-screen play, and resize freely. A change of window size, mode or monitor must never tear down the renderer or reload content. The headless server has no window.

Who owns the window, how does it handle DPI, and which window modes exist?

## Decision Drivers

* Sharp rendering and correct UI scale on every monitor, including mixed-DPI set-ups.
* Size, mode and monitor changes without renderer re-creation.
* Window modes that suit the flip-model presentation of Direct3D 12, which has no exclusive full-screen mode.
* A clear split: the platform layer owns the window and its messages; the renderer owns the swap chain; the input layer owns input messages.

## Considered Options

* A platform-owned Win32 window with per-monitor DPI awareness v2
* A windowing library as the window owner
* A system-DPI-aware or DPI-unaware window

## Decision Outcome

Chosen option: "A platform-owned Win32 window with per-monitor DPI awareness v2".

* **Ownership:** the platform layer registers the window class and creates the window with the wide-character Win32 API, runs the message loop on the main thread, and forwards input messages to the input layer ([ADR-028](ADR-028-input-device-back-ends.md)) and size, DPI and focus events to the renderer and UI ([ADR-033](ADR-033-ui-layout-scaling-and-dpi.md)).
* **Application manifest:** per-monitor DPI awareness v2; `<supportedOS>` entries for Windows 10 and 11 ([ADR-050](ADR-050-client-platform-and-minimum-os.md)); the UTF-8 process code page ([ADR-063](ADR-063-localisation.md)).
* **DPI changes:** on `WM_DPICHANGED` the window takes the suggested rectangle and the UI receives the new scale; the UI scale combines the monitor DPI with the player's text-scale setting ([ADR-064](ADR-064-accessibility-baseline-xag.md)).
* **Window modes:** windowed and borderless full screen (a borderless window covering one monitor). Borderless full screen is the default. Display-mode changes are not used; a lower render resolution is achieved by render-resolution scaling.
* **Resizing:** size, mode and monitor changes reach the renderer as a resize event; the renderer resizes its swap-chain buffers and size-dependent targets without re-creating the device or reloading content (presentation details in [ADR-012](ADR-012-swap-chain-presentation-and-frame-pacing.md)).
* **Limits:** a restored window larger than the current monitor's work area is clamped to it; the last mode, monitor and windowed rectangle are restored at start-up when the monitor still exists.

### Consequences

* Good, because text and UI are sharp at every DPI and stay correct when the window moves between monitors.
* Good, because mode switches are fast and keep all GPU resources.
* Bad, because the platform layer owns window code that a library would otherwise provide.

### Confirmation

* A test moves the window between a 100 % and a 200 % scaled monitor and checks the window rectangle, the UI scale and that no device re-creation occurred.
* A test toggles windowed and borderless full screen repeatedly and checks that no content reload or device re-creation is logged.

## Pros and Cons of the Options

### Platform-owned Win32 window, per-monitor DPI v2

* Good, because it gives full control over DPI messages, modes and the message loop.
* Neutral, because the window code is small and Windows-only by nature; the server has no window.

### A windowing library

* Good, because it hides window code.
* Bad, because IME, DPI and raw input handling still need Win32-specific code, and Species targets only Windows for the client (vision §7).

### System-DPI-aware or DPI-unaware window

* Bad, because Windows bitmap-scales the window on other monitors, which blurs text and the glow effects.

## More Information

* Facts and sources: technology baseline items 1.1 and 4.6 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the operating-system version page and the flip-model and swap-chain pages on learn.microsoft.com.
* Component: platform layer (`PLT`).
