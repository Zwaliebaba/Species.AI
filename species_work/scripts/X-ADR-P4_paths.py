"""Check ADR files for backticked design paths and relative links that do not exist.
Args: design_root adr_dir"""
import re, sys, os
root, adr = sys.argv[1], sys.argv[2]
for fn in sorted(os.listdir(adr)):
    if not fn.endswith('.md'):
        continue
    t = open(os.path.join(adr, fn), encoding='utf-8').read()
    for m in re.finditer(r'`((?:reference|architecture|decisions|assets|glossary|adr)/[^`\s]*)`', t):
        p = m.group(1).split('#')[0]
        if not os.path.exists(os.path.join(root, p)):
            print(f'{fn}: missing path {p}')
    for m in re.finditer(r'\]\(([^)\s]+)\)', t):
        link = m.group(1)
        if link.startswith('http') or link.startswith('#'):
            continue
        p = link.split('#')[0]
        if not os.path.exists(os.path.normpath(os.path.join(adr, p))):
            print(f'{fn}: broken link {link}')
    for m in re.finditer(r'`(ui|map)\.[a-z_]+`', t):
        print(f'{fn}: old asset stem? {m.group(0)}')
