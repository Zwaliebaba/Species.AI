"""CheckWorldFrame.py: the render check of ADR-113 -- the world frame drawn on the WARP adapter with the debug layer silent.

  python Build/CheckWorldFrame.py [--exe <path>] [--frames <n>] [--seed <n>] [--world] [--ui-test] [--timeout <seconds>]

Runs the client's Debug build (x64\\Debug\\Species.exe by default) with `--seed <n> --render-check <n>`. The client starts
the battle sandbox, or with --world a new world with the steward's settlement (ADR-139, ADR-140), builds the world, and,
with --ui-test, the UI's test page over it (ADR-145), and draws that many world frames on the WARP adapter with the debug layer and GPU-based validation; it shows no dialog, and
it ends with exit code 0 only when every frame was drawn and the debug layer raised no message of warning severity or
above. This prints the run's log, then fails unless the client passed.
CI's hosted runner has no GPU; WARP is the one adapter that every Windows install holds.
"""
import argparse
import glob
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLIENT_EXE = os.path.join('x64', 'Debug', 'Species.exe')
LOG_LINES = 400 # the tail of the log that is printed


def newest_log(folder, before):
  logs = [p for p in glob.glob(os.path.join(folder, '*.log')) if p not in before]
  return max(logs, key=os.path.getmtime) if logs else None


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--exe', default=CLIENT_EXE, help='the client to run, relative to the repository root')
  parser.add_argument('--frames', type=int, default=40, help='the world frames to draw')
  parser.add_argument('--seed', default='1', help='the battle sandbox\'s seed')
  parser.add_argument('--world', action='store_true', help='a settlement world in place of the battle sandbox (ADR-140)')
  parser.add_argument('--ui-test', action='store_true', help='the UI\'s test page over the world (ADR-145)')
  parser.add_argument('--timeout', type=int, default=900, help='seconds before the run is stopped')
  args = parser.parse_args()

  exe = os.path.join(ROOT, args.exe)
  if not os.path.exists(exe):
    print(f'CheckWorldFrame: {args.exe} does not exist; build Debug|x64 first')
    return 1
  logs = os.path.join(os.path.dirname(exe), 'logs')
  before = set(glob.glob(os.path.join(logs, '*.log')))
  command = [exe, '--seed', args.seed, '--render-check', str(args.frames)] + (['--world'] if args.world else []) + \
    (['--ui-test'] if args.ui_test else [])
  try:
    run = subprocess.run(command, cwd=os.path.dirname(exe), timeout=args.timeout)
    code = run.returncode
  except subprocess.TimeoutExpired:
    print(f'CheckWorldFrame: the client did not end within {args.timeout} s')
    code = None
  log = newest_log(logs, before)
  if log is None:
    print('CheckWorldFrame: the client wrote no log')
  else:
    with open(log, encoding='utf-8', errors='replace') as file:
      lines = file.read().splitlines()
    print(f'CheckWorldFrame: the last {min(len(lines), LOG_LINES)} lines of {os.path.relpath(log, ROOT)}')
    print('\n'.join(lines[-LOG_LINES:]))
  if code != 0:
    print(f'CheckWorldFrame: the render check failed (exit code {code}): {" ".join(command)}')
    return 1
  print(f'CheckWorldFrame: {args.frames} world frames drawn on WARP with the debug layer silent')
  return 0


if __name__ == '__main__':
  sys.exit(main())
