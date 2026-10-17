"""Phase 4 wave B: build one work packet per document-fix agent from the wave A outputs.

Usage: python3 -I p4_build_fix_packets.py <REPO>

Reads: species_work/gate_report.md, species_work/p4_routes_*.csv, species_work/p4_dec_triage_*.csv,
species_work/p4_new_assets.csv, species_work/p4_arbitration.md, species_work/packages/<Phase 3>.json.
Writes: species_work/packages/FX-NN.json and prints a summary per packet.
"""
import csv
import glob
import json
import os
import re
import sys

REPO = sys.argv[1]
WORK = os.path.join(REPO, 'species_work')
GROUPS = {
    'FX-01': ['G00', 'G05'],
    'FX-02': ['G01', 'G11'],
    'FX-03': ['G02a1', 'G02a2', 'G02b'],
    'FX-04': ['G03a', 'G03b', 'G04'],
    'FX-05': ['G06', 'G07'],
    'FX-06': ['G08a', 'G08b', 'G09a', 'G09b'],
    'FX-07': ['G10', 'C-SIM-a', 'C-SIM-b', 'C-SIM-c'],
    'FX-08': ['C-RND-a', 'C-RND-b', 'C-TER'],
    'FX-09': ['C-AST', 'C-VFX', 'C-AEN'],
    'FX-10': ['C-UI-a', 'C-UI-b', 'C-INP', 'C-CAM'],
    'FX-11': ['C-DAT', 'C-PLT', 'C-BLD', 'C-DBG'],
    'FX-12': ['X-GLO-1', 'X-GLO-2'],
}
EXTRA_OWN = {'FX-12': ['glossary/', 'CONVENTIONS.md']}
PKG = {}
for p in glob.glob(os.path.join(WORK, 'packages', '*.json')):
    d = json.load(open(p, encoding='utf-8'))
    PKG[d['id']] = d


def norm(path):
    path = (path or '').strip().strip('`')
    path = re.sub(r'^(/home/user/Species\.AI/)?(design_next/)?', '', path)
    return path.split('#')[0].split(' (')[0].strip()


def owned(fx):
    own = set(EXTRA_OWN.get(fx, []))
    for pid in GROUPS[fx]:
        for t in PKG[pid]['targets']:
            own.add(t)
            if '/' in t and t.rsplit('/', 1)[0].count('/') >= 1:
                folder = t.rsplit('/', 1)[0] + '/'
                # a split folder belongs to the group when the group owns its README or all its targets
                if folder.startswith(('reference/', 'architecture/components/')) and folder.count('/') >= 2:
                    own.add(folder)
    return own


OWN = {fx: owned(fx) for fx in GROUPS}
PKG2FX = {pid: fx for fx, pids in GROUPS.items() for pid in pids}
PKG2FX['X-GLO'] = 'FX-12'


def fx_of_file(path):
    path = norm(path)
    best, blen = None, -1
    for fx, own in OWN.items():
        for o in own:
            if (path == o or (o.endswith('/') and path.startswith(o))) and len(o) > blen:
                best, blen = fx, len(o)
    if best is None:
        # a document that was split into a folder keeps its old name in some requests
        if path.endswith('.md'):
            return fx_of_file(path[:-3] + '/README.md') if not path.endswith('/README.md') else None
    return best


packets = {fx: dict(id=fx, phase=4, wave='B', packages=GROUPS[fx], owns=sorted(OWN[fx]),
                    id_ranges={pid: PKG[pid].get('id_ranges', {}) for pid in GROUPS[fx]},
                    gate_failures=[], routes=[], dec_citations=[], document_fixes=[], new_assets=[],
                    arbitration=[]) for fx in GROUPS}

# gate failures: "- <file>:<line> ..." or "... (<file>:<line>) ..."
gr = open(os.path.join(WORK, 'gate_report.md'), encoding='utf-8').read() if os.path.exists(os.path.join(WORK, 'gate_report.md')) else ''
for line in gr.split('\n'):
    if not line.startswith('- '):
        continue
    m = re.search(r'([\w./-]+\.md):\d+', line)
    fx = fx_of_file(m.group(1)) if m else None
    if fx:
        packets[fx]['gate_failures'].append(line[2:])

# routes
for p in sorted(glob.glob(os.path.join(WORK, 'p4_routes_*.csv'))):
    for r in csv.DictReader(open(p, encoding='utf-8', newline='')):
        if r.get('status', 'open').strip().lower() != 'open':
            continue
        kind = (r.get('kind') or '').strip().lower()
        fx = 'FX-12' if kind in ('ledger', 'conventions', 'glossary') else None
        fx = fx or fx_of_file(r.get('to_file', '')) or PKG2FX.get((r.get('to_package') or '').strip())
        if fx:
            packets[fx]['routes'].append({k: r.get(k, '') for k in ('route_id', 'source', 'to_file', 'kind', 'instruction', 'note')})

# DEC triage: citations and document fixes
for p in sorted(glob.glob(os.path.join(WORK, 'p4_dec_triage_*.csv'))):
    for r in csv.DictReader(open(p, encoding='utf-8', newline='')):
        dec = (r.get('dec_id') or '').strip()
        cite = r.get('cite_in') or ''
        fix = r.get('document_fix') or ''
        for target in re.findall(r'[\w./-]+\.md', cite):
            fx = fx_of_file(target)
            if fx and dec:
                packets[fx]['dec_citations'].append({'dec': dec, 'cite_in': cite, 'outcome': r.get('outcome', ''),
                                                     'source': r.get('source', '')})
                break
        if fix.strip():
            files = re.findall(r'[\w./-]+\.md', fix) or re.findall(r'[\w./-]+\.md', cite)
            for fx in {fx_of_file(f) for f in files} - {None}:
                packets[fx]['document_fixes'].append({'dec': dec, 'fix': fix, 'source': r.get('source', '')})

# new assets
na = os.path.join(WORK, 'p4_new_assets.csv')
if os.path.exists(na):
    for r in csv.DictReader(open(na, encoding='utf-8', newline='')):
        for f in re.split(r'[;,]', r.get('consumer_file', '')):
            fx = fx_of_file(f)
            if fx:
                packets[fx]['new_assets'].append({'asset_id': r.get('asset_id', ''), 'consumer_file': norm(f),
                                                  'request': r.get('cited_by_request', '')})

# arbitration entries that name an owned file or package
arb = os.path.join(WORK, 'p4_arbitration.md')
if os.path.exists(arb):
    t = open(arb, encoding='utf-8').read()
    for m in re.finditer(r'^#{2,4}\s+(A-\d+)\b.*?(?=^#{2,4}\s+A-\d+\b|\Z)', t, re.M | re.S):
        body = m.group(0)
        hit = {fx_of_file(f) for f in re.findall(r'[\w./-]+\.md', body)} - {None}
        hit |= {PKG2FX[p] for p in PKG2FX if re.search(r'\b' + re.escape(p) + r'\b', body)}
        for fx in hit:
            packets[fx]['arbitration'].append(m.group(1))

for fx, pk in packets.items():
    json.dump(pk, open(os.path.join(WORK, 'packages', fx + '.json'), 'w', encoding='utf-8'), indent=1, ensure_ascii=False)
    print(f"{fx}: gate={len(pk['gate_failures'])} routes={len(pk['routes'])} cites={len(pk['dec_citations'])} "
          f"fixes={len(pk['document_fixes'])} assets={len(pk['new_assets'])} arb={len(pk['arbitration'])}")
