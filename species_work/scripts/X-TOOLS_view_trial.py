"""X-TOOLS: trial of extra text views for the hashed scan (console only).
Usage: python3 -I X-TOOLS_view_trial.py <CheckDesign.py> <scan_patterns.py> <forbidden.tsv> <corpus> [<corpus>...]"""
import importlib.util
import re
import sys
import time


def load(n, p):
  s = importlib.util.spec_from_file_location(n, p)
  m = importlib.util.module_from_spec(s)
  s.loader.exec_module(m)
  return m


cd = load('cd', sys.argv[1])
sp = load('sp', sys.argv[2])
ip_pats = [p for p in sp.load_patterns(sys.argv[3]) if p.cls in ('brand', 'map-name', 'string-key', 'identifier')]
JOIN_SPLIT = re.compile(r'[^a-z0-9_]+')
MIN_AFFIX = 6


def joined_words(text):
  out = []
  for w in JOIN_SPLIT.split(cd.fold(cd.DASHES.sub(' ', text)).lower()):
    w = w.replace('_', '')
    if w:
      out.append(w)
  return out


def phrases(text, views, affix):
  for view in views:
    words, lines = [], []
    for ln, line in enumerate(text.split('\n'), 1):
      if not line.strip():
        yield from cd._ngrams(words, lines)
        words, lines = [], []
        continue
      w = view(line)
      words.extend(w)
      lines.extend([ln] * len(w))
    yield from cd._ngrams(words, lines)
  if affix:
    for ln, line in enumerate(text.split('\n'), 1):
      for w in joined_words(line):
        for k in range(MIN_AFFIX, len(w)):
          yield ln, w[:k]
          yield ln, w[-k:]


def scan(deny, text, views, affix):
  found = {p for p in {ph for _, ph in phrases(text, views, affix)} if deny.listed(p)}
  if not found:
    return set()
  return {ln for ln, ph in phrases(text, views, affix) if ph in found}


configs = {
  'P+S (current)': ((cd.phrase_words, cd.split_words), False),
  'P+S+U': ((cd.phrase_words, cd.split_words, joined_words), False),
  'P+S+U+affix': ((cd.phrase_words, cd.split_words, joined_words), True),
}
for corpus in sys.argv[4:]:
  run_lines = {}
  files = []
  for path in cd.iter_files(corpus):
    t = cd.read_text(path)
    if t is None:
      continue
    files.append((path, t))
    hits = set()
    sp.scan_file(path, t, ip_pats, lambda p, ln, cls, m: hits.add(ln))
    run_lines[path] = hits
  print('==', corpus, 'run IP lines', sum(len(v) for v in run_lines.values()))
  for name, (views, affix) in configs.items():
    deny = cd.Denylist(cd.load_hashes(cd.IP_DENYLIST))
    t0 = time.time()
    both = run_only = deny_only = 0
    sample = []
    for path, t in files:
      d = scan(deny, t, views, affix)
      r = run_lines[path]
      both += len(d & r)
      run_only += len(r - d)
      deny_only += len(d - r)
    print(f'  {name:16} {time.time() - t0:6.1f} s  both {both}  run only {run_only}  denylist only {deny_only}')
