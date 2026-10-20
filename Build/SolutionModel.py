"""SolutionModel.py: the solution and its projects as data, for the checkers of AGENTS.md §6.

CheckProjectFiles.py and RunClangTidy.py read the build definition through this module rather than each parsing the
.slnx and the .vcxproj files on its own, so the two cannot disagree about what a project holds.

The evaluation of a project file is deliberately narrow. It understands what the projects of this repository write:
unconditioned groups, and groups conditioned on exactly `'$(Configuration)|$(Platform)'=='<Configuration>|<Platform>'`
for the configurations and platforms below. Any other condition is recorded as unknown, and CheckProjectFiles.py fails
on it, so a project cannot hide a setting behind a condition that the checkers do not evaluate. Settings are kept per
configuration and platform; a query names the configuration and reads x64 unless it names the platform too.
"""
import os
import re
import xml.etree.ElementTree as ElementTree

MSBUILD = '{http://schemas.microsoft.com/developer/msbuild/2003}'
CONFIGURATIONS = ('Debug', 'Release')
PLATFORMS = ('x64', 'ARM64')  # x64 ships; ARM64 is the development build of owner decision O66 (ADR-094)
BUILDS = tuple((c, p) for p in PLATFORMS for c in CONFIGURATIONS)
CONDITION = re.compile(r"^\s*'\$\(Configuration\)\|\$\(Platform\)'\s*==\s*'(\w+)\|(\w+)'\s*$")
ITEM_KINDS = ('ClCompile', 'ClInclude', 'FXCompile', 'None', 'Text', 'ResourceCompile', 'Image', 'Manifest')
TOOLS = ('ClCompile', 'Link', 'Lib', 'FXCompile')


def local(tag):
  return tag[len(MSBUILD):] if tag.startswith(MSBUILD) else tag


def find_solutions(root):
  return sorted(n for n in os.listdir(root) if os.path.splitext(n)[1].lower() in ('.slnx', '.sln'))


def read_solution(root, name):
  """Returns [(solution folder, project path relative to the root, with forward slashes)]."""
  tree = ElementTree.parse(os.path.join(root, name))
  out = []

  def walk(element, folder):
    for child in element:
      tag = local(child.tag)
      if tag == 'Folder':
        walk(child, child.get('Name', ''))
      elif tag == 'Project':
        out.append((folder, child.get('Path', '').replace('\\', '/')))
  walk(tree.getroot(), '/')
  return out


class Project:
  """One .vcxproj, its .filters and its packages.config."""

  def __init__(self, root, relative_path, folder):
    self.root = root
    self.path = relative_path
    self.folder = folder
    self.directory = os.path.dirname(relative_path)
    self.name = os.path.splitext(os.path.basename(relative_path))[0]
    self.absolute = os.path.join(root, relative_path)
    self.unknown_conditions = []
    self.configurations = []
    self.items = []          # (kind, include, {metadata})
    self.references = []     # project names
    self.imports = []
    self.settings = {b: {} for b in BUILDS}  # (configuration, platform) -> {key: value}
    self.globals = {}
    self.filters = None      # include -> filter name, or None when there is no .filters file
    self.filter_names = []
    self.packages = []       # (id, version)
    self._read_project()
    self._read_filters()
    self._read_packages()

  # -- reading ----------------------------------------------------------------------------------------------------

  def _applies(self, element):
    """The (configuration, platform) pairs an element applies to, from its Condition."""
    condition = element.get('Condition')
    if condition is None:
      return list(BUILDS)
    m = CONDITION.match(condition)
    if not m:
      if not condition.strip().lower().startswith(('exists(', '!exists(')):
        self.unknown_conditions.append(condition)
      return []
    configuration, platform = m.groups()
    if platform not in PLATFORMS or configuration not in CONFIGURATIONS:
      self.unknown_conditions.append(condition)
      return []
    return [(configuration, platform)]

  def _read_project(self):
    tree = ElementTree.parse(self.absolute)
    for element in tree.getroot():
      tag = local(element.tag)
      if tag == 'ItemGroup':
        for item in element:
          kind = local(item.tag)
          include = item.get('Include', '')
          metadata = {local(m.tag): (m.text or '').strip() for m in item}
          if kind == 'ProjectConfiguration':
            self.configurations.append(include)
          elif kind == 'ProjectReference':
            self.references.append(os.path.splitext(os.path.basename(include.replace('\\', '/')))[0])
          elif kind in ITEM_KINDS:
            self.items.append((kind, include.replace('\\', '/'), metadata))
      elif tag == 'PropertyGroup':
        applies = self._applies(element)
        for prop in element:
          name = local(prop.tag)
          value = (prop.text or '').strip()
          if element.get('Label') == 'Globals':
            self.globals[name] = value
          for c in applies:
            self.settings[c]['Property.' + name] = value
      elif tag == 'ItemDefinitionGroup':
        applies = self._applies(element)
        for tool in element:
          for setting in tool:
            name = local(setting.tag)
            key = local(tool.tag) + '.' + name
            for c in applies:
              # %(Name) in a value is the value defined before it, as MSBuild evaluates item definitions.
              value = (setting.text or '').strip().replace(f'%({name})', self.settings[c].get(key, ''))
              self.settings[c][key] = value
      elif tag in ('Import', 'ImportGroup'):
        nodes = [element] if tag == 'Import' else list(element)
        for node in nodes:
          if local(node.tag) == 'Import':
            self.imports.append(node.get('Project', ''))

  def _read_filters(self):
    path = self.absolute + '.filters'
    if not os.path.exists(path):
      return
    self.filters = {}
    tree = ElementTree.parse(path)
    for group in tree.getroot():
      if local(group.tag) != 'ItemGroup':
        continue
      for item in group:
        kind = local(item.tag)
        include = item.get('Include', '').replace('\\', '/')
        if kind == 'Filter':
          self.filter_names.append(include)
          continue
        node = item.find(MSBUILD + 'Filter')
        self.filters[include] = (node.text or '').strip() if node is not None else ''

  def _read_packages(self):
    path = os.path.join(self.root, self.directory, 'packages.config')
    if not os.path.exists(path):
      return
    tree = ElementTree.parse(path)
    for package in tree.getroot():
      if local(package.tag) == 'package':
        self.packages.append((package.get('id', ''), package.get('version', '')))

  # -- queries ------------------------------------------------------------------------------------------------------

  def setting(self, configuration, key, platform='x64'):
    return self.settings[(configuration, platform)].get(key)

  def configuration_type(self):
    return self.setting('Debug', 'Property.ConfigurationType') or self.setting('Release', 'Property.ConfigurationType')

  def is_library(self):
    return self.configuration_type() == 'StaticLibrary'

  def is_test(self):
    return self.globals.get('ProjectSubType') == 'NativeUnitTestProject' or self.name.endswith('Tests')

  def is_tool(self):
    return self.directory.replace('\\', '/').startswith('Tools/')

  def sources(self, kinds=('ClCompile', 'ClInclude')):
    return [include for kind, include, _ in self.items if kind in kinds]

  def include_directories(self, configuration='Debug', platform='x64'):
    value = self.setting(configuration, 'ClCompile.AdditionalIncludeDirectories', platform) or ''
    return [d.strip() for d in value.split(';') if d.strip()]

  def preprocessor_definitions(self, configuration='Debug', platform='x64'):
    value = self.setting(configuration, 'ClCompile.PreprocessorDefinitions', platform) or ''
    return [d.strip() for d in value.split(';') if d.strip()]


def load(root):
  """Returns (solution file name or None, [Project]) for the single solution at the root."""
  solutions = find_solutions(root)
  if len(solutions) != 1:
    return None, []
  projects = []
  for folder, path in read_solution(root, solutions[0]):
    if os.path.exists(os.path.join(root, path)):
      projects.append(Project(root, path, folder))
  return solutions[0], projects


def strip_comments_and_strings(text):
  """C++ source with comments, string literals and character literals blanked, line structure kept."""
  out = []
  i = 0
  n = len(text)
  while i < n:
    c = text[i]
    if c == '/' and i + 1 < n and text[i + 1] == '/':
      j = text.find('\n', i)
      j = n if j < 0 else j
      out.append(' ' * (j - i))
      i = j
    elif c == '/' and i + 1 < n and text[i + 1] == '*':
      j = text.find('*/', i + 2)
      j = n if j < 0 else j + 2
      out.append(re.sub(r'[^\n]', ' ', text[i:j]))
      i = j
    elif c == 'R' and text.startswith('R"', i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == '_')):
      m = re.match(r'R"([^(\s]*)\(', text[i:])
      if m:
        end = text.find(')' + m.group(1) + '"', i)
        end = n if end < 0 else end + len(m.group(1)) + 2
        out.append(re.sub(r'[^\n]', ' ', text[i:end]))
        i = end
      else:
        out.append(c)
        i += 1
    elif c in '"\'':
      j = i + 1
      while j < n and text[j] != c and text[j] != '\n':
        j += 2 if text[j] == '\\' else 1
      j = min(j + 1, n)
      out.append(' ' * (j - i))
      i = j
    else:
      out.append(c)
      i += 1
  return ''.join(out)


INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.MULTILINE)


def includes(text):
  """[(line number, quoted, header)] of the #include lines of a source text."""
  out = []
  for m in INCLUDE.finditer(text):
    out.append((text.count('\n', 0, m.start()) + 1, m.group(1) == '"', m.group(2).strip()))
  return out
