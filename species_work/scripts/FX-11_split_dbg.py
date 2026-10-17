"""FX-11: split diagnostics_test_and_performance.md into a folder (Phase 4 item 113).

Usage: python3 -I FX-11_split_dbg.py <components dir>

Moves the text of the document verbatim into <components>/diagnostics_test_and_performance/,
re-basing every relative link for the deeper folder. Writes nothing else and deletes nothing.
"""
import os
import re
import sys

COMP = os.path.abspath(sys.argv[1])
SRC = os.path.join(COMP, 'diagnostics_test_and_performance.md')
OUT = os.path.join(COMP, 'diagnostics_test_and_performance')
L = open(SRC, encoding='utf-8').read().split('\n')
LINK = re.compile(r'(?<!!)(\[(?:[^\]\[]|\[[^\]]*\])*\]\()([^)\s]+)(\))')


def rebase(text):
    def fix(m):
        t = m.group(2)
        if re.match(r'^[a-z][a-z0-9+.-]*:', t, re.I) or t.startswith('#'):
            return m.group(0)
        path, sep, anchor = t.partition('#')
        dest = os.path.normpath(os.path.join(COMP, path))
        new = os.path.relpath(dest, OUT).replace(os.sep, '/')
        return m.group(1) + new + (sep + anchor if sep else '') + m.group(3)
    return LINK.sub(fix, text)


L = [rebase(x) for x in L]  # the moved text is re-based once; the new text below is written for the folder


def lines(a, b):
    """1-based inclusive line range of the source, as text."""
    return '\n'.join(L[a - 1:b])


def find(prefix, start=1):
    for i in range(start - 1, len(L)):
        if L[i].startswith(prefix):
            return i + 1
    raise SystemExit(f'not found: {prefix}')


# section boundaries (1-based line numbers), found by heading text so the script is exact
H = {}
for key, text in [('s1', '## 1 Responsibilities and boundaries'), ('s2', '## 2 Interfaces'),
                  ('s3', '## 3 Data model'), ('s31', '### 3.1 '), ('s32', '### 3.2 '), ('s33', '### 3.3 '),
                  ('s34', '### 3.4 '), ('s35', '### 3.5 '), ('s36', '### 3.6 '), ('s4', '## 4 Behaviour'),
                  ('s41', '### 4.1 '), ('s42', '### 4.2 '), ('s43', '### 4.3 '), ('s44', '### 4.4 '),
                  ('s45', '### 4.5 '), ('s46', '### 4.6 '), ('s47', '### 4.7 '), ('s48', '### 4.8 '),
                  ('s49', '### 4.9 '), ('s410', '### 4.10 '), ('s411', '### 4.11 '), ('s5', '## 5 '),
                  ('s51', '### 5.1 '), ('s52', '### 5.2 '), ('s53', '### 5.3 '), ('s54', '### 5.4 '),
                  ('s55', '### 5.5 '), ('s6', '## 6 '), ('s7', '## 7 '), ('s8', '## 8 '), ('s81', '### 8.1 '),
                  ('s82', '### 8.2 '), ('s83', '### 8.3 ')]:
    H[key] = find(text)
N = len(L)
dev_text = find('**Development text.**')
keys_elsewhere = find('Keys cited here and defined elsewhere')
dbg221 = find('**DBG-221**')
dbg222 = find('**DBG-222**')

# ---- tuning rows, by key prefix
rows = [l for l in L[H['s411'] - 1:dev_text - 1] if l.startswith('| `dbg.')]
assert len(rows) == 124, len(rows)
tune_head = '| key | value | unit | range or notes | requirement | DEC |\n|---|---|---|---|---|---|'
convention = L[H['s411'] + 1]
assert convention.startswith('A value whose notes say'), convention


def pick(*prefixes):
    return '\n'.join(r for r in rows if r.startswith(tuple('| `' + p for p in prefixes)))


T_README = pick('dbg.setting.', 'dbg.replay.')
T_LOG = pick('dbg.log.', 'dbg.blackbox.', 'dbg.profile.')
T_DEV = pick('dbg.overlay.', 'dbg.netstats.', 'dbg.commands.', 'dbg.draw.', 'dbg.bot_overlay.')
T_TEST = pick('dbg.trace.', 'dbg.soak.', 'dbg.telemetry.')
T_PERF = pick('dbg.budget.', 'dbg.stress.', 'dbg.machine.')
assert sum(len(t.split('\n')) for t in (T_README, T_LOG, T_DEV, T_TEST, T_PERF)) == 124

# ---- acceptance-test rows
trow = {}
for l in L[H['s7'] - 1:H['s8'] - 1]:
    m = re.match(r'^\| (T\d+) \|', l)
    if m:
        trow[m.group(1)] = l
assert len(trow) == 30, len(trow)
test_head = '| # | Kind | Test | Requirements |\n|---|---|---|---|'


def tests(*ids):
    return '\n'.join(trow[i] for i in ids)


T7 = ('T7', 'T11')
TLOG = ('T1', 'T2', 'T3', 'T4', 'T5', 'T6', 'T8', 'T9', 'T12', 'T13')
TDEV = ('T14', 'T15', 'T16', 'T17', 'T18', 'T19')
TTEST = ('T20', 'T21', 'T22', 'T23', 'T24', 'T25', 'T26', 'T27', 'T28', 'T30')
TPERF = ('T10', 'T29')
assert sorted(T7 + TLOG + TDEV + TTEST + TPERF) == sorted(trow)

README = 'README.md'
F_LOG = 'logging_checks_and_profiling.md'
F_DEV = 'developer_tools.md'
F_TEST = 'test_harness_soak_and_telemetry.md'
F_PERF = 'performance_baseline.md'
FOLDER = 'Folder: [diagnostics, test and performance](README.md)'

status = L[2]
assert status.startswith('> Status: Engine baseline · Prefix: DBG')

index = f"""## Folder index

| File | Holds | Requirement IDs |
|---|---|---|
| [README.md](README.md) (this file) | responsibilities and boundaries, the facilities per build configuration, the interface sketch, the debug entry points and development bindings, the counter providers and data contracts, the threading contract, ownership and lifetimes, the hidden development settings, the diagnostic outputs, the tuning-table convention, diagnostics and concurrency, the failure modes, the component-wide tests and the open decisions | DBG-001 to DBG-006, DBG-020 to DBG-031, DBG-040 to DBG-042, DBG-049, DBG-200, DBG-220, DBG-222, DBG-223 |
| [{F_LOG}]({F_LOG}) | §3.3 to §3.5 and §4.1 to §4.3: log records and categories, the profile marker catalogue, the black-box contents, logging, the check levels, the black box and the fatal path, profiling and the profile view | DBG-043 to DBG-048, DBG-060 to DBG-066, DBG-070 to DBG-075, DBG-080 to DBG-090 |
| [{F_DEV}]({F_DEV}) | §4.4 to §4.7: the development overlay, the network statistics window, the debug commands and the debug-command window, development drawing and the bot overlay; the development text; the classification of the reference cheats | DBG-095 to DBG-099, DBG-105 to DBG-110, DBG-115 to DBG-121, DBG-130 to DBG-136, DBG-221 |
| [{F_TEST}]({F_TEST}) | §4.8 to §4.10 and §8.2: the test harness, the soak test, telemetry and the replay hooks | DBG-140 to DBG-150, DBG-160 to DBG-167, DBG-180 to DBG-183, DBG-240, DBG-241 |
| [{F_PERF}]({F_PERF}) | §5.2 to §5.5: the frame and tick structure of the reference, the reference performance figures, the Species budget baseline and the reference test machine | DBG-201 to DBG-209 |

Section numbers are those of the whole component: a section cited as §4.8 is in the file that the index names for it."""

readme = '\n'.join([
    L[0], '', status, '',
    lines(5, H['s1'] - 1).rstrip('\n'), '',
    index, '',
    lines(H['s1'], H['s33'] - 1).rstrip('\n'), '',
    lines(H['s36'], H['s4'] - 1).rstrip('\n'), '',
    L[H['s4'] - 1], '',
    f'The behaviour of the component is specified in the topic files of this folder: §4.1 to §4.3 in [logging, checks and profiling]({F_LOG}), §4.4 to §4.7 in [developer tools]({F_DEV}) and §4.8 to §4.10 in [test harness, soak test and telemetry]({F_TEST}).', '',
    L[H['s411'] - 1], '',
    convention, '',
    f'Each topic file defines the keys its requirements own in its own tuning table: `dbg.log.*`, `dbg.blackbox.*` and `dbg.profile.*` in [logging, checks and profiling]({F_LOG}); `dbg.overlay.*`, `dbg.netstats.*`, `dbg.commands.*`, `dbg.draw.*` and `dbg.bot_overlay.*` in [developer tools]({F_DEV}); `dbg.trace.*`, `dbg.soak.*` and `dbg.telemetry.*` in [test harness, soak test and telemetry]({F_TEST}); `dbg.budget.*`, `dbg.stress.*` and `dbg.machine.*` in [performance baseline]({F_PERF}). The keys of the hidden development settings (§3.2) are these:', '',
    tune_head, T_README, '',
    L[keys_elsewhere - 1], '',
    lines(H['s5'], H['s52'] - 1).rstrip('\n'), '',
    f'Sections 5.2 to 5.5 are in [performance baseline]({F_PERF}).', '',
    lines(H['s6'], dbg221 - 1).rstrip('\n'), '',
    f'The classification of the reference cheats, DBG-221, is in [developer tools]({F_DEV}) §6.', '',
    lines(dbg222, H['s7'] - 1).rstrip('\n'), '',
    L[H['s7'] - 1], '',
    f'The tests of each topic are in its file: T1 to T6, T8, T9, T12 and T13 in [logging, checks and profiling]({F_LOG}); T14 to T19 in [developer tools]({F_DEV}); T20 to T28 and T30 in [test harness, soak test and telemetry]({F_TEST}); T10 and T29 in [performance baseline]({F_PERF}). The component-wide tests are these:', '',
    test_head, tests(*T7), '',
    lines(H['s8'], H['s82'] - 1).rstrip('\n'), '',
    f'Section 8.2 is in [test harness, soak test and telemetry]({F_TEST}).', '',
    lines(H['s83'], N).rstrip('\n'), ''])


def subfile(title, adrs, related, intro, holds, interfaces, data, behaviour, conc, fail, tests_rows, open_dec, tune):
    parts = [f'# {title}', '', f'> Status: Engine baseline · Prefix: DBG · ADRs: {adrs} · {FOLDER} · Related: {related}', '',
             intro, '', '## 1 Responsibilities and boundaries', '', holds, '', '## 2 Interfaces', '', interfaces, '',
             '## 3 Data model', '', data, '', '## 4 Behaviour and algorithms', '', behaviour, '',
             '## 5 Concurrency and performance budget', '', conc, '', '## 6 Failure modes, diagnostics and telemetry', '',
             fail, '', '## 7 Acceptance tests', '', test_head, tests_rows, '', '## 8 Open decisions', '', open_dec, '',
             '## 9 Tuning table', '', convention, '', tune_head, tune, '']
    return '\n'.join(parts)


A = lambda n, slug: f'[ADR-{n}](../../../ADR/ADR-{n}-{slug}.md)'

log = subfile(
    'Logging, checks and profiling',
    ' · '.join([A('067', 'logging'), A('065', 'crash-reporting'), A('068', 'profiling-and-gpu-timing'),
                A('057', 'version-identity'), A('074', 'platform-clock-with-injectable-time')]),
    '[platform layer](../platform_layer.md) · [frame graph and passes](../renderer/frame_graph_and_passes.md) · [build tooling and distribution](../build_tooling_and_distribution.md)',
    'This document specifies the logging system, the check levels, the black box and the fatal path, and the profiling layer with its marker catalogue and its profile view. The responsibilities of the component, the interface sketch (Listing 2.1-A), the facilities per build configuration (DBG-004) and the threading contract are in the [README](README.md).',
    'This document holds items 1 to 3 of DBG-001 ([README](README.md) §1). The crash capture that the fatal path hands over to, and the files and memory registered with Windows Error Reporting, are the [platform layer](../platform_layer.md)\'s (DBG-003).',
    'The logging, check, black-box and profiling declarations are those of Listing 2.1-A ([README](README.md) §2.1). Their threading contract is DBG-027 and DBG-028.',
    lines(H['s33'], H['s36'] - 1).rstrip('\n'),
    lines(H['s41'], H['s44'] - 1).rstrip('\n'),
    f'Diagnostics stay outside the simulation budget (DBG-200). The costs of a log call, of a logging flood and of the markers of a frame are budgets of DBG-206 ([performance baseline]({F_PERF}) §5.4).',
    'The failures of logging, checks and profiling are rows of the failure table of DBG-220 ([README](README.md) §6).',
    tests(*TLOG),
    'DEC-341 (Dropped) applies to the fatal path (DBG-073). The decisions and design-phase inputs of the whole component are in the [README](README.md) §8.',
    T_LOG)

dev = subfile(
    'Developer tools',
    ' · '.join([A('056', 'editions-build-flavours-and-feature-flags'), A('068', 'profiling-and-gpu-timing'),
                A('008', 'gpu-memory-uploads-and-device-removal'), A('065', 'crash-reporting')]),
    '[input](../input.md) · [UI framework](../ui_framework/README.md) · [entity and overlay renderers](../renderer/entity_and_overlay_renderers.md) · [camera and sequences](../camera_and_sequences.md)',
    'This document specifies the developer tools of the engine: the development overlay, the network statistics window, the debug commands and the debug-command window, development drawing with the bot overlay, the development text, and the classification of the reference cheats. The debug entry points and development bindings, the counter-provider contract and the hidden development settings are in the [README](README.md) §2.2, §2.3 and §3.2.',
    'This document holds items 4 to 7 of DBG-001 ([README](README.md) §1). Drawing the development drawing pass and the text overlays is the renderer\'s (DBG-003).',
    'The development drawing queue and the debug-command declarations are those of Listing 2.1-A ([README](README.md) §2.1); the debug actions are DBG-020, the counter providers DBG-023 and the debug-command payload DBG-024. The tools run on the presentation thread (DBG-029), and debug commands enter the simulation only through the command path (DBG-030).',
    lines(dev_text, H['s5'] - 1).rstrip('\n'),
    lines(H['s44'], H['s48'] - 1).rstrip('\n'),
    'The shown overlay page has the presentation-thread budget of DBG-206 ([performance baseline](performance_baseline.md) §5.4).',
    lines(dbg221, dbg222 - 1).rstrip('\n') + '\n\nThe failures of the developer tools are rows of the failure table of DBG-220 ([README](README.md) §6).',
    tests(*TDEV),
    'DEC-041 (Resolved), DEC-202 (Open) and DEC-208 (Open) apply to this document; their defaults are listed in the [README](README.md) §8.1, with the decisions and design-phase inputs of the whole component.',
    T_DEV)

tst = subfile(
    'Test harness, soak test and telemetry',
    ' · '.join([A('069', 'test-framework'), A('070', 'continuous-integration'), A('066', 'telemetry-with-consent'),
                A('060', 'content-identity-by-hash'), A('074', 'platform-clock-with-injectable-time')]),
    '[randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) · [build tooling and distribution](../build_tooling_and_distribution.md) · [test and telemetry records](../data_schemas/test_and_telemetry_records.md) · [platform layer](../platform_layer.md)',
    'This document specifies the test harness with its headless simulation tests and per-tick checks, the headless soak test, the opt-in telemetry data set and the replay hooks. The responsibilities of the component, the interface sketch (Listing 2.1-A) and the soak and telemetry data contracts (DBG-025, DBG-026) are in the [README](README.md).',
    'This document holds items 9 to 11 of DBG-001 ([README](README.md) §1) and the replay hooks of §8.2. The record schemas of the soak configuration, the soak report and the telemetry session record are the data schemas\' ([test and telemetry records](../data_schemas/test_and_telemetry_records.md)).',
    'The soak and telemetry declarations are those of Listing 2.1-A ([README](README.md) §2.1). The soak runner, the headless simulation tests and the telemetry sender run on threads they own (DBG-031).',
    'The soak report and the telemetry session record are the data contracts DBG-025 and DBG-026 ([README](README.md) §2.3).',
    lines(H['s48'], H['s411'] - 1).rstrip('\n'),
    'The per-tick checks exist in test builds only and stay outside the simulation budget (DBG-200).',
    'The failures of the test harness, the soak test and telemetry are rows of the failure table of DBG-220 ([README](README.md) §6).',
    tests(*TTEST),
    lines(H['s82'], H['s83'] - 1).rstrip('\n') + '\n\nThe decisions and design-phase inputs of the whole component are in the [README](README.md) §8.',
    T_TEST)

perf = subfile(
    'Performance baseline',
    ' · '.join([A('066', 'telemetry-with-consent'), A('068', 'profiling-and-gpu-timing'), A('070', 'continuous-integration')]),
    '[tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [renderer options and budgets](../renderer/options_and_budgets.md) · [build tooling and distribution](../build_tooling_and_distribution.md)',
    'This document holds the **reference performance figures** of the engine: the frame and tick structure that they assume, the population per map, the hotspot catalogue, the stress populations, the Species budget baseline and the reference test machine. They are the starting point of the Species design phase.',
    'This document holds item 12 of DBG-001 ([README](README.md) §1). Each component owns its own budgets; this document collects the baseline and the performance tests that report against it (DBG-208).',
    'This document has no interface of its own. The performance tests run in the test harness (DBG-141, [test harness, soak test and telemetry](test_harness_soak_and_telemetry.md) §4.8).',
    'The figures and budgets are the tables of §5.2 to §5.4; the reference test machine is §5.5.',
    'This document specifies no behaviour of its own: the performance tests that report against these budgets are run by the test harness (DBG-141, DBG-208).',
    lines(H['s52'], H['s6'] - 1).rstrip('\n'),
    'A result above its budget fails the nightly performance job and never changes run-time behaviour (DBG-208).',
    tests(*TPERF),
    'DEC-106, DEC-227, DEC-228 and DEC-229 (Open) apply to this document; their defaults are listed in the [README](README.md) §8.1, with the design-phase inputs of the whole component.',
    T_PERF)

os.makedirs(OUT, exist_ok=True)
for name, text in [(README, readme), (F_LOG, log), (F_DEV, dev), (F_TEST, tst), (F_PERF, perf)]:
    text = re.sub(r'\n{3,}', '\n\n', text)
    open(os.path.join(OUT, name), 'w', encoding='utf-8').write(text)
    print(f'{len(text.encode()):7} {name}')
