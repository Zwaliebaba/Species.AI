"""RES-2 helper: report whether each cpp block of the given Markdown files is clang-format clean.

Usage: python3 -I RES-2_fmtcheck.py <clang-format> <style file> <file.md> [...]
Prints one line per cpp block: file, first line, CLEAN or the number of differing lines.
With --show, prints the formatted block of every block that differs.
"""
import difflib
import subprocess
import sys


def blocks(path):
    lines = open(path, encoding='utf-8').read().split('\n')
    out = []
    i = 0
    while i < len(lines):
        if lines[i].strip() in ('```cpp', '```c++'):
            j = i + 1
            while not lines[j].startswith('```'):
                j += 1
            out.append((i + 2, lines[i + 1:j]))
            i = j
        i += 1
    return out


def main():
    exe, style = sys.argv[1], sys.argv[2]
    show = '--show' in sys.argv
    for path in [a for a in sys.argv[3:] if a != '--show']:
        for start, body in blocks(path):
            src = '\n'.join(body) + '\n'
            res = subprocess.run([exe, f'--style=file:{style}', '--assume-filename=Sketch.h'], input=src,
                                 capture_output=True, text=True)
            got = res.stdout
            if got == src:
                print(f'{path}:{start}\tCLEAN')
            else:
                diff = [d for d in difflib.unified_diff(src.split('\n'), got.split('\n'), lineterm='', n=0)
                        if d.startswith(('+', '-')) and not d.startswith(('+++', '---'))]
                print(f'{path}:{start}\tDIFF {len(diff)} lines')
                if show:
                    print('\n'.join(diff[:80]))


if __name__ == '__main__':
    main()
