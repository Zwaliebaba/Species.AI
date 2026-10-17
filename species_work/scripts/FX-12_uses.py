"""FX-12 helper: list the documents that still use each old identifier.
Usage: python3 -I FX-12_uses.py <design_root> <pairs.tsv>
pairs.tsv: old<TAB>new<TAB>regex (optional; default: the old name as a whole word)
Skips CONVENTIONS.md, glossary/ and requirements_index/ (generated)."""
import glob, os, re, sys
root, pairs = sys.argv[1], sys.argv[2]
files = [p for p in glob.glob(os.path.join(root, '**', '*.md'), recursive=True)]
texts = {os.path.relpath(p, root): open(p, encoding='utf-8').read() for p in files}
for raw in open(pairs, encoding='utf-8').read().splitlines():
    if not raw.strip():
        continue
    parts = raw.split('\t')
    old, new = parts[0], parts[1]
    rx = re.compile(parts[2]) if len(parts) > 2 and parts[2] else re.compile(r'(?<![A-Za-z0-9_])' + re.escape(old) + r'(?![A-Za-z0-9_])')
    hits = []
    for f, t in sorted(texts.items()):
        if f == 'CONVENTIONS.md' or f.startswith('glossary/') or f.startswith('requirements_index/'):
            continue
        n = len(rx.findall(t))
        if n:
            hits.append(f'{f} ({n})')
    print(f'{old}\t{new}\t' + ('; '.join(hits) if hits else 'none'))
