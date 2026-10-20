"""CheckProjectFiles.py: the build-shape gate of AGENTS.md §1, §2 and §6 (BLD-123, table 4.1-B).

  python Build/CheckProjectFiles.py [--skip-fixtures]

Checks the solution and every project of it against the rules of table 4.1-B of the build tooling component, and
prints each finding as `rule <n>: <path>: <message>`. Before the tree it runs its fixtures (T39): a copy of the tree is
broken once per fixture, and each break must be reported under the rule it breaks, so a rule that has stopped firing
fails the gate instead of passing it quietly. --skip-fixtures checks the tree alone.

Rule 14 holds the source rules of other components. Those whose code does not exist yet are listed in
`NOT_YET_CHECKED`, printed on every run, and each is added here in the change that brings the code it governs.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ElementTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import SolutionModel  # noqa: E402  (the path above makes the sibling module importable)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINS_DOCUMENT = 'Design/architecture/components/build_tooling_and_distribution/README.md'
REQUIREMENTS_INDEX = 'Design/requirements_index'

RULES = {
  1: 'solution, platform, configurations and toolset',
  2: 'Debug and Release alignment and the required settings',
  3: 'registration in the solution, the .vcxproj, the .filters and .clang-tidy',
  4: 'flat project folders',
  5: 'file kinds and names',
  6: 'type-name affixes and spellings',
  7: 'shader files',
  8: 'switches and the Windows macro family',
  9: 'packages and pins',
  10: 'include directories',
  11: 'references and edges',
  12: 'test projects',
  13: 'includes across projects',
  14: 'source rules of the components',
}

NOT_YET_CHECKED = [
  'BLD-076: an embedded map hash (no map code exists yet)',
  'DAT-277: a mesh socket looked up by name text (no mesh code exists yet)',
  'AEN-025, AEN-003: allocator, lock, logger or voice calls in an audio callback; XAudio2 includes outside the voice '
  'back-end (no audio code exists yet)',
  'VFX-005, UI-002, DEC-222: SimRng.h or a writable simulation header included outside the simulation (no such '
  'header exists yet)',
]

# ADR-090 table 1: the projects of the solution and the projects each may reference directly. A program may also
# reference what the libraries it names reference.
LAYOUT = {
  'NeuronCore': set(),
  'NeuronClient': {'NeuronCore'},
  'NeuronServer': {'NeuronCore'},
  'GameShared': {'NeuronCore'},
  'GameLogic': {'GameShared', 'NeuronServer', 'NeuronCore'},
  'GameClient': {'GameShared', 'NeuronClient', 'NeuronCore'},
  'Species': {'GameClient', 'GameLogic'},
  'SpeciesServer': {'GameLogic'},
}
LIBRARIES = {'NeuronCore', 'NeuronClient', 'NeuronServer', 'GameShared', 'GameLogic', 'GameClient'}
PROGRAMS = {'Species', 'SpeciesServer'}
SERVER = 'SpeciesServer'
PRESENTATION_PROJECTS = {'NeuronClient', 'GameClient'}
PRESENTATION_LIBRARIES = ('d3d12', 'dxgi', 'xaudio2', 'x3daudio', 'gameinput', 'd2d1', 'dwrite')
SHADER_PROJECT = 'NeuronClient'                           # ADR-090 item 1

# R14 and BLD-038: the packages a game or engine project may restore, with the tuning key of each pin.
ALLOWED_PACKAGES = {
  'Microsoft.Direct3D.D3D12': 'bld.pin.agility_sdk',
  'Microsoft.GameInput': 'bld.pin.gameinput',
  'WinPixEventRuntime': 'bld.pin.pix_event_runtime',
  'Microsoft.Native.Quic.MsQuic.Schannel': 'bld.pin.msquic',
}
PRESENTATION_PACKAGES = {'Microsoft.Direct3D.D3D12', 'Microsoft.GameInput'}  # BLD-045

# BLD-031 and AGENTS.md §3: the settings that may differ between Debug and Release.
ALLOWED_DIFFERENCES = {
  'ClCompile.Optimization', 'ClCompile.PreprocessorDefinitions', 'ClCompile.FunctionLevelLinking',
  'ClCompile.IntrinsicFunctions', 'ClCompile.RuntimeLibrary', 'ClCompile.WholeProgramOptimization',
  'Property.UseDebugLibraries', 'Property.LinkIncremental', 'Property.WholeProgramOptimization',
  'Link.EnableCOMDATFolding', 'Link.OptimizeReferences', 'Link.LinkTimeCodeGeneration', 'Lib.LinkTimeCodeGeneration',
}

# BLD-028, BLD-030, BLD-040: what every project states, identically in both configurations. BasicRuntimeChecks and
# SupportJustMyCode are here because MSBuild turns them on in Debug alone unless they are stated (BLD-031).
REQUIRED_COMPILE = {
  'PrecompiledHeader': 'Use', 'PrecompiledHeaderFile': 'pch.h', 'LanguageStandard': 'stdcpplatest',
  'ConformanceMode': 'true', 'WarningLevel': 'Level4', 'TreatWarningAsError': 'true', 'FloatingPointModel': 'Precise',
  'ExceptionHandling': 'Sync',
  'DebugInformationFormat': 'ProgramDatabase', 'ControlFlowGuard': 'Guard', 'BufferSecurityCheck': 'true',
  'BasicRuntimeChecks': 'Default', 'SupportJustMyCode': 'false',
}
REQUIRED_COMPILE_OPTIONS = ('/Zc:__cplusplus', '/Zc:preprocessor', '/utf-8')
REQUIRED_LINK = {
  'GenerateDebugInformation': 'DebugFull', 'RandomizedBaseAddress': 'true', 'DataExecutionPrevention': 'true',
}
REQUIRED_LINK_OPTIONS = ('/HIGHENTROPYVA', '/GUARD:CF')
# R16 and ADR-094: the instruction set is stated per platform, and CET shadow stacks exist on x64 alone. These are the
# only settings in which the x64 and ARM64 builds of one configuration differ.
REQUIRED_COMPILE_BY_PLATFORM = {
  'x64': {'EnableEnhancedInstructionSet': 'AdvancedVectorExtensions2'},
  'ARM64': {'EnableEnhancedInstructionSet': 'CPUExtensionRequirementsARMv87'},
}
REQUIRED_LINK_BY_PLATFORM = {
  'x64': {'CETCompat': 'true'},
  'ARM64': {'CETCompat': 'false'},
}
PLATFORM_DIFFERENCES = {'ClCompile.EnableEnhancedInstructionSet', 'Link.CETCompat'}
REQUIRED_BY_CONFIGURATION = {
  'Debug': {'ClCompile.RuntimeLibrary': 'MultiThreadedDebugDLL', 'Property.UseDebugLibraries': 'true'},
  'Release': {'ClCompile.RuntimeLibrary': 'MultiThreadedDLL', 'Property.UseDebugLibraries': 'false'},
}
REQUIRED_LINK_BY_CONFIGURATION = {
  'Debug': {'Property.LinkIncremental': 'true'},
  'Release': {'Link.EnableCOMDATFolding': 'true', 'Link.OptimizeReferences': 'true',
              'Link.LinkTimeCodeGeneration': 'UseLinkTimeCodeGeneration', 'Property.LinkIncremental': 'false'},
}

WINDOWS_MACROS = ('NOMINMAX', 'WIN32_LEAN_AND_MEAN', 'NOMCX', 'NOSERVICE', 'NOHELP')

# PLT-003: the operating-system services of the platform layer's table 1-A that no other project calls directly. The
# platform layer is NeuronCore (ADR-090 table 1); the window is NeuronClient's, so it may create one too.
PLATFORM_PROJECT = 'NeuronCore'
WINDOW_PROJECTS = {'NeuronCore', 'NeuronClient'}
# ADR-083 item 1: no header of the server or simulation libraries includes a Windows header; they reach the system
# through NeuronCore's platform layer only.
HEADLESS_PROJECTS = {'NeuronServer', 'GameShared', 'GameLogic'}
# R-SIM-019, R-SIM-730: simulation code holds no mutable static state. .clang-tidy names every mutable global and
# static variable g_ and every mutable static member sm_, so such a name in a simulation source is such a variable.
SIMULATION_PROJECTS = {'GameShared', 'GameLogic'}
MUTABLE_STATIC_NAME = re.compile(r'\b(?:g|sm)_[A-Za-z]\w*')
PLATFORM_SERVICES = (
  (r'\bQueryPerformance(?:Counter|Frequency)\b', 'reads the performance counter (ADR-074)'),
  (r'\b(?:GetTickCount(?:64)?|timeGetTime|timeBeginPeriod)\b', 'reads a tick-count or multimedia timer (ADR-074)'),
  (r'\b(?:steady_clock|system_clock|high_resolution_clock|utc_clock|tai_clock|gps_clock|file_clock)\s*::\s*now\b',
   'reads a standard-library clock (ADR-074)'),
  (r'\bSHGetKnownFolderPath\b|\bSHGetFolderPath[AW]?\b', 'looks up a known folder'),
  (r'\bWer(?:Register|Unregister)\w*', 'registers with Windows Error Reporting'),
  (r'\bstd::j?thread\b(?!\s*::)|\b(?:CreateThread|CreateRemoteThread|_beginthreadex|_beginthread)\b',
   'creates a thread other than through PlatformThread'),
)
WINDOW_CREATION = r'\bCreateWindow(?:Ex)?[AW]?\b'
RENDERER_PROJECT = 'NeuronClient'                         # ADR-078: no other project includes Direct3D 12 or DXGI
DIRECT3D_HEADER = re.compile(r'^(?:d3d12|dxgi|d3dx12)[a-z0-9_]*\.h$', re.IGNORECASE)
CLIENT_FORMAT_HEADER = 'dxgiformat.h'                     # ADR-078: what the factory's clients see of DXGI
PRAGMA_LIBRARY = re.compile(r'^\s*#\s*pragma\s+comment\s*\(\s*lib\s*,\s*"([^"]+)"', re.MULTILINE)
SOCKET_HEADERS = {'winsock.h', 'winsock2.h', 'ws2tcpip.h', 'mswsock.h', 'ws2def.h', 'afunix.h'}  # ADR-088 item 1, ADR-089
AGILITY_PACKAGE = 'Microsoft.Direct3D.D3D12'
AGILITY_EXPORTS = 'Species/AgilitySdkExports.cpp'         # BLD-042: the constants Species.exe exports
AGILITY_EXPORT_OPTIONS = ('/EXPORT:D3D12SDKVersion=AGILITY_SDK_VERSION,DATA', '/EXPORT:D3D12SDKPath=AGILITY_SDK_PATH,DATA')
AGILITY_SDK_PATH = r'".\\D3D12\\"'                     # bld.pin.d3d12_sdk_path, as a C++ literal
PIX_FILE = 'NeuronCore/Profile.cpp'                       # ADR-068, DBG-080: no other code calls the PIX runtime
WINDOWS_HEADERS = 'NeuronCore/WindowsHeaders.h'           # PLT-132
FILE_KIND_DEFAULTS = {'Source Files', 'Header Files', 'Resource Files'}
SOURCE_EXTENSIONS = ('.h', '.cpp')
SHADER_EXTENSIONS = ('.hlsl', '.hlsli')
BANNED_EXTENSIONS = ('.hpp', '.hh', '.hxx', '.h++', '.cc', '.cxx', '.c++', '.c', '.inl', '.ipp', '.tpp', '.ixx',
                     '.cppm', '.mpp', '.mxx')
R7_EXCEPTIONS = {'pch', 'framework', 'targetver', 'Resource'}
# R7 (owner decision O65): a program project also holds its application manifest and, with an icon, its resource script,
# each named for the program and registered as the item kind MSBuild compiles it with.
PROGRAM_FILE_KINDS = {'.manifest': 'Manifest', '.rc': 'ResourceCompile'}
PASCAL_CASE = re.compile(r'^[A-Z][A-Za-z0-9]*$')
SHADER_NAME = re.compile(r'^[A-Z][A-Za-z0-9]*(VS|PS|CS)\.hlsl$')
SHADER_INCLUDE_NAME = re.compile(r'^[A-Z][A-Za-z0-9]*\.hlsli$')
TYPE_DEFINITION = re.compile(
  r'\b(?:class|struct|union|enum\s+class|enum\s+struct|enum)\s+(?:\[\[[^\]]*\]\]\s*)?([A-Za-z_]\w*)\s*(?:final\s*)?[:{]'
  r'|\busing\s+([A-Za-z_]\w*)\s*='
  r'|\bconcept\s+([A-Za-z_]\w*)\s*=')
R2_AFFIX = re.compile(r'^(?:[ICSE][A-Z][a-z]|Base[A-Z]|Abstract[A-Z])|(?:Base|Abstract|Impl|_t)$')
R11_BRITISH = ('colour', 'initialis', 'serialis', 'normalis', 'quantis', 'synchronis', 'behaviour', 'neighbour',
               'centre', 'grey', 'cancell')
IDENTIFIER = re.compile(r'[A-Za-z_]\w*')
REQUIREMENT_ATTRIBUTE = re.compile(r'TEST_METHOD_ATTRIBUTE\s*\(\s*L"Requirement"\s*,\s*L"([^"]*)"\s*\)')
MODULE_LINE = re.compile(r'^\s*(?:export\s+)?(?:import\s+(?:std\b|<)|module\b)', re.MULTILINE)


class Tree:
  """The checked tree: its files, its solution and its projects."""

  def __init__(self, root):
    self.root = root
    self.files = tree_files(root)
    self.git = os.path.isdir(os.path.join(root, '.git'))
    self.solutions = SolutionModel.find_solutions(root)
    self.solution = self.solutions[0] if len(self.solutions) == 1 else None
    self.entries = SolutionModel.read_solution(root, self.solution) if self.solution else []
    self.projects = []
    self.parse_errors = []
    for folder, path in self.entries:
      if os.path.exists(os.path.join(root, path)):
        try:
          self.projects.append(SolutionModel.Project(root, path, folder))
        except ElementTree.ParseError as error:
          self.parse_errors.append((path, str(error)))
    self.by_name = {p.name: p for p in self.projects}

  def read(self, path):
    with open(os.path.join(self.root, path), encoding='utf-8', errors='replace') as fh:
      return fh.read()

  def project_files(self, project):
    prefix = project.directory + '/'
    return [f[len(prefix):] for f in self.files if f.startswith(prefix)]

  def closure(self, name):
    """The projects a project reaches through its references."""
    seen = set()
    stack = [name]
    while stack:
      current = self.by_name.get(stack.pop())
      for r in current.references if current else []:
        if r not in seen:
          seen.add(r)
          stack.append(r)
    return seen


def tree_files(root):
  """The files of the tree: what git tracks or would add in a checkout, every file in a fixture copy."""
  if os.path.isdir(os.path.join(root, '.git')):
    output = subprocess.run(['git', 'ls-files', '--cached', '--others', '--exclude-standard'], cwd=root, check=True,
                            capture_output=True, text=True).stdout
    return sorted(p for p in output.splitlines() if os.path.exists(os.path.join(root, p)))
  out = []
  for directory, folders, names in os.walk(root):
    folders[:] = [f for f in folders if f not in ('.git', 'x64', 'ARM64', 'packages', 'node_modules', 'CompiledShader')]
    for name in names:
      out.append(os.path.relpath(os.path.join(directory, name), root).replace('\\', '/'))
  return sorted(out)


def allowed_references(name):
  """ADR-090 table 1, with a program's libraries' own references added."""
  if name not in LAYOUT:
    return None
  allowed = set(LAYOUT[name])
  if name in PROGRAMS:
    stack = list(allowed)
    while stack:
      for r in LAYOUT.get(stack.pop(), ()):
        if r not in allowed:
          allowed.add(r)
          stack.append(r)
  return allowed


def library_closure(name):
  seen = set()
  stack = [name]
  while stack:
    for r in LAYOUT.get(stack.pop(), ()):
      if r not in seen:
        seen.add(r)
        stack.append(r)
  return seen


# ---------------------------------------------------------------------------------------------------- the rules

def rule_1(tree):
  if len(tree.solutions) != 1:
    yield '.', f'the root holds {len(tree.solutions)} solutions ({", ".join(tree.solutions) or "none"}); exactly one'
    return
  for p in tree.projects:
    configurations = sorted(p.configurations)
    expected = sorted(f'{c}|{pl}' for c, pl in SolutionModel.BUILDS)
    if configurations != expected:
      yield p.path, f'configurations {configurations}; exactly {", ".join(expected)} (ADR-094)'
    for c, pl in SolutionModel.BUILDS:
      toolset = p.setting(c, 'Property.PlatformToolset', pl)
      if toolset != 'v145':
        yield p.path, f'{c}|{pl} uses the toolset {toolset!r}; v145 (bld.pin.msvc_toolset)'


def rule_2(tree):
  for p in tree.projects:
    for condition in p.unknown_conditions:
      yield p.path, f'the condition {condition!r} is not one the checkers evaluate'
    for pl in SolutionModel.PLATFORMS:
      debug, release = p.settings[('Debug', pl)], p.settings[('Release', pl)]
      for key in sorted(set(debug) | set(release)):
        if key in ALLOWED_DIFFERENCES or key.startswith('Property.Label'):
          continue
        if debug.get(key) != release.get(key):
          yield p.path, f'{key} is {debug.get(key)!r} in Debug|{pl} and {release.get(key)!r} in Release|{pl}'
    for c in SolutionModel.CONFIGURATIONS:
      x64, arm64 = p.settings[(c, 'x64')], p.settings[(c, 'ARM64')]
      for key in sorted(set(x64) | set(arm64)):
        if key in PLATFORM_DIFFERENCES or key.startswith('Property.Label'):
          continue
        if x64.get(key) != arm64.get(key):
          yield p.path, f'{key} is {x64.get(key)!r} in {c}|x64 and {arm64.get(key)!r} in {c}|ARM64 (ADR-094)'
    for c, pl in SolutionModel.BUILDS:
      values = p.settings[(c, pl)]
      build = f'{c}|{pl}'
      defines = p.preprocessor_definitions(c, pl)
      expected = '_DEBUG' if c == 'Debug' else 'NDEBUG'
      if expected not in defines or ({'_DEBUG', 'NDEBUG'} - {expected}) & set(defines):
        yield p.path, f'{build} must define {expected} and not the other configuration\'s macro'
      for name, value in {**REQUIRED_COMPILE, **REQUIRED_COMPILE_BY_PLATFORM[pl]}.items():
        if values.get('ClCompile.' + name) != value:
          yield p.path, f'{build} lacks ClCompile {name} = {value} (BLD-030, BLD-040)'
      options = values.get('ClCompile.AdditionalOptions', '')
      for option in REQUIRED_COMPILE_OPTIONS:
        if option not in options.split():
          yield p.path, f'{build} lacks the compiler option {option} (BLD-030)'
      for key, value in REQUIRED_BY_CONFIGURATION[c].items():
        if values.get(key) != value:
          yield p.path, f'{build} lacks {key} = {value} (BLD-030)'
      if p.is_library():
        expected_lib = 'true' if c == 'Release' else 'false'
        if values.get('Lib.LinkTimeCodeGeneration') != expected_lib:
          yield p.path, f'{build} lacks Lib LinkTimeCodeGeneration = {expected_lib} (BLD-030)'
        continue
      for name, value in {**REQUIRED_LINK, **REQUIRED_LINK_BY_PLATFORM[pl]}.items():
        if values.get('Link.' + name) != value:
          yield p.path, f'{build} lacks Link {name} = {value} (BLD-030)'
      link_options = values.get('Link.AdditionalOptions', '')
      for option in REQUIRED_LINK_OPTIONS:
        if option not in link_options.split():
          yield p.path, f'{build} lacks the linker option {option} (BLD-030)'
      for key, value in REQUIRED_LINK_BY_CONFIGURATION[c].items():
        if values.get(key) != value:
          yield p.path, f'{build} lacks {key} = {value} (BLD-030)'


def rule_3(tree):
  listed = {path for _, path in tree.entries}
  for path in listed:
    if not os.path.exists(os.path.join(tree.root, path)):
      yield path, 'the solution names a project that does not exist'
  for path, error in tree.parse_errors:
    yield path, f'the project file does not parse: {error}'
  for f in tree.files:
    if f.endswith('.vcxproj') and f not in listed:
      yield f, 'a project that the solution does not name'
  owned = set()
  for p in tree.projects:
    owned.add(p.directory + '/')
    on_disk = [f for f in tree.project_files(p) if f.endswith(SOURCE_EXTENSIONS + SHADER_EXTENSIONS + tuple(PROGRAM_FILE_KINDS))
               and not f.startswith('CompiledShader/')]
    registered = {include for _, include, _ in p.items}
    for f in on_disk:
      if f not in registered:
        yield f'{p.directory}/{f}', f'not an item of {p.name}.vcxproj'
    for kind, include, _ in p.items:
      if not os.path.exists(os.path.join(tree.root, p.directory, include)):
        yield f'{p.directory}/{include}', f'an item of {p.name}.vcxproj that does not exist'
    if p.filters is None:
      yield p.path + '.filters', 'missing'
      continue
    for name in p.filter_names:
      if name in FILE_KIND_DEFAULTS:
        yield p.path + '.filters', f'the file-kind filter "{name}"; filters are functional (AGENTS.md §2)'
    for include in sorted(registered):
      flt = p.filters.get(include)
      if not flt:
        yield f'{p.directory}/{include}', f'has no filter in {p.name}.vcxproj.filters'
      elif flt not in p.filter_names:
        yield f'{p.directory}/{include}', f'is in the undeclared filter "{flt}"'
    for include in sorted(set(p.filters) - registered):
      yield f'{p.directory}/{include}', f'in {p.name}.vcxproj.filters but not in the .vcxproj'
    for include in sorted(registered):
      stem, extension = os.path.splitext(include)
      if extension == '.h' and stem + '.cpp' in registered and p.filters.get(include) != p.filters.get(stem + '.cpp'):
        yield f'{p.directory}/{include}', f'is in another filter than {os.path.basename(stem)}.cpp'
  for f in tree.files:
    if f.endswith(SOURCE_EXTENSIONS) and not any(f.startswith(d) for d in owned) and not f.startswith('Build/'):
      yield f, 'a source file of no project'
  yield from clang_tidy_registry(tree)


def clang_tidy_registry(tree):
  path = os.path.join(tree.root, '.clang-tidy')
  if not os.path.exists(path):
    yield '.clang-tidy', 'missing'
    return
  m = re.search(r"^HeaderFilterRegex:\s*'\((.*)\)\[", tree.read('.clang-tidy'), re.MULTILINE)
  if not m:
    yield '.clang-tidy', 'HeaderFilterRegex is not of the form (<Project>|...)[/\\\\]...'
    return
  names = set(m.group(1).split('|'))
  for p in tree.projects:
    if p.is_test():
      if '[A-Za-z0-9]+Tests' not in names and p.name not in names:
        yield '.clang-tidy', f'HeaderFilterRegex does not match the test project {p.name}'
    elif p.name not in names:
      yield '.clang-tidy', f'HeaderFilterRegex does not name {p.name}, so its headers would go unlinted'


def rule_4(tree):
  for p in tree.projects:
    for f in tree.project_files(p):
      parts = f.split('/')
      if len(parts) == 1 or not f.endswith(SOURCE_EXTENSIONS + SHADER_EXTENSIONS + BANNED_EXTENSIONS):
        continue
      if parts[0] == 'Shader' and len(parts) == 2 and f.endswith(SHADER_EXTENSIONS):
        continue
      if parts[0] == 'CompiledShader' and len(parts) == 2:
        continue
      yield f'{p.directory}/{f}', 'source in a subfolder; only Shader/ and CompiledShader/ are sanctioned (AGENTS.md §2)'
  if tree.git:
    for f in tree.files:
      if '/CompiledShader/' in '/' + f and f in git_tracked(tree.root):
        yield f, 'CompiledShader/ is build output and is never committed'


_TRACKED = {}


def git_tracked(root):
  if root not in _TRACKED:
    output = subprocess.run(['git', 'ls-files'], cwd=root, check=True, capture_output=True, text=True).stdout
    _TRACKED[root] = set(output.splitlines())
  return _TRACKED[root]


def rule_5(tree):
  for p in tree.projects:
    kinds = {include: kind for kind, include, _ in p.items}
    for f in tree.project_files(p):
      name = os.path.basename(f)
      stem, extension = os.path.splitext(name)
      if extension.lower() in BANNED_EXTENSIONS:
        yield f'{p.directory}/{f}', f'the file kind {extension} is not used; sources are .h and .cpp (AGENTS.md R7)'
      if extension.lower() in PROGRAM_FILE_KINDS:
        if p.name not in PROGRAMS:
          yield f'{p.directory}/{f}', f'a {extension} file belongs to a program project only (AGENTS.md R7)'
        elif stem != p.name:
          yield f'{p.directory}/{f}', f'a program\'s {extension} file is {p.name}{extension} (AGENTS.md R7)'
        elif f in kinds and kinds[f] != PROGRAM_FILE_KINDS[extension.lower()]:
          yield f'{p.directory}/{f}', f'is a {kinds[f]} item, not a {PROGRAM_FILE_KINDS[extension.lower()]} item (AGENTS.md R7)'
      if extension in SOURCE_EXTENSIONS and not f.startswith('CompiledShader/'):
        if stem not in R7_EXCEPTIONS and not PASCAL_CASE.match(stem):
          yield f'{p.directory}/{f}', 'not named in PascalCase for its primary type (AGENTS.md R7)'
        text = SolutionModel.strip_comments_and_strings(tree.read(f'{p.directory}/{f}'))
        if MODULE_LINE.search(text):
          yield f'{p.directory}/{f}', 'uses C++ modules or import std (BLD-034)'


def rule_6(tree):
  for path, text in hand_written_sources(tree):
    stripped = SolutionModel.strip_comments_and_strings(text)
    for m in TYPE_DEFINITION.finditer(stripped):
      name = next(g for g in m.groups() if g)
      if R2_AFFIX.search(name):
        line = stripped.count('\n', 0, m.start()) + 1
        yield f'{path}:{line}', f'the type name {name} carries an R2 prefix or affix'
    for m in IDENTIFIER.finditer(stripped):
      lowered = m.group(0).lower()
      for british in R11_BRITISH:
        if british in lowered:
          line = stripped.count('\n', 0, m.start()) + 1
          yield f'{path}:{line}', f'the identifier {m.group(0)} uses a spelling other than the SDK\'s (AGENTS.md R11)'
          break


def hand_written_sources(tree):
  for p in tree.projects:
    for kind, include, _ in p.items:
      if kind in ('ClCompile', 'ClInclude') and include.endswith(SOURCE_EXTENSIONS):
        path = f'{p.directory}/{include}'
        if os.path.exists(os.path.join(tree.root, path)):
          yield path, tree.read(path)


def rule_7(tree):
  for p in tree.projects:
    kinds = {include: kind for kind, include, _ in p.items}
    shader_files = [f for f in tree.project_files(p) if f.startswith('Shader/')]
    if shader_files and p.name != SHADER_PROJECT:
      yield f'{p.directory}/Shader', f'only {SHADER_PROJECT} holds shaders (ADR-090 item 1)'
    for f in shader_files:
      name = os.path.basename(f)
      if name.endswith('.hlsl'):
        if not SHADER_NAME.match(name):
          yield f'{p.directory}/{f}', 'a shader is named <Shader>[<Permutation>]<VS|PS|CS>.hlsl (BLD-121)'
        if kinds.get(f) != 'FXCompile':
          yield f'{p.directory}/{f}', 'a .hlsl is an FXCompile item (BLD-121)'
      elif name.endswith('.hlsli'):
        if not SHADER_INCLUDE_NAME.match(name):
          yield f'{p.directory}/{f}', 'a shader include is named <Shader>.hlsli (BLD-121)'
        if kinds.get(f) == 'FXCompile':
          yield f'{p.directory}/{f}', 'an .hlsli is never compiled on its own (BLD-121)'
      else:
        yield f'{p.directory}/{f}', 'Shader/ holds only .hlsl and .hlsli files (AGENTS.md §2)'


def rule_8(tree):
  for p in tree.projects:
    for c, pl in SolutionModel.BUILDS:
      names = [d.split('=')[0].strip() for d in p.preprocessor_definitions(c, pl)]
      options = (p.setting(c, 'ClCompile.AdditionalOptions', pl) or '').split()
      names += [o[2:].split('=')[0] for o in options if o.startswith(('/D', '-D'))]
      for name in names:
        if name == 'ENGINE_DEVELOPMENT' or name.startswith('ENGINE_'):
          yield p.path, f'{c}|{pl} defines the switch {name}; switches follow from NDEBUG in one engine header (BLD-014)'
        if name in WINDOWS_MACROS:
          yield p.path, f'{c}|{pl} defines {name}; only {WINDOWS_HEADERS} defines the Windows macro family (AGENTS.md §4)'


def rule_9(tree):
  pins = read_pins(tree)
  for p in tree.projects:
    for package, version in p.packages:
      if p.is_tool():
        if not adr_records(tree, package, version):
          yield p.directory + '/packages.config', f'{package} {version} is recorded in no ADR with its licence (BLD-043)'
        continue
      key = ALLOWED_PACKAGES.get(package)
      if key is None:
        yield p.directory + '/packages.config', f'{package} is not one of the packages R14 allows (BLD-037)'
      elif pins.get(key) != version:
        yield p.directory + '/packages.config', f'{package} {version}; the pin {key} is {pins.get(key)} (BLD-038)'
  committed = [f for f in (git_tracked(tree.root) if tree.git else tree.files) if f.startswith('packages/')]
  if committed:
    yield 'packages/', 'restored packages are build output and are never committed (BLD-036)'
  yield from agility_exports(tree)


def agility_exports(tree):
  """BLD-042 and BLD T7: the exported D3D12SDKVersion is the minor version of the restored Agility SDK package, and
  the path is bld.pin.d3d12_sdk_path; the client exports both, and the server neither."""
  client = tree.by_name.get('Species')
  if client is None or not any(package == AGILITY_PACKAGE for package, _ in client.packages):
    return
  if not os.path.exists(os.path.join(tree.root, AGILITY_EXPORTS)):
    yield AGILITY_EXPORTS, f'missing: Species restores {AGILITY_PACKAGE} and exports no D3D12SDKVersion (BLD-042)'
    return
  text = strip_comments_only(tree.read(AGILITY_EXPORTS))
  m = re.search(r'\bAGILITY_SDK_VERSION\s*=\s*(\d+)\s*;', text)
  exported = m.group(1) if m else None
  for p in tree.projects:
    for package, version in p.packages:
      parts = version.split('.')
      if package == AGILITY_PACKAGE and (len(parts) < 2 or parts[1] != exported):
        yield AGILITY_EXPORTS, f'exports D3D12SDKVersion {exported}, and {p.name} restores {package} {version} (BLD-042)'
  if not re.search(r'\bAGILITY_SDK_PATH\s*=\s*' + re.escape(AGILITY_SDK_PATH) + r'\s*;', text):
    yield AGILITY_EXPORTS, 'AGILITY_SDK_PATH is not ".\\\\D3D12\\\\" (bld.pin.d3d12_sdk_path, BLD-042)'
  for c, pl in SolutionModel.BUILDS:
    options = client.setting(c, 'Link.AdditionalOptions', pl) or ''
    for option in AGILITY_EXPORT_OPTIONS:
      if option not in options:
        yield client.path, f'{c}|{pl} lacks the linker option {option} (BLD-042)'
  server = tree.by_name.get(SERVER)
  for c, pl in SolutionModel.BUILDS:
    if server and 'D3D12SDK' in (server.setting(c, 'Link.AdditionalOptions', pl) or ''):
      yield server.path, f'{c}|{pl} exports an Agility SDK constant; the server exports neither (BLD-042)'


def read_pins(tree):
  path = os.path.join(tree.root, PINS_DOCUMENT)
  pins = {}
  if os.path.exists(path):
    for m in re.finditer(r'^\|\s*`(bld\.pin\.[a-z_0-9]+)`\s*\|\s*`?([^|`]+?)`?\s*\|', tree.read(PINS_DOCUMENT),
                         re.MULTILINE):
      pins[m.group(1)] = m.group(2).strip()
  return pins


def adr_records(tree, package, version):
  directory = os.path.join(tree.root, 'Design', 'ADR')
  if not os.path.isdir(directory):
    return False
  for name in os.listdir(directory):
    if name.endswith('.md'):
      text = tree.read(f'Design/ADR/{name}')
      if package in text and version in text and re.search('licen[cs]e', text, re.IGNORECASE):
        return True
  return False


def rule_10(tree):
  for p in tree.projects:
    for directory in p.include_directories('Debug'):
      normalized = directory.replace('/', '\\').rstrip('\\')
      if normalized.startswith(('$(VCInstallDir)', '$(WindowsSdkDir)', '$(WindowsSDK_IncludePath)',
                                '$(SolutionDir)packages\\')):
        continue
      m = re.match(r'^\$\(SolutionDir\)(Tools\\)?([A-Za-z0-9]+)$', normalized)
      if not m:
        yield p.path, f'the include directory {directory} is not $(SolutionDir)<Project>, the toolset, the SDK or ' \
                      'packages\\ (BLD-037, BLD-041)'
        continue
      name = m.group(2)
      if name == p.name:
        yield p.path, 'lists its own folder as an include directory (AGENTS.md §3)'
      elif name not in tree.by_name:
        yield p.path, f'the include directory {directory} is not a project of the solution'
      elif name not in p.references:
        yield p.path, f'includes {name} without referencing it (BLD-041)'


def rule_11(tree):
  names = set(tree.by_name)
  for p in tree.projects:
    for r in p.references:
      if r not in names:
        yield p.path, f'references {r}, which the solution does not hold'
      elif tree.by_name[r].is_tool() and not p.is_tool():
        yield p.path, f'references the tool project {r}; no game or engine project does (AGENTS.md §2)'
    if p.is_tool():
      for r in p.references:
        if r in PROGRAMS or (r in tree.by_name and tree.by_name[r].is_test()):
          yield p.path, f'a tool project references the program or test project {r}'
      continue
    if p.is_test():
      library = p.name[:-len('Tests')]
      allowed = ({library} | library_closure(library)) if library in LAYOUT else set()
    else:
      allowed = allowed_references(p.name)
      if allowed is None:
        yield p.path, f'{p.name} is not a project of ADR-090 table 1; a layout change amends ADR-090 first'
        continue
      expected_type = 'StaticLibrary' if p.name in LIBRARIES else 'Application'
      if p.configuration_type() != expected_type:
        yield p.path, f'is a {p.configuration_type()}; ADR-090 makes it a {expected_type}'
    for r in p.references:
      if r in names and r not in allowed:
        yield p.path, f'references {r}, an edge that ADR-090 table 1 does not allow (BLD-041)'
  server = tree.by_name.get(SERVER)
  if server:
    reached = tree.closure(SERVER) | {SERVER}
    for r in sorted(reached & PRESENTATION_PROJECTS):
      yield server.path, f'the server reaches the presentation library {r} (BLD-045)'
    for name in sorted(reached):
      project = tree.by_name.get(name)
      for package, _ in project.packages if project else []:
        if package in PRESENTATION_PACKAGES:
          yield server.path, f'the server links {name}, which restores {package} (BLD-045)'
    for c, pl in SolutionModel.BUILDS:
      dependencies = (server.setting(c, 'Link.AdditionalDependencies', pl) or '').lower()
      for library in PRESENTATION_LIBRARIES:
        if re.search(r'\b' + library + r'\.lib\b', dependencies):
          yield server.path, f'{c}|{pl} links {library}.lib (BLD-045)'


def rule_12(tree):
  for p in tree.projects:
    if p.is_library() and not p.is_tool() and p.name + 'Tests' not in tree.by_name:
      yield p.path, f'the library {p.name} has no {p.name}Tests project (ADR-069)'
    if not p.is_test():
      continue
    library = tree.by_name.get(p.name[:-len('Tests')]) if p.name.endswith('Tests') else None
    if library is None or not library.is_library():
      yield p.path, 'a test project is named <Library>Tests for a library of the solution (ADR-069)'
    if p.configuration_type() != 'DynamicLibrary':
      yield p.path, f'a test project builds a DLL, not a {p.configuration_type()} (ADR-069)'
    for kind, include, _ in p.items:
      if kind == 'ClCompile' and include != 'pch.cpp':
        text = SolutionModel.strip_comments_and_strings(tree.read(f'{p.directory}/{include}'))
        if not re.search(r'\bnamespace\s+' + re.escape(p.name) + r'\b', text):
          yield f'{p.directory}/{include}', f'test code lives in namespace {p.name} (AGENTS.md R9, ADR-069)'


def rule_13(tree):
  owner = {}
  for p in tree.projects:
    for include in p.sources():
      owner.setdefault(os.path.basename(include), set()).add(p.name)
  integrated = []
  for p in tree.projects:
    local_headers = {os.path.basename(i) for i in p.sources()}
    for include in p.sources():
      path = f'{p.directory}/{include}'
      if not os.path.exists(os.path.join(tree.root, path)):
        continue
      for line, quoted, header in SolutionModel.includes(tree.read(path)):
        if '..' in header.replace('\\', '/').split('/'):
          yield f'{path}:{line}', f'includes {header} by a relative path; other projects are reached through ' \
                                  '$(SolutionDir)<Project> (BLD-041)'
          continue
        name = os.path.basename(header.replace('\\', '/'))
        if name in local_headers:
          continue
        for q in sorted(owner.get(name, ())):
          if q not in p.references:
            yield f'{path}:{line}', f'includes {name} of {q}, which {p.name} does not reference (BLD-041, DEC-222)'
          if p.name == 'Species' and q == 'GameLogic' and name != 'IntegratedServer.h':
            yield f'{path}:{line}', 'the client includes no GameLogic header but IntegratedServer.h (ADR-090 item 4)'
          if p.name == 'Species' and q == 'NeuronServer':
            yield f'{path}:{line}', 'the client includes no NeuronServer header (ADR-090 item 4)'
          if p.name == 'Species' and q == 'GameLogic' and name == 'IntegratedServer.h':
            integrated.append(path)
  if len(set(integrated)) > 1:
    yield 'Species', f'only the composition root includes IntegratedServer.h, not {len(set(integrated))} files ' \
                     '(ADR-090 item 4)'


def rule_14(tree):
  requirements = read_requirement_ids(tree)
  for path, text in hand_written_sources(tree):
    stripped = SolutionModel.strip_comments_and_strings(text)
    if path != WINDOWS_HEADERS:
      for line, _, header in SolutionModel.includes(text):
        if header.lower() == 'windows.h':
          yield f'{path}:{line}', f'includes <windows.h>; include {os.path.basename(WINDOWS_HEADERS)} (PLT-132)'
      for m in re.finditer(r'^\s*#\s*define\s+(' + '|'.join(WINDOWS_MACROS) + r')\b', stripped, re.MULTILINE):
        line = stripped.count('\n', 0, m.start()) + 1
        yield f'{path}:{line}', f'defines {m.group(1)}; only {os.path.basename(WINDOWS_HEADERS)} does (PLT-132)'
    project = path.split('/')[0]
    if project != PLATFORM_PROJECT:
      for pattern, what in PLATFORM_SERVICES:
        for m in re.finditer(pattern, stripped):
          line = stripped.count('\n', 0, m.start()) + 1
          yield f'{path}:{line}', f'{what} outside the platform layer (PLT-003)'
    if project not in WINDOW_PROJECTS:
      for m in re.finditer(WINDOW_CREATION, stripped):
        line = stripped.count('\n', 0, m.start()) + 1
        yield f'{path}:{line}', 'creates a window outside the platform layer and the client engine (PLT-003, ADR-090)'
    if project in SIMULATION_PROJECTS:
      for m in MUTABLE_STATIC_NAME.finditer(stripped):
        line = stripped.count('\n', 0, m.start()) + 1
        yield f'{path}:{line}', f'{m.group(0)} is mutable static state in the simulation (R-SIM-019, R-SIM-730)'
    if project in HEADLESS_PROJECTS:
      for line, _, header in SolutionModel.includes(text):
        if os.path.basename(header.replace('\\', '/')) == os.path.basename(WINDOWS_HEADERS):
          yield f'{path}:{line}', f'includes {header} in {project}; the server and simulation reach the system through ' \
                                  'the platform layer only (ADR-083 item 1)'
    if path != PIX_FILE:
      for line, _, header in SolutionModel.includes(text):
        if header.lower() in ('pix3.h', 'pix3_win.h', 'pixevents.h'):
          yield f'{path}:{line}', f'includes {header}; only {os.path.basename(PIX_FILE)} calls the PIX runtime (DBG-080)'
    for line, _, header in SolutionModel.includes(text):
      name = os.path.basename(header.replace('\\', '/')).lower()
      if name in SOCKET_HEADERS:
        yield f'{path}:{line}', f'includes {header}; MsQuic owns the socket, and no Species code opens one (ADR-089)'
      if name.startswith('d3dx12'):
        yield f'{path}:{line}', f'includes {header}; the d3dx12 helpers are not used, barriers are written by hand (R14)'
      elif project != RENDERER_PROJECT and DIRECT3D_HEADER.match(name) and name != CLIENT_FORMAT_HEADER:
        yield f'{path}:{line}', f'includes {header} outside {RENDERER_PROJECT}; the factory\'s clients see its interface ' \
                                f'and <{CLIENT_FORMAT_HEADER}> (ADR-078)'
    without_comments = strip_comments_only(text)
    if project not in PRESENTATION_PROJECTS:
      for m in PRAGMA_LIBRARY.finditer(without_comments):
        library = m.group(1).lower()
        if any(re.fullmatch(name + r'(?:\.lib)?', library) for name in PRESENTATION_LIBRARIES):
          line = without_comments.count('\n', 0, m.start()) + 1
          yield f'{path}:{line}', f'links {m.group(1)} outside the client libraries (ADR-001, BLD-045)'
    for m in REQUIREMENT_ATTRIBUTE.finditer(without_comments):
      if m.group(1) not in requirements:
        line = without_comments.count('\n', 0, m.start()) + 1
        yield f'{path}:{line}', f'the Requirement attribute names {m.group(1)!r}, no requirement id of the design ' \
                                '(DBG-140)'


def strip_comments_only(text):
  return re.sub(r'//[^\n]*|/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text, flags=re.DOTALL)


def read_requirement_ids(tree):
  directory = os.path.join(tree.root, REQUIREMENTS_INDEX)
  ids = set()
  if os.path.isdir(directory):
    for name in os.listdir(directory):
      if name.endswith('.md'):
        ids.update(re.findall(r'^\|\s*([A-Z][A-Z-]*-\d+)\s*\|', tree.read(f'{REQUIREMENTS_INDEX}/{name}'),
                              re.MULTILINE))
  return ids


CHECKS = {n: globals()[f'rule_{n}'] for n in RULES}


def check(root):
  tree = Tree(root)
  findings = []
  for n, rule in CHECKS.items():
    for path, message in rule(tree):
      findings.append((n, path, message))
  return findings


# ----------------------------------------------------------------------------------------------- the fixtures

def copy_tree(source, target):
  """The parts of the tree the rules read, copied into a directory with no git."""
  solutions = SolutionModel.find_solutions(source)
  for name in solutions + ['.clang-tidy']:
    shutil.copy2(os.path.join(source, name), os.path.join(target, name))
  for _, path in SolutionModel.read_solution(source, solutions[0]):
    directory = os.path.dirname(path)
    if not os.path.exists(os.path.join(target, directory)):
      shutil.copytree(os.path.join(source, directory), os.path.join(target, directory),
                      ignore=shutil.ignore_patterns('x64', 'ARM64', 'CompiledShader'))
  for path in (PINS_DOCUMENT,):
    os.makedirs(os.path.dirname(os.path.join(target, path)), exist_ok=True)
    shutil.copy2(os.path.join(source, path), os.path.join(target, path))
  for directory in (REQUIREMENTS_INDEX, 'Design/ADR'):
    shutil.copytree(os.path.join(source, directory), os.path.join(target, directory))


def write(root, path, text, append=False):
  full = os.path.join(root, path)
  os.makedirs(os.path.dirname(full), exist_ok=True)
  with open(full, 'a' if append else 'w', encoding='utf-8', newline='') as fh:
    fh.write(text)


def edit(root, path, old, new):
  full = os.path.join(root, path)
  with open(full, encoding='utf-8') as fh:
    text = fh.read()
  if old not in text:
    raise AssertionError(f'fixture anchor {old!r} not in {path}')
  with open(full, 'w', encoding='utf-8', newline='') as fh:
    fh.write(text.replace(old, new, 1))


def edit_project(root, path, change):
  """Edits a project file as XML; change(root element, ns) mutates it."""
  ElementTree.register_namespace('', SolutionModel.MSBUILD[1:-1])
  full = os.path.join(root, path)
  tree = ElementTree.parse(full)
  change(tree.getroot(), SolutionModel.MSBUILD)
  tree.write(full, encoding='utf-8', xml_declaration=True)


def group(element, ns, tag, configuration=None, platform='x64'):
  for child in element.findall(ns + tag):
    condition = child.get('Condition')
    if configuration is None and condition is None:
      return child
    if configuration and condition and f"'{configuration}|{platform}'" in condition:
      return child
  raise AssertionError(f'no {tag} for {configuration}')


def sub(parent, ns, tag, text=None):
  node = parent.find(ns + tag)
  if node is None:
    node = ElementTree.SubElement(parent, ns + tag)
  if text is not None:
    node.text = text
  return node


def fixture_release_differs(root):
  edit_project(root, 'NeuronCore/NeuronCore.vcxproj',
               lambda e, ns: sub(sub(group(e, ns, 'ItemDefinitionGroup', 'Release'), ns, 'ClCompile'), ns, 'SDLCheck',
                                 'true'))


def fixture_arm64_on_avx2(root):
  edit_project(root, 'NeuronCore/NeuronCore.vcxproj',
               lambda e, ns: sub(sub(group(e, ns, 'ItemDefinitionGroup', 'Debug', 'ARM64'), ns, 'ClCompile'), ns,
                                 'EnableEnhancedInstructionSet', 'AdvancedVectorExtensions2'))


def fixture_toolset(root):
  edit_project(root, 'NeuronServer/NeuronServer.vcxproj',
               lambda e, ns: [p.find(ns + 'PlatformToolset').__setattr__('text', 'v143')
                              for p in e.findall(ns + 'PropertyGroup') if p.find(ns + 'PlatformToolset') is not None])


def fixture_switch(root):
  edit_project(root, 'NeuronCore/NeuronCore.vcxproj',
               lambda e, ns: sub(sub(group(e, ns, 'ItemDefinitionGroup'), ns, 'ClCompile'), ns,
                                 'PreprocessorDefinitions', 'ENGINE_DEVELOPMENT=1;%(PreprocessorDefinitions)'))


def fixture_own_include(root):
  edit_project(root, 'NeuronClient/NeuronClient.vcxproj',
               lambda e, ns: sub(sub(group(e, ns, 'ItemDefinitionGroup'), ns, 'ClCompile'), ns,
                                 'AdditionalIncludeDirectories',
                                 '$(SolutionDir)NeuronClient;$(SolutionDir)NeuronCore;%(AdditionalIncludeDirectories)'))


def fixture_server_reaches_client(root):
  def change(e, ns):
    items = ElementTree.SubElement(e, ns + 'ItemGroup')
    reference = ElementTree.SubElement(items, ns + 'ProjectReference', Include='..\\NeuronClient\\NeuronClient.vcxproj')
    sub(reference, ns, 'Project', '{00000000-0000-0000-0000-000000000000}')
  edit_project(root, 'NeuronServer/NeuronServer.vcxproj', change)


FIXTURES = [
  (1, 'a second solution at the root', lambda r: write(r, 'Second.sln', '')),
  (1, 'a project on another toolset', fixture_toolset),
  (2, 'a setting that Release states and Debug does not', fixture_release_differs),
  (2, 'an ARM64 build on the x64 instruction set (ADR-094)', fixture_arm64_on_avx2),
  (3, 'a source file that its project does not list', lambda r: write(r, 'NeuronCore/Stray.cpp', '')),
  (3, 'a file-kind filter',
   lambda r: edit(r, 'NeuronCore/NeuronCore.vcxproj.filters', '<ItemGroup>',
                  '<ItemGroup>\n    <Filter Include="Source Files">\n      <UniqueIdentifier>{00000000-0000-0000-0000-'
                  '000000000001}</UniqueIdentifier>\n    </Filter>')),
  (4, 'a header in a subfolder', lambda r: write(r, 'NeuronCore/Platform/Deep.h', '#pragma once\n')),
  (5, 'a .hpp file', lambda r: write(r, 'NeuronCore/Legacy.hpp', '#pragma once\n')),
  (6, 'a type name with an I prefix', lambda r: write(r, 'NeuronCore/WindowsHeaders.h', '\nclass IThing\n{\n};\n',
                                                      append=True)),
  (6, 'an identifier with a British spelling',
   lambda r: write(r, 'GameShared/pch.h', '\ninline int colourIndex = 0;\n', append=True)),
  (7, 'a shader with a lower-case name that is not an FXCompile item',
   lambda r: write(r, 'NeuronClient/Shader/bad.hlsl', '')),
  (8, 'a project that defines ENGINE_DEVELOPMENT', fixture_switch),
  (9, 'a package version that is not its pin',
   lambda r: edit(r, 'NeuronCore/packages.config', 'version="1.0.240308001"', 'version="1.0.0"')),
  (10, 'a project that lists its own folder as an include directory', fixture_own_include),
  (11, 'NeuronServer referencing NeuronClient (ADR-090 confirmation)', fixture_server_reaches_client),
  (12, 'test code outside namespace <Project>Tests',
   lambda r: edit(r, 'NeuronCoreTests/TestSupport.cpp', 'namespace NeuronCoreTests\n', 'namespace CoreTests\n')),
  (13, 'GameClient including a GameLogic header (ADR-090 confirmation)',
   lambda r: (write(r, 'GameLogic/IntegratedServer.h', '#pragma once\n'),
              edit_project(r, 'GameLogic/GameLogic.vcxproj',
                           lambda e, ns: ElementTree.SubElement(ElementTree.SubElement(e, ns + 'ItemGroup'),
                                                                ns + 'ClInclude', Include='IntegratedServer.h')),
              write(r, 'GameClient/pch.h', '#include "IntegratedServer.h"\n', append=True))),
  (14, '<windows.h> included outside WindowsHeaders.h',
   lambda r: write(r, 'GameShared/pch.h', '\n#include <windows.h>\n', append=True)),
  (14, 'a Requirement attribute that names no requirement id',
   lambda r: write(r, 'NeuronCoreTests/TestSupport.cpp',
                   '\n// BEGIN_TEST_METHOD_ATTRIBUTE(SuiteRuns)\nTEST_METHOD_ATTRIBUTE(L"Requirement", L"XYZ-999")\n',
                   append=True)),
  (14, 'the performance counter read outside the platform layer (PLT-003)',
   lambda r: write(r, 'GameShared/pch.h', '\ninline long long Ticks() { LARGE_INTEGER t; QueryPerformanceCounter(&t); return 0; }\n', append=True)),
  (14, 'a tick count read outside the platform layer (PLT-003)',
   lambda r: write(r, 'GameLogic/pch.h', '\ninline auto g_start = GetTickCount64();\n', append=True)),
  (14, 'a standard-library clock read outside the platform layer (PLT-003)',
   lambda r: write(r, 'NeuronServer/pch.h', '\ninline auto g_start = std::chrono::steady_clock::now();\n', append=True)),
  (14, 'a known folder looked up outside the platform layer (PLT-003)',
   lambda r: write(r, 'GameClient/pch.h', '\ninline auto g_folder = SHGetKnownFolderPath;\n', append=True)),
  (14, 'a Windows Error Reporting registration outside the platform layer (PLT-003)',
   lambda r: write(r, 'Species/pch.h', '\ninline auto g_register = WerRegisterFile;\n', append=True)),
  (14, 'a thread created other than through PlatformThread (PLT-003)',
   lambda r: write(r, 'NeuronCoreTests/TestSupport.h', '\ninline std::jthread g_worker;\n', append=True)),
  (14, 'a window created outside the platform layer and the client engine (PLT-003)',
   lambda r: write(r, 'GameClient/pch.h', '\ninline auto g_create = CreateWindowExW;\n', append=True)),
  (14, 'pix3.h included outside Profile.cpp (DBG-080)',
   lambda r: write(r, 'NeuronCore/Log.cpp', '\n#include <pix3.h>\n', append=True)),
  (5, 'an application manifest in a library project',
   lambda r: write(r, 'NeuronCore/NeuronCore.manifest', '<assembly />\n')),
  (9, 'an exported D3D12SDKVersion that is not the package\'s minor version (BLD T7)',
   lambda r: edit(r, AGILITY_EXPORTS, 'AGILITY_SDK_VERSION = 619;', 'AGILITY_SDK_VERSION = 618;')),
  (14, 'd3d12.h included outside the renderer project (ADR-078)',
   lambda r: write(r, 'GameClient/pch.h', '\n#include <d3d12.h>\n', append=True)),
  (14, 'the d3dx12 helpers included (R14)',
   lambda r: write(r, 'NeuronClient/pch.h', '\n#include "d3dx12.h"\n', append=True)),
  (14, 'a socket header included (ADR-089)',
   lambda r: write(r, 'NeuronCore/InProcessTransport.cpp', '\n#include <winsock2.h>\n', append=True)),
  (14, 'a mutable namespace-scope variable in the simulation (R-SIM-019)',
   lambda r: write(r, 'GameLogic/pch.h', '\ninline int g_ticksSeen = 0;\n', append=True)),
  (14, 'a Windows header included by the server library (ADR-083 item 1)',
   lambda r: write(r, 'NeuronServer/pch.h', '\n#include "WindowsHeaders.h"\n', append=True)),
  (14, 'a Direct3D library linked by a pragma outside the client libraries (BLD-045)',
   lambda r: write(r, 'NeuronServer/pch.h', '\n#pragma comment(lib, "d3d12.lib")\n', append=True)),
]


def run_fixtures():
  """Runs every fixture on a fresh copy of the tree; returns the failures."""
  failures = []
  with tempfile.TemporaryDirectory(prefix='CheckProjectFiles.') as scratch:
    base = os.path.join(scratch, 'base')
    os.makedirs(base)
    copy_tree(ROOT, base)
    clean = check(base)
    if clean:
      failures.append(f'the fixture copy of the tree is not clean: {clean[0]}')
      return failures
    for index, (rule, description, mutate) in enumerate(FIXTURES):
      target = os.path.join(scratch, f'fixture{index}')
      shutil.copytree(base, target)
      mutate(target)
      fired = {n for n, _, _ in check(target)}
      if rule not in fired:
        failures.append(f'rule {rule} did not fire on its fixture: {description} (fired: {sorted(fired) or "none"})')
  covered = {rule for rule, _, _ in FIXTURES}
  for rule in RULES:
    if rule not in covered:
      failures.append(f'rule {rule} has no fixture (BLD-123)')
  return failures


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--skip-fixtures', action='store_true', help='check the tree only')
  args = parser.parse_args()
  status = 0
  if not args.skip_fixtures:
    failures = run_fixtures()
    for failure in failures:
      print(f'CheckProjectFiles: fixture: {failure}')
    if failures:
      status = 1
    else:
      print(f'CheckProjectFiles: {len(FIXTURES)} fixtures, each reported under its rule; all {len(RULES)} rules fire')
  findings = check(ROOT)
  for n, path, message in findings:
    print(f'rule {n} ({RULES[n]}): {path}: {message}')
  for item in NOT_YET_CHECKED:
    print(f'CheckProjectFiles: rule 14 does not check yet: {item}')
  tree = Tree(ROOT)
  print(f'CheckProjectFiles: {len(tree.projects)} projects of {tree.solution}, {len(findings)} findings')
  return 1 if findings or status else 0


if __name__ == '__main__':
  sys.exit(main())
