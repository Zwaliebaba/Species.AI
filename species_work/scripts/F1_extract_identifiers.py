#!/usr/bin/env python3
"""F1: extract the prose vocabulary of ORIG and the original code identifiers.

Usage: python3 -I F1_extract_identifiers.py <ORIG dir> <out dir>

Writes:
  <out>/prose_vocab.tsv   word, lower_count, any_count   (words seen in prose, outside code)
  <out>/identifiers.tsv   identifier, shape, sources, count, example_context

Identifier sources (brief F1): ORIG/21 (inline code and fenced code), ORIG/90 §2
(term column and inline code of the A–Z table), ORIG/91 (class names, inline code,
key source files: basenames only).
Inputs are treated as data only.
"""
import csv
import os
import re
import sys
from collections import Counter, defaultdict

FENCE = re.compile(r'^\s*(```|~~~)')
INLINE = re.compile(r'`([^`\n]+)`')
LINK_URL = re.compile(r'\]\([^)]*\)')
WORD = re.compile(r"[A-Za-z][A-Za-z'-]*[A-Za-z]|[A-Za-z]")
# identifier-like tokens inside code
IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]*(?:::~?[A-Za-z_][A-Za-z0-9_]*)*')
FILE = re.compile(r'(?<![\w.-])([A-Za-z0-9_][\w-]*\.(?:cpp|h|inc|txt|bmp|shp|ogg|dat|anim|max|psp|ico|icns|png|jpg|avi|ini|bat|iss|vcproj|sln|mwm|xpm|grd|pdf|docx|tga|wav|csv|rc|plist))\b')

CPP_WORDS = set('''
alignas alignof and asm auto bool break case catch char class const constexpr const_cast continue decltype
default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline
int long mutable namespace new noexcept not nullptr operator or private protected public register
reinterpret_cast return short signed sizeof static static_assert static_cast struct switch template this
throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor
define ifdef ifndef endif include pragma undef elif NULL TRUE FALSE BOOL DWORD WORD BYTE UINT HRESULT
std size_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t
'''.split())


def iter_lines(path):
    with open(path, encoding='utf-8', errors='replace') as fh:
        for i, line in enumerate(fh, 1):
            yield i, line.rstrip('\n')


def prose_and_code(path):
    """Yield (lineno, prose_text, [inline code spans], is_fenced, raw)."""
    in_fence = False
    for i, line in iter_lines(path):
        if FENCE.match(line):
            in_fence = not in_fence
            yield i, '', [], True, line
            continue
        if in_fence:
            yield i, '', [], True, line
            continue
        line_nourl = LINK_URL.sub(']', line)
        # drop link texts truncated mid-code (odd number of backticks inside [...])
        line_nourl = re.sub(r'\[[^\]]*\]', lambda m: m.group(0) if m.group(0).count('`') % 2 == 0 else ' ', line_nourl)
        spans = INLINE.findall(line_nourl)
        prose = INLINE.sub(' ', line_nourl)
        yield i, prose, spans, False, line


def shape_of(tok):
    if '::' in tok:
        return 'qualified'
    if re.fullmatch(r'[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+_?', tok):
        return 'macro'
    if re.fullmatch(r'[A-Z][A-Z0-9]+', tok):
        return 'caps_word'
    if re.match(r'[mgs]_[A-Za-z]', tok):
        return 'member'
    humps = re.findall(r'[A-Z][a-z0-9]+|[A-Z]+(?![a-z])', tok)
    if '_' in tok:
        return 'snake'
    if re.fullmatch(r'[A-Z][a-z0-9]+', tok):
        return 'single'
    if re.fullmatch(r'[a-z][a-z0-9]*', tok):
        return 'lower'
    if len(humps) >= 2 or re.search(r'[a-z][A-Z]', tok):
        return 'compound'
    return 'other'


def main():
    orig, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    md_files = sorted(f for f in os.listdir(orig) if f.endswith('.md'))

    # 1 prose vocabulary over every ORIG document
    lower = Counter()
    anyc = Counter()
    for f in md_files:
        for _, prose, _, fenced, _ in prose_and_code(os.path.join(orig, f)):
            if fenced or not prose:
                continue
            for w in WORD.findall(prose):
                anyc[w.lower()] += 1
                if w[0].islower():
                    lower[w.lower()] += 1
    with open(os.path.join(out, 'prose_vocab.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['word', 'lower_count', 'any_count'])
        for w, c in sorted(anyc.items(), key=lambda x: (-x[1], x[0])):
            wr.writerow([w, lower[w], c])

    # 2 identifiers
    found = defaultdict(lambda: {'sources': set(), 'count': 0, 'ctx': ''})

    def add(tok, src, ctx):
        tok = tok.strip()
        if not tok or tok in CPP_WORDS:
            return
        if len(tok) < 3 and not re.fullmatch(r'[A-Z]{2}', tok):
            return
        if re.fullmatch(r'[0-9_]+', tok):
            return
        rec = found[tok]
        rec['sources'].add(src)
        rec['count'] += 1
        if not rec['ctx']:
            rec['ctx'] = ctx[:160]

    def harvest_code(text, src, ctx):
        for m in FILE.finditer(text):
            add(m.group(1), src, ctx)
        text_nofile = FILE.sub(' ', text)
        # drop path prefixes such as code/worldobject/
        text_nofile = re.sub(r'\b[\w.-]+/', ' ', text_nofile)
        for m in IDENT.finditer(text_nofile):
            tok = m.group(0)
            add(tok, src, ctx)
            if '::' in tok:
                for part in tok.split('::'):
                    add(part.lstrip('~'), src, ctx)

    # ORIG/21: inline code everywhere, fenced code too (mermaid and C++ sketches)
    p21 = os.path.join(orig, '21_simulation_and_entity_model.md')
    for i, prose, spans, fenced, raw in prose_and_code(p21):
        if fenced:
            if FENCE.match(raw):
                continue
            # only identifier-shaped tokens from fenced blocks (skip mermaid keywords)
            for m in IDENT.finditer(re.sub(r'"[^"]*"', ' ', raw)):
                tok = m.group(0)
                sh = shape_of(tok)
                if sh in ('compound', 'macro', 'member', 'qualified', 'snake'):
                    add(tok, '21', raw.strip())
            continue
        for s in spans:
            harvest_code(s, '21', s)
        # CamelCase compounds in prose headings and tables are identifiers too
        for m in re.finditer(r'\b[A-Z][a-z0-9]+(?:[A-Z][a-z0-9]+)+\b', prose):
            add(m.group(0), '21', prose.strip())

    # ORIG/90 §2: the A–Z table
    p90 = os.path.join(orig, '90_glossary.md')
    in2 = False
    for i, prose, spans, fenced, raw in prose_and_code(p90):
        if raw.startswith('## 2 '):
            in2 = True
            continue
        if raw.startswith('## 3 '):
            break
        if not in2 or not raw.startswith('|') or raw.startswith('|---') or raw.startswith('| Term'):
            continue
        cells = [c.strip() for c in raw.strip().strip('|').split(' | ')]
        term = cells[0]
        for s in INLINE.findall(term):
            harvest_code(s, '90', term)
        bare = INLINE.sub(' ', term)
        for m in IDENT.finditer(re.sub(r'\([^)]*\)', ' ', bare)):
            tok = m.group(0)
            if shape_of(tok) in ('compound', 'macro', 'member', 'qualified', 'snake', 'caps_word'):
                add(tok, '90', term)
            elif shape_of(tok) == 'single' and 'engine (class)' in raw:
                add(tok, '90', term)
        # Term and Definition cells only: the IP column holds placeholders, not original identifiers
        definition = cells[2] if len(cells) > 2 else ''
        definition = LINK_URL.sub(']', definition)
        for s in INLINE.findall(definition):
            harvest_code(s, '90', term)

    # ORIG/91: class names (§4), inline code in every table, key source file basenames
    p91 = os.path.join(orig, '91_cross_reference_index.md')
    for i, prose, spans, fenced, raw in prose_and_code(p91):
        if not raw.startswith('|') or raw.startswith('|---'):
            continue
        for s in spans:
            harvest_code(s, '91', s)
        for m in re.finditer(r'\b[A-Z][a-z0-9]+(?:[A-Z][a-z0-9]+)+\b', prose):
            add(m.group(0), '91', '')

    with open(os.path.join(out, 'identifiers.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['identifier', 'shape', 'sources', 'count', 'context'])
        for tok in sorted(found):
            rec = found[tok]
            sh = 'file' if FILE.fullmatch(tok) else shape_of(tok)
            wr.writerow([tok, sh, ','.join(sorted(rec['sources'])), rec['count'], rec['ctx']])
    print('vocab words', len(anyc), 'identifiers', len(found))


if __name__ == '__main__':
    main()
