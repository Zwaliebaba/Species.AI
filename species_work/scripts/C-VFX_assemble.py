"""C-VFX: assemble the animation_and_vfx folder from the scratch parts, fix links and
section references, and renumber requirement ids per file.

Usage: python3 -I C-VFX_assemble.py <PARTS_DIR> <OUT_DIR>
"""
import os
import re
import sys

PARTS, OUT = sys.argv[1], sys.argv[2]
P_LINK = '[procedural animation](explainers_sprites_and_procedural_animation.md)'
Q_LINK = '[particles](particles_shatter_and_screen_effects.md)'


def read(name):
    with open(os.path.join(PARTS, name), encoding='utf-8') as fh:
        return fh.read()


def sub_all(text, pairs, name):
    for old, new in pairs:
        if old not in text:
            raise SystemExit(f'{name}: missing text: {old!r}')
        text = text.replace(old, new)
    return text


def fix_links(text):
    pairs = [('](renderer/', '](../renderer/'), ('](../../reference/', '](../../../reference/'),
             ('](../../adr/', '](../../../adr/'), ('](camera_and_sequences.md', '](../camera_and_sequences.md'),
             ('](ui_framework/', '](../ui_framework/'), ('](audio_engine.md', '](../audio_engine.md'),
             ('](platform_layer.md', '](../platform_layer.md'),
             ('](asset_pipeline_and_resources.md', '](../asset_pipeline_and_resources.md'),
             ('](data_schemas.md', '](../data_schemas.md'), ('](terrain.md', '](../terrain.md')]
    for old, new in pairs:
        text = text.replace(old, new)
    return text


# ---------- README parts ----------
r_s1 = fix_links(read('r_s1.md'))
r_s1 = sub_all(r_s1, [
    ('(§4.4 to §4.13)', f'({P_LINK} §4.2 to §4.11)'),
    ('(`ParticlePool`, §4.14)', f'(`ParticlePool`, {Q_LINK} §4.1 to §4.5)'),
    ('(`ShatterEffects`, §4.17)', f'(`ShatterEffects`, {Q_LINK} §4.10)'),
    ('this component draw (§4.16);', f'this component draw ({Q_LINK} §4.9);'),
    ('explainer animations** (§4.3)', f'explainer animations** ({P_LINK} §4.1)'),
    ('full-screen flashes (§4.18).', f'full-screen flashes ({Q_LINK} §4.11).'),
    ('the catalogue entries of §3.9.', 'the catalogue entries of §3.5.'),
    ('the shake rotation offset of the frame (§4.18)', 'the shake rotation offset of the frame (VFX-Q28)'),
    ('the explainer timeline player (§4.3)', 'the explainer timeline player (VFX-027)'),
    ('(DEC-007, §4.10)', '(DEC-007, VFX-P01)'),
    ('this component (§4.9)', 'this component (VFX-049)'),
    ('the last column names where each is specified.',
     'the last column names where each is specified. In that column, P is '
     '[explainers_sprites_and_procedural_animation.md](explainers_sprites_and_procedural_animation.md) and Q is '
     '[particles_shatter_and_screen_effects.md](particles_shatter_and_screen_effects.md).'),
    ('the effect events and presentation emitters of §4.15 | §4.14, §4.15 |',
     'the effect events and presentation emitters of Q §4.7 and §4.8 | Q §4.1 to §4.8 |'),
    ('the shatter events of §4.17 | §4.17 |', 'the shatter sources of Q table 4.10-A | Q §4.10 |'),
    ('cursor_and_markers.md); §4.15 |', 'cursor_and_markers.md); Q §4.8 |'),
], 'r_s1')
inv_start = r_s1.index('### 1.4 Effect and animation inventory')
head, inv = r_s1[:inv_start], r_s1[inv_start:]
MAP = {'3': 'P §4.1', '5': 'P §4.3', '6': 'P §4.4', '9': 'P §4.7', '10': 'P §4.8', '11': 'P §4.9',
       '12': 'P §4.10', '13': 'P §4.11', '15': 'Q §4.7', '16': 'Q §4.9', '18': 'Q §4.11'}
inv = re.sub(r'(?<![PQ] )§4\.(\d+)\b', lambda m: MAP.get(m.group(1), m.group(0)), inv)
r_s1 = head + inv

r_s2 = fix_links(read('r_s2.md'))
r_s2 = sub_all(r_s2, [
    ('// closed enumeration, table 4.15-A', '// closed enumeration, particles table 4.7-A'),
    ('// EffectKind::Shatter (§4.17)', '// EffectKind::Shatter (particles §4.10)'),
    ('// kind-specific scalars of table 4.15-A', '// kind-specific scalars of particles table 4.7-A'),
    ('// table 4.14-A, in this order', '// particles table 4.2-A, in this order'),
    ('// VFX-012', '// VFX-021'),
    ("(VFX-047)", '(VFX-013)'),
    ('(table 4.15-A)', f'({Q_LINK} table 4.7-A)'),
], 'r_s2')

r_s3a = fix_links(read('r_s3_clock_rng_ease.md'))
r_s3a = sub_all(r_s3a, [
    ('(§4.15.3)', f'({Q_LINK} §4.8)'),
    ('the particle sub-step of VFX-045', 'the particle sub-step of VFX-Q05'),
    ('the explainer-free effect lifetimes that the simulation does not own',
     'the lifetimes of presentation-only effects'),
], 'r_s3a')
r_s3b = fix_links(read('r_s3_defs.md'))
r_s3b = sub_all(r_s3b, [
    ('### 3.5 Effect definitions', '### 3.4 Effect definitions'),
    ('table 4.15-A', f'{Q_LINK} table 4.7-A'),
    ('the reference values of §4.14 and §4.15', f'the reference values of {Q_LINK} §4.2 and §4.7'),
], 'r_s3b')
r_s3c = fix_links(read('r_s3_catalogue_lifetimes.md'))
r_s3c = sub_all(r_s3c, [
    ('### 3.9 Catalogue entries owned by this component', '### 3.5 Catalogue entries owned by this component'),
    ('### 3.10 Lifetimes', '### 3.6 Lifetimes'),
], 'r_s3c')
r_s4 = fix_links(read('r_s4_clock_events.md'))
r_s4 = sub_all(r_s4, [
    ("emission steps (§4.15.3);", f"emission steps ({Q_LINK} §4.8);"),
    ('(§4.9 to §4.13);', f'({P_LINK} §4.7 to §4.11);'),
    ('compute work (§4.14);', f'compute work ({Q_LINK} §4.1 to §4.5);'),
    ('explosions (§4.17);', f'explosions ({Q_LINK} §4.10);'),
    ('pass 10a (§4.18);', f'pass 10a ({Q_LINK} §4.11);'),
    ('ADR-0036, §4.14.4)', f'ADR-0036, {Q_LINK} §4.4)'),
], 'r_s4')

readme = (read('r_head.md') + r_s1 + '\n' + r_s2 + '\n## 3 Data model\n\n' + r_s3a + r_s3b + r_s3c +
          '\n## 4 Behaviour and algorithms\n\n' + r_s4 + read('r_tail.md'))

# ---------- procedural document ----------
p_s3 = fix_links(read('p_s3_state_explainer.md'))
p_s3 = sub_all(p_s3, [
    ('### 3.7 Procedural state', '### 3.1 Procedural state'),
    ('### 3.8 Explainer animation data', '### 3.2 Explainer animation data'),
    ('the smoothed velocity of VFX-073', 'the smoothed velocity of VFX-P11'),
], 'p_s3')
p_s4a = fix_links(read('p_s4_a.md'))
p_s4a = sub_all(p_s4a, [
    ('### 4.3 Explainer animations', '### 4.1 Explainer animations'),
    ('### 4.4 Sprite and billboard conventions', '### 4.2 Sprite and billboard conventions'),
    ('### 4.5 Worker cards and overlays', '### 4.3 Worker cards and overlays'),
    ('### 4.6 Seasonal theme', '### 4.4 Seasonal theme'),
    ('### 4.7 Team colour in effects', '### 4.5 Team colour in effects'),
    ('### 4.8 Other billboards and sprites', '### 4.6 Other billboards and sprites'),
    ('### 4.9 Damage flicker', '### 4.7 Damage flicker'),
    ('`sprite.worker.hurt` (§4.5).', '`sprite.worker.hurt` (§4.3).'),
    ('R-CRATE-500, §4.16 |', f'R-CRATE-500, {Q_LINK} VFX-Q20 |'),
    ('mesh it came from (§4.17) |', f'mesh it came from ({Q_LINK} §4.10) |'),
    ('cloth flags (§4.12) |', 'cloth flags (§4.10) |'),
], 'p_s4a')
p_s4a = p_s4a.replace('the saturated team colour (§4.7)', 'the saturated team colour (§4.5)')
p_s4a = p_s4a.replace('## 4 Behaviour and algorithms\n', '')
procedural = (read('p_head.md') + p_s3 + '\n## 4 Behaviour and algorithms\n\n' + p_s4a.lstrip('\n') +
              read('p_s4_b.md') + read('p_tail.md'))

# ---------- particles document ----------
q_s3a = fix_links(read('q_s3_particle.md'))
q_s3a = sub_all(q_s3a, [
    ('### 3.4 Particle record and particle type fields', '### 3.1 Particle record and particle type fields'),
    ('a cap (§4.14.4)', 'a cap (§4.4)'),
    ('the fade of VFX-049', 'the fade of VFX-Q11'),
], 'q_s3a')
q_s3b = fix_links(read('q_s3_shatter.md'))
q_s3b = sub_all(q_s3b, [('### 3.6 Shatter records', '### 3.2 Shatter records')], 'q_s3b')
q_tail = sub_all(read('q_tail.md'), [], 'q_tail')
q_head = sub_all(read('q_head.md'), [
    ('receive the shake requests of table 4.11-A', 'receive the shake requests of VFX-Q29'),
], 'q_head')
particles = q_head + q_s3a + q_s3b + read('q_s4_a.md') + read('q_s4_b.md') + q_tail

# ---------- renumber requirement ids ----------
ID = re.compile(r'VFX-(?:\d{3}|[PQR][0-9A-Z]{2})')
docs = [('README.md', readme, 1), ('explainers_sprites_and_procedural_animation.md', procedural, 100),
        ('particles_shatter_and_screen_effects.md', particles, 200)]
mapping = {}
for name, text, base in docs:
    n = base
    for m in re.finditer(r'\*\*(' + ID.pattern + r')\*\*', text):
        old = m.group(1)
        if old in mapping:
            raise SystemExit(f'{old} defined twice')
        mapping[old] = f'VFX-{n:03d}'
        n += 1
os.makedirs(OUT, exist_ok=True)
RANGES = ['VFX-001 to VFX-099', 'VFX-100 to VFX-199', 'VFX-200 to VFX-299']
for name, text, base in docs:
    for i, r in enumerate(RANGES):
        text = text.replace(r, f'@@RANGE{i}@@')
    unknown = sorted({t for t in ID.findall(text) if t not in mapping})
    if unknown:
        raise SystemExit(f'{name}: undefined ids {unknown}')
    text = ID.sub(lambda m: mapping[m.group(0)], text)
    for i, r in enumerate(RANGES):
        text = text.replace(f'@@RANGE{i}@@', r)
    with open(os.path.join(OUT, name), 'w', encoding='utf-8') as fh:
        fh.write(text)
    print(name, len(text.encode('utf-8')), 'bytes')
with open(os.path.join(PARTS, 'id_mapping.tsv'), 'w', encoding='utf-8') as fh:
    for k, v in mapping.items():
        fh.write(f'{k}\t{v}\n')
print('ids:', len(mapping))
