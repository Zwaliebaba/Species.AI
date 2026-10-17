"""FX-08: replace or format fenced code blocks of a Markdown file.

Usage:
  python3 -I FX-08_blocks.py replace <file.md> <lang> <index> <new_body_file>
      replace the body of the index-th (0-based) ```<lang> block by the text of new_body_file
  python3 -I FX-08_blocks.py format <clang_format_style_file> <file.md> [<file.md> ...]
      run clang-format (the repository's style) over the body of every ```cpp block, in place
"""
import re
import subprocess
import sys

FENCE_OPEN = re.compile(r'^```(\w+)\s*$')


def blocks(lines):
    out, i = [], 0
    while i < len(lines):
        m = FENCE_OPEN.match(lines[i])
        if m:
            j = i + 1
            while j < len(lines) and lines[j].strip() != '```':
                j += 1
            out.append((m.group(1), i, j))
            i = j + 1
        else:
            i += 1
    return out


def main():
    mode = sys.argv[1]
    if mode == 'replace':
        path, lang, index, body_path = sys.argv[2], sys.argv[3], int(sys.argv[4]), sys.argv[5]
        lines = open(path, encoding='utf-8').read().split('\n')
        found = [b for b in blocks(lines) if b[0] == lang]
        _, a, b = found[index]
        body = open(body_path, encoding='utf-8').read().rstrip('\n').split('\n')
        lines[a + 1:b] = body
        open(path, 'w', encoding='utf-8').write('\n'.join(lines))
        print(f'{path}: {lang} block {index} replaced ({b - a - 1} -> {len(body)} lines)')
    elif mode == 'format':
        style = sys.argv[2]
        for path in sys.argv[3:]:
            lines = open(path, encoding='utf-8').read().split('\n')
            for lang, a, b in reversed(blocks(lines)):
                if lang != 'cpp':
                    continue
                src = '\n'.join(lines[a + 1:b]) + '\n'
                res = subprocess.run(['clang-format', f'--style=file:{style}', '--assume-filename=Sketch.cpp'],
                                     input=src, capture_output=True, text=True, check=True)
                new = res.stdout.rstrip('\n').split('\n')
                if new != lines[a + 1:b]:
                    print(f'{path}: cpp block at line {a + 1} reformatted')
                lines[a + 1:b] = new
            open(path, 'w', encoding='utf-8').write('\n'.join(lines))
    else:
        raise SystemExit(__doc__)


main()
