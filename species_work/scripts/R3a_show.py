"""R3a helper: show requirements with the tuning rows, requirements and DECs they cite.

Usage: python3 -I R3a_show.py <lib.json> show <id> [<id> ...]
       python3 -I R3a_show.py <lib.json> cand <file> [n]   (candidate requirements of a file, scored)
       python3 -I R3a_show.py <lib.json> key <key> [...]
       python3 -I R3a_show.py <lib.json> dec <DEC-NNN> [...]
"""
import json
import re
import sys

LIB = json.load(open(sys.argv[1], encoding='utf-8'))
reqs, keys, decs = LIB['reqs'], LIB['keys'], LIB['decs']
mode = sys.argv[2]
KEY_RX = re.compile(r'`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.]+)`')
REQ_RX = re.compile(r'\b((?:R-)?[A-Z]{2,6}-\d{3})\b')


def key_line(k):
  v = keys.get(k)
  if not v:
    return f'    KEY {k}: (not a tuning key)'
  r = v['row']
  return f"    KEY {k} = {r.get('value', '')} [{r.get('unit', '')}] {r.get('range or notes', '')[:120]} | req {r.get('requirement', '')} | {r.get('dec', '')} ({v['file'].split('/')[-1]}:{v['line']})"


def show(rid):
  r = reqs.get(rid)
  if not r:
    print(f'== {rid}: NOT DEFINED')
    return
  print(f"== {rid}  {r['file']}:{r['line']}  [{r['section']}]")
  print(r['text'])
  seen = set()
  for k in KEY_RX.findall(r['text']):
    if k in seen:
      continue
    seen.add(k)
    print(key_line(k))
  for c in sorted(set(REQ_RX.findall(r['text']))):
    if c == rid or c.startswith('DEC') or c.startswith('ADR'):
      continue
    rr = reqs.get(c)
    print(f"    REQ {c}: " + (f"{rr['file'].split('/')[-1]} {rr['text'][len(c)+14:][:110]!r}" if rr else 'UNDEFINED'))
  for d in sorted(set(re.findall(r'\bDEC-\d{3}\b', r['text']))):
    e = decs.get(d)
    print(f"    {d}: " + (f"{e['status']} | {e['title']} | {e['specified'][:200]}" if e else 'UNDEFINED'))
  print()


if mode == 'show':
  for rid in sys.argv[3:]:
    show(rid)
elif mode == 'cand':
  f = sys.argv[3]
  n = int(sys.argv[4]) if len(sys.argv) > 4 else 30
  scored = []
  for rid, r in reqs.items():
    if r['file'] != f or not rid.startswith('R-'):
      continue
    t = r['text']
    s = 0
    s += 2 * len(re.findall(r'\bstate|enters|leaves|until|timer|countdown', t))
    s += len(re.findall(r'tick', t))
    s += 2 * len(re.findall(r'[÷×=<>≤≥]|\bmin\(|\bmax\(|clamp', t))
    s += 2 * len(re.findall(r'reference/\d\d', t))
    s += len(KEY_RX.findall(t))
    scored.append((s, rid, r['section'], t[len(rid) + 14:][:110]))
  scored.sort(reverse=True)
  for s, rid, sec, t in scored[:n]:
    print(f'{s:3d} {rid} [{sec[:40]}] {t!r}')
elif mode == 'key':
  for k in sys.argv[3:]:
    print(key_line(k))
elif mode == 'dec':
  for d in sys.argv[3:]:
    e = decs.get(d)
    print(f"{d} {e['status']} {e['title']} ({e['file']}:{e['line']})\n{e['body']}")
