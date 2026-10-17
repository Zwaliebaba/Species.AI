#!/usr/bin/env python3
"""Phase 1 manifest: every file under code/, contrib/systemiv/, data/, tools/, targets/ (+ docs/ and root files as supporting).
Usage: python3 -I manifest.py <MULTIWINIA_ROOT> <OUT_CSV>"""
import csv, os, sys, re
from xml.etree import ElementTree as ET

root, out = sys.argv[1], sys.argv[2]
SRC_EXT = {'.cpp', '.h', '.c', '.cc', '.hpp', '.mm', '.m', '.inc', '.S', '.asm'}
SHADER_EXT = {'.vs', '.ps', '.fx', '.vsh', '.psh'}
TYPE_BY_EXT = {
    '.bmp': 'image/bmp', '.png': 'image/png', '.jpg': 'image/jpeg', '.gif': 'image/gif', '.ico': 'image/ico',
    '.shp': 'mesh/shp', '.max': 'mesh/3dsmax-source', '.anim': 'anim/text', '.ogg': 'audio/ogg', '.wav': 'audio/wav',
    '.txt': 'text', '.html': 'text/html', '.ini': 'text/ini', '.list': 'text/list', '.iss': 'installer/innosetup',
    '.bat': 'script/batch', '.sh': 'script/sh', '.pl': 'script/perl', '.mk': 'script/make', '.exe': 'binary/exe',
    '.dll': 'binary/dll', '.lib': 'binary/lib', '.dat': 'archive/dat', '.vcproj': 'build/vcproj', '.sln': 'build/sln',
    '.vsprops': 'build/vsprops', '.rc': 'build/rc', '.pdf': 'doc/pdf', '.doc': 'doc/word', '.docx': 'doc/word',
    '.rtf': 'doc/rtf', '.mo': 'i18n/mo', '.po': 'i18n/po', '.zip': 'archive/zip', '.cab': 'archive/cab',
    '.xml': 'text/xml', '.grd': 'binary/photoshop-gradient', '.nsi': 'installer/nsis', '.pbxproj': 'build/xcode',
}
SCOPED = ['code', 'contrib/systemiv', 'contrib/eclipse', 'data', 'tools', 'targets', 'docs']
# Other sub-projects of targets/vs2005/Darwinia.sln (linked into the exe): third-party libraries, analysed only at API level.
SLN_LINKED = ['contrib/unrar', 'contrib/libogg-1.1.3', 'contrib/libvorbis-1.1.2', 'contrib/lpng1215', 'contrib/zlib']

def classify(rel):
    ext = os.path.splitext(rel)[1]
    if ext in SRC_EXT: return 'source/' + ext.lstrip('.')
    if ext in SHADER_EXT: return 'shader/' + ext.lstrip('.')
    return TYPE_BY_EXT.get(ext.lower(), 'other' + (ext and '/' + ext.lstrip('.').lower()))

def area(rel):
    for s in SCOPED:
        if rel == s or rel.startswith(s + '/'):
            return s
    for s in SLN_LINKED:
        if rel.startswith(s + '/'): return 'contrib/thirdparty-linked'
    if rel.startswith('contrib/'): return 'contrib/thirdparty-unlinked'
    return 'root' if '/' not in rel else rel.split('/')[0]

# vcproj membership (live in the Win32 project), with per-config exclusion
vc = os.path.join(root, 'targets/vs2005/Darwinia.vcproj')
vcmap = {}
tree = ET.parse(vc)
for f in tree.iter('File'):
    p = f.get('RelativePath').replace('\\', '/')
    norm = os.path.normpath(os.path.join('targets/vs2005', p)).replace('\\', '/')
    excl = []
    for fc in f.findall('FileConfiguration'):
        if fc.get('ExcludedFromBuild', 'false').lower() == 'true':
            excl.append(fc.get('Name').split('|')[0])
    vcmap[norm.lower()] = ';'.join(excl) if excl else 'all'

rows = []
for dp, dns, fns in os.walk(root):
    dns[:] = [d for d in dns if d != '.git']
    for fn in fns:
        full = os.path.join(dp, fn)
        rel = os.path.relpath(full, root).replace(os.sep, '/')
        a = area(rel)
        t = classify(rel)
        size = os.path.getsize(full)
        loc = nb = ''
        if t.startswith('source/') or t.startswith('shader/'):
            with open(full, 'rb') as fh:
                lines = fh.read().decode('latin-1').splitlines()
            loc = len(lines); nb = sum(1 for l in lines if l.strip())
        inproj = vcmap.get(rel.lower(), '')
        if not inproj and a == 'contrib/eclipse' and t.startswith('source/'): inproj = 'all(eclipse.vcproj)'
        rows.append([rel, a, t, size, loc, nb, inproj])

rows.sort()
# Files not listed in the project but #included (by basename) from a project file are compiled when the include is
# reached; record the including files so the build matrix and coverage check can treat them as conditionally live.
live_set = [r[0] for r in rows if r[2].startswith('source/') and r[6] and r[6] != 'Debug;Release;DebugDirect3D;ReleaseDirect3D']
inc_by = {}
for lp in live_set:
    with open(os.path.join(root, lp), 'rb') as fh:
        txt = fh.read().decode('latin-1')
    for m in re.finditer(r'#\s*include\s+"([^"]+)"', txt):
        inc_by.setdefault(os.path.basename(m.group(1)).lower(), set()).add(lp)
for r in rows:
    r.append(';'.join(sorted(inc_by.get(os.path.basename(r[0]).lower(), set()))[:5]) if (r[2].startswith('source/') and not r[6] and r[1] == 'code') else '')
with open(out, 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['path', 'area', 'type', 'bytes', 'lines', 'nonblank_lines', 'vcproj_build', 'included_by_live'])
    w.writerows(rows)
missing = [k for k in vcmap if not os.path.exists(os.path.join(root, k)) and not any(r[0].lower() == k for r in rows)]
print('rows', len(rows), 'vcproj entries', len(vcmap), 'vcproj entries not on disk', len(missing))
for m in sorted(missing): print('  MISSING', m)
