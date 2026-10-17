# Inventory of preference keys referenced via g_prefsManager in live code.
# Usage: python3 -I P14_pref_keys.py <SRC_ROOT> <manifest.csv> <out.csv>
import sys, os, re, csv
src, manifest, out = sys.argv[1], sys.argv[2], sys.argv[3]
live = set()
for r in csv.DictReader(open(manifest)):
    if r['vcproj_build'] and r['path'].startswith(('code/', 'contrib/')) and r['path'].endswith(('.cpp', '.h')):
        live.add(r['path'])
# include headers even if not in vcproj (macro definitions)
defines = {}
aliases = {}
files = []
for root, _, fs in os.walk(os.path.join(src, 'code')):
    for f in fs:
        if f.endswith(('.cpp', '.h', '.inc')):
            p = os.path.relpath(os.path.join(root, f), src).replace(os.sep, '/')
            files.append(p)
for p in files:
    try:
        txt = open(os.path.join(src, p), encoding='latin-1').read()
    except Exception:
        continue
    for m in re.finditer(r'^\s*#\s*define\s+([A-Z_0-9]+)\s+"([^"]*)"', txt, re.M):
        defines.setdefault(m.group(1), m.group(2))
    for m in re.finditer(r'^\s*#\s*define\s+([A-Z_0-9]+)\s+([A-Z_][A-Z_0-9]+)\s*$', txt, re.M):
        aliases.setdefault(m.group(1), m.group(2))
for _ in range(5):
    for a, b in list(aliases.items()):
        if b in defines and a not in defines:
            defines[a] = defines[b]
call = re.compile(r'g_prefsManager\s*->\s*(GetInt|GetFloat|GetString|GetUnicodeString|SetInt|SetFloat|SetString|DoesKeyExist|AddLine)\s*\(\s*([^,\)]+?)\s*(?:,\s*([^;]*?))?\)\s*[;\)\?:=!<>&|,]')
rows = {}
for p in files:
    if p not in live and not p.endswith('.h'):
        continue
    txt = open(os.path.join(src, p), encoding='latin-1').read()
    for ln, line in enumerate(txt.splitlines(), 1):
        if line.strip().startswith('//'):
            continue
        for m in call.finditer(line):
            fn, keyexpr, dflt = m.group(1), m.group(2).strip(), (m.group(3) or '').strip()
            if keyexpr.startswith('"') and keyexpr.endswith('"'):
                key = keyexpr.strip('"')
            elif keyexpr in defines:
                key = defines[keyexpr] + ' (' + keyexpr + ')'
            elif re.match(r'[A-Z_0-9]+\s+"', keyexpr):
                mm = re.match(r'([A-Z_0-9]+)\s+"([^"]*)"', keyexpr)
                key = (defines.get(mm.group(1), mm.group(1)) + mm.group(2))
            else:
                key = '<expr:' + keyexpr + '>'
            rows.setdefault(key, []).append((fn, dflt, '%s:L%d' % (p, ln)))
with open(out, 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['key', 'accessors', 'defaults_seen', 'num_sites', 'example_sites'])
    for k in sorted(rows, key=str.lower):
        v = rows[k]
        acc = ';'.join(sorted(set(x[0] for x in v)))
        d = ';'.join(sorted(set(x[1] for x in v if x[1] and x[0].startswith('Get'))))
        w.writerow([k, acc, d, len(v), ' '.join(x[2] for x in v[:4])])
print(len(rows), 'keys')
