"""P6-RND: replace every 'normalise(' with 'Normalize(' in the given files of P6-RND (R4a-59, P6-RB request 14).

Usage: python3 -I P6-RND_normalize.py <file.md> [<file.md> ...]
Prints the count per file; prose such as 'normalising' is untouched.
"""
import pathlib
import sys

for name in sys.argv[1:]:
  p = pathlib.Path(name)
  text = p.read_text(encoding='utf-8')
  n = text.count('normalise(')
  if n:
    p.write_text(text.replace('normalise(', 'Normalize('), encoding='utf-8')
  print(n, name)
