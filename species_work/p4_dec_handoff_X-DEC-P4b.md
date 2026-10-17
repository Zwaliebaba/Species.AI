# X-DEC-P4b hand-off to X-DEC-P4a

Cross-file amendments that X-DEC-P4b needs in decision files owned by X-DEC-P4a. X-DEC-P4a has finished, so the coordinator applies them. Each item gives the entry, the field, the exact text to replace (copied from the file as it stood at 2026-10-17, after X-DEC-P4a's own edits to DEC-011, DEC-024, DEC-036, DEC-102, DEC-116 and DEC-567) and the exact new text. Byte deltas are UTF-8 bytes of new minus old text.

**Size budget of `verdicts_and_fixes.md`.** Items H1 to H4 change that file by −115 + 25 = **−90 bytes in total** (H1 +10, H2 −72, H3 −53, H4 +25). Applying all four keeps the file smaller than it is now; if only some are applied, apply H2 or H3 together with H1 and H4.

Earlier triage items whose amendment X-DEC-P4a has already made in its own files, so no hand-off is given for them (H3 below is a different change, the Affects line of DEC-102 only): item 14 (DEC-011, stale Source Node id), item 12 (DEC-116 and DEC-036, start hold), item 93 (DEC-102, percent sign), item 98 (DEC-102, colour-row availability), item 101 (DEC-024, timeline-only operations and the fly mode) and item 89 (DEC-567, DSP defaults).

## H1 DEC-016 Tie-breaks are explicit ordered rules (`verdicts_and_fixes.md`) — items 9 and 16, DEC-260

Field: Specified behaviour. Two replacements (delta +13 and −3 bytes).

1. Replace:
   > whose stable key is cell x, then cell z, then team 0 to 5, then cell slot;

   with:
   > whose stable key is cell x, then cell z, then team 0 to 5, then actor serial (DEC-260);

2. Replace:
   > Simulation containers iterate in stable-id order, never in slot-reuse or hash order.

   with:
   > Containers iterate in their documented slot order, never in hash order (DEC-260).

Reason: DEC-260 (Resolved) reconciles DEC-016 with DEC-650 and DEC-455. The order within an actor-grid cell is history-free (ascending actor serial), and containers iterate in the slot order of the documented allocation rules, whose same-tick consequences stay open in DEC-455.

## H2 DEC-066 Multi-round Siege (`verdicts_and_fixes.md`) — item 15, DEC-261

Three replacements (delta +81, −97 and −56 bytes; −72 in total).

1. Field Affects. Replace:
   > - Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE)

   with:
   > - Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-029)

2. Field Specified behaviour. Replace:
   > The reference round reset leaves the object id seed unchanged and runs after each process has advanced a variable number of update slices from the round-end flag, which consume simulation draws.

   with:
   > The round reset runs at a tick boundary (DEC-261) and leaves the object serial counter unchanged.

3. Field Options. Replace:
   > B — expose it in the lobby, with the round reset performed at a tick boundary as a sequenced simulation event that also resets the object id seed.

   with:
   > B — expose it in the lobby, with a round reset that also resets the object serial counter.

Reason: reference/12 R-SIM-030 fixes the rebuild at a tick boundary, which DEC-261 (Resolved) records; DEC-066 keeps the selectability question and the serial-counter reset. The Recommendation sentence "If B is chosen, the round reset is never applied outside a tick." stays.

## H3 DEC-102 UI framework corrections by construction (`verdicts_and_fixes.md`) — C-UI-b cross-document request 6

Field Affects (delta −53 bytes). Replace:
> - Affects: architecture/components/ui_framework/README.md (UI), architecture/components/ui_framework/core_and_widgets.md (UI), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI)

with:
> - Affects: architecture/components/ui_framework/ (UI: README.md, core_and_widgets.md, layout_rendering_input_and_localisation.md, pages_and_screen_recipes.md)

Reason: the defect suite DT-01 to DT-33 that DEC-102 covers lives in `pages_and_screen_recipes.md` (DT-18 colour rows, DT-19 Back on every page, DT-20 list scrolling). C-UI-b requests 2, 3 and 4 (Back on an open drop-down list, session-list edge scrolling, colour-row availability) are therefore merged into DEC-102.

## H4 DEC-039 Speed crates are tick scheduling (`verdicts_and_fixes.md`) — item 11, DEC-265

Field Specified behaviour (delta +25 bytes). Replace:
> Time Dilation runs the game at 0.2–0.4× and Time Rush at 1.3–2.0× for 15 s,

with:
> Time Dilation runs the game at 0.2–0.4× and Time Rush at 1.3–2.0× for a hold of 150 ticks (DEC-265),

Reason: under tick scheduling the hold counts world ticks (DEC-092, reference/12 R-SIM-557), so "15 s" is no longer the real duration at every speed; whether the hold should count real time again is the Open DEC-265.

## H5 DEC-455 Same-tick advance of objects created or handed over during a tick (`units_buildings_crates_economy.md`) — item 16, DEC-260

Field Options (delta +45 bytes; the file has room). Replace:
> B — an object created or handed over during a tick first advances in the next tick, and containers iterate in stable-id order (DEC-016);

with:
> B — an object created or handed over during a tick first advances in the next tick, and containers iterate in an order that does not depend on allocation history (DEC-016, DEC-260);

Reason: DEC-260 defines the stable key of container iteration as the documented slot order; option B's "stable-id order" now means an order that is also independent of allocation history.

## H6 DEC-554 Input devices, key names and binding drivers (`controls_bots_maps_meta_audio.md`) — C-INP request 1, DEC-953

Field Specified behaviour, item 5 (delta +59 bytes; the file has room). Replace:
> 5. each local player has an assigned gamepad, and a gamepad drives only the player it is assigned to;

with:
> 5. each local player has an assigned gamepad, and a gamepad drives only the player it is assigned to; a gamepad is assigned by its first button press (DEC-953);

Reason: DEC-953 (Resolved) records the assignment rule of input.md INP-145 to INP-149.

## Overlaps with X-DEC-P4a entries (no change requested)

- DEC-856 item 4 (one smoothed tail velocity shared by every Brood Drifter) covers C-VFX request 9's first quirk; DEC-963 cites DEC-856 instead of repeating it.
- DEC-865 item 4 (maps that are not square) bounds DEC-966 item 2 (the highest-sample scan on a non-square grid): under DEC-865 option B the case never arises. DEC-966 cites DEC-865.

## Applied from X-DEC-P4a's hand-off

X-DEC-P4b applied all four items of `p4_dec_handoff_X-DEC-P4a.md` to its files: H-1 (DEC-653 item 7, replacing X-DEC-P4b's own earlier rewording of that item), H-2 (DEC-652, forced crate outcome), H-3 (DEC-653 items 5 and 6) and H-4 (DEC-229 Owner decision line for O20).
