"""X-TOOLS: compare two gate reports (check_design.py and Tools/DesignCheck/CheckDesign.py) gate by gate.

Usage: python3 -I X-TOOLS_compare_gates.py <run report.md> <retained report.md>

Compares, per gate: result, failure count, summary line and the set of failure lines. Gate 1 and 2 failure lines
are compared without their class label, because the two tools name the classes differently. Prints one line per
gate and exits 1 when anything differs.
"""
import re
import sys


def parse(path):
  table, fails, gate = {}, {}, None
  for line in open(path, encoding='utf-8'):
    line = line.rstrip('\n')
    m = re.match(r'^\| (\d+) [^|]+\| (PASS|FAIL|SKIP) \| (\d+) \| (.*) \|$', line)
    if m:
      table[int(m.group(1))] = (m.group(2), int(m.group(3)), m.group(4))
      continue
    m = re.match(r'^## Gate (\d+) ', line)
    if m:
      gate = int(m.group(1))
      fails[gate] = set()
      continue
    if gate is not None and line.startswith('- '):
      item = line[2:]
      if gate in (1, 2):
        item = re.sub(r' \[[^\]]+\] ', ' ', item, count=1)
      fails[gate].add(item)
  return table, fails


def main():
  a_table, a_fails = parse(sys.argv[1])
  b_table, b_fails = parse(sys.argv[2])
  differs = False
  for g in range(1, 12):
    a, b = a_table.get(g), b_table.get(g)
    fa, fb = a_fails.get(g, set()), b_fails.get(g, set())
    same = a == b and fa == fb
    differs |= not same
    print(f'gate {g:2}: {"equal" if same else "DIFFERENT"}  run={a}  retained={b}')
    if not same:
      for x in sorted(fa - fb)[:10]:
        print(f'    only run:      {x}')
      for x in sorted(fb - fa)[:10]:
        print(f'    only retained: {x}')
  return 1 if differs else 0


if __name__ == '__main__':
  sys.exit(main())
