"""FX-06: update the requirement-id ranges in reference/09 README.md after R-META-235 was added.

Usage: python3 -I FX-06_readme09.py <path to README.md>
"""
import sys

path = sys.argv[1]
t = open(path, encoding='utf-8').read()
pairs = [
    ('the explainer panel shown during ready-up; the match start as the front end sees it | R-META-200 to R-META-234 |',
     'the explainer panel shown during ready-up; the match start as the front end sees it; the version string | R-META-200 to R-META-235 |'),
    ('R-META-106 to R-META-199 and R-META-235 to R-META-399 are unassigned.',
     'R-META-106 to R-META-199 and R-META-236 to R-META-399 are unassigned.'),
]
for a, b in pairs:
    if t.count(a) != 1:
        sys.exit(f'not found once: {a[:60]}')
    t = t.replace(a, b)
open(path, 'w', encoding='utf-8', newline='\n').write(t)
print('README ranges updated')
