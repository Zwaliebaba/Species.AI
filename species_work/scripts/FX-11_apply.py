"""FX-11 copy of X-ADR-P4_apply.py: apply exact text replacements, all or nothing per file. Args: spec.txt
Spec format, repeated:
@@@FILE <path>
@@@OLD
<old text lines>
@@@NEW
<new text lines>
@@@END
Each old text must occur exactly once in the file (after earlier edits of that file).
The trailing newline before @@@NEW / @@@END is not part of the text."""
import sys, collections
lines = open(sys.argv[1], encoding='utf-8').read().split('\n')
edits = []
i = 0
cur = None
while i < len(lines):
    l = lines[i]
    if l.startswith('@@@FILE '):
        cur = {'file': l[8:].strip()}
        i += 1
    elif l == '@@@OLD':
        j = i + 1
        while lines[j] != '@@@NEW':
            j += 1
        cur['old'] = '\n'.join(lines[i+1:j])
        k = j + 1
        while lines[k] != '@@@END':
            k += 1
        cur['new'] = '\n'.join(lines[j+1:k])
        edits.append(dict(cur))
        i = k + 1
    else:
        i += 1
by_file = collections.OrderedDict()
for e in edits:
    by_file.setdefault(e['file'], []).append(e)
bad = False
for f, es in by_file.items():
    text = open(f, encoding='utf-8').read()
    ok = True
    for n_, e in enumerate(es):
        n = text.count(e['old'])
        if n != 1:
            print(f'FAIL {f} edit {n_}: old text occurs {n} times: {e["old"][:100]!r}')
            bad = ok = True
            ok = False
            break
        text = text.replace(e['old'], e['new'])
    if ok:
        open(f, 'w', encoding='utf-8').write(text)
        print(f'OK {f}: {len(es)} edits, {len(text.encode("utf-8"))} bytes')
sys.exit(1 if bad else 0)
