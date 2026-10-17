"""FX-01 helper: replace an exact block of lines in a file.

Usage: python3 -I FX-01_replace.py <file> <first_line> <last_line> <new_text_file>
Lines are 1-based and inclusive. Prints the replaced line count.
"""
import sys

path, first, last, newp = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
lines = open(path, encoding='utf-8').read().split('\n')
new = open(newp, encoding='utf-8').read().rstrip('\n').split('\n')
lines[first - 1:last] = new
open(path, 'w', encoding='utf-8').write('\n'.join(lines))
print(f'replaced lines {first}-{last} with {len(new)} lines')
