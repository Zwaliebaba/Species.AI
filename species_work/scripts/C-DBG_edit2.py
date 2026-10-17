"""C-DBG edit pass 2: compact the listing, header and notes. Usage: python3 -I C-DBG_edit2.py <file> <listing>"""
import re, sys
p, lp = sys.argv[1], sys.argv[2]
t = open(p, encoding='utf-8').read()
lst = open(lp, encoding='utf-8').read()
a = t.index('### 2.1 Interface sketch')
b = t.index('### 2.2 Debug entry points')
t = t[:a] + lst + t[b:]
# header: shorter Related list
lines = t.split('\n')
h = lines[2]
i = h.index(' · Related: ')
lines[2] = h[:i] + ' · Related: [tick timing and main loop](../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [randomness and integrity](../../reference/12_simulation_model/07_randomness_and_integrity.md) · [platform layer](platform_layer.md) · [build tooling and distribution](build_tooling_and_distribution.md) · [renderer options and budgets](renderer/options_and_budgets.md)'
t = '\n'.join(lines)
# tuning notes
t = t.replace('; reference |', ' |')
t = re.sub(r'(?m)^(\| `dbg\.[^|]*\|[^|]*\|[^|]*\|) reference (\|)', r'\1 \2', t)
t = t.replace('Every value is a Species engine-baseline value; a value marked "reference" is the reference tool\'s and is kept (DEC-004).',
              'A value whose notes say "Species" is a Species engine-baseline value; every other value is the reference tool\'s value, kept as the starting value (DEC-004).')
for k in ['dbg.log.thread_buffer_kib', 'dbg.blackbox.block_bytes', 'dbg.blackbox.recent_markers', 'dbg.blackbox.recent_checks',
          'dbg.profile.gpu_readback_latency', 'dbg.overlay.warning_rgb', 'dbg.telemetry.queue_max_records', 'dbg.budget.log_call_us',
          'dbg.budget.log_flood_frame_ms', 'dbg.budget.markers_frame_ms', 'dbg.budget.overlay_ms', 'dbg.log.files_kept', 'dbg.telemetry.percentiles']:
    m = re.search(r'(?m)^\| `' + re.escape(k) + r'` \|([^|]*)\|([^|]*)\|([^|]*)\|', t)
    if not m:
        print('miss', k); continue
    note = m.group(3).strip()
    new = ('Species; ' + note) if note else 'Species'
    t = t[:m.start(3)] + ' ' + new + ' ' + t[m.end(3):]
t = t.replace('\n### 6.2 Debug commands and simulation state\n', '\n')
t = t.replace('| ADR-0065 to ADR-0070 | Accepted or Proposed | crash reporting, telemetry, logging, profiling, test framework, CI as cited |\n', '')
open(p, 'w', encoding='utf-8').write(t)
print(len(t.encode('utf-8')))
