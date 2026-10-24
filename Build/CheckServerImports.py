"""CheckServerImports.py: the headless server loads no presentation library (ADR-090 confirmation, PLT-005, ADR-100).

  python Build/CheckServerImports.py [--exe <path>] [--dumpbin <path>] [--listing <file>]

Runs `dumpbin /dependents` on SpeciesServer.exe (x64\\Debug by default) and fails when its imports or its delay-loaded
imports name a Direct3D, DXGI, GameInput or XAudio2 library. The import table is what the loader reads, so a library
that a static library pulls into the link is seen as well. dumpbin is the one given, else the one on PATH, else the
one in the toolset that VCToolsInstallDir names. --listing checks a saved dumpbin listing instead, and skips dumpbin.
Before the executable it runs its fixture: a listing that names d3d12.dll must be reported, and one that names only
system libraries must not.

From M4 task T11 (ADR-071, ADR-089, ADR-188, ADR-192) it also fails when the server imports msquic.dll, which the
transport loads itself at its first use and no project links, and, for a built executable, when the msquic.dll beside
it, which every program of the folder loads, is not the pinned package's copy for its platform, byte for byte.
"""
import argparse
import filecmp
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
# ADR-089: the pinned MsQuic package, restored by NeuronCore, whose build copies its msquic.dll beside the programs.
MSQUIC_PACKAGE = 'Microsoft.Native.Quic.MsQuic.Schannel'
MSQUIC_PIN = re.compile(r'<package id="' + re.escape(MSQUIC_PACKAGE) + r'" version="([^"]+)"')

FIXTURE_BAD = """Dump of file SpeciesServer.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    KERNEL32.dll
    d3d12.dll

  Image has the following delay load dependencies:

    GameInput.dll

  Summary
"""
FIXTURE_LINKED = """Dump of file SpeciesServer.exe

File Type: EXECUTABLE IMAGE

  Image has the following dependencies:

    msquic.dll
    KERNEL32.dll

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
  found = [f'imports {name}; the headless server links no presentation library (PLT-005, ADR-090)'
           for name in libraries(listing) if PRESENTATION_LIBRARY.match(name)]
  found += [f'imports {name}; the transport loads it at its first use, and no project links it (ADR-188)'
            for name in libraries(listing) if name.lower() == 'msquic.dll']
  return found


def pinned_msquic(platform):
  """The pinned package's msquic.dll for _platform, and the pinned version."""
  with open(os.path.join(ROOT, 'NeuronCore', 'packages.config'), encoding='utf-8-sig') as fh:
    pinned = MSQUIC_PIN.search(fh.read())
  version = pinned.group(1) if pinned else None
  return os.path.join(ROOT, 'packages', f'{MSQUIC_PACKAGE}.{version}', 'build', 'native', 'bin', platform, 'msquic.dll'), version


def msquic_findings(exe):
  """ADR-071 and ADR-089: the msquic.dll beside _exe is the pinned package's, for the platform its folder is built for."""
  folder = os.path.dirname(os.path.abspath(exe))
  platform = os.path.basename(os.path.dirname(folder)).lower()
  pinned, version = pinned_msquic(platform)
  beside = os.path.join(folder, 'msquic.dll')
  if version is None:
    return [f'NeuronCore\\packages.config pins no {MSQUIC_PACKAGE}']
  if not os.path.isfile(pinned):
    return [f'the pinned {MSQUIC_PACKAGE} {version} is not restored for {platform}']
  if not os.path.isfile(beside):
    return [f'no msquic.dll beside {os.path.basename(exe)}; the build copies the pinned one there (ADR-089)']
  if not filecmp.cmp(beside, pinned, shallow=False):
    return [f'the msquic.dll beside {os.path.basename(exe)} is not {MSQUIC_PACKAGE} {version}\'s for {platform} (ADR-089)']
  return []


def run_fixture():
  failures = []
  if len(findings(FIXTURE_BAD)) != 2:
    failures.append('the fixture: a listing that names d3d12.dll and GameInput.dll was not reported twice')
  if len(findings(FIXTURE_LINKED)) != 1:
    failures.append('the fixture: a listing that names msquic.dll was not reported once')
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
  if not args.listing:
    found += msquic_findings(args.exe)
  for finding in found:
    print(f'CheckServerImports: {subject}: {finding}')
  print(f'CheckServerImports: {subject} imports {len(libraries(listing))} libraries: {", ".join(libraries(listing))}; '
        f'{len(found)} findings')
  if not args.listing and not found:
    print(f'CheckServerImports: the msquic.dll beside it is the pinned {MSQUIC_PACKAGE} {pinned_msquic("x64")[1]}\'s')
  return 1 if found or failures else 0


if __name__ == '__main__':
  sys.exit(main())
