"""List types, enumerations and free functions declared in the C++ sketches of the library that CONVENTIONS does not name,
and names registered in CONVENTIONS §8.12 that no other document writes.

Usage: python3 -I P6-GLO_registry.py <design root>
"""
import os, re, sys
root = sys.argv[1]
conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read()
conv_names = set(re.findall(r'`([A-Za-z_][A-Za-z0-9_:]*)', conv))
conv_flat = set()
for n in conv_names:
  conv_flat.update(n.split('::'))
decl = {}
alltext = {}
for dp, _, fs in os.walk(root):
  for fn in fs:
    if not fn.endswith('.md'):
      continue
    p = os.path.join(dp, fn)
    rel = os.path.relpath(p, root)
    t = open(p, encoding='utf-8').read()
    alltext[rel] = t
    if rel.startswith('requirements_index') or rel == 'CONVENTIONS.md':
      continue
    for block in re.findall(r'```cpp\n(.*?)```', t, re.S):
      for line in block.split('\n'):
        m = re.match(r'^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct|enum class|using)\s+([A-Z][A-Za-z0-9_]*)', line)
        if m:
          decl.setdefault(m.group(1), rel)
        m = re.match(r'^(?:\[\[[a-z]+\]\]\s*)*(?:static\s+)?(?:const\s+)?[A-Za-z_:<>,\s\*&]+?\s+([A-Z][A-Za-z0-9]*)\s*\(', line)
        if m and not line.startswith(' '):
          decl.setdefault(m.group(1), rel)
missing = sorted((n, f) for n, f in decl.items() if n not in conv_flat)
print('declared, not in CONVENTIONS:')
for n, f in missing:
  print('  ', n, f)
other = '\n'.join(t for r, t in alltext.items() if r != 'CONVENTIONS.md' and not r.startswith('requirements_index'))
s = conv[conv.index('### 8.12'):conv.index('### 8.13')]
reg = set(re.findall(r'`([A-Z][A-Za-z0-9_]*)(?:::[A-Za-z0-9_]+)?`', s))
unused = sorted(n for n in reg if not re.search(r'(?<![A-Za-z0-9_])' + re.escape(n) + r'(?![A-Za-z0-9_])', other))
print('registered in 8.12, used nowhere else:', unused)
