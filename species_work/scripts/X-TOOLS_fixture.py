"""X-TOOLS: build a fixture design tree with one planted defect per gate, for the failure-path comparison.

Usage: python3 -I X-TOOLS_fixture.py <snapshot repo dir> <ip_tokens.txt> <original_strings.txt> <out repo dir>

<snapshot repo dir> holds design_next/, vision/ and prompts/. The fixture is a copy of it at <out repo dir> with
reference/zz_fixture.md (template headings plus planted defects), reference/zz_noreadme/ (a folder with no README),
and glossary/zz_big.md (over 120 KB). The planted name and string are taken from the plain-text lists at run time
and written only to the scratch copy.
"""
import os
import shutil
import sys

snap, tokens, strings, out = sys.argv[1:5]
if os.path.exists(out):
  shutil.rmtree(out)
shutil.copytree(snap, out)
root = os.path.join(out, 'design_next')
names = [l.strip() for l in open(tokens, encoding='utf-8') if l.strip()]
first_name = next(n for n in names if n.isalpha() and len(n) > 8)   # a plain single-word entry
one_string = next(l.strip() for l in open(strings, encoding='utf-8') if len(l.split()) >= 4)
lines = [
  '# Fixture',
  '',
  '> Status: Reference · Prefix: R-VIS',
  '',
  '## 1 Purpose and player experience',
  '',
  f'The fixture names {first_name.title()} once (gate 1, name).',
  '',
  'It draws with Open' + 'GL (gate 1, superseded technology) and cites code' + '/foo/bar.cpp (gate 1, provenance).',
  '',
  'It says the mode was original' + 'ly different (gate 2, soft word not allowed here).',
  '',
  f'The label reads "{one_string}" (gate 2, interface string).',
  '',
  '## 2 Rules',
  '',
  '**R-VIS-001** (MUST) A duplicated requirement id (gate 5).',
  '',
  '**R-ZZZ-001** (MUST) An unregistered prefix (gate 5), citing DEC-999 and ADR-999 (gate 6).',
  '',
  'See [missing](no_such_file.md), [anchor](README.md#no-such-anchor) and '
  '[the conversion prompt](../../prompts/species-design-conversion.md) (gate 3).',
  '',
  '## 3 States and flows',
  '',
  '```mermaid',
  'stateDiagram-v2',
  '  [*] --> Broken --> ((',
  '```',
  '',
  '## 4 Tuning table',
  '',
  '| key | value | unit | range or notes | requirement | DEC |',
  '|---|---|---|---|---|---|',
  '| `ref.zz.fixture.uncited` | 1 | tick | — | — | — |',
  '',
  'It cites the asset `sfx.zz.not_in_manifest` (gate 7).',
  '',
  '## 5 Edge cases and interactions',
  '',
  'Workers, Builders, etc. (gate 9).',
  '',
  '## 6 Acceptance criteria',
  '',
  '## 7 Dependencies',
  '',
  '## 8 Open decisions',
  '',
]
with open(os.path.join(root, 'reference', 'zz_fixture.md'), 'w', encoding='utf-8', newline='\n') as fh:
  fh.write('\n'.join(lines))
os.makedirs(os.path.join(root, 'reference', 'zz_noreadme'), exist_ok=True)
with open(os.path.join(root, 'reference', 'zz_noreadme', 'note.txt'), 'w', encoding='utf-8') as fh:
  fh.write('a folder without a README (gate 10)\n')
with open(os.path.join(root, 'glossary', 'zz_big.md'), 'w', encoding='utf-8') as fh:
  fh.write('# Big\n\n' + ('Plain filler text for the size gate.\n' * 3500))
print('fixture written to', out)
