"""List backticked code-like tokens in ADR files, flagging AGENTS.md naming problems.
Args: adr_dir"""
import re, sys, os, collections
adr = sys.argv[1]
brit = re.compile(r'(?i)colour|behaviour|initialis|serialis|normalis|quantis|synchronis|neighbour|centre|grey|cancelled|optimis|favour|harbour|flavour|analys|recognis|organis|licen[cs]e')
seen = collections.defaultdict(set)
for fn in sorted(os.listdir(adr)):
    if not fn.endswith('.md'):
        continue
    t = open(os.path.join(adr, fn), encoding='utf-8').read()
    for tok in re.findall(r'`([^`\n]+)`', t):
        flags = []
        if re.search(r'\bSPECIES_|\bspecies::|\bSpecies::', tok): flags.append('ns/macro')
        if re.search(r'\b[a-z]+_[a-z_]+\.(h|cpp|hpp)\b', tok) or re.search(r'\.(hpp|inl|cc|ixx)\b', tok): flags.append('file')
        if re.search(r'\b[A-Z][A-Za-z0-9]*::[a-z][a-z0-9_]*\b', tok) and not re.search(r'std::|Microsoft::|WRL', tok): flags.append('lower-member')
        if re.search(r'\b(I|C)[A-Z][a-z]+[A-Za-z]*\b', tok) and not re.search(r'\b(ID3D|IDXGI|IDWrite|IXAudio2|IX3D|IMF|IUnknown|IGameInput|IWIC)', tok): flags.append('affix?')
        if re.search(r'(Base|Impl)\b', tok): flags.append('affix?')
        if re.search(r'\bk[A-Z]\w+', tok): flags.append('kConst')
        if re.search(r'[A-Za-z]', tok) and brit.search(tok) and re.search(r'[A-Z][a-z]|::|\(|_[a-z]', tok): flags.append('british?')
        if flags:
            seen[(tok, ','.join(flags))].add(fn[:7])
for (tok, f), files in sorted(seen.items(), key=lambda x: x[0][1]):
    print(f'{f:14} {tok!r:60} {" ".join(sorted(files))}')
