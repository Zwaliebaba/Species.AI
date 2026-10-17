"""RES-2 helper: split one `namespace Engine` cpp block of a Markdown file into an Engine block and a Species block.

Usage: python3 -I RES-2_split.py <file.md> <block start line> <species names> <qualify names>
  <block start line>  the 1-based line of `namespace Engine` inside the block
  <species names>     comma-separated names of the top-level declarations that move to Species
  <qualify names>     comma-separated Engine names to qualify as Engine::<name> inside the moved declarations

Top-level declarations are the chunks of the block separated by blank lines; a chunk moves when its first
declaration line declares one of the species names. Comment lines directly above a declaration stay with it.
The Species block follows the Engine block, in the original order of its chunks.
"""
import re
import sys


def main():
    path, start, species, qualify = sys.argv[1], int(sys.argv[2]), sys.argv[3].split(','), sys.argv[4].split(',')
    lines = open(path, encoding='utf-8').read().split('\n')
    i = start - 1
    assert lines[i].strip() == 'namespace Engine', lines[i]
    assert lines[i + 1].strip() == '{'
    j = i + 2
    while not lines[j].startswith('} // namespace Engine'):
        j += 1
    body = lines[i + 2:j]
    chunks, cur, depth = [], [], 0
    for line in body:
        if line.strip() == '' and cur and depth == 0:
            chunks.append(cur)
            cur = []
        elif line.strip() != '' or depth > 0:
            cur.append(line)
            code = line.split('//')[0]
            depth += code.count('{') - code.count('}')
    if cur:
        chunks.append(cur)
    decl = re.compile(r'^(?:template\s*<[^>]*>\s*)?(?:enum class|class|struct|using)\s+(\w+)')
    eng, spe = [], []
    for c in chunks:
        name = None
        for l in c:
            m = decl.match(l)
            if m:
                name = m.group(1)
                break
        (spe if name in species else eng).append(c)
    q = re.compile(r'(?<![\w:])(' + '|'.join(re.escape(n) for n in qualify if n) + r')\b')
    spe = [[q.sub(r'Engine::\1', l) if not l.lstrip().startswith('//') else l for l in c] for c in spe]
    out = ['namespace Engine', '{', '']
    for c in eng:
        out += c + ['']
    out += ['} // namespace Engine', '', 'namespace Species', '{', '']
    for c in spe:
        out += c + ['']
    out += ['} // namespace Species']
    lines[i:j + 1] = out
    open(path, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
    print(f'{len(eng)} chunks stay in Engine, {len(spe)} move to Species')


if __name__ == '__main__':
    main()
