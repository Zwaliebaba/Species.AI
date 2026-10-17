"""RES-2 helper: list declarations in C++ sketch blocks of component docs, with namespace.

Usage: python3 -I RES-2_inventory.py <file-or-folder> [...]
Prints: file:line  namespace  kind  name
"""
import os
import re
import sys

DECL = re.compile(r'^\s*(?:template\s*<[^>]*>\s*)?(?:export\s+)?(struct|class|enum\s+class|enum|union|concept|using)\s+([A-Za-z_]\w*)')
FUNC = re.compile(r'^\s*(?:\[\[[^\]]*\]\]\s*)*(?:inline\s+|constexpr\s+|static\s+|extern\s+|consteval\s+)*'
                  r'(?:[\w:<>,\s\*&]+?)\s+([A-Z]\w*)\s*\(')
CONST = re.compile(r'^\s*(?:inline\s+)?(?:constexpr|const)\s+[\w:<>]+\s+([A-Z][A-Z0-9_]+)\s*=')
NS_OPEN = re.compile(r'^\s*namespace\s+([A-Za-z_][\w:]*)\s*(\{)?\s*$')
NS_CLOSE = re.compile(r'^\s*\}\s*//\s*namespace\s+([A-Za-z_][\w:]*)')
MACRO = re.compile(r'^\s*#\s*define\s+([A-Z_][A-Z0-9_]*)')


def files(args):
    for a in args:
        if os.path.isdir(a):
            for root, _, names in os.walk(a):
                for n in sorted(names):
                    if n.endswith('.md'):
                        yield os.path.join(root, n)
        else:
            yield a


def scan(path):
    out = []
    with open(path, encoding='utf-8') as f:
        lines = f.read().split('\n')
    in_code = False
    ns = []
    depth = 0  # brace depth inside the namespace (class bodies)
    for i, line in enumerate(lines, 1):
        s = line.strip()
        if s.startswith('```'):
            if not in_code:
                in_code = s[3:].strip().lower() in ('cpp', 'c++', 'hlsl', 'cxx', 'h')
                ns = []
                depth = 0
            else:
                in_code = False
            continue
        if not in_code:
            continue
        m = NS_CLOSE.match(line)
        if m:
            if ns:
                ns.pop()
            depth = 0
            continue
        m = NS_OPEN.match(line)
        if m:
            ns.append(m.group(1))
            continue
        cur = '::'.join(ns) or '-'
        top = depth == 0
        m = DECL.match(line)
        if m and top:
            out.append((i, cur, m.group(1), m.group(2)))
        else:
            m2 = MACRO.match(line)
            if m2:
                out.append((i, cur, 'macro', m2.group(1)))
            elif top:
                m3 = CONST.match(line)
                if m3:
                    out.append((i, cur, 'const', m3.group(1)))
                else:
                    m4 = FUNC.match(line)
                    if m4 and not s.startswith('//') and not s.startswith('return'):
                        out.append((i, cur, 'func', m4.group(1)))
        # brace depth tracking (ignore namespace braces)
        code = line.split('//')[0]
        if NS_OPEN.match(line) or s == '{' and False:
            pass
        opens = code.count('{')
        closes = code.count('}')
        depth += opens - closes
        if s == '{' and ns and depth == 1 and lines[i - 2].strip().startswith('namespace'):
            depth = 0
        if depth < 0:
            depth = 0
    return out


def main():
    for p in files(sys.argv[1:]):
        for i, ns, kind, name in scan(p):
            print(f'{p}:{i}\t{ns}\t{kind}\t{name}')


if __name__ == '__main__':
    main()
