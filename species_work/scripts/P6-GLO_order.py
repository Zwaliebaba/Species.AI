"""Check the alphabetical order of the glossary rows (README reading conventions)."""
import re, sys
for path in sys.argv[1:]:
  sec = None
  prev = None
  for ln, line in enumerate(open(path, encoding='utf-8'), 1):
    if line.startswith('## '):
      sec = line.strip()
      prev = None
      continue
    if not line.startswith('| ') or line.startswith('| Term') or line.startswith('|---'):
      continue
    term = line.split('|')[1].strip()
    key = re.sub(r'\s*\([^)]*\)', '', term).lower().replace('-', ' ').split()
    if prev and key < prev[0]:
      print(f'{path}:{ln} {term!r} sorts before {prev[1]!r} ({sec})')
    prev = (key, term)
