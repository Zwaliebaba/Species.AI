#!/usr/bin/env python3
"""CheckDesign.py: the structural gates of the Species design set.

Usage:
  python3 Tools/DesignCheck/CheckDesign.py <design root> [--write-index] [--report <file>] [--skip-mermaid]

Everything the gates need beyond the design root sits beside this file: TechPatterns.tsv (technology, provenance and
soft-flag patterns in plain text), IpDenylist.sha256 and OriginalStrings.sha256 (hashed denylists of the names and
text of the reference product and its IP), MermaidCheck.js with the pinned packages of package.json, installed into
node_modules/ by "npm ci --prefix Tools/DesignCheck". The prefix registry is read from <root>/CONVENTIONS.md §3.1 and
the allowed terms from its §9 (Paths column), so a new prefix or allowed term needs no change here. Links may leave
the design root only for the vision/ and prompts/ folders beside it (in the design root's parent).

Gates:
  1  hard-fail scan: the hard rows of TechPatterns.tsv and the hashed IP denylist; zero hits
  2  soft-flag scan: the soft rows of TechPatterns.tsv and the hashed string denylist (quoted strings of three or more
     words); every hit must be allowed by CONVENTIONS.md §9 for its path
  3  relative links resolve and their anchors exist (GitHub slugs); no link leaves the root except to vision/ and
     prompts/ beside it
  4  every Mermaid block parses (MermaidCheck.js, pinned Mermaid build in headless Chromium through Playwright); the
     gate fails when the build or Chromium is missing, and is skipped only with --skip-mermaid
  5  requirement ids: prefix registered (CONVENTIONS.md §3.1), defined exactly once, every citation exists
  6  DEC and ADR citations exist; every Open DEC is cited inside at least one requirement
  7  asset closure: cited asset ids exist in assets/asset_manifest.csv; every manifest row names a consumer
  8  tuning keys: defined once; each cited by a requirement (in the requirement text or the row's requirement column)
  9  truncation words outside code: zero
  10 files of 120 KB or less (assets/*.csv and assets/GameAssets.md exempt); every folder has a README.md; reference and
     component documents carry their template headings
  11 --write-index writes REQUIREMENTS_INDEX.md and requirements_index/ at the root
Prints the per-gate table; --report also writes it, with every failure, as Markdown. Exit status 1 when any gate
fails, 2 on a usage error.
"""
import argparse
import bisect
import csv
import fnmatch
import glob
import hashlib
import os
import re
import shutil
import subprocess
import sys
import unicodedata
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
TECH_PATTERNS = os.path.join(HERE, 'TechPatterns.tsv')
IP_DENYLIST = os.path.join(HERE, 'IpDenylist.sha256')
STRINGS_DENYLIST = os.path.join(HERE, 'OriginalStrings.sha256')
MERMAID_CHECK = os.path.join(HERE, 'MermaidCheck.js')
MERMAID_JS = os.path.join(HERE, 'node_modules', 'mermaid', 'dist', 'mermaid.min.js')

MAX_PHRASE_WORDS = 5           # denylist entries are phrases of one to five words
MIN_AFFIX_CHARS = 6            # a listed word of this length or more is also found at the start or end of a word
MAX_FILE_BYTES = 120 * 1024
SIZE_EXEMPT = ('assets/GameAssets.md',)
OUTSIDE_FOLDERS = ('vision', 'prompts')

REF_HEADINGS = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table',
                'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
CMP_HEADINGS = ['Responsibilities and boundaries', 'Interfaces', 'Data model', 'Behaviour and algorithms',
                'Concurrency and performance budget', 'Failure modes, diagnostics and telemetry',
                'Acceptance tests', 'Open decisions']
GATE_NAMES = {1: 'Hard-fail patterns', 2: 'Soft-flag patterns', 3: 'Links and anchors', 4: 'Mermaid',
              5: 'Requirement ids', 6: 'DEC and ADR references', 7: 'Asset closure', 8: 'Tuning keys',
              9: 'Truncation words', 10: 'Sizes, READMEs, templates', 11: 'Requirements index'}

TRUNC = re.compile(r'\betc\b\.?|\band so on\b|\band others\b|\bvarious\b|\bsimilar to the above\b|…', re.I)
FENCE = re.compile(r'^[ \t]*(```|~~~)')
LINK = re.compile(r'(?<!!)\[(?:[^\]\[]|\[[^\]]*\])*\]\(([^)\s]+)(?:\s+"[^"]*")?\)')
QUOTED = re.compile(r'"([^"\n]{3,300})"|“([^”\n]{3,300})”')
REGEX_META = set('.^$*+?{}[]|()')


# ------------------------------------------------------------------------------------------------ normalisation
# One normalisation for the denylist entries and for the scanned text, so that equal text gives equal hashes.

WORD_STRIP = re.compile(r'[^a-z0-9!]')
JOIN_SPLIT = re.compile(r'[^a-z0-9_]+')
DASHES = re.compile('[-\u2010-\u2015\u2212\u2e3a\u2e3b\ufe58\ufe63\uff0d]')   # hyphen-minus and Unicode dashes
NON_ALNUM = re.compile(r'[^A-Za-z0-9]+')
CASE_STEP = re.compile(r'(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])')


def fold(text):
  """NFKD, then ASCII only (accents and other non-ASCII characters dropped)."""
  if text.isascii():
    return text
  return unicodedata.normalize('NFKD', text).encode('ascii', 'ignore').decode('ascii')


def phrase_words(text):
  """Words of a phrase: hyphens and dashes become spaces; then ASCII-folded, lower case, split on whitespace, and
  every other punctuation mark removed from inside each word (so an identifier such as a_b or A::B is one word).

  An exclamation mark stays part of its word: it is the only thing that tells one listed interface string apart from
  an everyday phrase of the design set."""
  return [w for w in (WORD_STRIP.sub('', chunk) for chunk in fold(DASHES.sub(' ', text)).lower().split()) if w]


def joined_words(text):
  """The second view of scanned text: as phrase_words, but split at every character that is not a letter, a digit or
  an underscore, and with the underscores removed, so that a listed file stem or identifier is found before a file
  extension, inside a path or before ::."""
  return [w for w in (c.replace('_', '') for c in JOIN_SPLIT.split(fold(DASHES.sub(' ', text)).lower())) if w]


def split_words(text):
  """The third view of scanned text: split at every character that is not an ASCII letter or digit (accents are
  folded first) and at every lower-to-upper case step, so that a listed name inside a CamelCase identifier, before a
  possessive or between slashes is found too."""
  if not text.isascii():
    text = ''.join(c for c in unicodedata.normalize('NFKD', text) if not unicodedata.combining(c))
  return [part.lower() for chunk in NON_ALNUM.split(text) for part in CASE_STEP.split(chunk) if part]


VIEWS = (phrase_words, joined_words, split_words)


def normalise_phrase(text):
  """The canonical form that IpDenylist.sha256 hashes: phrase words joined by single spaces."""
  return ' '.join(phrase_words(text))


def normalise_string(text):
  """The canonical form of a quoted interface string (OriginalStrings.sha256): ASCII-folded, lower case, printf and
  *T placeholders removed, punctuation removed except apostrophes inside words, whitespace collapsed."""
  s = fold(text.replace('\\n', ' ')).lower()
  s = re.sub(r'%[-+ #0-9.]*[a-z]', ' ', s)
  s = re.sub(r'\*[a-z]\b', ' ', s)
  s = re.sub(r"[^a-z0-9']+", ' ', s)
  s = re.sub(r"(?<![a-z])'|'(?![a-z])", ' ', s)
  return re.sub(r'\s+', ' ', s).strip()


def digest(canonical):
  return hashlib.sha256(canonical.encode('ascii')).hexdigest()


def load_hashes(path):
  """A hash file: one lower-case SHA-256 hex digest per line; lines starting with # are comments."""
  with open(path, encoding='utf-8') as fh:
    return {ln.strip().lower() for ln in fh if ln.strip() and not ln.startswith('#')}


def text_phrases(text):
  """Yield (line, phrase) for every 1 to MAX_PHRASE_WORDS word n-gram of the three views of the text, and for every
  prefix and suffix of MIN_AFFIX_CHARS or more characters of each longer word of the joined view (a listed name inside
  a compound or before a plural ending). N-grams run across line breaks inside a paragraph and stop at a blank line;
  the line is that of the n-gram's first word."""
  for view in VIEWS:
    words, lines = [], []
    for ln, line in enumerate(text.split('\n'), 1):
      if not line.strip():
        yield from _ngrams(words, lines)
        words, lines = [], []
        continue
      w = view(line)
      words.extend(w)
      lines.extend([ln] * len(w))
    yield from _ngrams(words, lines)
  for ln, line in enumerate(text.split('\n'), 1):
    for w in joined_words(line):
      for k in range(MIN_AFFIX_CHARS, len(w)):
        yield ln, w[:k]
        yield ln, w[-k:]


def _ngrams(words, lines):
  for i in range(len(words)):
    phrase = words[i]
    yield lines[i], phrase
    for j in range(i + 1, min(i + MAX_PHRASE_WORDS, len(words))):
      phrase = phrase + ' ' + words[j]
      yield lines[i], phrase


class Denylist:
  """A hashed phrase denylist. Each distinct phrase of a file is hashed once."""

  def __init__(self, hashes):
    self.hashes = hashes

  def listed(self, phrase):
    return digest(phrase) in self.hashes

  def scan(self, text):
    """Hits as (line, phrase), one per phrase and line, in line order."""
    found = {p for p in {phrase for _, phrase in text_phrases(text)} if self.listed(p)}
    if not found:
      return []
    return sorted({(ln, phrase) for ln, phrase in text_phrases(text) if phrase in found})


def scan_strings(text, hashes):
  """Hits as (line, quoted string) for quoted strings of three or more words whose canonical form is listed."""
  out = []
  for ln, line in enumerate(text.split('\n'), 1):
    for m in QUOTED.finditer(line):
      s = m.group(1) or m.group(2)
      n = normalise_string(s)
      if len(n.split()) >= 3 and digest(n) in hashes:
        out.append((ln, s))
  return out


# ------------------------------------------------------------------------------------------------ pattern rows

def required_literal(pattern):
  """A lower-case literal that every match must contain, or None (conservative); a cheap pre-filter."""
  body = pattern.replace(r'\b', '')
  out, i = [], 0
  while i < len(body):
    ch = body[i]
    if ch == '\\':
      if i + 1 < len(body) and not body[i + 1].isalnum():
        out.append(body[i + 1])
        i += 2
        continue
      return None
    if ch in REGEX_META:
      return None
    out.append(ch)
    i += 1
  lit = ''.join(out).lower()
  return lit if len(lit) >= 3 else None


class PatternRow:
  def __init__(self, row):
    self.pattern = row['pattern']
    self.flags = row.get('flags') or ''
    self.gate = (row.get('gate') or '').strip().lower()
    self.cls = row.get('class') or ''
    self.rx = re.compile(self.pattern, re.IGNORECASE if 'i' in self.flags else 0)
    self.literal = required_literal(self.pattern)

  def scan(self, text, low, line_of):
    if self.literal is not None and self.literal not in low:
      return
    for m in self.rx.finditer(text):
      yield line_of(m.start()), m.group(0).replace('\n', ' ')


def load_pattern_rows(path):
  """TechPatterns.tsv: TAB-separated, header row, columns pattern, flags (i = ignore case), gate (hard or soft),
  class and note."""
  with open(path, encoding='utf-8', newline='') as fh:
    rows = [PatternRow(r) for r in csv.DictReader(fh, delimiter='\t') if r.get('pattern')]
  bad = [r.pattern for r in rows if r.gate not in ('hard', 'soft')]
  if bad:
    raise ValueError(f'gate column must be "hard" or "soft": {bad[0]}')
  return rows


def iter_files(root):
  for top, dirs, files in os.walk(root):
    dirs[:] = sorted(d for d in dirs if d != '.git')
    for f in sorted(files):
      yield os.path.join(top, f)


def read_text(path):
  """The file as text, or None for a binary file (a NUL byte in the first 8 KB)."""
  with open(path, 'rb') as fh:
    data = fh.read()
  if b'\0' in data[:8192]:
    return None
  return data.decode('utf-8', errors='replace')


def scan_tree(root, rows, denylist=None, string_hashes=None):
  """Hits as (relative path, line, class, match) for every file under root, in file order."""
  hits = []
  for path in iter_files(root):
    text = read_text(path)
    if text is None:
      continue
    rel = os.path.relpath(path, root).replace(os.sep, '/')
    newlines = [m.start() for m in re.finditer('\n', text)]
    low = text.lower()

    def line_of(pos):
      return bisect.bisect_right(newlines, pos) + 1

    for row in rows:
      for ln, match in row.scan(text, low, line_of):
        hits.append((rel, ln, row.cls, match))
    if denylist is not None:
      for ln, phrase in denylist.scan(text):
        hits.append((rel, ln, 'ip-denylist', phrase))
    if string_hashes is not None:
      for ln, s in scan_strings(text, string_hashes):
        hits.append((rel, ln, 'orig-string', s))
  return hits


# ------------------------------------------------------------------------------------------------ Markdown helpers

def prose_lines(text):
  """Yield (line number, line) outside fenced code blocks."""
  inside = False
  for i, line in enumerate(text.split('\n'), 1):
    if FENCE.match(line):
      inside = not inside
      continue
    if not inside:
      yield i, line


def strip_code_spans(line):
  return re.sub(r'`[^`]*`', ' ', line)


def cells_of(line):
  """Split a Markdown table row on unescaped pipes."""
  return [c.strip() for c in re.split(r'(?<!\\)\|', line.strip().strip('|'))]


def slug(heading):
  """GitHub heading anchor."""
  h = re.sub(r'!\[([^\]]*)\]\([^)]*\)', r'\1', heading)
  h = re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', h)
  h = re.sub(r'<[^>]+>', '', h)
  h = h.strip().lower()
  h = re.sub(r'[^\w\- ]', '', h)
  return h.replace(' ', '-')


def anchors_of(text):
  out, seen = set(), Counter()
  for _, line in prose_lines(text):
    m = re.match(r'^(#{1,6})\s+(.*?)\s*#*\s*$', line)
    if m:
      s = slug(m.group(2))
      out.add(s if seen[s] == 0 else f'{s}-{seen[s]}')
      seen[s] += 1
    for a in re.findall(r'<a\s+(?:id|name)="([^"]+)"', line):
      out.add(a)
  return out


# ------------------------------------------------------------------------------------------------ the gates

class Check:
  def __init__(self, root, skip_mermaid=False, write_index=False):
    self.root = os.path.abspath(root)
    self.parent = os.path.dirname(self.root)
    self.skip_mermaid = skip_mermaid
    self.write_index = write_index
    self.results = {}    # gate -> failure strings
    self.info = {}       # gate -> summary
    self.skipped = set()
    paths = sorted(glob.glob(os.path.join(self.root, '**', '*.md'), recursive=True))
    self.text = {}
    for p in paths:
      with open(p, encoding='utf-8') as fh:
        self.text[self.rel(p)] = fh.read()
    self.conventions = self.text.get('CONVENTIONS.md', '')

  def rel(self, path):
    return os.path.relpath(path, self.root).replace(os.sep, '/')

  def fail(self, gate, message):
    self.results.setdefault(gate, []).append(message)

  def run(self):
    try:
      rows = load_pattern_rows(TECH_PATTERNS)
    except (OSError, ValueError, re.error) as e:
      rows = None
      self.fail(1, f'cannot load TechPatterns.tsv: {e}')
      self.fail(2, f'cannot load TechPatterns.tsv: {e}')
    self.gate_hard(rows)
    self.gate_soft(rows)
    self.gate_links()
    self.gate_mermaid()
    self.gate_requirements()
    self.gate_decisions()
    self.gate_tuning_keys()      # before gate 7, which needs the keys
    self.gate_assets()
    self.gate_truncation()
    self.gate_files()
    self.gate_index()

  # ---------------------------------------------------------------- gate 1
  def gate_hard(self, rows):
    try:
      denylist = Denylist(load_hashes(IP_DENYLIST))
    except OSError as e:
      denylist = None
      self.fail(1, f'cannot load IpDenylist.sha256: {e}')
    if rows is None or denylist is None:
      self.info[1] = 'not run'
      return
    hits = scan_tree(self.root, [r for r in rows if r.gate == 'hard'], denylist=denylist)
    for f, ln, cls, m in hits:
      self.fail(1, f'{f}:{ln} [{cls}] {m}')
    self.info[1] = f'{len(hits)} hard-fail hits'

  # ---------------------------------------------------------------- gate 2
  def allowed_terms(self):
    """CONVENTIONS.md §9 rows: (term, [path globs]); CONVENTIONS.md itself is always allowed."""
    sec = re.search(r'^## 9 Allowed terms\n(.*?)(?=^## |\Z)', self.conventions, re.M | re.S)
    rows = []
    if not sec:
      self.fail(2, 'CONVENTIONS.md has no "## 9 Allowed terms" section')
      return rows
    lines = [ln for ln in sec.group(1).split('\n') if ln.startswith('|')]
    if not lines:
      return rows
    head = [c.lower() for c in cells_of(lines[0])]
    if 'paths' not in head:
      self.fail(2, 'CONVENTIONS.md §9 table has no "Paths" column')
      return rows
    pi = head.index('paths')
    for ln in lines[2:]:
      cells = cells_of(ln)
      globs = re.findall(r'`([^`]+)`', cells[pi]) if pi < len(cells) else []
      rows.append((cells[0].lower(), globs + ['CONVENTIONS.md']))
    return rows

  def gate_soft(self, rows):
    allow = self.allowed_terms()
    try:
      string_hashes = load_hashes(STRINGS_DENYLIST)
    except OSError as e:
      string_hashes = None
      self.fail(2, f'cannot load OriginalStrings.sha256: {e}')
    if rows is None or string_hashes is None:
      self.info[2] = 'not run'
      return
    hits = scan_tree(self.root, [r for r in rows if r.gate == 'soft'], string_hashes=string_hashes)
    n_allowed = 0
    for f, ln, cls, m in hits:
      term = m.lower().replace(' ', '-')
      if any(term == t.replace(' ', '-') and any(fnmatch.fnmatch(f, g) for g in globs) for t, globs in allow):
        n_allowed += 1
      else:
        self.fail(2, f'{f}:{ln} [{cls}] {m}')
    self.info[2] = (f'{len(hits)} soft hits, {n_allowed} allowed by CONVENTIONS §9, '
                    f'{len(self.results.get(2, []))} not allowed')

  # ---------------------------------------------------------------- gate 3
  def gate_links(self):
    anchors = {f: anchors_of(t) for f, t in self.text.items()}
    outside = [os.path.join(self.parent, d) for d in OUTSIDE_FOLDERS]
    n_links = 0
    for f, t in self.text.items():
      base = os.path.dirname(os.path.join(self.root, f))
      for ln, line in prose_lines(t):
        for target in LINK.findall(re.sub(r'`[^`]*`', 'code', line)):
          if re.match(r'^[a-z][a-z0-9+.-]*:', target, re.I):
            continue
          n_links += 1
          path, _, anchor = target.partition('#')
          dest = os.path.normpath(os.path.join(base, path)) if path else os.path.join(self.root, f)
          if not (dest == self.root or dest.startswith(self.root + os.sep)):
            if not any(dest == o or dest.startswith(o + os.sep) for o in outside):
              self.fail(3, f'{f}:{ln} link leaves the design root: {target}')
              continue
          if not os.path.exists(dest):
            self.fail(3, f'{f}:{ln} broken link: {target}')
            continue
          if anchor:
            if os.path.isdir(dest):
              dest = os.path.join(dest, 'README.md')
            if dest.endswith('.md'):
              if dest.startswith(self.root + os.sep):
                have = anchors.get(self.rel(dest))
              else:
                with open(dest, encoding='utf-8') as fh:
                  have = anchors_of(fh.read())
              if have is not None and anchor.lower() not in have and anchor not in have:
                self.fail(3, f'{f}:{ln} missing anchor: {target}')
    self.info[3] = f'{n_links} relative links checked'

  # ---------------------------------------------------------------- gate 4
  def gate_mermaid(self):
    if self.skip_mermaid:
      self.skipped.add(4)
      self.info[4] = 'skipped (--skip-mermaid)'
      return
    setup = 'run "npm ci --prefix Tools/DesignCheck" and "npx --prefix Tools/DesignCheck playwright install chromium"'
    if not os.path.exists(MERMAID_JS):
      self.fail(4, f'pinned Mermaid build missing: {os.path.relpath(MERMAID_JS, self.parent)} ({setup}, '
                   'or pass --skip-mermaid)')
      self.info[4] = 'not run'
      return
    node = shutil.which('node')
    if not node:
      self.fail(4, 'node is not on PATH (Node.js runs MermaidCheck.js)')
      self.info[4] = 'not run'
      return
    r = subprocess.run([node, MERMAID_CHECK, self.root, MERMAID_JS], capture_output=True, text=True)
    summary = None
    for line in r.stdout.splitlines():
      if line.startswith('FAIL '):
        self.fail(4, line[5:])
      elif line.startswith('ERROR '):
        self.fail(4, f'{line[6:]} (or pass --skip-mermaid)')
      elif line.startswith('blocks='):
        summary = line
    if summary is None and not self.results.get(4):
      self.fail(4, f'MermaidCheck.js failed (exit {r.returncode}): {r.stderr.strip()[:300]}')
    self.info[4] = summary or 'not run'

  # ---------------------------------------------------------------- gate 5
  def registry(self):
    sec = re.search(r'^### 3\.1 Prefix registry\n(.*?)(?=^##)', self.conventions, re.M | re.S)
    if not sec:
      return {}
    rx = r'^\|\s*`([A-Z][A-Z-]*[A-Z])`\s*\|[^|]*\|\s*`?([^`|]*)`?'
    return {m: d for m, d in re.findall(rx, sec.group(1), re.M)}

  def gate_requirements(self):
    self.prefixes = self.registry()
    if not self.prefixes:
      self.fail(5, 'CONVENTIONS.md §3.1 prefix registry not found')
    pfx = '|'.join(sorted((re.escape(p) for p in self.prefixes), key=len, reverse=True)) or r'(?!)'
    self.def_rx = re.compile(r'\*\*(' + pfx + r')-(\d{3})\*\*')
    any_def = re.compile(r'\*\*((?:R-)?[A-Z]{2,6})-(\d{3})\*\*')
    self.cite_rx = re.compile(r'(?<![\w-])(' + pfx + r')-(\d{3})(?!\d)')
    range_rx = re.compile(r'(?<![\w-])(' + pfx + r')-\d{3}\s*(?:to|–|—|-|…|\.\.)\s*(?:(?:' + pfx + r')-)?'
                          r'\d{3}(?!\d)')
    self.defs = defaultdict(list)     # id -> [(file, line)]
    self.req_blocks = {}              # id -> requirement text, from the id to the next definition or blank line
    for f, t in self.text.items():
      lines = t.split('\n')
      for ln, line in prose_lines(t):
        for p, n in any_def.findall(line):
          if p not in self.prefixes and p not in ('DEC', 'ADR'):
            self.fail(5, f'{f}:{ln} unregistered prefix {p}-{n}')
        for m in self.def_rx.finditer(line):
          rid = f'{m.group(1)}-{m.group(2)}'
          self.defs[rid].append((f, ln))
          body = [line[m.start():]]
          for j in range(ln, min(ln + 40, len(lines))):
            nxt = lines[j]
            if not nxt.strip() or self.def_rx.search(nxt) or nxt.startswith('#'):
              break
            body.append(nxt)
          self.req_blocks[rid] = '\n'.join(body)
    for rid, where in self.defs.items():
      if len(where) > 1:
        self.fail(5, f'{rid} defined {len(where)} times: ' + ', '.join(f'{f}:{n}' for f, n in where))
    n_cites = 0
    for f, t in self.text.items():
      for ln, line in prose_lines(t):
        line2 = range_rx.sub(' ', line)
        for p, n in self.cite_rx.findall(line2):
          n_cites += 1
          if f'{p}-{n}' not in self.defs:
            self.fail(5, f'{f}:{ln} cites {p}-{n}, which is not defined')
        for m in re.findall(r'(?<![\w-])(R-[A-Z]+)-\d{3}', line2):
          if m not in self.prefixes:
            self.fail(5, f'{f}:{ln} cites unregistered prefix {m}')
    self.req_text = '\n'.join(self.req_blocks.values())
    self.info[5] = (f'{len(self.defs)} requirement ids defined, {n_cites} citations, '
                    f'{len(self.prefixes)} prefixes registered')

  # ---------------------------------------------------------------- gate 6
  def gate_decisions(self):
    status = {}
    for f, t in self.text.items():
      if not f.startswith('decisions/') or f.endswith('README.md'):
        continue
      lines = t.split('\n')
      for i, line in enumerate(lines):
        m = re.match(r'^#{2,4} DEC-(\d{3,4})\s', line)
        if m:
          st = ''
          for nxt in lines[i + 1:i + 8]:
            s = re.match(r'^- \*{0,2}Status\*{0,2}:\*{0,2}\s*([A-Za-z]+)', nxt)
            if s:
              st = s.group(1)
              break
          status[int(m.group(1))] = st
    adr_files = glob.glob(os.path.join(self.root, 'ADR', 'ADR-[0-9][0-9][0-9]-*.md'))
    adrs = {int(os.path.basename(p)[4:7]) for p in adr_files}
    for f, t in self.text.items():
      t2 = re.sub(r'(DEC|ADR)-\d{3,4}\s*(?:to|–|-|…)\s*(?:DEC-|ADR-)?\d{3,4}', ' ', t)
      for m in sorted(set(re.findall(r'DEC-(\d{3,4})\b', t2))):
        if int(m) not in status:
          self.fail(6, f'{f}: cites DEC-{m}, which does not exist')
      for m in sorted(set(re.findall(r'ADR-(\d{3})\b', t2))):
        if int(m) not in adrs:
          self.fail(6, f'{f}: cites ADR-{m}, which does not exist')
    cited = {int(x) for x in re.findall(r'DEC-(\d{3,4})\b', self.req_text)}
    open_decs = sorted(n for n, s in status.items() if s == 'Open')
    uncited = [n for n in open_decs if n not in cited]
    for n in uncited:
      self.fail(6, f'Open DEC-{n:03d} is not cited by any requirement')
    self.info[6] = (f'{len(status)} DECs ({len(open_decs)} Open, {len(open_decs) - len(uncited)} of them cited by a '
                    f'requirement), {len(adrs)} ADRs')

  # ---------------------------------------------------------------- gate 8
  def gate_tuning_keys(self):
    self.key_defs = defaultdict(list)   # key -> [(file, line, requirement cell)]
    for f, t in self.text.items():
      lines = t.split('\n')
      cols = None
      for i, line in enumerate(lines):
        if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
          head = [c.lower() for c in cells_of(line)]
          cols = head if head[:2] == ['key', 'value'] else None
          continue
        if cols is None:
          continue
        if not line.startswith('|'):
          cols = None
          continue
        if re.match(r'^\|\s*:?-{3,}', line):
          continue
        cells = cells_of(line)
        m = re.match(r'^`([^`]+)`$', cells[0])
        if not m:
          self.fail(8, f'{f}:{i + 1} tuning row without a single backticked key: {cells[0][:60]}')
          continue
        ri = cols.index('requirement') if 'requirement' in cols else None
        reqcell = cells[ri] if ri is not None and ri < len(cells) else ''
        self.key_defs[m.group(1)].append((f, i + 1, reqcell))
    for k, where in self.key_defs.items():
      if len(where) > 1:
        self.fail(8, f'tuning key {k} defined {len(where)} times: ' + ', '.join(f'{f}:{n}' for f, n, _ in where))
    n_uncited = 0
    for k, where in self.key_defs.items():
      if k in self.req_text:
        continue
      reqs = [f'{p}-{n}' for p, n in self.cite_rx.findall(' '.join(c for _, _, c in where))]
      if any(r in self.defs for r in reqs):
        continue
      n_uncited += 1
      self.fail(8, f'tuning key {k} ({where[0][0]}:{where[0][1]}) is cited by no requirement')
    self.info[8] = f'{len(self.key_defs)} tuning keys, {n_uncited} cited by no requirement'

  # ---------------------------------------------------------------- gate 7
  def gate_assets(self):
    man_path = os.path.join(self.root, 'assets', 'asset_manifest.csv')
    manifest = []
    if os.path.exists(man_path):
      with open(man_path, encoding='utf-8', newline='') as fh:
        manifest = list(csv.DictReader(fh))
    if not manifest:
      self.fail(7, 'assets/asset_manifest.csv missing or empty')
    asset_ids = {r['asset_id'] for r in manifest}
    stems = sorted({a.split('.')[0] for a in asset_ids}, key=len, reverse=True)
    asset_rx = None
    if stems:
      asset_rx = re.compile(r'`((?:' + '|'.join(map(re.escape, stems)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)`')
    n_cites = 0
    for f, t in self.text.items():
      if not (f.startswith('reference/') or f.startswith('architecture/components/')) or not asset_rx:
        continue
      for ln, line in prose_lines(t):
        for a in asset_rx.findall(line):
          if a in self.key_defs:
            continue
          n_cites += 1
          if a not in asset_ids:
            self.fail(7, f'{f}:{ln} cites asset {a}, which is not in the manifest')
    string_prefixes, col = set(), None
    for line in self.conventions.split('\n'):
      if not line.startswith('|'):
        col = None
        continue
      cells = cells_of(line)
      if 'String-id prefix' in cells:
        col = cells.index('String-id prefix')
      elif col is not None and col < len(cells) and not cells[0].startswith('---'):
        string_prefixes.update(re.findall(r'`([a-z_]+)(?:\.[a-z0-9_.]+)?`', cells[col]))
    key_prefixes = {k.split('.')[0] for k in self.key_defs}
    for c in sorted(set(stems) & key_prefixes):
      self.fail(7, f'asset stem "{c}." is also a tuning-key prefix; asset ids and tuning keys must not share a stem')
    for c in sorted(set(stems) & string_prefixes):
      self.fail(7, f'asset stem "{c}." is also a string-id prefix (CONVENTIONS §8); asset ids and string ids must not '
                   'share a stem')
    for r in manifest:
      if not (r.get('consumers') or '').strip():
        self.fail(7, f'manifest row {r["asset_id"]} names no consumer')
    self.info[7] = (f'{len(asset_ids)} asset ids ({len(manifest)} manifest rows), {n_cites} asset citations in '
                    'reference and component documents')

  # ---------------------------------------------------------------- gate 9
  def gate_truncation(self):
    for f, t in self.text.items():
      if f == 'CONVENTIONS.md':
        continue   # it quotes the banned words when it states the rule
      for ln, line in prose_lines(t):
        for m in TRUNC.findall(strip_code_spans(line)):
          self.fail(9, f'{f}:{ln} truncation word "{m}"')
    self.info[9] = f'{len(self.results.get(9, []))} truncation words'

  # ---------------------------------------------------------------- gate 10
  def gate_files(self):
    for p in glob.glob(os.path.join(self.root, '**', '*'), recursive=True):
      r = self.rel(p)
      if os.path.isdir(p):
        if not os.path.exists(os.path.join(p, 'README.md')):
          self.fail(10, f'{r}/ has no README.md')
        continue
      exempt = r in SIZE_EXEMPT or (r.startswith('assets/') and r.endswith('.csv'))
      if os.path.getsize(p) > MAX_FILE_BYTES and not exempt:
        self.fail(10, f'{r} is {os.path.getsize(p) // 1024} KB (> 120 KB)')
    n_templ = 0
    for f, t in self.text.items():
      if os.path.basename(f) == 'README.md':
        continue
      if f.startswith('reference/'):
        heads = REF_HEADINGS
      elif f.startswith('architecture/components/'):
        heads = CMP_HEADINGS
      else:
        continue
      n_templ += 1
      have = [m for _, ln in prose_lines(t) for m in re.findall(r'^##\s+(?:\d+\s+)?(.+?)\s*$', ln)]
      for h in heads:
        if not any(x.lower().startswith(h.lower()) for x in have):
          self.fail(10, f'{f}: template heading "{h}" missing')
    self.info[10] = f'{len(self.text)} documents, {n_templ} checked for template headings'

  # ---------------------------------------------------------------- gate 11
  def section_of(self, f, ln):
    lines = self.text[f].split('\n')
    for j in range(ln - 1, -1, -1):
      m = re.match(r'^(#{2,4})\s+(.*?)\s*$', lines[j])
      if m:
        return m.group(2)
    return ''

  def one_line(self, rid):
    t = self.req_blocks.get(rid, '')
    t = re.sub(r'^\*\*[A-Z-]+-\d{3}\*\*\s*(\((?:MUST|SHOULD|MAY)(?: NOT)?\))?\s*', '', t)
    t = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', t)
    t = re.sub(r'\s+', ' ', t.replace('**', '')).strip()
    m = re.match(r'^(.{20,140}?[.;:])(\s|$)', t)
    t = m.group(1) if m else t
    if len(t) > 140:
      t = t[:140].rsplit(' ', 1)[0]
    return t.replace('|', '\\|')

  def gate_index(self):
    if not self.write_index:
      self.info[11] = 'index not written (no --write-index)'
      return
    generated = '> Generated by `Tools/DesignCheck/CheckDesign.py --write-index`. Do not edit by hand.'
    idx_dir = os.path.join(self.root, 'requirements_index')
    os.makedirs(idx_dir, exist_ok=True)
    for old in glob.glob(os.path.join(idx_dir, '*.md')):
      os.remove(old)
    scope = {}
    for m in re.finditer(r'^\|\s*`([A-Z][A-Z-]*[A-Z])`\s*\|\s*([^|]+?)\s*\|', self.conventions, re.M):
      scope[m.group(1)] = m.group(2)
    by_prefix = defaultdict(list)
    for rid in self.defs:
      by_prefix[rid.rsplit('-', 1)[0]].append(rid)
    pages = []   # (prefix, page file, first id, last id, count)
    for p in self.prefixes:
      rows = []
      for rid in sorted(by_prefix.get(p, []), key=lambda r: int(r.rsplit('-', 1)[1])):
        f, ln = self.defs[rid][0]
        sec = self.section_of(f, ln)
        rows.append((rid, f'| {rid} | {self.one_line(rid)} | [{os.path.basename(f)}](../{f}#{slug(sec)}) |'))
      head = ['| Id | Requirement (first sentence) | Defined in |', '|---|---|---|']
      chunks, cur, size = [], [], 0
      for rid, row in rows:
        if size + len(row.encode()) > 100 * 1024 and cur:
          chunks.append(cur)
          cur, size = [], 0
        cur.append((rid, row))
        size += len(row.encode()) + 1
      if cur:
        chunks.append(cur)
      for k, chunk in enumerate(chunks, 1):
        name = p.lower() + (f'_{k}' if len(chunks) > 1 else '') + '.md'
        title = f'# {p} requirements' + (f' (part {k} of {len(chunks)})' if len(chunks) > 1 else '')
        body = [title, '', generated, '',
                f'Scope: {scope.get(p, "")}. Back to the [requirements index](../REQUIREMENTS_INDEX.md).', '']
        body += head + [r for _, r in chunk]
        with open(os.path.join(idx_dir, name), 'w', encoding='utf-8', newline='\n') as fh:
          fh.write('\n'.join(body) + '\n')
        pages.append((p, name, chunk[0][0], chunk[-1][0], len(chunk)))
    out = ['# Requirements index', '', generated, '',
           f'{len(self.defs)} requirements under {len(self.prefixes)} prefixes. Every requirement id maps to the '
           'document and section that defines it, with its first sentence. Ids are stable and never reused.', '',
           '| Prefix | Scope | Requirements | Index page |', '|---|---|---|---|']
    for p, name, a, b, n in pages:
      out.append(f'| `{p}` | {scope.get(p, "")} | {n} | [{a} to {b}](requirements_index/{name}) |')
    with open(os.path.join(self.root, 'REQUIREMENTS_INDEX.md'), 'w', encoding='utf-8', newline='\n') as fh:
      fh.write('\n'.join(out) + '\n')
    with open(os.path.join(idx_dir, 'README.md'), 'w', encoding='utf-8', newline='\n') as fh:
      fh.write('# Requirements index pages\n\nGenerated pages, one per requirement prefix. Start from the '
               '[requirements index](../REQUIREMENTS_INDEX.md).\n')
    big = max(os.path.getsize(os.path.join(idx_dir, n)) for _, n, *_ in pages) // 1024 if pages else 0
    self.info[11] = f'REQUIREMENTS_INDEX.md and {len(pages)} index pages written (largest {big} KB)'

  # ---------------------------------------------------------------- report
  def status(self, gate):
    if self.results.get(gate):
      return 'FAIL'
    return 'SKIP' if gate in self.skipped else 'PASS'

  def table(self):
    rows = []
    for g in range(1, 12):
      rows.append((g, GATE_NAMES[g], self.status(g), len(self.results.get(g, [])), self.info.get(g, '')))
    return rows

  def markdown(self):
    rep = ['# Design gate report', '', f'Design root: `{os.path.basename(self.root)}`', '',
           '| Gate | Result | Failures | Summary |', '|---|---|---|---|']
    for g, name, st, n, summary in self.table():
      rep.append(f'| {g} {name} | {st} | {n} | {summary} |')
    for g in range(1, 12):
      if self.results.get(g):
        rep += ['', f'## Gate {g} {GATE_NAMES[g]}', ''] + [f'- {x}' for x in self.results[g]]
    return '\n'.join(rep) + '\n'

  def failed(self):
    return any(self.results.values())


def main(argv=None):
  ap = argparse.ArgumentParser(description='Run the design gates over a design root.')
  ap.add_argument('root', help='the design root, for example Design')
  ap.add_argument('--write-index', action='store_true', help='write REQUIREMENTS_INDEX.md and requirements_index/')
  ap.add_argument('--report', help='also write the gate table and every failure to this Markdown file')
  ap.add_argument('--skip-mermaid', action='store_true', help='skip gate 4 (it fails when Mermaid is not installed)')
  args = ap.parse_args(argv)
  if not os.path.isdir(args.root):
    print(f'CheckDesign: {args.root} is not a directory', file=sys.stderr)
    return 2
  check = Check(args.root, skip_mermaid=args.skip_mermaid, write_index=args.write_index)
  check.run()
  if args.report:
    with open(args.report, 'w', encoding='utf-8', newline='\n') as fh:
      fh.write(check.markdown())
  for g, name, st, n, summary in check.table():
    print(f'gate {g:2} {name:28} {st} {n:5}  {summary}')
  return 1 if check.failed() else 0


if __name__ == '__main__':
  sys.exit(main())
