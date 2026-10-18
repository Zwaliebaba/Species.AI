---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Swap chain, presentation and frame pacing

## Context and Problem Statement

Species runs in a window on Windows 11, either windowed or covering the whole display. Players expect a working vertical-sync option, an uncapped mode that uses variable-refresh-rate displays, a frame-rate cap, low input latency for camera and cursor, and instant switching between window sizes without the renderer being torn down. The game's pace must never depend on how often frames are presented: the simulation advances in ticks, and presentation interpolates between them (the reference main-loop timing in `reference/12_simulation_model/`). Display brightness must be adjustable without touching the system's display settings.

The problem is how frames reach the display: the swap chain, the present modes, the window modes, how far the CPU may run ahead of the GPU, and how frame pacing is controlled.

## Decision Drivers

- Lowest latency and power use for a windowed or borderless game on Windows 11.
- A vertical-sync option that always works, and an uncapped option that uses variable refresh rate where available.
- Frame pacing set by the engine, independent of the simulation tick.
- Resize and mode changes without recreating the device or the renderer.
- Brightness adjustable in the game, with no system-wide side effects.
- Fullscreen exclusive mode does not exist on Direct3D 12.

## Considered Options

1. A **flip-model swap chain** (`FLIP_DISCARD`), windowed and borderless full screen only, with a frame-latency waitable object, tearing support for variable refresh rate, and an engine-side frame-rate cap.
2. A flip-model swap chain, presenting with **vertical sync always on** and no tearing support.

## Decision Outcome

Chosen option: **1, flip model with waitable latency, tearing support and an engine frame cap**, because it gives the lowest latency and power use, supports variable refresh rate, and is the presentation model Direct3D 12 is designed for.

- **Swap chain.** `DXGI_SWAP_EFFECT_FLIP_DISCARD`, **3** back buffers, format `R8G8B8A8_UNORM` for SDR output, created with `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` and, when `IDXGIFactory5::CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING)` reports support, `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` (this flag cannot be added later by `ResizeBuffers`). The back buffer holds sRGB-encoded values written without conversion, as the working colour space requires (ADR-013). HDR output changes the format and colour space as ADR-014 describes.
- **No multisampled back buffer.** Flip-model swap chains cannot be multisampled; anti-aliasing happens in the scene targets and is resolved before the final pass (ADR-015).
- **Latency.** The maximum frame latency is **2** by default, and **1** when the player chooses the low-latency option. The renderer waits on the waitable object before it starts recording each frame. The CPU never runs more than two frames ahead of the GPU, enforced by a fence per frame in flight (two frames in flight).
- **Present modes** (a player option):
  - *vertical sync*: `Present(1, 0)`;
  - *uncapped*: `Present(0, DXGI_PRESENT_ALLOW_TEARING)` in a borderless full-screen window when tearing is supported, which drives variable-refresh-rate displays; `Present(0, 0)` otherwise.
- **Frame-rate cap.** An optional cap (player option) is enforced by the engine with a high-resolution waitable timer before frame start; it never relies on the display's refresh rate.
- **Window modes.** Windowed and borderless full screen only; borderless full screen is the default. The window itself (creation, styles, DPI awareness, monitor handling) belongs to the platform layer and its ADR (ADR-075).
- **Resolution.** The display mode is never changed. The swap chain always matches the window's client area. A **render-resolution scale** setting sizes the scene targets below or at the window size; the UI is drawn at the window's native resolution. The one exception is an overlay that the pass table places among the scene passes to keep its overdraw order (the in-match message, which debris and particles draw over); it is drawn at render resolution, which is identical at a render scale of 1.
- **Resize.** On a size or mode change, the renderer waits for the GPU, releases its back-buffer references, calls `ResizeBuffers` and recompiles the frame graph (ADR-004). The device, the asset resources and the pipeline-state catalogue survive.
- **Occlusion and minimise.** When the window is minimised, or `Present` returns `DXGI_STATUS_OCCLUDED`, the renderer stops rendering and polls at a low rate until the window is visible again; the simulation is unaffected.
- **Brightness.** A brightness setting is applied as a constant in the final pass; the system gamma ramp is never changed.
- **Frame pacing.** The renderer presents as fast as the present mode and cap allow. Each frame renders the presentation state at the current presentation time, interpolated between simulation ticks as `reference/12_simulation_model/` specifies; the number of frames presented never changes the simulation's pace.

### Consequences

- Good, because flip model with a waitable object gives the lowest presentation latency available to a windowed game.
- Good, because variable-refresh-rate displays work in uncapped mode, and vertical sync always works when chosen.
- Good, because resizing and changing render scale never recreate the device.
- Good, because brightness changes affect only the game.
- Bad, because there is no exclusive full-screen mode; players who want a different display resolution use the render-resolution scale instead.
- Bad, because tearing support must be detected at swap-chain creation, so a change of adapter or output may need a new swap chain.

### Confirmation

- A presentation test switches between windowed, borderless, every present mode and the frame cap during a running match, and checks for no device recreation and no validation messages.
- A latency test measures input-to-photon latency with maximum frame latency 1 and 2 on the minimum-capability GPU.
- A pacing test runs the same replayed match with the cap at 30, 60 and 144 frames per second and checks that the simulation's tick count over the same wall-clock period is identical.
- A unit test checks the present-mode selection for every combination of setting, tearing support and window mode.

## Pros and Cons of the Options

### 1. Flip model with waitable latency, tearing support and an engine cap

- Good, because it combines low latency, variable refresh rate and player choice.
- Bad, because it needs feature detection and three present paths.

### 2. Flip model with vertical sync always on

- Good, because it is the simplest correct presentation.
- Bad, because variable-refresh-rate displays and uncapped play are not supported, and latency is higher.

## More Information

- Technology baseline item 4.6 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model · https://learn.microsoft.com/windows/win32/direct3d12/swap-chains · https://learn.microsoft.com/windows/win32/direct3ddxgi/variable-refresh-rate-displays · https://learn.microsoft.com/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag.
- Related: ADR-004 (frame graph recompilation), ADR-008 (frames in flight), ADR-013 (working colour space), ADR-014 (HDR output), ADR-015 (anti-aliasing), ADR-075 (application window and DPI awareness).
- **Amendment 2026-10-17.** The native-resolution rule for the UI has one exception, the match message drawn inside the scene passes (RND-072). The renderer's setting defaults (vertical sync, brightness 1, the display's maximum as HDR highlight limit) are those of RND-442.
- **Amendment 2026-10-18.** The considered options are the two flip-model swap chains only: Direct3D 12 supports no other swap effect, and no swap chain of it is multisampled, so anti-aliasing is resolved from the scene targets (ADR-015). Source, checked 2026-10-18: "The only supported swap effects are DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD" (https://learn.microsoft.com/windows/win32/direct3d12/swap-chains).
