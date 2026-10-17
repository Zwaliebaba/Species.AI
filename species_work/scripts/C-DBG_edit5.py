"""C-DBG edit pass 5: align checks, fatal path and outputs with platform_layer.md. Usage: python3 -I C-DBG_edit5.py <file>"""
import re, sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("[[noreturn]] void FatalExit(std::string_view message);                        // DBG-074\n", ""),
 ("#define SPECIES_ASSERT(condition)            /* debug assertion: Debug and Development */", "#define SPECIES_ASSERT(condition)            /* development check: Debug and Development (PLT-122) */"),
 ("2. the assertion levels, the black-box record and the fatal-check path up to its hand-over to crash handling (§4.2);",
  "2. the check macros, the black-box ring and what a failed check records before the platform layer's fatal path takes over (§4.2);"),
 ("| debug assertions (`SPECIES_ASSERT`) | yes | yes | no | no |", "| development checks (`SPECIES_ASSERT`, PLT-122) | yes | yes | no | no |"),
 ("""| debug assertion `SPECIES_ASSERT(c)` | debug assertion, no effect outside debug builds | `Debug`, `Development` | log at error level with the condition, file and line, note in the black box, break into an attached debugger; execution continues when no debugger is attached |
| release check `SPECIES_CHECK(c, message)` | release assertion with a formatted message | every flavour | the fatal path of DBG-073 |
| plain release check (no message) | assertion that reports the condition text, file and line | every flavour | the fatal path, message = condition text |
| container bounds check `SPECIES_CHECK_INDEX(i, n)` | the assertion with arguments that every container bounds check uses | every flavour | the fatal path; an out-of-range container access is a fatal, reported error in every build |
| abort | release assertion that is always false, with a message | every flavour | the fatal path |
| fatal exit `FatalExit(message)` | logged exit with a message dialog and the assertion record | every flavour | DBG-074 |""",
  """| development check `SPECIES_ASSERT(c)` | debug assertion, with no effect outside debug builds | `Debug`, `Development` (PLT-122) | breaks into an attached debugger, otherwise takes the release-check path (PLT-124) |
| release check `SPECIES_CHECK(c, message)` | release assertion with a formatted message | every flavour (PLT-122) | DBG-073 and PLT-123 |
| plain release check (no message) | assertion that reports the condition text, file and line | every flavour | as a release check; the message is the condition text, file and line |
| container bounds check `SPECIES_CHECK_INDEX(i, n)` | the assertion with arguments that every container bounds check uses | every flavour | as a release check: an out-of-range container access is a fatal, reported error in every build |
| abort | release assertion that is always false, with a message | every flavour | as a release check |
| fatal exit | logged exit with a message dialog and the assertion record | every flavour | the platform layer's `FatalExit` (PLT-042), for failures of the environment that are not defects; DBG-074 |"""),
 ("**DBG-072** (MUST) The test builds' simulation checks of §4.8 are debug assertions that the test harness turns into test failures;",
  "**DBG-072** (MUST) The test builds' simulation checks of §4.8 are development checks that the test harness turns into test failures;"),
 ("""**DBG-073** (MUST) The fatal path of a failed check runs in this order:

1. write the message, the condition text, the source location and the thread name into the black box (DBG-047);
2. write a fatal log record with the same content and flush the log synchronously (ADR-0067), so the registered log file holds it;
3. hand over to the platform layer's fatal path, which shows the fatal-error message through the platform message path when a window exists and ends the process with a fail-fast exception, so that Windows Error Reporting captures a dump with the registered files and the black box ([platform layer](platform_layer.md), ADR-0065).

In the reference a failed release assertion formats its message into 512 bytes, shows a fatal-error message box, writes the assertion record and terminates through the runtime's terminate path; whether its crash handler also runs is a property of its runtime library (DEC-341). Species has one defined path.""",
  """**DBG-073** (MUST) A failed release check takes the path of [platform layer](platform_layer.md) PLT-123. This component's part comes first: it writes the check's condition text, source location and thread name next to the black-box record of PLT-123 (DBG-047 item 7), and the full message as a fatal log record, which it flushes synchronously (ADR-0067), so the registered log file holds it. The platform layer then breaks into an attached debugger, or shows `ui.error.internal` unless the run is headless, and ends the process with a fail-fast exception, which Windows Error Reporting captures with the registered files and the black box (ADR-0065). The terminate, invalid-parameter and pure-call handlers take the same path (PLT-114). In the reference a failed release assertion shows its own message in a fatal-error message box, writes the assertion record and ends through the runtime's terminate path; whether its crash handler also runs is a property of its runtime library (DEC-341)."""),
 ("""**DBG-074** (MUST) `FatalExit` is an orderly fatal exit that is not a defect, for a condition such as a missing required capability: it writes the message to the black box and the log, flushes the log, hands the message to the platform message path and ends the process with a non-zero exit code, without a dump.""",
  """**DBG-074** (MUST) Before the platform layer's `FatalExit` ends the process with `plt.lifecycle.exit_code.fatal` (PLT-042), this component writes the exit's string id and arguments to the black box and as a fatal log record and flushes the log; no dump is written."""),
 ("each warning and each failed debug assertion is noted as a check message.", "each warning and each failed development check is noted as a check message."),
 ("7. for a fatal check: its message, its condition text, its source file and line, and the thread name.",
  "7. for a failed release check: the black-box record of PLT-123 (product name, build identity, UTC date and time, the message truncated to `plt.crash.check_message_max_chars`), and the condition text, the source file and line, and the thread name."),
 ("| a failed debug assertion | logged, noted in the black box, debugger break; execution continues | DBG-070 |",
  "| a failed development check | debugger break, otherwise the release-check path | DBG-070, PLT-124 |"),
 ("| language completeness report | the client's language test (R-META-473) | three lists per language | development |\n| command-history recording | the replay hook (§8.2) | the command frames of a match | development |",
  "| language completeness report, in the `diagnostics` folder (PLT) | the client's language test (R-META-473) | three lists per language | development |\n| command-history recording, in the `diagnostics` folder | the replay hook (§8.2) | the command frames of a match | development |"),
 ("| crash report and dump | platform layer, through WER (ADR-0065) | dump of every thread, registered files and the black box | every flavour |",
  "| crash report and dump; on test machines also a local dump in the `crashes` folder (PLT-118) | platform layer, through WER (ADR-0065) | dump of every thread, registered files, the black box and the crash-context record (PLT-120) | every flavour |"),
 ("The reference start-up switches with a diagnostic role belong to their owners: writing the version identity to a file and exiting ([build tooling and distribution](build_tooling_and_distribution.md), ADR-0057), and keeping every log in the working directory instead of the profile folder ([platform layer](platform_layer.md)).",
  "The reference start-up switches with a diagnostic role belong to the platform layer: writing the build identity to a file and exiting (PLT-023, ADR-0057), and moving every log and user file out of the profile folder (PLT-024)."),
 ("The world-state dump on divergence is DEC-206. The version file is the build tooling's (ADR-0057).", "The world-state dump on divergence is DEC-206. The version file is the platform layer's (PLT-023)."),
 ("| 17–20 | crash tests | a fault on the main thread, a fault on a worker thread, a call of the terminate handler, and a hang of the main thread, for the crash-capture test of ADR-0065 | local |",
  "| 17–22 | crash tests | a fault on the main thread, a fault on a worker thread, a call of the terminate handler, an invalid parameter, a pure call, and a hang of the main thread, for the crash-capture tests of ADR-0065 and the platform layer | local |"),
 ("Local commands (rows 14 to 20)", "Local commands (rows 14 to 22)"),
 ("| `ui.dev.command.crash_main`, `.crash_worker`, `.crash_terminate`, `.hang_main` | Fault: main thread · Fault: worker thread · Fault: terminate · Hang: main thread |",
  "| `ui.dev.command.crash_main`, `.crash_worker`, `.crash_terminate`, `.crash_invalid_parameter`, `.crash_pure_call`, `.hang_main` | Fault: main thread · Fault: worker thread · Fault: terminate · Fault: invalid parameter · Fault: pure call · Hang: main thread |"),
 ("the four crash-test commands each produce a dump. | DBG-047, DBG-073, DBG-117 |", "the six crash-test commands each produce a report (platform layer T22). | DBG-047, DBG-073, DBG-117 |"),
 ("**DBG-222** (MUST) Crash capture is the platform layer's ([platform layer](platform_layer.md), ADR-0065):", "**DBG-222** (MUST) Crash capture is the platform layer's ([platform layer](platform_layer.md) PLT-113 to PLT-121, ADR-0065):"),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:80]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok', len(t.encode('utf-8')))
