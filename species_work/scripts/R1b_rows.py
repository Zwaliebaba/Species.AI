import csv, sys
path, start, end = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
with open(path, newline='', encoding='utf-8') as f:
    rows = list(csv.DictReader(f))
for i in range(start, end + 1):
    r = rows[i - 1]
    print(f"### {i} | {r['orig_file']}:{r['line']} | -> {r['routed_target']}")
    print(r['text'])
    print()
