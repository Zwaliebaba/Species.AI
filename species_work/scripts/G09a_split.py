"""G09a: cut the block between two exact heading lines out of a file.
Usage: python3 -I G09a_split.py <src> <start heading> <end heading> <out block file>"""
import sys
src, start, end, out = sys.argv[1:5]
text = open(src, encoding='utf-8').read()
i = text.index('\n' + start + '\n') + 1
j = text.index('\n' + end + '\n') + 1
open(out, 'w', encoding='utf-8').write(text[i:j])
open(src, 'w', encoding='utf-8').write(text[:i] + text[j:])
print('moved', j - i, 'bytes')
