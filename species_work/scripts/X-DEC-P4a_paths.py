"""Repoint stale document paths in the Affects lines of decision files.
Usage: python3 -I X-DEC-P4a_paths.py <decision_file> [<decision_file> ...]
Folder documents become their folder path; the split units file is mapped per entry."""
import re, sys
FOLDERS = {
    'reference/06_controls_camera_and_commands.md': 'reference/06_controls_camera_and_commands/',
    'reference/07_bots.md': 'reference/07_bots/',
    'reference/10_audio_design.md': 'reference/10_audio_design/',
    'architecture/components/data_schemas.md': 'architecture/components/data_schemas/',
    'architecture/components/animation_and_vfx.md': 'architecture/components/animation_and_vfx/',
}
HAZ = {'DEC-014', 'DEC-090', 'DEC-107', 'DEC-113'}   # Abductor, airstrike, Missile Barrage, meteor
for p in sys.argv[1:]:
    t = open(p, encoding='utf-8').read()
    before = len(t.encode())
    out, cur = [], None
    for line in t.split('\n'):
        m = re.match(r'^#{2,4} (DEC-\d{3})\s', line)
        if m:
            cur = m.group(1)
        if line.startswith('- Affects:') or (line.startswith('| [DEC-') or line.startswith('| DEC-')):
            for a, b in FOLDERS.items():
                line = line.replace(a, b)
            if 'creatures_and_hazards.md' in line:
                rid = cur
                if line.startswith('|'):
                    rid = re.match(r'^\| \[?(DEC-\d{3})', line).group(1)
                tgt = 'hazards_and_projectiles.md' if rid in HAZ else 'creatures.md'
                line = line.replace('creatures_and_hazards.md', tgt)
        out.append(line)
    t2 = '\n'.join(out)
    open(p, 'w', encoding='utf-8').write(t2)
    print(p, before, '->', len(t2.encode()))
