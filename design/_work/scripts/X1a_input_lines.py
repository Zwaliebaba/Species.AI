#!/usr/bin/env python3
"""X1a: classify every line of input binding files the way InputManager::parseInputPrefs does
(code/lib/input/input.cpp). Usage: python3 -I X1a_input_lines.py SRC_ROOT CONTROL_TYPES_INC FILE...
Prints per file: blank/comment lines, '=' binding lines, '~' icon lines, 'Assignment not found' lines,
unknown-control lines, and the distinct first spec tokens (driver words)."""
import collections, os, re, sys
SEPS = set(' \t\r\n:,')
def clean(l):
    for ch in '#\n\r':
        i = l.find(ch)
        if i >= 0: l = l[:i]
    return l
def toks(c):
    out, cur = [], ''
    for ch in c:
        if ch in SEPS:
            if cur: out.append(cur); cur = ''
        else: cur += ch
    if cur: out.append(cur)
    return out
def main():
    root, inc = sys.argv[1], sys.argv[2]
    controls = [m.group(1).lower() for m in re.finditer(r'^\s*DEF_CONTROL_TYPE\(\s*(\w+)', open(inc).read(), re.M)]
    for rel in sys.argv[3:]:
        c = collections.Counter(); drivers = collections.Counter(); bad = []
        for n, l in enumerate(open(os.path.join(root, rel), encoding='latin-1').read().split('\n'), 1):
            t = toks(clean(l))
            if not t: c['blank/comment'] += 1; continue
            if len(t) < 2 or t[1] not in ('=', '~'):
                c['assignment-not-found'] += 1; bad.append(f'{n}:{" ".join(t)}'); continue
            if t[0].lower() not in controls:
                c['unknown-control'] += 1; bad.append(f'{n}:{" ".join(t)}'); continue
            if t[1] == '~': c['icon'] += 1; continue
            c['binding'] += 1
            spec = clean(l).split(None)  # spec re-tokenised on whitespace only
            i = spec.index('=') if '=' in spec else 1
            rest = spec[i+1:]
            if '&&' in rest: drivers['(conjunction &&)'] += 1
            elif '++' in rest: drivers['(chord ++)'] += 1
            elif rest and rest[0].lower() in ('not', '!'): drivers['(invert)'] += 1
            elif rest: drivers[rest[0]] += 1
            else: drivers['(empty spec)'] += 1
        print(rel, dict(c))
        print('   spec heads:', dict(drivers))
        for b in bad: print('   rejected line', b)
main()
