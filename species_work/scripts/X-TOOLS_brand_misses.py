"""X-TOOLS: sample the brand-class hits of the run's list that the hashed denylist misses (console only).
Usage: python3 -I X-TOOLS_brand_misses.py <scan_patterns.py> <forbidden.tsv> <CheckDesign.py> <corpus dir>"""
import importlib.util
import sys
from collections import Counter


def load(name, path):
  spec = importlib.util.spec_from_file_location(name, path)
  m = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(m)
  return m


sp = load('sp', sys.argv[1])
cd = load('cd', sys.argv[3])
brand = [p for p in sp.load_patterns(sys.argv[2]) if p.cls in ('brand', 'map-name', 'string-key')]
deny = cd.Denylist(cd.load_hashes(cd.IP_DENYLIST))
miss = Counter()
for path in cd.iter_files(sys.argv[4]):
  text = cd.read_text(path)
  if text is None:
    continue
  hits = []
  sp.scan_file(path, text, brand, lambda p, ln, cls, m: hits.append((ln, cls, m)))
  dl = {ln for ln, _ in deny.scan(text)}
  for ln, cls, m in hits:
    if ln not in dl:
      miss[(cls, m.lower())] += 1
for (cls, m), n in miss.most_common(60):
  print(n, cls, m)
print('distinct', len(miss), 'total', sum(miss.values()))
