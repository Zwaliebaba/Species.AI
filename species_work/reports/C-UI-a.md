# C-UI-a report — UI framework: README, core and widgets, layout, rendering, input and localisation

## Files written

| File | Size | Requirements |
|---|---|---|
| `design_next/architecture/components/ui_framework/README.md` | 20 590 bytes | UI-001 to UI-019 (19) |
| `design_next/architecture/components/ui_framework/core_and_widgets.md` | 96 931 bytes | UI-020 to UI-127 (108) |
| `design_next/architecture/components/ui_framework/layout_rendering_input_and_localisation.md` | 68 479 bytes | UI-160 to UI-241 (82) |

Scripts: `species_work/scripts/C-UI-a_check.py` (self-check), `C-UI-a_palette.py` (moved the palette values into the tuning table), `C-UI-a_renumber.py` (shifted UI-194 to UI-240 by one to insert the text-draw variants). All run with `python3 -I`.

Every file follows the component template (§6.4), with the eight headings and a §9 tuning table. Numbering is ascending and unique; UI-128 to UI-159 and UI-242 to UI-299 are unused and free for later use. The folder README is the index of all four files, including C-UI-b's `pages_and_screen_recipes.md` (UI-300 to UI-499).

## Counts

- **Requirements:** 209 (19 + 108 + 82).
- **Tuning keys owned:** 179 (96 in core_and_widgets §9.1, 83 in layout §9.1, 46 of them `ui.palette.*`). Every key is defined once in design_next and cited by a requirement. Key families: `ui.core`, `ui.chrome`, `ui.button`, `ui.value_control`, `ui.field`, `ui.increment`, `ui.scroll`, `ui.check`, `ui.dropdown`, `ui.message_dialog`, `ui.options_screen`, `ui.menu_control`, `ui.page_button`, `ui.page_back`, `ui.page_title`, `ui.fill_box`, `ui.page_dropdown`, `ui.page_field`, `ui.layout`, `ui.scale`, `ui.access`, `ui.text`, `ui.font`, `ui.glyph`, `ui.lang`, `ui.budget`, `ui.palette`. None collides with C-UI-b's (`ui.pages`, `ui.scroll_text`, `ui.picker`, `ui.map_row`, `ui.chat_log`).
- **Keys cited from other documents** (not redefined): `ref.meta.layout.*`, `ref.meta.title.emblem_*`, `ref.meta.backdrop.glow_colour`, `ref.meta.nav.transition_time`, `ref.meta.option_controls.*`, `ref.meta.setup.mode_option_slots`, `ref.meta.setup.small_control_height`, `ref.meta.player.name_max_length`, `ref.meta.chat.*`, `ref.meta.settings.large_menus.default`, `ref.meta.lang.base` (reference/09); `ref.match.option.password_max_length` (reference/01); `rnd.hud.ortho_height` (renderer).
- **Diagrams:** 6 Mermaid, all with captions, all parsed by the Mermaid parser (`G06_mermaid.mjs`): layering flowchart and frame sequence (README); pointer state machine and text-field editing states (core); string look-up flowchart and language-switch sequence (layout).
- **Enumerations:** 27 tables (bindings, palette, actions, font roles, layout grid with four worked resolutions, failure modes and others), 59 acceptance tests (R1–R7, T1–T30, L1–L22).
- **Gates:** forbidden patterns: no hit; soft patterns: no hit; self-check: clean (the only notes are the range bounds UI-159, UI-299 and UI-499 in the README's index table).

## Decisions cited

- **Open DECs of `decs_to_cite`, each cited by a requirement:** DEC-221 (UI-005, UI-239), DEC-222 (UI-002), DEC-316 (UI-106, UI-191).
- **Resolved / Dropped of `decs_to_cite`:** DEC-007 (UI-002), DEC-099 (UI-018, UI-161, UI-219, UI-239), DEC-102 (UI-014 and the corrected rules listed in core §8), DEC-801 (UI-127, UI-222, UI-225), DEC-337 Dropped (layout §8).
- **Further DECs cited:** DEC-013, DEC-015, DEC-045, DEC-050, DEC-100, DEC-200, DEC-243, DEC-244, DEC-245, DEC-800.
- **ADRs cited:** ADR-0005, 0006, 0008, 0012, 0014, 0017, 0028, 0029, 0030, 0031, 0032, 0033, 0034, 0059, 0061, 0063, 0064, 0066, 0067, 0075.

## New DEC requests

1. **Keyboard bindings of the menu actions** (UI-204). The reference control scheme binds menu up, down, left, right, activate and close to the gamepad only; keyboard players navigate with the pointer, and the arrow keys and Enter exist only in an alternative control set that is never selected. ADR-0032 (Accepted) and ADR-0064 require every screen to be usable with the keyboard alone, so I specified the arrow keys, Enter and Esc (outside a match) as the Species baseline. Options: A — the baseline as written; B — the reference (pointer only for keyboard players). This departs from "reference default" because an Accepted ADR requires it; please record it.
2. **Scroll-arrow repeat timing** (UI-077). DEC-102 corrects the frame-rate-dependent repeat (one step per frame, no delay), but gives no rate. I wrote one step on the press and one every 1/60 s (`ui.scroll.arrow_repeat_period`), which equals the reference at 60 frames per second, with no initial delay. Alternative: the input component's menu repeat delay and rate (UI-216).
3. **Float increment curve** (UI-072). The reference float stepping is change × elapsed × 0.2 per frame, which accelerates and depends on the frame rate. I normalised it at 60 updates per second (`ui.increment.float_reference_rate`). Please confirm the normalisation rate.
4. **Caps Lock in text fields** (UI-066). An upper-case letter typed into a text-type field is stored lower-case unless Shift is held, so Caps Lock alone gives lower case. This is kept as the reference rule. Options: A — keep; B — store the character as the platform delivers it.
5. **Percent sign in text fields** (UI-066). The reference rejects '%' as typed input, and with it every character whose UTF-16 low byte is 0x25 (for example U+0125 and U+4E25), to protect a printf-style draw path. Species has no such path (R-META-469, DEC-102), so I made '%' an ordinary character. Please confirm under DEC-102, or record an Open DEC with the reference rejection as option B.
6. **Drop-down automatic values** (UI-082). The input gives the counter's behaviour (advances in call order and is reset by clearing) but not its initial value. Default proposal: 0.
7. **Drop-down selection of an absent value** (UI-083). `Select(value)` writes the bound value even when no option has it, and the caption then falls back to the drop-down's label. Kept as the reference rule; an Open DEC could offer "leave the binding unchanged" as option B.
8. **3-D text centring** (UI-194 item 3, layout §8 input 8). Centred and right-aligned 3-D text is shifted by a product of the CJK and Latin terms instead of their sum, so single-script labels are drawn left-aligned. Kept as the reference rule; Open, default reference.
9. **Upward rotated text has no outline** (UI-194 item 2). Kept as the reference rule; low priority.

## ADR conflicts and clarifications

1. **ADR-0033 and tool windows.** ADR-0033 applies DPI to text minimums, line widths and icons, and does not say how fixed-pixel tool windows scale. I multiplied tool-window lengths by the DPI scale and the UI scale (UI-170), so a tool window keeps its physical size on a high-DPI monitor. ADR-0033 should say so, or say otherwise.
2. **ADR-0064 minimum text size.** The ADR gives 9 px at 1080p and 19 px at 4K only. I interpolate linearly between them and scale proportionally outside (UI-171, `ui.access.min_text_*`). Please confirm, or amend the ADR with the rule.
3. **ADR-0031 and RND-058.** ADR-0031 names three UI blend states (alpha, additive, ink). RND-058 maps ink with source alpha 0 to `Darkening`. The arithmetic is identical; I state that equivalence (UI-180) and keep the three UI states. This is not a conflict, but the ADR could say it.
4. **ADR-0033 relayout and R-META-450.** ADR-0033 lays pages out again on a language change without losing state; reference/09 R-META-450 then closes every window and rebuilds the front end. Both are specified (UI-238): the framework relays out, and then the options apply rebuilds. No change is needed unless the design phase wants the state kept across a language switch.
5. **Baseline values I introduced** (Species baseline, not reference): UI budget 32 draws, 65 536 vertices, 1.0 ms (`ui.budget.*`, UI-240); glyph-cache pages 1024 × 1024 R8, 8 pages (`ui.glyph.*`, UI-198). ADR-0034 says only "fixed size"; it could adopt these values or name its own.

## Missing assets

None. Asset ids I cite beyond `assets_for_you`, all in the manifest (X-AST: please add my files as consumers):

- `cursor.pointer` (scroll-bar arrows, UI-076) and `ui.panel_accent` (window body, UI-054);
- `ui.icon_check` (UI-081, UI-125), `ui.menu_highlight` (UI-114), `ui.fill_blur` and `fx.glow_cloudy` (UI-115), `ui.logo_emblem` (UI-102);
- `glyph.pad.face_east` (UI-111, UI-112), `glyph.key.tab` and `glyph.pad.*` (UI-224);
- the five `font.ui.*` assets and the nine `strings.*` assets already list the layout file as a consumer; nothing to add for them.

New string ids (core §9.3): `ui.dialog.close`, `ui.dialog.error`, `ui.dialog.success`.

## Cross-document requests

1. **C-UI-b (`pages_and_screen_recipes.md`).**
   - UI-390 says the session list header, the slot rows and the page overlays use "the compact role". The three roles are caption, body and title (UI-195). Those elements use the **caption** role, whose face is `font.ui.stylised`. "Compact" is the face of the **body** role (`font.ui.compact`): rows, the chat log and scrolling text.
   - "The alternate caption colour for body-role buttons": the alternative-colour flag of a page button is a colour variant only (UI-105, UI-108, `ui.palette.fe_caption_alt`), with no font change.
   - Your references to my numbers (UI-002 to UI-075) are all current. My layout requirements from UI-194 on were shifted by one before you cited any of them.
2. **C-INP (`input.md`).**
   - Adopt the action ids `action.menu.up`, `action.menu.down`, `action.menu.left`, `action.menu.right`, `action.menu.activate`, `action.menu.front_end_activate`, `action.menu.close`, `action.menu.dismiss` and `action.menu.scroll_speedup`, with the default bindings of UI-203 and UI-204.
   - Own the repeat delay and rate of the menu directions (UI-216).
   - Deliver the frame's pointer events in order (README UI-006), text characters with their Shift, Ctrl and Alt state (UI-007), and map the editing keys (arrows, Home, End, Delete, Ctrl+A, C, X and V) to `EditCommand` (UI-038, UI-067).
   - State the input-mode rule (the last active device class, with the gamepad preferred; UI-205).
3. **G06 (reference/06).** R-CTRL-035 points to the UI framework for the menu navigation actions: the ids are now in layout UI-203. Please link UI-203, or add the ids to table 2.1-A. G09a asked G06 to confirm that `action.match.chat` applies on the match setup page; I use it there (UI-092).
   - Answers to G06's C-UI request: the Power slots panel is drawn by the renderer in the HUD space (RND-377 to RND-379), with this folder's text styles (UI-189 to UI-192) and string table. The withholding of a press made over a window is README UI-006 step 3; the bar's own withholding stays R-CTRL-226. The in-match menu is an options screen (UI-104), with the content of R-META-062 and R-META-063. The chat input is UI-091 to UI-094. The missing-string rule is UI-219.
4. **G09a (reference/09).**
   - The page grid is cited from your keys and not redefined (UI-163).
   - The action ids you asked for: menu close `action.menu.close`, dismiss `action.menu.dismiss`, scroll speed-up `action.menu.scroll_speedup`.
   - R-META-208 links the fill boxes to `pages_and_screen_recipes.md`; the fill box is `core_and_widgets.md` UI-115, the highlight beam UI-114 and the page title UI-113.
   - The options-screen title draws the game name as text, at 0.2 × the title-bar height, with an emblem top at 0.1 × that height (UI-102). The front-end screen draws the wordmark (your R-META-207).
5. **G09b (reference/09 options_localisation_and_hooks).** The keys you asked me to own now exist; please cite them:
   - the large-menu factor: `ui.layout.large_menu_factor` and `ui.layout.large_menu_height` (UI-167; R-META-449 item 4);
   - the per-language horizontal scale: `ui.text.horizontal_scale_latin`, `ui.text.horizontal_scale_cjk` and `ui.text.cjk_first_code_point` (UI-197; R-META-475);
   - the hand-over between string tables: `ui.lang.previous_table_frames` (UI-239; R-META-466), now with a reference-counted generation, so a table is never released while it is still viewed.
6. **C-RND-a / C-RND-b.**
   - The outline and fill text styles are UI-189 to UI-192, the draw point UI-193 and the ink rule UI-191 (DEC-316).
   - The page-transition duration is `ref.meta.nav.transition_time` (reference/09).
   - I agree that the −0.325 px projection offset is dropped.
   - The hint overlay's 8 px and 2 px lines (pass 17) and the chat talk hint are **not** in my input ranges; reference/11, C-UI-b or the renderer must own them.
   - For the coordinator: `rnd.hud.chat.*` (renderer README) duplicates `ref.meta.chat.scale_*`.
7. **C-DAT (data schemas).**
   - Schemas are needed for: the page layout record (cells, string ids, accessible names, focus neighbours, overflow rules, bindings); the palette, with default and high-contrast variants; text styles; the font-role map per language; and the string table (plain text, keyboard-and-mouse and gamepad variants, and markup with a mode passage, a binding argument and an embedded id).
   - The string-table limits are 1 024 characters expanded, 64 characters per marker and 96 per id (UI-225).
   - The UI-scale and text-scale settings need entries.
8. **C-AEN.** The sound-settings failure message that the audio engine shows through the message dialog (UI-090) needs a string id, for example `ui.dialog.sound_settings_failed`, with new text.
9. **C-PLT.** The message dialog centres itself; the placement at (100, 100) used by the screen-mode and sound callers is the callers' rule (G09b asked you for the screen-mode case). The UI needs the DPI scale and the resize events of ADR-0075 (UI-009).
10. **C-DBG.** The development windows (the network statistics window and others) are tool windows on the UI window layer (UI-001, UI-060, UI-061, UI-166). The UI counters for your overlay are in core §6 and layout §6.
11. **X-GLO.** Terms to add or relink:
    - new terms: draw point, menu size, navigation order, navigation cursor, back action, base press, birth cascade, fill box, highlight beam, page list, captured widget, text row convention, tool window, increment glyph, inset panel, close box, palette token, font role (caption, body, title), horizontal scale, horizontal spacing factor;
    - existing entries: "options screen" (core §4.24) and "string table" (layout §4.11) are correct as linked.
12. **Coordinator.**
    - Component tuning keys (`ui.<topic>.<parameter>`) and UI string ids (`ui.<prefix>.<item>`) share the `ui.` namespace. GameAssets §18 reserves `ui.window.*` and `ui.dialog.*` for strings, so I named my key families `ui.chrome.*` and `ui.message_dialog.*` to stay clear of them. A gate that checks backticked `ui.x.y` names must tell keys (tuning-table rows) from string ids.
    - My page-level constants that live in other documents are listed below.

## Constants of the input's constant table and where they live

- **Owned here (keys in §9):** every UI core, window, button, value-control, text-field, increment, scroll-bar, check-box, drop-down, message-dialog, options-screen, menu-control, page-widget and fill-box constant; the large-menu factor; the text offsets, default size, spacing factors and horizontal scales; the string-table limits and the lifetime of the previous table.
- **Cited, owned elsewhere:**
  - the page grid, the fonts and the standard button: `ref.meta.layout.*`;
  - the emblem and the wordmark: `ref.meta.title.*`;
  - the menu transition time: `ref.meta.nav.transition_time`; its curve: RND-096;
  - the front-end fade-in: `ref.meta.front_end.fade_in`;
  - the level-row cascade: `ref.meta.map_select.slide_*`;
  - scrolling text: C-UI-b `ui.scroll_text.*`;
  - the backdrop: `ref.meta.backdrop.*`; the blink: `ref.meta.blink.*`;
  - the connecting dots: `ref.meta.connect.*`; the slot flash: `ref.meta.slot.flash_time`;
  - the visible session rows: C-UI-b `ui.pages.browser_visible_rows`;
  - the page count of 38: DEC-026 and C-UI-b;
  - the mode option slots of 64: `ref.meta.setup.mode_option_slots`;
  - the player name and password length of 16: `ref.meta.player.name_max_length` and `ref.match.option.password_max_length`;
  - the chat limits of 255 and 15, and the in-match chat box: `ref.meta.chat.*`;
  - the team colours (8) and the Siege colour ids: reference/01;
  - the client lag threshold on leaving: `ref.meta.setup.leave_lag_threshold`;
  - the achievement rows: `ref.meta.achievement.*`;
  - the thumbnail and mode-card sizes: R-MAP-022 and R-META-032;
  - the slot column, the map-list columns and the lobby chat box: `ref.meta.setup.*`, `ref.meta.map_select.column_*` and `ref.meta.chat.*`;
  - the browser rebuild and list-request intervals and the browser columns: DEC-245, deferred;
  - the settings keys that the front end reads: the reference/09 settings register.
- **Dropped:**
  - fixed name and title buffers of 256 (typed handles; DEC-801);
  - the random-range constant of automatic placement (the presentation generator);
  - the front-end per-character width of 25 (measured widths);
  - the double-click window and the rows of the file dialogs (editor only);
  - the authentication key length and its files;
  - the campaign and prologue game types;
  - the store address, the buy-now art and the store window size;
  - the dead error overlay and help image;
  - the beta-tester and updater files.

## Names needed that are not in the ledger (proposals)

- **Types:**
  - handles and geometry: `PanelId`, `WidgetId`, `PointF`, `SizeF`, `RectF`;
  - input: `PointerEvent`, `PointerEventKind`, `TextInput`, `EditCommand`, `Callback`, `InputMode`;
  - the widget set: `UiBorderlessButton`, `UiCloseButton`, `UiInsetPanel`, `UiTextField`, `UiIncrementGlyph`, `UiScrollBar`, `UiCheckBox`, `UiDropDown`, `UiDropDownList`, `UiDropDownOption`, `UiMessageDialog`, `UiChatEntry`, `UiChatField`;
  - value binding: `UiValueBinding`, `ValueType`;
  - the front-end family: `PageButton`, `PageBackButton`, `ScreenCloseButton`, `PageTitle`, `PageDropDown`, `PageDropDownList`, `PageDropDownOption`, `PageTextField`, `PageIncrementGlyph`, `PageCheckBox`;
  - layout and drawing: `PageGrid`, `UiScaleState`, `UiBlend`, `UiVertex`, `FontRole` (`Caption`, `Body`, `Title`), `TextPass`, `TextAlign`, `TextStyle`, `DrawContext`;
  - text: `TextView`, `MessageArguments`, `LanguageTag`, `StringId`, `AssetId`.
  - `UiWindow` and `UiButton` are in the ledger.
- **Functions:**
  - layout: `ComputePageGrid`, `LargeMenuScale`, `ToolWindowScale`, `FinalTextSize`;
  - the UI core: `RegisterPanel`, `RemovePanel`, `BringToFront`, `SetPanelPosition`, `SetPanelSize`, `PanelAt`, `WidgetAt`, `PointerInPanel`, `PointerInWidget`, `UpdatePointer`, `RegisterPopup`, `ClosePopup`, `Maximise`, `Unmaximise`, `SetFocus`, `IsTextEditing`, `WindowsOnScreen`, `RemoveAllWindows`, `SetTooltipHandler`;
  - the standard window: `MenuSize`, `SetMenuSize`, `ClientRect`, `AddToNavigation`, `SetCursor`, `SetBackAction`, `SkipNextUpdate`, `ConsumeScroll`, `AddValueControl`, `RemoveValueControl`, `AddColourControl`, `AddMenuControl`, `ShowDialog`;
  - text and strings: `MeasureWidth`, `Prerasterise`, `Lookup`, `Exists`, `Format`.
- **Action ids:** the nine `action.menu.*` ids of UI-203; please also add the naming pattern `action.<group>.<name>` that G06 proposed.
- **String ids:** `ui.dialog.close`, `ui.dialog.error`, `ui.dialog.success`.
- **Terms:** draw point, text row convention, menu size, hover-highlight mode, navigation order, navigation cursor, back action, base press, birth cascade, pop, fill box, highlight beam, page list, captured widget, text-type field, increment glyph, inset panel, close box, tool window, palette token, horizontal scale, spacing factor.

## Dropped as legacy, one line per group

- **Library identity:** the UI library's name, its author credits and its file lists; its layers are restated as Species layers.
- **Name-keyed registry:** look-up by name, duplicate-name shadowing, 256-byte name fields, the unique-name generator and the dead caption fallback. Typed handles replace them (UI-012).
- **Dirty rectangles and the clear-screen callback**, which were never cleared; the UI redraws every frame (UI-177).
- **Superseded graphics state:** the −0.325 px projection shift, per-widget toggling of blend, shading, depth and line-width state, and the state leaks of the scroll arrows. The batched renderer replaces them (UI-177 to UI-184).
- **Bitmap font atlases:** atlas discovery and file naming, the per-range inventory, the UV and margin formulas, magenta keying, the single-bitmap fallback path, per-edition atlas packaging and the misnamed Hangul atlas (DEC-337). The glyph cache replaces them (UI-198).
- **Text encoding details:** fixed-advance width estimates used to place text, narrow-string conversions, the internal UTF-16 string class and its quirks, and the surrogate decoding defect. Species uses UTF-8 and measured widths (UI-200, UI-202).
- **The printf text path:** printf-style drawing, the percent rejection that guards it, the low-byte character tests and the full-stop mapping of U+00BE.
- **A determinism helper** that truncated parsed floats, which has no UI meaning.
- **Widgets used only by the editor, the sound editor or removed windows:** the exit button, the label button, the colour-editing window, the file dialog with its double-click and multi-select (and the Ctrl multi-select binding), the editor file pickers (kept as a hook), the authentication key field, and the small profile, mod, bindings, about and profiler windows.
- **Options-screen classes outside the live build:** the old main menu, reset, profile and leaderboard windows, and the debug crate-help window.
- **Resolution change by re-creating the renderer**, the temporary 640 × 480 window and the website button's re-initialisation. Species relays out instead (UI-172 to UI-174).
- **Numeric codes:** the input-mode values, the value-type codes and the phrase suffixes. They become Species enums and string variants.
- **Literal English text:** the window-chrome tooltips, the increment-glyph tooltips and the message-dialog texts. They become string ids (UI-090, UI-232).
