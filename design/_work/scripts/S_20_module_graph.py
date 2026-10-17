#!/usr/bin/env python3
"""S_20_module_graph.py — module (directory) include-dependency graph for 20_architecture.

Usage:  python3 -I S_20_module_graph.py <SRC multiwinia root> <manifest.csv> <out.md>

Method (approximation, see 20 §3):
  * Seed set = every .cpp/.c file under code/, contrib/systemiv/lib, contrib/eclipse whose
    manifest row has a non-empty vcproj_build (i.e. compiled into Darwinia.vcproj), plus the
    eclipse library sources (separate sub-project, always linked).
  * #include "x" / <x> lines are resolved like MSVC: quoted includes first relative to the
    including file's directory, then the Common.vsprops AdditionalIncludeDirectories in order
    (contrib/libogg-1.1.3/include, contrib/libvorbis-1.1.2/include, code, contrib, contrib/unrar,
    targets/vs2005, contrib/systemiv, contrib/systemiv/lib/netlib, contrib/eclipse,
    contrib/lpng1215, contrib/zlib). Unresolved includes are OS/SDK headers and are bucketed by
    name into pseudo-modules (OpenGL, DirectX, Win32, CRT/STL).
  * Headers reached transitively from the seed set are added to the file set.
  * Conditional compilation is NOT evaluated: includes inside dead #ifdef blocks are counted too.
  * Edge weight A->B = number of distinct (including file in A, included file in B) pairs, A != B.
Output: a Markdown table + a Mermaid flowchart with the strongest edges.
"""
import csv
import os
import re
import sys
from collections import defaultdict

INC_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')


def module_of(rel):
    p = rel.replace('\\', '/')
    parts = p.split('/')
    if parts[0] == 'code':
        if len(parts) == 2:
            return 'code (top level)'
        if parts[1] == 'lib':
            if len(parts) >= 4 and parts[2] == 'input':
                return 'code/lib/input'
            return 'code/lib'
        return 'code/' + parts[1]
    if parts[0] == 'contrib':
        if len(parts) > 1 and parts[1].lower() == 'systemiv':
            if len(parts) >= 4 and parts[2] == 'lib':
                if len(parts) == 4:
                    return 'contrib/systemiv/lib (root files)'
                return 'contrib/systemiv/lib/' + parts[3]
            return 'contrib/systemiv (other)'
        if len(parts) > 1:
            name = parts[1]
            if name.startswith('libogg'):
                return 'third-party: libogg'
            if name.startswith('libvorbis'):
                return 'third-party: libvorbis'
            if name in ('lpng1215', 'zlib'):
                return 'third-party: libpng/zlib'
            if name == 'unrar':
                return 'contrib/unrar'
            if name == 'eclipse':
                return 'contrib/eclipse'
            return 'contrib/' + name
    if parts[0] == 'targets':
        return 'targets/vs2005 (resources)'
    return parts[0]


def sys_bucket(name):
    n = name.lower()
    if n.startswith('gl/') or n in ('glext.h',) or 'opengl' in n:
        return 'OS: OpenGL/GLU'
    if n.startswith('d3d') or n.startswith('dx') or n in ('dsound.h', 'dinput.h', 'xinput.h', 'dxerr9.h', 'dxdiag.h'):
        return 'OS: DirectX (D3D9/DSound/XInput)'
    if n in ('windows.h', 'winsock.h', 'winsock2.h', 'shlobj.h', 'mmsystem.h', 'shellapi.h', 'wpc.h', 'tchar.h',
             'vfw.h', 'dbghelp.h', 'crtdbg.h', 'direct.h', 'io.h', 'objbase.h', 'process.h', 'commdlg.h',
             'mmreg.h', 'msacm.h', 'winbase.h', 'wininet.h', 'ws2tcpip.h', 'iphlpapi.h', 'rpc.h', 'shlwapi.h',
             'ole2.h', 'initguid.h', 'wbemidl.h', 'comdef.h', 'atlbase.h', 'sys/stat.h', 'sys/types.h',
             'sys/timeb.h', 'conio.h', 'malloc.h', 'float.h', 'time.h', 'winuser.h', 'winerror.h'):
        return 'OS: Win32 / CRT'
    if n.startswith('sdl') or n.startswith('carbon') or n.startswith('coreservices') or n.startswith('cocoa') \
            or n.startswith('mach') or n.startswith('unistd') or n.startswith('pthread') or n.startswith('sys/') \
            or n.startswith('netinet') or n.startswith('arpa') or n.startswith('openal') or n.startswith('al/') \
            or n.startswith('applicationservices') or n.startswith('iokit'):
        return 'OS: SDL/POSIX/Mac (non-baseline)'
    if n.startswith('steam'):
        return 'third-party: Steam SDK (absent)'
    return 'C/C++ standard library'


def main():
    src, manifest, out = sys.argv[1], sys.argv[2], sys.argv[3]
    rows = list(csv.DictReader(open(manifest, newline='', encoding='utf-8')))
    compiled = set()
    for r in rows:
        p = r['path']
        if not (p.startswith('code/') or p.startswith('contrib/systemiv/lib') or p.startswith('contrib/eclipse')):
            continue
        if not p.endswith(('.cpp', '.c')):
            continue
        if p.startswith('contrib/eclipse') or r['vcproj_build']:
            compiled.add(p)
    # case-insensitive file index (Windows semantics)
    index = {}
    for dp, dn, fn in os.walk(src):
        for f in fn:
            full = os.path.join(dp, f)
            rel = os.path.relpath(full, src).replace('\\', '/')
            index[rel.lower()] = rel
    inc_dirs = ['contrib/libogg-1.1.3/include', 'contrib/libvorbis-1.1.2/include', 'code', 'contrib',
                'contrib/unrar', 'targets/vs2005', 'contrib/systemiv', 'contrib/systemiv/lib/netlib',
                'contrib/eclipse', 'contrib/lpng1215', 'contrib/zlib']

    def resolve(cur, name, quoted):
        name = name.replace('\\', '/')
        cands = []
        if quoted:
            cands.append(os.path.normpath(os.path.join(os.path.dirname(cur), name)).replace('\\', '/'))
        for d in inc_dirs:
            cands.append(os.path.normpath(os.path.join(d, name)).replace('\\', '/'))
        for c in cands:
            if c.lower() in index:
                return index[c.lower()]
        return None

    edges = defaultdict(set)
    files = set(compiled)
    todo = list(compiled)
    seen = set()
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.add(f)
        try:
            text = open(os.path.join(src, f), encoding='latin-1').read().splitlines()
        except OSError:
            continue
        for line in text:
            m = INC_RE.match(line)
            if not m:
                continue
            quoted = m.group(1) == '"'
            tgt = resolve(f, m.group(2), quoted)
            a = module_of(f)
            if tgt is None:
                b = sys_bucket(m.group(2))
                edges[(a, b)].add((f, m.group(2)))
                continue
            b = module_of(tgt)
            if a != b:
                edges[(a, b)].add((f, tgt))
            if tgt not in seen:
                files.add(tgt)
                todo.append(tgt)
    mod_files = defaultdict(int)
    for f in seen:
        mod_files[module_of(f)] += 1
    lines = []
    lines.append('| From module | To module | Include pairs |')
    lines.append('|---|---|---|')
    for (a, b), s in sorted(edges.items(), key=lambda kv: (kv[0][0], -len(kv[1]), kv[0][1])):
        lines.append('| %s | %s | %d |' % (a, b, len(s)))
    lines.append('')
    lines.append('| Module | Files in compiled closure |')
    lines.append('|---|---|')
    for m in sorted(mod_files):
        lines.append('| %s | %d |' % (m, mod_files[m]))
    with open(out, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(lines) + '\n')
    print('files in closure:', len(seen), 'edges:', len(edges))


if __name__ == '__main__':
    main()
