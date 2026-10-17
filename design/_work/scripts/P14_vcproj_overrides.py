import sys, xml.etree.ElementTree as ET, collections
tree = ET.parse(sys.argv[1])
root = tree.getroot()
out = collections.Counter()
rows = []
def walk(node, filt):
    for ch in node:
        if ch.tag == 'Filter':
            walk(ch, filt + [ch.get('Name')])
        elif ch.tag == 'File':
            path = ch.get('RelativePath')
            for fc in ch.findall('FileConfiguration'):
                cfg = fc.get('Name')
                attrs = {k: v for k, v in fc.attrib.items() if k != 'Name'}
                for t in fc.findall('Tool'):
                    for k, v in t.attrib.items():
                        if k == 'Name' or v == '':
                            continue
                        attrs[t.get('Name') + '.' + k] = v
                if attrs:
                    rows.append((path, cfg, attrs))
            walk(ch, filt)
walk(root.find('Files'), [])
for p, c, a in rows:
    print(p, c, a)
