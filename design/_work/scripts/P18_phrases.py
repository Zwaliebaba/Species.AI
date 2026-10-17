import re, sys, os, glob
src_dir, lang_file = sys.argv[1], sys.argv[2]
phr = {}
for line in open(lang_file, encoding='utf-8', errors='replace'):
    m = re.match(r'^(\S+)\s', line)
    if m: phr[m.group(1)] = True
ids = {}
pat = re.compile(r'(?:LANGUAGEPHRASE|ISLANGUAGEPHRASE)\(\s*"([^"]+)"\s*\)')
pat2 = re.compile(r'(?:TextButton|WaitingButton|AddOption|SetShortProperties|SetProperties|GameMenuButton|BackPageButton)\(\s*"([a-z][a-z0-9_]+)"')
for f in sorted(glob.glob(os.path.join(src_dir, '*'))):
    txt = open(f, encoding='latin-1').read()
    for i, line in enumerate(txt.splitlines(), 1):
        s = line.strip()
        commented = s.startswith('//') or s.startswith('/*') or s.startswith('*')
        for m in pat.finditer(line):
            ids.setdefault(m.group(1), []).append((os.path.basename(f), i, commented))
        for m in pat2.finditer(line):
            ids.setdefault(m.group(1), []).append((os.path.basename(f), i, commented))
for k in sorted(ids):
    locs = ids[k]
    files = sorted(set(l[0] for l in locs))
    allc = all(l[2] for l in locs)
    print(f"{k}\t{'Y' if k in phr else 'N'}\t{'commented' if allc else ''}\t{';'.join(files)}")
