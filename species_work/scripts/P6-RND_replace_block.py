"""P6-RND: replace one exact text block in one of P6-RND's own files (an Edit with an exact-match guard).

Usage: python3 -I P6-RND_replace_block.py <file.md> <old block file> <new block file>
The old block must occur exactly once in the file; otherwise nothing is written.
"""
import pathlib
import sys

target, old_path, new_path = (pathlib.Path(p) for p in sys.argv[1:4])
text = target.read_text(encoding='utf-8')
old = old_path.read_text(encoding='utf-8')
new = new_path.read_text(encoding='utf-8')
count = text.count(old)
if count != 1:
  sys.exit(f'old block occurs {count} times in {target}; nothing written')
target.write_text(text.replace(old, new), encoding='utf-8')
print(f'replaced {len(old)} bytes with {len(new)} bytes in {target}')
