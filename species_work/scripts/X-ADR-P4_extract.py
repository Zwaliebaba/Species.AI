"""Extract the 'ADR conflicts' sections of Phase 3 reports. Args: out_file report_files..."""
import re, sys
out = open(sys.argv[1], 'w', encoding='utf-8')
for path in sys.argv[2:]:
    lines = open(path, encoding='utf-8').read().splitlines()
    i = 0
    while i < len(lines):
        m = re.match(r'^(#+)\s+(.*)', lines[i])
        if m and re.search(r'ADR|ADR-0063', m.group(2)) and not m.group(2).startswith(path.split('/')[-1][:-3] + ' report'):
            lvl = len(m.group(1))
            out.write(f'\n===== {path.split("/")[-1]} : {lines[i]}\n')
            j = i + 1
            while j < len(lines):
                m2 = re.match(r'^(#+)\s', lines[j])
                if m2 and len(m2.group(1)) <= lvl:
                    break
                out.write(lines[j] + '\n')
                j += 1
            i = j
        else:
            i += 1
