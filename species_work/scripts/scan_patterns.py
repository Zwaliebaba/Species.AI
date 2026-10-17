#!/usr/bin/env python3
"""Shared pattern scanner for the Species reference library.

Usage:
  python3 -I scan_patterns.py [options] <patterns.tsv> <path> [<path> ...]

<patterns.tsv> has the columns  pattern, flags, class, note  (TAB-separated, header row).
  pattern  Python `re` syntax (look-ahead allowed).
  flags    "i" = case-insensitive; empty = case-sensitive.
  class    the hit class printed with each hit.
  A pattern of the form  @strings:<file>  is a string-list check: every quoted string
  ("…" or “…”) of three or more words on a line is normalised (ASCII-folded, lower case,
  punctuation and printf or *T placeholders removed, whitespace collapsed) and reported
  when it equals a line of <file> (path relative to the patterns file).

<path> may be a file or a directory (recursed; .git is skipped; binary files are skipped).

Output: one line per hit,  file:line:class:match  ; exit status 0 = no hit, 1 = hits, 2 = usage error.

Options:
  --prose-only            skip fenced code blocks (``` or ~~~); default scans everything
  --counts <out.tsv>      also write the hit count of every pattern (pattern, flags, class, hits, files)
  --quiet                 do not print hits (useful with --counts)
  --summary               print hit totals per class to stderr
  --exclude <glob>        skip paths matching the glob (repeatable), e.g. --exclude '*/species_work/*'
"""
import argparse
import bisect
import csv
import fnmatch
import os
import re
import sys
import unicodedata

FENCE = re.compile(r'^[ \t]*(```|~~~)')
QUOTED = re.compile(r'"([^"\n]{3,300})"|“([^”\n]{3,300})”')
META = set('.^$*+?{}[]|()')


def norm_string(s):
    s = s.replace('\\n', ' ')
    s = unicodedata.normalize('NFKD', s).encode('ascii', 'ignore').decode('ascii')
    s = s.lower()
    s = re.sub(r'%[-+ #0-9.]*[a-z]', ' ', s)
    s = re.sub(r'\*[a-z]\b', ' ', s)
    s = re.sub(r"[^a-z0-9']+", ' ', s)
    s = re.sub(r"(?<![a-z])'|'(?![a-z])", ' ', s)
    return re.sub(r'\s+', ' ', s).strip()


def required_literal(pat):
    """Return a lower-case literal every match must contain, or None (conservative)."""
    body = pat.replace(r'\b', '')
    out, i = [], 0
    while i < len(body):
        ch = body[i]
        if ch == '\\':
            if i + 1 < len(body) and not body[i + 1].isalnum():
                out.append(body[i + 1])
                i += 2
                continue
            return None
        if ch in META:
            return None
        out.append(ch)
        i += 1
    lit = ''.join(out).lower()
    return lit if len(lit) >= 3 else None


class Pattern:
    def __init__(self, pattern, flags, cls, note, base):
        self.pattern, self.flags, self.cls, self.note = pattern, flags, cls, note
        self.hits, self.files = 0, set()
        self.strings = None
        if pattern.startswith('@strings:'):
            path = os.path.join(base, pattern.split(':', 1)[1])
            with open(path, encoding='utf-8') as fh:
                self.strings = {ln.strip() for ln in fh if ln.strip()}
            self.rx, self.lit = None, None
        else:
            self.rx = re.compile(pattern, re.IGNORECASE if 'i' in (flags or '') else 0)
            self.lit = required_literal(pattern)


def load_patterns(path):
    base = os.path.dirname(os.path.abspath(path))
    pats = []
    with open(path, encoding='utf-8', newline='') as fh:
        for row in csv.DictReader(fh, delimiter='\t'):
            if not row.get('pattern'):
                continue
            pats.append(Pattern(row['pattern'], row.get('flags', ''), row.get('class', ''), row.get('note', ''), base))
    return pats


def iter_files(paths, excludes):
    for p in paths:
        if os.path.isfile(p):
            yield p
            continue
        for root, dirs, files in os.walk(p):
            dirs[:] = sorted(d for d in dirs if d != '.git')
            for f in sorted(files):
                fp = os.path.join(root, f)
                if any(fnmatch.fnmatch(fp, g) for g in excludes):
                    continue
                yield fp


def read_text(path):
    with open(path, 'rb') as fh:
        data = fh.read()
    if b'\0' in data[:8192]:
        return None
    return data.decode('utf-8', errors='replace')


def blank_fences(text):
    out, in_fence = [], False
    for line in text.split('\n'):
        if FENCE.match(line):
            in_fence = not in_fence
            out.append(' ' * len(line))
        elif in_fence:
            out.append(' ' * len(line))
        else:
            out.append(line)
    return '\n'.join(out)


def scan_file(path, text, pats, emit):
    nl = [m.start() for m in re.finditer('\n', text)]
    low = text.lower()

    def line_of(pos):
        return bisect.bisect_right(nl, pos) + 1

    for p in pats:
        if p.strings is not None:
            for ln_no, line in enumerate(text.split('\n'), 1):
                for m in QUOTED.finditer(line):
                    s = m.group(1) or m.group(2)
                    n = norm_string(s)
                    if len(n.split()) >= 3 and n in p.strings:
                        p.hits += 1
                        p.files.add(path)
                        emit(path, ln_no, p.cls, s)
            continue
        if p.lit is not None and p.lit not in low:
            continue
        for m in p.rx.finditer(text):
            p.hits += 1
            p.files.add(path)
            emit(path, line_of(m.start()), p.cls, m.group(0))


def main(argv=None):
    ap = argparse.ArgumentParser(add_help=True, description='Scan files for hard-fail or soft-flag patterns.')
    ap.add_argument('patterns')
    ap.add_argument('paths', nargs='+')
    ap.add_argument('--prose-only', action='store_true')
    ap.add_argument('--counts')
    ap.add_argument('--quiet', action='store_true')
    ap.add_argument('--summary', action='store_true')
    ap.add_argument('--exclude', action='append', default=[])
    try:
        args = ap.parse_args(argv)
    except SystemExit as e:
        return 2 if e.code else 0
    try:
        pats = load_patterns(args.patterns)
    except (OSError, re.error) as e:
        print('scan_patterns: cannot load patterns: %s' % e, file=sys.stderr)
        return 2
    total = 0
    by_class = {}
    out = sys.stdout

    def emit(path, line, cls, match):
        nonlocal total
        total += 1
        by_class[cls] = by_class.get(cls, 0) + 1
        if not args.quiet:
            out.write('%s:%d:%s:%s\n' % (path, line, cls, match.replace('\n', ' ')))

    for path in iter_files(args.paths, args.exclude):
        text = read_text(path)
        if text is None:
            continue
        if args.prose_only:
            text = blank_fences(text)
        scan_file(path, text, pats, emit)
    if args.counts:
        with open(args.counts, 'w', encoding='utf-8', newline='') as fh:
            wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
            wr.writerow(['pattern', 'flags', 'class', 'hits', 'files', 'note'])
            for p in pats:
                wr.writerow([p.pattern, p.flags, p.cls, p.hits, len(p.files), p.note])
    if args.summary:
        print('hits: %d %s' % (total, dict(sorted(by_class.items()))), file=sys.stderr)
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main())
