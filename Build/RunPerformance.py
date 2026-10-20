"""RunPerformance.py: the MVP's performance job (DBG-208, DBG-209; M1 plan §4, ADR-110), which the owner runs on the
reference test machine.

  python Build/RunPerformance.py [--skip-build] [--tool <SpeciesPerformance.exe>] [--record <file>] [--quick]
  python Build/RunPerformance.py --self-test --tool <SpeciesPerformance.exe>

--tuning-source <GameData> makes the tool read the authored tuning record instead of the cooked one beside it, for a
tool built without its content folder.

Builds Release|ARM64 through Species.slnx (AGENTS.md §3), runs the measurement scenes of SpeciesPerformance, each from a
fixed seed with a recorded command stream, reads the machine's configuration (processor, memory size, graphics adapter,
driver version, display mode and OS build) and writes the configuration, the build's identity and each figure against
its budget into the performance section of playtests/M1.md.

The first run pins the machine. Every later run compares the configuration with the pinned one and stops on a
difference, because a change of the machine re-baselines every result (DBG-209). Once the machine is pinned, a figure
above its budget fails the run (exit code 1); the run that pins only reports. --quick runs every scene for a few ticks
to show that it runs, and records nothing. --self-test is CI's: it runs the given tool with --quick, then proves the
record's rules on fixtures in a temporary folder (the pin, a changed machine, a figure above its budget), and writes
nothing into the checkout.
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
PIN_MARK = re.compile(r'<!-- reference-machine (\{.*?\}) -->')
SECTION = '## Performance'

# ADR-080 item 2's cost budgets, item 3's region step budget and item 7's integrated server budget, and the travel
# factor of WLD-100 (O71). When ADR-080 is amended with measured figures, this table follows it.
BUDGETS = {
  'battle_figure_us': (2.1, 'µs per figure per tick', 'figure in a battle (search, combat movement, laser bolts, Essence orbs)'),
  'work_figure_us': (1.2, 'µs per figure per tick', 'Worker at settlement work (wandering stand-in until M2)'),
  'expedition_figure_us': (0.4, 'µs per figure per tick', 'figure of an expedition outside a battle (the travel scene)'),
  'draw_us': (0.03, 'µs per draw', 'a keyed simulation draw'),
  'ai_us': (20.0, 'µs per AI per tick', 'AI settlement or deputy'),
  'side_command_us': (30.0, 'µs per side per tick', 'command of one side of a battle'),
  'region_fixed_us': (30.0, 'µs per tick', 'region fixed cost (an empty region with its terrain)'),
  'region_step_ms': (12.5, 'ms per tick, 95th percentile', 'region step at its bound'),
  'integrated_cores': (1.0, 'reference cores', 'integrated server (the sandbox battle\'s mean step over the tick period)'),
}
BATTLE_MARKERS = ['sim.teams.advance_ungrouped', 'sim.laser_bolts.advance', 'sim.effects.advance', 'sim.essence_orbs.advance']


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
  if 'empty-region' in scenes:
    out.append(('empty-region', 'region_fixed_us', scenes['empty-region']['step_us']['mean']))
  if 'region-bound' in scenes:
    out.append(('region-bound', 'region_step_ms', scenes['region-bound']['step_us']['p95'] / 1000.0))
  edges = result.get('travel') or []
  figure_ticks = sum(e['figure_ticks'] for e in edges)
  if figure_ticks:
    out.append(('travel', 'expedition_figure_us', sum(e['advance_us'] for e in edges) / figure_ticks))
  if result.get('draw_ns') is not None:
    out.append(('draws', 'draw_us', result['draw_ns'] / 1000.0))
  return [f for f in out if f[2] is not None]


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


def pinned_machine(text):
  m = PIN_MARK.search(text)
  return json.loads(m.group(1)) if m else None


def machine_table(config):
  rows = '\n'.join(f'| {label} | {config.get(key, "")} |' for key, label in MACHINE_ITEMS)
  return f'| Item | Value |\n|---|---|\n{rows}\n'


def run_section(when, build_id, configuration, platform, figures, factor, current_factor):
  lines = [f'### Run {when}, build {build_id}, {configuration}|{platform}', '',
           '| Scene | Figure | Measured | Budget | Result |', '|---|---|---|---|---|']
  failed = False
  for scene, key, value in figures:
    budget, unit, label = BUDGETS[key]
    status = status_of(value, budget)
    failed = failed or status == 'above budget'
    lines.append(f'| {scene} | {label} | {value:.4g} {unit} | {budget:g} | {status} |')
  if factor is not None:
    lines.append(f'| travel | `wld.travel.valley_path_factor` (WLD-100): the largest per-edge median of walking time × speed ÷ '
                 f'straight distance | {factor:.4f} | {current_factor:g} in the record | '
                 f'{"within the record" if factor <= current_factor else "above the record"} |')
  return '\n'.join(lines) + '\n', failed


def record(path, config, result, build_id, when):
  """Writes the run into the record's performance section; returns its exit code (0, 1 above budget, 2 machine changed)."""
  text = open(path, encoding='utf-8').read() if os.path.exists(path) else (
    '# M1\n\nThe record of milestone M1 (README, "A milestone record"). Its exit criteria are added when the milestone '
    'ends.\n')
  if SECTION not in text:
    text = text.rstrip('\n') + (f'\n\n{SECTION}\n\nWritten by `Build/RunPerformance.py` (M1 plan §4, DBG-208, DBG-209, ADR-110): '
                                'the reference test machine, pinned by the first run, and each run with every figure '
                                'against its budget.\n')
  pinned = pinned_machine(text)
  if pinned is not None and pinned != config:
    changed = [label for key, label in MACHINE_ITEMS if pinned.get(key) != config.get(key)]
    print(f'RunPerformance: the machine differs from the pinned one in: {", ".join(changed)}. A change of the machine '
          're-baselines every result (DBG-209); nothing was recorded.')
    return 2
  if pinned is None:
    text = text.rstrip('\n') + (f'\n\n### Reference test machine\n\nPinned by the run of {when}, build {build_id} (DBG-209).\n\n'
                                f'{machine_table(config)}\n<!-- reference-machine {json.dumps(config, sort_keys=True)} -->\n')
  section, above = run_section(when, build_id, result['configuration'], result['platform'], figures_of(result), travel_factor(result),
                               result['valley_path_factor'])
  text = text.rstrip('\n') + '\n\n' + section
  with open(path, 'w', encoding='utf-8', newline='\n') as f:
    f.write(text)
  print(f'RunPerformance: recorded in {os.path.relpath(path, ROOT)}' + ('; the machine is pinned by this run' if pinned is None else ''))
  # The run that pins only reports; after it, a figure above its budget fails the run (DBG-208).
  return 1 if pinned is not None and above else 0


def self_test(tool, tuning_source):
  with tempfile.TemporaryDirectory() as folder:
    result = run_tool(tool, True, os.path.join(folder, 'quick.json'), tuning_source)
    figures = figures_of(result)
    keys = {key for _, key, _ in figures}
    missing = set(BUDGETS) - keys
    if missing:
      sys.exit(f'RunPerformance: the quick run gives no figure for {sorted(missing)}')
    path = os.path.join(folder, 'M1.md')
    reference = {key: f'fixture {key}' for key, _ in MACHINE_ITEMS}
    within = dict(result, scenes=[dict(s, step_us=dict(s['step_us'])) for s in result['scenes']])
    for s in within['scenes']:
      s['markers'] = {}
      s['step_us'] = {'mean': 1.0, 'p50': 1.0, 'p95': 1.0, 'max': 1.0}
    for e in within['travel']:
      e['advance_us'] = 0.0
    within['draw_ns'] = 1.0
    checks = [
      ('the first run pins the machine and only reports', record(path, reference, result, 'fixture', 'pin') == 0 and
       pinned_machine(open(path, encoding='utf-8').read()) == reference),
      ('a run on another machine stops', record(path, dict(reference, driver='other'), result, 'fixture', 'moved') == 2),
      ('a pinned run within every budget passes', record(path, reference, within, 'fixture', 'within') == 0),
    ]
    above = dict(within, draw_ns=1000.0)
    checks.append(('a pinned run above a budget fails', record(path, reference, above, 'fixture', 'above') == 1))
    failed = [name for name, ok in checks if not ok]
    for name, ok in checks:
      print(f'RunPerformance: fixture: {name}: {"ok" if ok else "FAILED"}')
    if failed:
      sys.exit(1)
    print(f'RunPerformance: the quick run gave {len(figures)} figures over {len(result["scenes"])} scenes and '
          f'{len(result["travel"])} travel edges')


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--skip-build', action='store_true')
  parser.add_argument('--tool', help='the SpeciesPerformance.exe to run (default: the Release|ARM64 build)')
  parser.add_argument('--record', default=RECORD)
  parser.add_argument('--quick', action='store_true')
  parser.add_argument('--self-test', action='store_true')
  parser.add_argument('--tuning-source')
  args = parser.parse_args()
  if args.self_test:
    if not args.tool:
      sys.exit('RunPerformance: --self-test needs --tool')
    self_test(args.tool, args.tuning_source)
    return 0
  if not args.skip_build:
    build()
  tool = args.tool or os.path.join(ROOT, PLATFORM, CONFIGURATION, 'SpeciesPerformance.exe')
  with tempfile.TemporaryDirectory() as folder:
    result = run_tool(tool, args.quick, os.path.join(folder, 'result.json'), args.tuning_source)
  for scene, key, value in figures_of(result):
    budget, unit, label = BUDGETS[key]
    print(f'RunPerformance: {scene}: {label}: {value:.4g} {unit} against {budget:g}: {status_of(value, budget)}')
  factor = travel_factor(result)
  if factor is not None:
    print(f'RunPerformance: travel: the largest per-edge median walking factor is {factor:.4f} '
          f'(wld.travel.valley_path_factor is {result["valley_path_factor"]:g})')
  if args.quick:
    print('RunPerformance: a quick run measures nothing and records nothing')
    return 0
  if (result['configuration'], result['platform']) != (CONFIGURATION, PLATFORM):
    print(f'RunPerformance: the tool is a {result["configuration"]}|{result["platform"]} build; only {CONFIGURATION}|{PLATFORM} '
          'is recorded (O69)')
    return 0
  when = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
  return record(args.record, machine(), result, identity(), when)


if __name__ == '__main__':
  sys.exit(main())
