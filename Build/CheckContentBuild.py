"""CheckContentBuild.py: the content build's reproducibility check and its build fixtures (ADR-108; M1 plan T11).

  python Build/CheckContentBuild.py [--tool <SpeciesContentBuild.exe>]

Runs the content build twice on the repository, each run into a temporary folder of its own, and fails unless the two
folders are identical byte for byte (AST-066). Then it runs the build on fixtures: each a copy of GameData/ and of the
asset manifest with one fault in it, and each must fail with a message that names the faulty file and the fault
(AST-093, ADR-019's confirmation): a texture off its row, a damaged mesh (one fixture per check of the CMO importer,
built here from the owner's meshes by a small CMO reader and writer, so no binary fixture is committed), a damaged
glyph table and glyph sheet, a stray file and a damaged record. The tool is the Debug|x64 build's unless --tool names
another; the check writes nothing into the checkout.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, 'x64', 'Debug', 'SpeciesContentBuild.exe')
MANIFEST = os.path.join('Design', 'assets', 'asset_manifest.csv')
LEADER = 'GameData/Meshes/LeaderFigure.cmo'
WEAPON = 'GameData/Meshes/ThrownWeapon.cmo'


# ── A CMO file of the Visual Studio mesh content pipeline, read and written whole ──────────────────────────────────

class Cursor:
  def __init__(self, data):
    self.data, self.at = data, 0

  def take(self, fmt):
    values = struct.unpack_from('<' + fmt, self.data, self.at)
    self.at += struct.calcsize('<' + fmt)
    return values

  def u32(self):
    return self.take('I')[0]

  def string(self):
    units = self.u32()
    raw = self.data[self.at:self.at + 2 * units]
    self.at += 2 * units
    return raw


def read_cmo(data):
  """The file as a list of meshes, each a dict of its parts, every string kept as its raw UTF-16 bytes."""
  c = Cursor(data)
  meshes = []
  for _ in range(c.u32()):
    mesh = {'name': c.string(), 'materials': []}
    for _ in range(c.u32()):
      name = c.string()
      fields = c.take('33f')
      shader = c.string()
      textures = [c.string() for _ in range(8)]
      mesh['materials'].append({'name': name, 'fields': list(fields), 'shader': shader, 'textures': textures})
    mesh['skeleton'] = c.take('B')[0]
    mesh['submeshes'] = [list(c.take('5I')) for _ in range(c.u32())]
    mesh['indices'] = []
    for _ in range(c.u32()):
      count = c.u32()
      mesh['indices'].append(list(c.take(f'{count}H')))
    mesh['vertices'] = []
    for _ in range(c.u32()):
      count = c.u32()
      mesh['vertices'].append([list(c.take('10fI2f')) for _ in range(count)])
    mesh['skinning'] = []
    for _ in range(c.u32()):
      count = c.u32()
      mesh['skinning'].append(c.data[c.at:c.at + 32 * count])
      c.at += 32 * count
    mesh['extents'] = list(c.take('10f'))
    mesh['bones'] = []
    mesh['clips'] = []
    if mesh['skeleton']:
      for _ in range(c.u32()):
        name = c.string()
        parent = c.take('i')[0]
        matrices = list(c.take('48f'))
        mesh['bones'].append({'name': name, 'parent': parent, 'matrices': matrices})
      for _ in range(c.u32()):
        mesh['clips'].append(c.string())
        raise ValueError('the fixtures read no clip')
    meshes.append(mesh)
  assert c.at == len(data), 'the reader read the whole file'
  return meshes


def text(value):
  return (value + '\0').encode('utf-16-le')


def write_cmo(meshes):
  out = bytearray()
  put = lambda fmt, *values: out.extend(struct.pack('<' + fmt, *values))

  def string(raw):
    put('I', len(raw) // 2)
    out.extend(raw)

  put('I', len(meshes))
  for mesh in meshes:
    string(mesh['name'])
    put('I', len(mesh['materials']))
    for material in mesh['materials']:
      string(material['name'])
      put('33f', *material['fields'])
      string(material['shader'])
      for texture in material['textures']:
        string(texture)
    put('B', mesh['skeleton'])
    put('I', len(mesh['submeshes']))
    for submesh in mesh['submeshes']:
      put('5I', *submesh)
    put('I', len(mesh['indices']))
    for buffer in mesh['indices']:
      put('I', len(buffer))
      put(f'{len(buffer)}H', *buffer)
    put('I', len(mesh['vertices']))
    for buffer in mesh['vertices']:
      put('I', len(buffer))
      for vertex in buffer:
        put('10fI2f', *vertex)
    put('I', len(mesh['skinning']))
    for buffer in mesh['skinning']:
      put('I', len(buffer) // 32)
      out.extend(buffer)
    put('10f', *mesh['extents'])
    if mesh['skeleton']:
      put('I', len(mesh['bones']))
      for bone in mesh['bones']:
        string(bone['name'])
        put('i', bone['parent'])
        put('48f', *bone['matrices'])
      put('I', len(mesh['clips']))
  return bytes(out)


# ── Fixtures: (name, the change to a copy of the tree, the file the message names, a part of the message) ─────────

def read(root, path):
  with open(os.path.join(root, path), 'rb') as fh:
    return fh.read()


def write(root, path, data):
  with open(os.path.join(root, path), 'wb') as fh:
    fh.write(data)


def patch_u32(root, path, offset, value):
  data = bytearray(read(root, path))
  struct.pack_into('<I', data, offset, value)
  write(root, path, bytes(data))


def mesh_change(path, change):
  def apply(root):
    meshes = read_cmo(read(root, path))
    assert write_cmo(meshes) == read(root, path), 'the CMO writer writes the owner\'s file back unchanged'
    change(meshes)
    write(root, path, write_cmo(meshes))
  return apply


def replace_text(path, old, new, line_with=None):
  """Replaces the first occurrence of old, or its occurrence on the first line that holds line_with."""
  def apply(root):
    lines = read(root, path).decode('utf-8').split('\n')
    at = next(i for i, line in enumerate(lines) if old in line and (line_with is None or line_with in line))
    lines[at] = lines[at].replace(old, new, 1)
    write(root, path, '\n'.join(lines).encode('utf-8'))
  return apply


def set_nan(meshes):
  meshes[0]['vertices'][0][0][0] = float('nan')


def unterminated(meshes):
  meshes[0]['name'] = meshes[0]['name'][:-2] + 'x\0'.encode('utf-16-le')[:2]


def different_corner(meshes):
  vertices = meshes[0]['vertices'][0]
  first = meshes[0]['indices'][0][0]
  vertices[first][10] ^= 0x00010101


def with_skinning(meshes):
  meshes[0]['skinning'].append(b'')


def coverage(root):
  data = bytearray(read(root, 'GameData/Fonts/GameFont.dds'))
  data[128 + 4 * 1000 + 3] = 128
  write(root, 'GameData/Fonts/GameFont.dds', bytes(data))


FIXTURES = [
  ('a texture in another format than its row', lambda r: patch_u32(r, 'GameData/Textures/Cursor/World.dds', 128, 99),
   'GameData/Textures/Cursor/World.dds', 'asks for BC4_UNORM'),
  ('a texture of another size than its row', lambda r: patch_u32(r, 'GameData/Textures/Sprite/WorkerSide.dds', 16, 64),
   'GameData/Textures/Sprite/WorkerSide.dds', 'the size is 64x64 px'),
  ('a texture with another level count than its row', lambda r: patch_u32(r, 'GameData/Textures/Fx/BeamLong.dds', 28, 7),
   'GameData/Textures/Fx/BeamLong.dds', 'holds 7 levels'),
  ('a texture without the DX10 header', lambda r: patch_u32(r, 'GameData/Textures/Terrain/SeaTile01.dds', 84, 0x31545844),
   'GameData/Textures/Terrain/SeaTile01.dds', 'no DX10 header'),
  ('a mesh that ends early (ADR-019 check 1)', lambda r: write(r, LEADER, read(r, LEADER)[:-3]), LEADER, 'check 1'),
  ('a mesh with bytes after its last mesh (check 1)', lambda r: write(r, WEAPON, read(r, WEAPON) + b'\0'), WEAPON, 'check 1'),
  ('a string without its NUL (check 2)', mesh_change(WEAPON, unterminated), WEAPON, 'check 2'),
  ('a submesh naming no material (check 3)', mesh_change(WEAPON, lambda m: m[0]['submeshes'][0].__setitem__(0, 5)), WEAPON,
   'check 3'),
  ('an index past its vertex buffer (check 4)', mesh_change(WEAPON, lambda m: m[0]['indices'][0].__setitem__(4, 9999)), WEAPON,
   'check 4'),
  ('a float that is not finite (check 5)', mesh_change(WEAPON, set_nan), WEAPON, 'check 5'),
  ('a bone whose parent comes after it (check 6)', mesh_change(LEADER, lambda m: m[0]['bones'][0].__setitem__('parent', 1)), LEADER,
   'check 6'),
  ('a mesh named after no bone (check 7)', mesh_change(LEADER, lambda m: m[0].__setitem__('name', text('figure'))), LEADER,
   'check 7'),
  ('a skinning buffer (check 8)', mesh_change(WEAPON, with_skinning), WEAPON, 'check 8'),
  ('a socket of no kind (AST-039)', mesh_change(LEADER, lambda m: m[0]['bones'][1].__setitem__('name', text('socket_flagpole'))),
   LEADER, 'AST-039'),
  ('a triangle of two colours (AST-076)', mesh_change(WEAPON, different_corner), WEAPON, 'different colours'),
  ('a mesh over its row\'s triangle budget (AST-076)',
   replace_text(MANIFEST, '<= 100 triangles budget', '<= 50 triangles budget', line_with='mesh.unit.leader_figure,'),
   LEADER, 'pass the budget of 50'),
  ('a glyph table listing a code point twice (DAT-219)', replace_text('GameData/Fonts/GameFontGlyphs.json', '{"cp": 34,', '{"cp": 33,'),
   'GameData/Fonts/GameFontGlyphs.json', 'listed twice'),
  ('a glyph table that is not JSON (DAT-021)', replace_text('GameData/Fonts/EditorFontGlyphs.json', '"missing": 63,', '"missing": 63,,'),
   'GameData/Fonts/EditorFontGlyphs.json', 'syntax'),
  ('a glyph cell outside its sheet (DAT-219)', replace_text('GameData/Fonts/GameFontGlyphs.json', '"row": 13,', '"row": 14,'),
   'GameData/Fonts/GameFontGlyphs.json', 'outside the 256x224 px sheet'),
  ('a glyph table with a field its schema lacks (DAT-026)',
   replace_text('GameData/Fonts/GameFontGlyphs.json', '"missing": 63,', '"missing": 63, "kerning": [],'),
   'GameData/Fonts/GameFontGlyphs.json', 'unknown_field'),
  ('a glyph sheet with partial coverage (O63)', coverage, 'GameData/Fonts/GameFont.dds', 'coverage of 0 or 255'),
  ('a stray file (AST-013)', lambda r: write(r, 'GameData/Textures/Fx/Notes.txt', b'x'), 'GameData/Textures/Fx/Notes.txt', 'stray'),
  ('a record value of the wrong type (DAT-024)', replace_text('GameData/Records/tuning/mvp.json', '"ai_count": 3', '"ai_count": "3"'),
   'GameData/Records/tuning/mvp.json', 'wrong_type'),
]


def run(tool, root, out):
  return subprocess.run([tool, '--root', root, '--out', out], capture_output=True, text=True, encoding='utf-8', errors='replace')


def copy_tree(work):
  root = os.path.join(work, 'root')
  shutil.copytree(os.path.join(ROOT, 'GameData'), os.path.join(root, 'GameData'))
  os.makedirs(os.path.join(root, os.path.dirname(MANIFEST)))
  shutil.copyfile(os.path.join(ROOT, MANIFEST), os.path.join(root, MANIFEST))
  return root


def folder_bytes(folder):
  files = {}
  for directory, _, names in os.walk(folder):
    for name in names:
      path = os.path.join(directory, name)
      with open(path, 'rb') as fh:
        files[os.path.relpath(path, folder).replace('\\', '/')] = fh.read()
  return files


def main():
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument('--tool', default=TOOL)
  tool = os.path.abspath(parser.parse_args().tool)
  if not os.path.exists(tool):
    print(f'CheckContentBuild: {tool} does not exist; build the solution first')
    return 1
  failures = []
  with tempfile.TemporaryDirectory() as work:
    outputs = []
    for n in (1, 2):
      out = os.path.join(work, f'run{n}')
      result = run(tool, ROOT, out)
      if result.returncode != 0:
        print(result.stdout + result.stderr)
        print(f'CheckContentBuild: the content build fails on the repository (exit {result.returncode})')
        return 1
      outputs.append(folder_bytes(out))
    if outputs[0] != outputs[1]:
      differing = sorted(k for k in set(outputs[0]) | set(outputs[1]) if outputs[0].get(k) != outputs[1].get(k))
      failures.append(f'two runs differ in {", ".join(differing)} (AST-066)')
    print(f'CheckContentBuild: two runs wrote the same {len(outputs[0])} files, byte for byte')
    for name, change, file, part in FIXTURES:
      case = os.path.join(work, 'case')
      root = copy_tree(case)
      change(root)
      result = run(tool, root, os.path.join(case, 'out'))
      lines = [line for line in result.stderr.splitlines() if f'{file}:' in line and part in line]
      if result.returncode != 1 or not lines or os.path.exists(os.path.join(case, 'out')):
        failures.append(f'{name}: expected exit 1, nothing written and a message naming {file} with "{part}"; got exit '
                        f'{result.returncode}:\n{result.stderr}')
      else:
        print(f'CheckContentBuild: refused: {name}: {lines[0]}')
      shutil.rmtree(case)
  for failure in failures:
    print(f'CheckContentBuild: {failure}')
  print(f'CheckContentBuild: {len(FIXTURES)} fixtures, {len(failures)} failures')
  return 1 if failures else 0


if __name__ == '__main__':
  sys.exit(main())
