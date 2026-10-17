"""S31 helper: emulate tools/build_multiwinia_data.bat + tools/listfiles.pl + tools/build_language_dat.bat
to compute the content of every .dat archive for a given SKU, from the checked-in data/ tree.

Usage: python3 -I S_31_archive_contents.py <SRC_ROOT> [platform] [language] [--list ARCHIVE]
  platform: '' (no argument, as build_multiwinia.bat), retail, pchwcompat, pcbetatest, pcpreview,
            pcassaultstress, ambx  (xbox is not emulated)
  language: '' -> 'all'  (only honoured by build_language_dat when platform == retail)

Emulation rules (from the scripts):
  * completeExclusions = tools/sounds.txt + tools/menu_sounds.txt + per-platform exclusion file.
  * listfiles.pl: lines '**/X' prune any path component whose basename == X (case-sensitive);
    other lines are bsd_glob patterns relative to data/ (Windows => case-insensitive), a visited
    path equal (case-insensitive) to an expanded pattern is pruned (directories prune subtrees).
    bsd_glob default flags include GLOB_NOMAGIC: a pattern without wildcards is kept verbatim.
  * main.dat = listAll minus lines containing 'sounds/' (+ platform inclusion list), filtered by
    extension *.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim (Windows dir => case-insensitive).
  * init.dat = tools/init.list minus textures/xboxlivearcade.bmp.
  * menusounds.dat = tools/menu_sounds.txt ; sounds.dat = tools/sounds.txt ;
    sounds2.dat = lines of listAll matching regex '.ogg' (grep, case-sensitive).
  * language.dat = union of *language.list files chosen by build_language_dat.bat, filtered to
    *.bmp *.dds *.tex *.txt.
"""
import sys, os, re, fnmatch, collections

src = sys.argv[1]
args = [a for a in sys.argv[2:] if not a.startswith('--')]
platform = args[0] if len(args) > 0 else ''
language = args[1] if len(args) > 1 else ''
listarc = None
if '--list' in sys.argv:
    listarc = sys.argv[sys.argv.index('--list') + 1]

data = os.path.join(src, 'data')
tools = os.path.join(src, 'tools')

def read_lines(p):
    out = []
    with open(p, 'rb') as f:
        for raw in f.read().decode('latin-1').split('\n'):
            l = raw.replace('\r', '')
            if l != '':
                out.append(l)
    return out

# ---------- data tree ----------
files = []      # relative paths with original case, '/' separators
dirs = set()
for d, ds, fs in os.walk(data):
    for x in ds:
        dirs.add(os.path.relpath(os.path.join(d, x), data).replace(os.sep, '/'))
    for x in fs:
        files.append(os.path.relpath(os.path.join(d, x), data).replace(os.sep, '/'))
allpaths = files + sorted(dirs)
lower_index = collections.defaultdict(list)
for p in allpaths:
    lower_index[p.lower()].append(p)

def bsd_glob(pattern):
    if not any(c in pattern for c in '*?[{'):
        return [pattern]                     # GLOB_NOMAGIC
    pat = pattern.lower()
    return [p for p in allpaths if fnmatch.fnmatchcase(p.lower(), pat) and pat.count('/') == p.count('/')]

excl_file = {
    'pchwcompat': 'multiwinia_exclusions_hwcompat.txt',
    'pcassaultstress': 'multiwinia_exclusions_hwcompat.txt',
    'pcbetatest': 'multiwinia_exclusions_beta_test.txt',
    'pcpreview': 'multiwinia_exclusions_beta_test.txt',
    'ambx': 'multiwinia_exclusions_beta_test.txt',
}.get(platform, 'multiwinia_exclusions.txt')
excl_lines = read_lines(os.path.join(tools, 'sounds.txt')) + read_lines(os.path.join(tools, 'menu_sounds.txt')) \
    + read_lines(os.path.join(tools, excl_file))
short, longx = set(), set()
for l in excl_lines:
    l = l.replace('\\', '/')
    if l.startswith('**/'):
        short.add(l[3:])
    else:
        for g in bsd_glob(l):
            longx.add(g.lower())

def pruned(path):
    parts = path.split('/')
    for i in range(1, len(parts) + 1):
        pre = '/'.join(parts[:i])
        if parts[i - 1] in short or pre.lower() in longx:
            return True
    return False

listAll = [p for p in files if not pruned(p)]
EXT_MAIN = ('.tex', '.bmp', '.jpg', '.shp', '.txt', '.vs', '.ps', '.ps3', '.h', '.anim')
incl_file = {
    'pchwcompat': 'multiwinia_inclusions_hwcompat.txt',
    'pcassaultstress': 'multiwinia_inclusions_assaultstresstest.txt',
    'pcbetatest': 'multiwinia_inclusions_beta_test_%s.txt' % language,
    'pcpreview': 'multiwinia_inclusions_previewcopy.txt',
    'ambx': 'multiwinia_inclusions_ambx.txt',
}.get(platform)
main = [p for p in listAll if 'sounds/' not in p]
if incl_file:
    ip = os.path.join(tools, incl_file)
    if os.path.exists(ip):
        for l in read_lines(ip):
            hits = lower_index.get(l.lower())
            if hits:
                main.append(hits[0])
    else:
        print('# inclusion list absent:', incl_file, file=sys.stderr)
main = sorted(set(p for p in main if p.lower().endswith(EXT_MAIN)))

init = [l for l in read_lines(os.path.join(tools, 'init.list')) if l != 'textures/xboxlivearcade.bmp']
menus = read_lines(os.path.join(tools, 'menu_sounds.txt'))
sounds = read_lines(os.path.join(tools, 'sounds.txt'))
sounds2 = [p for p in listAll if re.search(r'.ogg', p)]

lang = language if language else 'all'
LISTS = {'all': ['language.list'], 'efigs': ['englishlanguage.list', 'frenchlanguage.list', 'germanlanguage.list',
         'italianlanguage.list', 'spanishlanguage.list']}
if platform == 'retail':
    chosen = LISTS.get(lang.lower(), ['%slanguage.list' % lang.lower()])
else:
    chosen = ['englishlanguage.list']
langfiles = []
for c in chosen:
    for l in read_lines(os.path.join(tools, c)):
        if l.lower().endswith(('.bmp', '.dds', '.tex', '.txt')):
            langfiles.append(l)
langfiles = sorted(set(langfiles), key=str.lower)

arcs = collections.OrderedDict([('init.dat', init), ('language.dat', langfiles), ('main.dat', main),
                                ('menusounds.dat', menus), ('sounds.dat', sounds), ('sounds2.dat', sounds2)])
if listarc:
    for p in arcs[listarc]:
        print(p)
    sys.exit(0)
print('# platform=%r language=%r exclusions=%s inclusions=%s language lists=%s' % (platform, language, excl_file, incl_file, chosen))
print('| Archive | Files | By top-level directory | By extension |')
print('|---|---|---|---|')
for name, lst in arcs.items():
    td = collections.Counter(p.split('/')[0] if '/' in p else '(root)' for p in lst)
    ex = collections.Counter(os.path.splitext(p)[1].lower() for p in lst)
    print('| `%s` | %d | %s | %s |' % (name, len(lst), ', '.join('%s %d' % kv for kv in sorted(td.items())),
                                       ', '.join('%s %d' % kv for kv in sorted(ex.items()))))
dup = sorted(set(p.lower() for p in init) & set(p.lower() for p in main))
print('# in both init.dat and main.dat:', dup)
unpacked = sorted(p for p in files if p not in main and p.lower() not in set(x.lower() for x in init + langfiles + menus + sounds + sounds2))
c = collections.Counter(p.split('/')[0] if '/' in p else '(root)' for p in unpacked)
print('# data files packed in no archive: %d %s' % (len(unpacked), dict(sorted(c.items()))))
