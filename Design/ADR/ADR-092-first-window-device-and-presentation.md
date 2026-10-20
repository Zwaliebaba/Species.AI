---
status: accepted
date: 2026-10-20
decision-makers: Species technology lead (M0 task T4)
---

# First window, device and presentation: the choices M0 makes before settings, input and the frame graph

## Context and Problem Statement

M0 task T4 builds the window of ADR-075 and the renderer's device, swap chain, frame pacing and device removal of ADR-001, ADR-002, ADR-008 and ADR-012, with a frame cleared to black. The components it builds on come later:
- the settings store (PLT-038 step 10) and the locations (step 5);
- the string table (step 12) and the input router (step 15);
- the development commands of the developer tools;
- the frame graph (ADR-004) and the bindless descriptor heap (ADR-006).

The specifications therefore leave choices open that the code cannot leave open:
- how the debug layer is switched on and where its messages go;
- which adapters the probe may choose;
- how long each wait of the renderer may last;
- how much of the device rebuild and of the GPU resource factory exists;
- where the log file goes, and which text a pre-UI message shows;
- how the client exports the Agility SDK constants under names that break AGENTS.md §1.

Owner decision O65 settles the four questions that belong to the owner: the window mode and Alt+Enter, the manifest, the removal key and the pointer. This record holds the engineering choices.

## Decision Drivers

* Build exactly what the specifications say, where they say it, and give every stand-in an owner and an end.
* Keep the device, the queue and the swap chain in one owner that a rebuild can release and create again as a whole (ADR-008).
* No wait without a bound (PLT-043, DEC-221), and no wait on the main thread that Windows would record as a hang (PLT-067).
* Unit tests for every decision that needs no device: the adapter choice, the present policy, the fallback chain, the resize bookkeeping, the window geometry and the factory's rules.
* No Direct3D header outside NeuronClient (ADR-078), and no game name in the engine (`AGENTS.md` R9).

## Considered Options

* 1 — a first form of every part T4 lists, with each missing input replaced by a recorded stand-in
* 2 — the device and the swap chain only, with a removed device ending the process, and the rest when its inputs exist
* 3 — no renderer until the settings store, the string table and the frame graph exist

## Decision Outcome

Chosen option: "1". It is the only one that gives the owner, at M0, a window that resizes, moves between monitors, minimises, closes, and survives a device removal, which T4's done-when asks for. Option 2 leaves ADR-008 untested until M1, and option 3 reverses the plan's order.

* **Debug layer.** Debug builds always enable the Direct3D 12 debug layer, from the `D3D12SDKLayers.dll` that the build copies beside the runtime (BLD-042), and the DXGI debug factory when the Graphics Tools are installed. When either is missing, the renderer logs a warning and goes on. GPU-based validation stays off until the development setting of ADR-001 exists, because it multiplies the cost of every frame. Release builds enable neither.
* **Debug messages.** The renderer registers an `ID3D12InfoQueue1` callback that writes each message to the log in the Render category:
  - corruption and error as an error, a warning as a warning, and the rest as debug records;
  - with a debugger attached, the queue also breaks on corruption and on error;
  - nothing is filtered, so "the debug layer is silent" means the log holds no such record.
* **DRED** is set to forced-on auto-breadcrumbs, breadcrumb contexts and page faults in every build, before the device is created (ADR-008).
* **Adapters.** The probe skips software adapters (WARP and the basic render driver), which never meet the frame budget, and logs every adapter it examines with what it lacks. With no hardware adapter, the message names the missing capability as "a hardware adapter". The shader-model query starts at `D3D_HIGHEST_SHADER_MODEL` of the pinned header.
* **Window association.** `MakeWindowAssociation` with `DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES`, because exclusive full screen does not exist (ADR-012) and the window belongs to the platform layer (ADR-075).
* **Bounds of the waits.** These are chosen values, not measurements; they are engineering constants of the renderer and the frame loop, not tuning:
  - a fence or the frame-latency object that has not signalled within 1000 ms is checked for a removed device: a removal starts the rebuild, and otherwise the wait is logged and continues;
  - teardown waits at most 2000 ms for the GPU and then releases regardless;
  - while the window is minimised or occluded, the frame loop polls every 100 ms (RND-032's "low rate"), keeps pumping messages, and checks occlusion with `Present(0, DXGI_PRESENT_TEST)`.
* **The frame.** Until the frame graph exists, a frame is one command list that moves the back buffer to the render-target state, clears it to black, moves it back to present, and presents. Its markers are the catalogue's `render.frame`, `render.clear` and `render.present` (DBG-045). The clear uses resource-state barriers, the fallback path of ADR-007, which every adapter has; the enhanced path comes with the frame graph's executor.
* **Device removal.** The rebuild of ADR-008 and RND-034 releases every device object and creates them again: the device through the same probe, the fence, the allocators, the command list, the render-target heap and the swap chain. It then raises `DeviceRebuilt`. A removal while the rebuild runs, or a recreation that fails, enters the fatal path, so it ends with a report. In Debug builds, Ctrl+Shift+F11 calls `ID3D12Device5::RemoveDevice` (O65). The key is F11 because F12 is the system's debugger key.
* **GPU resource factory.** T4 delivers its first form:
  - the interface of ADR-078 and its description checks;
  - an in-memory implementation that keeps the rules of RND-009 and RND-035 and records every call, for the factory's clients' tests (RND T11).

  The implementation on the device needs the persistent region of the bindless heap (ADR-006) and the copy queue. It arrives with the first textured draw, which is where ADR-078 is confirmed. Until then, `FrameRenderer` has no `Resources()`.
* **Fallback chain.** At start-up each attempt of PLT-112 makes a new window and a new renderer, because a failed attempt in M0 leaves nothing worth keeping. A failure other than the swap chain's, such as no capable adapter, ends the start-up at once. A failed first attempt is logged as `ui.error.resolution_rejected` until the UI can show the notice. Nothing is written back until the settings store exists.
* **Logs folder and texts.**
  - Until the locations of PLT-015 and PLT-016 exist, the log file goes to `logs\` beside the executable.
  - Until the string table loads, the composition root compiles in the base-language text of every pre-UI message M0 can show. For `ui.error.gpu_unsupported`, `ui.error.display_mode` and `ui.error.resolution_rejected`, which table 6.4-A does not mark as compiled-in, this is interim.
  - The texts belong to the game, so the engine takes them, the dialog title and the run id from the root (`PlatformMessageContext`).
* **Pre-UI messages.**
  - The task dialog of PLT-039 is looked up in version 6 of the common controls at run time; the application manifest selects that version.
  - A process without it, such as a test host, falls back to a message box.
  - A run without an interactive desktop writes the log only.
  - FailFast shows `ui.error.internal` with the run id once a window owns the messages (PLT-127 step 2).
* **Agility SDK exports.** `Species/AgilitySdkExports.cpp` defines `AGILITY_SDK_VERSION` and `AGILITY_SDK_PATH`, named by `AGENTS.md` §1. The linker exports them as `D3D12SDKVersion` and `D3D12SDKPath` (`/EXPORT:<name>=<symbol>,DATA`), so the SDK's spelling stays in the export table, where R4 keeps it, and no diagnostic is silenced. `Build/CheckProjectFiles.py` rule 9 checks that the version equals the minor version of every restored `Microsoft.Direct3D.D3D12` package and that the path is `.\D3D12\` (BLD T7). It also checks that the client's linker exports both and the server's neither.
* **Departures from the interface sketches.** The sketches of the platform layer §2.1 and the renderer §2 say they are not implementations. T4 departs from them where M0 needs it:
  - `AppWindow::Create` takes a description with the clock, which stamps forwarded messages (PLT-008), and the title, which the game supplies.
  - The window gains `Mode()` and `RemoveListener`.
  - `FrameRenderer::Create` takes the clock for the frame-rate cap and fails with a `StartupFailure` that carries the adapter and the capability for the message.
  - `RenderFrame()` takes no `FrameInputs` until the frame graph has passes to give them to.
  - `EndStartup` ends a failed start-up through `FatalExit`.
* **Not yet.** These wait for their inputs:
  - the monitors' mode sizes (PLT-070), until the offered render sizes need them (RND-026);
  - raw-mouse registration, with input at M1;
  - the window icon, `uitex.app_icon`, with the resource script that O65 allows;
  - the crash-report attachment of the DRED data, with crash reporting (PLT-119).

### Consequences

* Good, because every part of T4's list runs on the owner's PC at M0, including the removal and rebuild, and the debug layer's messages are in the log for the run checklist.
* Good, because the adapter choice, the present policy, the fallback chain, the resize bookkeeping, the window geometry and the factory's rules are unit-tested on CI, which has no GPU.
* Good, because a rebuild releases and recreates one set of objects, which the frame graph, the catalogue and the device factory join later without a second path.
* Bad, because the in-memory factory is the only factory until the first textured draw, so the device side of RND-009 is untested until then.
* Bad, because the stand-ins (the logs folder, the interim texts, the Alt+Enter and removal keys in the root) must each be removed when their component lands; each is named in the code where it lives.
* Neutral, because the wait bounds are chosen, not measured; the performance job measures the frame (DEC-971) and may move them.
* It forecloses: GPU-based validation by default; a software adapter as a fallback; Direct3D in any project but NeuronClient; the SDK's export names spelled in source.

### Confirmation

* `NeuronClientTests`:
  - `PresentationTests` checks the adapter choice (all pass, each single failure, a software adapter, an unknown shader model), the present-mode selection for every combination, the frame latency, and the fallback chain with each attempt failing in turn.
  - `WindowGeometryTests` checks the windowed placement, the minimum size, the DPI scale, and the resize coalescing of PLT T9.
  - `AppWindowTests` creates the window, windowed and borderless full screen, and checks that it exists and is hidden (PLT-057). A window needs no GPU, so CI runs it (amendment 2026-10-20).
  - `GpuResourceFactoryTests` checks RND T11 on the in-memory factory.
* `Build/CheckProjectFiles.py` fixtures fail on:
  - a `d3d12.h` include outside NeuronClient;
  - a `d3dx12` include;
  - a Direct3D library linked by a pragma outside the client libraries;
  - a manifest in a library;
  - an exported SDK version that is not the package's.
* The owner's run checklist of T4 checks what needs a GPU: a black window that resizes, moves between monitors, minimises, switches mode with Alt+Enter, rebuilds after Ctrl+Shift+F11, and closes cleanly, with no debug-layer error in the log.

## Pros and Cons of the Options

### 1 — a first form with recorded stand-ins

* Good, because the owner can verify ADR-008 and ADR-012 at M0.
* Bad, because each stand-in is code to remove later.

### 2 — device and swap chain only

* Good, because it is less code now.
* Bad, because a driver reset ends the client, against ADR-008, and the rebuild path stays unexercised until it is built around assets.

### 3 — wait for the inputs

* Good, because nothing temporary is written.
* Bad, because the plan's order puts the window and device before the session and the content, and nothing could be run until then.

## More Information

* Related:
  - [ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md), [ADR-002](ADR-002-minimum-gpu-capability.md), [ADR-007](ADR-007-barrier-model.md), [ADR-008](ADR-008-gpu-memory-uploads-and-device-removal.md), [ADR-012](ADR-012-swap-chain-presentation-and-frame-pacing.md);
  - [ADR-075](ADR-075-application-window-and-dpi-awareness.md), [ADR-078](ADR-078-gpu-resource-factory.md), [ADR-090](ADR-090-solution-and-project-layout.md);
  - owner decision O65.
* The code is in `NeuronClient`: `AppWindow`, `FrameRenderer`, `GraphicsDevice`, `AdapterChoice`, `PresentPolicy`, `DisplayAttempt`, `GpuResourceFactory` and `InMemoryGpuResourceFactory`. NeuronCore holds `PlatformMessage` and `RunId`, and `Species/Species.cpp` holds the composition root.
* **Amendment 2026-10-20 (M0 task T4).** The owner's first run of `Species.exe` stopped at the window:
  - every attempt of the fallback chain logged "the window was not created: error 1400", and the start-up ended with `ui.error.display_mode`;
  - the cause: the window procedure stored the window's state at `WM_NCCREATE`, but the state learnt the window's handle only after `CreateWindowExW` returned;
  - the creation messages therefore reached `DefWindowProcW` with a null handle, `WM_NCCREATE` returned `FALSE`, and the creation failed with `ERROR_INVALID_WINDOW_HANDLE`.

  The procedure now gives the state the handle at `WM_NCCREATE`, before any message reaches it. No test had created a window, because the tests of the Confirmation covered only what the window computes. `AppWindowTests` now creates one in each mode, and both of its tests fail without the fix.
* **Accepted 2026-10-20, at the end of M0.** Every item of the Confirmation is met. The tests and the fixtures pass in CI. The owner's run of T4's checklist passed every step, among them the device rebuilt after Ctrl+Shift+F11, the debug layer silent and the Agility SDK's exports in `Release|x64` (`playtests/M0.md`).
