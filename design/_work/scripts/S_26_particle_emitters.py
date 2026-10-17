"""S26: catalogue every ParticleSystem::CreateParticle call site.
Usage: python3 -I S_26_particle_emitters.py <src_root> <file> [<file> ...]   (files relative to src_root)
Prints a markdown table: site | enclosing function | path | particle type | size argument | RNG near call."""
import re, sys, os
root = sys.argv[1]
fnre = re.compile(r'^[A-Za-z_][\w\s\*&<>:,]*?([A-Za-z_]\w*::~?[A-Za-z_]\w*)\s*\(')
typere = re.compile(r'Particle::(Type\w+)')
sync = re.compile(r'\b(syncfrand|syncsfrand|syncrand|FRAND|SFRAND)\s*\(')
unsync = re.compile(r'(?<![\w])(frand|sfrand|AppRandom|rand)\s*\(')
rows = []
for rel in sys.argv[2:]:
    path = os.path.join(root, rel)
    raw = open(path, encoding='latin-1').read()
    # blank out comments (keep newlines so line numbers survive); strings are respected
    out = []; k = 0; n = len(raw)
    while k < n:
        c = raw[k]
        if c == '"' or c == "'":
            q = c; out.append(c); k += 1
            while k < n and raw[k] != q:
                if raw[k] == '\\' and k + 1 < n:
                    out.append(raw[k:k+2]); k += 2; continue
                out.append(raw[k]); k += 1
            if k < n: out.append(raw[k]); k += 1
        elif raw.startswith('//', k):
            while k < n and raw[k] != '\n': k += 1
        elif raw.startswith('/*', k):
            e = raw.find('*/', k + 2); e = n if e < 0 else e + 2
            out.append(''.join('\n' if ch == '\n' else ' ' for ch in raw[k:e])); k = e
        else:
            out.append(c); k += 1
    lines = ''.join(out).split('\n')
    cur = '?'
    for i, l in enumerate(lines):
        s = l.strip()
        m = fnre.match(l)
        if m and not l.rstrip().endswith(';'):
            cur = m.group(1)
        if 'CreateParticle' in l and not s.startswith('//') and 'void ParticleSystem::CreateParticle' not in l:
            # join until parentheses balance
            txt = l
            j = i
            while txt.count('(') > txt.count(')') and j + 1 < len(lines) and j - i < 6:
                j += 1; txt += ' ' + lines[j].strip()
            types = typere.findall(txt)
            # look back up to 12 lines for a type variable assignment if none on the call
            if not types:
                back = ' '.join(lines[max(0, i-15):i])
                types = [t + ' (var)' for t in dict.fromkeys(typere.findall(back))]
            # size argument = 4th top-level arg
            inner = txt[txt.find('CreateParticle'):]
            inner = inner[inner.find('(')+1:]
            depth = 0; args = ['']
            for ch in inner:
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    if depth == 0: break
                    depth -= 1
                if ch == ',' and depth == 0:
                    args.append('')
                else:
                    args[-1] += ch
            size = args[3].strip() if len(args) > 3 else 'type default'
            colour = args[4].strip() if len(args) > 4 else ''
            ctx = ' '.join(lines[max(0, i-8):j+1])
            rng = []
            if sync.search(ctx): rng.append('synced')
            if unsync.search(ctx): rng.append('unsynced')
            pathk = 'per frame (render)' if 'Render' in cur.split('::')[-1] else 'sim/event'
            rows.append((f"`{rel}:{i+1}`", f"`{cur}`", pathk, ', '.join(f"`{t}`" for t in types) or '?', f"`{size}`" if size else '', f"`{colour}`" if colour else '', '/'.join(rng) or '–'))
print('| Site | Function | Path | Particle type | Size arg | Colour arg | RNG within 8 lines |')
print('|---|---|---|---|---|---|---|')
for r in rows:
    print('| ' + ' | '.join(r) + ' |')
print(f"\n{len(rows)} call sites.")
