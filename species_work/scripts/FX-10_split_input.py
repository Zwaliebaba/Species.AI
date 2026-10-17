"""FX-10: split input.md into the folder input/ (Phase 4, wave B).

Usage: python3 -I FX-10_split_input.py <components dir>

Moves the text of input.md verbatim into <components>/input/ (README.md and three sub-topic files),
re-basing every relative link for the deeper folder. Every requirement id, key, heading and anchor
text is kept. Writes the four files only; deletes nothing.
"""
import os
import re
import sys

COMP = os.path.abspath(sys.argv[1])
SRC = os.path.join(COMP, 'input.md')
OUT = os.path.join(COMP, 'input')
L = open(SRC, encoding='utf-8').read().split('\n')
LINK = re.compile(r'(?<!!)(\[(?:[^\]\[]|\[[^\]]*\])*\]\()([^)\s]+)(\))')


def rebase(text):
    def fix(m):
        t = m.group(2)
        if re.match(r'^[a-z][a-z0-9+.-]*:', t, re.I) or t.startswith('#'):
            return m.group(0)
        path, sep, anchor = t.partition('#')
        dest = os.path.normpath(os.path.join(COMP, path))
        new = os.path.relpath(dest, OUT).replace(os.sep, '/')
        return m.group(1) + new + (sep + anchor if sep else '') + m.group(3)
    return LINK.sub(fix, text)


L = [rebase(x) for x in L]


def find(prefix):
    hits = [i for i, x in enumerate(L) if x.startswith(prefix)]
    if len(hits) != 1:
        raise SystemExit(f'{len(hits)} hits: {prefix}')
    return hits[0]


H = {k: find(v) for k, v in {
    's1': '## 1 Responsibilities', 's2': '## 2 Interfaces', 's3': '## 3 Data model',
    's31': '### 3.1 ', 's32': '### 3.2 ', 's33': '### 3.3 ', 's36': '### 3.6 ', 's39': '### 3.9 ',
    's4': '## 4 Behaviour', 's41': '### 4.1 ', 's413': '### 4.13 ', 's416': '### 4.16 ', 's417': '### 4.17 ',
    's418': '### 4.18 ', 's5': '## 5 ', 's8': '## 8 '}.items()}
N = len(L)


def rng(a, b):
    """Lines [a, b) of the re-based source, trailing blank lines dropped."""
    block = L[a:b]
    while block and block[-1] == '':
        block.pop()
    return '\n'.join(block)


status = L[2]
assert status.startswith('> Status:')
intro = L[4]
notation = rng(6, H['s1'])

CTRL = 'controls_and_default_bindings.md'
DEV = 'devices_frames_and_queries.md'
LAY = 'binding_layers_and_rebinding.md'

index = f"""## Folder index

| File | Holds | Requirement IDs |
|---|---|---|
| [README.md](README.md) (this file) | responsibilities and boundaries; the interface sketch, the router operations, the contracts and the helpers; value kinds and conditions, binding nodes, action bindings, the binding-set schema, action values and suppression and the class diagram; the tuning table; budgets, failure modes, acceptance tests and open decisions | INP-001 to INP-053, INP-060 to INP-073, INP-090 to INP-093, INP-200 to INP-244 |
| [{CTRL}]({CTRL}) | §3.2, §3.6 to §3.8, §3.11 and §4.16: the name table of every bindable control, the default binding set, the glyphs, the reverse index, the development binding layer and the conditional actions, and the notes on the default scheme | INP-055, INP-056, INP-075 to INP-085, INP-094 to INP-097, INP-181 to INP-185 |
| [{DEV}]({DEV}) | §4.1 to §4.12: the lifecycle and the frame order, the device back-ends, the event queue, the device state, condition evaluation, analogue processing, repeat timers, composite and environment nodes, query evaluation, local players and gamepad assignment, the input mode and idle time, and the pointer | INP-100 to INP-160 |
| [{LAY}]({LAY}) | §4.13 to §4.15 and §4.17: the binding layers, keyboard layouts and locales, loading and validation, the rebinding hook services and the binding graph | INP-162 to INP-179 |

Section numbers are shared by the four files of this folder, so a reference such as §4.7 or table 3.6-B names one section or table of the folder."""

readme = '\n'.join([
    '# Input', '', status, '', intro, '', index, '', notation, '',
    rng(H['s1'], H['s32']), '',
    f'§3.2 is in [{CTRL}]({CTRL}).', '',
    rng(H['s33'], H['s36']), '',
    f'§3.6 to §3.8 and §3.11 are in [{CTRL}]({CTRL}).', '',
    rng(H['s39'], H['s4']), '',
    '## 4 Behaviour and algorithms', '',
    f'§4.1 to §4.12 are in [{DEV}]({DEV}), §4.13 to §4.15 and §4.17 in [{LAY}]({LAY}), and §4.16 in [{CTRL}]({CTRL}).', '',
    rng(H['s418'], N), ''])


def sub(title, scope, sections):
    rel = (f'> Status: Engine baseline · Prefix: INP · ADRs: [ADR-028](../../../ADR/ADR-028-input-device-back-ends.md) · '
           f'[ADR-029](../../../ADR/ADR-029-input-event-model-and-binding-contract.md) · '
           f'[ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · Folder: [Input](README.md)')
    pointer = ('The responsibilities, interfaces, budgets, failure modes, acceptance tests and open decisions of the input '
               'component are those of the [README](README.md); this document adds none of its own.')
    parts = [f'# {title}', '', rel, '', scope + ' Section numbers are shared by the files of this folder, and the notation is that of the [README](README.md).', '',
             '## 1 Responsibilities and boundaries', '', 'The responsibilities and boundaries of the input component are [README](README.md) §1 (INP-001 to INP-010).', '',
             '## 2 Interfaces', '', 'The interface sketch and the router operations are [README](README.md) §2.', '']
    parts += sections
    parts += ['## 5 Concurrency and performance budget', '', 'The budget is [README](README.md) §5.', '',
              '## 6 Failure modes, diagnostics and telemetry', '', 'Failure modes and diagnostics are [README](README.md) §6.', '',
              '## 7 Acceptance tests', '', 'The acceptance tests are [README](README.md) §7.', '',
              '## 8 Open decisions', '', 'The open decisions and design-phase inputs are [README](README.md) §8.', '']
    return '\n'.join(parts)


ctrl = sub('Input: controls and default bindings',
           'This document specifies the bindable controls and the default bindings of the input component: the name table of every key, mouse control and gamepad control, the default binding set of the match and UI navigation actions, the glyphs, the reverse index, the development binding layer and the conditional actions, and the notes on the default scheme.',
           ['## 3 Data model', '', rng(H['s32'], H['s33']), '', rng(H['s36'], H['s39']), '',
            '## 4 Behaviour and algorithms', '', rng(H['s416'], H['s417']), ''])
dev = sub('Input: devices, frames and queries',
          'This document specifies how the input router builds the state of each frame and answers queries from it: the lifecycle and the frame order, the device back-ends, the event queue, the device state, the evaluation of conditions, analogue processing, repeat timers, composite and environment nodes, query evaluation, local players and gamepad assignment, the input mode and idle time, and the pointer.',
          ['## 3 Data model', '', 'The data model is [README](README.md) §3 and, for the controls and the default bindings, [' + CTRL + '](' + CTRL + ').', '',
           '## 4 Behaviour and algorithms', '', rng(H['s41'], H['s413']), ''])
lay = sub('Input: binding layers and rebinding',
          'This document specifies how the input router builds its action bindings from their layers: the binding layers, keyboard layouts and locales, loading and validation, the services of the rebinding hook, and the binding graph.',
          ['## 3 Data model', '', 'The binding-set schema is [README](README.md) §3.5; the default binding set is [' + CTRL + '](' + CTRL + ') §3.6.', '',
           '## 4 Behaviour and algorithms', '', rng(H['s413'], H['s416']), '', rng(H['s417'], H['s418']), ''])

os.makedirs(OUT, exist_ok=True)
for name, text in [('README.md', readme), (CTRL, ctrl), (DEV, dev), (LAY, lay)]:
    with open(os.path.join(OUT, name), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(text)
    print(name, len(text.encode('utf-8')))

# verification: every non-blank re-based source line occurs in exactly one output file, except the
# lines the split replaces (the title is rewritten in place)
outs = [readme, ctrl, dev, lay]
missing = [x for x in L if x.strip() and not any(x in o.split('\n') for o in outs)]
print('missing source lines:', len(missing))
for x in missing[:10]:
    print('  ', x[:120])
