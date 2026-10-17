# C-UI-b report: pages and screen recipes

## Files written

| File | Size (bytes) | Content |
|---|---|---|
| `design_next/architecture/components/ui_framework/pages_and_screen_recipes.md` | 112 211 | component specification (template §6.4, plus a §9 tuning table as in the renderer files): page ids and page catalogue, page host and page stack, page state machine and per-frame order, applying a navigation request, widget registration, navigation on pages, rendering conventions, scrolling text, image widgets, colour and side picker, list and other front-end widgets, recipes A (tool window), B (options screen), C (front-end page) and D (HUD panel), page-level budget, failure modes, tests and the defect regression suite |
| `species_work/scripts/C-UI-b_check.py` | helper | self-check: ids, keys, sibling ids and keys, foreign keys, DEC/ADR/asset ids, `decs_to_cite`, `assets_for_you`, headings, truncation words, links, Mermaid captions, size |

One file, under 120 KB (112.2 KB), so no split. Headroom is about 7 KB; a larger addition should split the defect suite (§7.2) into its own file.

## Counts

- **Requirements:** 109, UI-300 to UI-490, unique and ascending, all in the range 300–499 (107 MUST, 1 MUST NOT, 1 SHOULD).
- **Tuning keys:** 13 owned keys (`ui.pages.*` 2, `ui.scroll_text.*` 7, `ui.picker.*` 1, `ui.map_row.*` 2, `ui.chat_log.*` 1); no key prefix of mine is used anywhere else in `design_next/`, and none collides with a key of the three C-UI-a files. About 45 foreign keys are cited and not redefined (§9.2 lists their owners): `ref.meta.*` of reference/09, `ref.match.*`, `ref.sim.team.*`, `rnd.transition.curve_gain`, `rnd.budget.target_frame_rate`, and the palette tokens `ui.palette.*` of `layout_rendering_input_and_localisation.md`. Every cited key exists.
- **Diagrams:** 4 Mermaid (page state machine, page graph with stack operations, page-change sequence with the transition, scrolling-text cycle), each with a caption; all four parse with mermaid 11.4.1, whose parser rejects broken samples.
- **Enumerations:** 14 tables (mechanisms, events, page catalogue with 19 page ids, stack operations, widget kinds with 22 kinds, leave hooks, animations, scrolling-text layout, failures, tests, defect suite, decisions, owned keys, cited keys) and 11 numbered lists (kept patterns, per-frame order, apply steps, enter outcomes, registration groups, picture kinds, availability rule, recipes A, B and C).
- **Acceptance tests:** 37 (T1–T37: unit, vector, property, golden-image, integration, performance) and the 33 defect tests DT-01 to DT-33 (one per framework defect of the input).

## Checks run

- `scan_patterns.py forbidden_patterns.tsv`: no output. `scan_patterns.py soft_patterns.tsv`: no output (two hits, "original" and "shipped", were rephrased).
- `C-UI-b_check.py`: 0 problems.

## Decisions cited

- **Open, from `decs_to_cite`:** DEC-044 (UI-444), DEC-050 (UI-320, UI-321, UI-452, UI-466), DEC-243 (UI-302, UI-490), DEC-245 (UI-320, UI-340, UI-363, UI-420, UI-421), DEC-316 (UI-391), DEC-556 (UI-462).
- **Resolved, from `decs_to_cite`, stated as the fixed rule:** DEC-026 (UI-305), DEC-083 (UI-462).
- **Also cited:** DEC-102 (the defects corrected by construction; UI-383, UI-416, UI-490 and §7.2), DEC-099, DEC-007, DEC-008, DEC-801, DEC-100, DEC-047, DEC-048, DEC-249, DEC-244, DEC-248, DEC-226, DEC-106, DEC-221, DEC-222, DEC-045, DEC-079, DEC-750, DEC-333 (Dropped).

## New DEC requests

1. **Scrolling-text overflow quirk** (UI-401, UI-404). The content height that decides whether the text scrolls uses the unraised top margin ⌊0.05 × h⌋, while the lines start at the raised margin (1.1 × font when 0.05 × h is below one font size). In a small widget the last line can therefore be clipped at the bottom without the text scrolling, by up to (1.1 × font − ⌊0.05 × h⌋). Default kept: the reference rule. Options: A — keep; B — use the raised margin in both places. Recommend B.
2. **Back on an open drop-down list** (UI-383). The reference does nothing on gamepad Back until an entry is chosen, a consequence of defect F19 (Back polled while drawing). I specified, under ADR-0032 (modal layers own Back) and DEC-102, that Back closes the list and keeps the value. Please add this to DEC-102's list of corrections, or record it as an Open DEC with the reference default.
3. **Session-list edge scrolling** (UI-384, UI-420). The input gives the row hook's conditions as "+1 on row 0" and "−1 on row 12", combined with defect F5 (the hook asked of the wrong widget), so the reference has no coherent behaviour to reproduce. I specified the intent that C-UI-a's UI-052 also states: down on the last visible row and up on the first visible row scroll the list by one. Please record it under DEC-102 or DEC-245 item 12.
4. **Colour picker availability** (UI-416, UI-417; defect F18). I applied DEC-102: one rule for drawing and pressing, the drawing variant (used slots only, the local slot never counted), computed in the update. reference/09 R-META-080 still states the press/draw difference as reference behaviour. The coordinator should either name F18 in DEC-102 (then R-META-080 changes, cross-document request 1) or open a DEC with the reference default.

## ADR conflicts

None in this document. It follows ADR-0032's deferred switch: every page change appears one frame after its request, while the reference rebuilds a page in the same frame for a click or the activate action and one frame later for a request made while drawing or in a page update (`ui.pages.switch_delay_frames`, UI-351). Note for G06: reference/06 R-CTRL-223 has the Power slots panel regenerate its screen zones while it draws, which ADR-0032 ("drawing never changes state") forbids; recipe D (UI-461) produces the zones in the panel's advance and keeps the same one-frame delay. See cross-document request 4.

## Missing assets

None needed. One gap for X-AST and G06: the manifest describes `cursor.pointer` as the "pointer arrow for menus", but no document says that the menus draw a different pointer from the world cursor's `cursor.world` (R-CTRL-350). UI-465 states only what the documents support: the world cursor draws the pointer, and `cursor.pointer` is the scroll-bar arrow and the selection arrows.

Asset ids cited: all 38 of `assets_for_you`, plus `sprite.worker.front`, `ui.backdrop_wheel`, `fx.beam_short`, `fx.sky_cloud`, `fx.glow_cloudy`, `help.mode_card`, `ui.difficulty_badge`, `ui.icon_check`, `cursor.unavailable`, `sfx.ui.power_slots.icon_hover` and the `glyph.pad.face_west`, `glyph.pad.face_north` glyphs. The start-up preload set of the front end (UI-335) maps onto existing ids: `glyph.pad.face_south`, `glyph.pad.face_east`, `glyph.pad.shoulder_left`, `glyph.pad.shoulder_right`, both shoulder shadows, `ui.fill_blur`, `sprite.worker.front`, `ui.backdrop_wheel`, `fx.beam_short` and `fx.sky_cloud`.

## Cross-document requests

1. **G09a, `screen_flow_and_match_setup.md` R-META-080.** Under DEC-102 (defect F18), availability is one rule for drawing and pressing, computed in the update: drop "counting the local slot when the row is pressed"; only used slots other than the local one count (UI-416, UI-417). See DEC request 4.
2. **G09a and C-UI-a, cycle-mode drop-downs.** R-META-044 says that in cycle mode "left and right step through the values"; C-UI-a's `layout_rendering_input_and_localisation.md` (the navigation rule on left and right) and `core_and_widgets.md` (the page drop-down) say that a cycle-mode drop-down changes only on activate and that left and right change only numeric page text fields. My UI-381 defers to C-UI-a. Please reconcile.
3. **G09b, `options_localisation_and_hooks.md` R-META-406.** My input has the colour or side row press, outside Siege and co-op, store the preferred colour **and save the settings file**; R-META-406 lists the save events "and no other" without it. Please add the event (UI-419).
4. **G06, `selection_and_power_slots.md` R-CTRL-223.** Zones must be produced in the panel's advance, not while drawing (ADR-0032, DEC-102); the one-frame delay stays (UI-461).
5. **C-UI-a:**
   - Links to `../data_schemas.md` should be `../data_schemas/README.md` (C-DAT's target is a folder). The renderer README (C-RND-a) has the same link.
   - `core_and_widgets.md` UI-037: the front-end screen also draws while the chat input window is open (UI-337; the input's front-end render rule).
   - README UI-014 cites UI-104 for "disabled widgets that never act"; UI-104 is the list of options screens; the page-button rule is UI-106.
   - My defect tests DT-01 to DT-33 and several rules cite C-UI-a requirement ids as they stood at the end of my run; C-UI-a was still renumbering `layout_rendering_input_and_localisation.md` while I worked. If any id moves, the citations in `pages_and_screen_recipes.md` §4.4, §4.5, §4.6 and §7.2 need the new numbers (the check script lists every cited sibling id with its text).
   - C-UI-a's own constants table (input §14) also lists the scrolling-text, level-row, browser-row-count and chat-log constants. I own the scrolling-text keys, `ui.pages.browser_visible_rows` and `ui.chat_log.line_step` (they come from my sections), and I cite reference/09 for the rest; C-UI-a should cite mine rather than define duplicates.
6. **Decision register (coordinator).** Add `architecture/components/ui_framework/pages_and_screen_recipes.md` to the Affects of DEC-102 (the defect suite lives here, as C-UI-a's README UI-014 says) and of DEC-801 (duplicate widget ids are build errors, UI-372).
7. **C-DAT (`data_schemas/`):** the page and options-screen layout record (UI-313, UI-370, UI-451: page id, debug name, fill boxes, initial focus, widget entries in registration order with kind, construction data, id, place, caption, tooltip, flags, binding, navigation entry and neighbours, back operation); the validations of UI-372; the preferences schema's apply-scope field (immediate, renderer reconfiguration, restart; UI-441).
8. **C-PLT (`platform_layer.md`):** the settings store notifies the readers of a setting when it changes (UI-441).
9. **C-DBG (`diagnostics_test_and_performance.md`):** the page fields of the diagnostics overlay (UI-482); tests T1–T37 and the defect suite DT-01 to DT-33 in the UI test harness, including the 30 and 144 frames-per-second repeat test (DT-13).
10. **C-AEN (`audio_engine.md`):** the interface events that pages and HUD panels raise are those of UI-315 (R-AUD-300); none is played directly.
11. **X-AST pass 2:** add `pages_and_screen_recipes.md` as a consumer of `sprite.worker.front`, `ui.backdrop_wheel`, `fx.beam_short`, `fx.sky_cloud`, `fx.glow_cloudy`, `help.mode_card`, `ui.difficulty_badge`, `cursor.unavailable`, `sfx.ui.power_slots.icon_hover`, `glyph.pad.face_west` and `glyph.pad.face_north`.
12. **C-RND-a (`frame_graph_and_passes.md`):** nothing to change; RND-095's "duration of its page changes" is `ref.meta.nav.transition_time` (UI-391), and the page host calls `StartTransition` only when the state is `Waiting` (UI-360 step 7), so the development warning of a start in `Finished` never fires from the front end.

## Page-id mapping to reference/09

Every one of the 15 specified pages of R-META-003 has a page id: `Title`, `Tutorials`, `HostOrJoin`, `ModeSelect`, `MapSelect`, `MatchSetup`, `AdvancedOptions`, `PlayerOptions`, `SessionBrowser`, `BrowserFilters`, `Password`, `Connecting`, `WaitForStart`, `Error`, `Credits`. The hook pages of reference/09 have hook ids: `QuickMatch` (R-META-028), `Achievements` (R-META-491), `LeaderboardSelect` (R-META-499) and `Leaderboard` (R-META-498, R-META-500, R-META-501). **No page of reference/09 is without a mapping.** The key-rebinding page (R-META-410) belongs to the settings branch and is an options screen, not a page id (UI-321). The windows of R-META-004 and R-META-061 to R-META-066 are options screens or layers (recipe B, UI-336, UI-440).

## Assumptions about C-UI-a's files

C-UI-a's three files existed while I worked and I cite them by id. I rely on: the window navigation order, cursor, back action, skip-next-update and lock (UI-050, UI-051); the scroll hook (UI-052) and the clamped scroll setter (UI-075); the page Back button with a target (UI-111) and the base press (UI-106); the roll-over cue rule (UI-110); the options screen (UI-099 to UI-104); the page grid and conventions (UI-163, UI-164); the front-end frame composition (UI-176); batching, the scissor stack and no state leak (UI-179, UI-181, UI-183); the font roles caption, body and title and the palette tokens (UI-188 and the font-role rule); the actions `action.menu.up`, `.down`, `.left`, `.right`, `.activate`, `.front_end_activate`, `.close`, `.dismiss` and `.scroll_speedup` (the action table of the navigation section). These answer G09a's request for action ids.

## Names needed that are not in the ledger (proposals)

- **Engine types:** `PageId` (enumerators `Title`, `Tutorials`, `HostOrJoin`, `ModeSelect`, `MapSelect`, `MatchSetup`, `AdvancedOptions`, `PlayerOptions`, `SessionBrowser`, `BrowserFilters`, `Password`, `Connecting`, `WaitForStart`, `Error`, `Credits`, `QuickMatch`, `Achievements`, `LeaderboardSelect`, `Leaderboard`), `NavOp` (`Push`, `Pop`, `Replace`, `UnwindTo`, `Rebuild`), `NavRequest`, `TransitionDirection`, `EnterOutcome` (`Built`, `Waiting`, `Redirect`), `EnterResult`, `FillBoxes`, `FrontEndPage`, `PageHost`, `PageContext`, `FrontEndViewModel`, `WidgetBuilder`, `ToolWindow`, `ToolPlacement`, `HudPanel`, `HudContext`; functions `OpenToolOnce`, `CloseTool`, `ToolString`; events `PageEntered`, `PageLeft`, `NavigationRejected`.
- **Front-end widget kinds:** `ScrollingText`, `ModeDescription`, `MapDescription`, `OptionDetails`, `StaticImage`, `ModePicture`, `MapThumbnail`, `HelpPicture`, `ColourPickerRow`, `ModeRow`, `MapRow`, `MapListHeader`, `SessionRow`, `SessionListHeader`, `LobbySlotRow`, `KickControl`, `SlotOpenControl`, `PrimaryButton`, `MapNameLabel`, `ChatLog`, `ConnectionStatus`, `TextLabel`.
- **Terms:** page stack, page host, page catalogue, hook page, back handler, enter rule, leave hook, match-setup sync, screen view-model, session-discovery listener, tool window, HUD panel.
- **String-id pattern:** `ui.debug.<tool>.<item>` for tool-window strings (the ledger has `ui.debug.unit_count`).
- **Tuning-key prefixes:** `ui.pages`, `ui.scroll_text`, `ui.picker`, `ui.map_row`, `ui.chat_log`.

## Dropped as legacy or out of scope (one line per group)

- Campaign, prologue and campaign-mode page slots and buttons: campaign content is out of the library.
- Debug single-player page, test-bed buttons, the spectator-request button and the debug "versus CPU" button: debug-only or unreachable.
- Trial-edition pages and widgets (upsell page, unlock-store button, trial key, trial status switch and its page redirect, "full game only" overlays): DEC-333 (Dropped).
- Credential page and widgets (key field, key paste, status box, credential page buttons and the start-up credential check): DEC-248.
- Update notice, update runner, platform updater and website link: online services, DEC-249.
- Message of the day and its 5.0 s extra hold: online service, DEC-249, R-META-024.
- Page slots of removed online platforms, downloadable-content slots and other-edition slots; the help-page sub-enumeration and the help-menu and help page slots (help is the help windows, R-META-061).
- Unreachable research page and the default-position helper it alone used; unused, uncompiled and not-in-project classes ("any map" and "any mode" rows, help selection, quick-match button, options-menu button, quick start, add-bot request, research mode, attract mode, map details panel, volume slider, empty error dialog stub).
- The page-name debug table and its count assertion (replaced by catalogue validation, UI-324), the never-set reload flag, the unused set-up flag and the unreachable overlay error dialogue with its menu lock.
- Implementation details of the reference: class, file and global names, string-keyed widget look-up, the copied-and-never-freed picture name, the per-character width estimate for wrapping, the per-frame re-wrap, and the read-buffer copy into a power-of-two texture for the page transition (replaced by the renderer's captured frame, RND-095).
- The reference's numeric page values (38 slots, 10 of them gaps) and its 10 compiled-but-unreachable slots: Species defines its own `PageId`, whose order carries no meaning.

## Published

Nothing. No publishing, upload or git write was used.
