# S24 helper: summary statistics of the .shp catalogue produced by P12 (drafts/P12_shapes.csv).
# Usage: python3 -I S_24_shape_stats.py <path/to/P12_shapes.csv>
# Prints a Markdown table of aggregate counts used in 24_rendering §4.4.
import csv, sys

rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
n = len(rows)
def ints(col):
    out = []
    for r in rows:
        try:
            out.append(int(r[col]))
        except (ValueError, KeyError):
            pass
    return out
frags = ints('fragments'); depth = ints('max_depth'); markers = ints('markers'); tris = ints('triangles')
fmt = {}
for r in rows:
    fmt[r['format']] = fmt.get(r['format'], 0) + 1
def top(col, k=5):
    s = sorted(rows, key=lambda r: -int(r[col]) if r[col].isdigit() else 0)[:k]
    return ', '.join('%s (%s)' % (r['shape'], r[col]) for r in s)
print('| Statistic | Value |')
print('|---|---|')
print('| Shape files | %d |' % n)
print('| Formats | %s |' % ', '.join('%s: %d' % kv for kv in sorted(fmt.items())))
print('| Fragments total / max | %d / %d |' % (sum(frags), max(frags)))
print('| Max hierarchy depth | %d |' % max(depth))
print('| Markers total / max per shape | %d / %d |' % (sum(markers), max(markers)))
print('| Triangles total / max / mean | %d / %d / %.1f |' % (sum(tris), max(tris), sum(tris) / max(1, len(tris))))
print('| Largest by triangles | %s |' % top('triangles'))
print('| Most fragments | %s |' % top('fragments'))
