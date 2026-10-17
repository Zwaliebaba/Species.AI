"""RES-2 helper: replace the span of a file that starts with one marker and ends with another (both included) by
the content of a replacement file. Each marker must occur exactly once (the end marker after the start).

Usage: python3 -I RES-2_replace_span.py <file> <start-marker-file> <end-marker-file> <replacement-file>
Marker files hold the marker text exactly (a trailing newline of the file is ignored).
"""
import sys


def read(path):
    with open(path, encoding='utf-8', newline='') as f:
        text = f.read()
    return text


def main():
    target, start_file, end_file, repl_file = sys.argv[1:5]
    text = read(target)
    start = read(start_file).rstrip('\n')
    end = read(end_file).rstrip('\n')
    repl = read(repl_file).rstrip('\n')
    if text.count(start) != 1:
        sys.exit(f'start marker found {text.count(start)} times')
    s = text.index(start)
    if text.count(end, s) != 1:
        sys.exit(f'end marker found {text.count(end, s)} times after the start')
    e = text.index(end, s) + len(end)
    text = text[:s] + repl + text[e:]
    with open(target, 'w', encoding='utf-8', newline='') as f:
        f.write(text)
    print(f'replaced {e - s} characters with {len(repl)} in {target}')


if __name__ == '__main__':
    main()
