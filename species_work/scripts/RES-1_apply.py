"""RES-1 helper: apply exact, unique-match replacements from a JSON spec.

Usage: python3 -I RES-1_apply.py <spec.json>
The spec is a list of {"file": path, "old": text, "new": text}. Every old text must
occur exactly once in its file, or nothing is written for that file.
"""
import json
import sys


def main(spec_path):
  with open(spec_path, encoding='utf-8') as f:
    spec = json.load(f)
  by_file = {}
  for item in spec:
    by_file.setdefault(item['file'], []).append(item)
  status = 0
  for path, items in by_file.items():
    with open(path, encoding='utf-8') as f:
      text = f.read()
    ok = True
    for item in items:
      n = text.count(item['old'])
      if n != 1:
        print(f'{path}: {n} matches for: {item["old"][:70]!r}')
        ok = False
        break
      text = text.replace(item['old'], item['new'])
    if ok:
      with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write(text)
      print(f'{path}: {len(items)} replacements')
    else:
      status = 1
  return status


if __name__ == '__main__':
  sys.exit(main(sys.argv[1]))
