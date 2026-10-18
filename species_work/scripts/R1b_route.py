import csv, sys
sample, routing, start, end = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
rows = list(csv.DictReader(open(sample, newline='', encoding='utf-8')))
routes = list(csv.DictReader(open(routing, newline='', encoding='utf-8')))
for i in range(start, end + 1):
    r = rows[i - 1]
    f, ln = r['orig_file'], int(r['line'])
    hits = [x for x in routes if x['orig_file'] == f and int(x['start_line']) <= ln <= int(x['end_line'])]
    for h in hits:
        print(f"{i}\t{f}:{ln}\t{h['anchor']}\t{h['action']}\t{h['target_file']}\t{h['target_section']}\t{h['note'][:220]}")
    if not hits:
        print(f"{i}\t{f}:{ln}\tNO-ROUTE")
