---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Logging: asynchronous, buffered, categorised logging with TraceLogging (ETW) and rotating file sinks

## Context and Problem Statement

Every Species program (client, dedicated server, tools) logs start-up, configuration, content mounting, errors and warnings, and in development builds much more. Logging must never stall a frame or a simulation update: high-frequency logging that opens a file per line, or writes under a global lock, is itself a performance hazard. Logs travel with crash reports ([ADR-065](ADR-065-crash-reporting.md)), so they must not contain personal data or credentials. Developers want live, filterable output and system-wide traces alongside PIX captures. The server may run on a host without ETW.

How does Species log?

## Decision Drivers

* No blocking I/O on the calling thread; bounded memory.
* Categories and levels, filterable at run time.
* Integration with Windows tracing tools (ETW, Windows Performance Analyzer).
* A portable API, so a non-Windows server build needs only a new sink.
* Messages never interpreted as format strings.

## Considered Options

* An asynchronous logger with pluggable sinks: TraceLogging (ETW) and rotating files
* Synchronous file logging
* A third-party logging library

## Decision Outcome

Chosen option: "An asynchronous logger with pluggable sinks: TraceLogging (ETW) and rotating files".

* **API:** `log(category, level, std::format_string, args...)`; format strings are compile-time constants, so text from content, players or the network is always an argument and never a format string. Levels: trace, debug, info, warning, error, fatal. Categories are a fixed enumeration per subsystem (platform, render, audio, input, ui, content, simulation, network, tools), extended only in code.
* **Pipeline:** callers write records into per-thread lock-free buffers; one logging thread drains them to the sinks. A full buffer drops trace and debug records first and counts the drops; warnings and errors are never dropped.
* **Sinks:** a TraceLogging provider (self-describing ETW events, so system-wide traces in Windows Performance Analyzer show log events next to thread and I/O activity, see [ADR-068](ADR-068-profiling-and-gpu-timing.md)); a rotating UTF-8 text file in the logs folder of [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md) (one file per run, the last ten kept); the debugger output in `Debug` and `Development` builds. A non-Windows server build replaces the ETW sink with a host sink that the design phase chooses.
* **Levels per flavour:** `Shipping` and `Profile` record info and above to file and ETW; `Development` and `Debug` record debug and above, with trace enabled per category at run time.
* **Header:** every file starts with the full build identity ([ADR-057](ADR-057-version-identity.md)), OS build and adapter description.
* **Privacy:** logs never contain chat text, account tokens or passwords; identifiers of other players are logged only as session-local numbers.
* **Fatal path:** a fatal record flushes synchronously before the process ends, so the crash report has the last lines.

### Consequences

* Good, because logging cost on the calling thread is a buffer write, independent of the sink.
* Good, because log events appear in system-wide ETW traces.
* Bad, because records can be dropped under extreme load (counted and reported in the log).

### Confirmation

* A performance test logs one million debug records from eight threads and checks the per-call cost and the frame-time impact against the diagnostics component's budget.
* A test passes a string containing format specifiers as an argument and checks it is logged verbatim.
* A crash test checks that the last fatal record is in the registered log file.

## Pros and Cons of the Options

### Asynchronous logger with TraceLogging and file sinks

* Good, because it combines low cost, Windows tooling integration and a portable API.
* Neutral, because ETW consumers need the provider's GUID, which the diagnostics component publishes.

### Synchronous file logging

* Bad, because every call pays file I/O and lock contention on the calling thread.

### A third-party logging library

* Good, because asynchronous sinks exist off the shelf.
* Bad, because ETW integration, the privacy filter and the drop policy would still be custom, and runtime-format-string APIs would need to be banned.

## More Information

* The standards baseline does not list TraceLogging or ETW versions; both are part of the Windows SDK pinned by [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md).
* Component: diagnostics, test and performance (`DBG`).
