"""C-DBG edit pass 7: re-align checks and fatal path with the revised platform layer
(PLT-118, PLT-126, PLT-127, PLT-044 now defer to DBG-070 to DBG-075). Usage: python3 -I C-DBG_edit7.py <file>"""
import sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("[[noreturn]] void CheckFailed(std::source_location, std::string_view condition, std::string_view message);\n",
  "[[noreturn]] void CheckFailed(std::source_location, std::string_view condition, std::string_view message);\n"
  "[[noreturn]] void FatalExit(std::string_view string_id, std::span<const MessageArgument> arguments);  // DBG-074\n"),
 ("#define SPECIES_ASSERT(condition)            /* development check: Debug and Development (PLT-122) */",
  "#define SPECIES_ASSERT(condition)            /* development check: Debug and Development */"),
 ("2. the check macros, the black-box ring and what a failed check records before the platform layer's fatal path takes over (§4.2);",
  "2. the check levels and the flavours that compile them in, the black box, and the fatal path up to the platform layer's last step (§4.2);"),
 ("| development checks (`SPECIES_ASSERT`, PLT-122) | yes | yes | no | no |", "| development checks (`SPECIES_ASSERT`) | yes | yes | no | no |"),
 ("""| development check `SPECIES_ASSERT(c)` | debug assertion, with no effect outside debug builds | `Debug`, `Development` (PLT-122) | breaks into an attached debugger, otherwise takes the release-check path (PLT-124) |
| release check `SPECIES_CHECK(c, message)` | release assertion with a formatted message | every flavour (PLT-122) | DBG-073 and PLT-123 |""",
  """| development check `SPECIES_ASSERT(c)` | debug assertion, with no effect outside debug builds | `Debug`, `Development` | breaks into an attached debugger (PLT-121); otherwise logs an error record with the condition, file and line, notes it in the black box and continues, and the test harness turns it into a test failure (DBG-072) |
| release check `SPECIES_CHECK(c, message)` | release assertion with a formatted message | every flavour | the fatal path of DBG-073 |"""),
 ("| fatal exit | logged exit with a message dialog and the assertion record | every flavour | the platform layer's `FatalExit` (PLT-042), for failures of the environment that are not defects; DBG-074 |",
  "| fatal exit `FatalExit(string id, arguments)` | logged exit with a message dialog and the assertion record | every flavour | DBG-074 |"),
 ("""**DBG-073** (MUST) A failed release check takes the path of [platform layer](platform_layer.md) PLT-123. This component's part comes first: it writes the check's condition text, source location and thread name next to the black-box record of PLT-123 (DBG-047 item 7), and the full message as a fatal log record, which it flushes synchronously (ADR-0067), so the registered log file holds it. The platform layer then breaks into an attached debugger, or shows `ui.error.internal` unless the run is headless, and ends the process with a fail-fast exception, which Windows Error Reporting captures with the registered files and the black box (ADR-0065). The terminate, invalid-parameter and pure-call handlers take the same path (PLT-114).""",
  """**DBG-073** (MUST) The fatal path of a failed release check, a container bounds check or an abort runs in this order; the terminate, invalid-parameter and pure-call handlers enter it at step 1 with their reason ([platform layer](platform_layer.md) PLT-118):

1. write the black-box record of the failure (DBG-047 item 7);
2. write the full message as a fatal log record and flush the log synchronously (ADR-0067), so the registered log file holds it;
3. hand over to the platform layer's `FailFast` (PLT-127), which breaks into an attached debugger, or shows `ui.error.internal` with the run id when a window exists, and ends the process with a fail-fast exception, which Windows Error Reporting captures with the registered files and the black box (ADR-0065).

"""),
 ("""**DBG-074** (MUST) Before the platform layer's `FatalExit` ends the process with `plt.lifecycle.exit_code.fatal` (PLT-042), this component writes the exit's string id and arguments to the black box and as a fatal log record and flushes the log; no dump is written.""",
  """**DBG-074** (MUST) `FatalExit(string id, arguments)` serves failures of the environment, which are not defects (PLT-044): it writes the string id and arguments to the black box and as a fatal log record, flushes the log, and hands over to the platform layer's `EndWithMessage`, which shows the message and ends the process with `plt.lifecycle.exit_code.fatal` (PLT-044); no dump is written."""),
 ("7. for a failed release check: the black-box record of PLT-123 (product name, build identity, UTC date and time, the message truncated to `plt.crash.check_message_max_chars`), and the condition text, the source file and line, and the thread name.",
  "7. for a failed check or a fatal exit, the black-box record: the product name, the build identity, the UTC date and time, the message truncated to `dbg.blackbox.check_message_max_chars` (512 characters; the full message goes to the log), the condition text, the source file and line, and the thread name."),
 ("| `dbg.blackbox.recent_checks` | 8 | messages |", "| `dbg.blackbox.check_message_max_chars` | 512 | characters | the reference's message length; the log keeps the full text | DBG-047 | — |\n| `dbg.blackbox.recent_checks` | 8 | messages |"),
 ("| a failed development check | debugger break, otherwise the release-check path | DBG-070, PLT-124 |",
  "| a failed development check | debugger break, otherwise logged and noted; the test fails | DBG-070, DBG-072 |"),
 ("**DBG-222** (MUST) Crash capture is the platform layer's ([platform layer](platform_layer.md) PLT-113 to PLT-121, ADR-0065):",
  "**DBG-222** (MUST) Crash capture is the platform layer's ([platform layer](platform_layer.md) PLT-117 to PLT-125, ADR-0065):"),
 ("| crash report and dump; on test machines also a local dump in the `crashes` folder (PLT-118) | platform layer, through WER (ADR-0065) | dump of every thread, registered files, the black box and the crash-context record (PLT-120) | every flavour |",
  "| crash report and dump; on test machines also a local dump in the `crashes` folder (PLT-122) | platform layer, through WER (ADR-0065) | dump of every thread, registered files, the black box and the crash-context record (PLT-124) | every flavour |"),
 ("and moving every log and user file out of the profile folder (PLT-024).", "and moving every log and user file out of the profile folder (PLT-025)."),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:90]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok', len(t.encode('utf-8')))
