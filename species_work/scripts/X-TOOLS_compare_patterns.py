"""X-TOOLS: compare the retained scans with the run's pattern scans on a corpus that has hits.

Usage: python3 -I X-TOOLS_compare_patterns.py <scan_patterns.py> <forbidden.tsv> <soft.tsv> <ip_tokens.txt>
       <CheckDesign.py> <corpus dir> [<corpus dir> ...]

Prints, for every corpus:
  tech-hard  the run's provenance and legacy-tech rows against the hard rows of TechPatterns.tsv (must be equal)
  tech-soft  the run's history, scoped, unfinished and renamed-term rows against the soft rows (equal except for the
             renamed-term members that moved to the IP denylist)
  strings    the run's @strings check against OriginalStrings.sha256 (must be equal)
  ip         lines flagged by the run's IP classes (brand, identifier, map-name, string-key) against lines flagged by
             IpDenylist.sha256: both, run only (by class), denylist only
Matched text is printed only for the denylist-only lines, to the console, for review; nothing is written.
"""
import bisect
import importlib.util
import os
import re
import sys
from collections import Counter

TECH_HARD = ('provenance', 'legacy-tech')
TECH_SOFT = ('history', 'scoped', 'unfinished', 'renamed-term')


def load(name, path):
  spec = importlib.util.spec_from_file_location(name, path)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


def run_scan(sp, pats, path, text):
  out = []
  sp.scan_file(path, text, pats, lambda p, ln, cls, m: out.append((ln, cls, m.replace('\n', ' '))))
  return out


def main():
  sp_path, hard_tsv, soft_tsv, tokens, cd_path = sys.argv[1:6]
  corpora = sys.argv[6:]
  sp = load('scan_patterns', sp_path)
  cd = load('CheckDesign', cd_path)
  hard = sp.load_patterns(hard_tsv)
  soft = sp.load_patterns(soft_tsv)
  run_tech_hard = [p for p in hard if p.cls in TECH_HARD]
  run_ip = [p for p in hard if p.cls not in TECH_HARD]
  run_tech_soft = [p for p in soft if p.cls in TECH_SOFT]
  run_strings = [p for p in soft if p.strings is not None]
  rows = cd.load_pattern_rows(cd.TECH_PATTERNS)
  new_hard = [r for r in rows if r.gate == 'hard']
  new_soft = [r for r in rows if r.gate == 'soft']
  denylist = cd.Denylist(cd.load_hashes(cd.IP_DENYLIST))
  string_hashes = cd.load_hashes(cd.STRINGS_DENYLIST)
  ip_norm = {cd.normalise_phrase(l) for l in open(tokens, encoding='utf-8') if l.strip()}
  for corpus in corpora:
    c = Counter()
    run_only_cls = Counter()
    deny_only = []
    soft_diff_ok, soft_diff_bad = 0, []
    for path in cd.iter_files(corpus):
      text = cd.read_text(path)
      if text is None:
        continue
      rel = os.path.relpath(path, corpus)
      nl = [m.start() for m in re.finditer('\n', text)]
      low = text.lower()

      def line_of(pos):
        return bisect.bisect_right(nl, pos) + 1

      a = {(ln, m) for ln, _, m in run_scan(sp, run_tech_hard, path, text)}
      b = {(ln, m) for r in new_hard for ln, m in r.scan(text, low, line_of)}
      c['tech-hard run'] += len(a)
      c['tech-hard retained'] += len(b)
      c['tech-hard differ'] += len(a ^ b)
      a = {(ln, m) for ln, _, m in run_scan(sp, run_tech_soft, path, text)}
      b = {(ln, m) for r in new_soft for ln, m in r.scan(text, low, line_of)}
      c['tech-soft run'] += len(a)
      c['tech-soft retained'] += len(b)
      for ln, m in a ^ b:
        if (ln, m) in a and cd.normalise_phrase(m) in ip_norm:
          soft_diff_ok += 1
        else:
          soft_diff_bad.append((rel, ln))
      a = {(ln, m) for ln, _, m in run_scan(sp, run_strings, path, text)}
      b = set(cd.scan_strings(text, string_hashes))
      c['strings run'] += len(a)
      c['strings retained'] += len(b)
      c['strings differ'] += len(a ^ b)
      run_lines = {}
      for ln, cls, _ in run_scan(sp, run_ip, path, text):
        run_lines.setdefault(ln, set()).add(cls)
      deny_lines = {}
      for ln, phrase in denylist.scan(text):
        deny_lines.setdefault(ln, []).append(phrase)
      both = run_lines.keys() & deny_lines.keys()
      c['ip lines both'] += len(both)
      for ln in run_lines.keys() - deny_lines.keys():
        c['ip lines run only'] += 1
        for cls in run_lines[ln]:
          run_only_cls[cls] += 1
      for ln in deny_lines.keys() - run_lines.keys():
        c['ip lines denylist only'] += 1
        deny_only.append((rel, ln, deny_lines[ln]))
    print(f'== {corpus}')
    for k in sorted(c):
      print(f'  {k:26} {c[k]}')
    print(f'  tech-soft differences that are IP members moved to the denylist: {soft_diff_ok}; other: {len(soft_diff_bad)}'
          f' {soft_diff_bad[:5]}')
    print(f'  run-only IP lines by class: {dict(run_only_cls)}')
    for rel, ln, phrases in deny_only[:25]:
      print(f'  denylist only: {rel}:{ln} {phrases}')


if __name__ == '__main__':
  main()
