---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Platform clock: one process-relative monotonic clock behind an injectable time source

## Context and Problem Statement

The client's frame loop, animation, audio scheduling, timeouts and profiling all need the current time. Tests, frame capture and replays need time that advances exactly as instructed: a fixed step per frame, or manual steps in a unit test. The simulation itself advances in ticks, so simulation rules never read a clock; how many ticks run per second and how ticks are scheduled is a design-phase decision. Logs and reports also need calendar time, which must never feed any timing logic.

Where does Species get time from, and how can it be replaced?

## Decision Drivers

* Monotonic, high-resolution time, unaffected by calendar or time-zone changes.
* Replaceable by a fake source for tests, capture and replays.
* Portable to a server build on any operating system.
* No path by which clock readings reach simulation state.

## Considered Options

* One process-relative monotonic clock behind an injectable time-source interface
* Direct calls to the OS clock wherever time is needed
* Calendar time

## Decision Outcome

Chosen option: "One process-relative monotonic clock behind an injectable time-source interface".

* **Real source:** `QueryPerformanceCounter` on Windows, wrapped as `std::chrono::steady_clock`-compatible durations; `std::chrono::steady_clock` on any other server platform. Time is reported relative to process start, as a 64-bit integer count of nanoseconds.
* **Injectable sources:** a fixed-step source (advances by a set duration per frame, for capture and replays) and a manual source (advanced explicitly by tests). The platform layer selects the source at start-up; components receive it through the platform interface and never call the OS clock themselves. In development builds the platform layer may replace the source once more during start-up, right after the settings have loaded, when a development setting asks for fixed-step time (command-history recording and playback); a replacement keeps time continuous, so the new source's first reading equals the old source's last reading and time never goes backwards. Tests may also replace the source while the process runs, with the same continuity.
* **Simulation boundary:** simulation code reads tick counts from the tick clock (`TickClock`), never the time source. The time source drives only the scheduling of ticks and presentation.
* **Calendar time:** UTC from the OS, used only for log and report timestamps and for display, never for timing logic.
* **Frame pacing** is explicit and independent of the simulation tick; [ADR-0012](0012-swap-chain-presentation-and-frame-pacing.md) specifies it.

### Consequences

* Good, because every timing-dependent behaviour can be tested deterministically with the manual source.
* Good, because capture and replay runs produce the same frame times on any machine with the fixed-step source.
* Bad, because every component must take time from the platform interface, which a lint rule has to enforce.

### Confirmation

* A lint rule rejects direct calls to `QueryPerformanceCounter`, `GetTickCount64`, `timeGetTime` and `std::chrono` clocks outside the platform layer.
* A test runs the frame loop on the manual source and checks the exact sequence of tick-scheduling decisions.
* A test changes the system time-zone and calendar time during a run and checks that no timing behaviour changes.

## Pros and Cons of the Options

### Injectable monotonic clock

* Good, because one implementation serves real play, tests, capture and replays.
* Neutral, because it is a small interface every component depends on.

### Direct OS clock calls

* Bad, because tests and captures cannot control time, and each call site may pick a different clock.

### Calendar time

* Bad, because it jumps when the user or the network time service changes it.

## More Information

* Component: platform layer (`PLT`).
* Related: [ADR-0068](0068-profiling-and-gpu-timing.md) (profiling markers read the real source directly through the platform layer).
* Tick rate, the per-tick budget and the threading model are design-phase decisions.
* **Amendment 2026-10-17.** The source can be replaced after the settings load in `Debug` and `Development` builds, with continuity, as the platform layer specifies (PLT-049, PLT-051) for the command-history feature of the diagnostics component (DBG-241).
