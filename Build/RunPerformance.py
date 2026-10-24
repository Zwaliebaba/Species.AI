"""RunPerformance.py: the MVP's performance job (DBG-208, DBG-209; M1 plan §4, ADR-110), which the owner runs on the
reference test machine.

  python Build/RunPerformance.py [--skip-build] [--tool <SpeciesPerformance.exe>] [--bench <SpeciesFrameBench.exe>]
                                 [--headless-only] [--record <file>] [--quick]
  python Build/RunPerformance.py --saved-world <world folder> [--skip-build] [--tool <SpeciesPerformance.exe>] [--record <file>]
  python Build/RunPerformance.py --valley [--skip-build] [--tool <SpeciesPerformance.exe>] [--record <file>] [--quick]
  python Build/RunPerformance.py --link [--skip-build] [--tool <SpeciesPerformance.exe>] [--record <file>] [--quick]
  python Build/RunPerformance.py --away [--skip-build] [--tool <SpeciesPerformance.exe>] [--record <file>] [--quick]
  python Build/RunPerformance.py --self-test --tool <SpeciesPerformance.exe> [--bench <SpeciesFrameBench.exe> --warp]

--tuning-source <GameData> makes the tool read the authored tuning record instead of the cooked one beside it, for a
tool built without its content folder.

Builds Release|x64 through Species.slnx (AGENTS.md §3; O100 moved the reference test machine from the ARM64
development build of O69 to an x64 PC), runs the headless measurement scenes of SpeciesPerformance,
each from a fixed seed with a recorded command stream, then the rendered scenes of SpeciesFrameBench (M1 plan T18,
ADR-126) unless --headless-only is given, reads the machine's configuration (processor, memory size, graphics adapters,
their driver versions, display mode and OS build; every hardware adapter, as a laptop has two, and the bench names the
one it drew on, O100) and writes the configuration, the build's identity and each figure against
its budget into the performance section of playtests/M1.md.

The first run pins the machine, and the first run of the rendered scenes pins the display they drew on, as the bench
reports its primary display (O75); that display must run at 60 Hz with the battle drawn at 1,920 x 1,080, borderless on
a display of that size or in a window of that size on a larger one (O85), and a first rendered run that drew otherwise
stops and records nothing. A later run whose processor, memory or graphics adapter differs from the pinned ones, or whose
display differs for the rendered scenes, stops and records nothing, because a change of the hardware re-baselines every
result (DBG-209); the driver and the OS build are recorded with each run and stop nothing. Once pinned, a figure above
its budget fails the run (exit code 1); the run that pins only reports. The sandbox's battle drawn, and from M2 task T19
the steward's settlement drawn at its plateau (ADR-156), pass when the 99th percentile of the frame interval, from one
frame's start to the next (ADR-148), is at most 1000 ÷ mvp.client.fps_target + 1 ms (O75); their missed frames, GPU
time, waits, the refreshes that showed no new frame and longest frames, the server's tick rate while drawn, how the
settlement grew and the scene of mvp.client.figures_max figures are reported. From M3 task T16 a crowd order's time to its
display in each judged scene passes at most 275 ms at the 95th percentile (ADR-079). Also reported are a
valley's generation and its land route graph, the route sweep and the log flood's detail (ADR-091, ADR-102, ADR-104).
From M2 task T10 the growth scene of each seed is reported against M2's exit, with the snapshot of its settlement, and
the settlement at work and the region with four scripted settlements are judged against the Worker's and the region's
budgets (ADR-138); M2's run is recorded with --record playtests/M2.md. --quick runs every scene briefly to show that it
runs, and records nothing. --saved-world runs no scene: from M2 task T19 it reads the growth of the steward's own world
from the server's statistics in the newest snapshot of the folder it names (the owner's growth run of the M2 plan §5,
ADR-156), judges it as the growth scene is, and records it in playtests/M2.md unless --record names another file; the
machine is neither read nor pinned for it. --valley runs the valley run alone (M3 task T9, ADR-164): the corpus of seeds,
each valley's four settlements driven by a policy for a run, judged against Playtest 1's measurable lines and recorded
in playtests/M3.md unless --record names another file; it fails (exit code 1) when a line is missed, and neither reads
nor pins the machine, since a valley run's figures are the simulation's, not the machine's speed. --link runs the link
run alone (M4 task T11, ADR-192): the link scene's 30 minutes over QuicTransport through the link simulator
(Tools/LinkSimulator, ADR-088 item 7) at 10 % loss each way, a 150 ms round trip and 5 % reordering, with a rejoin at
minute 20, judged against the transport's confirmations (ADR-086, ADR-088, ADR-089) and recorded in playtests/M4.md
unless --record names another file; it fails (exit code 1) when a line is missed, and neither reads nor pins the machine.
--away runs the away run alone (M4 task T12, ADR-193): each always-on valley of the corpus played for its run at PIL-102's
typical and check-in patterns, judged against Playtest 2's measurable lines and recorded in playtests/M4.md unless
--record names another file; it fails (exit code 1) when a line is missed, and neither reads nor pins the machine.
--self-test is CI's: it runs the given tool with --quick, and the bench too when one is given (--warp draws it on the
WARP adapter), the link simulator's own test and a quick link run through it, then proves the record's rules on
fixtures in a temporary folder, and writes nothing into the checkout.
"""
import argparse
import datetime
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOLUTION = os.path.join(ROOT, 'Species.slnx')
RECORD = os.path.join(ROOT, 'playtests', 'M1.md')
SAVED_WORLD_RECORD = os.path.join(ROOT, 'playtests', 'M2.md')  # ADR-156: the owner's growth run is M2's evidence
SAVED_WORLD_SECTION = "## The owner's growth run"
VALLEY_RECORD = os.path.join(ROOT, 'playtests', 'M3.md')  # ADR-164: the valley run predicts Playtest 1's measured lines
VALLEY_SECTION = '## The valley run'
LINK_RECORD = os.path.join(ROOT, 'playtests', 'M4.md')  # ADR-192: the transport's confirmations are M4's evidence
LINK_SECTION = '## The link run'
LINK_SIMULATOR = os.path.join(ROOT, 'Tools', 'LinkSimulator', 'LinkSimulator.py')
# ADR-086 and ADR-088's confirmations at M4, as ADR-192 measures them: a figure frozen at most 2 ticks over one lost
# datagram; the overhead per state datagram within 5 bytes of ADR-088 item 3's 69 bytes on IPv4 with ADR-111's header
# (the 3-byte message header and the 10-bit record count on top of the 8 bytes item 3 counts); a rejoin within 10 s.
LINK_FREEZE_TICKS = 2
LINK_OVERHEAD_BYTES = 73.25
LINK_OVERHEAD_TOLERANCE_BYTES = 5.0
LINK_REJOIN_SECONDS = 10.0
AWAY_SECTION = '## The away run'
# Playtest 2's measurable lines (the MVP scope's table 6), as ADR-193 reads them: PIL-022's absence of
# pil.session.away_short_hours or more with an attack, its strength back within pil.guard.regain_days; and PIL-023's
# guard, the typical pattern's growth against the check-in pattern's, reported against pil.guard.presence_gain_share and
# pil.guard.check_in_floor_share.
AWAY_SHORT_HOURS = 8.0
AWAY_REGAIN_DAYS = 2.0
AWAY_PRESENCE_GAIN = 1.25
AWAY_CHECK_IN_FLOOR = 0.70
CONFIGURATION = 'Release'
PLATFORM = 'x64'  # O100: the reference test machine is an x64 PC, in place of O69's ARM64 development build

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
# ADR-079 (M3 task T16): a crowd order reaches the display within 275 ms at the 95th percentile on the M3 build. The bench
# times an order from the frame that sends it to the present of the first frame that displays its tick; ADR-079's model
# then counts the input's wait for its frame (at most one frame), the queued frame and half a scan-out, 2.5 frames.
ORDER_BUDGET_MS = 275.0
ORDER_DISPLAY_FRAMES = 2.5
RENDERED_REFRESH_HZ = 60  # O75: the rendered scenes run on a 60 Hz display, at 1,920 x 1,080 (O85)
# The rendered scenes judged against the frame budget: the sandbox's battle (O75), from M2 task T19 the steward's
# settlement at its plateau (ADR-156), and from M3 task T9 the valley at war (ADR-164); the rest are reported.
JUDGED_SCENES = ('sandbox-drawn', 'settlement-drawn', 'valley-drawn')
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
  if 'valley-battles' in scenes:
    # M3 task T9 (ADR-164): an AI settlement's decisions, its economy and its battle cores, per settlement per tick.
    s = scenes['valley-battles']
    if s['ais'] and s['ticks']:
      out.append(('valley-battles', 'ai_us', marker_us(s, 'sim.stewards.step') / (s['ticks'] * s['ais'])))
  for name in ('region-bound', 'region-bound-settled', 'valley-battles'):
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


def run_tool(tool, quick, out, tuning_source=None, scenes=None, extra=()):
  args = ([tool, '--out', out] + (['--quick'] if quick else []) + (['--tuning-source', tuning_source] if tuning_source else []) +
          (['--scenes', scenes] if scenes else []) + list(extra))
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
    # O100: every hardware adapter, in name order, since a laptop's display adapter is not the one the game draws on.
    "$a = Get-CimInstance Win32_VideoController | Where-Object { $_.Name -notlike 'Microsoft *' } | Sort-Object Name; "
    "$o = Get-CimInstance Win32_OperatingSystem; "
    "$u = (Get-ItemProperty 'HKLM:\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion').UBR; "
    "[pscustomobject]@{ processor = $p.Name.Trim(); memory = ('{0} GB' -f [math]::Round($c.TotalPhysicalMemory / 1GB)); "
    "adapter = ($a.Name -join ' + '); driver = ($a.DriverVersion -join ' + '); "
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
    # The C++ tools are required, since another product's MSBuild, such as SQL Server Management Studio's, may be the latest.
    found = subprocess.run([vswhere, '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                            '-find', r'MSBuild\**\Bin\MSBuild.exe'], capture_output=True,
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
  Only the judged scenes drawn at 1,920 x 1,080 are judged; the rest is reported (O75). The server's tick rate while drawn
  is the ticks measured over the frames' intervals, against the tick period of the headless result when it is given,
  which also gives the hours the settlement grew for (ADR-156)."""
  budget = frame_budget_ms(frames)
  rows = []
  above = False
  for scene in frames['scenes']:
    name = scene['name']
    size = (scene['window']['width'], scene['window']['height'])
    p99 = scene['interval_ms']['p99']
    if name in JUDGED_SCENES and size == RENDERED_SIZE:
      status = 'within budget' if p99 <= budget else 'above budget'
      above = above or p99 > budget
    elif name in JUDGED_SCENES:
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
    ] + pacing_rows(scene) + order_rows(scene, name in JUDGED_SCENES and size == RENDERED_SIZE) + [
      (name, 'figures alive and on screen', f'{scene["figures_alive"]["max"]} alive at most; {scene["figures_on_screen"]["mean"]:.0f} '
       f'on screen on average, {scene["figures_on_screen"]["max"]} at most', '—', 'reported'),
    ]
    grown = scene.get('grown')
    if grown:
      played = f'{grown["ticks"] * tick_period_s / 3600.0:.2f} hours of play' if tick_period_s else f'{grown["ticks"]} ticks'
      rows.append((name, 'the settlement drawn (ADR-156)', f'{grown["workers"]} Workers and {grown["structures"]} structures after '
                   f'{played}', '—', 'reported'))
    valley = scene.get('valley')
    if valley:
      rows.append((name, 'the valley at war drawn (ADR-164)', f'{valley["battles"]} battles in progress and {valley["figures"]} figures '
                   f'after {valley["ticks"]} ticks', '2 battles', 'reported'))
    above = above or order_late(scene, name in JUDGED_SCENES and size == RENDERED_SIZE)
    seconds = scene['interval_ms']['mean'] * scene['frames'] / 1000.0
    if seconds > 0:
      rows.append((name, 'server ticks per second while drawn (MVP-085)', f'{scene["ticks"] / seconds:.2f}',
                   f'{1.0 / tick_period_s:g}' if tick_period_s else '—', 'reported'))
  return rows, above


def order_display_ms(scene):
  """ADR-079: a crowd order's time to its display at the 95th percentile, the bench's time to the present plus 2.5 of the
  scene's median frame intervals; none when the bench timed no order."""
  if not scene.get('order_ms'):
    return None
  return scene['order_ms']['p95'] + ORDER_DISPLAY_FRAMES * scene['interval_ms']['p50']


def order_late(scene, judged):
  """Whether a judged scene's orders reach the display later than ADR-079's 275 ms, or one was given up on."""
  if not judged or 'orders' not in scene:
    return False
  display = order_display_ms(scene)
  return scene['orders_lost'] > 0 or (display is not None and display > ORDER_BUDGET_MS)


def order_rows(scene, judged):
  """ADR-079's row of a rendered scene (M3 task T16): a crowd order to its display at the 95th percentile, judged in a
  judged scene drawn at 1,920 x 1,080. A result from an older bench has none."""
  if 'orders' not in scene:
    return []
  display = order_display_ms(scene)
  given_up = f', {scene["orders_lost"]} given up on' if scene['orders_lost'] else ''
  if display is None:
    measured, status = f'no order timed{given_up}', ('above budget' if judged and scene['orders_lost'] else 'not measured')
  else:
    measured = f'{display:.0f} ms ({scene["order_ms"]["p95"]:.0f} ms to the present; {scene["orders"]} orders{given_up})'
    status = ('above budget' if order_late(scene, True) else 'within budget') if judged else 'reported'
  return [(scene['name'], 'a crowd order to its display, 95th percentile (ADR-079)', measured, f'{ORDER_BUDGET_MS:.0f} ms', status)]


def pacing_rows(scene):
  """ADR-148's rows of a rendered scene, which say where a long frame's time went: its time on the GPU, the waits at the
  99th percentile, the refreshes on which the display showed no new frame by the swap chain's own statistics, and the
  longest frames apart. A result from an older bench has none of them."""
  name = scene['name']
  rows = []
  if 'gpu_ms' in scene:
    gpu = scene['gpu_ms']
    measured = f'{gpu["p99"]:.2f} ms, {gpu["max"]:.1f} ms' if scene.get('gpu_frames') else 'not read: no timestamp queries'
    if scene.get('samples'):
      measured += f', the scene at {scene["samples"]}x multisampling (ADR-015)'
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
  """Whether the rendered scenes may pin the display they drew on: a 60 Hz display on which every judged scene was
  drawn at 1,920 x 1,080, borderless on a display of that size (O75) or in a window of that size on a larger one (O85)."""
  sizes = [(s['window']['width'], s['window']['height']) for s in frames['scenes'] if s['name'] in JUDGED_SCENES]
  return (drawn_on or '').endswith(f' at {RENDERED_REFRESH_HZ} Hz') and bool(sizes) and all(size == RENDERED_SIZE for size in sizes)


def run_section(when, build_id, config, result, frames):
  """The run's section, and whether a headless and a rendered figure is above its budget."""
  drawn_on = f', the rendered scenes on {rendered_display(frames, config)}' if frames is not None else ''
  if frames is not None and frames.get('adapter'):
    drawn_on += f' drawn by {frames["adapter"]}'  # O100
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


def ai_settlements_of(folder):
  """O103: the AI settlements a saved world was founded with, from its description (world.txt, DAT-196's form); 0 when it
  names none, as a world described before M3 does."""
  try:
    with open(os.path.join(folder, 'world.txt'), encoding='utf-8') as f:
      lines = f.read().splitlines()
  except OSError:
    return 0
  for line in lines:
    key, _, value = line.partition('=')
    if key.strip() == 'world.ai_settlements':
      return int(value.strip()) if value.strip().isdigit() else 0
  return 0


def run_saved_world(tool, folder, out, tuning_source=None):
  """ADR-156: the tool's reading of a saved world's growth."""
  args = [tool, '--saved-world', folder, '--out', out] + (['--tuning-source', tuning_source] if tuning_source else [])
  completed = subprocess.run(args, capture_output=True, text=True)
  if completed.returncode != 0:
    raise SystemExit(f'RunPerformance: {os.path.basename(tool)} failed:\n{completed.stdout}{completed.stderr}')
  with open(out, encoding='utf-8') as f:
    return json.load(f)


def saved_world_rows(saved):
  """The owner's growth run (figure, measured, target, result), judged as the growth scene's runs are (the M2 plan §5)."""
  world = saved['saved_world']
  kinds = [step[1] for step in world['steps']]
  plateau = 'not reached' if world['plateau_h'] is None else f'{world["plateau_h"]:.2f} h, {world["largest_workers"]} Workers at most'
  if world['plateau_h'] is None:
    plateau_result = 'open: not reached yet'
  elif world['plateau_met']:
    plateau_result = 'met: in hours 4 to 6'
  else:
    plateau_result = 'outside hours 4 to 6 (O84: it yields first)'
  # A gap still open after a last step within 95 % of the largest Worker count may be the plateau's hour forming, so it
  # is judged once the world has run on.
  last = world['steps'][-1] if world['steps'] else None
  forming = (world['plateau_h'] is None and last is not None and last[3] >= 0.95 * world['largest_workers'] and
             abs(last[0] * saved['tick_period_s'] / 3600.0 - world['gap_from_h']) < 1e-3)
  gap_result = 'met' if world['gap_met'] else 'open: the plateau may be forming' if forming else 'above 20 min'
  return [
    ('played', f'{world["hours"]:.2f} hours of world time ({world["ticks"]} ticks)', '—', 'reported'),
    ('the largest gap between growth steps up to the plateau or hour 5', f'{world["gap_minutes"]:.1f} min from {world["gap_from_h"]:.2f} h',
     '20 min (pil.settlement.growth_step_minutes)', gap_result),
    ('the plateau, an hour with no growth step within 95 % of the largest Worker count', plateau, '4 to 6 h', plateau_result),
    ('growth steps', f'{len(kinds)}: {kinds.count(1)} structures built, {kinds.count(2)} population steps, {kinds.count(3)} resource '
     'sites taken in', '—', 'reported'),
  ]


def record_saved_world(path, saved, build_id, when):
  """Writes the owner's growth run into the record's section of its own; returns 0."""
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else f'# {os.path.splitext(os.path.basename(path))[0]}\n'
  if SAVED_WORLD_SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{SAVED_WORLD_SECTION}\n\nWritten by `Build/RunPerformance.py --saved-world` (ADR-156): the growth '
                                'of the steward\'s own world, read from the server\'s statistics in its newest snapshot (MVP-064) '
                                'and judged as the growth scene is (the M2 plan §5).\n')
  world = saved['saved_world']
  rows = '\n'.join(f'| {figure} | {measured} | {target} | {result} |' for figure, measured, target, result in saved_world_rows(saved))
  section = (f'### The world of seed {world["seed"]}, read {when}, build {build_id}, from {world["snapshot"]}\n\n'
             f'| Figure | Measured | Target | Result |\n|---|---|---|---|\n{rows}\n')
  # The section's runs come before any later section of the record.
  head, marker, rest = text.partition(SAVED_WORLD_SECTION)
  following = rest.find('\n## ')
  if following < 0:
    text = text.rstrip('\n') + '\n\n' + section
  else:
    text = head + marker + rest[:following].rstrip('\n') + '\n\n' + section + rest[following:]
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: the owner\'s growth run recorded in {shown(path)}')
  return 0


def valley_rows(result):
  """ADR-164: the valley run against Playtest 1's measurable lines (the MVP scope's table 5): each seed's figures, then the
  corpus judged, as (figure, measured, target, result). A battle is [start hour, minutes, attacker, defender, claim,
  figures, end, attacker won] and a claim [claimant, area, hour made, hour ended or None]; a claim is a conflict between
  neighbours with an end state, judged when it ended or was made 7 hours of play before the run's end, PIL-123's 7 world
  days at the session pace (MVP-070)."""
  runs = result.get('valley') or []
  rows = []
  for run in runs:
    figures = 'none' if run['steward_figures_median'] is None else (
      f'{run["steward_figures_median"]} typical, {run["steward_figures_least"]} least')
    contact = 'none' if run['contact_h'] is None else f'{run["contact_h"]:.2f} h'
    rows.append((f'seed {run["seed"]}', f'{run["steward_battles"]} of the steward\'s battles ({figures}); {run["battles"]} battles, '
                 f'{run["battles_4_to_12"] * 100:.0f} % of 4 to 12 min, the longest {run["longest_battle_min"]:.1f} min; '
                 f'{len(run["claim_list"])} claims; the strongest {run["strongest_to_median"]:.2f} times the median; the largest '
                 f'gap {run["step_gap_min"]:.1f} min; first contact {contact}; Workers {run["workers"]}', '—', 'reported'))
  if not runs:
    return rows
  hours = max(run['ticks'] for run in runs) * result['tick_period_s'] / 3600.0
  steward_figures = sorted(b[5] for run in runs for b in run['battle_list'] if run['steward_site'] in (b[2], b[3]))
  lengths = [b[1] for run in runs for b in run['battle_list']]
  judged = [c for run in runs for c in run['claim_list'] if c[3] is not None or c[2] + 7.0 <= hours]
  settled = [c for c in judged if c[3] is not None and c[3] - c[2] <= 7.0]
  per_run = [run['steward_battles'] for run in runs]
  gaps = [run['step_gap_min'] for run in runs]
  strongest = [run['strongest_to_median'] for run in runs]
  within = sum(1 for m in lengths if 4.0 <= m <= 12.0)
  typical = steward_figures[len(steward_figures) // 2] if steward_figures else None

  def met(ok):
    return 'met' if ok else 'missed'

  rows += [
    ('the largest gap between the steward\'s growth steps in hour 0 to 5, the most of any seed', f'{max(gaps):.1f} min', '20 min',
     met(max(gaps) <= 20.0)),
    ('figures in the steward\'s battles, typical and least', 'no battle' if typical is None else
     f'{typical} typical, {steward_figures[0]} least over {len(steward_figures)} battles', '300 to 450, never below 200',
     met(typical is not None and 300 <= typical <= 450 and steward_figures[0] >= 200)),
    ('battle length', f'{within / max(len(lengths), 1) * 100:.0f} % of {len(lengths)} within 4 to 12 min, the longest '
     f'{max(lengths, default=0.0):.1f} min', '90 %, none above 20 min',
     met(bool(lengths) and within >= 0.9 * len(lengths) and max(lengths) <= 20.0)),
    ('conflicts that reach an end state within 7 hours of play', f'{len(settled)} of {len(judged)} claims', '60 %',
     met(bool(judged) and len(settled) >= 0.6 * len(judged))),
    ('the strongest settlement\'s strength at the end against the median, the most of any seed', f'{max(strongest):.2f} times',
     '3 times', met(max(strongest) <= 3.0)),
    ('battles of the steward\'s settlement in a run, the fewest and the most of any seed', f'{min(per_run)} to {max(per_run)}', '4 to 10',
     met(min(per_run) >= 4 and max(per_run) <= 10)),
  ]
  return rows


def record_valley(path, result, build_id, when):
  """Writes the valley run into the record's section of its own; returns 0 when every line is met, 1 otherwise."""
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else f'# {os.path.splitext(os.path.basename(path))[0]}\n'
  if VALLEY_SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{VALLEY_SECTION}\n\nWritten by `Build/RunPerformance.py --valley` (ADR-164): the valley run of the M3 '
                                'plan §4 on its corpus of seeds, every settlement driven by a policy for a run, read against '
                                'Playtest 1\'s measurable lines (the MVP scope\'s table 5).\n')
  rows = valley_rows(result)
  table = '\n'.join(f'| {figure} | {measured} | {target} | {status} |' for figure, measured, target, status in rows)
  seeds = [run['seed'] for run in result['valley']]
  section = (f'### Seeds {seeds[0]} to {seeds[-1]}, run {when}, build {build_id}, {result["configuration"]}|{result["platform"]}\n\n'
             f'| Figure | Measured | Target | Result |\n|---|---|---|---|\n{table}\n')
  head, marker, rest = text.partition(VALLEY_SECTION)
  following = rest.find('\n## ')
  if following < 0:
    text = text.rstrip('\n') + '\n\n' + section
  else:
    text = head + marker + rest[:following].rstrip('\n') + '\n\n' + section + rest[following:]
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: the valley run recorded in {shown(path)}')
  return 0 if all(status != 'missed' for _, _, _, status in rows) else 1


def free_udp_port(taken=()):
  while True:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
      probe.bind(('127.0.0.1', 0))
      port = probe.getsockname()[1]
    if port not in taken:
      return port


def run_link(tool, quick, folder, tuning_source=None):
  """ADR-192: the link scene through the link simulator at its default link, on two free loopback ports; the result
  carries the simulator's figures as link_simulator."""
  server_port = free_udp_port()
  link_port = free_udp_port((server_port,))
  stats = os.path.join(folder, 'link-simulator.json')
  simulator = subprocess.Popen([sys.executable, LINK_SIMULATOR, '--listen', str(link_port), '--server', str(server_port), '--stats', stats],
                               stdout=subprocess.DEVNULL)
  try:
    result = run_tool(tool, quick, os.path.join(folder, 'link.json'), tuning_source, 'link',
                      ('--server-port', str(server_port), '--link-port', str(link_port)))
  finally:
    simulator.terminate()
    simulator.wait(timeout=30)
  if os.path.exists(stats):
    with open(stats, encoding='utf-8') as f:
      result['link_simulator'] = json.load(f)
  return result


def link_rows(result):
  """ADR-192: the link run against the transport's confirmations (ADR-086, ADR-088, ADR-089), as (figure, measured,
  target, result)."""
  link = result['link']
  window = link['window']
  server, client = link['server_path'], link['client_path']
  single, multiple, none = link['figures_single_loss'], link['figures_multiple_loss'], link['figures_no_loss']
  minutes = link['ticks'] * result['tick_period_s'] / 60.0

  def met(ok):
    return 'met' if ok else 'missed'

  rows = [
    ('a figure\'s longest freeze over one lost datagram that carried it',
     f'{single["longest_ticks"]} ticks, over {single["count"]} such freezes', f'at most {LINK_FREEZE_TICKS} ticks',
     met(single['count'] > 0 and single['longest_ticks'] <= LINK_FREEZE_TICKS)),
    ('the overhead per state datagram on IPv4, every packet the server sent charged to its datagrams',
     f'{window["overhead_bytes_per_datagram"]:.1f} bytes over {window["state_datagrams"]} datagrams, '
     f'{window["packets_per_datagram"]:.2f} packets a datagram', f'{LINK_OVERHEAD_BYTES:g} ± {LINK_OVERHEAD_TOLERANCE_BYTES:g} bytes',
     met(window['state_datagrams'] > 0 and abs(window['overhead_bytes_per_datagram'] - LINK_OVERHEAD_BYTES) <= LINK_OVERHEAD_TOLERANCE_BYTES)),
    ('a rejoin, from its connection\'s start to its first state', f'{link["rejoin_s"]:.2f} s' if link['rejoined'] else 'none',
     f'within {LINK_REJOIN_SECONDS:g} s', met(link['rejoined'] and 0.0 <= link['rejoin_s'] <= LINK_REJOIN_SECONDS)),
    ('the hash of every tick against the session\'s journal replayed without a transport',
     f'{link["hashes_compared"]} ticks, {"equal" if link["hashes_match"] else "different"}', 'equal on every tick',
     met(link['hashes_match'] and link['hashes_compared'] >= link['ticks'])),
    ('the peer\'s datagram support (ADR-089), at the server and the client',
     ', '.join('enabled' if path['datagrams_enabled'] else 'disabled' for path in (server, client)), 'enabled at both',
     met(server['datagrams_enabled'] and client['datagrams_enabled'])),
    ('the session', f'{minutes:.1f} min, {link["evaluated_ticks"]} ticks compared, {link["unevaluated_ticks"]} without a view', '—',
     'reported'),
    ('state datagrams', f'{link["state_datagrams"]}, {link["datagrams_lost"]} reported lost, {link["datagrams_unresolved"]} unresolved',
     '—', 'reported'),
    ('a figure\'s freezes over two or more lost datagrams, and over none',
     f'{multiple["count"]}, the longest {multiple["longest_ticks"]} ticks; {none["count"]}, the longest {none["longest_ticks"]} ticks',
     '—', 'reported'),
    ('the client\'s missing baselines, late updates and undecoded messages',
     f'{link["missing_baselines"]}, {link["late_updates"]}, {link["messages_dropped"]}', '—', 'reported'),
    ('the statuses asked for and the answers', f'{link["requests_sent"]} and {link["answers"]}', '—', 'reported'),
    ('the first join', f'{link["join_s"]:.2f} s', '—', 'reported'),
    ('the server end\'s round trip, congestion window and congestion events (MsQuic)',
     f'{server["rtt_us"] / 1000:.1f} ms, {server["congestion_window_bytes"]} bytes, {server["congestion_events"]}', '—', 'reported'),
    ('the largest datagram MsQuic sends, and the path MTU', f'{server["datagram_max_send_bytes"]} and {server["path_mtu_bytes"]} bytes',
     '—', 'reported'),
  ]
  simulator = result.get('link_simulator')
  if simulator:
    settings = simulator['settings']
    ways = '; '.join(f'{name.replace("_", " ")} {way["lost"]} of {way["taken"]} lost, {way["reordered"]} reordered'
                     for name, way in ((n, simulator[n]) for n in ('client_to_server', 'server_to_client')))
    rows.append(('the link simulator', f'{settings["rtt_ms"]:g} ms, {settings["loss_percent"]:g} % lost and {settings["reorder_percent"]:g} % '
                 f'reordered each way; {ways}', '—', 'reported'))
  return rows


def record_link(path, result, build_id, when):
  """Writes the link run into the record's section of its own; returns 0 when every line is met, 1 otherwise."""
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else f'# {os.path.splitext(os.path.basename(path))[0]}\n'
  if LINK_SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{LINK_SECTION}\n\nWritten by `Build/RunPerformance.py --link` (ADR-192): the link scene over '
                                'QuicTransport through the link simulator, read against the transport\'s confirmations of '
                                'ADR-086, ADR-088 and ADR-089.\n')
  rows = link_rows(result)
  table = '\n'.join(f'| {figure} | {measured} | {target} | {status} |' for figure, measured, target, status in rows)
  section = (f'### Run {when}, build {build_id}, {result["configuration"]}|{result["platform"]}\n\n'
             f'| Figure | Measured | Target | Result |\n|---|---|---|---|\n{table}\n')
  head, marker, rest = text.partition(LINK_SECTION)
  following = rest.find('\n## ')
  if following < 0:
    text = text.rstrip('\n') + '\n\n' + section
  else:
    text = head + marker + rest[:following].rstrip('\n') + '\n\n' + section + rest[following:]
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: the link run recorded in {shown(path)}')
  return 0 if all(status != 'missed' for _, _, _, status in rows) else 1


def away_growth(run):
  """The steward's strength gained per world day over the run, and the AI settlements' mean, in resource units."""
  days = max(run['hours'] / 24.0, 1e-9)
  return ((run['steward_strength'][-1] - run['steward_strength'][0]) / days, (run['ai_strength'][-1] - run['ai_strength'][0]) / days)


def away_rows(result):
  """ADR-193: the away run against Playtest 2's measurable lines (the MVP scope's table 6), as (figure, measured, target,
  result). An absence is [from hour, to hour, battles defended, deputy actions, strength before, strength after, hour
  regained or None]."""
  runs = result.get('away') or []
  rows = []
  for run in runs:
    attacked = [a for a in run['absences'] if a[2] > 0]
    steward, ai = away_growth(run)
    rows.append((f'seed {run["seed"]}, the {run["pattern"].replace("_", "-")} pattern',
                 f'{run["hours"] / 24.0:.1f} world days; strength {run["steward_strength"][0]:.0f} to {run["steward_strength"][-1]:.0f} '
                 f'({steward:+.1f} a day, the AI settlements {ai:+.1f}); Workers {run["steward_workers"][-1]}; '
                 f'{run["steward_battles"]} of the steward\'s battles; {len(run["absences"])} absences, {len(attacked)} with an attack, '
                 f'{sum(a[3] for a in run["absences"])} deputy actions', '—', 'reported'))
  if not runs:
    return rows

  def met(ok):
    return 'met' if ok else 'missed'

  # PIL-022 at the typical pattern: every absence of 8 hours or more with an attack, its strength back within 2 days of
  # the return; one not back with fewer than 2 days of the run left is open and not judged.
  judged, regains, still_open = [], [], 0
  for run in runs:
    if run['pattern'] != 'typical':
      continue
    for start, end, battles, _, _, _, regained in run['absences']:
      if battles == 0 or end - start < AWAY_SHORT_HOURS:
        continue
      if regained is None and run['hours'] - end < AWAY_REGAIN_DAYS * 24.0:
        still_open += 1
        continue
      judged.append(regained)
      if regained is not None:
        regains.append((regained - end) / 24.0)
  never = sum(1 for r in judged if r is None)
  longest = max(regains, default=0.0)
  rows.append(('after an absence of 8 hours or more with an attack, world days back to the strength before it (the typical pattern)',
               'no such absence' if not judged else
               f'the longest {longest:.2f} days over {len(judged)} absences, {never} never back' +
               (f'; {still_open} still open at the run\'s end' if still_open else ''),
               f'at most {AWAY_REGAIN_DAYS:g} world days', met(bool(judged) and never == 0 and longest <= AWAY_REGAIN_DAYS)))
  # PIL-023, as Playtest 2 reads it: the typical pattern's growth against the check-in pattern's, seed by seed.
  pairs = []
  for run in runs:
    if run['pattern'] != 'typical':
      continue
    other = next((r for r in runs if r['seed'] == run['seed'] and r['pattern'] == 'check_in'), None)
    if other is not None:
      pairs.append((away_growth(run)[0], away_growth(other)[0]))
  more = sum(1 for typical, check_in in pairs if typical > check_in)
  ratios = [typical / check_in for typical, check_in in pairs if check_in > 0]
  mean_ratio = sum(ratios) / len(ratios) if ratios else None
  rows.append(('the steward\'s growth per world day, the typical pattern against the check-in pattern',
               f'more on {more} of {len(pairs)} seeds' + (f', {mean_ratio:.2f} times on average' if mean_ratio is not None else ''),
               'more on every seed', met(bool(pairs) and more == len(pairs))))
  rows.append(('PIL-023\'s guard: the typical pattern at least 25 % more, and the check-in pattern at least 70 % of it',
               'no pair' if mean_ratio is None else f'{mean_ratio:.2f} times; the check-in pattern {1.0 / mean_ratio * 100:.0f} %',
               f'{AWAY_PRESENCE_GAIN:g} times; {AWAY_CHECK_IN_FLOOR * 100:.0f} %', 'reported'))
  return rows


def record_away(path, result, build_id, when):
  """Writes the away run into the record's section of its own; returns 0 when every line is met, 1 otherwise."""
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else f'# {os.path.splitext(os.path.basename(path))[0]}\n'
  if AWAY_SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{AWAY_SECTION}\n\nWritten by `Build/RunPerformance.py --away` (ADR-193): each always-on valley of the '
                                'corpus played for its run at PIL-102\'s typical and check-in patterns, read against Playtest 2\'s '
                                'measurable lines (the MVP scope\'s table 6).\n')
  rows = away_rows(result)
  table = '\n'.join(f'| {figure} | {measured} | {target} | {status} |' for figure, measured, target, status in rows)
  seeds = sorted({run['seed'] for run in result['away']})
  section = (f'### Seeds {seeds[0]} to {seeds[-1]}, run {when}, build {build_id}, {result["configuration"]}|{result["platform"]}\n\n'
             f'| Figure | Measured | Target | Result |\n|---|---|---|---|\n{table}\n')
  head, marker, rest = text.partition(AWAY_SECTION)
  following = rest.find('\n## ')
  if following < 0:
    text = text.rstrip('\n') + '\n\n' + section
  else:
    text = head + marker + rest[:following].rstrip('\n') + '\n\n' + section + rest[following:]
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: the away run recorded in {shown(path)}')
  return 0 if all(status != 'missed' for _, _, _, status in rows) else 1


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
              scenes=[dict(scene, interval_ms=dict(scene['interval_ms'], p99=p99, p50=min(scene['interval_ms']['p50'], p99)),
                           window={'width': size[0], 'height': size[1]}, **ORDERS_FIXTURE)
                      for scene in frames['scenes']])


# ADR-156: the tool's reading of a saved world, its figures made up.
SAVED_WORLD_FIXTURE = {
  'tool': 'SpeciesPerformance', 'configuration': 'Release', 'platform': 'x64', 'tick_period_s': 0.1,
  'saved_world': {'snapshot': 'world-00000000000000169000.snapshot', 'seed': 7, 'ticks': 169000, 'hours': 4.694, 'largest_workers': 300,
                  'plateau_h': 4.3, 'gap_minutes': 15.3, 'gap_from_h': 0.0, 'gap_met': True, 'plateau_met': True,
                  'steps': [[9169, 2, 30, 30], [16287, 2, 40, 40], [23837, 1, 3, 40], [30000, 3, 4, 44]],
                  'samples': [[0, 20, 400.0], [3000, 22, 461.088]]},
}


# ADR-079: a scene's timed orders, made up, within the budget at a median frame interval of up to 16.7 ms.
ORDERS_FIXTURE = {'orders': 30, 'orders_lost': 0, 'order_ms': {'p50': 160.0, 'p95': 190.0, 'p99': 200.0, 'max': 205.0, 'mean': 165.0}}


# The bench's result when the self-test runs none: the shape SpeciesFrameBench writes, its figures made up.
FRAMES_FIXTURE = {
  'tool': 'SpeciesFrameBench', 'configuration': 'Debug', 'platform': 'x64', 'quick': True, 'fps_target': 60, 'figures_max': 1500,
  'adapter': 'fixture adapter', 'display': {'width': 1920, 'height': 1080, 'refresh_hz': 60},
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
              'figures_alive': {'min': figures, 'max': figures}, 'figures_on_screen': {'mean': figures * 0.8, 'max': figures},
              'grown': {'ticks': 154000, 'workers': 300, 'structures': 40} if name == 'settlement-drawn' else None,
              'valley': {'ticks': 3100, 'battles': 2, 'figures': 1204} if name == 'valley-drawn' else None, 'samples': 4,
              **ORDERS_FIXTURE}
             for name, figures in (('sandbox-drawn', 400), ('figures-max-drawn', 1500), ('settlement-drawn', 300), ('valley-drawn', 1204))],
}


# ADR-164: a valley run of two seeds as SpeciesPerformance writes it, its figures made up: seed 1 meets every line, and
# seed 2's steward fights 3 battles.
VALLEY_FIXTURE = {
  'tool': 'SpeciesPerformance', 'configuration': 'Release', 'platform': 'x64', 'tick_period_s': 0.1,
  'valley': [
    {'seed': 1, 'ticks': 288000, 'steward_site': 0, 'policy': 2, 'run_ended': True, 'contact_h': 2.4, 'steward_battles': 5,
     'steward_figures_median': 380, 'steward_figures_least': 260, 'battles': 9, 'battles_4_to_12': 1.0, 'longest_battle_min': 11.0,
     'claims_judged': 2, 'claims_settled': 1.0, 'strongest_to_median': 1.6, 'step_gap_min': 14.0, 'step_us': 900.0,
     'step_us_max': 4000.0, 'workers': [250, 240, 230, 260], 'renown': [12, 8, 5, 9],
     'battle_list': [[3.0, 8.0, 1, 0, 0, 380, 0, 0], [4.0, 9.0, 0, 1, 0, 400, 0, 1], [5.0, 6.0, 2, 3, 0, 300, 0, 0],
                     [5.5, 7.0, 1, 0, 1, 260, 0, 0], [6.0, 11.0, 0, 1, 0, 420, 0, 1], [6.5, 5.0, 3, 2, 0, 310, 0, 0],
                     [7.0, 6.0, 1, 0, 0, 350, 0, 0], [7.2, 5.0, 2, 3, 0, 300, 0, 0], [7.4, 4.5, 3, 2, 0, 280, 0, 0]],
     'claim_list': [[1, 0, 2.5, 4.0], [0, 1, 3.0, 5.5], [2, 6, 7.5, None]]},
    {'seed': 2, 'ticks': 288000, 'steward_site': 0, 'policy': 2, 'run_ended': True, 'contact_h': None, 'steward_battles': 3,
     'steward_figures_median': 330, 'steward_figures_least': 300, 'battles': 3, 'battles_4_to_12': 1.0, 'longest_battle_min': 10.0,
     'claims_judged': 0, 'claims_settled': 0.0, 'strongest_to_median': 1.2, 'step_gap_min': 16.0, 'step_us': 800.0,
     'step_us_max': 3000.0, 'workers': [240, 230, 220, 250], 'renown': [6, 4, 3, 5],
     'battle_list': [[3.0, 10.0, 1, 0, 0, 330, 0, 0], [5.0, 8.0, 0, 1, 0, 340, 0, 1], [6.0, 7.0, 1, 0, 0, 300, 0, 0]],
     'claim_list': []},
  ],
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
      if sorted(drawn) != ['figures-max-drawn', 'sandbox-drawn', 'settlement-drawn', 'valley-drawn'] or min(drawn.values()) == 0:
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
    # ADR-156: the settlement drawn above the budget while the battle is within it.
    slow_settlement = dict(smooth, scenes=[dict(s, interval_ms=dict(s['interval_ms'], p99=25.0)) if s['name'] == 'settlement-drawn' else s
                                           for s in smooth['scenes']])
    # ADR-079: the valley at war's orders late, and one of them given up on.
    late_orders = dict(smooth, scenes=[dict(s, order_ms=dict(s['order_ms'], p95=260.0)) if s['name'] == 'valley-drawn' else s
                                       for s in smooth['scenes']])
    lost_order = dict(smooth, scenes=[dict(s, orders_lost=1) if s['name'] == 'sandbox-drawn' else s for s in smooth['scenes']])

    def recorded(*phrases):
      text = open(path, encoding='utf-8').read()
      return all(phrase in text for phrase in phrases)

    # ADR-164: the valley run's lines over the corpus, and its record.
    valley_quick = run_tool(tool, True, os.path.join(folder, 'valley.json'), tuning_source, 'valley')
    valley_path = os.path.join(folder, 'M3.md')

    def valley_judged():
      rows = {figure: status for figure, _, _, status in valley_rows(VALLEY_FIXTURE)}
      one = dict(VALLEY_FIXTURE, valley=VALLEY_FIXTURE['valley'][:1])
      return (rows['battles of the steward\'s settlement in a run, the fewest and the most of any seed'] == 'missed' and
              rows['conflicts that reach an end state within 7 hours of play'] == 'met' and
              all(status in ('met', 'reported') for _, _, _, status in valley_rows(one)) and
              record_valley(valley_path, one, 'fixture', 'first') == 0 and record_valley(valley_path, VALLEY_FIXTURE, 'fixture', 'second') == 1)

    def valley_recorded():
      text = open(valley_path, encoding='utf-8').read()
      return (text.count(VALLEY_SECTION) == 1 and text.index('Seeds 1 to 1, run first') < text.index('Seeds 1 to 2, run second') and
              '2 of 2 claims' in text and '3 to 5' in text)

    # ADR-192: the link simulator's own test, then a quick link run through it, and the link run's lines and record.
    simulated = subprocess.run([sys.executable, LINK_SIMULATOR, '--self-test'], capture_output=True, text=True)
    print(simulated.stdout.strip() or simulated.stderr.strip())
    link_quick = run_link(tool, True, folder, tuning_source)
    link_path = os.path.join(folder, 'M4.md')
    for figure, measured, target, status in link_rows(link_quick):
      print(f'RunPerformance: the quick link run: {figure}: {measured} against {target}: {status}')

    def link_ran():
      link = link_quick['link']
      return (link['ticks'] >= 300 and link['rejoined'] and link['hashes_match'] and link['messages_dropped'] == 0 and
              link['answers'] > 0 and link['server_path']['datagrams_enabled'] and link['client_path']['datagrams_enabled'] and
              all(link_quick.get('link_simulator', {}).get(way, {}).get('sent', 0) > 0 for way in ('client_to_server', 'server_to_client')))

    def link_judged():
      link = link_quick['link']
      good = dict(link_quick, link=dict(link, figures_single_loss=dict(link['figures_single_loss'], count=10, longest_ticks=1),
                                        window=dict(link['window'], state_datagrams=100, overhead_bytes_per_datagram=74.0),
                                        rejoined=True, rejoin_s=1.5, hashes_match=True, hashes_compared=link['ticks']))
      frozen = dict(good, link=dict(good['link'], figures_single_loss=dict(good['link']['figures_single_loss'], longest_ticks=3)))
      heavy = dict(good, link=dict(good['link'], window=dict(good['link']['window'], overhead_bytes_per_datagram=80.0)))
      slow_rejoin = dict(good, link=dict(good['link'], rejoin_s=12.0))
      unequal = dict(good, link=dict(good['link'], hashes_match=False))
      return (record_link(link_path, good, 'fixture', 'first') == 0 and record_link(link_path, frozen, 'fixture', 'second') == 1 and
              record_link(link_path, heavy, 'fixture', 'third') == 1 and record_link(link_path, slow_rejoin, 'fixture', 'fourth') == 1 and
              record_link(link_path, unequal, 'fixture', 'fifth') == 1)

    # ADR-193: the quick away run plays both patterns, and the away run's lines and record.
    away_quick = run_tool(tool, True, os.path.join(folder, 'away.json'), tuning_source, 'away')
    away_path = os.path.join(folder, 'M4-away.md')

    def away_judged():
      def run(pattern, strengths, absences):
        return {'seed': 1, 'pattern': pattern, 'ticks': 1920, 'hours': 192.0, 'steward_site': 0, 'steward_battles': 2, 'run_ended': True,
                'mean_step_us': 1.0, 'steward_strength': strengths, 'ai_strength': [400.0, 900.0], 'steward_workers': [20, 60],
                'absences': absences}
      night = [12.0, 23.0, 1, 3, 800.0, 600.0, 40.0]
      good = dict(away_quick, away=[run('typical', [400.0, 1200.0], [night]), run('check_in', [400.0, 900.0], [])])
      slow = dict(good, away=[run('typical', [400.0, 1200.0], [[12.0, 23.0, 1, 3, 800.0, 600.0, 90.0]]), good['away'][1]])
      never = dict(good, away=[run('typical', [400.0, 1200.0], [[12.0, 23.0, 1, 3, 800.0, 600.0, None]]), good['away'][1]])
      flat = dict(good, away=[good['away'][0], run('check_in', [400.0, 1300.0], [])])
      unattacked = dict(good, away=[run('typical', [400.0, 1200.0], [[12.0, 23.0, 0, 0, 800.0, 790.0, 24.0]]), good['away'][1]])
      late = dict(good, away=[run('typical', [400.0, 1200.0], [[150.0, 161.0, 1, 2, 900.0, 700.0, None]] + [night]), good['away'][1]])
      rows = {figure: status for figure, _, _, status in away_rows(late)}
      return (record_away(away_path, good, 'fixture', 'first') == 0 and record_away(away_path, slow, 'fixture', 'second') == 1 and
              record_away(away_path, never, 'fixture', 'third') == 1 and record_away(away_path, flat, 'fixture', 'fourth') == 1 and
              record_away(away_path, unattacked, 'fixture', 'fifth') == 1 and
              rows['after an absence of 8 hours or more with an attack, world days back to the strength before it (the typical pattern)'] == 'met')

    def away_recorded():
      text = open(away_path, encoding='utf-8').read()
      return (text.count(AWAY_SECTION) == 1 and text.index('run first') < text.index('run second') < text.index('run fifth') and
              'the longest 0.71 days over 1 absences' in text and 'more on 1 of 1 seeds' in text)

    def link_recorded():
      text = open(link_path, encoding='utf-8').read()
      return (text.count(LINK_SECTION) == 1 and text.index('Run first') < text.index('Run second') < text.index('Run fifth') and
              '74.0 bytes' in text and '1.50 s' in text)

    # O85: a record of its own, whose first rendered run draws a 1,920 x 1,080 window on a larger 60 Hz panel.
    panel = os.path.join(folder, 'M1-panel.md')
    # ADR-156: the owner's growth run is a section of its own at the record's end, and a later run stays in it when
    # another section follows it.
    grown = os.path.join(folder, 'M2.md')
    with open(grown, 'w', encoding='utf-8', newline='\n') as f:
      f.write('# M2\n\n## Session log\n\nA session.\n')

    def grown_recorded():
      record_saved_world(grown, SAVED_WORLD_FIXTURE, 'fixture', 'first')
      with open(grown, 'a', encoding='utf-8', newline='\n') as f:
        f.write('\n## Performance\n\nA run.\n')
      record_saved_world(grown, dict(SAVED_WORLD_FIXTURE, saved_world=dict(SAVED_WORLD_FIXTURE['saved_world'], plateau_h=None,
                                                                         plateau_met=False)), 'fixture', 'second')
      text = open(grown, encoding='utf-8').read()
      return (text.count(SAVED_WORLD_SECTION) == 1 and
              text.index('## Session log') < text.index('read first') < text.index('read second') < text.index('## Performance') and
              '15.3 min from 0.00 h' in text and 'met: in hours 4 to 6' in text and 'open: not reached yet' in text and
              '1 structures built, 2 population steps, 1 resource sites taken in' in text)

    def forming_judged():
      # A gap open since a last step at the largest Worker count is the plateau forming; one below it is a gap.
      world = dict(SAVED_WORLD_FIXTURE['saved_world'], plateau_h=None, plateau_met=False, gap_met=False, gap_minutes=42.7,
                   gap_from_h=30000 * 0.1 / 3600.0, largest_workers=44)
      below = dict(world, steps=world['steps'][:-1] + [[30000, 3, 4, 30]])
      result = lambda w: saved_world_rows(dict(SAVED_WORLD_FIXTURE, saved_world=w))[1][3]
      return result(world) == 'open: the plateau may be forming' and result(below) == 'above 20 min'

    def neighbors_read():
      # The description's count, and 0 for a description that names none or a folder that has none.
      alone, valley, older = (os.path.join(folder, name) for name in ('alone', 'valley', 'older'))
      for world, text in ((alone, 'world.site = 1\nworld.ai_settlements = 0\n'), (valley, 'world.site = 1\nworld.ai_settlements = 3\n'),
                          (older, 'world.site = 2\n')):
        os.makedirs(world)
        with open(os.path.join(world, 'world.txt'), 'w', encoding='utf-8', newline='\n') as f:
          f.write(text)
      return (ai_settlements_of(alone), ai_settlements_of(valley), ai_settlements_of(older), ai_settlements_of(folder)) == (0, 3, 0, 0)

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
      ('a rendered run names the adapter it drew on (O100)', lambda: recorded(f' drawn by {frames["adapter"]}')),
      ('a rendered run reports where its frames\' time went (ADR-148)',
       lambda: recorded("frame's time on the GPU", 'waits at the 99th percentile', 'showed no new frame', 'longest frames')),
      ('a rendered run names the multisampling its GPU time was measured at (ADR-015)', lambda: recorded('x multisampling (ADR-015)')),
      ('a rendered run on another display stops',
       lambda: record(path, reference, within, frames_like(frames, 16.0, display=(1920, 1080, 144)), 'fixture', 'other display') == 2),
      ('a rendered run is judged by the display the bench drew on, not the machine\'s reading',
       lambda: record(path, dict(reference, display='other'), within, smooth, 'fixture', 'reading') == 0),
      ('a headless run on another display is recorded',
       lambda: record(path, dict(reference, display='other'), within, None, 'fixture', 'headless') == 0),
      ('a rendered run within its frame budget passes', lambda: record(path, reference, within, smooth, 'fixture', 'smooth') == 0),
      ('a rendered run above its frame budget fails', lambda: record(path, reference, within, slow, 'fixture', 'slow') == 1),
      ('a settlement drawn above its frame budget fails (ADR-156)',
       lambda: record(path, reference, within, slow_settlement, 'fixture', 'slow settlement') == 1),
      ('a rendered run reports how the settlement grew', lambda: recorded('the settlement drawn (ADR-156)')),
      ('a rendered run reports a crowd order\'s time to its display (ADR-079)',
       lambda: recorded('a crowd order to its display, 95th percentile (ADR-079)')),
      ('a rendered run whose orders reach the display late fails (ADR-079)',
       lambda: record(path, reference, within, late_orders, 'fixture', 'late orders') == 1),
      ('a rendered run that gave up on an order fails (ADR-079)',
       lambda: record(path, reference, within, lost_order, 'fixture', 'lost order') == 1),
      ('the owner\'s growth run is recorded and judged in its own section (ADR-156)', grown_recorded),
      ('a gap open while the plateau may be forming is not judged yet (ADR-156)', forming_judged),
      ('a saved world founded with neighbors is told apart from one alone (O103)', neighbors_read),
      ('the quick valley run runs one seed (ADR-164)', lambda: [run['seed'] for run in valley_quick['valley']] == [1]),
      ('the valley run is judged over its corpus and recorded (ADR-164)', valley_judged),
      ('the valley run keeps its runs in its own section (ADR-164)', valley_recorded),
      ('a rendered run reports the valley at war drawn (ADR-164)', lambda: recorded('the valley at war drawn (ADR-164)')),
      ('the link simulator passes its own test (ADR-192)', lambda: simulated.returncode == 0),
      ('the quick link run joins, rejoins, answers and replays to the same hashes (ADR-192)', link_ran),
      ('the link run is judged against the transport\'s confirmations (ADR-192)', link_judged),
      ('the link run keeps its runs in its own section (ADR-192)', link_recorded),
      ('the quick away run plays both patterns of its seed (ADR-193)',
       lambda: sorted(run['pattern'] for run in away_quick['away']) == ['check_in', 'typical']),
      ('the away run is judged against Playtest 2\'s lines (ADR-193)', away_judged),
      ('the away run keeps its runs in its own section (ADR-193)', away_recorded),
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
  parser.add_argument('--tool', help='the SpeciesPerformance.exe to run (default: the Release|x64 build)')
  parser.add_argument('--bench', help='the SpeciesFrameBench.exe to run (default: the Release|x64 build)')
  parser.add_argument('--headless-only', action='store_true', help='run the headless scenes alone')
  parser.add_argument('--warp', action='store_true', help='with --self-test: draw the bench on the WARP adapter (a Debug build)')
  parser.add_argument('--record', help='the record to write (default: playtests/M1.md, playtests/M2.md with --saved-world, '
                      'playtests/M3.md with --valley or playtests/M4.md with --link or --away)')
  parser.add_argument('--saved-world', help='a saved world\'s folder: read its growth rather than run the scenes (ADR-156)')
  parser.add_argument('--valley', action='store_true', help='run the valley run alone and judge Playtest 1\'s lines (ADR-164)')
  parser.add_argument('--link', action='store_true', help='run the link run alone and judge the transport\'s confirmations (ADR-192)')
  parser.add_argument('--away', action='store_true', help='run the away run alone and judge Playtest 2\'s lines (ADR-193)')
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
  if args.saved_world:
    # O103: M2's growth run is a world alone; one founded with neighbors is not judged against it.
    neighbors = ai_settlements_of(args.saved_world)
    if neighbors:
      sys.exit(f'RunPerformance: {args.saved_world} was founded with {neighbors} AI settlements; the growth run is judged on a world '
               'started with Neighbors set to None (O103)')
    with tempfile.TemporaryDirectory() as folder:
      saved = run_saved_world(tool, args.saved_world, os.path.join(folder, 'saved.json'), args.tuning_source)
    for figure, measured, target, result in saved_world_rows(saved):
      print(f'RunPerformance: the owner\'s growth run: {figure}: {measured} against {target}: {result}')
    if args.quick:
      return 0
    return record_saved_world(args.record or SAVED_WORLD_RECORD, saved, identity(), datetime.datetime.now().strftime('%Y-%m-%d %H:%M'))
  if args.away:
    with tempfile.TemporaryDirectory() as folder:
      result = run_tool(tool, args.quick, os.path.join(folder, 'away.json'), args.tuning_source, 'away')
    for figure, measured, target, status in away_rows(result):
      print(f'RunPerformance: the away run: {figure}: {measured} against {target}: {status}')
    if args.quick:
      print('RunPerformance: a quick run measures nothing and records nothing')
      return 0
    return record_away(args.record or LINK_RECORD, result, identity(), datetime.datetime.now().strftime('%Y-%m-%d %H:%M'))
  if args.link:
    with tempfile.TemporaryDirectory() as folder:
      result = run_link(tool, args.quick, folder, args.tuning_source)
    for figure, measured, target, status in link_rows(result):
      print(f'RunPerformance: the link run: {figure}: {measured} against {target}: {status}')
    if args.quick:
      print('RunPerformance: a quick run measures nothing and records nothing')
      return 0
    return record_link(args.record or LINK_RECORD, result, identity(), datetime.datetime.now().strftime('%Y-%m-%d %H:%M'))
  if args.valley:
    with tempfile.TemporaryDirectory() as folder:
      result = run_tool(tool, args.quick, os.path.join(folder, 'valley.json'), args.tuning_source, 'valley')
    for figure, measured, target, status in valley_rows(result):
      print(f'RunPerformance: the valley run: {figure}: {measured} against {target}: {status}')
    if args.quick:
      print('RunPerformance: a quick run measures nothing and records nothing')
      return 0
    return record_valley(args.record or VALLEY_RECORD, result, identity(), datetime.datetime.now().strftime('%Y-%m-%d %H:%M'))
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
            'is recorded (O100)')
      return 0
  when = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
  return record(args.record or RECORD, machine(), result, frames, identity(), when)


if __name__ == '__main__':
  sys.exit(main())
