"""FX-12 helper: show how owning documents use a term.
Usage: python3 -I FX-12_lookup.py <design_root> <terms.tsv> [max_hits] [width]
terms.tsv: term<TAB>path (file or folder, relative to the root; several paths separated by ';')
Prints, per term, up to max_hits lines that contain the term (case-insensitive; a hyphen or a
space between words matches either), each cut to `width` characters around the first match."""
import glob, os, re, sys
root, tsv = sys.argv[1], sys.argv[2]
mx = int(sys.argv[3]) if len(sys.argv) > 3 else 4
width = int(sys.argv[4]) if len(sys.argv) > 4 else 420
for raw in open(tsv, encoding='utf-8').read().splitlines():
    if not raw.strip() or raw.startswith('#'):
        continue
    term, paths = raw.split('\t')[:2]
    words = re.split(r'[\s-]+', term.strip())
    rx = re.compile(r'(?<![A-Za-z])' + r'[\s-]+'.join(re.escape(w) for w in words) + r'(?![a-z])', re.I)
    files = []
    for p in paths.split(';'):
        full = os.path.join(root, p.strip())
        files += sorted(glob.glob(os.path.join(full, '**', '*.md'), recursive=True)) if os.path.isdir(full) else [full]
    print(f'=== {term}')
    n = 0
    for f in files:
        if not os.path.exists(f):
            print('   (missing file', f, ')'); continue
        for i, line in enumerate(open(f, encoding='utf-8'), 1):
            m = rx.search(line)
            if m:
                s = max(0, m.start() - width // 3)
                print(f'  {os.path.relpath(f, root)}:{i}: …{line[s:s + width].strip()}…')
                n += 1
                if n >= mx: break
        if n >= mx: break
    if n == 0:
        print('  (no hit)')
