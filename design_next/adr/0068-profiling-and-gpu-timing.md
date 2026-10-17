---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Profiling: PIX markers through one Species macro layer, GPU timestamp queries and an in-game overlay

## Context and Problem Statement

Species renders and simulates crowds of hundreds of figures; frame time and simulation update time are the budgets that decide crowd size, tick cost and minimum hardware. The team needs CPU and GPU timing at the granularity of frame-graph passes and engine subsystems, viewable in an external capture tool and live in the game, from many threads at once, without the measurement itself stalling the GPU. Measurement must be available in every development build and must cost nothing in shipping builds unless deliberately kept.

How does Species measure CPU and GPU performance?

## Decision Drivers

* Thread-safe scoped CPU markers with negligible cost.
* GPU timing without CPU–GPU synchronisation.
* One instrumentation layer feeding the external tool, the overlay and telemetry ([ADR-0066](0066-telemetry-with-consent.md)).
* Zero cost in shipping builds by default.

## Considered Options

* PIX markers and GPU timestamp queries behind a Species macro layer, plus an in-game overlay
* An own CPU profiler with in-game display only
* A third-party frame profiler

## Decision Outcome

Chosen option: "PIX markers and GPU timestamp queries behind a Species macro layer, plus an in-game overlay".

* **Tool:** PIX on Windows **2603.25** or later for GPU captures and timing captures; it supports every Agility SDK 1.619 feature.
* **Markers:** one Species macro layer (`SPECIES_PROFILE_SCOPE(name)` and its GPU counterpart on a command list) emits PIX events through **WinPixEventRuntime 1.0.240308001** (MIT licence), linked as a DLL in `Debug`, `Development` and `Profile` builds. In `Shipping` builds the macros expand to nothing and `WinPixEventRuntime.dll` is not distributed ([ADR-0056](0056-editions-build-flavours-and-feature-flags.md)).
* **Coverage:** every frame-graph pass ([ADR-0004](0004-frame-graph-from-the-ordered-pass-table.md)), every engine subsystem update and every content-loading stage carries a marker; the task groups of the threading model carry markers once the design phase defines that model.
* **GPU timing:** timestamp queries around every frame-graph pass, resolved into a readback buffer and read one or more frames later, so the CPU never waits for the GPU to measure it.
* **Overlay:** in `Development` and `Profile` builds, an in-game overlay shows the frame-time graph, per-pass GPU times, per-subsystem CPU times against their budgets, and entity counts per type.
* **System-wide traces:** ETW traces (with the TraceLogging provider of [ADR-0067](0067-logging.md)) are captured with Windows Performance Analyzer when a problem is outside the game process.
* **Validation:** the Direct3D 12 debug layer and GPU-based validation run in a CI test configuration ([ADR-0070](0070-continuous-integration.md)); `D3D12SDKLayers.dll` is never distributed.

### Consequences

* Good, because one instrumentation set serves PIX, the overlay and telemetry.
* Good, because shipping builds carry no profiling code by default.
* Bad, because field captures from shipping builds have no markers unless the design phase decides to keep them.

### Confirmation

* A CI check confirms that `Shipping` binaries import no PIX runtime and contain no marker strings.
* A performance test measures the marker overhead in `Profile` builds against the diagnostics component's budget.
* A test checks that GPU timestamps are read without a CPU wait (no fence wait on the current frame).

## Pros and Cons of the Options

### PIX markers, GPU timestamps and an overlay

* Good, because PIX is the vendor tool for Direct3D 12 and understands Agility SDK features.
* Neutral, because the macro layer must be kept thin so it adds no cost of its own.

### An own CPU profiler with in-game display only

* Bad, because GPU work and driver behaviour would be invisible, and captures could not be inspected offline.

### A third-party frame profiler

* Good, because it offers a ready-made viewer.
* Bad, because it adds a second marker system next to PIX and a further dependency to license and pin.

## More Information

* Facts and sources: technology baseline item 4.7 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the PIX download page, the WinPixEventRuntime NuGet package and its GitHub repository.
* Whether PIX markers stay in shipping builds for field captures is a design-phase choice; the default is compiled out.
* Component: diagnostics, test and performance (`DBG`).
