---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Profiling: PIX markers through one Species macro layer, GPU timestamp queries and an in-game overlay

## Context and Problem Statement

Species renders and simulates crowds of hundreds of figures; frame time and simulation update time are the budgets that decide crowd size, tick cost and minimum hardware. The team needs CPU and GPU timing at the granularity of frame-graph passes and engine subsystems, viewable in an external capture tool and live in the game, from many threads at once, without the measurement itself stalling the GPU. Markers must be available in both configurations, Debug and Release ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)), so optimised builds can be captured, and must cost little when no capture is attached.

How does Species measure CPU and GPU performance?

## Decision Drivers

* Thread-safe scoped CPU markers with negligible cost.
* GPU timing without CPU–GPU synchronisation.
* One instrumentation layer feeding the external tool, the overlay and telemetry ([ADR-066](ADR-066-telemetry-with-consent.md)).
* Low, measured cost in Release builds when no capture is attached.
* Only the allow-listed WinPixEventRuntime package (`AGENTS.md` R14).

## Considered Options

* PIX markers and GPU timestamp queries behind an engine macro layer, plus an in-game overlay
* An own CPU profiler with in-game display only
* A third-party frame profiler

## Decision Outcome

Chosen option: "PIX markers and GPU timestamp queries behind an engine macro layer, plus an in-game overlay".

* **Tool:** PIX on Windows **2603.25** or later for GPU captures and timing captures; it supports every Agility SDK 1.619 feature.
* **Markers:** one engine macro layer, `ENGINE_PROFILE_SCOPE(name)` for a CPU scope and `ENGINE_PROFILE_GPU_SCOPE(commandList, name)` for a GPU scope on a command list, emits PIX events through **WinPixEventRuntime 1.0.240308001** (MIT licence), one of the three packages `AGENTS.md` R14 allows. The markers are compiled into Debug and Release alike, and `WinPixEventRuntime.dll` is installed beside the executable ([ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)). No other code calls the PIX runtime directly.
* **Coverage:** every frame-graph pass ([ADR-004](ADR-004-frame-graph-from-the-ordered-pass-table.md)), every engine subsystem update and every content-loading stage carries a marker; the task groups of the threading model carry markers once the design phase defines that model.
* **GPU timing:** timestamp queries around every frame-graph pass, resolved into a readback buffer and read one or more frames later, so the CPU never waits for the GPU to measure it.
* **Overlay:** in Debug builds, the development overlay shows the frame-time graph, per-pass GPU times, per-subsystem CPU times against their budgets, and entity counts per type. Release builds have no overlay ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)); an optimised build is measured with PIX captures and the telemetry data set.
* **System-wide traces:** ETW traces (with the TraceLogging provider of [ADR-067](ADR-067-logging.md)) are captured with Windows Performance Analyzer when a problem is outside the game process.
* **Validation:** the Direct3D 12 debug layer and GPU-based validation run in Debug builds, on developer machines and in a CI job on a GPU agent ([ADR-070](ADR-070-continuous-integration.md)); `D3D12SDKLayers.dll` is never distributed.

### Consequences

* Good, because one instrumentation set serves PIX, the overlay and telemetry.
* Good, because Release builds keep their markers, so PIX captures of the optimised game are fully annotated.
* Bad, because every Release install carries `WinPixEventRuntime.dll` and pays the cost of markers with no capture attached.
* Bad, because the overlay exists only in the unoptimised Debug build.

### Confirmation

* Before a release, the shipping builder confirms that `WinPixEventRuntime.dll` in the install matches the pinned package version.
* A performance test measures the marker overhead in Release builds with no capture attached against the diagnostics component's budget.
* A test checks that GPU timestamps are read without a CPU wait (no fence wait on the current frame).

## Pros and Cons of the Options

### PIX markers, GPU timestamps and an overlay

* Good, because PIX is the vendor tool for Direct3D 12 and understands Agility SDK features.
* Neutral, because the macro layer must be kept thin so it adds no cost of its own.

### An own CPU profiler with in-game display only

* Bad, because GPU work and driver behaviour would be invisible, and captures could not be inspected offline.

### A third-party frame profiler

* Good, because it offers a ready-made viewer.
* Bad, because it adds a second marker system next to PIX, and `AGENTS.md` R14 excludes it.

## More Information

* Facts and sources: technology baseline item 4.7 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the PIX download page, the WinPixEventRuntime NuGet package and its GitHub repository.
* Component: diagnostics, test and performance (`DBG`).
* **Amendment 2026-10-17.** The overlay's performance pages are also present in `Debug` builds (ADR-056, DBG-004).
* **Amendment 2026-10-17 (AGENTS.md, O21).** WinPixEventRuntime is one of the three allow-listed packages. The macro layer is `ENGINE_PROFILE_SCOPE` and `ENGINE_PROFILE_GPU_SCOPE`; markers stay in Debug and Release, so `WinPixEventRuntime.dll` ships beside the executable; the overlay is a Debug-only development feature.
* **Amendment 2026-10-19 (M0 task T3).** `pix3.h` defines `USE_PIX` by itself only in Debug and profile builds, so the markers would be compiled out of Release. `NeuronCore/Profile.cpp`, the one file that includes `pix3.h` (`Build\CheckProjectFiles.py` rule 14 rejects it anywhere else), defines `USE_PIX` before the include in both configurations. It is not one of the switches of BLD-014: it does not differ between Debug and Release. `ENGINE_PROFILE_SCOPE(name)` declares a `ProfileScope` whose out-of-line constructor and destructor call `PIXBeginEvent` and `PIXEndEvent`, so no other file sees `pix3.h` and the `strsafe.h` that it includes. A Debug build's profile tree (DBG-081) and the GPU scope arrive with the overlay and the renderer.
* **Amendment 2026-10-20 (M1 task T10, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** `ProfileScope` also times the inside of its event while marker timing is on (`NeuronCore/MarkerTimings`), which a measurement turns on to read per-object costs in-process; it is off otherwise, and the markers still reach the PIX runtime only through `Profile.cpp`.
