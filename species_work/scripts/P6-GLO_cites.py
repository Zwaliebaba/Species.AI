"""List where each given asset id (or any id) is cited in backticks, outside assets/ and requirements_index/."""
import os, re, sys
root = sys.argv[1]
ids = sys.argv[2:]
for dirpath, _, files in os.walk(root):
  for fn in files:
    if not fn.endswith('.md'):
      continue
    p = os.path.join(dirpath, fn)
    rel = os.path.relpath(p, root)
    with open(p, encoding='utf-8') as fh:
      for ln, line in enumerate(fh, 1):
        for i in ids:
          if re.search(r'`' + re.escape(i) + r'`', line) or re.search(r'(?<![\w.])' + re.escape(i) + r'(?![\w])', line):
            print(f'{i}\t{rel}:{ln}\t{line.strip()[:160]}')
