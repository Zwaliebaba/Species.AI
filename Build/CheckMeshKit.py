"""CheckMeshKit.py: the drift check of the mesh kit (M2 task T2, ADR-128).

  python Build/CheckMeshKit.py [--tool <SpeciesMeshKit.exe>]

Builds the mesh kit through the solution (/t:Tools\\SpeciesMeshKit, Debug|x64), runs it twice, each run into a
temporary folder of its own, and fails unless the two runs write the same files byte for byte and every file is the one
committed under GameData/Meshes/. A file the kit writes that is not committed, or that differs from the committed one,
is a finding: a recipe changed without its output being committed, or a host that rounds the recipe differently. The
check writes nothing into the checkout. --tool runs a kit built elsewhere, and skips the build. Before the runs it runs
its fixture: a changed copy of a written file must be reported.
"""
import argparse
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import SolutionModel  # noqa: E402  (the path above makes the sibling module importable)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KIT_TARGET = 'Tools\\SpeciesMeshKit'
KIT_EXE = os.path.join(ROOT, 'x64', 'Debug', 'SpeciesMeshKit.exe')
MESH_FOLDER = os.path.join(ROOT, 'GameData', 'Meshes')


def folder_bytes(folder):
  files = {}
  for name in sorted(os.listdir(folder)):
    with open(os.path.join(folder, name), 'rb') as fh:
      files[name] = fh.read()
  return files


def compare(written, committed_folder):
  findings = []
  for name, data in sorted(written.items()):
    path = os.path.join(committed_folder, name)
    if not os.path.isfile(path):
      findings.append(f'GameData/Meshes/{name}: written by the kit, but not committed (run SpeciesMeshKit and commit its output)')
      continue
    with open(path, 'rb') as fh:
      if fh.read() != data:
        findings.append(f'GameData/Meshes/{name}: differs from what the kit writes now')
  return findings


def run_fixture(written):
  if not written:
    return ['the kit wrote no file']
  name, data = next(iter(written.items()))
  with tempfile.TemporaryDirectory(prefix='CheckMeshKit.fixture.') as folder:
    with open(os.path.join(folder, name), 'wb') as fh:
      fh.write(data + b'\0')
    if not compare({name: data}, folder):
      return [f'the fixture: a changed copy of {name} was not reported']
  return []


def build():
  solutions = SolutionModel.find_solutions(ROOT)
  if len(solutions) != 1:
    return f'expected one solution at the root, found {len(solutions)}'
  command = ['msbuild', solutions[0], f'/t:{KIT_TARGET}', '/p:Configuration=Debug', '/p:Platform=x64', '/m', '/nologo', '/v:minimal']
  if subprocess.run(command, cwd=ROOT).returncode != 0:
    return f'the mesh kit did not build: {" ".join(command)}'
  return None


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--tool', help='a mesh kit built elsewhere; the build is skipped')
  args = parser.parse_args()
  tool = os.path.abspath(args.tool) if args.tool else KIT_EXE
  if not args.tool:
    error = build()
    if error:
      print(f'CheckMeshKit: {error}')
      return 1
  runs = []
  with tempfile.TemporaryDirectory(prefix='CheckMeshKit.') as work:
    for n in (1, 2):
      out = os.path.join(work, f'run{n}')
      result = subprocess.run([tool, '--root', ROOT, '--out', out], capture_output=True, text=True)
      if result.returncode != 0:
        print(result.stdout + result.stderr)
        print(f'CheckMeshKit: the kit failed with exit code {result.returncode}')
        return 1
      runs.append(folder_bytes(out))
  findings = run_fixture(runs[0])
  if runs[0] != runs[1]:
    differing = sorted(k for k in set(runs[0]) | set(runs[1]) if runs[0].get(k) != runs[1].get(k))
    findings.append(f'two runs differ in {", ".join(differing)}')
  findings += compare(runs[0], MESH_FOLDER)
  for finding in findings:
    print(f'CheckMeshKit: {finding}')
  print(f'CheckMeshKit: the kit writes {len(runs[0])} files; {len(findings)} findings')
  return 1 if findings else 0


if __name__ == '__main__':
  sys.exit(main())
