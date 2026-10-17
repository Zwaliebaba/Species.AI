# usage: G05_keys.py <file> : tuning-key consistency check
import sys, re, pathlib
t = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
rows = re.findall(r'^\| `(ref\.econ\.[a-z0-9_.]+)` \|', t, re.M)
dup = sorted({r for r in rows if rows.count(r) > 1})
body = re.sub(r'^\| `ref\.econ\.[^\n]*$', '', t, flags=re.M)  # text outside table rows
used = set(re.findall(r'`(ref\.econ\.[a-z0-9_.]+)`', body))
print('rows', len(rows), 'dup', dup)
print('used-not-in-table', sorted(used - set(rows)))
print('table-not-used', sorted(set(rows) - used))
