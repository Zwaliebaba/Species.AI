---
status: proposed
date: 2026-10-19
decision-makers: Species technology lead (M0 task T3)
---

# Log pipeline: two rings per thread, a drain thread, and the logger's registration

## Context and Problem Statement

ADR-067 and the diagnostics component fix what the logging system does. Each thread writes into its own lock-free buffer of `dbg.log.thread_buffer_kib` (256 KiB), and one logging thread drains the buffers into the sinks (DBG-061). When a buffer is full, trace records are dropped first and then debug records. Warnings, errors and fatal records are never dropped, and a writer waits only when its buffer holds nothing but such records. Drops are counted per category and level, and reported in a warning (DBG-062). A long message is logged whole (DBG-060). Nothing reaches the logger through a process-wide global (DBG-040), yet `Log` is a free function. The specifications leave open:
- the mechanism that meets the drop rule;
- how `Log` finds the logger;
- the layout of a record and the log file's name;
- where the TraceLogging provider comes from;
- how many records the platform keeps before its sink is set (PLT-133).

M0 task T3 builds the logger in NeuronCore and needs each answer.

## Decision Drivers

* The drop rule exactly as DBG-061 states it, including "waits only when its buffer holds nothing but such records".
* No lock and no allocation on the calling thread beyond formatting into a reused per-thread buffer (`dbg.budget.log_call_us`, DBG-206).
* Each thread's records in their order, and a drain that never reorders one thread's records.
* One owner and an explicit lifetime for the logger (DBG-040), with no game name in the engine (`AGENTS.md` R9).
* Deterministic unit tests: a drain that a test can run without a thread.

## Considered Options

* 1 — two single-producer, single-consumer byte rings per thread, one for trace and debug and one for info and above, merged by a per-thread sequence number
* 2 — one ring per thread whose free space trace and debug records may use only up to high-water marks
* 3 — a queue per thread behind a mutex, from which the drain removes trace and debug records when it is full

## Decision Outcome

Chosen option: "1". It is the only one that meets the drop rule's last clause without a lock: in option 2 a warning waits on a buffer that still holds debug records, and option 3 takes a lock on every call.

* **Rings.** Each thread's buffer is two rings of half its size, in records aligned to 8 bytes with a 40-byte header: the kind, level, category, sequence number, platform-clock time, UTC time and message length.
  * Trace and debug records share one ring, which admits a trace record only while it is less than half full and a debug record until it is full, and counts what it drops per category and level.
  * Info, warning, error and fatal records have the other ring to themselves, and a writer waits on its consumed position (`std::atomic::wait`) only when that ring is full, after asking the drain to run.
  * A message longer than a quarter of a ring goes in parts. A trace or debug message is admitted whole or not at all; an info or higher message waits between parts.
  * The drain reads every record below the thread's published sequence number from both rings, in sequence order, so one thread's order is kept across the two. It then sorts the records of all threads by their platform-clock time, keeping the order of equal times.
* **Drain.** The logging thread drains every buffer at least every 10 ms, and at once when a writer asks. Each drain that finds new drops writes the DBG-062 warning, `log records dropped since the last report:` followed by the category, level and count of each. A test builds the logger without the thread and drains it with `Flush`. The logging thread's own records go straight to the sinks, because it cannot wait for itself.
* **Registration.** The composition root owns the `Logger`. While it exists it registers itself as the target of `Log`, in an atomic pointer that it sets in its constructor and clears in its destructor. This is a registration of the owner's object for the owner's lifetime, as `SetPlatformLogSink` is for the platform, not a global object (DBG-040). The owner destroys the logger last, when no other thread logs. A thread that outlives one logger keeps its old buffer alive until it ends and never writes to it again: each logger has an id, and a thread's buffer belongs to the logger whose id it holds.
* **Record line.** `<platform-clock seconds, 6 decimals> <UTC hh:mm:ss.mmm> [<thread name>] <Category> <level>: <message>`, one line per record. A hex or value dump is one record per line, led by its offset (DBG-063).
* **File.** `<UTC start yyyyMMddTHHmmss>Z-<run id, 16 hex digits>.log` in the logs folder that the composition root passes, after the oldest `.log` files are deleted so that `dbg.log.files_kept` remain with the new one (DBG-049). Name order is age order. The file's header holds the run id, the UTC start time and the fields that the composition root supplies (DBG-044). Until the build identity (BLD-020), the channel tag and the platform's system report exist, those fields are whatever the root passes.
* **TraceLogging provider.** The composition root defines and registers the provider and passes its handle. The engine writes its events (`Log`, with the fields `Category`, `Thread` and `Message`, at the level that matches the record's) but defines no provider, so the provider carries the game's name without the engine knowing it (R9). With no handle there is no ETW sink.
* **Platform records before the sink.** The platform keeps up to 1024 records before its sink is set. It counts the records that do not fit and reports the count when the sink is set (PLT-133).
* **Not yet.** The black box (DBG-047) and its failure record (DBG-073 step 1), the Windows Error Reporting registrations (PLT-119) and `FatalExit` come with crash reporting, which the M0 plan puts before the first external build. The fatal path has steps 2 and 3, the flushed fatal record and `FailFast`.

### Consequences

* Good, because the drop rule holds as written, and the writer takes no lock on any path but the full-ring wait.
* Good, because the drain can run on the test's own thread, so the drop policy, the order and the long messages have deterministic tests.
* Good, because the engine names no provider and no game.
* Bad, because each thread that logs costs 256 KiB, and a thread that logs only warnings uses half of it, as a thread that logs only trace and debug does.
* Bad, because one thread's records interleave with another's in time order only to the precision of the platform clock; two threads' records at the same reading keep the order in which their buffers were drained.
* Neutral, because the logger's pointer is process-wide state, set and cleared by its owner; a second logger alive at the same time registers nothing.
* It forecloses: a mutex on the calling thread; a record dropped because of its length; a provider defined in NeuronCore.

### Confirmation

* `NeuronCoreTests`:
  * `LogBufferTests` checks that trace is dropped before debug, that warnings are never dropped (their ring fills and reports full), that the order holds across both rings over several wraps, that a 5000-byte message arrives whole, and that drops are counted per category and level.
  * `LogTests` checks a verbatim argument, the record line, the hex and value dumps, the level rules, the drop warning, platform records and the files kept.
* Not yet measured: the per-call cost and the frame-time effect of DBG T2. The performance job reports them from M1 (DEC-971).

## Pros and Cons of the Options

### 1 — two rings and a sequence number

* Good, because each of DBG-061's clauses is a property of one ring.
* Bad, because the drain merges two rings per thread.

### 2 — one ring with high-water marks

* Good, because it is one ring.
* Bad, because a warning waits while the buffer still holds debug records, against DBG-061.

### 3 — a locked queue that the drain prunes

* Good, because the drain could drop the oldest trace records first.
* Bad, because every log call takes a lock that the drain also takes.

## More Information

* Related: [ADR-067](ADR-067-logging.md) (logging), [ADR-068](ADR-068-profiling-and-gpu-timing.md) (markers), [ADR-074](ADR-074-platform-clock-with-injectable-time.md) (the clock), [ADR-090](ADR-090-solution-and-project-layout.md) (NeuronCore). The code is `NeuronCore/Log.h`, `NeuronCore/LogBuffer.h` and `NeuronCore/LogFile.h`.
* **Amendment 2026-10-22 (M1's exit, DBG T2, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md) amendment).** The performance tool measures the per-call cost. Eight threads, started together, log 10⁶ debug records between them into a logger of their own, which has its drain thread and writes its file into a temporary folder. The script judges the mean cost of a call on the calling thread against `dbg.budget.log_call_us` (DBG-206), and reports the slowest thread's mean and the records written and dropped. A Release build folds a debug call away at compile time (DBG-064), so the tool calls `LogFormatted` at the debug level: the path of a record that has passed the level test. The frame-time effect is not measured at M1. The performance job measures a Release build (DBG-208), where no debug record is formatted or written, so a flood of them cannot reach a measured frame. That item of the confirmation stays open until a measured build records debug records.
