"""List requirement ids in component/reference docs whose text uses terms that the AGENTS.md-aligned ADRs exclude.
Args: design_root"""
import os, re, sys, collections
root = sys.argv[1]
terms = {
 'cmake/presets/ninja': r'\bCMake\b|CMakePresets|\bpreset[s]?\b|\bNinja\b|cmake_',
 'vcpkg': r'\bvcpkg\b|overlay triplet|builtin-baseline',
 'flavours': r'`Development`|`Profile`|`Shipping`|\bDevelopment builds?\b|\bProfile builds?\b|\bShipping builds?\b|\bshipping builds?\b|\bprofile builds?\b|Development and Profile|Debug and Development',
 'tools product/SPECIES_PRODUCT': r'SPECIES_PRODUCT|`tools` product|tools product',
 'test framework': r'GoogleTest|GoogleMock|\bgtest\b|\bCTest\b|ctest|Catch2|doctest',
 'zstd': r'Zstandard|\bzstd\b|ZSTD',
 'opus': r'\bOpus\b|libopus',
 'dxc redistributable': r'DXC v1\.|v1\.9\.2609|dxcompiler|redistributable DXC|DirectX Shader Compiler',
 'shaders in packs': r'shader blob|DXIL blob|cooked shader|shader pack|\.cso\b|compiled shaders? (?:in|into) (?:the )?pack',
 'clang-cl/analysis builds': r'clang-cl|analysis build|/analyze',
 'SPECIES_ macros': r'SPECIES_[A-Z_]+',
 'import std/modules': r'import std|SPECIES_IMPORT_STD|\.ixx\b',
 'non-Windows server compilers': r'libc\+\+|libstdc\+\+|\bGCC\b|Clang 23',
 'cxx23 switch': r'/std:c\+\+23|c\+\+23preview',
 'third-party libs': r'DirectX-Headers|d3dx12|libcurl|Sentry|Crashpad|DirectXTK',
 'loose files dev': r'loose[- ]file[s]? .*Development|Development.*loose',
}
pat = {k: re.compile(v) for k, v in terms.items()}
out = collections.defaultdict(lambda: collections.defaultdict(set))
for dp, dn, fns in os.walk(root):
    if '/ADR' in dp or dp.endswith('/ADR'):
        continue
    for fn in fns:
        if not fn.endswith('.md'):
            continue
        path = os.path.join(dp, fn)
        rel = os.path.relpath(path, root)
        if not (rel.startswith('architecture') or rel.startswith('reference') or rel.startswith('assets') or rel.startswith('decisions') or rel == 'CONVENTIONS.md' or rel.startswith('glossary')):
            continue
        cur = '(no id)'
        for line in open(path, encoding='utf-8'):
            m = re.match(r'\*\*((?:R-[A-Z]+|[A-Z]{2,4})-\d{3})\*\*', line)
            if m:
                cur = m.group(1)
            elif line.startswith('#'):
                cur = '§' + line.strip('# \n')[:40]
            elif line.startswith('### DEC-'):
                cur = line[4:11]
            for k, p in pat.items():
                if p.search(line):
                    out[rel][k].add(cur)
for rel in sorted(out):
    print(rel)
    for k in sorted(out[rel]):
        ids = sorted(out[rel][k])
        print(f'   {k}: {", ".join(ids[:40])}{" (+%d)" % (len(ids)-40) if len(ids) > 40 else ""}')
