"""X-TOOLS: build Tools/DesignCheck/TechPatterns.tsv from the run's pattern files.

Usage: python3 -I X-TOOLS_build_patterns.py <forbidden_patterns.tsv> <soft_patterns.tsv> <ip_tokens.txt> <out.tsv>
       <ledger.tsv>

<ledger.tsv> (rewritten pattern -> run pattern) goes to a scratch folder: its second column can hold IP members.

Takes the hard rows of class provenance and legacy-tech and the soft rows of class history, scoped, unfinished
and renamed-term. Every other row (brand, identifier, map-name, string-key, orig-string) is IP and is left to the
hashed denylists. Alternation members of a renamed-term row that are IP tokens are dropped (they are hard-fail
through the IP denylist). Notes are rewritten so that they cite nothing that the cutover removes. Patterns that
would match their own text (so that a hard scan of Tools/ would flag the pattern file) are rewritten into an
equivalent regular expression from REWRITE; equivalence is tested by X-TOOLS_compare_patterns.py.
"""
import csv
import re
import sys
import unicodedata

HARD_CLASSES = ('provenance', 'legacy-tech')
SOFT_CLASSES = ('history', 'scoped', 'unfinished', 'renamed-term')

# Original pattern -> equivalent pattern that does not match its own text. Filled from the self-match probe.
REWRITE = {
  'OpenGL': 'Open(?:GL)',
  'Direct3D ?9': 'Direct3D(?: ?)9',
  'DirectX ?9': 'DirectX(?: ?)9',
  'DirectX SDK': 'DirectX(?: )SDK',
  'DirectSound': 'Direct(?:Sound)',
  'waveOut': 'wave(?:Out)',
  'XInput ?1\\.3': 'XInput(?: ?)1\\.3',
  'xinput1_3': 'xinput1(?:_)3',
  '_control87': '_control(?:87)',
  'Visual Studio 200[58]': 'Visual Studio(?: )200[58]',
  'MSVC ?200[58]': 'MSVC(?: ?)200[58]',
  'unrar': 'un(?:rar)',
  'Inno Setup': 'Inno(?: )Setup',
  'GnuWin32': 'Gnu(?:Win32)',
  'OpenMP': 'Open(?:MP)',
  'display lists?': 'display(?: )lists?',
  'lost device': 'lost(?: )device',
  'metaserver': 'meta(?:server)',
  'authkey': 'auth(?:key)',
  'Verbatim excerpt': 'Verbatim(?: )excerpt',
  'Source map': 'Source(?: )map',
  'Docs-vs-code': 'Docs(?:-)vs-code',
  '[Cc]overage matrix': '[Cc]overage(?: )matrix',
  '\\bf909630\\b': '\\bf9096(?:30)\\b',
  '\\bINTERNAL_STUDY\\b': '\\bINTERNAL(?:_)STUDY\\b',
  '\\.(bmp|shp|mwm|dat|vcproj)\\b': '(?:\\.)(bmp|shp|mwm|dat|vcproj)\\b',
  '(?<!\\w)_work/': '(?<!\\w)_work(?:/)',
  '\\bFixedFunc\\d\\w*|\\bFixedPipeline\\b|fixedpipeline': '\\bFixedFunc\\d\\w*|\\bFixedPipeline\\b|fixed(?:pipeline)',
  '\\bB:(?:FULL|LEGACY|DEAD|D3D|DEMO|DEBUG|EDITOR)\\b': '\\bB:(?:FULL|LEG(?:ACY)|DEAD|D3D|DEM(?:O)|DEBUG|EDITOR)\\b',
}

# The class names printed with each hit; the run's "legacy-tech" would itself be a soft-flag word.
CLASS_NAME = {'legacy-tech': 'superseded-tech'}

NOTE_HARD = {
  'provenance': {
    0: 'source path; the look-behind skips URLs and longer paths',
    1: 'source path outside the retained design tooling',
    2: 'line reference',
    3: 'evidence tag',
    4: 'build tag',
    5: 'build tag without brackets',
    6: 'closing-section and excerpt names',
    7: 'closing-section name',
    8: 'closing-section name',
    9: 'work folder of the source corpus; not inside longer folder names',
    10: 'coverage index of the source corpus',
    11: 'file extension of the source corpus used as a file reference',
    12: 'revision id of the source corpus',
    13: 'unknown-item ids of the source corpus (NN-U<n>)',
    14: 'draft and script files of the source corpus',
    15: 'licence marker of the source corpus',
    16: 'link to a document of the source corpus',
  },
}


def fold(s):
  return unicodedata.normalize('NFKD', s).encode('ascii', 'ignore').decode('ascii').lower()


def rows_of(path):
  with open(path, encoding='utf-8', newline='') as fh:
    return list(csv.DictReader(fh, delimiter='\t'))


def legacy_note(note):
  if 'fixed-function' in note:
    return 'superseded emulation shaders of the fixed pipeline'
  n = note.replace('legacy ', 'superseded ')
  n = n.replace(' (opengl_directx)', '')
  return n


def soft_note(cls, note):
  n = re.sub(r'^prompt §7\.2[:;]?\s*', '', note)
  if cls == 'history' and not n:
    return 'history narrative'
  if cls == 'unfinished' and not n:
    return 'unfinished text'
  return n


def main():
  hard_src, soft_src, tokens_src, out, ledger = sys.argv[1:6]
  ip = {' '.join(fold(l).split()) for l in open(tokens_src, encoding='utf-8') if l.strip()}
  rows = []
  prov = 0
  for r in rows_of(hard_src):
    if r['class'] not in HARD_CLASSES:
      continue
    pat = REWRITE.get(r['pattern'], r['pattern'])
    if r['class'] == 'provenance':
      note = NOTE_HARD['provenance'][prov]
      prov += 1
    else:
      note = legacy_note(r['note'])
    rows.append((pat, r['flags'], 'hard', CLASS_NAME.get(r['class'], r['class']), note, r['pattern']))
  dropped = []
  for r in rows_of(soft_src):
    if r['class'] not in SOFT_CLASSES:
      continue
    pat = r['pattern']
    note = soft_note(r['class'], r['note'])
    if r['class'] == 'renamed-term':
      keep = []
      for member in pat.split('|'):
        literal = member.replace('\\b', '')
        if fold(literal) in ip:
          dropped.append(member)
        else:
          keep.append(member)
      if not keep:
        continue
      if len(keep) != len(pat.split('|')):
        note = 'placeholder names replaced by the collision screen'
      pat = '|'.join(keep)
    if pat == '\\bExplorer\\b':
      note = 'achievement name; use Tourist (allowed for the Windows file manager)'
    rows.append((REWRITE.get(pat, pat), r['flags'], 'soft', r['class'], note, r['pattern']))
  with open(out, 'w', encoding='utf-8', newline='') as fh:
    fh.write('pattern\tflags\tgate\tclass\tnote\n')
    for pat, flags, gate, cls, note, _ in rows:
      fh.write(f'{pat}\t{flags}\t{gate}\t{cls}\t{note}\n')
  # equivalence ledger for the compare script: rewritten pattern -> run pattern
  with open(ledger, 'w', encoding='utf-8', newline='') as fh:
    fh.write('pattern\tsource_pattern\n')
    for pat, _, _, _, _, src in rows:
      fh.write(f'{pat}\t{src}\n')
  print(f'{len(rows)} rows ({sum(1 for r in rows if r[2] == "hard")} hard, {sum(1 for r in rows if r[2] == "soft")} soft);'
        f' {len(dropped)} IP alternation members dropped; {sum(1 for r in rows if r[0] != r[5])} patterns rewritten')


if __name__ == '__main__':
  main()
