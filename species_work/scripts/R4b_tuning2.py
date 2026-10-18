"""R4b: refine the value-citation check of R4b_tuning.py.

Usage: python3 -I R4b_tuning2.py <out dir>
Reads cites.json, keys.json; writes cites_bad2.txt.
"""
import json
import os
import re
import sys

out = sys.argv[1]
keys = json.load(open(os.path.join(out, 'keys.json')))
hits = json.load(open(os.path.join(out, 'cites.json')))

ID = re.compile(r'\b(?:R-[A-Z]+|[A-Z]{2,4}|DEC|ADR)-\d{3}\b')
SUP = str.maketrans('⁰¹²³⁴⁵⁶⁷⁸⁹', '0123456789')


def nums(s):
  s = ID.sub(' ', s)
  s = re.sub(r'\[[^\]]*\]\([^)]*\)', ' ', s)
  s = s.replace('−', '-').replace(' ', ' ').replace(' ', ' ')
  # powers like 2²⁴ and 10⁹
  out_ = []
  for m in re.finditer(r'(\d+)([⁰¹²³⁴⁵⁶⁷⁸⁹]+)', s):
    out_.append(float(int(m.group(1)) ** int(m.group(2).translate(SUP))))
  s = re.sub(r'(\d+)([⁰¹²³⁴⁵⁶⁷⁸⁹]+)', ' ', s)
  # thousands separators: digit groups separated by single space or comma
  s = re.sub(r'(?<=\d)[ ,](?=\d{3}\b)', '', s)
  for m in re.finditer(r'(?<![\w.])-?(?:0x[0-9A-Fa-f]+|\d+(?:\.\d+)?)(?:\s*(?:/|÷)\s*(\d+(?:\.\d+)?))?', s):
    x = m.group(0)
    try:
      if x.lower().startswith('0x') or x.lower().startswith('-0x'):
        out_.append(float(int(x, 16)))
        continue
      if m.group(1):
        a = re.split(r'\s*(?:/|÷)\s*', x)[0]
        out_.append(float(a) / float(m.group(1)))
        out_.append(float(a))
      else:
        out_.append(float(x))
    except Exception:
      pass
  return out_


def match(cn, vn):
  for a in cn:
    for b in vn:
      for f in (1, 10, 0.1, 1000, 0.001, 100, 0.01):
        if abs(a * f - b) < 1e-6 * max(1, abs(b)):
          return True
  return False


bad = []
for h in hits:
  c = h['cited']
  if c.startswith('[') or not nums(c):
    continue
  if match(nums(c), nums(h['value'])):
    continue
  bad.append(h)
with open(os.path.join(out, 'cites_bad2.txt'), 'w') as fh:
  for h in bad:
    fh.write(f"{h['file']}:{h['line']} {h['key']} cited [{h['cited']}] ({h['side']}) table [{h['value']} | {h['unit']}]\n   {h['text'][:260]}\n")
print(len(bad))
