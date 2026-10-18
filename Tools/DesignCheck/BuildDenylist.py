#!/usr/bin/env python3
"""BuildDenylist.py: hash plain-text lists into the denylists that CheckDesign.py reads.

Usage:
  python3 Tools/DesignCheck/BuildDenylist.py [--names <file>] [--strings <file>] [--replace]

  --names <file>    one name or short phrase per line, one to five words once normalised -> IpDenylist.sha256
                    (hard-fail, gate 1). Normalisation: CheckDesign.normalise_phrase.
  --strings <file>  one interface string per line, three or more words once normalised -> OriginalStrings.sha256
                    (soft-flag, gate 2, matched against quoted strings). Normalisation: CheckDesign.normalise_string.
  --replace         rebuild each named hash file from the list alone; without it the new hashes are merged into the
                    hashes already there.

The plain-text lists stay outside the repository: only the hashes are kept. This script never prints a list line; it
prints counts, and the line numbers of lines it rejects. Exit status 1 when a line is rejected (nothing is written).
"""
import argparse
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def load_check_design():
  spec = importlib.util.spec_from_file_location('CheckDesign', os.path.join(HERE, 'CheckDesign.py'))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


HEADERS = {
  'IpDenylist.sha256': [
    '# IpDenylist.sha256: SHA-256 digests of the names and text of the reference product and its IP.',
    '# One digest per normalised phrase of one to five words (CheckDesign.normalise_phrase), sorted.',
    '# Gate 1 of CheckDesign.py hashes every 1-5 word n-gram of the scanned text the same way.',
    '# Built by BuildDenylist.py --names; never edit by hand.',
  ],
  'OriginalStrings.sha256': [
    '# OriginalStrings.sha256: SHA-256 digests of the interface text of the reference product.',
    '# One digest per normalised string of three or more words (CheckDesign.normalise_string), sorted.',
    '# Gate 2 of CheckDesign.py hashes every quoted string of three or more words the same way.',
    '# Built by BuildDenylist.py --strings; never edit by hand.',
  ],
}


def hashes_of(path, normalise, min_words, max_words, digest):
  """(set of digests, rejected line numbers) for the non-blank lines of a list."""
  out, rejected = set(), []
  with open(path, encoding='utf-8') as fh:
    for ln, line in enumerate(fh, 1):
      if not line.strip():
        continue
      canonical = normalise(line)
      n = len(canonical.split())
      if n < min_words or (max_words and n > max_words):
        rejected.append(ln)
        continue
      out.add(digest(canonical))
  return out, rejected


def write_hashes(name, new, replace, load_hashes):
  path = os.path.join(HERE, name)
  old = set() if replace or not os.path.exists(path) else load_hashes(path)
  merged = old | new
  with open(path, 'w', encoding='utf-8', newline='\n') as fh:
    fh.write('\n'.join(HEADERS[name] + sorted(merged)) + '\n')
  return len(old), len(merged)


def main(argv=None):
  ap = argparse.ArgumentParser(description='Hash name and string lists into the CheckDesign.py denylists.')
  ap.add_argument('--names', help='plain-text name list -> IpDenylist.sha256')
  ap.add_argument('--strings', help='plain-text string list -> OriginalStrings.sha256')
  ap.add_argument('--replace', action='store_true', help='rebuild instead of merging')
  args = ap.parse_args(argv)
  if not args.names and not args.strings:
    ap.error('give --names, --strings or both')
  cd = load_check_design()
  jobs = []
  if args.names:
    jobs.append(('IpDenylist.sha256', args.names, cd.normalise_phrase, 1, cd.MAX_PHRASE_WORDS))
  if args.strings:
    jobs.append(('OriginalStrings.sha256', args.strings, cd.normalise_string, 3, 0))
  built, failed = [], False
  for name, src, normalise, lo, hi in jobs:
    new, rejected = hashes_of(src, normalise, lo, hi, cd.digest)
    if rejected:
      failed = True
      limit = f'{lo} to {hi}' if hi else f'{lo} or more'
      print(f'{os.path.basename(src)}: {len(rejected)} lines are not {limit} words once normalised: lines '
            + ', '.join(map(str, rejected[:40])))
    built.append((name, new))
  if failed:
    print('nothing written')
    return 1
  for name, new in built:
    before, after = write_hashes(name, new, args.replace, cd.load_hashes)
    print(f'{name}: {len(new)} digests from the list; {before} before, {after} after')
  return 0


if __name__ == '__main__':
  sys.exit(main())
