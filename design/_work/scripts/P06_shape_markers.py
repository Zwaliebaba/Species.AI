"""P06: list markers per shape file and count those matched by Building::SetShapePorts
(strstr "MarkerPort") and SetShapeLights (strstr "MarkerLight").
Usage: python3 -I P06_shape_markers.py <shapes_dir> shape1.shp [shape2.shp ...]
"""
import sys, os

def main():
    d = sys.argv[1]
    for s in sys.argv[2:]:
        p = os.path.join(d, s)
        if not os.path.isfile(p):
            print(f"{s}: MISSING"); continue
        names = []
        with open(p, 'r', encoding='latin-1') as f:
            for ln in f:
                if ln.startswith('Marker:'):
                    names.append(ln.split(':', 1)[1].strip())
        ports = [n for n in names if 'MarkerPort' in n]
        lights = [n for n in names if 'MarkerLight' in n]
        print(f"{s}: ports={len(ports)} lights={len(lights)} markers={names}")

if __name__ == '__main__':
    main()
