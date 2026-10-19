---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Accessibility baseline: Xbox Accessibility Guidelines 3.2 as the checklist, with engine features built in from the start

## Context and Problem Statement

Species' look is additive glow on black with a darkening layer and crowds of small figures (vision O9). That style raises specific accessibility risks: low-contrast text over glow, flashing effects, team identity carried by colour alone, and dense camera motion. Several mitigations need engine support that is expensive to add late: text scaling in the glyph cache, palette switching, full input remapping, and accessible names on widgets. The engine needs a recognised checklist to design against and test with.

Which accessibility guideline does Species adopt, and which features does the engine provide from the start?

## Decision Drivers

* A published, maintained checklist written for games.
* Engine features that are costly to retrofit are built from the start.
* Measurable criteria that tests can check.
* Design choices (difficulty options, narration scope) stay with the design phase.

## Considered Options

* Xbox Accessibility Guidelines (XAG) 3.2 as the checklist
* Web Content Accessibility Guidelines as the checklist
* No formal checklist; features added on request

## Decision Outcome

Proposed option: "Xbox Accessibility Guidelines (XAG) 3.2 as the checklist". **Default while Proposed:** the team proceeds with it, and the engine features below are built in from the start. **Confirmed when:** the per-guideline conformance table exists and the contrast, text-size and flash tests of the Confirmation section pass on the screens and combat scenes of the first playable (O20); the design phase's acceptance tests then record conformance per guideline. XAG 3.2 (published 2023-06-08) is the current version; Microsoft describes the guidelines as best practices, not a compliance checklist, and Species uses them as its design and test checklist.

Engine features provided from the start:

* **Text scale** setting that feeds the glyph cache and UI layout (XAG 101); minimum default text size of 9 px at 1080p and 19 px at 4K for icons and glyph text. For a client height *H* in pixels, the minimum is 9 px × *H* ÷ 1080 up to 1080, linear from 9 px at 1080 to 19 px at 2160, and 19 px × *H* ÷ 2160 above 2160.
* **Contrast:** gameplay-relevant text at 4.5:1 or more, inactive elements at 3:1, and a high-contrast mode at 7:1 (XAG 102). UI palettes are data, so the high-contrast palette is a content variant.
* **Identity beyond colour:** team and faction identity always has a second channel, shape or pattern, as well as colour (XAG 103).
* **Input:** full remapping, hold-or-toggle options for held actions, and remapping done once in the input abstraction for every device (XAG 107).
* **UI navigation and focus:** every screen is fully usable with keyboard alone and with gamepad alone through the UI focus graph (XAG 112, 113).
* **Photosensitivity:** a build-time and capture-time check of glow and flash effects against the XAG 118 flash criterion (a 10 % change in luminance counts as a flash), and a setting that reduces flash intensity.
* **Motion:** a setting that reduces camera shake and screen motion (XAG 117).
* **Captions:** important audio cues have a visual indicator (XAG 103, 104).
* **Accessible names:** every widget carries an accessible-name string id, so screen narration (XAG 106) can be added; whether menus are narrated in v1 is a design-phase decision.

### Consequences

* Good, because the costly-to-retrofit features exist before content is built on top of them.
* Good, because the measurable criteria become automated tests.
* Bad, because the high-contrast palette and the reduced-flash setting add content variants that art must author and test.

### Confirmation

* A per-guideline conformance table is kept by the UI framework component and reviewed at each milestone.
* Automated tests check contrast ratios of every UI palette entry against its role, and the minimum text size at 1080p and 4K.
* A capture-based test measures luminance change per frame in recorded combat scenes and fails above the flash criterion.

## Pros and Cons of the Options

### XAG 3.2

* Good, because it is written for games, covers input, audio, motion and photosensitivity, and gives measurable criteria.
* Neutral, because it is best practice, not a legal standard; legal requirements per market are checked separately before release.

### Web Content Accessibility Guidelines

* Good, because they are widely referenced.
* Bad, because they target web content and do not cover game input, camera motion or gameplay feedback.

### No formal checklist

* Bad, because features would be added late, at high cost, and without a test basis.

## More Information

* Facts and sources: technology baseline item 10.1 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the XAG overview, the XAG version history and the accessibility feature-tag pages on learn.microsoft.com.
* Related: [ADR-063](ADR-063-localisation.md) (text and fonts).
* Components: UI framework (`UI`), input (`INP`), renderer (`RND`), audio engine (`AEN`), animation and VFX (`VFX`, the flash and motion rules).
* **Amendment 2026-10-17.** The minimum text size is defined for every client height by the rule of the UI framework (UI-171, `ui.access.min_text_1080p`, `ui.access.min_text_2160p`), which interpolates between the two XAG reference points.
* **Amendment 2026-10-18 (owner decision O55).** Since O55 the first playable of the "Confirmed when" line is the MVP's M3 build ([MVP scope](../implementation/mvp_scope.md) §3, §5.3), not the skirmish of O20, and the evidence is gathered on that build: the conformance table and the contrast, text-size and flash tests cover its front-end page, its HUD and recorded battles of that build in place of the combat scenes of a match.
