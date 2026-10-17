"""RES-2 helper: apply exact, unique text replacements from a spec file.

Usage: python3 -I RES-2_apply.py <spec> [--check]

Spec format (one or more blocks):
    @@@ <file path>
    <<<
    old text (one or more lines)
    ===
    new text (zero or more lines)
    >>>
Every old text must occur exactly once in its file, after the earlier blocks of the same file have applied.
Nothing is written unless every block applies.
"""
import sys


def parse(spec_text):
    blocks = []
    lines = spec_text.split('\n')
    i = 0
    path = None
    while i < len(lines):
        line = lines[i]
        if line.startswith('@@@ '):
            path = line[4:].strip()
            i += 1
            continue
        if line == '<<<':
            j = lines.index('===', i + 1)
            k = lines.index('>>>', j + 1)
            old = '\n'.join(lines[i + 1:j])
            new = '\n'.join(lines[j + 1:k])
            blocks.append((path, old, new))
            i = k + 1
            continue
        i += 1
    return blocks


def main():
    with open(sys.argv[1], encoding='utf-8') as f:
        blocks = parse(f.read())
    check_only = '--check' in sys.argv[2:]
    texts = {}
    failures = []
    for n, (path, old, new) in enumerate(blocks):
        if path not in texts:
            with open(path, encoding='utf-8', newline='') as f:
                texts[path] = f.read()
        count = texts[path].count(old)
        if count != 1:
            failures.append(f'block {n} {path}: old text found {count} times: {old[:100]!r}')
            continue
        texts[path] = texts[path].replace(old, new, 1)
    if failures:
        print('\n'.join(failures))
        sys.exit(1)
    if check_only:
        print(f'{len(blocks)} blocks apply to {len(texts)} files')
        return
    for path, text in texts.items():
        with open(path, 'w', encoding='utf-8', newline='') as f:
            f.write(text)
        print(f'wrote {path} ({len(text.encode("utf-8"))} bytes)')


if __name__ == '__main__':
    main()
