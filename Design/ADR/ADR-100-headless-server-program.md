---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T11)
---

# The headless server program at M0: its composition root, its stop and the check of what it loads

## Context and Problem Statement

ADR-090 gives the solution a second program, `SpeciesServer.exe`, that links NeuronCore, NeuronServer, GameShared and GameLogic and never NeuronClient or GameClient. ADR-083 items 1 to 3 give each process role a composition root with explicit lifetimes. PLT-005 lets the headless server use only the portable platform services and link no window, display, input or audio code. MVP-071 makes the program the always-on world's host from M4, reached over MsQuic.

M0 task T11 builds the program running the same world with no client. Its done-when is ADR-090's confirmation: `dumpbin /dependents SpeciesServer.exe` shows no Direct3D, DXGI or GameInput library.

The specifications leave open, for M0:
- what the program composes before it has a transport;
- how an operator stops it so that the world freezes rather than dies;
- where it writes its log and how it reports a failure;
- how the check of what it loads runs.

The server's process roles, start-up options and data locations stay open in DEC-220 and DEC-251.

## Decision Drivers

* The integrated and the dedicated server are one program (ADR-090): the same `WorldServer` behind the same `ServerHost`.
* Stopping freezes the world and loses nothing (O4, ADR-083 item 9), whichever way the console asks.
* A headless process shows no dialog: its reader is an operator at a console, or nobody.
* What the executable loads is checked from the executable, not inferred from the project files.

## Considered Options

* 1 — a console program that runs the world until its console asks it to stop, through a stop service in the platform layer; failures on the console and in the log; the import check in CI after the build
* 2 — the same program stopped by a key read from standard input
* 3 — the program as a Windows service from the start

## Decision Outcome

Chosen option: "1".
- Option 2 cannot see the console window closing, the user logging off or the system shutting down, so the world would die rather than freeze.
- Option 3 decides DEC-220's process roles before M4 has evidence for them, and the MVP's server is one that the client starts (MVP-071).

* **The composition root** (`SpeciesServer/SpeciesServer.cpp`) runs PLT-038's steps that a headless process has:
  - the fatal handlers, the main thread's name, the counter clock, the floating-point environment, the run id and the logger;
  - then the tuning record from the interim content root (ADR-098), the `WorldServer` of one region, and `ServerHost` on the real clock;
  - the host ticks at the period the record states (`ref.sim.tick_period`), with 2 workers, the integrated server's count (ADR-080 item 7). M0's one region needs no more, and ADR-080 item 8's hosts size the pool when a world has more regions than one.
* **No transport and no saves at M0.**
  - The host has no listener: the MsQuic listener of ADR-088 and ADR-089 arrives with MVP-071 at M4, and the in-process transport has no client in another process.
  - The world server has no snapshot folder, because saving starts at M2 (MVP-075, ADR-099).
* **The stop** is a platform service, NeuronCore's `ConsoleStop.h`, because it is an operating-system call (PLT-003).
  - It handles Ctrl+C, Ctrl+Break, the console window closing, the user logging off and the system shutting down, each as a stop request. The composition root waits for it, stops the host and so freezes the world (ADR-083 item 9), then flushes the log.
  - The system ends a process as soon as its handler returns from a close, log-off or shutdown event. So the handler holds those events until the root calls `ConsoleStopFinished`, and the system's own time limit still ends a stop that hangs.
* **What the operator sees.**
  - The console shows three lines: the run id and the log file, the tick period, and the tick the world froze before. The log holds the rest.
  - A failure goes to the log and to standard error: no game data, a record that cannot be read with each finding, or a stop handler that cannot be installed. The program then ends with `plt.lifecycle.exit_code.fatal`.
  - It shows no dialog. The pre-UI messages of PLT-039 are the client's, and the fatal path shows `ui.error.internal` only when a window owns pre-UI messages, which the server never has.
* **The log folder** is `logs\server` beside the executable, apart from the client's `logs` (ADR-092's interim folder), until DEC-251 decides the server's data locations.
  - Each folder keeps its own `dbg.log.files_kept`, so a run of one program never prunes the other's logs.
* **The entry point** is `wmain`, the Unicode console counterpart of the client's `wWinMain`. Like it, it catches nothing, so an exception that leaves the root reaches Windows Error Reporting untouched (PLT-117).
  - `.clang-tidy` exempts the name `wmain` from the function naming rule, because the C runtime fixes it (R4). The SDK declares `wWinMain`, which exempts that name already.
* **The fatal path's header is headless.** The window handle's alias, which spells the SDK's `HWND__`, and the pre-UI message owner's functions move from `PlatformMessage.h` to `NativeWindowHandle.h`.
  - Only the code that holds a window includes that header: NeuronClient's window and NeuronCore's message and fatal-path sources.
  - So a headless source can include `FatalPath.h` with no Windows header before it.
* **The checks.**
  - `Build\CheckProjectFiles.py` adds `SpeciesServer` to the projects that include no Windows header (ADR-083 item 1).
  - `Build\CheckServerImports.py`, a CI step after the build, runs `dumpbin /dependents` on `x64\Debug\SpeciesServer.exe`. It fails on an import or a delay-loaded import that names a Direct3D, DXGI, DXCore, GameInput, XAudio2 or XInput library: ADR-090's three and PLT-005's input and audio.
  - It finds dumpbin on the path or under the toolset that `VCToolsInstallDir` names, and its fixture proves that a listing naming `d3d12.dll` is reported.

### Consequences

* Good, because the program that M4 extends with a listener already runs the world, and the import check guards it from the first build.
* Good, because every way a console asks a process to end freezes the world.
* Bad, because the M0 server serves nobody: it proves the headless build and the world's run, not a connection.
* Bad, because an exception in the root is not caught. The process ends through Windows Error Reporting rather than with a message, which is PLT-117's choice for the client too.
* It forecloses: a headless source that includes a Windows header; a dialog from the headless server; a server stopped by killing its process as the normal way.

### Confirmation

* CI's `Check the headless server's libraries` step passes on the Debug|x64 build.
* `CheckProjectFiles.py` passes with `SpeciesServer` among the headless projects.
* The owner runs `SpeciesServer.exe` from a console in `x64\Debug`:
  - it prints its run id and log file, then the 100 ms period;
  - Ctrl+C freezes the world and prints the tick it froze before, and the exit code is 0;
  - closing the console window instead writes the same freeze to the log;
  - with the record's `tick_period` set to -1, it lists the finding and ends with exit code -1.
* ADR-090's confirmation is met when the CI step passes.

## Pros and Cons of the Options

### 1 — a console program with a platform stop service

* Good, because it sees every stop the console can send.
* Bad, because the handler runs on a thread of the system's, so the stop's state is process-wide.

### 2 — a key on standard input

* Good, because it needs no operating-system call.
* Bad, because closing the console kills the world instead of freezing it.

### 3 — a Windows service

* Good, because it is what an unattended host runs.
* Bad, because it decides DEC-220 early, and MVP-071's server is started by the client.

## More Information

* Related: [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md), [ADR-089](ADR-089-msquic-transport-package.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-092](ADR-092-first-window-device-and-presentation.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md), [ADR-099](ADR-099-world-snapshots.md).
* The code is in NeuronCore (`ConsoleStop`, `NativeWindowHandle.h`), `SpeciesServer` and `Build\CheckServerImports.py`.
