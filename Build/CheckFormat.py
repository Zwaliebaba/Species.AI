"""CheckFormat.py: the clang-format gate of AGENTS.md §4 (BLD-091, table 4.9-A).

Checks every hand-written C++ file of the tree, `.h` and `.cpp`, against /.clang-format, and fails on any file that
clang-format would change. `CompiledShader/` is build output and `packages/` is restored, so neither is checked.

  python3 Build/CheckFormat.py [--clang-format <executable>] [--fix]

--fix rewrites the offending files in place. The version is pinned (bld.pin.clang_format): 18 and 22 break a long
argument list in different places, so a run with another major version fails rather than report a layout that CI
would disagree with. `python -m pip install clang-format==18.1.3` gets the pinned version on any machine.

Before the tree, the gate checks itself: a deliberately misformatted fixture must be reported, so a clang-format that
silently ignores /.clang-format cannot pass the tree.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINNED_MAJOR = 18                      # bld.pin.clang_format
EXTENSIONS = ('.h', '.cpp')
SKIPPED_PARTS = ('CompiledShader', 'packages', 'x64', 'ARM64', 'node_modules')
BATCH = 64
FIXTURE = 'namespace Fixture {\nint  Misformatted( int _a ){ return _a; }\n}\n'


def tracked_sources():
  """The C++ files git knows about or would add, so that a new file is checked before it is committed."""
  output = subprocess.run(['git', 'ls-files', '--cached', '--others', '--exclude-standard'], cwd=ROOT, check=True,
                          capture_output=True, text=True).stdout
  files = []
  for path in output.splitlines():
    parts = path.split('/')
    if os.path.splitext(path)[1] in EXTENSIONS and not any(p in SKIPPED_PARTS for p in parts):
      if os.path.exists(os.path.join(ROOT, path)):
        files.append(path)
  return sorted(set(files))


def check_version(executable):
  try:
    version = subprocess.run([executable, '--version'], check=True, capture_output=True, text=True).stdout.strip()
  except (OSError, subprocess.CalledProcessError) as error:
    sys.exit(f'CheckFormat: cannot run {executable}: {error}')
  m = re.search(r'version (\d+)\.', version)
  print(f'CheckFormat: {version}')
  if not m or int(m.group(1)) != PINNED_MAJOR:
    sys.exit(f'CheckFormat: clang-format {PINNED_MAJOR} is required (bld.pin.clang_format); got "{version}". '
             f'Install it with: python -m pip install clang-format=={PINNED_MAJOR}.1.3')


def check_fixture(executable):
  style = '--style=file:' + os.path.join(ROOT, '.clang-format')
  result = subprocess.run([executable, '--dry-run', '--Werror', style, '--assume-filename=Fixture.cpp'],
                          input=FIXTURE, capture_output=True, text=True)
  if result.returncode == 0:
    sys.exit('CheckFormat: the misformatted fixture was not reported, so this clang-format does not apply '
             '/.clang-format; the tree is not checked.')
  print('CheckFormat: the misformatted fixture is reported')


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--clang-format', default='clang-format', help='the clang-format executable')
  parser.add_argument('--fix', action='store_true', help='rewrite the offending files in place')
  args = parser.parse_args()
  check_version(args.clang_format)
  check_fixture(args.clang_format)
  files = tracked_sources()
  style = '--style=file:' + os.path.join(ROOT, '.clang-format')
  offenders = []
  for start in range(0, len(files), BATCH):
    batch = files[start:start + BATCH]
    result = subprocess.run([args.clang_format, '--dry-run', '--Werror', style] + batch, cwd=ROOT,
                            capture_output=True, text=True)
    if result.returncode != 0:
      for path in batch:
        one = subprocess.run([args.clang_format, '--dry-run', '--Werror', style, path], cwd=ROOT,
                             capture_output=True, text=True)
        if one.returncode != 0:
          offenders.append((path, one.stderr.strip()))
  if offenders and args.fix:
    subprocess.run([args.clang_format, '-i', style] + [p for p, _ in offenders], cwd=ROOT, check=True)
    print(f'CheckFormat: rewrote {len(offenders)} of {len(files)} files')
    return 0
  for path, report in offenders:
    print(report)
  if offenders:
    print(f'CheckFormat: {len(offenders)} of {len(files)} files are not formatted; run python Build/CheckFormat.py '
          '--fix and commit the result.')
    return 1
  print(f'CheckFormat: {len(files)} files formatted')
  return 0


if __name__ == '__main__':
  sys.exit(main())
