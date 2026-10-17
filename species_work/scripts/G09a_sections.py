"""G09a: remap internal section numbers (headings and section references) of one file.
Usage: python3 -I G09a_sections.py <file> <map.tsv> <protect.txt>
map.tsv: old<TAB>new, for example 2.5<TAB>2.3, or 2.28<TAB>P:2.3 for a section of the presentation file.
protect.txt: exact strings (one per line) left untouched."""
import re, sys
path, mpath, ppath = sys.argv[1:4]
text = open(path, encoding='utf-8').read()
mp = dict(l.rstrip('\n').split('\t') for l in open(mpath, encoding='utf-8') if l.strip())
prot = [l.rstrip('\n') for l in open(ppath, encoding='utf-8') if l.strip()]
for i, s in enumerate(prot):
    assert text.count(s) >= 1, s
    text = text.replace(s, '\x00%d\x00' % i)
def ref(m):
    old = m.group(1)
    new = mp.get(old, old)
    if new.startswith('P:'):
        return '[front_end_presentation.md](front_end_presentation.md) §' + new[2:]
    return '§' + new
def head(m):
    old = m.group(1)
    new = mp.get(old, old)
    assert not new.startswith('P:'), old
    return '### ' + new + ' '
text = re.sub(r'(?m)^### (2\.\d+) ', head, text)
text = re.sub(r'§(2\.\d+)\b', ref, text)
for i, s in enumerate(prot):
    text = text.replace('\x00%d\x00' % i, s)
open(path, 'w', encoding='utf-8').write(text)
print('ok')
