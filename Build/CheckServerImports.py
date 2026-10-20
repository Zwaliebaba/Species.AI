"""CheckServerImports.py: the headless server loads no presentation library (ADR-090 confirmation, PLT-005, ADR-100).

  python Build/CheckServerImports.py [--exe <path>] [--dumpbin <path>] [--listing <file>]

Runs `dumpbin /dependents` on SpeciesServer.exe (x64\\Debug by default) and fails when its imports or its delay-loaded
imports name a Direct3D, DXGI, GameInput or XAudio2 library. The import table is what the loader reads, so a library
that a static library pulls into the link is seen as well. dumpbin is the one given, else the one on PATH, else the
one in the toolset that VCToolsInstallDir names. --listing checks a saved dumpbin listing instead, and skips dumpbin.
Before the executable it runs its fixture: a listing that names d3d12.dll must be reported, and one that names only
system libraries must not.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER_EXE = os.path.join('x64', 'Debug', 'SpeciesServer.exe')
# PLT-005: no window, display, input-forwarding or audio-endpoint code; ADR-090: no Direct3D, DXGI or GameInput.
PRESENTATION_LIBRARY = re.compile(r'^(?:d3d\w*|dxgi\w*|dxcore|gameinput\w*|xaudio2\w*|xinput\w*)\.dll$', re.IGNORECASE)
LISTED_LIBRARY = re.compile(r'^\s+(\S+\.dll)\s*$', re.IGNORECASE | re.MULTILINE)

FIXTURE_BAD = """Dump of file SpeciesServer.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    KERNEL32.dll
    d3d12.dll

  Image has the following delay load dependencies:

    GameInput.dll

  Summary
"""
FIXTURE_GOOD = """Dump of file SpeciesServer.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    WinPixEventRuntime.dll
    bcrypt.dll
    KERNEL32.dll
    USER32.dll

  Summary
"""


def libraries(listing):
  return LISTED_LIBRARY.findall(listing)


def findings(listing):
  return [f'imports {name}; the headless server links no presentation library (PLT-005, ADR-090)'
          for name in libraries(listing) if PRESENTATION_LIBRARY.match(name)]


def run_fixture():
  failures = []
  if len(findings(FIXTURE_BAD)) != 2:
    failures.append('the fixture: a listing that names d3d12.dll and GameInput.dll was not reported twice')
  if findings(FIXTURE_GOOD) or len(libraries(FIXTURE_GOOD)) != 4:
    failures.append('the fixture: a listing of system libraries was not read as four libraries and no finding')
  return failures


def find_dumpbin(given):
  if given:
    return given
  on_path = shutil.which('dumpbin')
  if on_path:
    return on_path
  tools = os.environ.get('VCToolsInstallDir', '')
  candidate = os.path.join(tools, 'bin', 'Hostx64', 'x64', 'dumpbin.exe')
  return candidate if tools and os.path.isfile(candidate) else None


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--exe', default=os.path.join(ROOT, SERVER_EXE), help='the executable to check')
  parser.add_argument('--dumpbin', help='the dumpbin.exe to run')
  parser.add_argument('--listing', help='a saved `dumpbin /dependents` listing to check in place of the executable')
  args = parser.parse_args()

  failures = run_fixture()
  for failure in failures:
    print(f'CheckServerImports: {failure}')
  if args.listing:
    with open(args.listing, encoding='utf-8', errors='replace') as fh:
      listing = fh.read()
    subject = args.listing
  else:
    if not os.path.isfile(args.exe):
      print(f'CheckServerImports: {args.exe} was not built')
      return 1
    dumpbin = find_dumpbin(args.dumpbin)
    if not dumpbin:
      print('CheckServerImports: no dumpbin on PATH or under VCToolsInstallDir; run it from a Developer PowerShell')
      return 1
    run = subprocess.run([dumpbin, '/nologo', '/dependents', args.exe], capture_output=True, text=True, errors='replace')
    if run.returncode != 0:
      print(f'CheckServerImports: dumpbin failed with exit code {run.returncode}\n{run.stdout}{run.stderr}')
      return 1
    listing = run.stdout
    subject = os.path.relpath(args.exe, ROOT)
  found = findings(listing)
  for finding in found:
    print(f'CheckServerImports: {subject}: {finding}')
  print(f'CheckServerImports: {subject} imports {len(libraries(listing))} libraries: {", ".join(libraries(listing))}; '
        f'{len(found)} findings')
  return 1 if found or failures else 0


if __name__ == '__main__':
  sys.exit(main())
