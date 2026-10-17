#!/usr/bin/env python3
"""X-DEC-3 build helper: validate decisions/open_questions.md, regenerate its index,
write species_work/dec_map_X-DEC-3.csv and render the report from a template.

Usage (run from the repository root, with python3 -I):
  python3 -I species_work/scripts/X-DEC-3_build.py <repo_root> [<report_template> <report_out>]

The template may contain the placeholders {{COUNTS}}, {{MAPPING}}, {{OUTSIDE}}, {{ENTRIES}} and {{CITES}}.
This is a WORK file: it names original question ids on purpose (they never reach NEXT).
"""
import csv
import os
import re
import sys

ORIG41 = '41_risks_and_open_questions.md'

# ---------------------------------------------------------------------------
# Every question row of the package ranges: (line, ids, mapping).
# mapping is either 'DEC-3nn' (an entry of this register) or 'covered: <refs>'.
ROWS = [
    (218, '01-U1 / 09-U3', 'covered: DEC-067'),
    (219, '01-U2 / 00-U2', 'covered: DEC-002, DEC-003'),
    (220, '01-U5', 'covered: DEC-068'),
    (221, '01-U6', 'covered: DEC-075'),
    (222, '01-U7', 'covered: DEC-076'),
    (223, '01-U8 / 03-U15', 'covered: DEC-064'),
    (224, '01-U9', 'covered: DEC-070'),
    (225, '01-U10 / 00-U5', 'covered: DEC-082'),
    (226, '01-U11 / 00-U6', 'covered: DEC-066'),
    (227, '01-U12', 'DEC-300'),
    (235, '02-U1', 'covered: DEC-090'),
    (236, '02-U3', 'covered: DEC-009'),
    (237, '02-U5', 'covered: DEC-113'),
    (238, '02-U6 / 06-U8', 'covered: DEC-117'),
    (239, '02-U7 / 03-U4', 'covered: DEC-115'),
    (240, '02-U12', 'covered: DEC-014'),
    (241, '02-U15', 'DEC-302'),
    (242, '02-U16', 'DEC-303'),
    (250, '03-U1', 'covered: DEC-117, DEC-079'),
    (251, '03-U2', 'covered: DEC-117'),
    (252, '03-U3 / 02-U17 / 04-U9', 'DEC-334'),
    (253, '03-U5', 'DEC-331'),
    (254, '03-U6', 'covered: DEC-052'),
    (255, '03-U7 / 30-U4', 'covered: DEC-015'),
    (256, '03-U8', 'covered: DEC-052'),
    (257, '03-U9', 'covered: DEC-089'),
    (258, '03-U12', 'covered: DEC-012'),
    (259, '03-U13 / 01-U3', 'covered: DEC-108'),
    (260, '03-U14', 'covered: DEC-117'),
    (261, '03-U16', 'DEC-301'),
    (262, '03-U17', 'DEC-309'),
    (270, '04-U1 / 07-U4', 'covered: DEC-014'),
    (271, '04-U2', 'covered: DEC-114'),
    (272, '04-U3', 'covered: DEC-014'),
    (273, '04-U4 / 02-U2 / 02-U4', 'covered: DEC-107'),
    (274, '04-U5 / 03-U10', 'DEC-305'),
    (275, '04-U6', 'covered: DEC-022'),
    (276, '04-U7 / 02-U11', 'covered: DEC-111'),
    (277, '04-U10', 'covered: DEC-116'),
    (278, '04-U11', 'covered: DEC-111'),
    (279, '04-U12', 'covered: DEC-002'),
    (287, '05-U1', 'covered: DEC-094, DEC-016'),
    (288, '05-U2', 'DEC-306'),
    (289, '05-U3 / 02-U9', 'DEC-307'),
    (290, '05-U5 / 00-U3', 'covered: DEC-081'),
    (291, '05-U6 / 00-U4 / 03-U11', 'covered: DEC-079'),
    (292, '05-U7', 'covered: DEC-065'),
    (293, '05-U9', 'DEC-308'),
    (294, '05-U10 / 02-U10', 'covered: DEC-011'),
    (295, '05-U11', 'DEC-304'),
    (296, '05-U12', 'covered: DEC-111'),
    (304, '06-U1', 'DEC-338'),
    (305, '06-U2', 'DEC-338'),
    (306, '06-U3', 'DEC-338'),
    (307, '06-U5', 'covered: DEC-008'),
    (308, '06-U6', 'DEC-338'),
    (309, '06-U9', 'covered: DEC-202 (item 7)'),
    (310, '06-U10', 'DEC-310'),
    (311, '06-U11', 'DEC-332'),
    (312, '06-U12', 'covered: DEC-015'),
    (320, '07-U1', 'covered: DEC-087, DEC-112'),
    (321, '07-U2', 'covered: DEC-087, DEC-112'),
    (322, '07-U5', 'covered: DEC-087'),
    (323, '07-U6', 'covered: DEC-031'),
    (324, '07-U7 / 02-U13', 'DEC-311'),
    (325, '07-U8', 'covered: DEC-087'),
    (326, '07-U9', 'covered: DEC-073'),
    (327, '07-U10', 'covered: DEC-087, DEC-112'),
    (328, '07-U11', 'covered: DEC-117'),
    (329, '07-U12', 'DEC-312'),
    (337, '08-U1 / 29-U1', 'DEC-332'),
    (338, '08-U3', 'DEC-332'),
    (339, '08-U4', 'DEC-314'),
    (340, '08-U5', 'DEC-334'),
    (341, '08-U6', 'DEC-333'),
    (342, '08-U7', 'DEC-342'),
    (343, '08-U8', 'covered: DEC-045'),
    (344, '08-U9', 'covered: DEC-045, DEC-015'),
    (345, '08-U10', 'covered: DEC-045, DEC-015'),
    (346, '08-U11', 'DEC-332'),
    (347, '08-U12', 'DEC-315'),
    (348, '08-U13', 'covered: DEC-204, ADR-0060'),
    (356, '09-U1 / 00-U1 / 01-U4', 'covered: DEC-047'),
    (357, '09-U2 / 22-U16 / 31-I4 / 31-U5', 'DEC-333'),
    (358, '09-U4 / 00-U9 / 29-U2 / 31-I5 / 31-U8', 'covered: DEC-100'),
    (359, '09-U5', 'DEC-335'),
    (360, '09-U6', 'DEC-316'),
    (361, '09-U7', 'DEC-335'),
    (362, '09-U8', 'DEC-342'),
    (363, '09-U9 / 30-U8', 'covered: DEC-053'),
    (364, '09-U10', 'DEC-327'),
    (365, '09-U13', 'covered: DEC-245 (item 10)'),
    (366, '09-U16', 'covered: DEC-224 (item 7)'),
    (367, '09-U17', 'DEC-333'),
    (375, '10-U1 / 25-U9', 'DEC-317'),
    (376, '10-U2 / 25-U8', 'DEC-332'),
    (377, '10-U3', 'DEC-342'),
    (378, '10-U4', 'covered: DEC-097'),
    (379, '10-U8', 'DEC-331'),
    (380, '10-U9', 'covered: DEC-097'),
    (381, '10-U10', 'covered: DEC-097'),
    (382, '10-U12 / 00-U10', 'covered: DEC-049'),
    (383, '10-U14', 'DEC-318'),
    (384, '10-U15', 'DEC-319'),
    (385, '10-U16', 'DEC-320'),
    (396, '20-U4', 'covered: DEC-066, DEC-011'),
    (397, '20-U6 / 04-U8', 'covered: DEC-011'),
    (402, '20-U13', 'covered: DEC-011, DEC-223 (item 2)'),
    (411, '21-U1 / 22-U9', 'covered: DEC-012, DEC-013'),
    (412, '21-U2 / 22-U14', 'covered: DEC-094'),
    (413, '21-U3', 'covered: DEC-095'),
    (414, '21-U4 / 02-U8 / 06-U4', 'covered: DEC-095, DEC-200 (item 5)'),
    (415, '21-U6 / 02-U14 / 05-U8', 'covered: DEC-028'),
    (416, '21-U7', 'covered: DEC-028'),
    (417, '21-U8', 'covered: DEC-014'),
    (418, '21-U9', 'covered: DEC-094'),
    (419, '21-U10', 'covered: DEC-028'),
    (420, '21-U11', 'DEC-313'),
    (441, '22-U17 / 05-U4 / 20-U5', 'covered: DEC-011, DEC-223 (item 2)'),
    (454, '23-U3', 'covered: DEC-013, DEC-242 (item 2)'),
    (455, '23-U4', 'covered: DEC-246 (item 7), DEC-113'),
    (457, '23-I3', 'DEC-324'),
    (458, '23-I4', 'DEC-324'),
    (459, '23-I5', 'covered: DEC-246 (item 7)'),
    (467, '24-U1 / 31-U1 / 31-U2', 'DEC-336'),
    (468, '24-U2', 'DEC-336'),
    (469, '24-U3', 'DEC-336'),
    (470, '24-U4', 'DEC-336'),
    (471, '24-U5', 'DEC-336'),
    (472, '24-U8', 'covered: DEC-098'),
    (473, '24-U9', 'DEC-337'),
    (474, '24-U10', 'covered: DEC-029, ADR-0020'),
    (475, '24-U11', 'DEC-337'),
    (476, '24-U12', 'DEC-336'),
    (477, '24-U13 / 31-U3', 'DEC-336'),
    (478, '24-U14', 'DEC-325'),
    (479, '24-U15', 'DEC-326'),
    (487, '25-U1 / 00-U8 / 29-U9 / 31-U7', 'DEC-332'),
    (488, '25-U2', 'DEC-332'),
    (489, '25-U3', 'DEC-329'),
    (490, '25-U4', 'DEC-337'),
    (491, '25-U5', 'covered: DEC-012'),
    (492, '25-U6', 'covered: DEC-098'),
    (500, '26-U1', 'covered: DEC-037'),
    (501, '26-U2', 'covered: DEC-101'),
    (502, '26-U5 / 02-U18 / 26-I4', 'covered: DEC-007, DEC-222'),
    (503, '26-U6', 'DEC-336'),
    (504, '26-U7', 'covered: DEC-222'),
    (505, '26-U8', 'DEC-328'),
    (506, '26-U9', 'DEC-339'),
    (507, '26-I1', 'covered: DEC-012'),
    (508, '26-I2', 'covered: DEC-013'),
    (509, '26-I3', 'covered: DEC-013'),
    (517, '27-U1 / 10-U7', 'DEC-339'),
    (518, '27-U2', 'covered: DEC-049'),
    (519, '27-U3', 'DEC-339'),
    (520, '27-U4', 'DEC-339'),
    (521, '27-U5 / 10-U6', 'DEC-321'),
    (522, '27-U6', 'DEC-339'),
    (523, '27-U7', 'DEC-339'),
    (524, '27-U8 / 10-U5', 'covered: DEC-109'),
    (525, '27-U9', 'DEC-339'),
    (526, '27-U10 / 10-U13', 'DEC-322'),
    (527, '27-U11', 'DEC-339'),
    (528, '27-U12', 'DEC-339'),
    (529, '27-U13 / 20-U7', 'covered: DEC-224 (item 7)'),
    (530, '27-U14 / 10-U11', 'DEC-323'),
    (536, '28-U1', 'DEC-316'),
    (537, '28-U2', 'DEC-316'),
    (538, '28-U3 / 09-U14 / 29-U3', 'covered: DEC-099'),
    (539, '28-U4', 'covered: ADR-0063, ADR-0032'),
    (540, '28-U5', 'covered: DEC-243 (item 1), DEC-202'),
    (541, '28-U6', 'covered: DEC-102'),
    (542, '28-U7', 'covered: DEC-102'),
    (543, '28-U8', 'covered: DEC-102'),
    (544, '28-U9', 'DEC-333'),
    (550, '29-U4 / 25-U10', 'DEC-330'),
    (551, '29-U5 / 25-U7', 'DEC-340'),
    (552, '29-U6', 'covered: DEC-015'),
    (553, '29-U7', 'DEC-340'),
    (554, '29-U8 / 26-U4', 'DEC-340'),
    (555, '29-U10', 'DEC-341'),
    (556, '29-U11', 'covered: DEC-015'),
    (557, '29-U13', 'DEC-340'),
    (558, '29-U14 / 23-I2 / 31-I2', 'covered: DEC-204'),
    (566, '30-U1 / 09-U15', 'covered: DEC-110'),
    (567, '30-U2', 'covered: DEC-110'),
    (568, '30-U5', 'covered: DEC-103'),
    (569, '30-U6', 'covered: DEC-053'),
    (577, '31-U6', 'DEC-340'),
    (578, '31-U9', 'DEC-340'),
    (579, '31-U10', 'DEC-332'),
    (580, '31-U11', 'covered: DEC-041'),
    (581, '31-U12', 'DEC-340'),
    (582, '31-U13', 'covered: ADR-0074, DEC-224 (item 5)'),
    (583, '31-U14 / 07-U3', 'covered: DEC-005'),
    (584, '31-I3', 'covered: ADR-0065'),
    (592, '32-U1 / 31-U4', 'DEC-341'),
    (593, '32-U2 / 24-U6 / 26-U3', 'covered: DEC-106, DEC-228'),
    (594, '32-U3 / 08-U2', 'DEC-334'),
    (595, '32-U4', 'DEC-341'),
    (596, '32-U5', 'DEC-341'),
    (597, '32-U6', 'DEC-341'),
    (598, '32-U7', 'DEC-341'),
    (599, '32-U8', 'DEC-332'),
    (600, '32-I1', 'covered: DEC-202 (item 8)'),
    (601, '32-I2', 'covered: DEC-225 (item 3)'),
    (602, '32-I3', 'covered: DEC-041, DEC-115'),
]

# Merge-pointer lines: line -> list of (merged id, owner id).
MERGES = {
    212: [('00-U1', '09-U1'), ('00-U2', '01-U2'), ('00-U3', '05-U5'), ('00-U4', '05-U6'), ('00-U5', '01-U10'),
          ('00-U6', '01-U11'), ('00-U7', '22-U1'), ('00-U8', '25-U1'), ('00-U9', '09-U4'), ('00-U10', '10-U12')],
    229: [('01-U3', '03-U13'), ('01-U4', '09-U1')],
    244: [('02-U2', '04-U4'), ('02-U4', '04-U4'), ('02-U8', '21-U4'), ('02-U9', '05-U3'), ('02-U10', '05-U10'),
          ('02-U11', '04-U7'), ('02-U13', '07-U7'), ('02-U14', '21-U6'), ('02-U17', '03-U3'), ('02-U18', '26-U5')],
    264: [('03-U4', '02-U7'), ('03-U10', '04-U5'), ('03-U11', '05-U6'), ('03-U15', '01-U8')],
    281: [('04-U8', '20-U6'), ('04-U9', '03-U3')],
    298: [('05-U4', '22-U17'), ('05-U8', '21-U6')],
    314: [('06-U4', '21-U4'), ('06-U7', '22-U8'), ('06-U8', '02-U6')],
    331: [('07-U3', '31-U14'), ('07-U4', '04-U1')],
    350: [('08-U2', '32-U3')],
    369: [('09-U3', '01-U1'), ('09-U11', '22-U1'), ('09-U12', '22-U2'), ('09-U14', '28-U3'), ('09-U15', '30-U1')],
    387: [('10-U5', '27-U8'), ('10-U6', '27-U5'), ('10-U7', '27-U1'), ('10-U11', '27-U14'), ('10-U13', '27-U10')],
    422: [('21-U5', '22-U8')],
    461: [('23-U2', '22-U12'), ('23-U5', '22-U13'), ('23-I2', '29-U14')],
    481: [('24-U6', '32-U2'), ('24-U7', '20-U1')],
    494: [('25-U7', '29-U5'), ('25-U8', '10-U2'), ('25-U9', '10-U1'), ('25-U10', '29-U4')],
    511: [('26-U3', '32-U2'), ('26-U4', '29-U8'), ('26-I4', '26-U5')],
    560: [('29-U1', '08-U1'), ('29-U2', '09-U4'), ('29-U3', '28-U3'), ('29-U9', '25-U1'), ('29-U12', '22-U3'),
          ('29-U15', '22-U20')],
    571: [('30-U3', '22-U20'), ('30-U4', '03-U7'), ('30-U7', '22-U21'), ('30-U8', '09-U9')],
    586: [('31-U1', '24-U1'), ('31-U2', '24-U1'), ('31-U3', '24-U13'), ('31-U4', '32-U1'), ('31-U5', '09-U2'),
          ('31-U7', '25-U1'), ('31-U8', '09-U4'), ('31-U15', '22-U13'), ('31-I1', '22-U15'), ('31-I2', '29-U14'),
          ('31-I4', '09-U2'), ('31-I5', '09-U4')],
}

# Rows of 41 §3 outside this package, with their owner and DECs (from routing.csv notes and the
# wave-2a dec maps); listed in the report only.
OUTSIDE = [
    ('167-170', '§3 heading and column notes', 'no DEC (routed DP-METHOD)'),
    ('171-208', '§3.0 questions resolved while the register was built', 'no DEC (routed DP-METHOD; each resolution lives in its owning document)'),
    ('389-395', '20-U1, 20-U2, 20-U3 (also 24-U7)', 'X-DEC-2a: DEC-203'),
    ('398-401', '20-U8, 20-U10, 20-U11; 20-U12', 'X-DEC-2b: DEC-224 (items 6-8, 12); 20-U12 dropped there as dead content'),
    ('403-406', '20-U14', 'X-DEC-2b: DEC-224 (item 10), DEC-223, DEC-226'),
    ('424-429', '22-U1, 22-U2 (also 00-U7, 09-U11, 09-U12)', 'X-DEC-2c: DEC-248, DEC-249'),
    ('430-433', '22-U3, 22-U4, 22-U5, 22-U6 (also 29-U12)', 'X-DEC-2a: DEC-203, DEC-205, DEC-207'),
    ('434', '22-U7', 'X-DEC-2c: DEC-240'),
    ('435', '22-U8 (also 06-U7, 21-U5)', 'X-DEC-2a: DEC-200 (item 4)'),
    ('436-440', '22-U10, 22-U11, 22-U12, 22-U13, 22-U15 (also 23-U2, 23-U5, 31-U15, 31-I1)', 'X-DEC-2c: DEC-240, DEC-246'),
    ('442-448', '22-U18, 22-U19, 22-U20, 22-U21 (also 29-U15, 30-U3, 30-U7)', 'X-DEC-2a: DEC-205, DEC-206'),
    ('449-453', '23-U1', 'X-DEC-2c: DEC-246'),
    ('456', '23-I1', 'X-DEC-2c: DEC-240'),
]

# ---------------------------------------------------------------------------

FIELDS_OPEN = ['Status', 'Affects', 'Specified behaviour', 'Options', 'Recommendation', 'Decide by']
FIELDS_OTHER = ['Status', 'Affects', 'Specified behaviour', 'Options', 'Recommendation']


def anchor(heading):
    a = heading.strip().lower()
    a = re.sub(r'[^\w\s-]', '', a)
    a = re.sub(r'\s', '-', a)
    return a


def parse_register(text):
    entries = []
    cur = None
    for line in text.split('\n'):
        m = re.match(r'^### (DEC-(\d{3})) (.+)$', line)
        if m:
            cur = {'id': m.group(1), 'num': int(m.group(2)), 'title': m.group(3), 'fields': []}
            entries.append(cur)
            continue
        if line.startswith('## '):
            cur = None
            continue
        if cur is not None:
            fm = re.match(r'^- ([A-Z][a-z ]+): (.*)$', line)
            if fm:
                cur['fields'].append((fm.group(1), fm.group(2)))
            elif line.strip():
                raise SystemExit('unexpected line in %s: %r' % (cur['id'], line))
    return entries


def index_targets(root):
    targets = set()
    with open(os.path.join(root, 'species_work/packages/INDEX.md'), encoding='utf-8') as f:
        for line in f:
            cells = [c.strip() for c in line.split('|')]
            if len(cells) > 6 and ('/' in cells[6]):
                for t in cells[6].split('<br>'):
                    targets.add(t.strip())
    return targets


def target_packages(root):
    pk = {}
    with open(os.path.join(root, 'species_work/packages/INDEX.md'), encoding='utf-8') as f:
        for line in f:
            cells = [c.strip() for c in line.split('|')]
            if len(cells) > 6 and ('/' in cells[6]):
                for t in cells[6].split('<br>'):
                    pk.setdefault(t.strip(), []).append(cells[1])
    return pk


def known_decs(root):
    ids = set()
    for name in ['dec_map_X-DEC-1.csv', 'dec_map_X-DEC-2a.csv', 'dec_map_X-DEC-2b.csv', 'dec_map_X-DEC-2c.csv']:
        with open(os.path.join(root, 'species_work', name), encoding='utf-8') as f:
            for r in csv.DictReader(f):
                ids.add(r['dec_id'])
    return ids


def known_adrs(root):
    ids = set()
    for n in os.listdir(os.path.join(root, 'design_next/adr')):
        m = re.match(r'^(\d{4})-', n)
        if m:
            ids.add('ADR-' + m.group(1))
    return ids


def ranges(lines):
    lines = sorted(set(lines))
    out = []
    for n in lines:
        if out and n == out[-1][1] + 1:
            out[-1][1] = n
        else:
            out.append([n, n])
    return ['%s:%d-%d' % (ORIG41, a, b) for a, b in out]


def main():
    root = sys.argv[1]
    reg_path = os.path.join(root, 'design_next/decisions/open_questions.md')
    with open(reg_path, encoding='utf-8') as f:
        text = f.read()
    entries = parse_register(text)
    errors = []

    # ids
    nums = [e['num'] for e in entries]
    if nums != list(range(300, 300 + len(nums))):
        errors.append('ids are not sequential from 300: %s' % nums)
    if max(nums) > 449:
        errors.append('id out of range')

    targets = index_targets(root)
    decs = known_decs(root) | {e['id'] for e in entries}
    adrs = known_adrs(root)

    for e in entries:
        names = [k for k, _ in e['fields']]
        status = dict(e['fields']).get('Status')
        want = FIELDS_OPEN if status == 'Open' else FIELDS_OTHER
        if status not in ('Open', 'Resolved', 'Dropped'):
            errors.append('%s: bad status %r' % (e['id'], status))
        if names != want:
            errors.append('%s: fields %s, expected %s' % (e['id'], names, want))
        aff = dict(e['fields']).get('Affects', '')
        files, prefixes = [], []
        for part in aff.split(', '):
            m = re.match(r'^(\S+?)(?: \(([A-Z-]+)\))?$', part.strip())
            if not m:
                errors.append('%s: bad Affects part %r' % (e['id'], part))
                continue
            files.append(m.group(1))
            if m.group(2):
                prefixes.append(m.group(2))
            if m.group(1) not in targets:
                errors.append('%s: Affects path not a package target: %s' % (e['id'], m.group(1)))
        e['files'] = files
        e['prefixes'] = list(dict.fromkeys(prefixes)) or ['assets']
        e['status'] = status
        body = ' '.join(v for _, v in e['fields'])
        for ref in re.findall(r'DEC-\d{3}', body):
            if ref not in decs:
                errors.append('%s: unknown reference %s' % (e['id'], ref))
        for ref in re.findall(r'ADR-\d{4}', body):
            if ref not in adrs:
                errors.append('%s: unknown reference %s' % (e['id'], ref))

    # mapping coverage
    ent_ids = {e['id'] for e in entries}
    by_dec = {e['id']: {'lines': [], 'ids': []} for e in entries}
    owner_map = {}
    for line, ids, mapping in ROWS:
        for qid in [x.strip() for x in ids.split('/')]:
            owner_map[qid] = mapping
        if mapping.startswith('DEC-'):
            if mapping not in ent_ids:
                errors.append('row %d maps to missing %s' % (line, mapping))
            else:
                by_dec[mapping]['lines'].append(line)
                by_dec[mapping]['ids'].append(ids)
        else:
            for ref in re.findall(r'(?:DEC-\d{3}|ADR-\d{4})', mapping):
                if ref not in decs and ref not in adrs:
                    errors.append('row %d: unknown covering ref %s' % (line, ref))
    for e in entries:
        if not by_dec[e['id']]['lines']:
            errors.append('%s: no source row' % e['id'])
    # merge pointers add their line to the owner's DEC
    for line, pairs in MERGES.items():
        for merged, owner in pairs:
            m = owner_map.get(owner)
            if m and m.startswith('DEC-'):
                by_dec[m]['lines'].append(line)
                by_dec[m]['ids'].append('%s (merged into %s)' % (merged, owner))

    if errors:
        print('\n'.join(errors))
        raise SystemExit(1)

    # index
    rows = ['| ID | Title | Status | Affects |', '|---|---|---|---|']
    for e in entries:
        heading = '%s %s' % (e['id'], e['title'])
        rows.append('| [%s](#%s) | %s | %s | %s |' % (e['id'], anchor(heading), e['title'], e['status'],
                                                      ', '.join(e['prefixes'])))
    new = re.sub(r'<!-- index:start -->.*?<!-- index:end -->',
                 '<!-- index:start -->\n' + '\n'.join(rows) + '\n<!-- index:end -->', text, flags=re.S)
    if new != text:
        with open(reg_path, 'w', encoding='utf-8') as f:
            f.write(new)

    # csv
    csv_path = os.path.join(root, 'species_work/dec_map_X-DEC-3.csv')
    with open(csv_path, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs', 'orig_ids'])
        for e in entries:
            d = by_dec[e['id']]
            w.writerow([e['id'], e['title'], e['status'], ';'.join(e['prefixes']), ';'.join(e['files']),
                        ';'.join(ranges(d['lines'])), '; '.join(dict.fromkeys(d['ids']))])

    counts = {}
    for e in entries:
        counts[e['status']] = counts.get(e['status'], 0) + 1
    print('entries:', len(entries), counts, 'size:', len(new.encode('utf-8')))

    if len(sys.argv) >= 4:
        with open(sys.argv[2], encoding='utf-8') as f:
            tpl = f.read()
        title = {e['id']: e['title'] for e in entries}
        status = {e['id']: e['status'] for e in entries}

        def show(mapping):
            if mapping.startswith('DEC-'):
                if status[mapping] == 'Dropped':
                    return 'Dropped: %s %s' % (mapping, title[mapping])
                return '%s %s (%s)' % (mapping, title[mapping], status[mapping])
            return 'covered by ' + mapping[len('covered: '):]

        mrows = ['| 41 line | Question ids | Mapped to |', '|---|---|---|']
        all_lines = sorted(set([r[0] for r in ROWS]) | set(MERGES))
        rowmap = {r[0]: r for r in ROWS}
        n_q = 0
        for line in all_lines:
            if line in rowmap:
                _, ids, mapping = rowmap[line]
                mrows.append('| %d | %s | %s |' % (line, ids, show(mapping)))
                n_q += len(ids.split('/'))
            else:
                for merged, owner in MERGES[line]:
                    m = owner_map.get(owner)
                    shown = show(m) if m else 'owner row outside this package (see below)'
                    mrows.append('| %d | %s | merged into %s: %s |' % (line, merged, owner, shown))
        orows = ['| 41 lines | Question ids | Owner and DEC |', '|---|---|---|']
        for rng, ids, own in OUTSIDE:
            orows.append('| %s | %s | %s |' % (rng, ids, own))
        crow = ['| Status | Entries |', '|---|---|']
        for k in ('Open', 'Resolved', 'Dropped'):
            crow.append('| %s | %d |' % (k, counts.get(k, 0)))
        crow.append('| **Total** | **%d** |' % len(entries))
        n_rows = len(ROWS)
        n_cov = sum(1 for r in ROWS if r[2].startswith('covered'))
        n_new = n_rows - n_cov
        crow.append('')
        crow.append('Package question rows: %d (%d question ids including merged ids on the same row); '
                    '%d rows map to an entry of this register, %d rows are covered by an existing DEC or ADR. '
                    'Merge-pointer lines: %d.' % (n_rows, n_q, n_new, n_cov, len(MERGES)))
        erows = ['| DEC | Title | Status | Question ids |', '|---|---|---|---|']
        for e in entries:
            erows.append('| %s | %s | %s | %s |' % (e['id'], e['title'], e['status'],
                                                   '; '.join(dict.fromkeys(by_dec[e['id']]['ids']))))
        pk = target_packages(root)
        crows = ['| DEC | Status | Packages that cite it (target files) |', '|---|---|---|']
        for e in entries:
            if e['status'] == 'Dropped':
                continue
            parts = []
            for fpath in e['files']:
                parts.append('%s (`%s`)' % ('/'.join(pk.get(fpath, ['?'])), fpath))
            crows.append('| %s | %s | %s |' % (e['id'], e['status'], '; '.join(parts)))
        tpl = tpl.replace('{{CITES}}', '\n'.join(crows))
        out = (tpl.replace('{{COUNTS}}', '\n'.join(crow)).replace('{{MAPPING}}', '\n'.join(mrows))
               .replace('{{OUTSIDE}}', '\n'.join(orows)).replace('{{ENTRIES}}', '\n'.join(erows)))
        with open(sys.argv[3], 'w', encoding='utf-8') as f:
            f.write(out)
        print('report written:', sys.argv[3])


if __name__ == '__main__':
    main()
