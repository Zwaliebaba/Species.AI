"""RunClangTidy.py: the clang-tidy gate of AGENTS.md §1 and §6 (BLD-091, table 4.9-A).

  python Build/RunClangTidy.py [--clang-tidy <executable>] [--jobs <n>]

Runs clang-tidy over every hand-written translation unit of the solution, with /.clang-tidy, through clang's MSVC
driver and the switches each .vcxproj sets for Debug, and fails on any finding. It needs the MSVC environment of a
Developer PowerShell (the INCLUDE variable is what makes the Windows SDK and the standard library visible to clang),
which the Windows CI job imports before it runs this script.

The version is pinned (bld.pin.clang_tidy, CLANG_TIDY_VERSION in .github/workflows/build.yml): a run with another
major version fails rather than report findings CI would not. `python -m pip install clang-tidy==22.1.8` gets it.

Before the tree, the gate checks itself: a fixture with a global variable that lacks its `g_` prefix must be
reported, so a clang-tidy that silently ignores /.clang-tidy cannot pass the tree.

The job runs without a build, so the shader headers that FXCompile writes into <Lib>/CompiledShader/ (AGENTS.md §2)
do not exist: for each FXCompile item a stand-in header declares its byte array, in a system include folder of the
run, so the translation unit that binds the pipeline states is checked like any other.
"""
import argparse
import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import SolutionModel  # noqa: E402  (the path above makes the sibling module importable)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINNED_MAJOR = 22                      # bld.pin.clang_tidy (22.1.8)
CONFIGURATION = 'Debug'                # CI builds Debug only (AGENTS.md §6)

# The include folder of each package that R14 allows, inside the restored package. A package that a project restores
# and that is not listed here stops the run, so that a new package cannot be linted without its headers.
PACKAGE_INCLUDES = {
  'Microsoft.Direct3D.D3D12': ['build/native/include'],
  'WinPixEventRuntime': ['Include/WinPixEventRuntime'],
  'Microsoft.Native.Quic.MsQuic.Schannel': ['build/native/include'],
}

FIXTURE = 'int badGlobal = 0;\n'


def check_version(executable):
  try:
    version = subprocess.run([executable, '--version'], check=True, capture_output=True, text=True).stdout
  except (OSError, subprocess.CalledProcessError) as error:
    sys.exit(f'RunClangTidy: cannot run {executable}: {error}')
  m = re.search(r'version (\d+)\.(\d+)\.(\d+)', version)
  found = m.group(0) if m else version.strip()
  print(f'RunClangTidy: clang-tidy {found}')
  if not m or int(m.group(1)) != PINNED_MAJOR:
    sys.exit(f'RunClangTidy: clang-tidy {PINNED_MAJOR} is required (bld.pin.clang_tidy); got "{found}". Install it '
             'with: python -m pip install clang-tidy==22.1.8')


def expand(directory):
  vc = os.environ.get('VCINSTALLDIR', '')
  value = directory.replace('$(SolutionDir)', ROOT + os.sep).replace('$(VCInstallDir)', vc)
  if '$(' in value:
    sys.exit(f'RunClangTidy: cannot expand the include directory {directory}')
  return os.path.normpath(value)


def arguments(project):
  """The clang-cl switches of a project's Debug configuration: the language, the defines and the include path."""
  args = ['--driver-mode=cl', '/std:c++latest', '/permissive-', '/EHsc', '/utf-8']
  if project.setting(CONFIGURATION, 'Property.CharacterSet') == 'Unicode':
    args += ['/DUNICODE', '/D_UNICODE']
  args += ['/D' + d for d in project.preprocessor_definitions(CONFIGURATION)]
  for directory in project.include_directories(CONFIGURATION):
    path = expand(directory)
    # Headers outside the solution's own projects are system headers to clang, as the SDK's are: their findings and
    # the macros they define are not this repository's to lint.
    if directory.startswith('$(SolutionDir)') and not directory.startswith('$(SolutionDir)packages'):
      args += ['/I' + path]
    else:
      args += ['/imsvc', path]
  for package, version in project.packages:
    if package not in PACKAGE_INCLUDES:
      sys.exit(f'RunClangTidy: {project.name} restores {package}, whose include folder this script does not know; '
               'add it to PACKAGE_INCLUDES')
    for folder in PACKAGE_INCLUDES[package]:
      args += ['/imsvc', os.path.join(ROOT, 'packages', f'{package}.{version}', *folder.split('/'))]
  return args


def write_shader_stand_ins(projects, folder):
  """For each FXCompile item, CompiledShader/<Name>.h declaring the byte array g_<Name> that the build would write."""
  written = 0
  for project in projects:
    for shader in project.sources(kinds=('FXCompile',)):
      name = os.path.splitext(os.path.basename(shader.replace('\\', '/')))[0]
      os.makedirs(os.path.join(folder, 'CompiledShader'), exist_ok=True)
      with open(os.path.join(folder, 'CompiledShader', name + '.h'), 'w', encoding='utf-8') as fh:
        fh.write(f'#pragma once\n// A stand-in for the header FXCompile writes (RunClangTidy.py).\nconst unsigned char g_{name}[] = {{0}};\n')
      written += 1
  return written


def run(executable, path, args):
  command = [executable, '--quiet', '--config-file=' + os.path.join(ROOT, '.clang-tidy'), path, '--'] + args
  result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
  return path, result.returncode, (result.stdout + result.stderr).strip()


def check_fixture(executable):
  with tempfile.TemporaryDirectory(prefix='RunClangTidy.') as scratch:
    path = os.path.join(scratch, 'Fixture.cpp')
    with open(path, 'w', encoding='utf-8') as fh:
      fh.write(FIXTURE)
    _, code, output = run(executable, path, ['--driver-mode=cl', '/std:c++latest'])
  if code == 0 or 'readability-identifier-naming' not in output:
    sys.exit('RunClangTidy: the fixture\'s unprefixed global was not reported, so this clang-tidy does not apply '
             f'/.clang-tidy; the tree is not checked.\n{output}')
  print('RunClangTidy: the fixture\'s unprefixed global is reported')


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--clang-tidy', default='clang-tidy', help='the clang-tidy executable')
  parser.add_argument('--jobs', type=int, default=os.cpu_count() or 4, help='parallel clang-tidy runs')
  args = parser.parse_args()
  if not os.environ.get('INCLUDE'):
    sys.exit('RunClangTidy: INCLUDE is not set; run this from a Developer PowerShell (AGENTS.md §3)')
  check_version(args.clang_tidy)
  check_fixture(args.clang_tidy)
  solution, projects = SolutionModel.load(ROOT)
  if solution is None:
    sys.exit('RunClangTidy: there is not exactly one solution at the root')
  failures = []
  with tempfile.TemporaryDirectory(prefix='RunClangTidy.') as stand_ins:
    shaders = write_shader_stand_ins(projects, stand_ins)
    units = []
    for project in projects:
      project_args = arguments(project) + ['/imsvc', stand_ins]
      for include in project.sources(kinds=('ClCompile',)):
        if include.endswith('.cpp') and not include.startswith('CompiledShader/'):
          units.append((os.path.join(project.directory, include), project_args))
    print(f'RunClangTidy: {shaders} compiled-shader stand-ins')
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
      for path, code, output in pool.map(lambda unit: run(args.clang_tidy, *unit), units):
        if code != 0 or output:
          failures.append((path, output))
  for path, output in sorted(failures):
    print(f'--- {path}\n{output}')
  print(f'RunClangTidy: {len(units)} translation units of {solution}, {len(failures)} with findings')
  return 1 if failures else 0


if __name__ == '__main__':
  sys.exit(main())
