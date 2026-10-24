---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T9; owner decisions O54, O109, O110)
---

# The always-on world's server process

## Context and Problem Statement

An always-on world runs while its steward's client is closed (MVP-071, PIL-104). At M4 it runs on the steward's own computer, in `SpeciesServer.exe`. The owner decided four things (O110):
- it runs in a console window of its own, and closing the window stops it;
- one always-on world runs at a time.

The client starts the server when the world is not running, connects to it over `QuicTransport` (ADR-189), and leaves it running when it leaves.

Until now the headless server ran the battle sandbox or a Debug `--world`, with no listener and no saves, and the world's description (`world.txt`) belonged to the client.

How does the server find its world, how does the client find the server, how is "one at a time" kept, and how does a client stop a server it cannot use?

## Decision Drivers

* The steward never types a port or a path: the client starts and finds the server (O110).
* One always-on world at a time, kept by the system rather than by a file that a crash leaves behind.
* Closing the console freezes the world cleanly (ADR-100, ADR-188).
* The platform layer is the only code that reaches the system (PLT-003, ADR-083 item 1).
* A server of another build is not used (ADR-190).

## Considered Options

1. **The world's folder is the meeting place.** The client starts `SpeciesServer.exe --always-on-world <folder>`. The server reads the world's description there, saves its snapshots there, and writes a listing there (its port, certificate fingerprint, build stamp and process). A named lock of the user's session keeps one server at a time, and a named stop signal lets a client stop it.
2. **A fixed port.** The server always listens on one port, and the client connects there.
3. **The client passes everything on the command line.** The client gives the server the world's parameters and a port it chose.

## Decision Outcome

Chosen option: 1.
- Option 2 fails when another program holds the port, and cannot tell this world's server from another's.
- Option 3 still needs a way back from the server for its certificate's fingerprint, and duplicates the description the world already has.

What is built:

* **The world's description is shared.** `WorldSaves` moves from GameClient to GameShared, so the headless server reads the same `world.txt`. The description gains the pace (`world.pace`, `session` or `always_on`). A description without one is at the session pace, and one with an unknown pace does not read. `DescribedWorld(folder)` reads one folder's world.
* **The platform's pieces** (`NeuronCore`):
  - `InstanceLock`: a named object of the session's namespace (`Local\`), which exists while its holder holds it and goes when that process ends, however it ends.
  - `StopSignal`: a named event that a process makes and waits on, and another raises.
  - `StartInOwnConsole`: `CreateProcessW` with `CREATE_NEW_CONSOLE`, in the program's folder, inheriting no handle, each argument quoted for `CommandLineToArgvW` (`CommandLineOf`).
  - `ProcessRunning` and `CurrentProcessId`.
  - `RequestConsoleStop`: the program's own stop request, which ends `WaitForConsoleStop` as Ctrl+C does.
  - `--always-on-world <folder>`, a start-up option of every build.
* **The server** (`SpeciesServer.exe --always-on-world <folder>`):
  1. takes `ALWAYS_ON_LOCK` or refuses to start, since another always-on world runs;
  2. reads the folder's description, which must be at the always-on pace, and makes its stop signal;
  3. opens its certificate (`Species server`, ADR-189) and listens on the loopback interface on a port the system chooses;
  4. runs the world at the always-on pace with its identity (ADR-190). A folder with snapshots is continued or not run (`restoreOnly`, ADR-155); one without is a new world from the description's seed, site and AI settlements;
  5. writes the listing `server.txt`: the port, the certificate's fingerprint, the build's stamp and its process id.
  
  It then runs until its console closes or a client raises the stop signal, which a thread of its own turns into a console stop. It removes the listing and freezes the world. Every refusal is printed in its console and logged.
* **The client** (`GameClient/AlwaysOnClient`, `ReachAlwaysOnWorld`):
  - When the lock is held, it reads the world's listing. A listing whose process runs and whose build is the client's is this world's server.
  - Any other running server (another world's, or this world's of another build) is asked to stop, and the client waits up to 10 s for the lock to go.
  - It then starts `SpeciesServer.exe` beside itself with the world's folder, and waits up to 30 s for a listing of that process. A server that ends first has written why in its console.
  - It connects with the listing's port and fingerprint. Leaving the world closes the session only; the server runs on.
  - A world that cannot be reached is the front end's `ContinueFault::Server`, "This world's server could not be started."
* **Continue.** An always-on world whose server runs can be continued before its first snapshot (`LatestWorld`).

### Consequences

* Good, because the steward chooses a world and plays, and the server's console is the only new thing they see.
* Good, because a crash leaves no lock: the lock is the process's own handle. A listing a crash leaves names a process that has ended, which counts as none.
* Bad, because opening another always-on world stops the running one without asking. That is O110's one-at-a-time rule, but the front end does not yet say it will happen.
* Bad, because the client waits on the main thread while a server starts, up to 30 s with nothing drawn.
* It forecloses nothing: a dedicated server on another computer is reached by an address and a listing the operator publishes, through the same transport and join.

### Confirmation

* `NeuronCoreTests/InstanceLockTests`: one holder at a time, the lock gone with its holder; a stop signal reaching its maker; the command line's quoting.
* `NeuronCoreTests/StartupOptionsTests`: `--always-on-world` in every build.
* `GameSharedTests/AlwaysOnServerTests`: the listing round-trips, a damaged one does not read, a removed one is gone.
* `GameSharedTests/WorldSavesTests`: the pace in the description, a world before M4 at the session pace, an unknown pace refused, and a running server's world continued before its first snapshot.
* T9's run checklist: the owner starts an always-on world, closes the client, reopens it and continues the running world, then closes the server's console and continues the world again, with the freeze recorded.

## Pros and Cons of the Options

### 1 — the world's folder

* Good, because everything about a world stays in its folder.
* Bad, because the client must wait for the server's listing.

### 2 — a fixed port

* Good, because the client needs nothing from the server.
* Bad, because a port may be taken, and nothing tells one world's server from another's.

### 3 — the command line

* Good, because the server needs no description.
* Bad, because the certificate still needs a way back to the client.

## More Information

* Owner decisions O54, O109, O110; MVP-071, MVP-075, MVP-103, PLT-003; the [M4 plan](../implementation/m4_plan.md), task T9.
* Amends [ADR-100](ADR-100-headless-server-program.md) (the always-on mode), [ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md) (the description's pace, `WorldSaves` in GameShared, Continue of a running world) and [ADR-189](ADR-189-the-quic-transport-and-the-servers-certificate.md) (the server's certificate name).
* The code: `NeuronCore/InstanceLock`, `NeuronCore/ProgramStart`, `NeuronCore/ConsoleStop`, `NeuronCore/StartupOptions`, `GameShared/AlwaysOnServer`, `GameShared/WorldSaves`, `GameClient/AlwaysOnClient`, `SpeciesServer/SpeciesServer.cpp`, `Species/Species.cpp`.
