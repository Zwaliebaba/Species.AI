import re, sys
# usage: R1b_loc.py file line [file line ...]  -> nearest preceding requirement id and heading
args = sys.argv[1:]
rid = re.compile(r'\*\*((?:R-[A-Z]+|[A-Z]{2,4})-\d{3})\*\*')
for k in range(0, len(args), 2):
    path, ln = args[k], int(args[k + 1])
    lines = open(path, encoding='utf-8').read().split('\n')
    req = head = None
    for i in range(ln - 1, -1, -1):
        if req is None:
            m = rid.search(lines[i])
            if m:
                req = (m.group(1), i + 1)
        if head is None and lines[i].startswith('#'):
            head = (lines[i].strip(), i + 1)
        if req and head:
            break
    print(f"{path}:{ln} -> req {req} | heading {head}")
