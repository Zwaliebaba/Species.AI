---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (owner decision O106)
---

# Distinct stand-ins for the HUD's icons

## Context and Problem Statement

The content build makes a stand-in for each texture row of the MVP that has no authored file (AST-082, [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md)). Every `generated glyph` row gets the same shape: a white ring round a dot.

Most of the HUD's glyphs have no file yet:
- the 17 files of the `icon.hud.*` rows from `icon.hud.resource` to `icon.hud.deputy`, `icon.hud.priority` having three;
- `icon.power.gun_turret`;
- `cursor.pointer`, whose art the priority arrows draw (O99);
- `cursor.place`.

So in the owner's run the stores, the growth, the three priorities with their arrows, the construction bar, the relations panel's raid and claim buttons and the notification cards all showed one identical mark. The owner found nothing that read as a button and could not tell what the game let them do. GameAssets.md §2.6 asks for a white geometric glyph per row (circle, arrow or ring), which ADR-140 narrowed to the one shape.

## Decision Drivers

* Until the owner draws them, each icon must be told apart, and a button must read as one.
* The content build stays deterministic: two builds write the same bytes (`Build\CheckContentBuild.py`).
* An authored file still replaces its stand-in, with nothing else to change.

## Considered Options

1. **A simple shape per file.** The build draws each of these rows' own white shape, chosen by its file's name.
2. **The asset id's initials** on the ring, as §2.6 also suggests. The build has no font of its own, and DirectWrite is only linked by `FontCoverage` (ADR-147).
3. **No change**, until the owner draws the icons.

## Decision Outcome

Chosen option: 1 (O106).
- Option 2 brings a text rasterizer into the content build for marks that are thrown away.
- Option 3 leaves the HUD unreadable until the art exists.

What is built:
* `Tools/SpeciesContentBuild/PlaceholderTexture` keeps a table of the files' stems and their shapes (`GLYPH_STAND_INS`). Each shape is a coverage function of the unit square, built from boxes, rings, triangles, thick segments and a star, and sampled as every stand-in is.
* The shapes are:

| Stem | Shape |
|---|---|
| `Pointer` | an arrow pointing down, which the HUD flips for up |
| `Place` | four corners round a dot |
| `Resource` | a crystal |
| `Growth` | a plus in a ring |
| `PriorityGather` | a basket |
| `PriorityConstruct` | a hammer |
| `PriorityGuard` | a shield |
| `Dwelling` | a house |
| `Storehouse` | a crate |
| `GunTurret` | a base and a barrel |
| `Contact` | two rings that meet |
| `Claim` | a flag |
| `Attack` | crossed blades |
| `Battle` | crossed blades in a ring |
| `Tribute` | a stack of coins |
| `Withdraw` | an arrow back to the left |
| `Respite` | a pause |
| `Renown` | a star |
| `RunClock` | a clock |
| `Deputy` | two rank chevrons |

* A cursor's derived shadow (`<stem>Shadow`, `derived by the cooker`) blurs its base's shape.
* Any other glyph keeps ADR-140's ring round a dot.

### Consequences

* Good, because the HUD reads before its art exists, and its buttons look like buttons.
* Bad, because a placeholder can be mistaken for the look. The cooked entries stay flagged as placeholders (AST-083), and the likeness review judges the owner's art, not these.
* It forecloses nothing: each authored file replaces its stand-in.

### Confirmation

* `Build\CheckContentBuild.py`: every stand-in is still at its key in its row's format, and two builds are identical.
* A contact sheet of the 20 shapes, sampled by the build's own coverage code, was reviewed when this was written.
* The owner's run: each HUD icon and arrow tells apart.

## Pros and Cons of the Options

### 1 — a shape per file

* Good, because it is small and deterministic.
* Bad, because the table must grow with each new glyph row the HUD draws.

### 2 — initials

* Good, because every row is covered without a table.
* Bad, because it needs a text rasterizer in the build.

### 3 — no change

* Good, because nothing is built.
* Bad, because the HUD stays unreadable.

## More Information

* Owner decisions O64, O88, O99, O106; AST-082, AST-083; GameAssets.md §2.6.
* Amends [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md) (a glyph's stand-in).
* The code: `Tools/SpeciesContentBuild/PlaceholderTexture`.
