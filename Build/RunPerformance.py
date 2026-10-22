"""RunPerformance.py: the MVP's performance job (DBG-208, DBG-209; M1 plan §4, ADR-110), which the owner runs on the
reference test machine.

  python Build/RunPerformance.py [--skip-build] [--tool <SpeciesPerformance.exe>] [--bench <SpeciesFrameBench.exe>]
                                 [--headless-only] [--record <file>] [--quick]
  python Build/RunPerformance.py --self-test --tool <SpeciesPerformance.exe> [--bench <SpeciesFrameBench.exe> --warp]

--tuning-source <GameData> makes the tool read the authored tuning record instead of the cooked one beside it, for a
tool built without its content folder.

Builds Release|ARM64 through Species.slnx (AGENTS.md §3), runs the headless measurement scenes of SpeciesPerformance,
each from a fixed seed with a recorded command stream, then the rendered scenes of SpeciesFrameBench (M1 plan T18,
ADR-126) unless --headless-only is given, reads the machine's configuration (processor, memory size, graphics adapter,
driver version, display mode and OS build) and writes the configuration, the build's identity and each figure against
its budget into the performance section of playtests/M1.md.

The first run pins the machine, and the first run of the rendered scenes pins the display they drew on, as the bench
reports its primary display (O75); that display must run at 60 Hz with the battle drawn at 1,920 x 1,080, borderless on
a display of that size or in a window of that size on a larger one (O85), and a first rendered run that drew otherwise
stops and records nothing. A later run whose processor, memory or graphics adapter differs from the pinned ones, or whose
display differs for the rendered scenes, stops and records nothing, because a change of the hardware re-baselines every
result (DBG-209); the driver and the OS build are recorded with each run and stop nothing. Once pinned, a figure above
its budget fails the run (exit code 1); the run that pins only reports. The sandbox's battle drawn passes when the 99th
percentile of its frame interval, from one frame's start to the next (ADR-148), is at most 1000 ÷ mvp.client.fps_target
+ 1 ms (O75); its missed frames, its GPU time, its waits, the refreshes that showed no new frame and its longest
frames, the server's tick rate while drawn and the scene of mvp.client.figures_max figures are reported, as are a
valley's generation and its land route graph, the route sweep and the log flood's detail (ADR-091, ADR-102, ADR-104).
From M2 task T10 the growth scene of each seed is reported against M2's exit, with the snapshot of its settlement, and
the settlement at work and the region with four scripted settlements are judged against the Worker's and the region's
budgets (ADR-138); M2's run is recorded with --record playtests/M2.md. --quick runs every scene briefly to show that it
runs, and records nothing. --self-test is CI's: it runs the given tool with --quick, and the bench too when one is
given (--warp draws it on the WARP adapter), then proves the record's rules on fixtures in a temporary folder, and
writes nothing into the checkout.
"""
import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOLUTION = os.path.join(ROOT, 'Species.slnx')
RECORD = os.path.join(ROOT, 'playtests', 'M1.md')
CONFIGURATION = 'Release'
PLATFORM = 'ARM64'  # O69, ADR-094: the reference test machine measures the ARM64 development build natively

MACHINE_ITEMS = [
  ('processor', 'Processor'),
  ('memory', 'Memory'),
  ('adapter', 'Graphics adapter'),
  ('driver', 'Driver version'),
  ('display', 'Display mode'),
  ('os_build', 'OS build'),
]
# O75: only the hardware stops a run; the driver and the OS build are recorded with each one.
HARDWARE_ITEMS = ('processor', 'memory', 'adapter')
PIN_MARK = re.compile(r'<!-- reference-machine (\{.*?\}) -->')
DISPLAY_MARK = re.compile(r'<!-- reference-display (\{.*?\}) -->')  # O75: the rendered scenes' own pin
FRAME_TOLERANCE_MS = 1.0  # O75: 60 frames per second is a 99th-percentile frame interval of 16.7 ms and 1 ms of tolerance
RENDERED_SIZE = (1920, 1080)  # MVP-085
RENDERED_REFRESH_HZ = 60  # O75: the rendered scenes run on a 60 Hz display, at 1,920 x 1,080 (O85)
SECTION = '## Performance'

# ADR-080 item 2's cost budgets, item 3's region step budget and item 7's integrated server budget, and the travel
# factor of WLD-100 (O71). When ADR-080 is amended with measured figures, this table follows it.
BUDGETS = {
  'battle_figure_us': (2.1, 'µs per figure per tick', 'figure in a battle (search, combat movement, laser bolts, Essence orbs)'),
  'work_figure_us': (1.2, 'µs per figure per tick', 'Worker at settlement work (the at-work scene is M1\'s wandering stand-in)'),
  'expedition_figure_us': (0.4, 'µs per figure per tick', 'figure of an expedition outside a battle (the travel scene)'),
  'draw_us': (0.03, 'µs per draw', 'a keyed simulation draw'),
  'ai_us': (60.0, 'µs per AI per tick', 'AI settlement or deputy'),
  'side_command_us': (30.0, 'µs per side per tick', 'command of one side of a battle'),
  'region_fixed_us': (30.0, 'µs per tick', 'region fixed cost (an empty region with its terrain)'),
  'region_step_ms': (12.5, 'ms per tick, 95th percentile', 'region step at its bound'),
  'integrated_cores': (1.0, 'reference cores', 'integrated server (the sandbox battle\'s mean step over the tick period)'),
  # DBG-206's dbg.budget.log_call_us, measured as DBG T2 states it (ADR-091).
  'log_call_us': (1.0, 'µs per call', 'a log call on the calling thread while 8 threads log 10⁶ debug records'),
}
BATTLE_MARKERS = ['sim.teams.advance_ungrouped', 'sim.laser_bolts.advance', 'sim.effects.advance', 'sim.essence_orbs.advance']
SWEEP_MARKERS = ['sim.routes.free_unheld']  # ADR-104, DEC-651: every step frees the routes that no figure holds
# WLD-165's budgets for a world's generation, wld.generation.budget_16_s and budget_200_s, with the regions of those
# worlds (WLD-004, table 3): (seconds, steward capacity, regions). A valley's generation is reported against the smaller
# share per region; the bake that WLD-165 includes is not built yet.
GENERATION_BUDGETS = ((20.0, 16, 8), (120.0, 200, 77))


def marker_us(scene, name):
  return scene['markers'].get(name, {}).get('microseconds', 0.0)


def per_figure_tick(scene, names):
  figure_ticks = scene['ticks'] * scene['mean_figures']
  return sum(marker_us(scene, n) for n in names) / figure_ticks if figure_ticks else None


def figures_of(result):
  """Each measured figure: (scene, key, value). A figure whose scene did not run is left out."""
  scenes = {s['name']: s for s in result['scenes']}
  out = []
  for name in ('sandbox-battle', 'synthetic-battle'):
    if name in scenes:
      s = scenes[name]
      out.append((name, 'battle_figure_us', per_figure_tick(s, BATTLE_MARKERS)))
      if s['ais'] and s['ticks']:
        out.append((name, 'ai_us', marker_us(s, 'sim.ai.step') / (s['ticks'] * s['ais'])))
      if s['sides'] and s['ticks']:
        out.append((name, 'side_command_us', marker_us(s, 'sim.region.commands') / (s['ticks'] * s['sides'])))
  if 'sandbox-battle' in scenes:
    s = scenes['sandbox-battle']
    out.append(('sandbox-battle', 'integrated_cores', s['step_us']['mean'] / 1e6 / result['tick_period_s']))
  if 'at-work' in scenes:
    out.append(('at-work', 'work_figure_us', per_figure_tick(scenes['at-work'], ['sim.teams.advance_ungrouped'])))
  if 'settlement-at-work' in scenes:
    # M2 task T10 (ADR-138): the Worker at work in a settlement at its plateau, in place of M1's wandering stand-in.
    out.append(('settlement-at-work', 'work_figure_us', per_figure_tick(scenes['settlement-at-work'], ['sim.teams.advance_ungrouped'])))
  if 'empty-region' in scenes:
    out.append(('empty-region', 'region_fixed_us', scenes['empty-region']['step_us']['mean']))
  for name in ('region-bound', 'region-bound-settled'):
    if name in scenes:
      out.append((name, 'region_step_ms', scenes[name]['step_us']['p95'] / 1000.0))
  edges = result.get('travel') or []
  figure_ticks = sum(e['figure_ticks'] for e in edges)
  if figure_ticks:
    out.append(('travel', 'expedition_figure_us', sum(e['advance_us'] for e in edges) / figure_ticks))
  if result.get('draw_ns') is not None:
    out.append(('draws', 'draw_us', result['draw_ns'] / 1000.0))
  if result.get('log'):
    out.append(('log-flood', 'log_call_us', result['log']['call_ns_mean'] / 1000.0))
  return [f for f in out if f[2] is not None]


def reported_rows(result):
  """The headless figures that are reported rather than judged (scene, figure, measured, budget, result): a valley's
  generation (WLD-165, ADR-102), its land route graph and the route sweep (ADR-104), and the log flood's detail
  (ADR-091). A result from an older tool has none of them."""
  rows = []
  samples = result.get('generation') or []
  if samples:
    generate = [s['generate_ms'] / 1000.0 for s in samples]
    graph = [s['graph_ms'] for s in samples]
    share = min(seconds / regions for seconds, _, regions in GENERATION_BUDGETS)
    shares = ' and '.join(f'{seconds:g} s for {regions} regions at {capacity}' for seconds, capacity, regions in GENERATION_BUDGETS)
    retried = sum(1 for s in samples if s['passed_seed'] != s['seed'] or s['attempt'] > 0)
    rows += [
      ('generation', f'a valley generated with every attempt, check and re-roll, worst and mean of {len(samples)} seeds (WLD-165; '
       f'the share per region of {shares}; no bake yet)', f'{max(generate):.3f} s, {sum(generate) / len(generate):.3f} s',
       f'{share:.2f} s', 'reported: within the share' if max(generate) <= share else 'reported: above the share'),
      ('generation', 'valleys that passed only at a later attempt or seed (MVP-011)', f'{retried} of {len(samples)}', '—', 'reported'),
      ('generation', f'the land route graph of a valley alone, worst and mean (ADR-104), at most {max(s["nodes"] for s in samples)} '
       'nodes', f'{max(graph):.1f} ms, {sum(graph) / len(graph):.1f} ms', '—', 'reported'),
    ]
  scenes = {s['name']: s for s in result['scenes']}
  for name in ('sandbox-battle', 'region-bound'):
    if name in scenes:
      sweep = per_figure_tick(scenes[name], SWEEP_MARKERS)
      if sweep is not None:
        rows.append((name, 'route sweep, the routes no figure holds freed (ADR-104, DEC-651)', f'{sweep:.4g} µs per figure per tick',
                     '—', 'reported'))
  # M2 task T10 (ADR-138): the growth scene of each seed against M2's exit, and the snapshot of a settlement at its end.
  for run in result.get('growth') or []:
    plateau = 'none' if run['plateau_h'] is None else f'{run["plateau_h"]:.2f} h'
    halfway = 'none' if run['halfway_h'] is None else f'{run["halfway_h"]:.2f} h'
    met = 'gap met' if run['gap_met'] else 'gap above 20 min'
    met += ', plateau in hours 4 to 6' if run['plateau_met'] else ', plateau outside hours 4 to 6 (O84: it yields first)'
    rows.append(('growth', f'seed {run["seed"]}: the largest gap between growth steps up to the plateau or hour 5, the plateau at '
                 f'{run["largest_workers"]} Workers, and the halfway line reached (M3\'s contact, hour 2 to 3)',
                 f'{run["gap_minutes"]:.1f} min from {run["gap_from_h"]:.2f} h; {plateau}; {halfway}', '20 min; 4 to 6 h; 2 to 3 h',
                 f'reported: {met}'))
  snapshot = result.get('snapshot')
  if snapshot:
    rows.append(('growth', 'a whole-world snapshot of the first seed\'s settlement at the run\'s end, encoded and saved (ADR-137)',
                 f'{snapshot["bytes"]} bytes; {snapshot["encode_ms"]:.3f} ms, {snapshot["save_ms"]:.3f} ms', '—', 'reported'))
  flood = result.get('log')
  if flood:
    rows.append(('log-flood', 'the slowest thread\'s log call, and the records written and dropped (DBG T2)',
                 f'{flood["call_ns_slowest_thread"] / 1000.0:.4g} µs per call; {flood["written"]} written, '
                 f'{flood["dropped"]} dropped of {flood["records"]}', '—', 'reported'))
  return rows


def status_of(value, budget):
  if value <= budget:
    return 'within budget'
  return 'within 25 %' if value <= budget * 1.25 else 'above budget'


def travel_factor(result):
  """WLD-100 (O71): the largest per-edge median of walking time × speed ÷ straight distance, over the corpus."""
  medians = [e['factor_median'] for e in result.get('travel') or [] if e['arrived']]
  return max(medians) if medians else None


def run_tool(tool, quick, out, tuning_source=None):
  args = [tool, '--out', out] + (['--quick'] if quick else []) + (['--tuning-source', tuning_source] if tuning_source else [])
  completed = subprocess.run(args, capture_output=True, text=True)
  if completed.returncode != 0:
    raise SystemExit(f'RunPerformance: {os.path.basename(tool)} failed:\n{completed.stdout}{completed.stderr}')
  with open(out, encoding='utf-8') as f:
    return json.load(f)


def powershell(command):
  completed = subprocess.run(['powershell', '-NoProfile', '-Command', command], capture_output=True, text=True)
  if completed.returncode != 0:
    raise SystemExit(f'RunPerformance: the machine query failed: {completed.stderr.strip()}')
  return completed.stdout.strip()


def machine():
  """The configuration DBG-209 pins, read through CIM and the registry."""
  query = (
    "$p = Get-CimInstance Win32_Processor | Select-Object -First 1; "
    "$c = Get-CimInstance Win32_ComputerSystem; "
    "$v = Get-CimInstance Win32_VideoController | Where-Object { $_.CurrentHorizontalResolution } | Select-Object -First 1; "
    "$o = Get-CimInstance Win32_OperatingSystem; "
    "$u = (Get-ItemProperty 'HKLM:\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion').UBR; "
    "[pscustomobject]@{ processor = $p.Name.Trim(); memory = ('{0} GB' -f [math]::Round($c.TotalPhysicalMemory / 1GB)); "
    "adapter = $v.Name; driver = $v.DriverVersion; "
    "display = ('{0} x {1} at {2} Hz' -f $v.CurrentHorizontalResolution, $v.CurrentVerticalResolution, $v.CurrentRefreshRate); "
    "os_build = ('{0}.{1}' -f $o.Version, $u) } | ConvertTo-Json -Compress")
  return json.loads(powershell(query))


def identity():
  """The build's identity as the playtest README defines it until ADR-057's exists: the commit, marked if modified."""
  commit = subprocess.run(['git', '-C', ROOT, 'rev-parse', '--short=12', 'HEAD'], capture_output=True, text=True).stdout.strip()
  dirty = subprocess.run(['git', '-C', ROOT, 'status', '--porcelain'], capture_output=True, text=True).stdout.strip()
  return commit + ('+modified' if dirty else '')


def find_msbuild():
  vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'), 'Microsoft Visual Studio', 'Installer',
                         'vswhere.exe')
  if os.path.exists(vswhere):
    found = subprocess.run([vswhere, '-latest', '-products', '*', '-find', r'MSBuild\**\Bin\MSBuild.exe'], capture_output=True,
                           text=True).stdout.splitlines()
    if found:
      return found[0]
  return shutil.which('msbuild') or sys.exit('RunPerformance: MSBuild not found; run from a Developer PowerShell')


def build():
  msbuild = find_msbuild()
  for args in ([SOLUTION, '/t:Restore', '/p:RestorePackagesConfig=true', '/nologo'],
               [SOLUTION, f'/p:Configuration={CONFIGURATION}', f'/p:Platform={PLATFORM}', '/m', '/v:minimal', '/nologo']):
    if subprocess.run([msbuild] + args).returncode != 0:
      sys.exit('RunPerformance: the build failed')


def shown(path):
  """A path as the script prints it: relative to the repository when it is inside it, else whole. On Windows a path on
  another drive, a temporary folder on C: beside a checkout on D:, has no relative form at all."""
  try:
    relative = os.path.relpath(path, ROOT)
  except ValueError:
    return path
  return path if relative.startswith('..') else relative


def pinned_machine(text):
  m = PIN_MARK.search(text)
  return json.loads(m.group(1)) if m else None


def machine_table(config):
  rows = '\n'.join(f'| {label} | {config.get(key, "")} |' for key, label in MACHINE_ITEMS)
  return f'| Item | Value |\n|---|---|\n{rows}\n'


# ADR-148: the longest frames the record lists with where their time went.
LONGEST_REPORTED = 3


def frame_budget_ms(frames):
  """O75: the sandbox's battle drawn passes at a 99th-percentile frame interval of 1000 ÷ mvp.client.fps_target + 1 ms."""
  return 1000.0 / max(frames['fps_target'], 1) + FRAME_TOLERANCE_MS


def rendered_rows(frames, tick_period_s=None):
  """The rendered scenes' rows (scene, figure, measured, budget, result), and whether a judged figure is above its budget.
  Only the sandbox's battle at 1,920 x 1,080 is judged; the rest is reported (O75). The server's tick rate while drawn
  is the ticks measured over the frames' intervals, against the tick period of the headless result when it is given."""
  budget = frame_budget_ms(frames)
  rows = []
  above = False
  for scene in frames['scenes']:
    name = scene['name']
    size = (scene['window']['width'], scene['window']['height'])
    p99 = scene['interval_ms']['p99']
    if name == 'sandbox-drawn' and size == RENDERED_SIZE:
      status = 'within budget' if p99 <= budget else 'above budget'
      above = above or p99 > budget
    elif name == 'sandbox-drawn':
      status = f'not judged: drawn at {size[0]} x {size[1]}, not 1,920 x 1,080 (MVP-085)'
    else:
      status = 'reported (O75)'
    count = max(scene['frames'], 1)
    rows += [
      (name, 'frame interval, 99th percentile (MVP-085)', f'{p99:.2f} ms', f'{budget:.1f} ms', status),
      (name, 'frame interval, median and longest', f'{scene["interval_ms"]["p50"]:.2f} ms, {scene["interval_ms"]["max"]:.1f} ms', '—',
       'reported'),
      (name, "frame's work (its interval less the renderer's waits), 99th percentile", f'{scene["work_ms"]["p99"]:.2f} ms', '—',
       'reported'),
      (name, 'missed frames (O75)', f'{scene["missed_frames"]} of {scene["frames"]} ({scene["missed_frames"] / count:.2%})', '—',
       'reported'),
    ] + pacing_rows(scene) + [
      (name, 'figures alive and on screen', f'{scene["figures_alive"]["max"]} alive at most; {scene["figures_on_screen"]["mean"]:.0f} '
       f'on screen on average, {scene["figures_on_screen"]["max"]} at most', '—', 'reported'),
    ]
    seconds = scene['interval_ms']['mean'] * scene['frames'] / 1000.0
    if seconds > 0:
      rows.append((name, 'server ticks per second while drawn (MVP-085)', f'{scene["ticks"] / seconds:.2f}',
                   f'{1.0 / tick_period_s:g}' if tick_period_s else '—', 'reported'))
  return rows, above


def pacing_rows(scene):
  """ADR-148's rows of a rendered scene, which say where a long frame's time went: its time on the GPU, the waits at the
  99th percentile, the refreshes on which the display showed no new frame by the swap chain's own statistics, and the
  longest frames apart. A result from an older bench has none of them."""
  name = scene['name']
  rows = []
  if 'gpu_ms' in scene:
    gpu = scene['gpu_ms']
    measured = f'{gpu["p99"]:.2f} ms, {gpu["max"]:.1f} ms' if scene.get('gpu_frames') else 'not read: no timestamp queries'
    rows.append((name, "frame's time on the GPU, 99th percentile and longest", measured, '—', 'reported'))
  if 'latency_wait_ms' in scene:
    rows.append((name, 'waits at the 99th percentile: the frame-latency object and the cap, the fence, the present',
                 f'{scene["latency_wait_ms"]["p99"]:.2f} ms, {scene["fence_wait_ms"]["p99"]:.2f} ms, {scene["present_ms"]["p99"]:.2f} ms',
                 '—', 'reported'))
  display = scene.get('display')
  if display is not None:
    refreshes = display['refreshes']
    measured = (f'{display["held_refreshes"]} of {refreshes} ({display["held_refreshes"] / refreshes:.2%})'
                if display['presents_shown'] and refreshes else 'no statistics from the swap chain')
    rows.append((name, "refreshes that showed no new frame (the swap chain's frame statistics)", measured, '—', 'reported'))
  longest = scene.get('longest') or []
  if longest:
    def gpu_of(frame):
      return 'GPU —' if frame['gpu'] is None else f'GPU {frame["gpu"]:.1f}'
    parts = [f'{f["interval"]:.1f} = work {f["work"]:.1f} + latency {f["latency_wait"]:.1f} + fence {f["fence_wait"]:.1f} + '
             f'present {f["present"]:.1f}, {gpu_of(f)}' for f in longest[:LONGEST_REPORTED]]
    rows.append((name, f'the {len(parts)} longest frames, in ms', '; '.join(parts), '—', 'reported'))
  return rows


def rendered_display(frames, config):
  """The display the rendered scenes drew on, as the bench reports its primary display's mode, in the form the machine's
  reading takes. A result from an older bench reports none, and the machine's reading stands in."""
  display = frames.get('display')
  if not display:
    return config.get('display')
  return f'{display["width"]} x {display["height"]} at {display["refresh_hz"]} Hz'


def drawn_at_target(frames, drawn_on):
  """Whether the rendered scenes may pin the display they drew on: a 60 Hz display on which the sandbox's battle was
  drawn at 1,920 x 1,080, borderless on a display of that size (O75) or in a window of that size on a larger one (O85)."""
  sizes = [(s['window']['width'], s['window']['height']) for s in frames['scenes'] if s['name'] == 'sandbox-drawn']
  return (drawn_on or '').endswith(f' at {RENDERED_REFRESH_HZ} Hz') and sizes == [RENDERED_SIZE]


def run_section(when, build_id, config, result, frames):
  """The run's section, and whether a headless and a rendered figure is above its budget."""
  drawn_on = f', the rendered scenes on {rendered_display(frames, config)}' if frames is not None else ''
  lines = [f'### Run {when}, build {build_id}, {result["configuration"]}|{result["platform"]}', '',
           f'Driver {config.get("driver", "")}, OS build {config.get("os_build", "")}, display {config.get("display", "")}{drawn_on} '
           '(O75).', '',
           '| Scene | Figure | Measured | Budget | Result |', '|---|---|---|---|---|']
  failed = False
  for scene, key, value in figures_of(result):
    budget, unit, label = BUDGETS[key]
    status = status_of(value, budget)
    failed = failed or status == 'above budget'
    lines.append(f'| {scene} | {label} | {value:.4g} {unit} | {budget:g} | {status} |')
  factor = travel_factor(result)
  current_factor = result['valley_path_factor']
  if factor is not None:
    lines.append(f'| travel | `wld.travel.valley_path_factor` (WLD-100): the largest per-edge median of walking time × speed ÷ '
                 f'straight distance | {factor:.4f} | {current_factor:g} in the record | '
                 f'{"within the record" if factor <= current_factor else "above the record"} |')
  lines += [f'| {scene} | {figure} | {measured} | {budget} | {status} |'
            for scene, figure, measured, budget, status in reported_rows(result)]
  rendered_above = False
  if frames is not None:
    rows, rendered_above = rendered_rows(frames, result.get('tick_period_s'))
    lines += [f'| {scene} | {figure} | {measured} | {budget} | {status} |' for scene, figure, measured, budget, status in rows]
  return '\n'.join(lines) + '\n', failed, rendered_above


def pinned_display(text):
  m = DISPLAY_MARK.search(text)
  return json.loads(m.group(1)) if m else None


def record(path, config, result, frames, build_id, when):
  """Writes the run into the record's performance section; returns its exit code (0, 1 above budget, 2 hardware changed).
  _frames is the bench's result, or None for a headless run."""
  milestone = os.path.splitext(os.path.basename(path))[0]
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else (
    f'# {milestone}\n\nThe record of milestone {milestone} (README, "A milestone record"). Its exit criteria are added when '
    'the milestone ends.\n')
  if SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{SECTION}\n\nWritten by `Build/RunPerformance.py` (M1 plan §4, DBG-208, DBG-209, ADR-110): '
                                'the reference test machine, pinned by the first run, and each run with every figure '
                                'against its budget.\n')
  pinned = pinned_machine(text)
  if pinned is not None:
    # O75: only the hardware stops a run.
    changed = [label for key, label in MACHINE_ITEMS if key in HARDWARE_ITEMS and pinned.get(key) != config.get(key)]
    if changed:
      print(f'RunPerformance: the machine differs from the pinned one in: {", ".join(changed)}. A change of the hardware '
            're-baselines every result (DBG-209); nothing was recorded.')
      return 2
  display = pinned_display(text)
  drawn_on = rendered_display(frames, config) if frames is not None else None
  if frames is not None and display is not None and display.get('display') != drawn_on:
    print(f'RunPerformance: the rendered scenes drew on {drawn_on}, and they pinned {display.get("display")} (O75). '
          'A change of the display re-baselines the rendered results (DBG-209); nothing was recorded.')
    return 2
  if frames is not None and display is None and not drawn_at_target(frames, drawn_on):
    # The first rendered run pins the display, so a run that did not draw the battle as MVP-085 asks would pin the wrong one.
    print(f'RunPerformance: the rendered scenes drew on {drawn_on}, and their first run pins the display, which must run at '
          f'{RENDERED_REFRESH_HZ} Hz with the battle drawn at {RENDERED_SIZE[0]} x {RENDERED_SIZE[1]} (O75, O85, MVP-085). '
          'Make a 60 Hz display of at least that size the primary display and run again, or run with --headless-only; '
          'nothing was recorded.')
    return 2
  if pinned is None:
    text = text.rstrip('\n') + (f'\n\n### Reference test machine\n\nPinned by the run of {when}, build {build_id} (DBG-209).\n\n'
                                f'{machine_table(config)}\n<!-- reference-machine {json.dumps(config, sort_keys=True)} -->\n')
  if frames is not None and display is None:
    pin = {'display': drawn_on}
    text = text.rstrip('\n') + (f'\n\n### Reference display\n\nPinned for the rendered scenes by the run of {when}, build '
                                f'{build_id} (O75, DBG-209): {drawn_on}.\n\n'
                                f'<!-- reference-display {json.dumps(pin, sort_keys=True)} -->\n')
  section, above, rendered_above = run_section(when, build_id, config, result, frames)
  text = text.rstrip('\n') + '\n\n' + section
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: recorded in {shown(path)}' + ('; the machine is pinned by this run' if pinned is None else '') +
        ('; the display is pinned by this run' if frames is not None and display is None else ''))
  # The run that pins only reports; after it, a figure above its budget fails the run (DBG-208).
  return 1 if (pinned is not None and above) or (display is not None and rendered_above) else 0


def run_bench(bench, quick, out, warp=False):
  args = [bench, '--out', out] + (['--quick'] if quick else []) + (['--warp'] if warp else [])
  completed = subprocess.run(args, capture_output=True, text=True)
  if completed.returncode != 0:
    raise SystemExit(f'RunPerformance: {os.path.basename(bench)} failed (exit code {completed.returncode}):\n'
                     f'{completed.stdout}{completed.stderr}')
  with open(out, encoding='utf-8') as f:
    return json.load(f)


def frames_like(frames, p99, size=RENDERED_SIZE, display=(*RENDERED_SIZE, RENDERED_REFRESH_HZ)):
  """A copy of the bench's result with every scene's 99th-percentile interval and window, and the display it drew on,
  set: the self-test's fixtures."""
  return dict(frames, display={'width': display[0], 'height': display[1], 'refresh_hz': display[2]},
              scenes=[dict(scene, interval_ms=dict(scene['interval_ms'], p99=p99), window={'width': size[0], 'height': size[1]})
                      for scene in frames['scenes']])


# The bench's result when the self-test runs none: the shape SpeciesFrameBench writes, its figures made up.
FRAMES_FIXTURE = {
  'tool': 'SpeciesFrameBench', 'configuration': 'Debug', 'platform': 'x64', 'quick': True, 'fps_target': 60, 'figures_max': 1500,
  'display': {'width': 1920, 'height': 1080, 'refresh_hz': 60},
  'scenes': [{'name': name, 'seed': 1, 'figures': figures, 'ticks': 20, 'frames': 120, 'suspended_frames': 0,
              'window': {'width': 1920, 'height': 1080},
              'interval_ms': {'p50': 16.6, 'p95': 16.7, 'p99': 16.8, 'max': 33.4, 'mean': 16.7},
              'work_ms': {'p50': 6.0, 'p95': 7.0, 'p99': 8.0, 'max': 12.0, 'mean': 6.2}, 'missed_frames': 1,
              'gpu_frames': 118, 'gpu_ms': {'p50': 9.0, 'p95': 10.0, 'p99': 11.0, 'max': 14.0, 'mean': 9.2},
              'latency_wait_ms': {'p50': 9.0, 'p95': 10.0, 'p99': 10.5, 'max': 26.0, 'mean': 9.1},
              'fence_wait_ms': {'p50': 0.0, 'p95': 0.1, 'p99': 0.2, 'max': 0.5, 'mean': 0.0},
              'present_ms': {'p50': 0.2, 'p95': 0.3, 'p99': 0.4, 'max': 0.9, 'mean': 0.2},
              'display': {'presents_shown': 118, 'refreshes': 119, 'held_refreshes': 1},
              'longest': [{'interval': 33.4, 'work': 6.5, 'latency_wait': 26.0, 'fence_wait': 0.5, 'present': 0.4, 'gpu': 14.0},
                          {'interval': 16.9, 'work': 7.0, 'latency_wait': 9.5, 'fence_wait': 0.1, 'present': 0.3, 'gpu': None}],
              'figures_alive': {'min': figures, 'max': figures}, 'figures_on_screen': {'mean': figures * 0.8, 'max': figures}}
             for name, figures in (('sandbox-drawn', 400), ('figures-max-drawn', 1500))],
}


def self_test(tool, tuning_source, bench=None, warp=False):
  with tempfile.TemporaryDirectory() as folder:
    result = run_tool(tool, True, os.path.join(folder, 'quick.json'), tuning_source)
    figures = figures_of(result)
    keys = {key for _, key, _ in figures}
    missing = set(BUDGETS) - keys
    if missing:
      sys.exit(f'RunPerformance: the quick run gives no figure for {sorted(missing)}')
    frames = FRAMES_FIXTURE
    if bench:
      frames = run_bench(bench, True, os.path.join(folder, 'frames.json'), warp)
      drawn = {scene['name']: scene['frames'] for scene in frames['scenes']}
      if sorted(drawn) != ['figures-max-drawn', 'sandbox-drawn'] or min(drawn.values()) == 0:
        sys.exit(f'RunPerformance: the bench\'s quick run drew {drawn}')
      print(f'RunPerformance: the bench\'s quick run drew {drawn}')
    path = os.path.join(folder, 'M1.md')
    reference = {key: f'fixture {key}' for key, _ in MACHINE_ITEMS}
    within = dict(result, scenes=[dict(s, step_us=dict(s['step_us'])) for s in result['scenes']])
    for s in within['scenes']:
      s['markers'] = {}
      s['step_us'] = {'mean': 1.0, 'p50': 1.0, 'p95': 1.0, 'max': 1.0}
    for e in within['travel']:
      e['advance_us'] = 0.0
    within['draw_ns'] = 1.0
    within['log'] = dict(within['log'], call_ns_mean=100.0)
    above = dict(within, draw_ns=1000.0)
    smooth = frames_like(frames, 16.0)
    slow = frames_like(frames, 25.0)

    def recorded(*phrases):
      text = open(path, encoding='utf-8').read()
      return all(phrase in text for phrase in phrases)

    # O85: a record of its own, whose first rendered run draws a 1,920 x 1,080 window on a larger 60 Hz panel.
    panel = os.path.join(folder, 'M1-panel.md')

    checks = [
      ('the first run pins the machine and only reports', lambda: record(path, reference, result, None, 'fixture', 'pin') == 0 and
       pinned_machine(open(path, encoding='utf-8').read()) == reference),
      ('a run on another processor stops', lambda: record(path, dict(reference, processor='other'), result, None, 'fixture', 'moved') == 2),
      ('a run with another driver and OS build is recorded (O75)',
       lambda: record(path, dict(reference, driver='other', os_build='other'), within, None, 'fixture', 'driver') == 0),
      ('a pinned run within every budget passes', lambda: record(path, reference, within, None, 'fixture', 'within') == 0),
      ('a pinned run above a budget fails', lambda: record(path, reference, above, None, 'fixture', 'above') == 1),
      ('a log call above its budget fails',
       lambda: record(path, reference, dict(within, log=dict(within['log'], call_ns_mean=5000.0)), None, 'fixture', 'log') == 1),
      ('a run reports the generation, the route graph, the route sweep and the log flood',
       lambda: recorded('a valley generated', 'the land route graph', 'route sweep', 'the slowest thread\'s log call')),
      ('a first rendered run drawn at another size pins nothing',
       lambda: record(path, reference, within, frames_like(frames, 16.0, (2880, 1920), (2880, 1920, 60)), 'fixture', 'panel') == 2 and
       pinned_display(open(path, encoding='utf-8').read()) is None),
      ('a first rendered run on a display not at 60 Hz pins nothing',
       lambda: record(path, reference, within, frames_like(frames, 16.0, display=(1920, 1080, 144)), 'fixture', 'fast') == 2 and
       pinned_display(open(path, encoding='utf-8').read()) is None),
      ('a first rendered run in a 1,920 x 1,080 window on a larger 60 Hz display pins it (O85)',
       lambda: record(panel, reference, within, frames_like(frames, 16.0, display=(2880, 1920, 60)), 'fixture', 'window') == 0 and
       pinned_display(open(panel, encoding='utf-8').read()) == {'display': '2880 x 1920 at 60 Hz'}),
      ('the first rendered run pins the display and only reports',
       lambda: record(path, reference, within, slow, 'fixture', 'display') == 0 and
       pinned_display(open(path, encoding='utf-8').read()) == {'display': '1920 x 1080 at 60 Hz'}),
      ('a rendered run reports the server\'s tick rate', lambda: recorded('server ticks per second while drawn')),
      ('a rendered run reports where its frames\' time went (ADR-148)',
       lambda: recorded("frame's time on the GPU", 'waits at the 99th percentile', 'showed no new frame', 'longest frames')),
      ('a rendered run on another display stops',
       lambda: record(path, reference, within, frames_like(frames, 16.0, display=(1920, 1080, 144)), 'fixture', 'other display') == 2),
      ('a rendered run is judged by the display the bench drew on, not the machine\'s reading',
       lambda: record(path, dict(reference, display='other'), within, smooth, 'fixture', 'reading') == 0),
      ('a headless run on another display is recorded',
       lambda: record(path, dict(reference, display='other'), within, None, 'fixture', 'headless') == 0),
      ('a rendered run within its frame budget passes', lambda: record(path, reference, within, smooth, 'fixture', 'smooth') == 0),
      ('a rendered run above its frame budget fails', lambda: record(path, reference, within, slow, 'fixture', 'slow') == 1),
      ('a rendered run at another size is not judged',
       lambda: record(path, reference, within, frames_like(frames, 25.0, (1280, 720)), 'fixture', 'small') == 0),
    ]
    results = [(name, check()) for name, check in checks]
    for name, ok in results:
      print(f'RunPerformance: fixture: {name}: {"ok" if ok else "FAILED"}')
    if not all(ok for _, ok in results):
      sys.exit(1)
    print(f'RunPerformance: the quick run gave {len(figures)} figures over {len(result["scenes"])} scenes and '
          f'{len(result["travel"])} travel edges')


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--skip-build', action='store_true')
  parser.add_argument('--tool', help='the SpeciesPerformance.exe to run (default: the Release|ARM64 build)')
  parser.add_argument('--bench', help='the SpeciesFrameBench.exe to run (default: the Release|ARM64 build)')
  parser.add_argument('--headless-only', action='store_true', help='run the headless scenes alone')
  parser.add_argument('--warp', action='store_true', help='with --self-test: draw the bench on the WARP adapter (a Debug build)')
  parser.add_argument('--record', default=RECORD)
  parser.add_argument('--quick', action='store_true')
  parser.add_argument('--self-test', action='store_true')
  parser.add_argument('--tuning-source')
  args = parser.parse_args()
  if args.self_test:
    if not args.tool:
      sys.exit('RunPerformance: --self-test needs --tool')
    self_test(args.tool, args.tuning_source, args.bench, args.warp)
    return 0
  if not args.skip_build:
    build()
  tool = args.tool or os.path.join(ROOT, PLATFORM, CONFIGURATION, 'SpeciesPerformance.exe')
  with tempfile.TemporaryDirectory() as folder:
    result = run_tool(tool, args.quick, os.path.join(folder, 'result.json'), args.tuning_source)
    frames = None
    if not args.headless_only:
      bench = args.bench or os.path.join(ROOT, PLATFORM, CONFIGURATION, 'SpeciesFrameBench.exe')
      frames = run_bench(bench, args.quick, os.path.join(folder, 'frames.json'))
  for scene, key, value in figures_of(result):
    budget, unit, label = BUDGETS[key]
    print(f'RunPerformance: {scene}: {label}: {value:.4g} {unit} against {budget:g}: {status_of(value, budget)}')
  factor = travel_factor(result)
  if factor is not None:
    print(f'RunPerformance: travel: the largest per-edge median walking factor is {factor:.4f} '
          f'(wld.travel.valley_path_factor is {result["valley_path_factor"]:g})')
  for scene, figure, measured, budget, status in reported_rows(result):
    print(f'RunPerformance: {scene}: {figure}: {measured} against {budget}: {status}')
  if frames is not None:
    for scene, figure, measured, budget, status in rendered_rows(frames, result.get('tick_period_s'))[0]:
      print(f'RunPerformance: {scene}: {figure}: {measured} against {budget}: {status}')
  if args.quick:
    print('RunPerformance: a quick run measures nothing and records nothing')
    return 0
  for name, built in (('tool', result), ('bench', frames)):
    if built is not None and (built['configuration'], built['platform']) != (CONFIGURATION, PLATFORM):
      print(f'RunPerformance: the {name} is a {built["configuration"]}|{built["platform"]} build; only {CONFIGURATION}|{PLATFORM} '
            'is recorded (O69)')
      return 0
  when = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
  return record(args.record, machine(), result, frames, identity(), when)


if __name__ == '__main__':
  sys.exit(main())
