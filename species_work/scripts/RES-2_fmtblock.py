"""RES-2 helper: clang-format the cpp blocks of a Markdown file that start at the given lines (1-based, the
line after the opening fence), in place.

Usage: python3 -I RES-2_fmtblock.py <clang-format> <style file> <file.md> <line> [<line> ...]
"""
import subprocess
import sys


def main():
    exe, style, path = sys.argv[1], sys.argv[2], sys.argv[3]
    starts = sorted((int(a) for a in sys.argv[4:]), reverse=True)
    lines = open(path, encoding='utf-8').read().split('\n')
    for start in starts:
        i = start - 1
        if not lines[i - 1].strip().startswith('```'):
            sys.exit(f'line {start - 1} is not a fence')
        j = i
        while not lines[j].startswith('```'):
            j += 1
        src = '\n'.join(lines[i:j]) + '\n'
        res = subprocess.run([exe, f'--style=file:{style}', '--assume-filename=Sketch.h'], input=src,
                             capture_output=True, text=True, check=True)
        lines[i:j] = res.stdout.rstrip('\n').split('\n')
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write('\n'.join(lines))
    print(f'formatted {len(starts)} block(s) of {path}')


if __name__ == '__main__':
    main()
