"""P06: build the Building type enum table from building.h / building.cpp.
Usage: python3 -I P06_building_enum.py <SRC>/code/worldobject
Prints markdown rows: value | enum | name string | class | header | blocked | forced global
"""
import sys, os, re

def main():
    d = sys.argv[1]
    h = open(os.path.join(d, 'building.h'), encoding='latin-1').read()
    c = open(os.path.join(d, 'building.cpp'), encoding='latin-1').read()
    enum_block = h[h.index('TypeInvalid'):h.index('NumBuildingTypes')]
    enums = re.findall(r'\b(Type[A-Za-z0-9]+)\b', enum_block)
    names_block = c[c.index('buildingNames[] = {'):]
    names_block = names_block[:names_block.index('};')]
    names = re.findall(r'"([^"]+)"', names_block)
    fac = dict(re.findall(r'case\s+(Type[A-Za-z0-9]+)\s*:\s*building\s*=\s*new\s+([A-Za-z0-9_]+)', c))
    blk_block = c[c.index('bool Building::BuildingIsBlocked'):c.index('Building *Building::CreateBuilding( char')]
    blocked = set(re.findall(r'Building::(Type[A-Za-z0-9]+)', blk_block))
    glob_block = c[c.index('if( _type == TypeRadarDish'):]
    glob_block = glob_block[:glob_block.index(')\n')+2]
    forced = set(re.findall(r'(Type[A-Za-z0-9]+)', glob_block))
    # class -> header
    cls_hdr = {}
    for fn in os.listdir(d):
        if fn.endswith('.h'):
            txt = open(os.path.join(d, fn), encoding='latin-1').read()
            for m in re.finditer(r'^\s*class\s+([A-Za-z0-9_]+)\s*(?::|\{|$)', txt, re.M):
                cls_hdr.setdefault(m.group(1), fn)
    print(len(enums), len(names))
    for i, e in enumerate(enums):
        n = names[i] if i < len(names) else '?'
        cl = fac.get(e, '— (none: CreateBuilding returns NULL)')
        hd = cls_hdr.get(fac.get(e, ''), '')
        print(f"| {i} | `{e}` | `{n}` | `{cl}` | {('`'+hd+'`') if hd else ''} | {'**BLOCKED**' if e in blocked else 'no'} | {'yes' if e in forced else ''} |")

if __name__ == '__main__':
    main()
