# 28 — UI framework

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md (build facts are specified in [31 §7](31_platform_build_and_tools.md#7-build-configurations-and-flags)).
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: P17a_ui_framework_editor (§1–§10), P17b_menus_options (§11), P18_ui_gamemenu (§12–§13, plus page table §2 and layout §1.3–§1.4 for context), P12_render_core (24.7 text rendering), P14_platform_lib (29.L language table, 31.11 localisation build), P10_input_camera (input modes), `_meta_*` cross-partition notes. Every numeric constant and state transition below was re-read in the code for this document; corrections are flagged "Resolved:". The labels P10, P12, P14, P17a, P17b and P18 (sometimes with a draft section number) name the analysis drafts listed here; they are provenance only and never the evidence for a claim — the evidence is the cited code or data.

## Contents

1. [Architecture, layering and build reality](#1-architecture-layering-and-build-reality)
2. [Eclipse core (`contrib/eclipse`)](#2-eclipse-core-contribeclipse)
3. [Darwinia layer: `DarwiniaWindow` and `DarwiniaButton`](#3-darwinia-layer-darwiniawindow-and-darwiniabutton)
4. [Widgets (inputs, scroll bars, check boxes, drop-downs, dialogs)](#4-widgets-inputs-scroll-bars-check-boxes-drop-downs-dialogs)
5. [Front-end layer: `GameOptionsWindow` and the `GameMenu*` widget family](#5-front-end-layer-gameoptionswindow-and-the-gamemenu-widget-family)
6. [`GameMenuWindow` page framework](#6-gamemenuwindow-page-framework)
7. [Layout conventions and formulas](#7-layout-conventions-and-formulas)
8. [Rendering of windows (palette, textures, blend states, fonts)](#8-rendering-of-windows-palette-textures-blend-states-fonts)
9. [Keyboard, controller and mouse navigation rules](#9-keyboard-controller-and-mouse-navigation-rules)
10. [Localisation integration](#10-localisation-integration)
11. [How a new screen is built (recipes) and re-implementation guidance](#11-how-a-new-screen-is-built-recipes-and-re-implementation-guidance)
12. [Class diagram and complete UI class inventory](#12-class-diagram-and-complete-ui-class-inventory)
13. [Framework defects and edge cases (do not carry forward)](#13-framework-defects-and-edge-cases-do-not-carry-forward)
14. [Constants table](#14-constants-table)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

Related documents: functional description of every menu page, option and lobby rule — [09 Meta systems](09_meta_systems.md); text/font rendering and the menu-transition pass — [24 Rendering](24_rendering.md); UI textures — [25 Textures and assets](25_textures_and_assets.md); input bindings and the input-mode model — [06 Player control and input](06_player_control_and_input.md); language-file grammar — [29 Data formats reference](29_data_formats_reference.md); language build/packaging — [31 Platform, build and tools](31_platform_build_and_tools.md); map-editor windows — [08 Levels and level design](08_levels_and_level_design.md); debug/cheat/network windows — [32 Debug, test and performance](32_debug_test_and_performance.md); UI sounds — [10 Audio design](10_audio_design.md).

---

## 1 Architecture, layering and build reality

### 1.1 The layers

The Multiwinia UI is a retained-hierarchy, immediate-mode-rendered window manager in **two framework layers**, on top of which a **third, front-end layer** builds the styled full-screen menus [C]:

| Layer | What it is | Files | Responsibility |
|---|---|---|---|
| **L1 Eclipse core** | Introversion's generic interface library, "ECLIPSE version 2.0, Generic Interface Library" (author credit in `eclbutton.h`) | `contrib/eclipse/*` | Window list and z-order, name registry, focus, mouse state machine (press/drag/release, window move/resize), keyboard routing to the focused window and its text-edit target, dirty rectangles, tooltip timer, popups, maximise. Knows nothing about drawing styles. [C] `contrib/eclipse/eclipse.h` |
| **L2 Darwinia layer** | Concrete look and behaviour inherited from Darwinia | `code/interface/darwinia_window.*`, `input_field.*`, `scrollbar.*`, `checkbox.*`, `drop_down_menu.*`, `message_dialog.*`, `filedialog.*` | `DarwiniaWindow` (chrome, controller/keyboard navigation list `m_buttonOrder`, large-menu scaling, value-control factory), `DarwiniaButton` (highlight model, captions) and the widget set (input fields, scrollers, colour picker, scroll bar, check box, drop-down, message box, file picker). [C] |
| **L3 Front-end layer** | Multiwinia's full-screen menu style | `code/game_menu.*`, `code/interface/mainmenus.*`, `code/UI/*` | `GameOptionsWindow` (full-screen page with title bar/left/right boxes, error dialogue), the `GameMenu*` widgets (animated buttons, cycle drop-downs, styled inputs/check boxes), `GameMenuWindow` (the whole front end as one window with a 38-slot page state machine) and 79 button/panel classes. [C] |

```mermaid
flowchart TB
  subgraph Platform["Platform / engine"]
    WM["WindowManagerWin32 WndProc: WM_CHAR"] --> EUK
    UIA["UserInput::AdvanceMenus (once per frame)"] --> EUM
    UIR["UserInput::Render (2D overlay pass)"] --> ER
  end
  subgraph L1["L1 Eclipse - contrib/eclipse (static lib 'eclipse', live)"]
    EUM["EclUpdateMouse + EclUpdate"]
    EUK["EclUpdateKeyboard"]
    ER["EclRender"]
    EW["EclWindow"] --- EB["EclButton"]
  end
  subgraph L2["L2 Darwinia layer - code/interface framework files"]
    DW["DarwiniaWindow"] --> EW
    DB["DarwiniaButton"] --> EB
    WID["InputField, InputScroller, ColourWidget, ScrollBar, CheckBox, DropDownMenu, MessageDialog, FileDialog"] --> DB
  end
  subgraph L3["L3 Front end - code/game_menu, code/interface/mainmenus, code/UI"]
    GOW["GameOptionsWindow pages (Help and Options, Settings, in-game menu)"] --> DW
    GMW["GameMenuWindow (front end, 38 page slots)"] --> DW
    GMB["GameMenuButton family, GameMenuDropDown, GameMenuInputField, GameMenuCheckBox, 79 code/UI classes"] --> WID
  end
  subgraph Tools["Tool windows on L2"]
    ED["Map editor windows [B:EDITOR]"] --> DW
    DBG["Network stats (F9) [B:FULL], cheat / sound editor [B:DEBUG]"] --> DW
  end
```

### 1.2 Build reality — what is compiled

**`contrib/eclipse` is the live window framework; `contrib/systemiv/interface` and `contrib/systemiv/lib/eclipse` are not compiled.** Evidence [C]:

1. `targets/vs2005/Darwinia.sln` contains a project `"eclipse"` whose file is `..\..\contrib\eclipse\eclipse.vcproj`; it is a static-library project (`_LIB`) compiled with `PreprocessorDefinitions="WIN32;NDEBUG;_LIB;_CRT_SECURE_NO_DEPRECATE;TARGET_MSVC"` and `AdditionalIncludeDirectories="..\..\code;..\..\contrib\SystemIV"` [C] `contrib/eclipse/eclipse.vcproj`. Its file list is exactly `eclbutton.cpp/.h`, `eclipse.cpp/.h`, `eclwindow.cpp/.h` (`contrib/eclipse/eclipse.dsp` is the VC6 predecessor and is not used by the VS2005 solution).
2. `targets/vs2005/Darwinia.vcproj` (803 `RelativePath` entries) references SystemIV files only under `contrib\systemIV\lib\{debug_utils, memory_leak, network_stream, profiler, string_utils, filesys\*, netlib\*, metaserver\{authentication, matchmaker, metaserver}, math\{filecrc, hash, math_stubs, random_number}, tosser\*, unicode\*}`. No path under `contrib\systemiv\interface\` or `contrib\systemiv\lib\eclipse\` appears in any project file [C] grep of `targets/vs2005/*.vcproj`, `*.sln`, `*.vsprops`.
3. Include resolution: game code writes `#include <eclipse.h>` / `"eclipse.h"`. The game project's include path is `…;..\..\code;..\..\contrib;…;..\..\contrib\SystemIV;..\..\contrib\SystemIV\lib\netlib;..\..\contrib\eclipse;…` [C] `targets/vs2005/Darwinia.vcproj` (VCCLCompilerTool). There is no `eclipse.h` in `code/`, `contrib/` or `contrib/systemiv/` (the SystemIV copy lives one level deeper at `contrib/systemiv/lib/eclipse/eclipse.h` and would need `"lib/eclipse/eclipse.h"`), so `eclipse.h` resolves to `contrib/eclipse/eclipse.h` [C] directory listing; no `code/` file includes `lib/eclipse/…` or `interface/components/…` [C] grep.
4. Consequently the SystemIV UI components (`contrib/systemiv/interface/components/{checkbox, core, drop_down_menu, filedialog, inputfield, message_dialog, scrollbar}`, `interface/options/{screenoptions_window, soundoptions_window}`, `interface/soundsystem/{soundeditor_window, soundparameter_window, soundstats_window}`, `interface/styleeditor_window`) and the SystemIV Eclipse copy are **[B:DEAD]**; their Multiwinia counterparts are the `code/interface` files [C] `targets/vs2005/Darwinia.vcproj` (no file under `contrib/systemiv/interface/` or `contrib/systemiv/lib/eclipse/` is in the project; [31 §7.4](31_platform_build_and_tools.md#74-per-file-overrides-in-the-vcproj-c)).

**`contrib/systemiv` usage by the live UI** [C] (header shadowing rule: `code` precedes `contrib/SystemIV` on the include path, so an include of the form "lib/name.h" resolves to the copy under the code directory when one exists — `targets/vs2005/Common.vsprops` `AdditionalIncludeDirectories`, [31 §7.2](31_platform_build_and_tools.md#72-property-sheets-applied-to-every-configuration-c)):

| SystemIV module | Used by the UI for | Ref |
|---|---|---|
| `contrib/systemiv/lib/tosser/llist.*` | `LList<T>` — every window list, button list, `m_buttonOrder`; `GetData(i)`/`operator[]` return NULL for an invalid index (relied upon by `m_buttonOrder[m_currentButton]` on an empty list) | [C] `contrib/eclipse/eclwindow.h:EclWindow`, `contrib/systemiv/lib/tosser/llist.cpp:LList::GetItem` |
| `contrib/systemiv/lib/unicode/unicode_string.*` | `UnicodeString` captions, tooltips, titles, input buffers | [C] `contrib/eclipse/eclbutton.h:EclButton` |
| `contrib/systemiv/lib/math/random_number.*` | `AppRandom()` (non-synchronised RNG, `APP_RAND_MAX = 32767`) for automatic child-window placement — does not touch the lock-step RNG | [C] `contrib/eclipse/eclipse.cpp:EclRegisterWindow`, `contrib/systemiv/lib/math/random_number.h` |
| `contrib/systemiv/lib/debug_utils.*` | included by `eclipse.cpp` (assertions are commented out) | [C] `contrib/eclipse/eclipse.cpp` |
| `contrib/systemiv/lib/metaserver/authentication.*` | `AUTHENTICATION_KEYLEN`, key check/status phrases in the auth-key page | [C] `code/UI/AuthInputField.h:AuthInputField` |
| (shadowed, **not** used) `contrib/systemiv/lib/language_table.*`, `preferences.*`, `hi_res_time.*` | Multiwinia uses `code/lib/language_table.*`, `code/lib/preferences.*`, `code/lib/hi_res_time.*` | [C] `targets/vs2005/Darwinia.vcproj` lists only the `code/lib` copies; `targets/vs2005/Common.vsprops` puts `code` before `contrib/SystemIV` on the include path |

### 1.3 Component status

| Component | Files | Compiled in FULL? | Reachable in FULL? | Tag |
|---|---|---|---|---|
| Eclipse core | `contrib/eclipse/*` | Yes (static lib `eclipse`) | Yes — every window | [B:FULL] |
| `DarwiniaWindow` / `DarwiniaButton` family | `code/interface/darwinia_window.*` | Yes | Yes | [B:FULL] |
| `InputField`, `InputScroller` | `code/interface/input_field.*` | Yes | Yes (`GameMenuInputField`, `ChatInputField`, `AuthInputField` subclass `InputField`; `GameMenuInputScroller` subclasses `InputScroller`) | [B:FULL] |
| `ColourWidget`, `ColourWindow` | `code/interface/input_field.*` | Yes | Only via `CreateColourControl`, whose callers are editor windows | compiled, tool-only |
| `ScrollBar` | `code/interface/scrollbar.*` | Yes | Yes — only on the `GameMenuWindow` server browser (`SetupJoinGamePage`) in FULL; also `SetupAchievementsPage` (unreachable), `CrateHelpWindow` ([B:DEBUG]), `SoundStatsWindow` and the sound editor ([B:DEBUG], `SOUND_EDITOR`), `FileDialog`/`MultiwiniaFileDialog` (editor) [C] grep `new ScrollBar`. Resolved: P17a listed `HelpAndOptions` and `SoundStatsWindow` as FULL users; the help-window user is `CrateHelpWindow::Create` (inside `INCLUDE_CRATE_HELP_WINDOW`, TARGET_DEBUG) and `soundstats_window.cpp` is wrapped in `#ifdef SOUND_EDITOR`. | [B:FULL] |
| `CheckBox` | `code/interface/checkbox.*` | Yes | Yes (`GameMenuCheckBox`) | [B:FULL] |
| `DropDownMenu` | `code/interface/drop_down_menu.*` | Yes | Yes (`GameMenuDropDown`; `ControlMethodDropDownMenu` is in the unreachable key-bindings window) | [B:FULL] |
| `MessageDialog` | `code/interface/message_dialog.*` | Yes | Yes — screen-mode failure (`code/renderer.cpp:Renderer::Initialise`) [C]; hardware-3D-sound failure (`code/sound/soundsystem.cpp`) only if pref `SoundHW3D` = 1, which no shipped control sets (the Hardware-3D control is inside `#ifdef USE_SOUND_HW3D`, off) [C]. Resolved: P17a also listed the "Restart Required" dialog of `code/interface/prefs_screen_window.cpp:RestartWindowManagerAndRenderer`; that call is inside `#ifdef TARGET_OS_LINUX` → [B:DEAD] on PC [C]. Other callers are editor/sound-editor windows. | [B:FULL] |
| `FileDialog` | `code/interface/filedialog.*` | Yes | Only from editor landscape window and sound editor | compiled, tool-only |
| `MultiwiniaFileDialog`, `MultiwiniaWindow`, `NewMapWindow` | `multiwinia_filedialog.*`, `multiwinia_window.*` | Yes (no `#ifdef`) | No (opened by `MultiwiniaEditorButton` under `LOCATION_EDITOR` and by the dead Darwinia `MainMenuWindow`) | [B:EDITOR] |
| Map-editor windows | `location_editor.*`, `editor_window.*`, `buildings_window.*`, `camera_anim_window.*`, `camera_mount_window.*`, `landscape_window.*`, `lights_window.*`, `instant_unit_window.*`, `tree_window.*` | Bodies inside `#ifdef LOCATION_EDITOR` | No | [B:EDITOR] |
| `InvalidCratesWindow` | `invalidcrates_window.*` | Yes | No (created only by `LocationEditor::CreateEditWindowForMode`) | [B:EDITOR] |
| `GlobalWorldEditorWindow` | `globalworldeditor_window.*` | Yes | No (`GlobalWorldEditorLoop`) | [B:LEGACY] |
| `DebugMenu` | `debugmenu.*` | Yes | No (F1 opener commented out in `code/user_input.cpp:UserInput::AdvanceMenus`) | [B:LEGACY] |
| `DebugKeyBindings::NetworkButton` → `NetworkWindow` | `debugmenu.cpp`, `network_window.*` | Yes | **Yes** — `ControlToggleNetworkStats` (F9) under `NETWORK_STATS_ENABLED` [C] `code/user_input.cpp:UserInput::Advance` | [B:FULL] |
| `CheatWindow` | `cheat_window.*` | `Create()` empty unless `CHEATMENU_ENABLED` | `TARGET_DEBUG` (F11, only when no window is open) | [B:DEBUG] |
| Sound editor/stats/profile/parameter windows | `soundeditor_window.*`, `soundstats_window.*`, `sound_profile_window.*`, `soundparameter_window.*` | Bodies inside `#ifdef SOUND_EDITOR` | `TARGET_DEBUG` only | [B:DEBUG] |
| `GrabberWindow` | `grabber_window.*` | `#ifdef AVI_GENERATOR` (TARGET_DEBUG only) | Not even in DEBUG (no caller of `DebugKeyBindings::GrabberButton`) | [B:DEAD] |
| `GestureWindow` | `gesture_window.*` | `#if defined(GESTURE_EDITOR) && defined(USE_SEPULVEDA_HELP_TUTORIAL)` | No | [B:DEAD] |
| Front-end layer | `code/game_menu.*`, `code/interface/mainmenus.*`, `code/UI/*` | Yes | Yes (details §5, §6, §12) | [B:FULL] |

### 1.4 Integration into the frame

| Call | Where | Behaviour |
|---|---|---|
| `EclInitialise(screenW, screenH)` | `code/user_input.cpp:UserInput::UserInput` (constructed in `code/app.cpp:App::Initialise`); again in `code/interface/prefs_screen_window.cpp:AdjustWindowPositions` after a resolution change | Stores screen size, dirties the full screen [C] |
| `EclUpdateMouse(x, y, lmb, rmb)` then `EclUpdate()` | `code/user_input.cpp:UserInput::AdvanceMenus`, called from `UserInput::Advance` once per main-loop iteration in every loop (main menu, location, editor) [C] `code/main.cpp` | `x,y` = `g_target->X()/Y()` (the game cursor, which follows the mouse or the controller-emulated cursor); `lmb = controlEvent(ControlEclipseLMousePressed)` (level: "mouse left pressed"); `rmb = controlEvent(ControlEclipseRMousePressed)` (bound to "mouse right **down**", an edge) [C] `data/input/input_preferences.txt`. After pumping, if `ControlEclipseLMouseDown` fired over any window the event is suppressed (`InputManager::suppressEvent`) so the game world does not also receive the click [C] |
| `EclUpdateKeyboard(keyCode, shift, ctrl, alt, ascii)` | `code/lib/input/inputdriver_win32.cpp` `WM_CHAR` case | `keyCode = wParam` (a UTF-16 code unit: the window class is registered with `RegisterClassW`/`CreateWindowW` [C] `code/lib/window_manager_win32.cpp`), `ascii = (unsigned char) wParam`, modifiers from `g_keys[KEY_SHIFT/KEY_CONTROL/KEY_ALT] == 1` [C]. Consequence: widgets receive **characters, not virtual keys** (§4.1). There is no `WM_KEYDOWN` path into Eclipse and no IME handling (`WM_IME_*` never handled) [C] grep |
| `EclRender()` | `code/user_input.cpp:UserInput::Render`, inside `g_editorFont.BeginText2D()/EndText2D()` with `GL_BLEND` on, culling off, depth test off, depth writes off; restored afterwards | 2D orthographic projection matching the viewport, shifted −0.325 px [C] `code/lib/text_renderer.cpp:TextRenderer::BeginText2D`. Position in the frame: after `MarkerSystem`, `MultiwiniaHelp`, `ShamanInterface`; before the game cursor, in-game chat lines, `Camera::Render`, `TaskManagerInterface::Render`, debug renderer, control help, fade, the Multiwinia HUD (`Multiwinia::Render`) and the FPS counter; the menu-transition overlay is drawn after everything, just before `Flip` (`AdvanceMenuTransition`) [C] `code/renderer.cpp:Renderer::RenderFrame` |
| `EclShutdown()` | `code/main.cpp:Finalise` | Deletes every window (and through `~EclWindow` every button) and every dirty rect [C] |
| `WindowsOnScreen()` / `RemoveAllWindows()` | `code/main.cpp` | `WindowsOnScreen` = window list non-empty. `RemoveAllWindows` = if in a location warp the cursor to screen centre, then remove `windows[0]` until the list is empty [C] |

Controller mouse emulation [C] `data/input/input_preferences.txt`: `EclipseLMouseDown = alias ShowMouseCursor && XInput a down`, `EclipseLMousePressed = alias ShowMouseCursor && XInput a pressed`, `ShowMouseCursor = XInput leftbumper pressed` — holding LB turns A into the left mouse button.

---
## 2 Eclipse core (`contrib/eclipse`)

Exactly two levels: windows contain buttons; button coordinates are relative to their window [C] `contrib/eclipse/eclwindow.h` header comment. All identity is by **name string**; the sentinel `"None"` means "no window/button" everywhere [C] `contrib/eclipse/eclipse.cpp`.

### 2.1 Data model

**`EclWindow`** [C] `contrib/eclipse/eclwindow.h:EclWindow`, `contrib/eclipse/eclwindow.cpp:EclWindow::EclWindow`

| Field | Type | Default (ctor) | Meaning |
|---|---|---|---|
| `m_x, m_y, m_w, m_h` | int | 0, 0, 0, 0 | Screen-space rectangle in pixels, origin top-left |
| `m_name` | `char[256]` (`SIZE_ECLWINDOW_NAME`) | ctor argument (`SetName`: names with `strlen > 256` are silently not copied, leaving the buffer uninitialised) | Unique key for every lookup |
| `m_title` | `UnicodeString` (`SetTitle` rejects `Length() > 256`, `SIZE_ECLWINDOW_TITLE`) | `"New Window"` | Title |
| `m_movable` | bool | true (`SetMovable(true)`) | Drag-to-move allowed |
| `m_resizable` | bool | true | Drag-edge-to-resize allowed |
| `m_dirty` | bool | false | Needs redraw (§2.10) |
| `m_buttons` | `LList<EclButton*>` | empty | Owned children; **index 0 = most recently registered** (`PutDataAtStart`) |
| `m_currentTextEdit` | `char[256]` | `"None"` | Name of the button receiving keystrokes |

Destructor deletes every button in `m_buttons` [C] `EclWindow::~EclWindow`.

**`EclButton`** [C] `contrib/eclipse/eclbutton.h:EclButton`, `contrib/eclipse/eclbutton.cpp`

| Field | Type | Default | Meaning |
|---|---|---|---|
| `m_name` | `char[256]` (`SIZE_ECLBUTTON_NAME`) | `"New Button"` (names with `strlen > 256` are ignored by `SetProperties`) | Key within the window |
| `m_x, m_y, m_w, m_h` | int | 0 | Window-relative rectangle |
| `m_caption` | `UnicodeString` | empty | Caption |
| `m_tooltip` | `UnicodeString` | `" "` | Tooltip text (never displayed, §2.11) |
| `m_inactive` | bool | false | Eclipse-level "inactive" flag: skipped by Darwinia navigation (§3.3), greyed and click-ignored by most front-end widgets (§5) |
| `m_parent` (protected) | `EclWindow*` | NULL | Set by `RegisterButton` |

**`DirtyRect`** `{int m_x, m_y, m_width, m_height}` [C] `contrib/eclipse/eclipse.h:DirtyRect`.

**Module state** (file statics) [C] `contrib/eclipse/eclipse.cpp`

| Static | Initial | Role |
|---|---|---|
| `windows` (`LList<EclWindow*>`) | empty | Z-ordered window list, **index 0 = front-most** |
| `dirtyrects` (`LList<DirtyRect*>`) | empty | Accumulated dirty rectangles |
| `clearDraw` (fn ptr `(x,y,w,h)`) | NULL | Optional background-clear callback — never registered by Multiwinia |
| `tooltipCallback` (fn ptr `(EclWindow*, EclButton*)`) | NULL | Optional tooltip callback — never registered by Multiwinia |
| `mouseX, mouseY, lmb, rmb` | 0, 0, false, false | Last mouse state |
| `buttonDownMouseX/Y` | 0 | Position used to resolve the release target (updated every held frame) |
| `mouseDownWindowX/Y` | 0 | Grab offset for window dragging |
| `tooltipTimer` | 0 | Millisecond timestamp when the tooltip becomes due |
| `maximiseOldX/Y/W/H` | 0 | Saved rectangle for un-maximise |
| `mouseDownWindow`, `windowFocus`, `popupWindow`, `maximisedWindow` | `"None"` | Window-name state slots (`char[256]`) |
| `currentButton` | `"None"` | Highlighted/pressed button name |
| `screenW, screenH` | 0 | From `EclInitialise` |

### 2.2 Registration, removal and z-order [C] `contrib/eclipse/eclipse.cpp`

`EclRegisterWindow(window, parent = NULL)`:
1. Duplicate check is a no-op (`if (EclGetWindow(name)) {}` has an empty body): duplicate names are accepted, and only the front-most of them is findable.
2. If `parent` is given **and** the window is at (0,0): automatic placement with `r1, r2 = AppRandom()/APP_RAND_MAX`:
   - `left = screenW/2 − parent.x`; `above = screenH/2 − parent.y`
   - `x = (left > w/2) ? parent.x + parent.w·r1 : parent.x − w·r1`
   - `y = (above > h/2) ? parent.y + parent.h·r2 : parent.y − (h/2)·r2`
3. `MakeAllOnScreen()`: `x = max(x, 0)`, `y = max(y, 0)`, then `if x + w > screenW: x = screenW − w`, `if y + h > screenH: y = screenH − h` — a window larger than the screen ends with a negative origin.
4. `windows.PutDataAtStart(window)` → **a new window is always front-most**.
5. `window->Create()` — subclasses build their buttons here.
6. `EclDirtyWindow(window)`.

Registration does **not** give focus; `DarwiniaWindow`'s constructor does (§3.1).

`EclRemoveWindow(name)`: find by name; dirty its rectangle; unlink it; if `mouseDownWindow` or `windowFocus` named it, reset that slot to `"None"`; `delete window`. Unknown names are a silent no-op.

`EclBringWindowToFront(name)`: move to index 0 and dirty — called on every left press that lands on a window (§2.4).

`EclSetWindowPosition(name, x, y)`: dirty, move, dirty the new rectangle (no clamping). `EclSetWindowSize(name, w, h)`: `window->Remove()`, dirty, set size, **`window->Create()` again** (every button rebuilt), dirty. Contrast `EclWindow::SetPosition/SetSize`, which only assign and dirty (no rebuild).

`EclWindow::RegisterButton(b)`: `b->SetParent(this)`; `m_buttons.PutDataAtStart(b)`; **auto-grow**: if `b.y + b.h + 10 > m_h` then `SetSize(m_w, b.y + b.h + 10)` and `MakeAllOnScreen()`; dirty [C] `contrib/eclipse/eclwindow.cpp:EclWindow::RegisterButton`. Windows therefore grow downwards to fit content and are pushed up to stay on-screen.

`EclWindow::RemoveButton(name)`: iterates from the **back** of `m_buttons` (oldest registration first), deletes the first match and stops. **Resolved:** P17a said "deletes the first match", P18 §12.3 said "removes all matches"; the code removes exactly one button — the *oldest*-registered button with that name — despite its comment "we're removing ALL buttons with this name" [C]. `EclWindow::GetButton(name)` returns the first match from index 0, i.e. the *newest* registration. With duplicate names, lookup and removal therefore address different buttons.

`EclGenerateUniqueWindowName(name)` returns `name`, `name2`, `name3`, … (first unused) in a static buffer [C]; unused by `code/`.

### 2.3 Hit testing

- `EclGetWindow(x, y)`: first window from index 0 (front) with `x ∈ [m_x, m_x+m_w]` and `y ∈ [m_y, m_y+m_h]` — **inclusive** of the far edges [C].
- `EclWindow::GetButton(x, y)` (window-relative): first button from index 0 with inclusive bounds [C]. Overlapping buttons: the later-registered one receives input and is drawn on top (rendering is back-to-front, §2.10).
- `EclMouseInButton(window, button)`: the window must be the top window under the cursor **and** the cursor inside the button's inclusive rectangle [C].
- `EclMouseInWindow(window)` ≡ `EclGetWindow(mouseX, mouseY) == window` [C].

### 2.4 Mouse state machine and capture semantics [C] `contrib/eclipse/eclipse.cpp:EclUpdateMouse`

Every call first stores the new position and dirties the window under the cursor. Exactly one LMB branch runs (comparing the new `_lmb` with the stored `lmb`); all three RMB branches are empty stubs — **Eclipse ignores the right button**.

```mermaid
stateDiagram-v2
  [*] --> Idle
  Idle --> Idle: move without LMB - hover tracking and tooltip timer
  Idle --> Pressed: LMB edge down - focus, bring to front, MouseDown
  Pressed --> Held: LMB still held next frame
  Held --> Held: LMB held - MouseDown every frame, or drag-move, or drag-resize
  Pressed --> Idle: LMB up - MouseUp on button under buttonDownMouse
  Held --> Idle: LMB up - MouseUp on button under buttonDownMouse
```

| Branch | Condition | Actions |
|---|---|---|
| **Move / hover** | `!_lmb && !lmb && !_rmb && !rmb` | Only the *focused* window is examined. Button under cursor: if its name ≠ `currentButton` → `currentButton = name`, dirty the window, `tooltipTimer = now + 1000 ms`; else if `now > tooltipTimer` → `tooltipCallback(window, button)` (every frame while hovering). No button under cursor: if `currentButton ≠ "None"` → reset to `"None"`, dirty, `tooltipCallback(NULL, NULL)`. No focused window: same reset without the dirty. Un-focused windows still show hover visually because rendering uses `EclMouseInButton` (§2.10). |
| **LMB down** (edge) | `_lmb && !lmb` | `buttonDownMouse = mouse`. If a window is under the cursor: dirty the old focus; **focus = this window**; **bring to front**; `mouseDownWindow = this`. Button under cursor → `currentButton = name`, `button->MouseDown()`. No button → `currentButton = "None"`, `window->MouseEvent(lmb=true, rmb=false, up=false, down=true)`, and if movable record the grab offset `mouse − window.xy`. No window under cursor → dirty the old focus and set focus `"None"`. Then `lmb = true`. |
| **LMB held** | `_lmb && lmb` | `buttonDownMouse = mouse` (**updated every frame**). If `mouseDownWindow` is a live window: button under cursor → `button->MouseDown()` **again, every frame** (whatever the press started on). Else if `currentButton == "None"` (press began on the background): resize test on the *previous* cursor position — within ±4 px (exclusive) of the bottom edge → `newH = mouseY − y`; within ±4 px of the right edge → `newW = mouseX − x`; if either and the window is resizable → clamp `newW ≥ 60`, `newH ≥ 40`, `EclSetWindowSize` (Remove/Create rebuild every frame of the drag). Otherwise, if movable → `EclSetWindowPosition(mouse − grabOffset)` (no clamping while dragging). A frame in which the cursor passes over a button during a window drag calls that button's `MouseDown` instead of moving the window. |
| **LMB up** (edge) | `!_lmb && lmb` | `mouseDownWindow = "None"`, `lmb = false`. Window under the **last `buttonDownMouse`**: a button there → dirty, `button->MouseUp()`; no button → `window->MouseEvent(lmb=true, rmb=false, up=true, down=false)` then `EclRemovePopup()`. No window → `EclRemovePopup()`. |

Design-relevant consequences [C-derived]:
- **`MouseUp` is the "click" event** and fires on whichever button lies under the release point; the press need not have started on it. There is **no mouse capture and no click cancellation** (press on A, release on B clicks B).
- **`MouseDown` is a held/repeat event**: called on the press frame and on every held frame while the cursor is over a button. Widgets wanting an edge test `ControlEclipseLMouseDown` themselves (`InputScroller::MouseDown`, §4.2).
- Dragging a widget (e.g. a scroll thumb) only continues while the cursor stays over that widget.
- Window dragging and resizing only start when the press lands on the window background.
- `MouseEvent` is the only window-level mouse hook; no live window overrides it [C] grep.

### 2.5 Focus rules

| Event | Focus result | Ref |
|---|---|---|
| LMB press on a window | that window (and it goes to the front) | [C] `EclUpdateMouse` |
| LMB press on empty screen | `"None"` | [C] |
| `DarwiniaWindow` constructed | `EclSetCurrentFocus(m_name)` — Darwinia windows take focus at construction, before registration | [C] `code/interface/darwinia_window.cpp:DarwiniaWindow::DarwiniaWindow` |
| `DarwiniaWindow` destroyed | focus = name of `windows[0]` if any (the window has already been unlinked by `EclRemoveWindow`, so this is the new front window) | [C] `DarwiniaWindow::~DarwiniaWindow` |
| `EclRemoveWindow` of the focused window | `"None"` (then the Darwinia destructor re-assigns as above) | [C] |
| `EclMaximiseWindow(name)` | that window | [C] |
| `EclSetCurrentFocus(name)` | `name` if `strlen < 256`; no existence check | [C] |

Only the focused window receives keyboard input (§2.6), hover/tooltip tracking (§2.4) and Darwinia controller navigation (§3.3). `EclRender` passes `hasFocus = (name == windowFocus)`; both full-screen front-end window classes draw **only while focused** (§5.1, §6.7), so at most one full-screen page is visible at a time.

### 2.6 Keyboard routing

`EclUpdateKeyboard(...)` → focused window's virtual `Keypress(...)` → default `EclWindow::Keypress` forwards to the button named `m_currentTextEdit` (if it exists) [C] `contrib/eclipse/eclwindow.cpp:EclWindow::Keypress`. `BeginTextEdit(name)` / `EndTextEdit()` set/reset that name. `EclIsTextEditing()` ≡ a focused window exists and its `m_currentTextEdit ≠ "None"` — used by the lobby chat to avoid re-opening the chat box while typing and by `ChatInputField` to switch between the prompt and the typed text [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::UpdateMultiplayerPage`, `code/interface/chatinput_window.cpp:ChatInputField::Render`. There is no tab order and no focus traversal at the Eclipse level; navigation between buttons is a Darwinia-layer feature driven by controller bindings (§3.3, §9).

### 2.7 Popups and maximise

- `EclRegisterPopup(w)`: remove the existing popup, record the name, register. `EclRemovePopup()` is called on every LMB release that does not land on a button (including clicks on any window background) and on clicks outside all windows → a popup closes on any non-button click [C]. No Multiwinia code registers popups [C] grep (drop-downs use their own singleton, §4.6).
- `EclMaximiseWindow(name)`: un-maximise any previous window; save the rectangle; set `maximisedWindow`, `mouseDownWindow` and focus to it; `SetPosition(0,0)`, `SetSize(screenW, screenH)` (no rebuild). While a window is maximised `EclRender` calls `clearDraw(rect)` and renders only that window — **`clearDraw` is NULL in Multiwinia, so maximising would dereference a null function pointer** [C code; I consequence]; unused by `code/` [C] grep. `EclUnMaximise()` restores the rectangle and dirties the full screen.

### 2.8 Dirty rectangles and render order

- `EclDirtyRectangle(x, y, w, h)` allocates a `DirtyRect`, appends it, and marks every overlapping window dirty (`EclRectangleOverlap` treats touching edges as overlapping: `maxtop ≤ minbottom && maxleft ≤ minright`).
- `EclDirtyWindow(w)`: if not already dirty → set dirty and add its rectangle.
- `EclRender()` (non-maximised): calls `clearDraw` for every dirty rectangle if registered (never in Multiwinia), then **for i = Size−1 down to 0** renders every window whose `m_dirty` is set → back-most first, front-most last (painter's order). The line that would clear `m_dirty` after rendering is commented out [C].
- `EclResetDirtyRectangles()` (frees rectangles, clears flags) is never called from `code/` [C] grep.

Net effect [C-derived]: every window is dirty from registration onwards and is **re-rendered every frame**; the dirty-rectangle list grows by one entry per `EclSetWindowPosition` / `EclSetWindowSize` / `EclRemoveWindow` / `EclDirtyRectangle` / first-time `EclDirtyWindow` and is only freed at `EclShutdown` (unbounded but small leak, e.g. while dragging a window). A re-implementation should treat the UI as a full immediate-mode redraw each frame and drop dirty rectangles.

### 2.9 Updates

`EclUpdate()` calls `Update()` on every window from index 0 (front) to the back [C] `contrib/eclipse/eclipse.cpp:EclUpdate`. A window removed during iteration shifts the indices (a window can be skipped for that frame) [I]. `EclWindow::Update` is empty; the Darwinia layer puts navigation there (§3.3).

### 2.10 Window and button rendering at L1

`EclWindow::Render(hasFocus)` renders buttons **from the back of `m_buttons` to index 0** (oldest first, newest on top) calling `button->Render(m_x + b.x, m_y + b.y, highlighted, clicked)` with `highlighted = EclMouseInButton(this, b) || m_currentTextEdit == b.name` and `clicked = hasFocus && EclGetCurrentClickedButton() == b.name` [C] `contrib/eclipse/eclwindow.cpp:EclWindow::Render`. `EclGetCurrentClickedButton()` = `currentButton` while LMB is held, else `"None"` [C]. `EclButton::Render` is empty.

### 2.11 Tooltips

Tooltip delay **1000 ms** of hovering the same button in the focused window, timed with `EclGetAccurateTime()` = Win32 `GetTickCount()` (ms) [C]. `EclRegisterTooltipCallback` is never called in `code/` → **no tooltips are ever shown**; every `m_tooltip` string ("Close this window", "Value left", "Value right", " ") is dead data [C] grep.

### 2.12 Complete public API reference

Every function declared in `contrib/eclipse/eclipse.h`, `eclwindow.h`, `eclbutton.h` [C]:

| Function | Semantics | Used by `code/` |
|---|---|---|
| `EclInitialise(screenW, screenH)` | store screen size; `EclDirtyRectangle(0,0,W,H)` | yes (§1.4) |
| `EclUpdateMouse(x, y, lmb, rmb)` | §2.4 | yes |
| `EclUpdateKeyboard(keyCode, shift, ctrl, alt, ascii)` | §2.6 | yes |
| `EclRender()` | §2.8, §2.7 | yes |
| `EclUpdate()` | §2.9 | yes |
| `EclShutdown()` | `EmptyAndDelete` windows and dirty rects | yes |
| `EclRegisterWindow(window, parent=NULL)` | §2.2 | yes |
| `EclRemoveWindow(name)` | §2.2 | yes |
| `EclRegisterPopup(window)` / `EclRemovePopup()` | §2.7 | no (internal call of `EclRemovePopup` only) |
| `EclBringWindowToFront(name)` | §2.2 | internal |
| `EclSetWindowPosition(name, x, y)` / `EclSetWindowSize(name, w, h)` | §2.2 | internal (drag) |
| `EclGetWindowIndex(name)` | index in z-order, −1 if absent | internal |
| `EclGetWindow(name)` / `EclGetWindow(x, y)` | §2.2, §2.3; NULL if none | yes (everywhere: "toggle" openers check `EclGetWindow(name)` first) |
| `EclMouseInWindow(window)` / `EclMouseInButton(window, button)` | §2.3 | yes (`DarwiniaButton::UpdateButtonHighlight`, `GameMenuWindow::Render`) |
| `EclIsTextEditing()` | §2.6 | yes |
| `EclRegisterTooltipCallback(cb)` | §2.11 | no |
| `EclMaximiseWindow(name)` / `EclUnMaximise()` | §2.7 | no |
| `EclGetCurrentButton()` | `currentButton` | no |
| `EclGetCurrentClickedButton()` | §2.10 | yes (`EclWindow::Render`, `GameMenuWindow::Render`) |
| `EclGetCurrentFocus()` / `EclSetCurrentFocus(name)` | §2.5 | yes |
| `EclGenerateUniqueWindowName(name)` | §2.2 | no |
| `EclGetWindows()` | pointer to the z-ordered list | yes (`RemoveAllWindows`, `WindowsOnScreen`, cheat hot-key guard, `DarwiniaWindow` destructor, `AdjustWindowPositions`) |
| `EclRegisterClearFunction(fn)` | §2.8 | no |
| `EclDirtyWindow(name)` / `EclDirtyWindow(window)` / `EclDirtyRectangle(x,y,w,h)` | §2.8 | yes (`ScrollBar::SetNumRows`/`SetCurrentValue`/`SetWinSize`, `FileDialog`, `MultiwiniaFileDialog`) |
| `EclRectangleOverlap(...)` | inclusive overlap test | internal |
| `EclResetDirtyRectangles()` / `EclGetDirtyRects()` | §2.8 | no |
| `EclGetAccurateTime()` | Win32 `GetTickCount()`; other platforms `gettimeofday` ms since first call | internal |
| `EclGetScreenW()` / `EclGetScreenH()` | stored size | internal (`MakeAllOnScreen`) |
| `EclWindow::SetName / SetTitle / SetPosition / SetSize / SetMovable / MakeAllOnScreen` | §2.1, §2.2 | yes |
| `EclWindow::RegisterButton / RemoveButton / GetButton(name) / GetButton(x,y)` | §2.2, §2.3 | yes |
| `EclWindow::BeginTextEdit / EndTextEdit` | §2.6 | yes |
| `EclWindow::Create / Remove / Update / Render / Keypress / MouseEvent` (virtual) | defaults: empty / empty / empty / §2.10 / forward to text-edit target / empty | overridden by L2 |
| `EclButton::SetProperties(name, x, y, w, h, caption, tooltip)` | copy fields; the "caption defaults to name" fallback tests `!&_caption` (address of a reference, always false) so it never triggers | yes |
| `EclButton::SetCaption / SetTooltip / SetParent` | assign | yes |
| `EclButton::Render / MouseUp / MouseDown / MouseMove / Keypress` (virtual) | empty defaults; `MouseMove` is never called by Eclipse | overridden by L2 |

### 2.13 Eclipse quirks not to replicate

| # | Quirk | Ref |
|---|---|---|
| Q1 | Click fires on the release target, not the press target; no capture | `EclUpdateMouse` |
| Q2 | `MouseDown` fires every held frame | `EclUpdateMouse` held branch |
| Q3 | Dirty flags never cleared; dirty-rect list leaks until shutdown | `EclRender`, `EclDirtyRectangle` |
| Q4 | Resize rebuilds all buttons every drag frame (`Remove`+`Create`) — widget state not stored outside the window is lost | `EclSetWindowSize` |
| Q5 | `RemoveButton` removes only the oldest of several same-named buttons while `GetButton` returns the newest | `EclWindow::RemoveButton`, `EclWindow::GetButton` |
| Q6 | Name-keyed lookup everywhere; duplicate names silently shadow; over-long names leave fields uninitialised | `EclRegisterWindow`, `EclWindow::SetName` |
| Q7 | `EclButton::SetProperties` caption fallback is dead (`!&_caption`) | `contrib/eclipse/eclbutton.cpp:EclButton::SetProperties` |
| Q8 | Tooltip machinery present but never registered | §2.11 |
| Q9 | Maximise path dereferences the NULL `clearDraw` | `EclRender` |
| Q10 | Right mouse button ignored entirely | `EclUpdateMouse` RMB branches |

---
## 3 Darwinia layer: `DarwiniaWindow` and `DarwiniaButton`

[B:FULL] `code/interface/darwinia_window.*`. Every Multiwinia window derives from `DarwiniaWindow`, every widget from `DarwiniaButton` (exceptions: `ScrollBarButton`, editor `FileButton` and `GuideGrid` derive directly from `EclButton`) [C] §12.

### 3.1 `DarwiniaWindow` fields, constructor, destructor [C] `code/interface/darwinia_window.h:DarwiniaWindow`

| Field | Type | Default | Meaning |
|---|---|---|---|
| `m_buttonOrder` | `LList<EclButton*>` | empty | Ordered list for controller/keyboard navigation; **opt-in** — a screen must `PutData` each navigable widget (editor windows never do) |
| `m_currentButton` | int | 0 | Index into `m_buttonOrder` of the navigation cursor |
| `m_buttonChangedThisUpdate` | bool | false | Set when navigation moved this update (cancels mouse-hover highlight mode) |
| `m_skipUpdate` | bool | false | One-shot: skip the next `Update` (auto-clears) |
| `m_closeable` | bool | true | Whether `MenuClose` triggers `m_backButton` |
| `m_lockMenu` | bool | false | When true `Update` returns immediately (navigation frozen) |
| `m_details` | `InputDetails` | default | Details of the last activate event (re-initialised each update) |
| `m_backButton` | `EclButton*` | NULL | Button whose `MouseUp` is invoked by `MenuClose` |

Constructor [C] `DarwiniaWindow::DarwiniaWindow`: title = `LANGUAGEPHRASE(name)` if the language table exists, else the raw name; then `StrUpr()` (upper-cased, locale-dependent, `setlocale(LC_CTYPE, language)` is set in `App::SetLanguage`); then `EclSetCurrentFocus(m_name)` (**new windows take focus at construction**). Because `LangTable::LookupPhrase` returns the key itself for a missing phrase [C] `code/lib/language_table.cpp:LangTable::LookupPhrase`, window names double as phrase keys with a readable fallback. The title is, however, **not drawn** when a language table exists (§3.4).

Destructor: if any window remains, `EclSetCurrentFocus(windows[0]->m_name)` [C] `DarwiniaWindow::~DarwiniaWindow`.

### 3.2 Create / Remove / client rectangle / large-menu scaling

- `Create()`: registers a `CloseButton` named `"Close"` at `(m_w − 12, 2, 10, 10)`, caption `"Close"`, tooltip `"Close this window"`, `m_iconised = true` [C] `DarwiniaWindow::Create`. Subclasses that do not call the base `Create` (`MessageDialog`, `GameOptionsWindow` and all its subclasses, `GameMenuWindow`, `MainEditWindow`) have no close box; `LandscapeGuideGridWindow` calls it last. A subclass that registers before `m_w` is final gets a mis-placed close box (e.g. `DropDownWindow` is registered at `m_w = 0`, so its inherited close box sits at x = −12) [C] `code/interface/drop_down_menu.cpp:DropDownMenu::CreateMenu`.
- `Remove()`: repeatedly takes `m_buttons[0]` and calls `RemoveButton(name)` until no buttons remain; empties `m_buttonOrder`; `m_currentButton = 0`; `EndTextEdit()` [C] `DarwiniaWindow::Remove`.
- Client rectangle: `X1 = 2`, `X2 = m_w − 2`, `Y1 = GetMenuSize(15) + 1`, `Y2 = m_h − 2` [C] `GetClientRectX1/Y1/X2/Y2`.
- **Large-menu scaling** [C] `DarwiniaWindow::GetMenuSize`, `DarwiniaWindow::SetMenuSize`: when `g_app->m_largeMenus` is true, `scale = 0.96 × screenH / 460`; `GetMenuSize(v) = (int)(v × scale)`; `SetMenuSize(w, h)` multiplies both by `scale` before `SetSize`. Otherwise identity. `m_largeMenus = (pref LargeMenus == 2)` read in `code/app.cpp:App::Initialise`; the Other Options Apply re-applies the stored value but the shipped Other Options page has **no** Large-Menus control (the member round-trips the value read at construction) [C] `code/interface/prefs_other_window.cpp:ApplyOtherButton::MouseUp`, `PrefsOtherWindow::PrefsOtherWindow`. Resolved: P17a said the flag is "also toggled from Other Options"; it can only change through the preferences file. Examples: scale 1.2522 at H = 600, 1.6028 at H = 768, 2.2539 at H = 1080.

### 3.3 `Update`: controller/keyboard navigation [C] `DarwiniaWindow::Update`

Executed every frame by `EclUpdate` for every window; navigation acts only on the focused window:

```text
Update():
  if m_lockMenu: return
  m_buttonChangedThisUpdate = false
  if m_skipUpdate: m_skipUpdate = false; return
  if EclGetCurrentFocus() != m_name: return
  if m_buttonOrder.Size() > 0:
    if controlEvent(MenuDown):
      b = dynamic_cast<DarwiniaButton*>(m_buttons[m_currentButton])   # BUG: indexes m_buttons, not m_buttonOrder
      if b == NULL or not b.SpecialScroll(+1):
        m_buttonChangedThisUpdate = true
        repeat at most Size+1 times: m_currentButton += 1; wrap to 0 at Size; stop at first entry with !m_inactive
    if controlEvent(MenuUp):
      same lookup with SpecialScroll(-1)
      if not consumed:
        m_buttonChangedThisUpdate = true
        loop "while num <= Size" whose counter is never incremented: m_currentButton -= 1, wrap to Size-1,
          stop at first entry with !m_inactive  -> infinite loop if every entry is m_inactive (BUG)
  m_details = InputDetails()
  if (g_app->m_atMainMenu and controlEvent(MainMenuActivate, m_details)) or controlEvent(MenuActivate, m_details):
    m_buttonOrder.GetData(m_currentButton)?.MouseUp()
  if m_closeable and controlEvent(MenuClose) and m_backButton: m_backButton.MouseUp()
```

`SpecialScroll(direction)` is a hook (default returns false) that lets a widget consume up/down (e.g. `GameServerButton` scrolls the browser list when the cursor is on the first/last visible row) [C] `DarwiniaButton::SpecialScroll`, `code/UI/GameServerButton.h:GameServerButton::SpecialScroll`. Because of the index bug the widget consulted is `m_buttons[m_currentButton]` — the registration list (newest first), unrelated to the navigation cursor [C].

`SetCurrentButton(b)`: set `m_currentButton` to `b`'s index in `m_buttonOrder` if present, else unchanged [C].

### 3.4 Window chrome rendering [C] `DarwiniaWindow::Render`

Order and exact parameters (screen pixels):

| Step | Primitive | Colour / state |
|---|---|---|
| 1 Body | Textured quad over the whole rectangle, `textures/interface_red.bmp` (on-disk asset 64×510, 8-bit, [C] file header), min filter `GL_LINEAR_MIPMAP_LINEAR`, mag `GL_LINEAR`, wrap `GL_REPEAT` both axes; tex-coords s ∈ [0, 8], t ∈ [0, 1] (code assumes 512×64: `texW = 1.0 × 512/64`) | `glColor4f(1,1,1,0.96)`; blend func inherited (normally `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`, restored by the text renderer after each string) |
| 2 Title bar | Quad from `m_y` to `m_y + GetClientRectY1() − 1` = `GetMenuSize(15)` px high, smooth-shaded | top `(199,214,220,255)` → bottom `(112,141,168,255)` |
| 3 Border | Four 2 px lines on the rectangle edges | `(199,214,220,255)` |
| 4 Outer frame | 1 px line loop `(x−2, y−2)…(x+w+1, y+h+1)` | `(42,56,82,255)` |
| 5 Title text | `g_gameFont` ("speccy"), shadow mode, centred at `x + w/2`, baseline `y + 9` (large menus: `y + fontSize/2`), size `GetMenuSize(12)`, drawn **twice** | `glColor4ub(255,255,150,30)`; text = `LANGUAGEPHRASE("multiwinia_mainmenu_title")` whenever a language table exists, **ignoring `m_title`** (the original title lines are commented out); `m_title` only when no table exists |
| 6 Buttons | `EclWindow::Render` (§2.10) | — |

### 3.5 `DarwiniaButton` [C] `DarwiniaButton::*`

Fields: `m_fontSize` 11.0, `m_centered` false, `m_disabled` false, `m_highlightedThisFrame` false, `m_mouseHighlightMode` false.

`SetShortProperties(name, x, y, w, h, caption, tooltip)`: caption defaults to `UnicodeString(name)` if empty; `w == −1` → `caption.Length() × 7 + 9` (or `strlen(name) × 7 + 9` if the caption argument is empty); `h == −1` → 15; then `SetProperties`.

`UpdateButtonHighlight()` (called from `Render`): if the parent moved its navigation cursor this update → `m_mouseHighlightMode = false`. If the cursor is in the button **and** the input mode is `INPUT_MODE_KEYBOARD`: on the first such frame → `parent->SetCurrentButton(this)`, `m_highlightedThisFrame = m_mouseHighlightMode = true`; when the cursor leaves → `m_highlightedThisFrame = false` [C].

Highlight decision in `Render`: the Eclipse `highlighted` flag is discarded unless `m_mouseHighlightMode`; it is forced true if this button is `parent->m_buttonOrder[parent->m_currentButton]`. Mouse hover and the controller cursor therefore share one visual state. (The cast `(DarwiniaWindow*)m_parent` is unconditional — every `DarwiniaButton` must live in a `DarwiniaWindow`.)

| State | Background | Edges | Caption |
|---|---|---|---|
| normal | quad `(107,37,39,64)` | 1 px: left + top `(100,34,34,200)`, right + bottom `glColor4f(0.1,0,0,1)` | white `(1,1,1,1)` additive; disabled `(0.5,0.5,0.5,1)` |
| highlighted | smooth vertical gradient `(199,214,220,255)` → `(112,141,168,255)` | none | shadow ("ink") mode, `(255,255,150,30)`, drawn twice; disabled `(128,128,75,30)` |
| clicked | gradient `(255,255,255,255)` → `(162,191,208,255)` | none | as highlighted |

Caption font `g_editorFont`, size `m_fontSize` (not menu-scaled), baseline `y = 7.5 + realY + (m_h − m_fontSize)/2`, x = `realX + 5`, or centred at `realX + m_w/2` when `m_centered`. `SetDisabled(b)` only changes colours — **disabled buttons still receive `MouseUp`**; each subclass must check `m_disabled` itself (only `DropDownMenu::MouseUp` does) [C].

### 3.6 `DarwiniaButton` subclasses in `darwinia_window.*` [C]

| Class | Behaviour | Render | Live users |
|---|---|---|---|
| `BorderlessButton` | `SetShortProperties`: width from the **name** length only (`strlen(name)×7+9`), height 15, no caption fallback | Forced "clicked" when it is the navigation cursor. Clicked: gradient as the DarwiniaButton highlight, shadow caption `(255,255,150,30)` ×2. Otherwise: no fill; if `highlighted` → **calls `parent->SetCurrentButton(this)` from inside `Render`** and draws 1 px edges left/top `(100,34,34,250)`, right/bottom black; caption white, plus an extra 50 %-alpha white pass when highlighted. Caption size `GetMenuSize(m_fontSize)`, baseline `realY + 10` | base of `InputField`, `DropDownMenuOption` |
| `CloseButton` | `MouseUp` → `EclRemoveWindow(parent)` | iconised: quad `(60,37,39,64)` (`(160,137,139,64)` when highlighted/clicked), edges left/top `(0,0,150,100)`, right/bottom `(0.1,0,0,1)`; non-iconised → DarwiniaButton look | Darwinia-style windows (NetworkWindow) |
| `GameExitButton` | `MouseUp` → `g_app->m_atMainMenu = true; renderer->StartFadeOut()` | DarwiniaButton | only `ReallyQuitWindow` ([B:LEGACY]) |
| `InvertedBox` | none (decorative inset panel) | quad `glColor4f(0.05,0,0,0.4)`; top/left black, right/bottom `(100,34,34,150)` | `NetworkWindow` and legacy windows |
| `LabelButton` | none | caption only, `g_editorFont` size 11.0 at `(realX+5, realY+10)`, white / grey if disabled | `LightsEditWindow` ([B:EDITOR]) |

### 3.7 Value-control factory [C] `DarwiniaWindow::CreateValueControl`, `RemoveValueControl`, `CreateColourControl`

`CreateValueControl(name, T *value, y, change, low, high, callback = NULL, x = −1, w = −1)` overloads map the pointer type to an `InputField` type:

| Pointer type | InputField type requested | Value adaptor |
|---|---|---|
| `bool*` | `TypeBool` (but `RegisterBool` stores `m_type = TypeChar`, defect F1) | `LocalBool` |
| `unsigned char*` | `TypeChar` | `LocalChar` |
| `int*` | `TypeInt` | `LocalInt` |
| `float*` | `TypeFloat` | `LocalFloat` |
| `double*` | `TypeDouble` | `LocalDouble` |
| `char*` (buffer) | `TypeString` — `high` is reused as the **buffer size** | `LocalString(buf, size)` |

Layout: `x` default 10; `w` default `m_w − 2x`; field height `GetMenuSize(15)`. Label = `LANGUAGEPHRASE(name)` if the language table exists and `ISLANGUAGEPHRASE(name)`, else the raw name. String types span `w`; numeric types span `w − 37` and add two `InputScroller` buttons: `"<name> left"` at `(field.x + field.w + 5, y, 15, 15)` caption `"<"`, tooltip `"Value left"`, `m_change = −change`; `"<name> right"` at `(field.x + field.w + 22, y, 15, 15)` caption `">"`, `m_change = +change`. `callback` (any `DarwiniaButton`) is "clicked" (`MouseUp`) on every value refresh (§4.1). `RemoveValueControl(name)` removes the field and both scrollers.

`CreateColourControl(name, int *rgba, y, callback, x = 10, w = m_w − x)`: a `ColourWidget` of width `w − 13`, height 15 (§4.3).

---
## 4 Widgets (inputs, scroll bars, check boxes, drop-downs, dialogs)

### 4.1 `InputField` — text input and validation [B:FULL] `code/interface/input_field.*`

`class InputField : BorderlessButton` [C] `code/interface/input_field.h:InputField`. Fields: `m_buf` (`UnicodeString` display/edit buffer), `m_type` ∈ {`TypeNowt` = 0, `TypeChar` = 1, `TypeBool` = 2, `TypeInt` = 3, `TypeFloat` = 4, `TypeDouble` = 5, `TypeString` = 6, `TypeUnicodeString` = 7}, `m_value` (`NetworkValue*`), `m_ownsValue`, `m_inputBoxWidth` (px), `m_callback` (`DarwiniaButton*`).

**Binding** [C] `InputField::Register*`: each `Register*` debug-asserts the field is still `TypeNowt`, wraps the caller's storage in a `Local*` adaptor that the field owns and deletes (`LocalChar/LocalBool/LocalInt/LocalFloat/LocalDouble` with `SetBounds(low, high)`; `LocalString(buf, size)`; `LocalUnicodeString(ref)`), or `RegisterNetworkValue(type, v)` (not owned — used by front-end fields bound to networked lobby options or window-owned adaptors, §4.12). Default bounds `[0, 1.0e4]`. Text conversions [C] `code/network/networkvalue.cpp`: int/bool/char → `"%d"`; float/double → `"%.2f"`; parsing with `swscanf("%d" | "%lf")`, float/double results passed through `iv_trunc_low_nibble`; string `Set` = `snprintf(buf, size, "%s")` with forced terminator.

**Editing state machine** [C] `InputField::MouseUp`, `InputField::Keypress`, `DarwiniaWindow::Remove`:

```mermaid
stateDiagram-v2
  [*] --> Display
  Display --> Editing: MouseUp - parent BeginTextEdit(name)
  Editing --> Editing: printable char, digit or backspace
  Editing --> Display: Enter or Esc - parse m_buf into value then Refresh
  Editing --> Display: parent Remove - EndTextEdit, no commit
  Editing --> Display: MouseUp on another InputField of the same window - that field becomes the target, this one is not committed
```

**Keypress** (`keyCode` = the `WM_CHAR` character, §1.4) [C] `InputField::Keypress`:

| Input | Effect |
|---|---|
| parent has no active text edit (`m_currentTextEdit == "None"`) | ignored |
| `ascii == '%'` | ignored (protects printf-style text drawing; note `ascii` is the low byte, so any UTF-16 unit ending in 0x25 is also rejected) |
| Backspace (8) | if non-empty, drop the last character (`UnicodeString::Truncate(Length())` shrinks by one because `Length()` = buffer size − 1 [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::Truncate`); string types write through (`FromUnicodeString`) and `Refresh()` |
| Enter (13) **or** Esc (27) | `EndTextEdit()`; `m_value->FromUnicodeString(m_buf)`; `Refresh()` — **Esc commits, it does not cancel** |
| `KEY_STOP` (190) | append `"."` — only reachable for character U+00BE "¾" under `WM_CHAR` (dead in practice [I]) |
| '0'–'9' (48–57) | append the digit; string types write through + `Refresh()` |
| 'A'–'Z' (65–90) on `TypeString` | append, lower-cased unless Shift is held (so Caps-Lock letters become lower-case); **no write-through** in this branch (the trailing write-through block below still runs) |
| anything else, Ctrl not held | append the raw UTF-16 unit (this is how '.', '-', lower-case and non-Latin letters arrive) |
| anything else with Ctrl held | ignored |
| (after the A–Z / other branches) | string types (`TypeString`, `TypeUnicodeString`) write through + `Refresh()` |

After every key `m_inputBoxWidth = len × 7 + 7` (`PIXELS_PER_CHAR = 7`) [C].

**Validation** [C]: numeric fields accept any characters while typing; on commit the buffer is parsed with `swscanf`; on a parse failure the stored value is unchanged and `Refresh` re-displays it. **Typed values are not clamped to the bounds** — only the scroller buttons apply `Bound()` (`NetworkValue::FromUnicodeString` vs `InputScroller::CheckInput`); networked option adaptors bound inside `Set` (§4.12). String fields write through on every keystroke, so the callback fires every keystroke. No maximum-length check beyond the target buffer's `snprintf` truncation.

`Refresh()`: `m_buf = value text`; if `m_callback` → `m_callback->MouseUp()` [C].

**Render** [C] `InputField::Render`:
- Edit well = right 40 % of the field (`m_w × 0.4`): quad `glColor4f(0.1,0,0,0.5)`, top/left black lines, right/bottom `(100,34,34,150)`.
- Label via `BorderlessButton::Render` (forced highlighted while editing).
- Caret while editing: 1 px vertical line at `fieldX + WcsLen × 7 + 2` from `y + 2` to `y + h − 2`, visible when `fmod(time, 1.0) < 0.5` (1 Hz, 50 % duty).
- When not highlighted and in the first half of each second, `m_buf` is re-read from the value (external changes appear within ≤ 1 s; typed text in a non-committed field is discarded at that point).
- Value text: `g_editorFont` at `x = realX + m_w − GetMenuSize(m_inputBoxWidth) + 2`, `y = realY + 10`, size `GetMenuSize(DEF_FONT_SIZE = 12.0)` — right-aligned by estimated width.

### 4.2 `InputScroller` (hold-to-repeat increment buttons) [C] `InputScroller::*`

- `MouseDown` (called every held frame): only when `ControlEclipseLMouseDown` (edge) fires → `m_mouseDownStartTime = now − INTEGER_INCREMENT_PERIOD` (0.1 s), so the first step happens immediately.
- `CheckInput` (called from `Render` every frame): active when `start > 0`, LMB still held (`ControlEclipseLMousePressed`) and the cursor inside the scroller (half-open rectangle). `change = m_change × 5` while `ControlScrollSpeedup` (Shift) is held. `elapsed = now − start`.
  - `TypeChar`: if `elapsed > 0.1` → `v += change` (**no bound**), `start += 0.1` (10 steps/s).
  - `TypeInt`, `TypeBool`: same cadence with `Bound()` clamp.
  - `TypeFloat`, `TypeDouble`: every frame `v = Bound(v + change × elapsed × 0.2)` — continuous and *accelerating* (rate grows linearly with hold time; frame-rate dependent).
  - Then `m_inputField->Refresh()` every active frame (callback fires every frame).
- `MouseUp` → `start = −1`. Releasing outside the scroller leaves `start > 0`, so a later held press that drags back over it resumes stepping (quirk).

### 4.3 `ColourWidget` / `ColourWindow` [C] `code/interface/input_field.cpp` (compiled; used by editor windows only)

- `ColourWidget : DarwiniaButton` renders normally plus a swatch `[w−30, w−5] × [4, h−3]` in the bound `int` reinterpreted as 4 bytes RGBA. `MouseUp` → `new ColourWindow("editor_coloureditor")`, `SetSize(200, 100)`, registered with the widget's window as parent (random placement, §2.2).
- `ColourWindow::Create`: base close box, then four value controls on the bytes: `editor_red` (y 25), `editor_green` (43), `editor_blue` (61), `editor_alpha` (79); step 1, range [0, 255], width `m_w − 80`, sharing the widget's callback. Render adds a swatch `[w−60, w−10] × [25, h−10]`.

### 4.4 `ScrollBar` [B:FULL] `code/interface/scrollbar.*`

`ScrollBar` is a plain controller object (not a widget) that remembers its parent window **by name** (copied at construction) and registers three buttons in that window [C] `ScrollBar::ScrollBar`, `ScrollBar::Create` — so it must be created after the window is registered and has its final name.

| Button | Name | Rect (window-relative) | Class |
|---|---|---|---|
| up | `"<name> up"` | `(x, y, w, 18)` | `ScrollChangeButton(−stepSize)` |
| track/thumb | `"<name> bar"` | `(x, y+18, w, h−36)` | `ScrollBarButton` |
| down | `"<name> down"` | `(x, y+h−18, w, 18)` | `ScrollChangeButton(+stepSize)` |

Model: `m_numRows` (content rows), `m_winSize` (visible rows), `m_currentValue` (first visible row). `SetCurrentValue(v)`: if `numRows ≤ winSize` → 0; else clamp to `[0, numRows − winSize]`; dirty the parent. `ChangeCurrentValue(d) = SetCurrentValue(cur + d)`. `SetNumRows`/`SetWinSize` re-clamp [C]. (`GameServerButton::SpecialScroll` writes `m_currentValue` directly, bypassing the clamp, §6.11.)

`ScrollBarButton::Render`: track quad `(85,93,78)`, outline `(187,187,187)`; thumb `barTop = h × cur/numRows`, `barEnd = min(h × (cur+win)/numRows, h−1)`; thumb colour `(50,55,120)` clicked, `(55,60,120)` highlighted, `(65,71,120)` normal, inset 1 px left/top. Division by `numRows = 0` (empty list) yields an inf/NaN rectangle [I].

Interaction [C]:
- `ScrollBarButton::MouseDown` (every held frame): if not grabbing and the cursor is within the thumb → `grabOffset = int(winSize × (mouseY − barTop)/(barEnd − barTop))` rows. If grabbing → `value = int((mouseY − trackTop)/h × numRows) − grabOffset`, then `SetCurrentValue` clamps.
- `ScrollBarButton::MouseUp`: if not grabbing → page: cursor above the thumb → `−winSize`; below → `+winSize` (the `barEnd` clamp compares an absolute screen y against `m_h`, so effectively any click not above the thumb pages down — benign because thumb clicks set a grab first). If grabbing → stop the grab.
- `ScrollChangeButton::MouseDown` → `ChangeCurrentValue(amount)` **every held frame** (frame-rate-dependent auto-repeat, no initial delay). Render: DarwiniaButton look plus `icons/selectionarrow.bmp` (64×64) quad inset 5 %, flipped vertically for the up arrow, white, with `glBlendFunc(SRC_ALPHA, ONE)` and `glDepthMask(false)`; it ends with `glDepthMask(true)` and **leaves the blend function additive** (two state leaks, §8.6) [C] `ScrollChangeButton::Render`.
- `Remove()` unregisters the three buttons; the `ScrollBar` object is owned (and deleted) by the window class that created it.

### 4.5 `CheckBox` [B:FULL] `code/interface/checkbox.*`

Binds a `NetworkBool*` through `RegisterBool(bool*)` (owned `LocalBool`), `RegisterInt(int*)` (owned `LocalIntAsBool`), `RegisterNetworkInt(NetworkInt*)` (owned `NetworkIntAsBool` adaptor), `RegisterNetworkBool(NetworkBool*)` (not owned); the first registration wins, later calls are ignored [C]. `MouseUp` → `Toggle()` (`v = !v`) **without checking `m_disabled` or `m_inactive`**; `Set(int|bool)`; `IsChecked()` (false if unbound).

Render [C] `CheckBox::Render`: highlighted → full-rect quad `(199,214,220,255)` with `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`, caption in shadow mode with colour alpha 0 (glyphs darken the highlight); normal → white additive caption. Caption at `(x+5, y + h/2 − fontSize/2 + 7)`. Box side `s = 0.75h` at `x = right − 2s` (= right − 1.5h), `y = top + (h − s)/2`, grey `(0.3,0.3,0.3,1)` fill, white 1 px outline; when checked, tick texture `icons/tick.bmp` (50×50, 32-bit) tinted `(0.6,1.0,0.4,1)`, inset `0.05h`, size `0.65h`.

### 4.6 `DropDownMenu` [B:FULL] `code/interface/drop_down_menu.*`

Classes [C]: `DropDownOptionData {char *m_word; int m_value}`; `DropDownMenu : DarwiniaButton`; `DropDownWindow : DarwiniaWindow` (the open list; **process-wide singleton** `DropDownWindow::s_window`); `DropDownMenuOption : BorderlessButton` (one per row).

Option list: `AddOption(word, value = INT_MIN)` — `INT_MIN` means "auto": `value = m_nextValue++` (the counter advances in *call order*, independent of sorting). With `_sortItems = true` the option is inserted before the first existing option that compares greater under `stricmp` (case-insensitive, stable for ties); otherwise appended. `Empty()` clears the list and resets the counter. A sorted menu therefore displays alphabetically while values still follow insertion order.

Selection: `SelectOption(value)`: `m_currentOption = FindValue(value)` (index or −1); caption = `LANGUAGEPHRASE(m_name)` if not found, else `LANGUAGEPHRASE(word)` if `word` is a phrase key, else the raw word; **always** writes `*m_int = value` (even if not found) [C] `DropDownMenu::SelectOption`. `SelectOption2(word)` selects by case-insensitive word. `GetSelectionValue()` (−1 if none), `GetSelectionName()` (LANGUAGEPHRASE of the word; its bounds check uses `>` instead of `>=` — off by one). `RegisterInt(int*)` (owned `LocalInt`) / `RegisterNetworkInt(NetworkInt*)` bind storage and immediately `SelectOption(current)` — callers that register before adding options get the "not found" caption until re-selected.

Open/close [C] `DropDownMenu::CreateMenu`, `MouseUp`, `IsMenuVisible`:
- `MouseUp` (ignored if `m_disabled`): toggle; "visible" ≡ a window named exactly the menu button's `m_name` exists.
- `CreateMenu`: `DropDownWindow::CreateDropDownWindow(menu name, owning window name)` removes any previous drop-down first (one open list at a time); position `(parent.x + m_x + m_w, parent.y + m_y)` (to the right of the button); registered **before** any option is added (so `MakeAllOnScreen` runs with `w = h = 0` and the list can extend past the right/bottom edge [I]); takes focus.
  - Columns: `numCols = 1 + int((m_h × N) / (screenH × 0.8))`; `perCol = ceil(N / numCols)`.
  - Option `index` at `(col × m_w + 2, (row + 1) × m_h)`, size `(m_w − 4, m_h)`, named `"<menu> <index>"`, caption = **raw `m_word`** (keys are not translated inside the open list — only in the closed caption); added to the window's `m_buttonOrder`; the option whose value equals the current selection becomes the navigation cursor.
  - Window size `(m_w × numCols, (perCol + 1) × m_h)` (row 0 is under the title bar).
- `DropDownWindow::Update`: base navigation, then self-removes if the owning window no longer exists.
- `DropDownMenuOption::MouseUp` → `menu->SelectOption(m_value)`, `menu->RemoveMenu()`. Render: BorderlessButton look, "clicked" when it is the drop-down window's navigation cursor. (Resolved detail: `DropDownMenuOption::Render`'s own cursor test looks at the *owning* window's `m_buttonOrder` and never matches; the effect comes from `BorderlessButton::Render`'s test against the drop-down window [C].)
- Closed-button render: DarwiniaButton (forced highlighted while open) + white triangle `(w−12,5), (w−3,5), (w−7,9)`.

### 4.7 `MessageDialog` [B:FULL] `code/interface/message_dialog.*`

`MessageDialog(name, message)` [C]: splits `message` on `'\n'` into at most **20** lines (fixed array `m_messageLines[20]`, no bound check → overflow on more lines); `m_w = g_editorFont.GetTextWidth(longestLineChars) + 10` where `GetTextWidth(n, size = 13) = n × 13 × horizScale` (0.6 for Latin/Cyrillic languages → 7.8 px/char; 1.0 for CJK → 13 px/char), minimum 100; `m_h = 65 + lines × DEF_FONT_SIZE(12)`, minimum 85; `SetMenuSize(w, h)`; centred on screen. `Create()` does **not** call the base (no close box): one `OKButton` named `"dialog_close"` captioned `LANGUAGEPHRASE("dialog_close")` (or literal `"Close"` without a table), `w = GetMenuSize(40)`, `h = GetMenuSize(18)`, at `((m_w − w)/2, m_h − GetMenuSize(30))`, font `GetMenuSize(11)`; `MouseUp` removes the dialog. Render: base chrome + each line with `g_editorFont` at `(x+10, y + GetMenuSize(32) + i × GetMenuSize(12))`, size `GetMenuSize(12)`, through the **printf-style** `char*` overload (a `%` in the message is interpreted as a format directive). The button is not in `m_buttonOrder` and `m_backButton` is unset → **a controller cannot dismiss it**. Live callers set `m_x = m_y = 100` directly after registering (no clamp) and pass literal English text (`"Failed to set requested screen resolution of …"`, `"Failed to set requested sound settings"`) [C] `code/renderer.cpp:Renderer::Initialise`, `code/sound/soundsystem.cpp`.

### 4.8 `FileDialog` [C] `code/interface/filedialog.*` (compiled; editor / sound-editor use only)

`FileDialog(name, parentName, path = "c:\\", filter = "*", multiSelect = false)`: `SetDirectory(path)` sets the (undrawn) title to the path and lists files via `g_app->m_resource->ListResources(path, filter, false)` (archives + data directory). `Create()`:

| Element | Name | Rect | Behaviour |
|---|---|---|---|
| close box | `Close` | base | — |
| rows ×`numRows = (m_h − 60)/13` | `"File <i>"` | `(5, 25 + 13i, m_w − 25, 12)` | shows `files[i + scroll]` at `(+30, +8)` size 11; selected → fill `(0.3,0.3,1.0,0.5)`; hover/click → white outline; `MouseUp` → `FileClicked`; a second `MouseUp` on the same row-button within **0.2 s** acts as OK (double-click) |
| selection label | `Selected` | `(10, h−30, w−140, 20)` | caption = file name, `LANGUAGEPHRASE("dialog_multiplefiles")` for > 1, `" "` for none |
| Cancel | `dialog_cancel` | `(w−60, h−30, 55, 20)` | close |
| OK | `dialog_ok` | `(w−120, h−30, 55, 20)` | call virtual `FileSelected(filename)` for each selected index in selection order, then close |
| scroll bar | `FileScroll` | `(w−20, 25, 15, numRows × 13)`, rows = file count, window = numRows | §4.4 |

`FileClicked(index)`: if not multi-select or `ControlFileMultiSelect` (Ctrl) not held → clear the selection; if already selected and multi-select → deselect; else append. Subclasses override `FileSelected` (editor: `TerrainSelectorFileDialog`, `WaterSelectorFileDialog`, `WavesSelectorFileDialog`; sound editor: `SoundSelector`, `SampleGroupFileDialog`). The double-click timer is per row-button, not per file.

### 4.9 `MultiwiniaFileDialog`, `MultiwiniaWindow`, `NewMapWindow` — the front-end styling of editor pages [B:EDITOR, compiled in]

These are the only places where the editor uses the front-end look; they show how a `GameOptionsWindow` page is styled (§5.1) [C] `code/interface/multiwinia_window.cpp`, `code/interface/multiwinia_filedialog.cpp`:
- `MultiwiniaWindow` (`"dialog_multiwinia"`, map-editor hub) and `NewMapWindow` derive from `GameOptionsWindow`; constructor `SetMenuSize(screenW, screenH)`, `SetPosition(0, 0)`, `SetMovable(false)`; `Create()` calls the base, `SetTitle(LANGUAGEPHRASE("multiwinia_mainmenu_title"))`, uses `GetPosition_LeftBox`/`GetFontSizes`/`GetButtonSizes`, puts a `GameMenuTitleButton` at `(leftX+10, leftY+10, leftW−20, 1.5 × buttonH)`, stacks `GameMenuButton`s at `y += buttonH + gap` with `m_fontSize = fontMed`, adds each to `m_buttonOrder`, and assigns a `MenuCloseButton("dialog_back")` at `leftY + leftH − 2 × buttonH` to `m_backButton`.
- `NewMapWindow` text entry uses `CreateMenuControl("multiwinia_editor_mapfilename", InputField::TypeString, &m_mapFilename, …)`; success/failure feedback through `CreateErrorDialogue(text, success)` with the `*F` placeholder replaced by the file name (`UnicodeString::ReplaceStringFlag('F', …)`).
- `MultiwiniaFileDialog` = the `FileDialog` model on a `GameOptionsWindow`: title button `LANGUAGEPHRASE("multiwinia_editor_selectmap")`; 10 fixed rows (`MultiwiniaFileButton`, height `0.65 × buttonH`, `g_titleFont`, outline + fill passes, selected fill `(191,191,255,127)`) **not** added to `m_buttonOrder` (line commented) so a controller cannot pick a file; a scroll bar at the left box's right edge (width `0.5 × rowH`); OK (`MultiwiniaFileOKButton`, `dialog_ok`) and Back (`MenuCloseButton("dialog_back")`, `m_backButton`) are navigable. `RefreshFileList`: resource listing first, if empty `ListDirectory(path, filter)` on disk. `FileSelected(f)` writes `((MultiwiniaWindow*)EclGetWindow(m_parent))->m_mapFilename`. The constructor creates a `ScrollBar` that `Create` replaces with a new one (the first leaks).

### 4.10 `ChatInputWindow` / `ChatInputField` — free-text entry [B:FULL] `code/interface/chatinput_window.*`

- `ChatInputWindow` (`"location_chat"`, `DarwiniaWindow`, no chrome — `Render` = `EclWindow::Render`, `Create` does not call the base). In game (`!m_atMainMenu`): font `0.015 H`; reference scale `W/1280`, or `W/1024` when `W ≤ 800`, or `0.75` when `W ≤ 640`; position `(10·scale, H − 10·scale − 1.5·font)`, size `(0.25 W, 1.5·font)`. `Create()` registers one `ChatInputField` `"chat_input"` filling the window, bound with `RegisterString(m_currentChatMessage, 255)`, and begins text edit immediately when not in the lobby. `Update()` (not in the lobby): the window removes itself as soon as it loses focus. `IsChatVisible()` ≡ `EclGetWindow("location_chat")` exists [C].
- The lobby embeds a `ChatInputField` directly in the `GameMenuWindow` page (§6.5); `ControlLobbyChat` (Enter) begins editing when `!EclIsTextEditing()` and not toggled this frame [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::UpdateMultiplayerPage`.
- `ChatInputField::Render`: black 50 % box, 2 px white outline, clipped (`Renderer::Clip`); text size `0.75 h`; when not editing it shows `LANGUAGEPHRASE("multiwinia_presstotalk")` in grey `(0.4,0.4,0.4)`; text longer than the box is right-aligned so the tail stays visible; caret = 1 px line after the text, 1 Hz blink.
- `ChatInputField::Keypress`: base `InputField::Keypress`, then on Enter (if chat was not toggled this frame): a message starting with `/rename ` or `/name ` (and not containing `//`) renames the player (≤ 15 characters, buffer 16) — in the lobby through `GameMenuWindow::m_teamName.Set` + `SetTeamNameAndColour`, in game through `ClientToServer::ReliableSetTeamName`; any other non-empty message is sent with `ClientToServer::SendChatMessage`; the buffer is cleared; in the lobby editing restarts, in game the window closes; the `m_chatToggledThisUpdate` flag of the owner is set [C].

### 4.11 `AuthInputField` — validated key entry [B:FULL] `code/UI/AuthInputField.h`

A specialised `InputField` for the 31-character authentication key (`AUTHENTICATION_KEYLEN = 32` incl. terminator) [C]:
- `Keypress`: accepted only while `len < 31` or the key is Backspace; after the base `Keypress` the buffer is upper-cased, written to the value, and the key is immediately set and saved to `<profile>authkey` (`Authentication_SetKey`, `Authentication_SaveKey`); after a non-Backspace key, when the length reaches 6, 13, 20 or 27 a `"-"` is appended automatically (format `XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXX`). Enter → if `GameMenuWindow::CheckNewAuthKeyStatus()` → page Main. (macOS only: Cmd-V pastes.)
- `Render`: black box `(10,10,10)` drawn at **window-relative** `m_x, m_y` (correct only because `GameMenuWindow` sits at (0,0)), 1 px border `(200,200,200)`; 31 fixed cells of width `fontSize`, one `g_titleFont` glyph per cell, separator lines alpha 100 (alpha 255 after every 7th cell group: `i % 7 ≥ 5`); block caret `0.8 × fontSize` wide while editing (1 Hz); below, the localised result line `authentication_result` with `*S` replaced by the status phrase (or `dialog_auth_status_demo_user`) at `y + 6 h`, size `1.5 × fontSize`, outline + green fill `(0.6,1.0,0.4)`.
- `MouseUp`: clears the placeholder value `"authkey not found"` before editing.

### 4.12 Value-binding model (`code/network/networkvalue.*`)

All editable widgets talk to a `NetworkValue` (abstract `ToUnicodeString` / `FromUnicodeString`), which lets the same widget edit a local variable or a lobby option that must round-trip through the server [C] `code/network/networkvalue.h`:

| Adaptor | Backing store | `Set` semantics | Used by |
|---|---|---|---|
| `LocalInt`, `LocalFloat`, `LocalDouble`, `LocalChar`, `LocalBool` (`LocalType<Type, Storage, Super>`) | reference to a caller variable | direct assignment, **no clamping** (`Bound` must be called explicitly) | `CreateValueControl`, `CheckBox::RegisterBool`, `DropDownMenu::RegisterInt`, options windows |
| `LocalIntAsBool` | `int&` viewed as bool | direct | `CheckBox::RegisterInt` |
| `NetworkIntAsBool` | wraps a `NetworkInt*` | forwards | `CheckBox::RegisterNetworkInt` |
| `NetworkIntAsString` | wraps a `NetworkInt*`, string I/O | forwards | Other Options port fields |
| `LocalString(buf, size)`, `LocalUnicodeString(ref)` | caller buffer / string | `snprintf`/assign | text fields, chat |
| `NetworkGameOptionInt(option, int &data)`, `NetworkGameOptionBool`, `NetworkGameOptionMapId` | `Get` = the local echo of the server state; `Set` = `ClientToServer::RequestSetGameOptionInt(option, Bound(v))` | **request only** — the visible value changes when the server echoes it | lobby drop-downs/check boxes (`GameMenuWindow::m_gameType`, `m_aiMode`, `m_coopMode`, `m_basicCrates`, `m_singleAssaultRound`, `m_requestedMapId`) |
| `NetworkGameParam` (elements of `NetworkIntArray m_params`, 64) | `Set` = `RequestSetGameOptionArrayElementInt(GAMEOPTION_GAME_PARAM, index, Bound(v))` | request only, bounded | Advanced Options page |
| `PlayerNameString` (16), `ServerPasswordString` (16) | `NetworkUnicodeString` subclasses with truncation | name: also saved to pref `PlayerName` and sent to the server when connected; password: proxied to `Server::Get/SetPassword` on the host | player options, password field |

Consequence for the UI [C-derived]: a widget bound to a networked option shows the *echoed* value, so a host's click appears one network round trip later; a client that is not the host can still issue requests (the UI hides the controls; whether the server rejects them is open, [09](09_meta_systems.md)/[22](22_determinism_and_networking.md)).

---
## 5 Front-end layer: `GameOptionsWindow` and the `GameMenu*` widget family

[B:FULL] `code/interface/mainmenus.*`, `code/game_menu.*`. This is the look of every live menu: green/red title-font captions on blurred, glowing "fill boxes" over the animated backdrop.

### 5.1 `GameOptionsWindow` — the full-screen page [C] `code/interface/mainmenus.cpp:GameOptionsWindow::*`

| Member | Default | Meaning |
|---|---|---|
| `m_renderRightBox` | false | also draw the right content box |
| `m_renderWholeScreen` | false | draw one content box of title-bar width instead of the left box |
| `m_errorMessage` | empty | in-page dialogue text |
| `m_showingErrorDialogue` | false | dialogue visible |
| `m_dialogueSuccessMessage` | false | header "Success" (`multiwinia_editor_success`) instead of "Error" (`error`) |

- Constructor: `SetMenuSize(screenW, screenH)`; `SetPosition(0, 0)`; `SetMovable(false)`; `m_resizable = false` — a full-screen, fixed page (with large menus the page becomes larger than the screen and `MakeAllOnScreen` gives it a negative origin, defect F15). `Create()` is **empty** (no close box; subclasses call it and then build their content).
- `Update()`: if `m_buttonOrder` is non-empty and no dialogue is shown → `DarwiniaWindow::Update()` (navigation); else if the dialogue is shown and `ControlCloseTaskHelp` fires (mouse left down, Space, Enter, Esc, XInput A [C] `data/input/input_preferences.txt`) → hide the dialogue, clear the message, `m_lockMenu = false`.
- `Render(hasFocus)`: **returns immediately unless focused** (windows underneath are not drawn). Then: `GameMenuWindow::RenderBigDarwinian()` (backdrop, §6.7) → title bar `DrawFillBox` → title `LANGUAGEPHRASE("multiwinia_mainmenu_title")` at `fontLarge`, centred at `titleX + titleW/2`, `titleY + 0.2 titleH`, outline pass (colour `(1,1,1,0)`) then white fill → subtitle `multiwinia_mainmenu_subtitle` at `fontMed`, one large line below → logo `icons/mwlogo.bmp` (aspect 277/194, height `0.8 titleH`, top at `titleY + 0.1 titleH`) at `titleX + 0.05 titleW` and mirrored at the right → content boxes (`m_renderWholeScreen` → `DrawFillBox(leftX, leftY, titleW, leftH)`; else left box, plus right box if `m_renderRightBox`) → `EclWindow::Render` (buttons) → error dialogue if shown.
- `CreateErrorDialogue(msg, success = false)`: set message, show, `m_lockMenu = true` (navigation frozen), remember success flag. `RenderErrorDialogue()`: centred `DrawFillBox` of `(W/3) × (H/3)`; header at `boxY + fontSmall` in `g_titleFont` `fontMed`; message word-wrapped with `WordWrapText(msg, 1.5 × boxW, fontSmall)` (the width argument is in characters × font width — see [29 §L.5](29_data_formats_reference.md)), lines `fontSmall` high at `1.1 × fontSmall` pitch.
- `CreateMenuControl(name, dataType, NetworkValue *value, y, change, callback, x, w, fontSize)`: `x = −1 → 10`, `w = −1 → m_w − 2x`; a `GameMenuInputField` named `name`, captioned `LANGUAGEPHRASE(name)`, height 30 (callers then override `m_h`), `m_fontSize = fontSize`, bound with `RegisterNetworkValue(dataType, value)`; for `dataType != TypeString` two `GameMenuInputScroller`s `"<name> left"` / `"<name> right"`, captions `<` / `>`, tooltips "Value left"/"Value right", size `fontSize × fontSize` at `(field.x + field.w − 4·fontSize, y + fontSize/2)` and `(field.x + field.w − fontSize, y + fontSize/2)`, `m_change = ∓change`. Returns the field; the caller adds it to `m_buttonOrder` [C] `GameOptionsWindow::CreateMenuControl`. (`GameMenuWindow::CreateMenuControl` is a copy that also suppresses scrollers for `TypeUnicodeString` [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::CreateMenuControl`.)

Window classes built on `GameOptionsWindow` [C] (window name in quotes; functional content in [09](09_meta_systems.md)):

| Class | Window name | Tag |
|---|---|---|
| `HelpAndOptionsWindow` | `"multiwinia_menu_helpandoptions"` | [B:FULL] |
| `SelectHelpWindow` | `"multiwinia_menu_selecthelp"` | [B:FULL] |
| `GeneralHelpWindow` | `"multiwinia_help_general"` | [B:FULL] |
| `GameTypeHelpWindow` | game-type blueprint name | [B:FULL] |
| `CrateHelpWindow` | `"multiwinia_menu_selecthelp"` (same name as `SelectHelpWindow`) | [B:DEBUG] |
| `OptionsMenuWindow` | `"dialog_options"` | [B:FULL] |
| `PrefsScreenWindow` | `"dialog_screenoptions"` | [B:FULL] |
| `PrefsGraphicsWindow` | `"dialog_graphicsoptions"` | [B:FULL] |
| `PrefsSoundWindow` | `"dialog_soundoptions"` | [B:FULL] (content in [27](27_audio_engine.md)) |
| `PrefsOtherWindow` | `"dialog_otheroptions"` | [B:FULL] |
| `LocationWindow` | `"dialog_locationmenu"` (in-game Esc menu) | [B:FULL] |
| `ConfirmExitWindow` | `"multiwinia_confirmexit"` | [B:FULL] |
| `ControllerUnpluggedWindow` | `"controller_unplugged"` | [B:FULL] |
| `MainMenuWindow` | `"dialog_mainmenu"` | [B:LEGACY] |
| `ConfirmResetWindow` | `"multiwinia_confirmreset"` | [B:LEGACY] |
| `ResetLocationWindow` | `"dialog_resetlocation"` | [B:LEGACY] |
| `UserProfileWindow` | `"dialog_profile"` | [B:LEGACY] |
| `LeaderboardWindow` / `LeaderboardSelectionWindow` | `"leaderboards"` / `"leaderboardselection"` | [B:LEGACY] |
| `MultiwiniaWindow` / `NewMapWindow` / `MultiwiniaFileDialog` | `"dialog_multiwinia"` / `"new_map"` / caller-given | [B:EDITOR] |

### 5.2 `GameMenuButton` — the animated menu button [C] `code/game_menu.cpp:GameMenuButton::*`

| Field | Default | Meaning |
|---|---|---|
| `m_fontSize` | 20 (callers override, normally `fontMed`) | caption size px |
| `m_useEditorFont` | false | colour variant only (the font switch is commented out) |
| `m_ir, m_ig, m_ib, m_ia` | 0.5, 0.5, 0.5, 1.0 | caption colour when `m_inactive` |
| `m_birthTime` | `GetHighResTime()` at construction | animation clock (> 0 cascading, negative = pop phase, −1000 = finished) |
| `m_transitionDirection` | +1 | +1 "forward" (`MenuSelect`), −1 "back" (`MenuCancel`) |
| `s_previousHighlightButton` (static) | NULL | last highlighted widget, for the roll-over sound |

`MouseUp()`: trigger `"MenuSelect"` (direction +1) or `"MenuCancel"` (−1) on `SoundSourceBlueprint::TypeMultiwiniaInterface`, then `Renderer::InitialiseMenuTransition(1.0, m_transitionDirection)` (captures the current frame for the page-swipe, §6.7). Subclasses call it first and then do their action. **`m_disabled` has no effect at all on `GameMenuButton`** (neither colour nor click): e.g. `NewServerButton` is given `m_disabled = true` when hosting is not permitted and stays clickable [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetupNewOrJoinPage`, `code/UI/NewServerButton.h`. Click suppression in the front end is done with `m_inactive` and an explicit early return in `MouseUp` (Resolved: P18 called `m_disabled` "Eclipse-level"; `m_inactive` is the `EclButton` field, `m_disabled` the `DarwiniaButton` field).

`Render(realX, realY, highlighted, clicked)` [C]:
1. **Birth cascade**: `numButtons = parent->m_buttonOrder.Size()`; `showTime = birth + 0.6 × realY/H`; `flashShowTime = birth + (5/numButtons) × realY/H`. While `birth > 0` and `now < flashShowTime`: the button is drawn shifted left by `(showTime − now) × 0.1 × W` px and an additive white box `(1,1,1, clamp(flashShowTime − now, 0, 0.9))` with `SRC_ALPHA, ONE` covers it. Buttons lower on screen appear later (top-to-bottom cascade); with few navigable buttons the flash lasts longer.
2. Highlight: Eclipse `highlighted` kept only in mouse-highlight mode; forced true for the navigation cursor. If highlighted: translucent box `(0.75,0.75,1.0,0.5)` with normal alpha blend.
3. `UpdateButtonHighlight()` (after the decision, so mouse-driven cursor moves show one frame later).
4. Caption in `g_titleFont`: outline pass with colour `(1,1,1,0)`, then fill pass in: normal `(0.6,1.0,0.4,1)`; `m_useEditorFont` `(1.0,1.0,0.3,1)`; highlighted `(1.0,0.3,0.3,1)`; inactive `(m_ir,m_ig,m_ib,m_ia)`. Position `x + 0.05 w` (left) or centred; baseline `y + h/2 − size/2 + 7`.
5. **Pop**: on the first frame with `now ≥ showTime` the clock flips to `−now`; for the next 0.1 s the caption is redrawn in white with alpha `1 − t/0.1` (left-aligned buttons also draw a ghost at `1.3 × size` with alpha `0.5 − t/0.1`), then `m_birthTime = −1000` (finished).
6. Roll-over sound: if highlighted and `s_previousHighlightButton != this` → if a previous one existed, trigger `"MenuRollOver"`; remember this button.

### 5.3 `BackPageButton` and `MenuCloseButton` [C] `code/game_menu.cpp:BackPageButton::*`, `code/interface/mainmenus.h:MenuCloseButton`

- `BackPageButton(page)` (caption argument `"multiwinia_menu_back"`; callers pass the translated caption to `SetShortProperties`): `MouseUp` = `GameMenuButton::MouseUp()` (so `MenuSelect` and a +1 transition fire), then `"MenuCancel"` and `InitialiseMenuTransition(1.0, −1)` (the later call wins), then `((GameMenuWindow*)m_parent)->m_newPage = page`. `Render`: base render; in `INPUT_MODE_GAMEPAD` it draws `icons/button_b.bmp` (size `m_fontSize`) at `realX + g_gameFont.GetTextWidth(len, size) + 60` (+ `w/2` if centred) and **calls its own `MouseUp()` when `ControlMenuClose` fires** — the pad "back" of a `GameMenuWindow` page lives in this render function.
- `MenuCloseButton(name)` (for `GameOptionsWindow` pages): `MouseUp` → `"MenuCancel"`, `InitialiseMenuTransition(1.0, −1)`, `EclRemoveWindow(parent)`, `g_app->m_doMenuTransition = true`. Its `Render` draws the same B glyph in gamepad mode but does **not** poll `ControlMenuClose`; pad back on those pages works through `DarwiniaWindow::Update` (`m_closeable` true) because every live page assigns its Back/Cancel button to `m_backButton` (`HelpAndOptionsWindow`, help pages, options pages, `LocationWindow`, `ConfirmExitWindow`, `OptionsMenuWindow`) [C] grep `m_backButton =` in `code/interface/*.cpp`. `MainCloseButton` (in-game menu) is a `MenuCloseButton` subclass [C] `code/interface/mainmenus.cpp`.

### 5.4 Titles, highlight beam and fill boxes [C] `code/game_menu.cpp`

- `GameMenuTitleButton::Render`: `RenderHighlightBeam(x, y + h/2, w, h)` behind; caption upper-cased; size `1.5 × m_fontSize`, shrunk by `w / (textWidthReal × 1.05)` when wider than the button; outline pass `(0.75,1.0,0.45,1)` then shadow pass `(1.0,0.9,0.9,0.0)`, centred, baseline `y + h/2 − size/2 + 7`. Never in `m_buttonOrder`.
- `RenderHighlightBeam(x, centreY, w, h)`: `textures/menuhighlight.bmp` quad (trilinear, repeat) with vertex colours top `(0.6,1.0,0.4,0.8)`, bottom `(0.3,0.5,0.2,0.7)`, normal alpha; black outline alpha 0.5.
- `DrawFillBox(x, y, w, h)`: (1) soft shadow frame `0.01 W` wide: four gradient quads from `(255,255,255,0)` at the box edge to `(0,0,0,0)` outside with `glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_COLOR)` (src alpha 0 → the result is `dst × (1 − srcColour)`: black at the edge fading to untouched outside — a darkening halo); (2) `textures/interface_blur.bmp`, colour `(0.8,0.8,1.0,0.9)`, u 0 … w/64, v 0 … 1, normal alpha; (3) `textures/cloudyglow.bmp` additive, colour `(0.3,0.6,0.9,0.4)`, tex origin `(−0.01 t, −0.02 t)`, extent `(w/W + 0.01 sin t, h/H + 0.02 sin t)` (slowly drifting glow; t = `GetHighResTime()` s); (4) 2 px outline `(0.8,0.7,1.0,0.8)` additive; inner outline inset 3 px, 1 px, alpha 0.4; finally the blend func is reset to normal alpha.

### 5.5 `GameMenuDropDown`, `GameMenuDropDownWindow`, `GameMenuDropDownMenuOption` [C] `code/game_menu.cpp`

- `GameMenuDropDown : DropDownMenu` with `m_noBackground` (false) and `m_cycleMode` (false). `MouseUp`: ignored when `m_inactive`; in cycle mode select the next option (wrap to 0) — no page change, no sound; otherwise `InitialiseMenuTransition(1.0, +1)`, `m_doMenuTransition = true`, `DropDownMenu::MouseUp()` (opens/closes).
- `Render`: row box `(0.75,0.75,1.0,0.5)` highlighted, else `(1,1,1,0.1)` (skipped when `m_noBackground` and not highlighted); label = `LANGUAGEPHRASE(m_name)` (**the widget name is the phrase key**) at `x + 0.05 w`; current value = `m_caption` right-aligned at `x + w − fontSize`, hidden while the list is open; colours green / red highlighted / grey `(0.5,0.5,0.5)` inactive; roll-over sound as §5.2.
- `CreateMenu` (full-screen list): `GameMenuDropDownWindow::CreateGMDropDownWindow(m_name, parent)` (singleton), position (0,0), registered, then `available = leftH − 1.5 bH − 1.8 bH`; `columns = 1 + int(bH × n / available)`; `perColumn = ceil(n / columns)`; option `w = int(buttonW / columns)`, `x = leftX + col × w + (leftW − w × columns)/2`, then `w × 0.95`; row `y = leftY + 1.8 bH + (i + 1) × m_h × 1.05`; caption **translated** (`LANGUAGEPHRASE(word)`, unlike the base class); the current option is pre-highlighted (`m_currentOption`) and becomes the navigation cursor; final `SetSize(W, H)`.
- `GameMenuDropDownWindow` (not movable/resizable): `Create` = `SetTitle(multiwinia_mainmenu_title)` + a `GameMenuTitleButton` captioned `LANGUAGEPHRASE(m_name)`; `Render` (no focus check) = backdrop + title bar + subtitle + mirrored logos + left fill box + buttons. Destructor clears the singleton.
- `GameMenuDropDownMenuOption::MouseUp` = `InitialiseMenuTransition(1.0, −1)`, `m_doMenuTransition = true`, then base (select + close). Render: box as the drop-down row (current option drawn "clicked"), caption at `x = realX + realX × 0.05` (uses `realX` where `m_w` was intended), `y = realY + h/2`, outline + white/red fill.

### 5.6 `GameMenuInputField` / `GameMenuInputScroller` [C] `code/game_menu.cpp`

- `GameMenuInputField : InputField` with `m_noBackground`, `m_renderStyle2`. `PIXELS_PER_CHAR = 25` (front-end redefinition used for `m_inputBoxWidth`).
- Gamepad editing of numbers: when the field is the navigation cursor and not a string type, `ControlMenuLeft` / `ControlMenuRight` look up the `"<name> left"` / `"<name> right"` scroller and apply `v->Set(v->Get() + scroller.m_change)` through a `NetworkInt` cast — clamped only if the bound adaptor clamps inside `Set` (networked options do; `Local*` values do not, §4.12).
- Style 1 (default): row box as the drop-down; caption at `x + 0.05 w` (outline + green/red); string value right-aligned at `x + w − fontSize`; numeric value at `0.8 × fontSize` right-aligned at `x + w − 2 × fontSize`; editing caret = 5 px line at the fixed position `x + w − 0.95 × fontSize` (it does not follow the text), 1 Hz.
- Style 2 (`m_renderStyle2`, string types only): black box `(10,10,10,255)` with white outline, text centred, 3 px caret after the centred text.
- `MouseUp` starts editing only for string types; `Keypress` sets the parent's `m_skipUpdate` on Enter (the next `DarwiniaWindow::Update` is skipped, so wherever Enter is also bound to `MenuActivate` — the Darwinia control set — the same key does not activate the navigation cursor), then calls the base.
- Roll-over sound guard compares `m_currentTextEdit` with `"none"` (lower case) while the sentinel is `"None"` → `strcmp` never matches and **input fields never play `MenuRollOver`** [C] `GameMenuInputField::Render`.
- `GameMenuInputScroller : InputScroller`: renders its `<`/`>` caption at `(realX, realY)` in `g_titleFont` (outline + green, red when highlighted/cursor), then `CheckInput` (hold-to-repeat every 0.1 s, ×5 with Shift, ints/bools bounded, §4.2).

### 5.7 `GameMenuCheckBox` [C] `code/game_menu.cpp:GameMenuCheckBox::Render`

Row box as the drop-down; caption `g_titleFont` at `x + 0.05 w` (outline + green/red); box side `0.75 h` at `x + w − 1.5 h`, grey `(0.3,0.3,0.3)` fill, white outline; tick `icons/tick.bmp` tinted `(0.6,1.0,0.4)` inset `0.05 h`, size `0.65 h`; roll-over sound. Click = `CheckBox::Toggle` (no inactive check).

---
## 6 `GameMenuWindow` page framework

[B:FULL] `code/UI/GameMenuWindow.*`, `code/game_menu.*`, `code/UI/*`. The entire Multiwinia front end (title, mode select, map select, lobby, server browser, auth key, buy-now, credits, tutorials entry) is **one full-screen Eclipse window** of class `GameMenuWindow`, registered under the name `"multiwinia_mainmenu_title"` (also the phrase id of the title) and found by other code with `EclGetWindow("multiwinia_mainmenu_title")` [C] `GameMenuWindow::GameMenuWindow`, `code/network/clienttoserver.cpp`, `code/game_menu.cpp:GetTimeOption`. Functional page content, flows and lobby rules are specified in [09 Meta systems](09_meta_systems.md); this section specifies the mechanism.

### 6.1 Class structure and lifetime

| Object | Role | Ref |
|---|---|---|
| `GameMenu` (`g_app->m_gameMenu`, created once in `App::Initialise`) | not a window: owns the map catalogue (`m_maps[7]`, `m_allMaps`), the 3D cloud background (`GameMenu::Render`), and creation/destruction of the window | [C] `code/game_menu.cpp:GameMenu::GameMenu` |
| `GameMenu::CreateMenu()` | `Renderer::StartFadeIn(0.25)`; `RemoveAllWindows()`; `g_app->m_multiwinia->Reset()`; `EclRegisterWindow(new GameMenuWindow())`; camera to the sphere-world focus; `m_gameMode = GameModeNone`; if returning from a tutorial `gmw->m_newPage = PageTutorials`; `m_menuCreated = true`. Called by `MainMenuLoop` whenever `!m_menuCreated` and the fade is complete; also by the Screen/Other options apply and `UnlockFullGameButton` | [C] `GameMenu::CreateMenu`, `code/main.cpp:MainMenuLoop` |
| `GameMenu::DestroyMenu()` | `m_menuCreated = false`; `EclRemoveWindow("multiwinia_mainmenu_title")`; `StartFadeOut()` unless editing | [C] |
| `GameMenuWindow` constructor | `SetPosition(0,0)`, `SetSize(W, H)` (not `SetMenuSize`), not resizable/movable, **`m_closeable = false`**; `m_currentPage = −1`, `m_newPage = PageMain` (or `PageAuthKeyEntry` when `Authentication_SimpleKeyCheck` fails, compiled out under `MULTIWINIA_DEMOONLY`); player name from pref `PlayerName` (`%` → space); filter booleans from prefs; release-assert `sizeof(s_pageNames)/sizeof(char*) == NumPages` (38) | [C] `GameMenuWindow::GameMenuWindow` |
| `GameMenuWindow::Create()` | empty (no close box) | [C] |
| `GameMenuWindow::Render(hasFocus)` | returns unless focused **or** the chat window `"location_chat"` exists; backdrop (`RenderBigDarwinian`), title bar (bitmap `textures/mwtitle.bmp` when the localised title equals `"MULTIWINIA"`, else outlined text), subtitle, mirrored logos, per-page fill boxes, page overlays, buttons; when the drop-down singleton exists only the owning button is rendered; overlay error dialogue last | [C] `GameMenuWindow::Render` |
| `GameMenuWindow::PreloadTextures()` (static, at start-up) | pre-loads `icons/button_b.bmp`, `icons/button_lb_shadow.bmp`, `icons/button_rb_shadow.bmp`, `icons/button_lb.bmp`, `icons/button_rb.bmp`, `icons/button_a.bmp`, `textures/interface_blur.bmp`, `sprites/darwinian.bmp`, `mwhelp/gear.bmp`, `textures/laser.bmp`, `textures/clouds.bmp` | [C] |

Leaving to a game: the server's start-game letter sets `m_atMainMenu = false`; `MainMenuLoop` exits, stops `"MultiwiniaInterface LobbyAmbience"` and calls `GameMenu::DestroyMenu()`. Returning: the in-game leave sets `m_atMainMenu = true`, and a **fresh** `GameMenuWindow` is constructed — no widget or page state survives except prefs and `g_app` members [C] `code/main.cpp:MainMenuLoop`, `code/interface/mainmenus.cpp`.

### 6.2 Page slots

`GameMenuWindow` declares an anonymous enum of pages with gaps; `s_pageNames[38]` (`code/game_menu.cpp:s_pageNames`) has one debug name per slot 0–37 (gap slots are removed Xbox Live Arcade, PDLC and Darwinia+ pages); `NumPages = 38` [C] `code/UI/GameMenuWindow.h`.

| Value | Enum | `s_pageNames` | Setup | Per-frame update (in `Update`) | `[B:FULL]` status |
|---|---|---|---|---|---|
| 0 | `PageMain` | "Main" | `SetupMainPage` | `UpdateGameOptions(false)` if `m_quickStart`; `UpdateMainPage` | live (default) |
| 1 | `PageDarwinia` | "Darwinia" | `SetupDarwiniaPage` | — | unreachable ([B:DEBUG] path) |
| 2 | `PageGameSelect` | "GameSelect" | `SetupGameSelectPage` | `UpdateGameOptions(true)` | live |
| 3 | `PageNewOrJoin` | "NewOrJoin" | `SetupNewOrJoinPage` | — | live |
| 4 | `PageJoinGame` | "JoinGame" | `SetupJoinGamePage` | `UpdateJoinGamePage` | live |
| 5 | `PageQuickMatchGame` | "QuickMatchGame" | `SetupQuickMatchGamePage` | `UpdateQuickMatchGamePage` | unreachable |
| 6 | `PageGameSetup` | "GameSetup" | `SetupMultiplayerPage(m_gameType)` | `UpdateMultiplayerPage` then fall-through `UpdateGameOptions(true)` | live (lobby) |
| 7 | `PageGameConnecting` | "GameConnecting" | `SetupGameConnectingPage` | `UpdateGameOptions(false)`; `UpdateGameConnectingPage` | live |
| 8 | `PageResearch` | "Research" | `SetupResearchPage` | — | unreachable |
| 9 | `PageMapSelect` | "MapSelect" | `SetupMapSelectPage` | `UpdateGameOptions(true)` (calls `UpdateMapSelectPage`) | live |
| 10 | `PageHelpMenu` | "HelpMenu" | `SetupHelpMenuPage` | — | unreachable |
| 11 | `PageHelp` | "Help" | `SetupHelpPage` | — | unreachable |
| 12 | `PageAdvancedOptions` | "AdvancedOptions" | `SetupAdvancedOptionsPage` | `UpdateAdvancedOptionsPage` | live (host) |
| 13 | (gap) | "XBLASelectSessionType" | — | — | [B:DEAD] |
| 14 | `PageXBLASignIn` | "XBLASignIn" | none (default case release-asserts "Game menu invalid page") | — | [B:DEAD] |
| 15 | `PageDemoSinglePlayer` | "DemoSinglePlayer" | `SetupSPDemoPage` | `UpdatePageSinglePlayer` | [B:DEBUG] |
| 16 | `PageBuyMeNow` | "BuyMeNow" | `SetupBuyMePage` | — | live [B:DEMO] (runtime gate) |
| 17 | `PageLeaderBoardSelect` | "LeaderBoardSelect" | `SetupLeaderBoardSelectPage` (empty) | — | unreachable |
| 18 | `PageLeaderBoard` | "Leaderboards" | `SetupLeaderboardPage` (empty) | — | unreachable |
| 19 | (gap) | "XBLASelectGameType" | — | — | [B:DEAD] |
| 20 | (gap) | "XBLASelectCustomMatchtype" | — | — | [B:DEAD] |
| 21 | (gap) | "Downloaded PDLC" | — | — | [B:DEAD] |
| 22 | (gap) | "PDLC Upsell" | — | — | [B:DEAD] |
| 23 | `PageAuthKeyEntry` | "AuthKeyEntry" | `SetupAuthKeyPage` | — | live |
| 24 | `PagePlayerOptions` | "PlayerOptions" | `SetupPlayerOptionsPage` | `UpdateGameOptions(true)` | live |
| 25 | `PageSearchFilters` | "SearcHFilters" (sic) | `SetupFiltersPage` | — | live |
| 26 | (gap) | "DarwiniaPlus SP Page" | — | — | [B:DEAD] |
| 27 | (gap) | "DarwiniaPlus MP Page" | — | — | [B:DEAD] |
| 28 | (gap) | "DarwiniaPlus Darwinia Page" | — | — | [B:DEAD] |
| 29 | (gap) | "DarwiniaPlus Darwinia SP Page" | — | — | [B:DEAD] |
| 30 | (gap) | "DarwiniaPlus Darwinia MP Page" | — | — | [B:DEAD] |
| 31 | `PageUpdateAvailable` | "Updates Available Page" | `SetupUpdatePage` | — | live |
| 32 | `PageCredits` | "Credits Page" | `SetupCreditsPage` | — | live |
| 33 | `PageTutorials` | "Tutorial Page" | `SetupTutorialPage` | `UpdateGameOptions(false)`; `UpdateMainPage` | live (`INCLUDE_TUTORIAL`) |
| 34 | `PageError` | "Error Page" | `SetupErrorPage` | — | live |
| 35 | `PageAchievements` | "Achievements Page" | `SetupAchievementsPage` | `UpdateAchievementsPage` | unreachable |
| 36 | `PageEnterPassword` | "Password Entry" | `SetupPageEnterPassword` | — | live |
| 37 | `PageWaitForGameToStart` | "Wait for Game to Start" | none (the case only leaves `Remove()`'s empty page) | nothing | live (host after Start) |

Counts: 18 live, 10 compiled-but-unreachable, 10 gap slots [C] (page table above). Help sub-enum (only `PageHelp`): `HelpPageGeneral = 1`, `HelpPageSkirmish = 2`, `HelpPageKoth = 3`, `HelpPageCts = 4`, `HelpPageRR = 5`, `NumHelpPages = 6`.

### 6.3 Page state machine and per-frame order

```mermaid
stateDiagram-v2
  [*] --> Pending: ctor sets m_newPage Main or AuthKeyEntry and m_currentPage -1
  Pending --> Building: Update sees m_currentPage differs from m_newPage
  Building --> Live: ShutdownPage old, SetupNewPage new, reassert 0,0 and W x H
  Building --> Deferred: Setup function returned early waiting for network echo or team id
  Deferred --> Building: page Update re-calls the Setup function when the data arrives
  Live --> Live: page Update functions run, widgets write m_newPage
  Live --> Building: next page-switch check sees m_newPage changed
  Live --> Building: game type changed under GameSetup or MapSelect - same page rebuilt
  Live --> [*]: GameMenu DestroyMenu removes the window
```

`GameMenuWindow::Update()` order every frame [C] `GameMenuWindow::Update`:
1. Start `"LobbyAmbience"` (`TypeMultiwiniaInterface`) once `SoundSystem::NotReadyForGameSoundsYet()` is false.
2. Navigation: `DarwiniaWindow::Update()` only if `m_buttonOrder` is non-empty, the (dead) overlay dialogue is not shown, the chat input is not visible and chat was not toggled this frame; else, if the overlay dialogue is up, `ControlCloseTaskHelp` dismisses it and clears `m_lockMenu`.
3. Page switch: if `m_currentPage != m_newPage` → `ShutdownPage(m_currentPage)`, `SetupNewPage(m_newPage)`, `SetPosition(0,0)`, `SetSize(W,H)`; else if `m_reloadPage` → `SetupNewPage(m_currentPage)` and clear the flag (no code sets the flag).
4. Demo/full status change: if `m_demoUpgradeCheck != IS_DEMO` → record; on `PageDemoSinglePlayer` → `m_newPage = PageDarwinia`.
5. Per-page update switch (table §6.2; `PageGameSetup` deliberately falls through into `UpdateGameOptions(true)`).
6. If a page change is pending, `m_setupNewPage = true` (unused).

**Latency of a page change.** Resolved: P18 described a uniform one-frame latency. In the code, `UserInput::AdvanceMenus` calls `EclUpdateMouse` (which runs `MouseUp` for clicks) *before* `EclUpdate` (which runs `GameMenuWindow::Update`), and step 2 (pad activate → `MouseUp`) precedes step 3; so a page requested by a mouse click or by `MenuActivate` is rebuilt **in the same frame**. A page requested from inside a `Render` (the gamepad B handler of `BackPageButton`, §5.3) or from a per-page update (step 5) is rebuilt in the **next** frame. When several writes happen before the check, the last write wins [C]. The "same page rebuilt" transition is not a page switch: `UpdateGameOptions(true)` detects `m_uiGameType != g_app->m_multiwinia->m_gameType` (both ≠ −1) on PageGameSetup/PageMapSelect and immediately calls `ShutdownPage(m_currentPage)` + `SetupNewPage(m_currentPage)` [C] `GameMenuWindow::UpdateGameOptions`.

### 6.4 Page set-up and teardown [C] `GameMenuWindow::SetupNewPage`, `GameMenuWindow::ShutdownPage`

`SetupNewPage(page)`:
1. Debug-log `s_pageNames[page]` (indexed before validation).
2. `Remove()` (DarwiniaWindow: every button deleted, `m_buttonOrder` emptied, `m_currentButton = 0`, `EndTextEdit()`); delete `m_scrollBar` and set it NULL.
3. `switch(page)` → `Setup*Page()`; `PageWaitForGameToStart` does nothing; unknown values release-assert "Game menu invalid page (%d)".
4. If `g_app->m_gameMenu->m_menuCreated && !m_reloadPage` → `Renderer::StartMenuTransition()` (animates the frame captured by the button's `InitialiseMenuTransition`).
5. `m_currentPage = page` — **after** the set-up call, so inside a `Setup*` function `m_currentPage` still holds the page being left. Pages use this for "return to caller" (`BackPageButton(m_currentPage)`) and for conditional set-up (lobby re-entry from PlayerOptions/AdvancedOptions keeps parameters).

Destroy-and-rebuild: nothing persists in widgets across a page change; persistent state lives in `GameMenuWindow` members, `g_app`, networked options and prefs.

`ShutdownPage(old)` hooks exist only for: `PageJoinGame` → `StopIdentifying()` unless the next page is `PageGameConnecting`, and forget the server row list; `PageGameConnecting` → `StopIdentifying()`; `PageAchievements` → forget achievement rows [C].

### 6.5 Widget registration pattern

Every widget on every page is created with the same five steps [C] (all `Setup*Page` functions):
1. `new <Class>(ctor args)` — caption/icon string for `GameMenuButton`s, a page id for `BackPageButton`s, a texture path for `ImageButton`, a phrase id for `WaitingButton`/`TextButton`, a colour id for `FancyColourButton`, a row index for `GameServerButton`/`AchievementButton`.
2. `SetShortProperties(name, x, y, w, h, caption)` (or `SetProperties(…, caption, tooltip)` for the paste button and scrollers). Names identify widgets (`GetButton(name)`), drive help-text look-ups (`<name>_description[_<mode>]` in `OptionDetailsButton`) and should be unique; duplicates exist (`image` and `info` twice on BuyMeNow; `start_testbed_server` twice under `TESTBED_ENABLED`; the single-player lobby's map description is named `chat_button` like the multiplayer chat log) — with Eclipse's newest-first lookup (§2.2) the later one wins.
3. Appearance/behaviour fields: `m_fontSize` (px), `m_inactive` (greyed, skipped by navigation, most `MouseUp`s early-out), `m_centered`, and widget flags (`m_noBackground`, `m_cycleMode` on drop-downs; `m_shadow`, `m_centered` on `TextButton`; `m_border` on `ImageButton`; `m_renderStyle2` on input fields; `m_transitionDirection = −1` on cancel-like buttons).
4. Bind data: `RegisterNetworkInt(&networkOption)` (drop-down/check box on a lobby option), `RegisterInt(&int)` / `RegisterBool(&bool)` (local), `RegisterNetworkValue(type, value)` (input field), `RegisterString(buf, len)` (auth field), `RegisterUnicodeString(str)` (chat).
5. `RegisterButton(widget)`; then `m_buttonOrder.PutData(widget)` **iff** it should be reachable by controller (titles, images, info panels, MOTD, status boxes and the chat log are not).

Standard page skeleton [C] e.g. `GameMenuWindow::SetupNewOrJoinPage`: `buttonX = leftX + (leftW − buttonW)/2`; title `GameMenuTitleButton "title"` at `(leftX+10, leftY+10, leftW−20, 1.5 bH)`, `m_fontSize = fontMed`; `yPos = leftY + 0.5 bH + 1.3 bH`; each stacked button at `yPos += bH + gap`; Back at `leftY + leftH − 2 bH`; right-box info panels not navigable.

Helpers: `CreateMenuControl` (§5.1); legacy `GetDefaultPositions(&x, &y, &gap)` (only the Research page): `x = min(200, W/1152 × 200)`; Main/Darwinia `y = H/864 × 200`, `gap = y`; NewOrJoin/JoinGame/GameConnecting/GameSelect `y = H/864 × 175`, `gap = y/1.5`; GameSetup/Research `y = H/864 × 70`, `gap = (H/864) × 60`; other pages leave `y`/`gap` unset [C] `GameMenuWindow::GetDefaultPositions`.

Dynamic rows (server browser): rows are created/destroyed incrementally inside `UpdateJoinGamePage`, registered at run time and inserted into `m_buttonOrder` at `Size() − 1` (before Back), but removed at `m_serverButtonOrderStartIndex + row` — when the list shrinks the wrong order entries can be removed [C mechanics; I consequence] `GameMenuWindow::UpdateJoinGamePage`. The browser scroll bar is created with `numRows = 0`, `winSize = 10` for a 13-row list (`SERVERS_ON_SCREEN = 13`) and its row count updated with `SetNumRows(serverList.Size())` [C].

### 6.6 Navigation on `GameMenuWindow` pages

Built on `DarwiniaWindow::Update` (§3.3) and `UpdateButtonHighlight` (§3.5) [C]:
- Active only while the window has Eclipse focus and `m_lockMenu` is false.
- `MenuDown`/`MenuUp` step through `m_buttonOrder` with wrap-around, skipping `m_inactive` entries; the `SpecialScroll` hook is looked up in the wrong list (`m_buttons[m_currentButton]`, defect F5) — on the server browser this means the scroll-instead-of-move behaviour of `GameServerButton` triggers for whichever registration happens to have that index.
- `MenuActivate` (A) or `MainMenuActivate` (Start, only while `g_app->m_atMainMenu`, which is true throughout the front end) → `MouseUp()` on the cursor entry.
- Back: the window is **not closeable** (`m_closeable = false`), so `MenuClose` is handled by each `BackPageButton`'s render function in gamepad mode (§5.3). Pages without a `BackPageButton` (Main, AuthKeyEntry) have no pad "back"; while a `GameMenuDropDownWindow` is open the menu window is unfocused and not rendered, so B does nothing until a list entry is chosen.
- Mouse: in keyboard input mode hovering makes a widget the navigation cursor; any pad move clears hover mode; several custom widgets ignore hover in gamepad mode (`KickPlayerButton`, `LevelSelectButtonBase::RenderBackground`).
- Initial cursor = index 0 of `m_buttonOrder`, except the lobby (`SetupMultiplayerPage` sets `m_currentButton = m_buttonOrder.Size() − 1`, the Play button) [C] `GameMenuWindow::SetupMultiplayerPage`.
- Navigation is skipped while the lobby chat input is visible or was toggled this frame.
- Text-entry pages begin editing immediately (`BeginTextEdit("auth_input")`, `BeginTextEdit("multiwinia_enterpassword")`); the lobby chat begins editing on `ControlLobbyChat`; `EndTextEdit` happens in `Remove()`.
- `m_lockMenu` is set by `TutorialButton::MouseUp` (freeze while the tutorial game starts) and cleared only by the dead overlay-dialogue dismissal — in practice the window is destroyed when the game starts [C] `code/UI/TutorialButton.h`.
- `m_skipUpdate` is set by `GameMenuInputField` on Enter (§5.6).

### 6.7 Rendering conventions and animations

- Text is drawn twice: an outline pass (`SetRenderOutline(true)`, colour alpha 0 → a 1 px dark border) and a fill pass. Fonts: `g_titleFont` (buttons, titles), `g_editorFont` (rows, chat, scrolling text), `g_gameFont` (server header, gamer tags, overlays) [C].
- Palette (RGBA 0–1): normal caption `(0.6,1.0,0.4,1)`; focused `(1.0,0.3,0.3,1)`; inactive `(0.5,0.5,0.5,1)`; editor-font variant `(1,1,0.3,1)`; focus band `(0.75,0.75,1.0,0.5)`; unfocused row band `(1,1,1,0.1)`; title outline `(0.75,1.0,0.45)` [C] `code/game_menu.cpp:GameMenuButton::Render`, `GameMenuTitleButton::Render`, `code/UI/LevelSelectButtonBase.h`.
- **Button birth cascade**: §5.2.
- **Level-row cascade** [C] `LevelSelectButtonBase::RenderMapName`: `showTime = birth + 1.0 × y/H`; until then the map name is shifted left by `(showTime − now) × 0.1 W` with alpha `showTime − now`.
- **Menu transition** (page swipe) [C] `code/renderer.cpp:Renderer::InitialiseMenuTransition`, `StartMenuTransition`, `AdvanceMenuTransition`, `RenderMenuTransition`: a click calls `InitialiseMenuTransition(1.0, dir)` → the current GL read buffer is copied into one texture (`glCopyTexImage2D`, `GL_RGB`, NEAREST/CLAMP; power-of-two size when required) and the state becomes *Waiting*; `SetupNewPage` (or `g_app->m_doMenuTransition`, consumed in `AdvanceMenuTransition`) calls `StartMenuTransition` → *InProgress* (only if not *Finished*). Every frame just before `Flip`, with `f = (now − start)/1.0` and `fraction = f × 1.2 × 3 = 3.6 f`: the captured frame is drawn as a quad whose edges move inwards by `0.2 × fraction × dir × W` (left/right) and `× H` (top/bottom), slid horizontally by `−W × fraction⁶ × dir`, alpha `max(0, 1 − 0.2 × fraction)`, with a 20 px black-to-transparent strip at its right edge; *Finished* when `f > 1`. Visually the old page shrinks and flies off to the left (dir +1; it grows and flies right for dir −1) and is gone after ≈ 0.28 s (`fraction = 1`); the remaining 0.72 s draw it off-screen. Resolves P18 U6 [C]. Full pass details in [24](24_rendering.md).
- **Backdrop** `GameMenuWindow::RenderBigDarwinian()` (static; also used by every `GameOptionsWindow` and the drop-down window) [C]: (1) full-screen additive smooth quad, black at three corners and `(1.0,0.5,0.1)` at the bottom-left; (2) `sprites/darwinian.bmp` (nearest filtering, tint `(1,0,0,1)`) centred at `(0.75 W, 0.7 H)`, half-size `0.3 W + 5 sin t` px, rotated 0.2 rad; high detail adds 10 echoes, each `size × 1.02`, centre `(−6, −6)` px further, tint `(1,0.2,0.2)` alpha `0.2 × (1 − i/10)`; (3) `mwhelp/gear.bmp`, half-size `0.6 W`, centred, angle `0.2 t` rad, additive white alpha 0.06, drawn 10 times each a further 0.02 rad (none in low detail). Detail switch (evaluated only while `Multiwinia::GameInLobby()`): `smoothFps = smoothFps × (1 − k) + fps × k`, `k = g_advanceTime × 0.5`; low detail when `smoothFps < 20`, high again at `≥ 60`; initial 60.
- Other timed effects [C] (P18 §13): gamer-tag type-change colour flash 1.0 s; achievement row slide-in 750 px/s; connecting dots 2 steps/s (0–3 dots); blink 1.0 s period, 50 % duty (alpha 1.0 / 0.5) for "requesting list"/"no servers"/"loading achievements" captions; caret blink 1 Hz.

### 6.8 `ScrollingTextButton` [C] `code/UI/ScrollingTextButton.h`

Used by credits, MOTD, game-type info, map info and option details.
- Clips drawing to its rectangle (`Renderer::Clip(realX, realY, m_w, m_h)` / `EndClip`).
- Unless `m_ignoreButtons`, whenever the parent's `m_currentButton` changes the scroll timer restarts (descriptions start from the top when focus moves).
- Layout: line gap `int(1.2 × font)`; top margin `int(0.05 h)`, raised to `1.1 × font` if smaller than one font height; left margin `0.05 w`.
- Wrapping: `WordWrapText(m_string, 0.95 w, g_editorFont.GetTextWidth(1, font))` (character width `font × 0.6` for Latin, `× 1.0` for CJK), cached in `m_wrapped`; `ClearWrapped()` invalidates it — the dynamic subclasses (`GameTypeInfoButton`, `MapInfoButton`, `OptionDetailsButton`, `MOTDButton`) set `m_string` and call `ClearWrapped()` every frame, re-wrapping every frame.
- Auto-scroll when content height `0.05 h + lines × gap` exceeds `h`: hold 1.5 s, then move up at `m_scrollSpeed` = 30 px/s; once the whole text has scrolled above its starting top, snap back and restart the timer (endless loop). `MOTDButton` sets the timer 5.0 s into the future for a longer first hold.
- A line beginning with `-` is drawn at 1.25× size without the dash (heading markup).
- Each line: `g_editorFont.DrawText2DSimple`, outline pass `(1,1,1,0)` then white.

### 6.9 Image widgets [C] `code/UI/ImageButton.cpp`, `GameTypeImageButton.h`, `MapImageButton.h`, `HelpImageButton.h`

- `ImageButton(filename)`: if `Resource::DoesTextureExist(m_filename)` draw the texture (loaded **without mip-maps**, `GetTexture(name, false)`, linear filtering) as a full quad, white, alpha-blended; white outline unless `m_border = false`. The file name is `strdup`ed and never freed.
- `GameTypeImageButton`, `MapImageButton`, `HelpImageButton` compute their texture from window state every frame: the mode picture of `m_gameType` (or `m_highlightedGameType` in single player / on GameSelect), the thumbnail of the focused or requested map, the help-page image; they add the "Full Game Only" overlay where relevant and an outline.

### 6.10 `FancyColourButton` — colour/side picker row [C] `code/UI/FancyColourButton.h`

`FancyColourButton(colourId)` (`GameMenuButton("")`, `m_colourIndex`; −1 = "no preference"):
- **Availability** ("taken") is recomputed in both `MouseUp` and `Render`, with different rules:
  - Assault (`m_gameType == GameTypeAssault`): count teams using this colour (Render: excluding unused teams and the local team; MouseUp: all `NUM_TEAMS` slots). With the requested map's `m_numPlayers == 3`: colour 0 is taken if `num > 0`, colour 1 if `num == 2`; otherwise taken if `num ≥ numPlayers/2` (sides: `ASSAULT_DEFENDER_COLOURID = 0`, `ASSAULT_ATTACKER_COLOURID = 1`).
  - Co-op (`m_coopMode`): taken unless `Multiwinia::IsValidCoopColour(index)`.
  - Otherwise: taken if any team uses it (Render: excluding unused teams and the local team; MouseUp: any of the `NUM_TEAMS` slots including the local one).
  - Index −1 is never taken.
- `Render`: sets `m_inactive = taken` (**render-time mutation of navigation state**: taken colours are skipped by the pad cursor from the next frame); "clicked" = the local team already has this colour; row box `(1,1,1,0.1)` or focus band `(0.75,0.75,1.0,0.5)`; caption outline + green, red when clicked, inactive grey; swatch at the right quarter (`x = realX + w − w/4`, `w × 0.95/4` wide, `0.8 h` high, `0.1 h` from the top) shaded horizontally from the team colour (`Multiwinia::GetColour(index)`, grey 0.7 for −1, grey 0.5 when taken) to black; roll-over sound.
- `MouseUp`: return if `m_inactive`; if not taken → `parent->m_currentRequestedColour = index`; outside Assault/co-op also save pref `PlayerPreferedColour` (+ `Save()`); `InitialiseMenuTransition(1.0, −1)` and `m_newPage = PageGameSetup` (no `GameMenuButton::MouseUp`, so **no select sound**). The colour request itself is sent by `GameMenuWindow::SetTeamNameAndColour`, which `UpdateGameOptions(true)` calls every frame on the GameSelect/MapSelect/GameSetup/PlayerOptions pages; first, if the local team id is valid (≠ −1, ≠ 255), the requested colour is not −1, the team does not already have it, it is taken, **and the game is neither co-op nor Assault**, it substitutes pref `PlayerPreferedColour` (when that is free) or else `Multiwinia::GetNextAvailableColour()`; then, only while on PageGameSetup or PagePlayerOptions (or when starting a tutorial), it issues `ClientToServer::ReliableSetTeamColour(m_currentRequestedColour)` — and only if the requested colour is not −1, unless the game is co-op or Assault (then −1 is sent too) [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetTeamNameAndColour`, `GameMenuWindow::UpdateGameOptions` (lobby rules in [09](09_meta_systems.md)).

### 6.11 Other custom widgets with framework-relevant behaviour [C]

| Widget | Framework-relevant behaviour | Ref |
|---|---|---|
| `GameServerButton` | row `r` shows entry `r + scrollBar.m_currentValue`; `SpecialScroll(+1)` on row 0 with an entry 13 further down → `m_scrollBar->m_currentValue++` (direct write, bypasses `SetCurrentValue` clamping); `SpecialScroll(−1)` on row 12 → `m_currentValue--` if > 0 | `code/UI/GameServerButton.h:GameServerButton::SpecialScroll` |
| `LevelSelectButtonBase` | shared row background (ignores mouse hover in gamepad mode), column positions name `0.025 w`, time `0.65 w`, players `0.83 w`, co-op `+0.1 w`, difficulty `+0.1 w`; animated name (§6.7) | `code/UI/LevelSelectButtonBase.h` |
| `GameMenuChatButton` | lobby chat log: fill alpha 200/255, wrap width `1.75 w` (characters × width), lines `1.1 × font`, stops 5 % from the top | `code/UI/GameMenuWindow.cpp:GameMenuChatButton::Render` |
| `GamertagButton` | lobby slot; CPU/unused slots `m_inactive` (not navigable); type-change flash 1.0 s; caption by team state ([09](09_meta_systems.md)) | `code/UI/GamerTagButton.h` |
| `KickPlayerButton` | ignores hover in gamepad mode | `code/UI/KickPlayerButton.h` |
| `WaitingButton` | connection status with animated dots | `code/UI/WaitingButton.h` |
| `TextButton` (defined in `helpandoptions_windows.h`) | plain/shadowed/centred/right-aligned text label used on several pages; its `UnicodeString` constructor leaves `m_string`, `m_rightAligned`, `m_shadow` uninitialised (calls another constructor as a statement) — only the `char*` constructor is used | `code/interface/helpandoptions_windows.cpp:TextButton::TextButton` |

### 6.12 Inventory of every class in `code/UI`

Status: **Live** = instantiated in `[B:FULL]`; **Unreached** = instantiated only on unreachable pages; **Unused** = compiled (header included) but never instantiated; **Uncompiled** = header included nowhere (never compiled although listed in the `.vcproj`); **Not in project** = file absent from `Darwinia.vcproj` [C] include/instantiation grep over `code/` (re-verified for every Uncompiled/Unused row for this document).

| Class | File | Base | Overrides | Status | Purpose / action |
|---|---|---|---|---|---|
| `GameMenuWindow` | GameMenuWindow.h/.cpp | `DarwiniaWindow` | Create (empty), Update, Render | Live | the front end |
| `ServerPasswordString` | GameMenuWindow.h | `NetworkUnicodeString` | Get/Set | Live | ≤ 16-char password; on the host proxies `Server::Get/SetPassword` |
| `GameMenuChatButton` | GameMenuWindow.cpp | `DarwiniaButton` | Render | Live | lobby chat log |
| `AchievementButton` | AchievementButton.h | `GameMenuButton` | Render, MouseUp (empty) | Unused | achievement row |
| `AdvancedOptionsModeButton` | AdvancedOptionsModeButton.h | `GameMenuButton` | MouseUp | Live | → AdvancedOptions unless demo key |
| `AnyLevelSelectButton` | AnyLevelSelectButton.h | `LevelSelectButtonBase` | Render, MouseUp | Unreached (custom-game mode never on) | "any map" → JoinGame |
| `ApplyNameButton` | ApplyNameButton.h | `BackPageButton` | MouseUp | Live | save `PlayerName` pref, back |
| `AuthInputField` | AuthInputField.h | `InputField` | Render, Keypress, MouseUp | Live | key entry (§4.11) |
| `AuthPageButton` | AuthPageButton.h | `GameMenuButton` | MouseUp | Live (in `HelpAndOptionsWindow`) | close parent window, menu → AuthKeyEntry |
| `AuthStatusButton` | AuthStatusButton.h | `GameMenuButton` | Render, MouseUp (empty) | Live | auth status box |
| `BuyNowInfoButton` | BuyNowInfoButton.h | `DarwiniaButton` | Render | Unused | 5 upsell lines |
| `BuyNowModeButton` | BuyNowModeButton.h | `GameMenuButton` | MouseUp | Live (demo) | → BuyMeNow |
| `CSpectatorButton` | CSpectatorButton.h | `GameMenuButton` | Render, MouseUp | Unused (`#ifdef TESTBED`) | request spectator |
| `CampaignButton` | CampaignButton.h | `GameMenuButton` | MouseUp, Render, static LoadCampaign | Unreached `[B:DEBUG]` | Darwinia campaign |
| `ServerConnectButton` | ConnectButton.h | `GameMenuButton` | Render, MouseUp | Live | password Connect |
| `DarwiniaModeButton` | DarwiniaModeButton.h | `GameMenuButton` | MouseUp | Live | "Single Player" |
| `DarwiniaPlusMPButton` | DarwiniaPlusMPButton.h | `GameMenuButton` | MouseUp | Unused `[B:LEGACY]` | → NewOrJoin / BuyMeNow |
| `DarwiniaPlusSPButton` | DarwiniaPlusSPButton.h | `GameMenuButton` | MouseUp | Unused `[B:LEGACY]` | sets single player |
| `EnterAuthkeyButton` | EnterAuthKeyButton.h | `GameMenuButton` | MouseUp | Live | auth Done |
| `ErrorDialog` | ErrorDialog.h/.cpp | `DarwiniaWindow` | — | Not in project `[B:DEAD]` | empty dialog stub |
| `FancyColourButton` | FancyColourButton.h | `GameMenuButton` | MouseUp, Render | Live | colour/side row (§6.10) |
| `FiltersPageButton` | FiltersPageButton.h | `GameMenuButton` | MouseUp | Live | → Filters |
| `FinishMultiwiniaButton` | FinishMultiwiniaButton.h | `BackPageButton` | MouseUp | Live | leave game/lobby |
| `FinishSinglePlayerButton` | FinishMultiwiniaButton.h | `BackPageButton` | MouseUp | Live | single-player lobby back |
| `SaveFiltersButton` | FinishMultiwiniaButton.h | `BackPageButton` | MouseUp | Live | save filter prefs, back |
| `GameMenuOptionsButton` | GameMenuOptionsButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | open `OptionsMenuWindow` |
| `GameServerButton` | GameServerButton.h | `GameMenuButton` | MouseUp, Render, SpecialScroll | Live | browser row |
| `GameTypeAnyButton` | GameTypeAnyButton.h | `GameMenuButton` | MouseUp | Unused | "any mode" → JoinGame |
| `GameTypeButton` | GameTypeButton.h | `GameMenuButton` | Render, MouseUp | Live | mode row |
| `GameTypeImageButton` | GameTypeImageButton.h | `DarwiniaButton` | Render | Live | mode picture |
| `GameTypeInfoButton` | GameTypeInfoButton.h | `ScrollingTextButton` | Render | Live | mode blurb |
| `GamertagButton` | GamerTagButton.h | `GameMenuButton` | MouseUp, Render | Live | lobby slot |
| `HelpImageButton` | HelpImageButton.h | `DarwiniaButton` | Render | Unreached | help image |
| `HelpModeButton` | HelpModeButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | → HelpMenu |
| `HelpSelectButton` | HelpSelectButton.h | `GameMenuButton` | MouseUp | Unused (creation inside a block comment) | → Help page n |
| `ImageButton` | ImageButton.h/.cpp | `DarwiniaButton` | Render | Live | static image (§6.9) |
| `JoinGameButton` | JoinGameButton.h | `GameMenuButton` | MouseUp | Live | → JoinGame |
| `KickPlayerButton` | KickPlayerButton.h | `DarwiniaButton` | MouseUp, Render | Live | kick |
| `LevelSelectButton` | LevelSelectButton.h | `LevelSelectButtonBase` | MouseUp, Render | Live | map row |
| `LevelSelectButtonBase` | LevelSelectButtonBase.h | `DarwiniaButton` | (helpers) | Live (base) | column layout, row background, animated name |
| `MapData` | MapData.h/.cpp | — | — | Live | map metadata record ([09](09_meta_systems.md)) |
| `MapImageButton` | MapImageButton.h | `DarwiniaButton` | Render | Live | thumbnail |
| `MapInfoButton` | MapInfoButton.h | `ScrollingTextButton` | Render | Live | map description |
| `MapListTitlesButton` | MapListTitlesButton.h | `DarwiniaButton` | Render | Live | map-list header |
| `MapSelectModeButton` | MapSelectModeButton.h | `GameMenuButton` | MouseUp (returns at once), Render | Live | lobby map name |
| `MultiwiniaEditorButton` | MultiwiniaEditorButton.h | `GameMenuButton` | MouseUp | `[B:EDITOR]` | open editor (`MultiwiniaWindow`) |
| `MultiwiniaModeButton` | MultiwiniaModeButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | → GameSelect + MP server |
| `MultiwiniaVersusCpuButton` | MultiwiniaVersusCpuButton.h | `GameMenuButton` | MouseUp | Unreached `[B:DEBUG]` | → GameSelect |
| `NewOrJoinButton` | NewOrJoinButton.h | `GameMenuButton` | MouseUp | Live | "Multiplayer" |
| `NewServerButton` | NewServerButton.h | `GameMenuButton` | MouseUp | Live | "Host" → GameSelect |
| `NextHelpButton` | NextHelpButton.h | `GameMenuButton` | MouseUp | Unreached | next help image |
| `OptionDetailsButton` | OptionDetailsButton.h | `ScrollingTextButton` | Render | Live | option help |
| `PasteKeyButton` | PasteKeyButton.h | `GameMenuButton` | MouseUp | Live | paste key from clipboard |
| `PlayDemoAuthkeyButton` | PlayDemoAuthkeyButton.h | `GameMenuButton` | MouseUp | Live (not in HWCOMPAT/BETA/ASSAULT targets) | demo key |
| `PlayGameButton` | PlayGameButton.h | `GameMenuButton` | Render, MouseUp | Live | Play / Ready |
| `PrevHelpButton` | PrevHelpButton.h | `GameMenuButton` | MouseUp | Unreached | previous help image |
| `PrologueButton` | PrologueButton.h | `GameMenuButton` | MouseUp, Render, static LoadPrologue | Unreached `[B:DEBUG]` | Darwinia prologue |
| `QuickMatchButton` | QuickMatchButton.h | `GameMenuButton` | MouseUp | Unused (creation commented "doesn't work") | → QuickMatch |
| `QuickStartButton` | QuickStartButton.h | `GameMenuButton` | Render, MouseUp | Uncompiled `[B:DEAD]` | quick KOTH game |
| `QuitAuthkeyButton` | QuitAuthKeyButton.h | `GameMenuButton` | MouseUp | other targets only (HWCOMPAT/BETATEST/ASSAULT) | quit from the key page |
| `QuitButton` | QuitButton.h | `GameMenuButton` | MouseUp | Live | quit |
| `RequestAIButton` | RequestAIButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | add CPU team |
| `ResearchModeButton` | ResearchModeButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | → Research |
| `RollingDemoButton` | RollingDemoButton.h | `GameMenuButton` | MouseUp | Uncompiled `[B:DEAD]` | attract/rolling demo |
| `ScrollingTextButton` | ScrollingTextButton.h | `DarwiniaButton` | Render | Live | §6.8 |
| `ServerTitleInfoButton` | ServerTitleInfoButton.h | `DarwiniaButton` | Render | Live | browser header |
| `StartTestbedButton` | TestBedButtons.h | `GameMenuButton` | MouseUp | `[B:DEAD]` (`TESTBED_ENABLED`) | start test bed |
| `TutorialPageButton` | TutorialButton.h | `GameMenuButton` | MouseUp | Live (`INCLUDE_TUTORIAL`) | → Tutorials |
| `TutorialButton` | TutorialButton.h | `GameMenuButton` | Render, MouseUp | Live | launch tutorial (sets `m_lockMenu`) |
| `UnlockFullGameButton` | UnlockFullGameButton.h | `GameMenuButton` | Render, MouseUp | Live (demo) | open store (forces 1024×768 windowed) |
| `WaitingButton` | WaitingButton.h | `DarwiniaButton` | Render | Live | connection status |
| `CreditsButton` | credits_button.h | `GameMenuButton` | MouseUp | Live | → Credits |
| `MapDetailsButton` | mapDetailsButton.h | `DarwiniaButton` | Render | Uncompiled `[B:DEAD]` | play-time/difficulty/players panel |
| `MOTDButton` | motdButton.h | `ScrollingTextButton` | Render | Live | message of the day |
| `VolumeControl` | soundsliderbutton.h | `DarwiniaButton` | — | Not in project `[B:DEAD]` | master-volume slider (`SoundMasterVolume`) |
| `UpdatePageButton` | update_page_buttons.h | `GameMenuButton` | MouseUp | Live (when an update exists) | → UpdateAvailable |
| `RunAutoPatcherButton` | update_page_buttons.h | `GameMenuButton` | MouseUp (Win32 only) | Live | run `autopatch.exe`, exit |
| `RunSparkleButton` | update_page_buttons.h | `GameMenuButton` | MouseUp | macOS only `[B:DEAD]` | Sparkle update |
| `VisitWebsiteButton` | update_page_buttons.h | `GameMenuButton` | MouseUp, Render | Live | open UpdateURL |

Totals: 79 classes = 76 widget classes + `GameMenuWindow` + `ServerPasswordString` + `MapData` (plus the file-local `ServerKey` struct). Of the 76 widget classes: 47 live (counting the base `LevelSelectButtonBase`), 7 unreached, 8 unused, 8 uncompiled, 2 not in the project, 4 other-build only (`QuitAuthkeyButton`, `StartTestbedButton`, `MultiwiniaEditorButton`, `RunSparkleButton`) [C] (the generated inventory of §12.3 agrees: 50 live rows for `code/UI`).

---
## 7 Layout conventions and formulas

The two framework styles use different layout systems: Darwinia-style tool windows use **fixed pixels** (optionally multiplied by the large-menu scale); front-end pages use **fractions of the screen size**, recomputed in every `Create()`/`Setup*Page()` [C].

### 7.1 Front-end page grid [C] `code/game_menu.cpp:GetPositions`, `GetPosition_TitleBar`, `GetPosition_LeftBox`, `GetPosition_RightBox`, `GetFontSizes`, `GetButtonSizes`

| Quantity | Formula (W, H = screen pixels) |
|---|---|
| `horizSplit` | `0.26 H` |
| `vertSplit` | `0.6 W` |
| `gap` | `0.05 × vertSplit` (= `0.03 W`) |
| outer edges | `left = top = gap`; `right = W − gap`; `bottom = H − gap`; if `W/H > 1.4` (widescreen): `left += 0.08 W`, `right −= 0.08 W` |
| Title bar | `x = left`, `y = top`, `w = right − left`, `h = horizSplit − 0.5 gap − top` |
| Left box | `x = left`, `y = horizSplit + 0.5 gap`, `w = vertSplit − 0.5 gap − left`, `h = bottom − y` |
| Right box | `x = vertSplit + 0.5 gap`, `y = horizSplit + 0.5 gap`, `w = right − x`, `h = bottom − y` |
| Fonts | `large = 0.095 H`; `med = 0.04 H` (written `H × 0.1 × 0.4`); `small = 0.022 H` (`H × 0.1 × 0.22`) |
| Standard button | `w = 0.9 × leftW`; `h = 1.3 × med` (= `0.052 H`); `gap = 0.1 × med` (= `0.004 H`) |

Worked examples (computed from the formulas; pixels):

| Screen | Title bar (x, y, w, h) | Left box (x, y, w, h) | Right box (x, y, w, h) | Fonts L / M / S | Button w × h, gap |
|---|---|---|---|---|---|
| 800×600 (4:3) | 24.0, 24.0, 752.0, 120.0 | 24.0, 168.0, 444.0, 408.0 | 492.0, 168.0, 284.0, 408.0 | 57.00 / 24.00 / 13.20 | 399.6 × 31.20, 2.40 |
| 1024×768 (4:3) | 30.7, 30.7, 962.6, 153.6 | 30.7, 215.0, 568.3, 522.2 | 629.8, 215.0, 363.5, 522.2 | 72.96 / 30.72 / 16.90 | 511.5 × 39.94, 3.07 |
| 1280×720 (16:9, widescreen) | 140.8, 38.4, 998.4, 129.6 | 140.8, 206.4, 608.0, 475.2 | 787.2, 206.4, 352.0, 475.2 | 68.40 / 28.80 / 15.84 | 547.2 × 37.44, 2.88 |
| 1920×1080 (16:9, widescreen) | 211.2, 57.6, 1497.6, 194.4 | 211.2, 309.6, 912.0, 712.8 | 1180.8, 309.6, 528.0, 712.8 | 102.60 / 43.20 / 23.76 | 820.8 × 56.16, 4.32 |

Page composition conventions [C] (all `Setup*Page`, `GameOptionsWindow` subclasses):
- Button column centred in the left box: `buttonX = leftX + (leftW − buttonW)/2`.
- Page title: `GameMenuTitleButton` at `(leftX+10, leftY+10, leftW−20, 1.5 bH)`, font `med` (rendered at `1.5 × med`, shrunk to fit).
- First stacked row: `y = leftY + 0.5 bH + 1.3 bH`, then `y += bH + gap` before each row.
- Back anchored at `leftY + leftH − 2 bH`; Apply + Back pairs at `− 3 bH` / `− 2 bH`.
- Options pages shrink rows to `0.65 × bH` and use `fontSmall`; compact controls on the lobby use `0.66 × bH`; the server browser rows are `0.6 × bH` high and `0.9 × titleW` wide.
- Right box carries information panels (images, scrolling descriptions, MOTD, chat, status) that are never navigable.
- Thumbnails `0.8 rW × 0.6 rW` (4:3); mode images `0.8 rW × (2/3)(0.8 rW)` [C] `GameMenuWindow::SetupMultiplayerPage`, `SetupGameSelectPage`.
- Text baseline convention for a row of height `h` and font `s`: `y + h/2 − s/2 + 7` (the `+7` compensates the `−7` offset inside `TextRenderer::DrawText2DSimple`) [C] `code/game_menu.cpp:GameMenuButton::Render`, `code/lib/text_renderer.cpp:DrawText2DSimple`.

### 7.2 Darwinia-style window layout [C] `code/interface/darwinia_window.cpp`

| Item | Value |
|---|---|
| Title bar height | `GetMenuSize(15)` px; client area `x ∈ [2, w−2]`, `y ∈ [GetMenuSize(15)+1, h−2]` |
| Close box | 10 × 10 at `(w − 12, 2)` |
| Auto button size | `w = chars × 7 + 9`, `h = 15` |
| Value control | field `w − 37` wide (numeric), height `GetMenuSize(15)`; scrollers 15 × 15 at `+5` and `+22` px right of the field |
| Row pitch used by tool windows | typically 17–20 px (`y += 17` in the editor windows) [I: convention, not enforced] |
| Window height | grows automatically to `lowest button bottom + 10` (§2.2) |
| Placement | caller-set before `EclRegisterWindow`; child windows placed randomly near the parent when left at (0,0) (§2.2); clamped on-screen at registration |
| Large menus | every `GetMenuSize`/`SetMenuSize` value × `0.96 H / 460` (§3.2) — front-end pages are unaffected except that `GameOptionsWindow` sizes itself with `SetMenuSize(W, H)` (defect F15) |

Older Darwinia small-window pattern (P17b §11.5) [C]: the legacy windows (key bindings, new profile, mods, really-quit, save-on-quit, about, profiler) use fixed pixel sizes passed through `SetMenuSize`/`GetMenuSize` (scaled only when Large Menus = 2), centred placement, `InvertedBox` panels, a `CloseButton`, `CreateValueControl` numeric/string fields and `g_editorFont`. No live Multiwinia options screen uses this pattern; in `[B:FULL]` it survives only in the F9 `NetworkWindow` and the renderer/sound `MessageDialog`s.

### 7.3 Resolution changes [C] `code/interface/prefs_screen_window.cpp:RestartWindowManagerAndRenderer`, `AdjustWindowPositions`

On a screen-mode change the renderer and window are destroyed and re-created (a temporary 640×480 windowed mode is created first when switching resolution in full-screen); then, if the size changed, `EclInitialise(newW, newH)` is called and every window is re-centred proportionally — `centre' = centre × new/old` per axis — keeping its size; the Screen Options apply then rebuilds the menus (full-screen pages recompute their layout from the new size) [C] (P17b §3.1). The only other `EclInitialise` call is the legacy `WebsiteButton` (800×600) [C] `code/interface/mainmenus.cpp`.

---

## 8 Rendering of windows (palette, textures, blend states, fonts)

### 8.1 Pass and GL state contract [C] `code/user_input.cpp:UserInput::Render`, `code/lib/text_renderer.cpp:TextRenderer::BeginText2D`, `EndText2D`

- One UI pass per frame (`EclRender`), drawn back-to-front, inside `g_editorFont.BeginText2D()`: matrices saved; orthographic projection over the viewport shifted −0.325 px; lighting off; colour white; blending on; culling off; depth test off; depth writes off. `EndText2D` restores matrices, depth mask/test and culling, and **disables blending**.
- All drawing is fixed-function immediate mode (`glBegin/glEnd` quads, lines, line loops); `glShadeModel(GL_SMOOTH)` only for gradients, reset to `GL_FLAT`; widgets set `glLineWidth` ad hoc (1, 2, 3 or 5 px). No scissor except `Renderer::Clip`/`EndClip` in `ScrollingTextButton` and `ChatInputField`; no widget clips its caption (long captions overflow their rectangle) [C].
- Under `USE_DIRECT3D` the same calls go through the GL-on-D3D9 emulation ([24](24_rendering.md)).

### 8.2 Fonts and text blend modes [C] `code/lib/text_renderer.cpp`, `code/app.cpp:App::SetLanguage`

| Global | Atlas family | Role in the UI | Horizontal spacing factor |
|---|---|---|---|
| `g_gameFont` | `"speccy"` | Darwinia window titles, server header, gamer tags, overlays | 1.0 |
| `g_editorFont` | `"editor"` | Darwinia captions, input values, rows, chat, scrolling text, message dialogs | 0.91 |
| `g_titleFont` | `"square"` | front-end captions and titles; the only family with CJK/Hangul atlases | 1.03 |

- Glyphs are fixed-advance cells: width = `size × horizScale` (0.6 for Latin/Cyrillic; 1.0 for code points ≥ U+3000 in CJK languages) × spacing factor; `GetTextWidth(n, size = 13) = n × size × horizScale` (approximation used by layout code), `GetTextWidthReal` sums per-character advances [C] ([24 text rendering](24_rendering.md)).
- Blend modes: normal text `GL_SRC_ALPHA, GL_ONE` (**additive** — text brightens what is behind); shadow or outline text `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR` — "ink": with the usual colour alpha 0 (outline idiom) or 30/255 (Darwinia highlighted captions) the glyph contributes little and multiplies the destination by `(1 − glyph colour)`, i.e. darkens it [I: blend arithmetic; visual not verified]. Outline mode draws the glyph 8 times at ±1 px offsets. After each string the blend func is reset to `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` and texturing is turned off; blending is left enabled ("Eclipse needs it").
- `DEF_FONT_SIZE = 12.0` [C] `code/lib/text_renderer.h`.

### 8.3 Palette

Darwinia layer — "red glass panel + steel-blue highlight" (RGBA bytes unless stated) [C] §3–§4:

| Token (proposed name) | Value | Used for |
|---|---|---|
| `panel.bodyTint` | (255,255,255,245) × texture `textures/interface_red.bmp` | window body |
| `accent.light` | (199,214,220,255) | title-bar top, window border, button highlight top, check-box highlight |
| `accent.dark` | (112,141,168,255) | title-bar bottom, highlight gradient bottom |
| `accent.pressedTop` / `accent.pressedBottom` | (255,255,255,255) / (162,191,208,255) | pressed button gradient |
| `frame.outer` | (42,56,82,255) | 1 px outer window frame |
| `button.fill` | (107,37,39,64) | idle button |
| `button.edgeLight` | (100,34,34,200); borderless 250; inverted/edit well 150 | top/left bevel |
| `button.edgeDark` | `glColor4f(0.1,0,0,1)` ≈ (25.5,0,0,255) | bottom/right bevel |
| `text.normal` | (255,255,255,255) additive | captions |
| `text.disabled` | (128,128,128,255) | disabled captions |
| `text.ink` | (255,255,150,30) in shadow blend; disabled (128,128,75,30) | captions on highlighted backgrounds, window title |
| `well.fill` | `glColor4f(0.1,0,0,0.5)` ≈ (25.5,0,0,127) | input-field edit well |
| `inset.fill` | `glColor4f(0.05,0,0,0.4)` ≈ (12.75,0,0,102) | `InvertedBox` |
| `close.fill` / `close.hot` | (60,37,39,64) / (160,137,139,64); edge (0,0,150,100) | close box |
| `scroll.track` / `scroll.outline` | (85,93,78) / (187,187,187) | scroll bar |
| `scroll.thumb` normal / hot / pressed | (65,71,120) / (55,60,120) / (50,55,120) | scroll thumb |
| `list.selected` | `(0.3,0.3,1.0,0.5)` ≈ (76,76,255,127); Multiwinia file dialog (191,191,255,127) | selected file rows |
| `check.box` / `check.tick` | (76,76,76,255) / tint (153,255,102,255) | check box |

Front-end layer (RGBA 0–1) [C] §5–§6:

| Token (proposed name) | Value | Used for |
|---|---|---|
| `fe.caption` | (0.6,1.0,0.4,1) | idle captions, check ticks, auth result |
| `fe.captionFocus` | (1.0,0.3,0.3,1) | focused / selected captions |
| `fe.captionInactive` | (0.5,0.5,0.5,1) | inactive captions |
| `fe.captionAlt` | (1.0,1.0,0.3,1) | `m_useEditorFont` buttons |
| `fe.focusBand` | (0.75,0.75,1.0,0.5) | focused row background |
| `fe.rowBand` | (1,1,1,0.1) | unfocused row background |
| `fe.titleOutline` | (0.75,1.0,0.45,1) | page title outline pass |
| `fe.beamTop` / `fe.beamBottom` | (0.6,1.0,0.4,0.8) / (0.3,0.5,0.2,0.7) | highlight beam |
| `fe.boxTint` | (0.8,0.8,1.0,0.9) × `textures/interface_blur.bmp` | fill box body |
| `fe.boxGlow` | (0.3,0.6,0.9,0.4) additive × `textures/cloudyglow.bmp` | fill box glow |
| `fe.boxOutline` / `fe.boxInner` | (0.8,0.7,1.0,0.8) 2 px / alpha 0.4 1 px | fill box outlines |
| `fe.flash` | (1,1,1, ≤ 0.9) additive | button birth flash |
| `fe.backdropGlow` | (1.0,0.5,0.1) at bottom-left, additive | backdrop |
| `fe.textBox` | (10,10,10,255) bytes + white outline | style-2 input, auth field |

### 8.4 Textures used by the framework [C]

| Texture | Used by | Notes |
|---|---|---|
| `textures/interface_red.bmp` | `DarwiniaWindow::Render` body | 64×510 on disk; code tiles it as if 512×64 (s × 8) |
| `icons/selectionarrow.bmp` | `ScrollChangeButton::Render` | 64×64; flipped for the up arrow |
| `icons/tick.bmp` | `CheckBox::Render`, `GameMenuCheckBox::Render` | 50×50, 32-bit |
| `icons/button_b.bmp` | `BackPageButton::Render`, `MenuCloseButton::Render` (gamepad mode) | pad "B" glyph |
| `textures/interface_blur.bmp`, `textures/cloudyglow.bmp` | `DrawFillBox` | |
| `textures/menuhighlight.bmp` | `RenderHighlightBeam` | |
| `icons/mwlogo.bmp` | `GameOptionsWindow::Render`, `GameMenuDropDownWindow::Render`, `GameMenuWindow::Render` | aspect 277/194, drawn mirrored twice |
| `textures/mwtitle.bmp` | `GameMenuWindow::Render` (when the localised title is exactly "MULTIWINIA") | aspect 5.4:1, width = text width × 1.1 |
| `sprites/darwinian.bmp`, `mwhelp/gear.bmp` | `GameMenuWindow::RenderBigDarwinian` | backdrop |
| `textures/unicode/<font>.<lo>.<hi>.bmp` | all text | glyph atlases ([24](24_rendering.md), [25](25_textures_and_assets.md)) |
| `camera.shp` (shape) | editor camera-mount gizmo drawn by `LocationEditor::Render` | [B:EDITOR] ([08](08_levels_and_level_design.md)) |

### 8.5 Front-end composition order of one frame (main menu)

`GameMenu::Render` (3D clouds, sky) → … → UI pass: focused full-screen page only (`GameMenuWindow` or a `GameOptionsWindow`): backdrop → title fill box + title/logo → content fill boxes → overlays → buttons back-to-front → in-page dialogue → after the UI pass: cursor, task-manager layer, fades → menu-transition overlay → `Flip` [C] `code/renderer.cpp:Renderer::RenderFrame`, §6.7.

### 8.6 State leaks [C]

| Leak | Effect | Ref |
|---|---|---|
| `ScrollChangeButton::Render` leaves `glBlendFunc(SRC_ALPHA, ONE)` | following primitives drawn additive until the next text draw resets the func | `code/interface/scrollbar.cpp:ScrollChangeButton::Render` |
| `ScrollChangeButton::Render` ends with `glDepthMask(true)` inside the UI pass | depth writes re-enabled for the rest of the pass (harmless while depth test is off) | same |
| Text rendering resets blend func after every string | widgets relying on an inherited blend func get `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` | `code/lib/text_renderer.cpp` |

### 8.7 New-engine mapping note [I]

Everything above is fixed-function immediate mode. A DX12 port can batch all UI quads/lines into one dynamic vertex buffer per frame with three blend states — alpha (`SRC_ALPHA, INV_SRC_ALPHA`), additive (`SRC_ALPHA, ONE`) and "ink" (`SRC_ALPHA, INV_SRC_COLOR`) — plus a scissor stack for clipped panels and a captured-backbuffer texture for the page swipe.

---

## 9 Keyboard, controller and mouse navigation rules

### 9.1 Bindings consumed by the framework [C] `data/input/input_preferences.txt` (the file always used: `MultiwiniaControlMethod` is forced to 1 on every preferences load [C] `code/lib/input/file_paths.cpp:InputPrefs::GetSystemPrefsPath`, `code/lib/preferences.cpp:PrefsManager::Load`)

| Control | Binding(s) | Consumer |
|---|---|---|
| `MenuUp` / `MenuDown` | XInput d-pad up/down, left and right thumbsticks up/down — all `repeat` | `DarwiniaWindow::Update` |
| `MenuLeft` / `MenuRight` | XInput d-pad / thumbsticks left/right (`repeat`) | `GameMenuInputField` number adjust |
| `MenuActivate` | XInput A (down) | `MouseUp` of the navigation cursor |
| `MainMenuActivate` | XInput Start (down) — only while `g_app->m_atMainMenu` | same |
| `MenuClose` | XInput Back, XInput B (down) | `m_backButton->MouseUp()` if `m_closeable`; `BackPageButton::Render` in gamepad mode |
| `MenuEscape` | key Esc (down), XInput Start (down) | screen level (opens the in-game `LocationWindow`, leaves editor loops) |
| `CloseTaskHelp` | mouse left down, Space, Enter, Esc, XInput A (all down) | dismisses in-page error dialogues |
| `EclipseLMouseDown` / `EclipseLMousePressed` | mouse left down / pressed; `ShowMouseCursor && XInput a down/pressed` | Eclipse pump, scrollers |
| `EclipseRMousePressed` | mouse right **down** (edge) | (ignored by Eclipse; editor guide grid) |
| `ShowMouseCursor` | XInput left bumper (pressed) | pad mouse emulation |
| `ScrollSpeedup` | Shift (pressed) | ×5 scroller step |
| `FileMultiSelect` | Control (pressed) | file dialogs |
| `LobbyChat` | Enter (P10) | lobby chat entry |

There are **no keyboard bindings for `MenuUp/Down/Left/Right/Activate/Close`** in the shipped file — keyboard users navigate with the mouse; arrow-key navigation exists only in the unused Darwinia control set (`data/input/input_preferences_darwinia.txt`: `MenuUp/Down/Left/Right = key up/down/left/right down`, `MenuActivate = key enter down`) which is selected only when `MultiwiniaControlMethod == 0` [C].

### 9.2 Rules

1. **One navigable list per window**: `m_buttonOrder` in registration order chosen by the screen; the cursor is `m_currentButton` (index). Only the focused window navigates [C] §3.3.
2. **Down/Up** move to the next/previous entry whose `m_inactive` is false, wrapping at both ends; a widget may consume the move via `SpecialScroll` (looked up in the wrong list, F5) [C].
3. **Activate** = `MouseUp()` of the cursor entry, i.e. exactly what a mouse click does; disabled/inactive checks are the widget's own responsibility [C].
4. **Back**: `MenuClose` → `m_backButton->MouseUp()` on closeable windows; on `GameMenuWindow` pages each `BackPageButton` polls `MenuClose` from its render in gamepad mode [C] §5.3.
5. **Left/Right** only change numeric `GameMenuInputField`s that are the cursor; drop-downs in cycle mode change only on Activate (next value) [C] §5.5, §5.6.
6. **Mouse and pad coexist**: in keyboard input mode the hovered widget becomes the cursor (`UpdateButtonHighlight`); any pad move clears hover mode for one update so the two never fight; a widget draws focused when (hover mode and hovered) or it is the cursor [C] §3.5.
7. **Text entry** is modal per window: while `m_currentTextEdit ≠ "None"` every `WM_CHAR` goes to that field; navigation keeps running (pad buttons are not characters), and `GameMenuInputField` sets `m_skipUpdate` on Enter so that one navigation update is skipped [C] §4.1, §5.6.
8. **Locks**: `m_lockMenu` freezes navigation (error dialogues, tutorial launch); `m_skipUpdate` skips exactly one update [C].
9. **Open drop-down**: the full-screen list window is front-most and focused; its own `m_buttonOrder` holds the options with the current value as the cursor; choosing an option closes it [C] §5.5.
10. **Input mode** (P10): `INPUT_MODE_KEYBOARD` (1) or `INPUT_MODE_GAMEPAD` (2), chosen by the most recently active driver with the gamepad preferred; it controls hover-to-focus, the B-glyph hints, the `_kbd`/`_xin` phrase variants (§10.2) and the controller-unplugged dialogue [C] `code/lib/input/input.cpp:InputManager::getInputMode` ([06](06_player_control_and_input.md)).

---
## 10 Localisation integration

### 10.1 Phrase lookup and fallbacks [C] `code/lib/language_table.h`, `code/lib/language_table.cpp:LangTable::LookupPhrase`

| Macro / call | Expands to | Semantics |
|---|---|---|
| `LANGUAGEPHRASE(key)` | `g_app->m_langTable->LookupPhrase(key)` → `const UnicodeString&` | table for the current input mode; NULL key or no table → `"ERROR (null)"`; **missing key → the key text itself** (no English fallback at phrase level). The miss result lives in one shared member (`m_notFound`), overwritten by the next miss — copy it before the next lookup |
| `ISLANGUAGEPHRASE(key)` / `ISLANGUAGEPHRASE_ANY(key)` | `DoesPhraseExist(key)` | existence test (used to choose between phrase and raw text) |
| `RAWLANGUAGEPHRASE(key)` | `RawLookupPhrase(key)` | `key_<mode>` then `key`, else `"ERROR (key)"` |

Language-level fallback exists only at load time: `App::InitLanguage` tries pref `TextLanguage`, then the OS locale's English language name, then `"english"`, then the first file in `language/`; `App::SetLanguage(name)` loads `language/<name>.txt` into a new `LangTable`, overlays `strings_<name>.txt` or `strings_default.txt` if present, calls `setlocale(LC_CTYPE, name)` (so `StrUpr` upper-cases correctly), swaps fonts (§10.5), rebuilds the input-mode tables and keeps the old table alive for 5 main-loop iterations (`DelayedJob`) so frames still holding references finish [C] `code/app.cpp:App::SetLanguage`, `App::InitLanguage` (P14 §31.11). Language files: grammar in [29](29_data_formats_reference.md) (`key rest-of-line`, `\n` → space + newline, later duplicates replace earlier, case-insensitive keys).

### 10.2 Input-mode variants and caption markup [C] `code/lib/language_table.cpp:LangTable::RebuildTables`, `buildCaption`

Two final tables are built — keyboard (suffix `_kbd`) and gamepad (suffix `_xin`); `foo_kbd`/`foo_xin` override `foo` in the matching mode, and a key present only in the other mode resolves to an empty string (the caption disappears). Captions are expanded at table-build time: `[IFMODE KEYBOARD]…[ELSE]…[ENDIF]`, `[KEY<control>]` (replaced by the noun of the input currently bound to the control), `[phrase_key]` references (recursive), output limit 1024 characters. Consequence for the UI: switching between mouse/keyboard and pad switches every caption that has variants with no UI code involved; but any caption cached in a widget (`m_caption` set at page build) only changes when the page is rebuilt [I: widgets copy the `UnicodeString` at `SetShortProperties` time].

### 10.3 How the UI layers use the table [C]

| Pattern | Mechanism | Ref |
|---|---|---|
| Window name = phrase key | `DarwiniaWindow` title = upper-cased `LANGUAGEPHRASE(name)` — but the drawn title is always `multiwinia_mainmenu_title` (§3.4); `GameMenuDropDownWindow` and the in-game menu use `LANGUAGEPHRASE(m_name)` for their page title | `DarwiniaWindow::DarwiniaWindow`, `code/game_menu.cpp:GameMenuDropDownWindow::Create`, `code/interface/mainmenus.cpp:LocationWindow::Create` |
| Widget name = phrase key | `GameMenuDropDown` label = `LANGUAGEPHRASE(m_name)`; `CreateMenuControl` caption = `LANGUAGEPHRASE(name)`; option help = `<name>_description[_<mode>]` | §5.1, §5.5, `code/UI/OptionDetailsButton.h` |
| Explicit caption | most buttons: `SetShortProperties(id, …, LANGUAGEPHRASE(id))` with the same id for name and caption | e.g. `GameMenuWindow::SetupNewOrJoinPage` |
| Value-control labels | phrase if `ISLANGUAGEPHRASE(name)`, else the raw name (editor names like `"R0"` show raw) | `DarwiniaWindow::CreateValueControl` |
| Drop-down captions | closed caption translated if the option word is a key; the Darwinia open list shows raw words; the front-end open list translates | §4.6, §5.5 |
| Placeholders | `UnicodeString::ReplaceStringFlag(flag, text)` replaces every `*<flag>` (`*F` file name, `*S` status) | `code/interface/multiwinia_window.cpp:CreateMapButton::MouseUp`, `code/UI/AuthInputField.h` |
| Implicit narrowing | `UnicodeString → const char*` conversion (characters > U+00FF become `?`) lets translated strings be passed where `char*` keys/words are expected (`menu->AddOption(LANGUAGEPHRASE("buildingname_gunturret"), …)`, `CreateValueControl(LANGUAGEPHRASE("editor_threshold"), …)`) — lossy for non-Latin languages | `contrib/systemiv/lib/unicode/unicode_string.h` |
| Untranslated UI | cheat window, debug menu, network window, camera-animation window, gesture/grabber windows, `MessageDialog` texts from the renderer and sound system, `"PAUSED"`, `"INCOMPATIBLE : <version>"`, `"Update"` use literal English | [C] grep of the caption literals in `code/interface/*.cpp`, `code/lib/*.cpp`, `code/sound/*.cpp` |
| Printf safety | `InputField` rejects `%` so typed text is safe if later passed to printf-style `DrawText2D(char*, …)`; `MessageDialog` lines *are* drawn through the printf overload | §4.1, §4.7 |

### 10.4 Unicode strings [C] `contrib/systemiv/lib/unicode/unicode_string.*` (P14 §29.L.6)

`UnicodeString` owns a `wchar_t` (UTF-16 on Win32) buffer plus a parallel narrow copy (`?` for code points > 255); `Length()` = buffer size − 1 (not `wcslen`); `WcsLen()` = `wcslen`; comparisons `wcscmp`; `StrUpr/StrLwr` via `_wcsupr/_wcslwr` (locale-dependent); `Truncate(n)` resizes the buffer; `ReplaceStringFlag`; `LastHanCharacter` for CJK word-wrap; `Trim()` advances the buffer pointer past leading spaces (heap-corruption risk on a later `delete[]`). Text rendering is BMP-only: surrogate pairs are decoded with an OR instead of an ADD and stored back into a 16-bit unit, so astral characters cannot render [C] ([24](24_rendering.md)).

### 10.5 Font selection per language [C] `code/app.cpp:App::SetLanguage`

| Language (`TextLanguage`) | Fonts used | Horizontal scale |
|---|---|---|
| `chinese-trad`, `chinese-simp`, `korean`, `japanese` | `g_gameFont` and `g_editorFont` become struct copies of `g_titleFont` ("square", the only family with CJK/Hangul atlases); originals saved in `g_oldGameFont`/`g_oldEditorFont` | 1.0 for all three (Latin characters < U+3000 still use the default 0.6) |
| every other language | original `speccy` / `editor` / `square` (restored from the saved copies if a CJK language was active) | 0.6 |

Shipped data [C] `data/language/*`, `data/textures/unicode/*`: `english` (8-bit, 2425 keys), `french` (UTF-16BE, 2516), `german` (UTF-16BE, 2489), `italian` (UTF-16BE, 2365), `spanish` (UTF-16BE, 2516), `russian` (UTF-16LE, 2142), `japanese` (UTF-16LE, 221), `korean` (UTF-16BE, 161), `portuguese` (UTF-16BE, 161) — these are **entry lines** (non-blank lines with a key); the **distinct phrases actually loaded** are english 2423, french 2514, german 2487, italian 2361, spanish 2514, russian 2141, japanese 220, korean 160, portuguese 160, because a repeated key replaces the earlier entry (`code/lib/language_table.cpp:LangTable::ParseLanguageFile`) and the UTF-16 readers drop an unterminated last line, which the japanese, korean and portuguese files have (`contrib/systemiv/lib/filesys/text_stream_readers.cpp`, [29 §6.1](29_data_formats_reference.md#61-language-phrase-tables-and-caption-markup)); no `chinese-*` files. The Cyrillic atlases (`U+0410–042F`, `U+0430–044F`) of `editor`, `speccy` and `square` are byte-identical copies of the speccy glyphs (md5 equal), matching `docs/RussianFontRequest.txt`. Because missing phrases return their key, choosing a language with a partial file (Japanese, Korean, Portuguese) would show raw phrase ids for most of the UI [I: lookup rule + key counts] — whether those files ship depends on the per-SKU `language.dat` list ([31](31_platform_build_and_tools.md)); see 28-U3.

### 10.6 Text entry of non-Latin text [C]

`WM_CHAR` delivers UTF-16 code units to Eclipse (Unicode window class), and `InputField::Keypress` appends any non-control unit not matched earlier, so precomposed non-Latin characters can be typed into string fields. There is no IME composition support (no `WM_IME_*` handling; with a Unicode window `DefWindowProcW` normally converts IME results into `WM_CHAR`, so committed IME text probably arrives as characters [I]), no caret movement (append/backspace only), no selection, no clipboard except the dedicated Paste button of the auth page (`CF_TEXT`, ANSI) [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::PasteFromClipboard`.

### 10.7 Runtime language switch

Other Options → Language → Apply: `App::SetLanguage(name)`, then `RemoveAllWindows()` and, at the main menu, `GameMenu::CreateMenu()` (fresh front end on `PageMain`); in game a new `LocationWindow` [C] `code/interface/prefs_other_window.cpp:ApplyOtherButton::MouseUp`. All captions are re-read because every window is rebuilt.

---

## 11 How a new screen is built (recipes) and re-implementation guidance

### 11.1 Recipe A — Darwinia-style tool window (editor/debug pattern)

Pseudocode distilled from `LightsEditWindow`, `TreeWindow`, `CheatWindow`, `NetworkWindow` (not verbatim):

```cpp
class MyToolWindow : public DarwiniaWindow {
public:
    MyToolWindow() : DarwiniaWindow("my_window_key") {}       // unique name = lookup key = phrase key; takes focus now
    void Create() override {
        DarwiniaWindow::Create();                              // close box at (w-12, 2)
        int y = 5, pitch = 17;                                 // rows below the 15 px title bar
        auto *b = new MyActionButton();                        // DarwiniaButton subclass overriding MouseUp()
        b->SetShortProperties("my_action", 10, y += pitch, m_w - 20, -1, LANGUAGEPHRASE("my_action"));
        RegisterButton(b);                                     // window auto-grows to fit
        m_buttonOrder.PutData(b);                              // optional: controller navigation
        CreateValueControl("my_value", &m_someFloat, y += pitch, 0.1f, 0.0f, 10.0f, b /*callback*/);
    }
};
// opener (idempotent toggle, as every DebugKeyBindings entry does)
if (!EclGetWindow("my_window_key")) {
    auto *w = new MyToolWindow();
    w->SetMenuSize(220, 100);                                  // width/height before registration
    w->SetPosition(0, screenH - 100);
    EclRegisterWindow(w);                                      // front-most; Create() runs now
}
```

Rules recovered from the code [C]:
1. Choose a unique `m_name`; it is the lookup key, the phrase key for the (undrawn) title and the target of `EclRemoveWindow`. Use `EclGetWindow(name)` to make openers idempotent.
2. Set `m_w` (and optionally `m_h`, `m_x`, `m_y`) **before** `EclRegisterWindow`; height grows with content; position is clamped on-screen at registration; pass a parent window to get automatic placement.
3. Build all buttons in `Create()`; `Remove()` must undo them (the base removes all buttons) because resizing calls `Remove()` + `Create()`. Never keep raw pointers to buttons across a resize or rebuild.
4. Behaviour lives in tiny `DarwiniaButton` subclasses overriding `MouseUp` (click) or `MouseDown` (held). Reach the window through `m_parent` (cast). Check `m_disabled`/`m_inactive` yourself.
5. Bind data with `CreateValueControl` / `InputField::Register*` / `CheckBox::Register*` / `DropDownMenu::RegisterInt` — the widget writes straight into the target variable; pass a callback button to react (e.g. regenerate terrain).
6. Rebuild dynamic lists with `Remove(); Create();`.
7. Close via `EclRemoveWindow(m_name)` (or the close box); put teardown side effects in the destructor.

### 11.2 Recipe B — full-screen `GameOptionsWindow` page (options/help/in-game menu pattern)

1. Derive from `GameOptionsWindow("<title phrase id>")` — the base constructor already makes it full-screen, fixed and non-resizable. In the constructor copy every preference the page edits into members (`m_x = g_prefsManager->GetInt(key, default)`).
2. In `Create()`: call `GameOptionsWindow::Create()` (empty, but keeps the pattern); fetch `GetPosition_LeftBox`, `GetFontSizes`, `GetButtonSizes`; `x = leftX + (leftW − buttonW)/2`; add a `GameMenuTitleButton` `"title"` captioned `LANGUAGEPHRASE(m_name)` at `(leftX+10, leftY+10, leftW−20, 1.5 bH)`; for each option add a `GameMenuDropDown` (`m_cycleMode = true` for short lists, `RegisterInt(&member)`), `GameMenuCheckBox` (`RegisterInt`/`RegisterBool`) or `CreateMenuControl(...)` field, each `RegisterButton` + `m_buttonOrder.PutData`; rows at `y += rowH + gap` (options pages use `0.65 bH` rows and `fontSmall`).
3. Add an Apply `GameMenuButton` whose `Render` compares members with stored prefs each frame and sets `m_inactive` when nothing changed; its `MouseUp` returns when inactive, else writes prefs, `Save()`s and performs the minimum re-initialisation (screen: renderer restart + window rebuild; graphics: landscape + water re-init; other: listener re-open, language reload, full window rebuild) (P17b §11.4).
4. Add a `MenuCloseButton("dialog_back")` at `leftY + leftH − 2 bH` and assign it to `m_backButton` (pad B works because `m_closeable` stays true).
5. Set `m_renderRightBox` / `m_renderWholeScreen` if needed; report errors with `CreateErrorDialogue(text, success)`.
6. Open it from a `GameMenuButton` subclass: `GameMenuButton::MouseUp()` (sound + frame capture), `EclRegisterWindow(new MyPage())`, `g_app->m_doMenuTransition = true`.
7. Live readers query `g_prefsManager` directly at render/advance time (no change notification), so most options take effect next frame; anything cached at construction needs the explicit re-init in step 3.

### 11.3 Recipe C — a new `GameMenuWindow` page

1. Add a page constant to the enum in `code/UI/GameMenuWindow.h` and a string to `s_pageNames` in `code/game_menu.cpp` (the constructor release-asserts the counts match).
2. Write `SetupXPage()`: layout from `GetPosition_LeftBox/RightBox/TitleBar`, `GetFontSizes`, `GetButtonSizes`; a `GameMenuTitleButton` `"title"`; stack widgets with the five-step pattern (§6.5); a `BackPageButton(target)` — or `BackPageButton(m_currentPage)` for "return to caller", remembering that `m_currentPage` is still the previous page during set-up — at `leftY + leftH − 2 bH`; right-box panels (`ScrollingTextButton` subclasses, `ImageButton`s) **not** in `m_buttonOrder`.
3. Add the `case` to `SetupNewPage`; add an `UpdateXPage()` and a `case` in `Update` if the page polls anything (network echo, metaserver, timers); add a `ShutdownXPage()` and a `case` in `ShutdownPage` if it starts a service that must stop.
4. Add a `case` in `Render` if the page needs non-default fill boxes (default = left box only) or overlays.
5. Navigation into the page: a small `GameMenuButton` subclass whose `MouseUp` calls `GameMenuButton::MouseUp()` (sound + transition capture) and sets `((GameMenuWindow*)m_parent)->m_newPage = PageX`.
6. Put any state that must survive the rebuild in a `GameMenuWindow` member, a networked option (lobby settings shared with clients, §4.12) or a pref.
7. If the page needs data that arrives over the network, return early from set-up and re-call it from the page's update when the data arrives (pattern of MapSelect and GameSetup).

### 11.4 Guidance for re-implementing on the new engine (Windows, C++23, DirectX 12, multiplayer) [I — design recommendations]

**Keep** (proven, cheap): the screen-fraction layout grid (§7.1) as data; the "page = full-screen panel built from a declarative widget list" model; a single navigation cursor shared by mouse hover and pad, with hover-to-focus only in mouse mode; the requested-vs-echoed value split for lobby options (§4.12); input-mode-dependent captions (§10.2); the cascade/pop button entrance and page-swipe as presentation polish.

**Replace**:

| Original mechanism | Problem | Replacement |
|---|---|---|
| Name-string identity everywhere (§2.2, Q5–Q6) | duplicate names shadow, lookups are linear `strcmp` | typed widget handles/IDs; names only for debugging and localisation keys |
| Release-target click, held `MouseDown`, no capture (Q1–Q2) | accidental clicks, frame-rate-dependent repeats | pointer capture on press; click = press+release on the same widget; explicit repeat timers (initial delay + rate) independent of frame rate |
| Dirty rectangles that never clear (Q3) | leak, no benefit | full immediate-mode redraw (or retained draw lists) every frame |
| Rebuild-on-resize / rebuild-on-page-change with state in window members | state loss, ad-hoc persistence | view-model per page; widgets bind to it; layout recomputed without destroying state |
| `m_currentPage`-as-caller and `m_newPage` writes from widgets | implicit flow, render-time triggers | explicit page stack (`push`, `pop`, `replace`) with a one-frame deferred commit; Back = pop |
| `m_buttonOrder` + `SpecialScroll` on the wrong list (F5) | navigation bugs | focus-scope object per page with 2D (up/down/left/right) neighbours, scroll-into-view for lists |
| Pad "back" polled inside `Render` (§5.3) | back fails whenever the button is not drawn | input handled in an update phase through a page-level `OnBack` |
| `WM_CHAR`-only text input, no IME, no caret | CJK input, editing | TSF/IME-aware text service, caret and selection, clipboard (UTF-16), validation hooks (numeric ranges, max length) on commit **and** while typing |
| Fixed-cell bitmap fonts, BMP-only, approximate widths | wrong layout, no shaping | glyph cache (DirectWrite or FreeType+HarfBuzz) with measured widths; full Unicode |
| Key-as-fallback localisation | raw ids on screen | fall back to the source language per string; build-time check that every key exists in every shipped language |
| Fixed-function immediate draw, state leaks (§8.6) | not portable to DX12 | one batched UI vertex stream, three blend states, scissor stack (§8.7) |
| Error dialogue as a `m_lockMenu` flag | input leaks, pad soft-locks (F10) | modal layer on the page stack that owns focus and Back |
| Scattered string-keyed `g_prefsManager->GetInt(key, default)` calls with inconsistent call-site defaults, no change notification | options apply inconsistently; defaults drift | typed settings schema (key, type, default, range, apply scope = immediate / rebuild-renderer / restart) with change notifications (P17b §11.4) |

**Networking note**: the lobby widgets must keep sending *requests* and displaying *echoed* values; the server must validate ownership/host rights (the UI only hides controls — see [22](22_determinism_and_networking.md)).

---
## 12 Class diagram and complete UI class inventory

### 12.1 Core classes with members

```mermaid
classDiagram
  class EclWindow {
    +int m_x
    +int m_y
    +int m_w
    +int m_h
    +char m_name
    +UnicodeString m_title
    +bool m_movable
    +bool m_resizable
    +bool m_dirty
    +LList m_buttons
    +char m_currentTextEdit
    +Create()
    +Remove()
    +Update()
    +Render(hasFocus)
    +Keypress(keyCode, shift, ctrl, alt, ascii)
    +MouseEvent(lmb, rmb, up, down)
    +RegisterButton(button)
    +RemoveButton(name)
    +GetButton(name)
    +GetButton(x, y)
    +BeginTextEdit(name)
    +EndTextEdit()
    +MakeAllOnScreen()
  }
  class EclButton {
    +char m_name
    +int m_x
    +int m_y
    +int m_w
    +int m_h
    +UnicodeString m_caption
    +UnicodeString m_tooltip
    +bool m_inactive
    #EclWindow m_parent
    +SetProperties(name, x, y, w, h, caption, tooltip)
    +Render(realX, realY, highlighted, clicked)
    +MouseUp()
    +MouseDown()
    +MouseMove()
    +Keypress(keyCode, shift, ctrl, alt, ascii)
  }
  class DarwiniaWindow {
    +LList m_buttonOrder
    +int m_currentButton
    +bool m_buttonChangedThisUpdate
    +bool m_skipUpdate
    +bool m_closeable
    +bool m_lockMenu
    +EclButton m_backButton
    +CreateValueControl()
    +CreateColourControl()
    +RemoveValueControl(name)
    +GetMenuSize(value)
    +SetMenuSize(w, h)
    +SetCurrentButton(button)
  }
  class DarwiniaButton {
    +float m_fontSize
    +bool m_centered
    +bool m_disabled
    +bool m_mouseHighlightMode
    +SetShortProperties()
    +SetDisabled(flag)
    +UpdateButtonHighlight()
    +SpecialScroll(direction)
  }
  class GameOptionsWindow {
    +bool m_renderRightBox
    +bool m_renderWholeScreen
    +bool m_showingErrorDialogue
    +CreateMenuControl()
    +CreateErrorDialogue(text, success)
  }
  class GameMenuWindow {
    +int m_currentPage
    +int m_newPage
    +NetworkGameOptionInt m_gameType
    +NetworkGameOptionMapId m_requestedMapId
    +NetworkIntArray m_params
    +ScrollBar m_scrollBar
    +SetupNewPage(page)
    +ShutdownPage(page)
    +Update()
    +Render(hasFocus)
    +RenderBigDarwinian()
  }
  class GameMenuButton {
    +double m_birthTime
    +int m_transitionDirection
    +float m_ir
    +float m_ig
    +float m_ib
    +float m_ia
    +MouseUp()
    +Render()
  }
  class InputField {
    +UnicodeString m_buf
    +int m_type
    +NetworkValue m_value
    +DarwiniaButton m_callback
    +RegisterTyped()
    +Refresh()
    +Keypress()
  }
  class ScrollBar {
    +char m_parentWindow
    +int m_numRows
    +int m_winSize
    +int m_currentValue
    +Create(name, x, y, w, h, numRows, winSize, stepSize)
    +Remove()
    +SetCurrentValue(v)
    +ChangeCurrentValue(d)
  }
  EclWindow "1" o-- "*" EclButton : m_buttons
  EclWindow <|-- DarwiniaWindow
  EclButton <|-- DarwiniaButton
  DarwiniaWindow <|-- GameOptionsWindow
  DarwiniaWindow <|-- GameMenuWindow
  DarwiniaButton <|-- GameMenuButton
  DarwiniaButton <|-- BorderlessButton
  BorderlessButton <|-- InputField
  ScrollBar ..> EclButton : registers three buttons
  GameMenuWindow ..> ScrollBar : m_scrollBar
```

### 12.2 Master class diagram — every framework, widget, window and `code/UI` class

Generated from every `class X [: public Y]` declaration in the UI sources of §12.3 and their status (provenance: `_work/scripts/S_28_ui_classes.py` lists the declarations, `_work/scripts/S_28_ui_inventory.py` assigns the status) (all three layers; annotations: none = live in `[B:FULL]`; `<<tooluse>>` = compiled in FULL, used only by editor/debug tools; `<<legacy>>` = compiled in FULL but never reached; `<<editor>>` = `[B:EDITOR]`; `<<debug>>` = `[B:DEBUG]`; `<<othertarget>>` = other product target/platform; `<<dead>>` = not compiled in any PC configuration, or compiled only in DEBUG and never instantiated (`GrabberWindow`); `<<external>>` = non-UI base). Screen-private button classes (declared inside one screen's `.cpp`, e.g. the 29 sound-editor buttons) are omitted from the picture for legibility but listed in §12.3.

```mermaid
classDiagram
  direction LR
  class NetworkUnicodeString
  <<external>> NetworkUnicodeString
  class EclButton
  class DirtyRect
  class EclWindow
  class BuildingEditWindow
  <<editor>> BuildingEditWindow
  class BuildingsCreateWindow
  <<editor>> BuildingsCreateWindow
  class BuyNowWindow
  <<dead>> BuyNowWindow
  class CameraAnimMainEditWindow
  <<editor>> CameraAnimMainEditWindow
  class CameraAnimSecondaryEditWindow
  <<editor>> CameraAnimSecondaryEditWindow
  class CameraMountEditWindow
  <<editor>> CameraMountEditWindow
  class ChatInputWindow
  class ChatInputField
  class CheatWindow
  <<debug>> CheatWindow
  class CheckBox
  class ControllerUnpluggedWindow
  class DarwiniaWindow
  class DarwiniaButton
  class BorderlessButton
  class CloseButton
  class GameExitButton
  <<legacy>> GameExitButton
  class InvertedBox
  class LabelButton
  <<editor>> LabelButton
  class DebugMenu
  <<legacy>> DebugMenu
  class DemoEndWindow
  <<legacy>> DemoEndWindow
  class DropDownOptionData
  class DropDownWindow
  class DropDownMenu
  class DropDownMenuOption
  class MainEditWindow
  <<editor>> MainEditWindow
  class FileOKButton
  <<tooluse>> FileOKButton
  class FileButton
  <<tooluse>> FileButton
  class FileCancelButton
  <<tooluse>> FileCancelButton
  class SelectedButton
  <<tooluse>> SelectedButton
  class FileDialog
  <<tooluse>> FileDialog
  class GestureWindow
  <<dead>> GestureWindow
  class GlobalWorldEditorWindow
  <<legacy>> GlobalWorldEditorWindow
  class GrabberWindow
  <<dead>> GrabberWindow
  class HelpAndOptionsWindow
  class SelectHelpWindow
  class CrateButton
  <<debug>> CrateButton
  class CrateHelpWindow
  <<debug>> CrateHelpWindow
  class GeneralHelpWindow
  class TextButton
  class GameTypeHelpWindow
  class InputField
  class InputScroller
  class ColourWidget
  <<tooluse>> ColourWidget
  class ColourWindow
  <<tooluse>> ColourWindow
  class InstantUnitEditWindow
  <<editor>> InstantUnitEditWindow
  class InstantUnitCreateWindow
  <<editor>> InstantUnitCreateWindow
  class InvalidCratesWindow
  <<editor>> InvalidCratesWindow
  class TerrainSelectorFileDialog
  <<editor>> TerrainSelectorFileDialog
  class WaterSelectorFileDialog
  <<editor>> WaterSelectorFileDialog
  class WavesSelectorFileDialog
  <<editor>> WavesSelectorFileDialog
  class LandscapeTileEditWindow
  <<editor>> LandscapeTileEditWindow
  class LandscapeFlattenAreaEditWindow
  <<editor>> LandscapeFlattenAreaEditWindow
  class LandscapeEditWindow
  <<editor>> LandscapeEditWindow
  class LandscapeGuideGridWindow
  <<editor>> LandscapeGuideGridWindow
  class LeaderboardWindow
  <<legacy>> LeaderboardWindow
  class LeaderboardSelectionWindow
  <<legacy>> LeaderboardSelectionWindow
  class LightsEditWindow
  <<editor>> LightsEditWindow
  class GameOptionsWindow
  class MainMenuWindow
  <<legacy>> MainMenuWindow
  class OptionsMenuWindow
  class LocationWindow
  class ResetLocationWindow
  <<legacy>> ResetLocationWindow
  class AboutDarwiniaWindow
  <<legacy>> AboutDarwiniaWindow
  class ConfirmExitWindow
  class ConfirmResetWindow
  <<legacy>> ConfirmResetWindow
  class MenuCloseButton
  class MenuGameExitButton
  <<legacy>> MenuGameExitButton
  class HelpAndOptionsButton
  class AchievementsButton
  <<legacy>> AchievementsButton
  class LeaderBoardButton
  <<legacy>> LeaderBoardButton
  class ConfirmResetButton
  <<legacy>> ConfirmResetButton
  class OKButton
  class MessageDialog
  class ModsWindow
  <<dead>> ModsWindow
  class NewModWindow
  <<dead>> NewModWindow
  class MultiwiniaFileOKButton
  <<editor>> MultiwiniaFileOKButton
  class MultiwiniaFileButton
  <<editor>> MultiwiniaFileButton
  class MultiwiniaFileDialog
  <<editor>> MultiwiniaFileDialog
  class MultiwiniaWindow
  <<editor>> MultiwiniaWindow
  class NewMapWindow
  <<editor>> NewMapWindow
  class MultiwiniaOptionsWindow
  <<editor>> MultiwiniaOptionsWindow
  class NetworkWindow
  class PrefsGraphicsWindow
  class PrefsKeybindingsWindow
  <<legacy>> PrefsKeybindingsWindow
  class PrefsOtherWindow
  class PrefsScreenWindow
  class PrefsSoundWindow
  class ProfileWindow
  <<debug>> ProfileWindow
  class ReallyQuitWindow
  <<legacy>> ReallyQuitWindow
  class SaveOnQuitWindow
  <<legacy>> SaveOnQuitWindow
  class ScrollBar
  class ScrollBarButton
  class ScrollChangeButton
  class SoundProfileWindow
  <<debug>> SoundProfileWindow
  class SoundSelector
  <<debug>> SoundSelector
  class SampleGroupFileDialog
  <<debug>> SampleGroupFileDialog
  class SoundEditorWindow
  <<debug>> SoundEditorWindow
  class DspEffectEditor
  <<debug>> DspEffectEditor
  class SampleGroupEditor
  <<debug>> SampleGroupEditor
  class RenameSampleGroupWindow
  <<debug>> RenameSampleGroupWindow
  class PurgeSoundsWindow
  <<debug>> PurgeSoundsWindow
  class SoundParameterEditor
  <<debug>> SoundParameterEditor
  class SoundStatsWindow
  <<debug>> SoundStatsWindow
  class TreeWindow
  <<editor>> TreeWindow
  class UpdateAvailableWindow
  <<dead>> UpdateAvailableWindow
  class UserProfileWindow
  <<legacy>> UserProfileWindow
  class NewUserProfileWindow
  <<legacy>> NewUserProfileWindow
  class AchievementButton
  <<legacy>> AchievementButton
  class AdvancedOptionsModeButton
  class AnyLevelSelectButton
  <<legacy>> AnyLevelSelectButton
  class ApplyNameButton
  class AuthInputField
  class AuthPageButton
  class AuthStatusButton
  class BuyNowInfoButton
  <<legacy>> BuyNowInfoButton
  class BuyNowModeButton
  class CSpectatorButton
  <<dead>> CSpectatorButton
  class CampaignButton
  <<debug>> CampaignButton
  class ServerConnectButton
  class DarwiniaModeButton
  class DarwiniaPlusMPButton
  <<legacy>> DarwiniaPlusMPButton
  class DarwiniaPlusSPButton
  <<legacy>> DarwiniaPlusSPButton
  class EnterAuthkeyButton
  class ErrorDialog
  <<dead>> ErrorDialog
  class FancyColourButton
  class FiltersPageButton
  class FinishMultiwiniaButton
  class FinishSinglePlayerButton
  class SaveFiltersButton
  class GameMenuOptionsButton
  <<dead>> GameMenuOptionsButton
  class GameMenuChatButton
  class ServerPasswordString
  class GameMenuWindow
  class GameServerButton
  class GameTypeAnyButton
  <<legacy>> GameTypeAnyButton
  class GameTypeButton
  class GameTypeImageButton
  class GameTypeInfoButton
  class GamertagButton
  class HelpImageButton
  <<legacy>> HelpImageButton
  class HelpModeButton
  <<dead>> HelpModeButton
  class HelpSelectButton
  <<legacy>> HelpSelectButton
  class ImageButton
  class JoinGameButton
  class KickPlayerButton
  class LevelSelectButton
  class LevelSelectButtonBase
  class MapData
  class MapImageButton
  class MapInfoButton
  class MapListTitlesButton
  class MapSelectModeButton
  class MultiwiniaEditorButton
  <<editor>> MultiwiniaEditorButton
  class MultiwiniaModeButton
  <<dead>> MultiwiniaModeButton
  class MultiwiniaVersusCpuButton
  <<debug>> MultiwiniaVersusCpuButton
  class NewOrJoinButton
  class NewServerButton
  class NextHelpButton
  <<legacy>> NextHelpButton
  class OptionDetailsButton
  class PasteKeyButton
  class PlayDemoAuthkeyButton
  class PlayGameButton
  class PrevHelpButton
  <<legacy>> PrevHelpButton
  class PrologueButton
  <<debug>> PrologueButton
  class QuickMatchButton
  <<legacy>> QuickMatchButton
  class QuickStartButton
  <<dead>> QuickStartButton
  class QuitAuthkeyButton
  <<othertarget>> QuitAuthkeyButton
  class QuitButton
  class RequestAIButton
  <<dead>> RequestAIButton
  class ResearchModeButton
  <<dead>> ResearchModeButton
  class RollingDemoButton
  <<dead>> RollingDemoButton
  class ScrollingTextButton
  class ServerTitleInfoButton
  class StartTestbedButton
  <<dead>> StartTestbedButton
  class TutorialPageButton
  class TutorialButton
  class UnlockFullGameButton
  class WaitingButton
  class CreditsButton
  class MapDetailsButton
  <<dead>> MapDetailsButton
  class MOTDButton
  class VolumeControl
  <<dead>> VolumeControl
  class UpdatePageButton
  class RunAutoPatcherButton
  class RunSparkleButton
  <<othertarget>> RunSparkleButton
  class VisitWebsiteButton
  class PlayerNameString
  class GameMenu
  class GameMenuTitleButton
  class GameMenuButton
  class GameMenuDropDown
  class GameMenuDropDownWindow
  class GameMenuDropDownMenuOption
  class GameMenuInputField
  class GameMenuInputScroller
  class BackPageButton
  class GameMenuCheckBox
  DarwiniaWindow <|-- BuildingEditWindow
  DarwiniaWindow <|-- BuildingsCreateWindow
  DarwiniaWindow <|-- BuyNowWindow
  DarwiniaWindow <|-- CameraAnimMainEditWindow
  DarwiniaWindow <|-- CameraAnimSecondaryEditWindow
  DarwiniaWindow <|-- CameraMountEditWindow
  DarwiniaWindow <|-- ChatInputWindow
  InputField <|-- ChatInputField
  DarwiniaWindow <|-- CheatWindow
  DarwiniaButton <|-- CheckBox
  GameOptionsWindow <|-- ControllerUnpluggedWindow
  EclWindow <|-- DarwiniaWindow
  EclButton <|-- DarwiniaButton
  DarwiniaButton <|-- BorderlessButton
  DarwiniaButton <|-- CloseButton
  DarwiniaButton <|-- GameExitButton
  DarwiniaButton <|-- InvertedBox
  DarwiniaButton <|-- LabelButton
  DarwiniaWindow <|-- DebugMenu
  DarwiniaWindow <|-- DemoEndWindow
  DarwiniaWindow <|-- DropDownWindow
  DarwiniaButton <|-- DropDownMenu
  BorderlessButton <|-- DropDownMenuOption
  DarwiniaWindow <|-- MainEditWindow
  DarwiniaButton <|-- FileOKButton
  EclButton <|-- FileButton
  DarwiniaButton <|-- FileCancelButton
  DarwiniaButton <|-- SelectedButton
  DarwiniaWindow <|-- FileDialog
  DarwiniaWindow <|-- GestureWindow
  DarwiniaWindow <|-- GlobalWorldEditorWindow
  DarwiniaWindow <|-- GrabberWindow
  GameOptionsWindow <|-- HelpAndOptionsWindow
  GameOptionsWindow <|-- SelectHelpWindow
  DarwiniaButton <|-- CrateButton
  GameOptionsWindow <|-- CrateHelpWindow
  GameOptionsWindow <|-- GeneralHelpWindow
  DarwiniaButton <|-- TextButton
  GameOptionsWindow <|-- GameTypeHelpWindow
  BorderlessButton <|-- InputField
  DarwiniaButton <|-- InputScroller
  DarwiniaButton <|-- ColourWidget
  DarwiniaWindow <|-- ColourWindow
  DarwiniaWindow <|-- InstantUnitEditWindow
  DarwiniaWindow <|-- InstantUnitCreateWindow
  DarwiniaWindow <|-- InvalidCratesWindow
  FileDialog <|-- TerrainSelectorFileDialog
  FileDialog <|-- WaterSelectorFileDialog
  FileDialog <|-- WavesSelectorFileDialog
  DarwiniaWindow <|-- LandscapeTileEditWindow
  DarwiniaWindow <|-- LandscapeFlattenAreaEditWindow
  DarwiniaWindow <|-- LandscapeEditWindow
  DarwiniaWindow <|-- LandscapeGuideGridWindow
  GameOptionsWindow <|-- LeaderboardWindow
  GameOptionsWindow <|-- LeaderboardSelectionWindow
  DarwiniaWindow <|-- LightsEditWindow
  DarwiniaWindow <|-- GameOptionsWindow
  GameOptionsWindow <|-- MainMenuWindow
  GameOptionsWindow <|-- OptionsMenuWindow
  GameOptionsWindow <|-- LocationWindow
  GameOptionsWindow <|-- ResetLocationWindow
  DarwiniaWindow <|-- AboutDarwiniaWindow
  GameOptionsWindow <|-- ConfirmExitWindow
  GameOptionsWindow <|-- ConfirmResetWindow
  GameMenuButton <|-- MenuCloseButton
  GameMenuButton <|-- MenuGameExitButton
  GameMenuButton <|-- HelpAndOptionsButton
  GameMenuButton <|-- AchievementsButton
  GameMenuButton <|-- LeaderBoardButton
  GameMenuButton <|-- ConfirmResetButton
  DarwiniaButton <|-- OKButton
  DarwiniaWindow <|-- MessageDialog
  DarwiniaWindow <|-- ModsWindow
  DarwiniaWindow <|-- NewModWindow
  GameMenuButton <|-- MultiwiniaFileOKButton
  DarwiniaButton <|-- MultiwiniaFileButton
  GameOptionsWindow <|-- MultiwiniaFileDialog
  GameOptionsWindow <|-- MultiwiniaWindow
  GameOptionsWindow <|-- NewMapWindow
  DarwiniaWindow <|-- MultiwiniaOptionsWindow
  DarwiniaWindow <|-- NetworkWindow
  GameOptionsWindow <|-- PrefsGraphicsWindow
  DarwiniaWindow <|-- PrefsKeybindingsWindow
  GameOptionsWindow <|-- PrefsOtherWindow
  GameOptionsWindow <|-- PrefsScreenWindow
  GameOptionsWindow <|-- PrefsSoundWindow
  DarwiniaWindow <|-- ProfileWindow
  DarwiniaWindow <|-- ReallyQuitWindow
  DarwiniaWindow <|-- SaveOnQuitWindow
  EclButton <|-- ScrollBarButton
  DarwiniaButton <|-- ScrollChangeButton
  DarwiniaWindow <|-- SoundProfileWindow
  FileDialog <|-- SoundSelector
  FileDialog <|-- SampleGroupFileDialog
  DarwiniaWindow <|-- SoundEditorWindow
  DarwiniaWindow <|-- DspEffectEditor
  DarwiniaWindow <|-- SampleGroupEditor
  DarwiniaWindow <|-- RenameSampleGroupWindow
  DarwiniaWindow <|-- PurgeSoundsWindow
  DarwiniaWindow <|-- SoundParameterEditor
  DarwiniaWindow <|-- SoundStatsWindow
  DarwiniaWindow <|-- TreeWindow
  MessageDialog <|-- UpdateAvailableWindow
  GameOptionsWindow <|-- UserProfileWindow
  DarwiniaWindow <|-- NewUserProfileWindow
  GameMenuButton <|-- AchievementButton
  GameMenuButton <|-- AdvancedOptionsModeButton
  LevelSelectButtonBase <|-- AnyLevelSelectButton
  BackPageButton <|-- ApplyNameButton
  InputField <|-- AuthInputField
  GameMenuButton <|-- AuthPageButton
  GameMenuButton <|-- AuthStatusButton
  DarwiniaButton <|-- BuyNowInfoButton
  GameMenuButton <|-- BuyNowModeButton
  GameMenuButton <|-- CSpectatorButton
  GameMenuButton <|-- CampaignButton
  GameMenuButton <|-- ServerConnectButton
  GameMenuButton <|-- DarwiniaModeButton
  GameMenuButton <|-- DarwiniaPlusMPButton
  GameMenuButton <|-- DarwiniaPlusSPButton
  GameMenuButton <|-- EnterAuthkeyButton
  DarwiniaWindow <|-- ErrorDialog
  GameMenuButton <|-- FancyColourButton
  GameMenuButton <|-- FiltersPageButton
  BackPageButton <|-- FinishMultiwiniaButton
  BackPageButton <|-- FinishSinglePlayerButton
  BackPageButton <|-- SaveFiltersButton
  GameMenuButton <|-- GameMenuOptionsButton
  DarwiniaButton <|-- GameMenuChatButton
  NetworkUnicodeString <|-- ServerPasswordString
  DarwiniaWindow <|-- GameMenuWindow
  GameMenuButton <|-- GameServerButton
  GameMenuButton <|-- GameTypeAnyButton
  GameMenuButton <|-- GameTypeButton
  DarwiniaButton <|-- GameTypeImageButton
  ScrollingTextButton <|-- GameTypeInfoButton
  GameMenuButton <|-- GamertagButton
  DarwiniaButton <|-- HelpImageButton
  GameMenuButton <|-- HelpModeButton
  GameMenuButton <|-- HelpSelectButton
  DarwiniaButton <|-- ImageButton
  GameMenuButton <|-- JoinGameButton
  DarwiniaButton <|-- KickPlayerButton
  LevelSelectButtonBase <|-- LevelSelectButton
  DarwiniaButton <|-- LevelSelectButtonBase
  DarwiniaButton <|-- MapImageButton
  ScrollingTextButton <|-- MapInfoButton
  DarwiniaButton <|-- MapListTitlesButton
  GameMenuButton <|-- MapSelectModeButton
  GameMenuButton <|-- MultiwiniaEditorButton
  GameMenuButton <|-- MultiwiniaModeButton
  GameMenuButton <|-- MultiwiniaVersusCpuButton
  GameMenuButton <|-- NewOrJoinButton
  GameMenuButton <|-- NewServerButton
  GameMenuButton <|-- NextHelpButton
  ScrollingTextButton <|-- OptionDetailsButton
  GameMenuButton <|-- PasteKeyButton
  GameMenuButton <|-- PlayDemoAuthkeyButton
  GameMenuButton <|-- PlayGameButton
  GameMenuButton <|-- PrevHelpButton
  GameMenuButton <|-- PrologueButton
  GameMenuButton <|-- QuickMatchButton
  GameMenuButton <|-- QuickStartButton
  GameMenuButton <|-- QuitAuthkeyButton
  GameMenuButton <|-- QuitButton
  GameMenuButton <|-- RequestAIButton
  GameMenuButton <|-- ResearchModeButton
  GameMenuButton <|-- RollingDemoButton
  DarwiniaButton <|-- ScrollingTextButton
  DarwiniaButton <|-- ServerTitleInfoButton
  GameMenuButton <|-- StartTestbedButton
  GameMenuButton <|-- TutorialPageButton
  GameMenuButton <|-- TutorialButton
  GameMenuButton <|-- UnlockFullGameButton
  DarwiniaButton <|-- WaitingButton
  GameMenuButton <|-- CreditsButton
  DarwiniaButton <|-- MapDetailsButton
  ScrollingTextButton <|-- MOTDButton
  DarwiniaButton <|-- VolumeControl
  GameMenuButton <|-- UpdatePageButton
  GameMenuButton <|-- RunAutoPatcherButton
  GameMenuButton <|-- RunSparkleButton
  GameMenuButton <|-- VisitWebsiteButton
  NetworkUnicodeString <|-- PlayerNameString
  DarwiniaButton <|-- GameMenuTitleButton
  DarwiniaButton <|-- GameMenuButton
  DropDownMenu <|-- GameMenuDropDown
  DropDownWindow <|-- GameMenuDropDownWindow
  DropDownMenuOption <|-- GameMenuDropDownMenuOption
  InputField <|-- GameMenuInputField
  InputScroller <|-- GameMenuInputScroller
  GameMenuButton <|-- BackPageButton
  CheckBox <|-- GameMenuCheckBox
  EclWindow "1" o-- "*" EclButton : m_buttons
  ScrollBar ..> ScrollBarButton : registers in parent
  ScrollBar ..> ScrollChangeButton : registers in parent
  DropDownMenu ..> DropDownWindow : opens singleton
  GameMenuDropDown ..> GameMenuDropDownWindow : opens singleton
  InputScroller ..> InputField : m_inputField
  GameMenu ..> GameMenuWindow : creates and destroys
  GameMenuWindow ..> ScrollBar : m_scrollBar
  GameMenuWindow ..> MapData : via GameMenu m_maps
```

### 12.3 Complete inventory of UI class declarations (generated)

Every `class X [: public Y]` declaration in `contrib/eclipse/*`, `code/interface/*`, `code/UI/*`, `code/game_menu.*` (384 declarations; `ClearLinksButton` is declared twice in `code/interface/buildings_window.cpp`, once per `#if` branch). Status rules: file-level reachability (vcproj membership, `#ifdef` guards, instantiation sites found by grep of `new <Class>`) with per-class overrides (provenance: `_work/scripts/S_28_ui_inventory.py`, which holds the status of every file and override). "[B:FULL]*" = compiled into the shipped exe but only instantiated by editor/debug tool windows.

| # | Class | Base | Declared in | Status | Tag |
|---|---|---|---|---|---|
| 1 | `EclButton` | — | `contrib/eclipse/eclbutton.h:EclButton` | live in the shipped build | [B:FULL] |
| 2 | `DirtyRect` | — | `contrib/eclipse/eclipse.h:DirtyRect` | live in the shipped build | [B:FULL] |
| 3 | `EclWindow` | — | `contrib/eclipse/eclwindow.h:EclWindow` | live in the shipped build | [B:FULL] |
| 4 | `ToolButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:ToolButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 5 | `DeleteBuildingButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:DeleteBuildingButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 6 | `TeamButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:TeamButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 7 | `IsGlobalButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:IsGlobalButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 8 | `CloneBuildingButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:CloneBuildingButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 9 | `ClearLinksButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:ClearLinksButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 10 | `ClearRadarLinksButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:ClearRadarLinksButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 11 | `ClearLinksButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:ClearLinksButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 12 | `NewBuildingButton` | `DarwiniaButton` | `code/interface/buildings_window.cpp:NewBuildingButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 13 | `BuildingEditWindow` | `DarwiniaWindow` | `code/interface/buildings_window.h:BuildingEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 14 | `BuildingsCreateWindow` | `DarwiniaWindow` | `code/interface/buildings_window.h:BuildingsCreateWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 15 | `BuyNowButton` | `DarwiniaButton` | `code/interface/buynow_window.cpp:BuyNowButton` | not compiled in any PC configuration | [B:DEAD] |
| 16 | `BuyNowWindow` | `DarwiniaWindow` | `code/interface/buynow_window.h:BuyNowWindow` | not compiled in any PC configuration | [B:DEAD] |
| 17 | `NewAnimButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:NewAnimButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 18 | `DeleteAnimButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:DeleteAnimButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 19 | `SelectAnimButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:SelectAnimButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 20 | `NewNodeButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:NewNodeButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 21 | `CamBeforeMountButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:CamBeforeMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 22 | `StartPreviewButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:StartPreviewButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 23 | `StopPreviewButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:StopPreviewButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 24 | `SelectMountButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:SelectMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 25 | `DeleteNodeButton` | `DarwiniaButton` | `code/interface/camera_anim_window.cpp:DeleteNodeButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 26 | `CameraAnimMainEditWindow` | `DarwiniaWindow` | `code/interface/camera_anim_window.h:CameraAnimMainEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 27 | `CameraAnimSecondaryEditWindow` | `DarwiniaWindow` | `code/interface/camera_anim_window.h:CameraAnimSecondaryEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 28 | `NewMountButton` | `DarwiniaButton` | `code/interface/camera_mount_window.cpp:NewMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 29 | `GotoMountButton` | `DarwiniaButton` | `code/interface/camera_mount_window.cpp:GotoMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 30 | `DeleteMountButton` | `DarwiniaButton` | `code/interface/camera_mount_window.cpp:DeleteMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 31 | `UpdateMountButton` | `DarwiniaButton` | `code/interface/camera_mount_window.cpp:UpdateMountButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 32 | `CameraMountEditWindow` | `DarwiniaWindow` | `code/interface/camera_mount_window.h:CameraMountEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 33 | `ChatInputWindow` | `DarwiniaWindow` | `code/interface/chatinput_window.h:ChatInputWindow` | live in the shipped build | [B:FULL] |
| 34 | `ChatInputField` | `InputField` | `code/interface/chatinput_window.h:ChatInputField` | live in the shipped build | [B:FULL] |
| 35 | `KillAllEnemiesButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:KillAllEnemiesButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 36 | `SpawnDarwiniansButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SpawnDarwiniansButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 37 | `SpawnTankButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SpawnTankButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 38 | `SpawnViriiButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SpawnViriiButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 39 | `SpawnSpiritButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SpawnSpiritButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 40 | `AllowArbitraryPlacementButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:AllowArbitraryPlacementButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 41 | `ShowDrSepulvedaFaceButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:ShowDrSepulvedaFaceButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 42 | `EnableGeneratorAndMineButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:EnableGeneratorAndMineButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 43 | `EnableReceiverAndBufferButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:EnableReceiverAndBufferButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 44 | `OpenAllLocationsButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:OpenAllLocationsButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 45 | `ClearResourcesButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:ClearResourcesButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 46 | `GiveAllResearchButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:GiveAllResearchButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 47 | `SpawnPortsButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SpawnPortsButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 48 | `ProfilerCreateButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:ProfilerCreateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 49 | `SoundEditorCreateButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SoundEditorCreateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 50 | `SoundStatsCreateButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:SoundStatsCreateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 51 | `NetworkStatsCreateButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:NetworkStatsCreateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 52 | `CrateCreateButton` | `DarwiniaButton` | `code/interface/cheat_window.cpp:CrateCreateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 53 | `CheatWindow` | `DarwiniaWindow` | `code/interface/cheat_window.h:CheatWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 54 | `CheckBox` | `DarwiniaButton` | `code/interface/checkbox.h:CheckBox` | live in the shipped build | [B:FULL] |
| 55 | `ControllerUnpluggedWindow` | `GameOptionsWindow` | `code/interface/controllerunplugged_window.h:ControllerUnpluggedWindow` | live in the shipped build | [B:FULL] |
| 56 | `DarwiniaWindow` | `EclWindow` | `code/interface/darwinia_window.h:DarwiniaWindow` | live in the shipped build | [B:FULL] |
| 57 | `DarwiniaButton` | `EclButton` | `code/interface/darwinia_window.h:DarwiniaButton` | live in the shipped build | [B:FULL] |
| 58 | `BorderlessButton` | `DarwiniaButton` | `code/interface/darwinia_window.h:BorderlessButton` | live in the shipped build | [B:FULL] |
| 59 | `CloseButton` | `DarwiniaButton` | `code/interface/darwinia_window.h:CloseButton` | live in the shipped build | [B:FULL] |
| 60 | `GameExitButton` | `DarwiniaButton` | `code/interface/darwinia_window.h:GameExitButton` | compiled into FULL, never reached | [B:LEGACY] |
| 61 | `InvertedBox` | `DarwiniaButton` | `code/interface/darwinia_window.h:InvertedBox` | live in the shipped build | [B:FULL] |
| 62 | `LabelButton` | `DarwiniaButton` | `code/interface/darwinia_window.h:LabelButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 63 | `ProfileButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:ProfileButton` | compiled into FULL, never reached | [B:LEGACY] |
| 64 | `NetworkButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:NetworkButton` | compiled into FULL, never reached | [B:LEGACY] |
| 65 | `EditorButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:EditorButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 66 | `DebugCameraButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:DebugCameraButton` | compiled into FULL, never reached | [B:LEGACY] |
| 67 | `FPSButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:FPSButton` | compiled into FULL, never reached | [B:LEGACY] |
| 68 | `PrefsScreenButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:PrefsScreenButton` | compiled into FULL, never reached | [B:LEGACY] |
| 69 | `PrefsGfxDetailButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:PrefsGfxDetailButton` | compiled into FULL, never reached | [B:LEGACY] |
| 70 | `PrefsSoundButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:PrefsSoundButton` | compiled into FULL, never reached | [B:LEGACY] |
| 71 | `PokeyEditorButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:PokeyEditorButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 72 | `GestureEditorButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:GestureEditorButton` | not compiled in any PC configuration | [B:DEAD] |
| 73 | `CheatButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:CheatButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 74 | `SoundStatsButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:SoundStatsButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 75 | `SoundProfileButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:SoundProfileButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 76 | `SoundEditorButton` | `DarwiniaButton` | `code/interface/debugmenu.cpp:SoundEditorButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 77 | `DebugMenu` | `DarwiniaWindow` | `code/interface/debugmenu.h:DebugMenu` | compiled into FULL, never reached | [B:LEGACY] |
| 78 | `DebugKeyBindings` | — | `code/interface/debugmenu.h:DebugKeyBindings` | live in the shipped build | [B:FULL] |
| 79 | `BuyOnlineButton` | `DarwiniaButton` | `code/interface/demoend_window.cpp:BuyOnlineButton` | compiled into FULL, never reached | [B:LEGACY] |
| 80 | `ExitDemoButton` | `DarwiniaButton` | `code/interface/demoend_window.cpp:ExitDemoButton` | compiled into FULL, never reached | [B:LEGACY] |
| 81 | `ReturnToGameButton` | `DarwiniaButton` | `code/interface/demoend_window.cpp:ReturnToGameButton` | compiled into FULL, never reached | [B:LEGACY] |
| 82 | `DemoEndWindow` | `DarwiniaWindow` | `code/interface/demoend_window.h:DemoEndWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 83 | `DropDownOptionData` | — | `code/interface/drop_down_menu.h:DropDownOptionData` | live in the shipped build | [B:FULL] |
| 84 | `DropDownWindow` | `DarwiniaWindow` | `code/interface/drop_down_menu.h:DropDownWindow` | live in the shipped build | [B:FULL] |
| 85 | `DropDownMenu` | `DarwiniaButton` | `code/interface/drop_down_menu.h:DropDownMenu` | live in the shipped build | [B:FULL] |
| 86 | `DropDownMenuOption` | `BorderlessButton` | `code/interface/drop_down_menu.h:DropDownMenuOption` | live in the shipped build | [B:FULL] |
| 87 | `MainEditWindowButton` | `BorderlessButton` | `code/interface/editor_window.cpp:MainEditWindowButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 88 | `MainEditWindow` | `DarwiniaWindow` | `code/interface/editor_window.h:MainEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 89 | `FileOKButton` | `DarwiniaButton` | `code/interface/filedialog.cpp:FileOKButton` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 90 | `FileButton` | `EclButton` | `code/interface/filedialog.cpp:FileButton` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 91 | `FileCancelButton` | `DarwiniaButton` | `code/interface/filedialog.cpp:FileCancelButton` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 92 | `SelectedButton` | `DarwiniaButton` | `code/interface/filedialog.cpp:SelectedButton` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 93 | `FileDialog` | `DarwiniaWindow` | `code/interface/filedialog.h:FileDialog` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 94 | `DrawGestureBox` | `DarwiniaButton` | `code/interface/gesture_window.cpp:DrawGestureBox` | not compiled in any PC configuration | [B:DEAD] |
| 95 | `GestureStatusButton` | `DarwiniaButton` | `code/interface/gesture_window.cpp:GestureStatusButton` | not compiled in any PC configuration | [B:DEAD] |
| 96 | `SaveDataButton` | `DarwiniaButton` | `code/interface/gesture_window.cpp:SaveDataButton` | not compiled in any PC configuration | [B:DEAD] |
| 97 | `LoadDataButton` | `DarwiniaButton` | `code/interface/gesture_window.cpp:LoadDataButton` | not compiled in any PC configuration | [B:DEAD] |
| 98 | `NewGestureButton` | `DarwiniaButton` | `code/interface/gesture_window.cpp:NewGestureButton` | not compiled in any PC configuration | [B:DEAD] |
| 99 | `RecordGestureDemoButton` | `DarwiniaButton` | `code/interface/gesture_window.cpp:RecordGestureDemoButton` | not compiled in any PC configuration | [B:DEAD] |
| 100 | `GestureWindow` | `DarwiniaWindow` | `code/interface/gesture_window.h:GestureWindow` | not compiled in any PC configuration | [B:DEAD] |
| 101 | `SetModeButton` | `DarwiniaButton` | `code/interface/globalworldeditor_window.cpp:SetModeButton` | compiled into FULL, never reached | [B:LEGACY] |
| 102 | `NewLocationButton` | `DarwiniaButton` | `code/interface/globalworldeditor_window.cpp:NewLocationButton` | compiled into FULL, never reached | [B:LEGACY] |
| 103 | `SaveLocationsButton` | `DarwiniaButton` | `code/interface/globalworldeditor_window.cpp:SaveLocationsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 104 | `GlobalWorldEditorWindow` | `DarwiniaWindow` | `code/interface/globalworldeditor_window.h:GlobalWorldEditorWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 105 | `StartRecordButton` | `DarwiniaButton` | `code/interface/grabber_window.cpp:StartRecordButton` | compiled only in TARGET_DEBUG builds, never instantiated (no caller) | [B:DEAD] |
| 106 | `StopRecordButton` | `DarwiniaButton` | `code/interface/grabber_window.cpp:StopRecordButton` | compiled only in TARGET_DEBUG builds, never instantiated (no caller) | [B:DEAD] |
| 107 | `GrabberWindow` | `DarwiniaWindow` | `code/interface/grabber_window.h:GrabberWindow` | compiled only in TARGET_DEBUG builds, never instantiated (no caller) | [B:DEAD] |
| 108 | `HelpWindowButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:HelpWindowButton` | live in the shipped build | [B:FULL] |
| 109 | `CrateHelpWindowButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:CrateHelpWindowButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 110 | `OptionsButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:OptionsButton` | live in the shipped build | [B:FULL] |
| 111 | `HelpTypeButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:HelpTypeButton` | live in the shipped build | [B:FULL] |
| 112 | `GeneralNextPageButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:GeneralNextPageButton` | live in the shipped build | [B:FULL] |
| 113 | `NextPageButton` | `GameMenuButton` | `code/interface/helpandoptions_windows.cpp:NextPageButton` | live in the shipped build | [B:FULL] |
| 114 | `HelpAndOptionsWindow` | `GameOptionsWindow` | `code/interface/helpandoptions_windows.h:HelpAndOptionsWindow` | live in the shipped build | [B:FULL] |
| 115 | `SelectHelpWindow` | `GameOptionsWindow` | `code/interface/helpandoptions_windows.h:SelectHelpWindow` | live in the shipped build | [B:FULL] |
| 116 | `CrateButton` | `DarwiniaButton` | `code/interface/helpandoptions_windows.h:CrateButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 117 | `CrateHelpWindow` | `GameOptionsWindow` | `code/interface/helpandoptions_windows.h:CrateHelpWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 118 | `GeneralHelpWindow` | `GameOptionsWindow` | `code/interface/helpandoptions_windows.h:GeneralHelpWindow` | live in the shipped build | [B:FULL] |
| 119 | `TextButton` | `DarwiniaButton` | `code/interface/helpandoptions_windows.h:TextButton` | live in the shipped build | [B:FULL] |
| 120 | `GameTypeHelpWindow` | `GameOptionsWindow` | `code/interface/helpandoptions_windows.h:GameTypeHelpWindow` | live in the shipped build | [B:FULL] |
| 121 | `InputField` | `BorderlessButton` | `code/interface/input_field.h:InputField` | live in the shipped build | [B:FULL] |
| 122 | `InputScroller` | `DarwiniaButton` | `code/interface/input_field.h:InputScroller` | live in the shipped build | [B:FULL] |
| 123 | `ColourWidget` | `DarwiniaButton` | `code/interface/input_field.h:ColourWidget` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 124 | `ColourWindow` | `DarwiniaWindow` | `code/interface/input_field.h:ColourWindow` | compiled into FULL, only used by editor/debug tools | [B:FULL]* |
| 125 | `EditButton` | `DarwiniaButton` | `code/interface/instant_unit_window.cpp:EditButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 126 | `TeamButton1` | `DarwiniaButton` | `code/interface/instant_unit_window.cpp:TeamButton1` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 127 | `DeleteInstantUnitButton` | `DarwiniaButton` | `code/interface/instant_unit_window.cpp:DeleteInstantUnitButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 128 | `CloneInstantUnitButton` | `DarwiniaButton` | `code/interface/instant_unit_window.cpp:CloneInstantUnitButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 129 | `CreateButton` | `DarwiniaButton` | `code/interface/instant_unit_window.cpp:CreateButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 130 | `InstantUnitEditWindow` | `DarwiniaWindow` | `code/interface/instant_unit_window.h:InstantUnitEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 131 | `InstantUnitCreateWindow` | `DarwiniaWindow` | `code/interface/instant_unit_window.h:InstantUnitCreateWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 132 | `InvalidCratesWindow` | `DarwiniaWindow` | `code/interface/invalidcrates_window.h:InvalidCratesWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 133 | `TerrainSelectorFileDialog` | `FileDialog` | `code/interface/landscape_window.cpp:TerrainSelectorFileDialog` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 134 | `WaterSelectorFileDialog` | `FileDialog` | `code/interface/landscape_window.cpp:WaterSelectorFileDialog` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 135 | `WavesSelectorFileDialog` | `FileDialog` | `code/interface/landscape_window.cpp:WavesSelectorFileDialog` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 136 | `NewFileDialogButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:NewFileDialogButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 137 | `LandscapeTileButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:LandscapeTileButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 138 | `LandscapeFlattenAreaDeleteButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:LandscapeFlattenAreaDeleteButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 139 | `NewTileButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:NewTileButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 140 | `NewFlattenAreaButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:NewFlattenAreaButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 141 | `ScaleLandscapeButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:ScaleLandscapeButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 142 | `GuideGridButton` | `DarwiniaButton` | `code/interface/landscape_window.cpp:GuideGridButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 143 | `GuideGrid` | `EclButton` | `code/interface/landscape_window.cpp:GuideGrid` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 144 | `GuideGridTool` | `DarwiniaButton` | `code/interface/landscape_window.cpp:GuideGridTool` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 145 | `LandscapeTileEditWindow` | `DarwiniaWindow` | `code/interface/landscape_window.h:LandscapeTileEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 146 | `LandscapeFlattenAreaEditWindow` | `DarwiniaWindow` | `code/interface/landscape_window.h:LandscapeFlattenAreaEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 147 | `LandscapeEditWindow` | `DarwiniaWindow` | `code/interface/landscape_window.h:LandscapeEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 148 | `LandscapeGuideGridWindow` | `DarwiniaWindow` | `code/interface/landscape_window.h:LandscapeGuideGridWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 149 | `LeaderboardSmallFontRenderingButton` | `GameMenuButton` | `code/interface/leaderboards_window.cpp:LeaderboardSmallFontRenderingButton` | compiled into FULL, never reached | [B:LEGACY] |
| 150 | `LeaderBoardGamertagButton` | `LeaderboardSmallFontRenderingButton` | `code/interface/leaderboards_window.cpp:LeaderBoardGamertagButton` | compiled into FULL, never reached | [B:LEGACY] |
| 151 | `LeaderBoardTypeButton` | `GameMenuButton` | `code/interface/leaderboards_window.cpp:LeaderBoardTypeButton` | compiled into FULL, never reached | [B:LEGACY] |
| 152 | `LeaderBoardMessageButton` | `LeaderboardSmallFontRenderingButton` | `code/interface/leaderboards_window.cpp:LeaderBoardMessageButton` | compiled into FULL, never reached | [B:LEGACY] |
| 153 | `LeaderBoardWeeklyButton` | `LeaderBoardMessageButton` | `code/interface/leaderboards_window.cpp:LeaderBoardWeeklyButton` | compiled into FULL, never reached | [B:LEGACY] |
| 154 | `LeaderBoardListButton` | `GameMenuButton` | `code/interface/leaderboards_window.cpp:LeaderBoardListButton` | compiled into FULL, never reached | [B:LEGACY] |
| 155 | `LeaderBoardMiniListButton` | `LeaderBoardListButton` | `code/interface/leaderboards_window.cpp:LeaderBoardMiniListButton` | compiled into FULL, never reached | [B:LEGACY] |
| 156 | `LeaderboardLoader` | — | `code/interface/leaderboards_window.h:LeaderboardLoader` | compiled into FULL, never reached | [B:LEGACY] |
| 157 | `LeaderboardWindow` | `GameOptionsWindow` | `code/interface/leaderboards_window.h:LeaderboardWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 158 | `LeaderboardSelectionWindow` | `GameOptionsWindow` | `code/interface/leaderboards_window.h:LeaderboardSelectionWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 159 | `LightButton` | `DarwiniaButton` | `code/interface/lights_window.cpp:LightButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 160 | `LightGammaButton` | `DarwiniaButton` | `code/interface/lights_window.cpp:LightGammaButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 161 | `NewLightButton` | `DarwiniaButton` | `code/interface/lights_window.cpp:NewLightButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 162 | `LightsEditWindow` | `DarwiniaWindow` | `code/interface/lights_window.h:LightsEditWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 163 | `AboutDarwiniaButton` | `DarwiniaButton` | `code/interface/mainmenus.cpp:AboutDarwiniaButton` | compiled into FULL, never reached | [B:LEGACY] |
| 164 | `MainMenuUserProfileButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:MainMenuUserProfileButton` | compiled into FULL, never reached | [B:LEGACY] |
| 165 | `ModsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ModsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 166 | `MultiwiniaButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:MultiwiniaButton` | compiled into FULL, never reached | [B:LEGACY] |
| 167 | `ScreenOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ScreenOptionsButton` | live in the shipped build | [B:FULL] |
| 168 | `GraphicsOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:GraphicsOptionsButton` | live in the shipped build | [B:FULL] |
| 169 | `SoundOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:SoundOptionsButton` | live in the shipped build | [B:FULL] |
| 170 | `OtherOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:OtherOptionsButton` | live in the shipped build | [B:FULL] |
| 171 | `KeybindingsOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:KeybindingsOptionsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 172 | `MainCloseButton` | `MenuCloseButton` | `code/interface/mainmenus.cpp:MainCloseButton` | live in the shipped build | [B:FULL] |
| 173 | `ResetLevelButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ResetLevelButton` | compiled into FULL, never reached | [B:LEGACY] |
| 174 | `ExitLevelButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ExitLevelButton` | live in the shipped build | [B:FULL] |
| 175 | `WebsiteButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:WebsiteButton` | compiled into FULL, never reached | [B:LEGACY] |
| 176 | `ConfirmExitButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ConfirmExitButton` | live in the shipped build | [B:FULL] |
| 177 | `ResetLocationButton` | `GameMenuButton` | `code/interface/mainmenus.cpp:ResetLocationButton` | compiled into FULL, never reached | [B:LEGACY] |
| 178 | `GameOptionsWindow` | `DarwiniaWindow` | `code/interface/mainmenus.h:GameOptionsWindow` | live in the shipped build | [B:FULL] |
| 179 | `MainMenuWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:MainMenuWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 180 | `OptionsMenuWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:OptionsMenuWindow` | live in the shipped build | [B:FULL] |
| 181 | `LocationWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:LocationWindow` | live in the shipped build | [B:FULL] |
| 182 | `ResetLocationWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:ResetLocationWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 183 | `AboutDarwiniaWindow` | `DarwiniaWindow` | `code/interface/mainmenus.h:AboutDarwiniaWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 184 | `ConfirmExitWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:ConfirmExitWindow` | live in the shipped build | [B:FULL] |
| 185 | `ConfirmResetWindow` | `GameOptionsWindow` | `code/interface/mainmenus.h:ConfirmResetWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 186 | `MenuCloseButton` | `GameMenuButton` | `code/interface/mainmenus.h:MenuCloseButton` | live in the shipped build | [B:FULL] |
| 187 | `MenuGameExitButton` | `GameMenuButton` | `code/interface/mainmenus.h:MenuGameExitButton` | compiled into FULL, never reached | [B:LEGACY] |
| 188 | `HelpAndOptionsButton` | `GameMenuButton` | `code/interface/mainmenus.h:HelpAndOptionsButton` | live in the shipped build | [B:FULL] |
| 189 | `AchievementsButton` | `GameMenuButton` | `code/interface/mainmenus.h:AchievementsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 190 | `LeaderBoardButton` | `GameMenuButton` | `code/interface/mainmenus.h:LeaderBoardButton` | compiled into FULL, never reached | [B:LEGACY] |
| 191 | `ConfirmResetButton` | `GameMenuButton` | `code/interface/mainmenus.h:ConfirmResetButton` | compiled into FULL, never reached | [B:LEGACY] |
| 192 | `OKButton` | `DarwiniaButton` | `code/interface/message_dialog.cpp:OKButton` | live in the shipped build | [B:FULL] |
| 193 | `MessageDialog` | `DarwiniaWindow` | `code/interface/message_dialog.h:MessageDialog` | live in the shipped build | [B:FULL] |
| 194 | `LoadModButton` | `DarwiniaButton` | `code/interface/mods_window.cpp:LoadModButton` | not compiled in any PC configuration | [B:DEAD] |
| 195 | `ToolsButton` | `DarwiniaButton` | `code/interface/mods_window.cpp:ToolsButton` | not compiled in any PC configuration | [B:DEAD] |
| 196 | `NewModWindowButton` | `DarwiniaButton` | `code/interface/mods_window.cpp:NewModWindowButton` | not compiled in any PC configuration | [B:DEAD] |
| 197 | `NewModButton` | `DarwiniaButton` | `code/interface/mods_window.cpp:NewModButton` | not compiled in any PC configuration | [B:DEAD] |
| 198 | `ModsWindow` | `DarwiniaWindow` | `code/interface/mods_window.h:ModsWindow` | not compiled in any PC configuration | [B:DEAD] |
| 199 | `NewModWindow` | `DarwiniaWindow` | `code/interface/mods_window.h:NewModWindow` | not compiled in any PC configuration | [B:DEAD] |
| 200 | `MultiwiniaFileOKButton` | `GameMenuButton` | `code/interface/multiwinia_filedialog.cpp:MultiwiniaFileOKButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 201 | `MultiwiniaFileButton` | `DarwiniaButton` | `code/interface/multiwinia_filedialog.cpp:MultiwiniaFileButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 202 | `MultiwiniaFileDialog` | `GameOptionsWindow` | `code/interface/multiwinia_filedialog.h:MultiwiniaFileDialog` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 203 | `CreateMapButton` | `GameMenuButton` | `code/interface/multiwinia_window.cpp:CreateMapButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 204 | `EditMapButton` | `GameMenuButton` | `code/interface/multiwinia_window.cpp:EditMapButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 205 | `PlayMapButton` | `DarwiniaButton` | `code/interface/multiwinia_window.cpp:PlayMapButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 206 | `SelectMapButton` | `GameMenuButton` | `code/interface/multiwinia_window.cpp:SelectMapButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 207 | `NewMapWindowButton` | `GameMenuButton` | `code/interface/multiwinia_window.cpp:NewMapWindowButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 208 | `CurrentMapButton` | `DarwiniaButton` | `code/interface/multiwinia_window.cpp:CurrentMapButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 209 | `MultiwiniaWindow` | `GameOptionsWindow` | `code/interface/multiwinia_window.h:MultiwiniaWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 210 | `NewMapWindow` | `GameOptionsWindow` | `code/interface/multiwinia_window.h:NewMapWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 211 | `TextTab` | `DarwiniaButton` | `code/interface/multiwiniaoptions_window.cpp:TextTab` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 212 | `AttackersButton` | `DarwiniaButton` | `code/interface/multiwiniaoptions_window.cpp:AttackersButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 213 | `CoopGroupButton` | `DarwiniaButton` | `code/interface/multiwiniaoptions_window.cpp:CoopGroupButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 214 | `MultiwiniaOptionsWindow` | `DarwiniaWindow` | `code/interface/multiwiniaoptions_window.h:MultiwiniaOptionsWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 215 | `NetworkWindow` | `DarwiniaWindow` | `code/interface/network_window.h:NetworkWindow` | live in the shipped build | [B:FULL] |
| 216 | `ApplyGraphicsButton` | `GameMenuButton` | `code/interface/prefs_graphics_window.cpp:ApplyGraphicsButton` | live in the shipped build | [B:FULL] |
| 217 | `PrefsGraphicsWindow` | `GameOptionsWindow` | `code/interface/prefs_graphics_window.h:PrefsGraphicsWindow` | live in the shipped build | [B:FULL] |
| 218 | `RestoreDefaultsButton` | `DarwiniaButton` | `code/interface/prefs_keybindings_window.cpp:RestoreDefaultsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 219 | `ApplyKeybindingsButton` | `DarwiniaButton` | `code/interface/prefs_keybindings_window.cpp:ApplyKeybindingsButton` | compiled into FULL, never reached | [B:LEGACY] |
| 220 | `ChangeKeybindingButton` | `DarwiniaButton` | `code/interface/prefs_keybindings_window.cpp:ChangeKeybindingButton` | compiled into FULL, never reached | [B:LEGACY] |
| 221 | `ControlMethodDropDownMenu` | `DropDownMenu` | `code/interface/prefs_keybindings_window.cpp:ControlMethodDropDownMenu` | compiled into FULL, never reached | [B:LEGACY] |
| 222 | `PrefsKeybindingsWindow` | `DarwiniaWindow` | `code/interface/prefs_keybindings_window.h:PrefsKeybindingsWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 223 | `ApplyOtherButton` | `GameMenuButton` | `code/interface/prefs_other_window.cpp:ApplyOtherButton` | live in the shipped build | [B:FULL] |
| 224 | `PrefsOtherWindow` | `GameOptionsWindow` | `code/interface/prefs_other_window.h:PrefsOtherWindow` | live in the shipped build | [B:FULL] |
| 225 | `ScreenResDropDownMenu` | `GameMenuDropDown` | `code/interface/prefs_screen_window.cpp:ScreenResDropDownMenu` | live in the shipped build | [B:FULL] |
| 226 | `FullscreenRequiredMenu` | `GameMenuDropDown` | `code/interface/prefs_screen_window.cpp:FullscreenRequiredMenu` | live in the shipped build | [B:FULL] |
| 227 | `SetScreenButton` | `GameMenuButton` | `code/interface/prefs_screen_window.cpp:SetScreenButton` | live in the shipped build | [B:FULL] |
| 228 | `PrefsScreenWindow` | `GameOptionsWindow` | `code/interface/prefs_screen_window.h:PrefsScreenWindow` | live in the shipped build | [B:FULL] |
| 229 | `RestartSoundButton` | `GameMenuButton` | `code/interface/prefs_sound_window.cpp:RestartSoundButton` | live in the shipped build | [B:FULL] |
| 230 | `HW3DDropDownMenu` | `GameMenuCheckBox` | `code/interface/prefs_sound_window.cpp:HW3DDropDownMenu` | not compiled in any PC configuration | [B:DEAD] |
| 231 | `PrefsSoundWindow` | `GameOptionsWindow` | `code/interface/prefs_sound_window.h:PrefsSoundWindow` | live in the shipped build | [B:FULL] |
| 232 | `ProfilerButton` | `DarwiniaButton` | `code/interface/profilewindow.cpp:ProfilerButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 233 | `ProfileWindow` | `DarwiniaWindow` | `code/interface/profilewindow.h:ProfileWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 234 | `ReallyQuitWindow` | `DarwiniaWindow` | `code/interface/reallyquit_window.h:ReallyQuitWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 235 | `YesButton` | `DarwiniaButton` | `code/interface/save_on_quit_window.cpp:YesButton` | compiled into FULL, never reached | [B:LEGACY] |
| 236 | `NoButton` | `DarwiniaButton` | `code/interface/save_on_quit_window.cpp:NoButton` | compiled into FULL, never reached | [B:LEGACY] |
| 237 | `CancelButton` | `DarwiniaButton` | `code/interface/save_on_quit_window.cpp:CancelButton` | compiled into FULL, never reached | [B:LEGACY] |
| 238 | `SaveOnQuitWindow` | `DarwiniaWindow` | `code/interface/save_on_quit_window.h:SaveOnQuitWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 239 | `ScrollBar` | — | `code/interface/scrollbar.h:ScrollBar` | live in the shipped build | [B:FULL] |
| 240 | `ScrollBarButton` | `EclButton` | `code/interface/scrollbar.h:ScrollBarButton` | live in the shipped build | [B:FULL] |
| 241 | `ScrollChangeButton` | `DarwiniaButton` | `code/interface/scrollbar.h:ScrollChangeButton` | live in the shipped build | [B:FULL] |
| 242 | `ResetHistoryButton` | `DarwiniaButton` | `code/interface/sound_profile_window.cpp:ResetHistoryButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 243 | `SoundProfileWindow` | `DarwiniaWindow` | `code/interface/sound_profile_window.h:SoundProfileWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 244 | `SoundEventButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SoundEventButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 245 | `ObjectTypeMenu` | `DropDownMenu` | `code/interface/soundeditor_window.cpp:ObjectTypeMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 246 | `ObjectGroupMenu` | `DropDownMenu` | `code/interface/soundeditor_window.cpp:ObjectGroupMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 247 | `NewSoundEventMenu` | `DropDownMenu` | `code/interface/soundeditor_window.cpp:NewSoundEventMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 248 | `SoundPositionButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SoundPositionButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 249 | `PlayButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:PlayButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 250 | `PlayRepeatButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:PlayRepeatButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 251 | `StopButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:StopButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 252 | `SelectObjectButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SelectObjectButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 253 | `SaveAllButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SaveAllButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 254 | `DeleteEventButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:DeleteEventButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 255 | `SoundSelector` | `FileDialog` | `code/interface/soundeditor_window.cpp:SoundSelector` | TARGET_DEBUG builds only | [B:DEBUG] |
| 256 | `SoundSelectorButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SoundSelectorButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 257 | `DspEffectButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:DspEffectButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 258 | `DeleteDspEffectButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:DeleteDspEffectButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 259 | `PurgeSoundsButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:PurgeSoundsButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 260 | `ConvertVolumesButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:ConvertVolumesButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 261 | `DspEffectTypeMenu` | `DropDownMenu` | `code/interface/soundeditor_window.cpp:DspEffectTypeMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 262 | `SampleGroupFileDialog` | `FileDialog` | `code/interface/soundeditor_window.cpp:SampleGroupFileDialog` | TARGET_DEBUG builds only | [B:DEBUG] |
| 263 | `SampleGroupSampleName` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SampleGroupSampleName` | TARGET_DEBUG builds only | [B:DEBUG] |
| 264 | `SampleGroupSampleDelete` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SampleGroupSampleDelete` | TARGET_DEBUG builds only | [B:DEBUG] |
| 265 | `SampleGroupAddSample` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:SampleGroupAddSample` | TARGET_DEBUG builds only | [B:DEBUG] |
| 266 | `SampleGroupSelector` | `DropDownMenu` | `code/interface/soundeditor_window.cpp:SampleGroupSelector` | TARGET_DEBUG builds only | [B:DEBUG] |
| 267 | `NewSampleGroupButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:NewSampleGroupButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 268 | `RenameSampleGroupCommit` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:RenameSampleGroupCommit` | TARGET_DEBUG builds only | [B:DEBUG] |
| 269 | `RenameSampleGroupButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:RenameSampleGroupButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 270 | `DeleteSoundButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:DeleteSoundButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 271 | `RefreshFileListButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:RefreshFileListButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 272 | `DeleteAllButton` | `DarwiniaButton` | `code/interface/soundeditor_window.cpp:DeleteAllButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 273 | `SoundEditorWindow` | `DarwiniaWindow` | `code/interface/soundeditor_window.h:SoundEditorWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 274 | `DspEffectEditor` | `DarwiniaWindow` | `code/interface/soundeditor_window.h:DspEffectEditor` | TARGET_DEBUG builds only | [B:DEBUG] |
| 275 | `SampleGroupEditor` | `DarwiniaWindow` | `code/interface/soundeditor_window.h:SampleGroupEditor` | TARGET_DEBUG builds only | [B:DEBUG] |
| 276 | `RenameSampleGroupWindow` | `DarwiniaWindow` | `code/interface/soundeditor_window.h:RenameSampleGroupWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 277 | `PurgeSoundsWindow` | `DarwiniaWindow` | `code/interface/soundeditor_window.h:PurgeSoundsWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 278 | `SoundParameterGraphScaleToggle` | `DarwiniaButton` | `code/interface/soundparameter_window.cpp:SoundParameterGraphScaleToggle` | TARGET_DEBUG builds only | [B:DEBUG] |
| 279 | `LinkTypeMenu` | `DropDownMenu` | `code/interface/soundparameter_window.cpp:LinkTypeMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 280 | `UpdateTypeMenu` | `DropDownMenu` | `code/interface/soundparameter_window.cpp:UpdateTypeMenu` | TARGET_DEBUG builds only | [B:DEBUG] |
| 281 | `SoundParameterButton` | `BorderlessButton` | `code/interface/soundparameter_window.h:SoundParameterButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 282 | `SoundParameterEditor` | `DarwiniaWindow` | `code/interface/soundparameter_window.h:SoundParameterEditor` | TARGET_DEBUG builds only | [B:DEBUG] |
| 283 | `SoundParameterGraph` | `InvertedBox` | `code/interface/soundparameter_window.h:SoundParameterGraph` | TARGET_DEBUG builds only | [B:DEBUG] |
| 284 | `SoundStatsWindow` | `DarwiniaWindow` | `code/interface/soundstats_window.h:SoundStatsWindow` | TARGET_DEBUG builds only | [B:DEBUG] |
| 285 | `TreeButton` | `DarwiniaButton` | `code/interface/tree_window.cpp:TreeButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 286 | `TreeWindow` | `DarwiniaWindow` | `code/interface/tree_window.h:TreeWindow` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 287 | `GetItNowButton` | `DarwiniaButton` | `code/interface/updateavailable_window.cpp:GetItNowButton` | not compiled in any PC configuration | [B:DEAD] |
| 288 | `UpdateAvailableWindow` | `MessageDialog` | `code/interface/updateavailable_window.h:UpdateAvailableWindow` | not compiled in any PC configuration | [B:DEAD] |
| 289 | `NewProfileButton` | `DarwiniaButton` | `code/interface/userprofile_window.cpp:NewProfileButton` | compiled into FULL, never reached | [B:LEGACY] |
| 290 | `LoadUserProfileButton` | `GameMenuButton` | `code/interface/userprofile_window.cpp:LoadUserProfileButton` | compiled into FULL, never reached | [B:LEGACY] |
| 291 | `NewProfileWindowButton` | `DarwiniaButton` | `code/interface/userprofile_window.cpp:NewProfileWindowButton` | compiled into FULL, never reached | [B:LEGACY] |
| 292 | `UserProfileWindow` | `GameOptionsWindow` | `code/interface/userprofile_window.h:UserProfileWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 293 | `ResetProfileButton` | `GameMenuButton` | `code/interface/userprofile_window.h:ResetProfileButton` | compiled into FULL, never reached | [B:LEGACY] |
| 294 | `NewUserProfileWindow` | `DarwiniaWindow` | `code/interface/userprofile_window.h:NewUserProfileWindow` | compiled into FULL, never reached | [B:LEGACY] |
| 295 | `AchievementButton` | `GameMenuButton` | `code/UI/AchievementButton.h:AchievementButton` | compiled into FULL, never reached | [B:LEGACY] |
| 296 | `AdvancedOptionsModeButton` | `GameMenuButton` | `code/UI/AdvancedOptionsModeButton.h:AdvancedOptionsModeButton` | live in the shipped build | [B:FULL] |
| 297 | `AnyLevelSelectButton` | `LevelSelectButtonBase` | `code/UI/AnyLevelSelectButton.h:AnyLevelSelectButton` | compiled into FULL, never reached | [B:LEGACY] |
| 298 | `ApplyNameButton` | `BackPageButton` | `code/UI/ApplyNameButton.h:ApplyNameButton` | live in the shipped build | [B:FULL] |
| 299 | `AuthInputField` | `InputField` | `code/UI/AuthInputField.h:AuthInputField` | live in the shipped build | [B:FULL] |
| 300 | `AuthPageButton` | `GameMenuButton` | `code/UI/AuthPageButton.h:AuthPageButton` | live in the shipped build | [B:FULL] |
| 301 | `AuthStatusButton` | `GameMenuButton` | `code/UI/AuthStatusButton.h:AuthStatusButton` | live in the shipped build | [B:FULL] |
| 302 | `BuyNowInfoButton` | `DarwiniaButton` | `code/UI/BuyNowInfoButton.h:BuyNowInfoButton` | compiled into FULL, never reached | [B:LEGACY] |
| 303 | `BuyNowModeButton` | `GameMenuButton` | `code/UI/BuyNowModeButton.h:BuyNowModeButton` | live in the shipped build | [B:FULL] |
| 304 | `CSpectatorButton` | `GameMenuButton` | `code/UI/CSpectatorButton.h:CSpectatorButton` | not compiled in any PC configuration | [B:DEAD] |
| 305 | `CampaignButton` | `GameMenuButton` | `code/UI/CampaignButton.h:CampaignButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 306 | `ServerConnectButton` | `GameMenuButton` | `code/UI/ConnectButton.h:ServerConnectButton` | live in the shipped build | [B:FULL] |
| 307 | `DarwiniaModeButton` | `GameMenuButton` | `code/UI/DarwiniaModeButton.h:DarwiniaModeButton` | live in the shipped build | [B:FULL] |
| 308 | `DarwiniaPlusMPButton` | `GameMenuButton` | `code/UI/DarwiniaPlusMPButton.h:DarwiniaPlusMPButton` | compiled into FULL, never reached | [B:LEGACY] |
| 309 | `DarwiniaPlusSPButton` | `GameMenuButton` | `code/UI/DarwiniaPlusSPButton.h:DarwiniaPlusSPButton` | compiled into FULL, never reached | [B:LEGACY] |
| 310 | `EnterAuthkeyButton` | `GameMenuButton` | `code/UI/EnterAuthKeyButton.h:EnterAuthkeyButton` | live in the shipped build | [B:FULL] |
| 311 | `ErrorDialog` | `DarwiniaWindow` | `code/UI/ErrorDialog.h:ErrorDialog` | not compiled in any PC configuration | [B:DEAD] |
| 312 | `FancyColourButton` | `GameMenuButton` | `code/UI/FancyColourButton.h:FancyColourButton` | live in the shipped build | [B:FULL] |
| 313 | `FiltersPageButton` | `GameMenuButton` | `code/UI/FiltersPageButton.h:FiltersPageButton` | live in the shipped build | [B:FULL] |
| 314 | `FinishMultiwiniaButton` | `BackPageButton` | `code/UI/FinishMultiwiniaButton.h:FinishMultiwiniaButton` | live in the shipped build | [B:FULL] |
| 315 | `FinishSinglePlayerButton` | `BackPageButton` | `code/UI/FinishMultiwiniaButton.h:FinishSinglePlayerButton` | live in the shipped build | [B:FULL] |
| 316 | `SaveFiltersButton` | `BackPageButton` | `code/UI/FinishMultiwiniaButton.h:SaveFiltersButton` | live in the shipped build | [B:FULL] |
| 317 | `GameMenuOptionsButton` | `GameMenuButton` | `code/UI/GameMenuOptionsButton.h:GameMenuOptionsButton` | not compiled in any PC configuration | [B:DEAD] |
| 318 | `GameMenuChatButton` | `DarwiniaButton` | `code/UI/GameMenuWindow.cpp:GameMenuChatButton` | live in the shipped build | [B:FULL] |
| 319 | `ServerPasswordString` | `NetworkUnicodeString` | `code/UI/GameMenuWindow.h:ServerPasswordString` | live in the shipped build | [B:FULL] |
| 320 | `GameMenuWindow` | `DarwiniaWindow` | `code/UI/GameMenuWindow.h:GameMenuWindow` | live in the shipped build | [B:FULL] |
| 321 | `GameServerButton` | `GameMenuButton` | `code/UI/GameServerButton.h:GameServerButton` | live in the shipped build | [B:FULL] |
| 322 | `GameTypeAnyButton` | `GameMenuButton` | `code/UI/GameTypeAnyButton.h:GameTypeAnyButton` | compiled into FULL, never reached | [B:LEGACY] |
| 323 | `GameTypeButton` | `GameMenuButton` | `code/UI/GameTypeButton.h:GameTypeButton` | live in the shipped build | [B:FULL] |
| 324 | `GameTypeImageButton` | `DarwiniaButton` | `code/UI/GameTypeImageButton.h:GameTypeImageButton` | live in the shipped build | [B:FULL] |
| 325 | `GameTypeInfoButton` | `ScrollingTextButton` | `code/UI/GameTypeInfoButton.h:GameTypeInfoButton` | live in the shipped build | [B:FULL] |
| 326 | `GamertagButton` | `GameMenuButton` | `code/UI/GamerTagButton.h:GamertagButton` | live in the shipped build | [B:FULL] |
| 327 | `HelpImageButton` | `DarwiniaButton` | `code/UI/HelpImageButton.h:HelpImageButton` | compiled into FULL, never reached | [B:LEGACY] |
| 328 | `HelpModeButton` | `GameMenuButton` | `code/UI/HelpModeButton.h:HelpModeButton` | not compiled in any PC configuration | [B:DEAD] |
| 329 | `HelpSelectButton` | `GameMenuButton` | `code/UI/HelpSelectButton.h:HelpSelectButton` | compiled into FULL, never reached | [B:LEGACY] |
| 330 | `ImageButton` | `DarwiniaButton` | `code/UI/ImageButton.h:ImageButton` | live in the shipped build | [B:FULL] |
| 331 | `JoinGameButton` | `GameMenuButton` | `code/UI/JoinGameButton.h:JoinGameButton` | live in the shipped build | [B:FULL] |
| 332 | `KickPlayerButton` | `DarwiniaButton` | `code/UI/KickPlayerButton.h:KickPlayerButton` | live in the shipped build | [B:FULL] |
| 333 | `LevelSelectButton` | `LevelSelectButtonBase` | `code/UI/LevelSelectButton.h:LevelSelectButton` | live in the shipped build | [B:FULL] |
| 334 | `LevelSelectButtonBase` | `DarwiniaButton` | `code/UI/LevelSelectButtonBase.h:LevelSelectButtonBase` | live in the shipped build | [B:FULL] |
| 335 | `MapData` | — | `code/UI/MapData.h:MapData` | live in the shipped build | [B:FULL] |
| 336 | `MapImageButton` | `DarwiniaButton` | `code/UI/MapImageButton.h:MapImageButton` | live in the shipped build | [B:FULL] |
| 337 | `MapInfoButton` | `ScrollingTextButton` | `code/UI/MapInfoButton.h:MapInfoButton` | live in the shipped build | [B:FULL] |
| 338 | `MapListTitlesButton` | `DarwiniaButton` | `code/UI/MapListTitlesButton.h:MapListTitlesButton` | live in the shipped build | [B:FULL] |
| 339 | `MapSelectModeButton` | `GameMenuButton` | `code/UI/MapSelectModeButton.h:MapSelectModeButton` | live in the shipped build | [B:FULL] |
| 340 | `MultiwiniaEditorButton` | `GameMenuButton` | `code/UI/MultiwiniaEditorButton.h:MultiwiniaEditorButton` | LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) | [B:EDITOR] |
| 341 | `MultiwiniaModeButton` | `GameMenuButton` | `code/UI/MultiwiniaModeButton.h:MultiwiniaModeButton` | not compiled in any PC configuration | [B:DEAD] |
| 342 | `MultiwiniaVersusCpuButton` | `GameMenuButton` | `code/UI/MultiwiniaVersusCpuButton.h:MultiwiniaVersusCpuButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 343 | `NewOrJoinButton` | `GameMenuButton` | `code/UI/NewOrJoinButton.h:NewOrJoinButton` | live in the shipped build | [B:FULL] |
| 344 | `NewServerButton` | `GameMenuButton` | `code/UI/NewServerButton.h:NewServerButton` | live in the shipped build | [B:FULL] |
| 345 | `NextHelpButton` | `GameMenuButton` | `code/UI/NextHelpButton.h:NextHelpButton` | compiled into FULL, never reached | [B:LEGACY] |
| 346 | `OptionDetailsButton` | `ScrollingTextButton` | `code/UI/OptionDetailsButton.h:OptionDetailsButton` | live in the shipped build | [B:FULL] |
| 347 | `PasteKeyButton` | `GameMenuButton` | `code/UI/PasteKeyButton.h:PasteKeyButton` | live in the shipped build | [B:FULL] |
| 348 | `PlayDemoAuthkeyButton` | `GameMenuButton` | `code/UI/PlayDemoAuthkeyButton.h:PlayDemoAuthkeyButton` | live in the shipped build | [B:FULL] |
| 349 | `PlayGameButton` | `GameMenuButton` | `code/UI/PlayGameButton.h:PlayGameButton` | live in the shipped build | [B:FULL] |
| 350 | `PrevHelpButton` | `GameMenuButton` | `code/UI/PrevHelpButton.h:PrevHelpButton` | compiled into FULL, never reached | [B:LEGACY] |
| 351 | `PrologueButton` | `GameMenuButton` | `code/UI/PrologueButton.h:PrologueButton` | TARGET_DEBUG builds only | [B:DEBUG] |
| 352 | `QuickMatchButton` | `GameMenuButton` | `code/UI/QuickMatchButton.h:QuickMatchButton` | compiled into FULL, never reached | [B:LEGACY] |
| 353 | `QuickStartButton` | `GameMenuButton` | `code/UI/QuickStartButton.h:QuickStartButton` | not compiled in any PC configuration | [B:DEAD] |
| 354 | `QuitAuthkeyButton` | `GameMenuButton` | `code/UI/QuitAuthKeyButton.h:QuitAuthkeyButton` | other product target / platform only | [B:DEAD] |
| 355 | `QuitButton` | `GameMenuButton` | `code/UI/QuitButton.h:QuitButton` | live in the shipped build | [B:FULL] |
| 356 | `RequestAIButton` | `GameMenuButton` | `code/UI/RequestAIButton.h:RequestAIButton` | not compiled in any PC configuration | [B:DEAD] |
| 357 | `ResearchModeButton` | `GameMenuButton` | `code/UI/ResearchModeButton.h:ResearchModeButton` | not compiled in any PC configuration | [B:DEAD] |
| 358 | `RollingDemoButton` | `GameMenuButton` | `code/UI/RollingDemoButton.h:RollingDemoButton` | not compiled in any PC configuration | [B:DEAD] |
| 359 | `ScrollingTextButton` | `DarwiniaButton` | `code/UI/ScrollingTextButton.h:ScrollingTextButton` | live in the shipped build | [B:FULL] |
| 360 | `ServerTitleInfoButton` | `DarwiniaButton` | `code/UI/ServerTitleInfoButton.h:ServerTitleInfoButton` | live in the shipped build | [B:FULL] |
| 361 | `StartTestbedButton` | `GameMenuButton` | `code/UI/TestBedButtons.h:StartTestbedButton` | not compiled in any PC configuration | [B:DEAD] |
| 362 | `TutorialPageButton` | `GameMenuButton` | `code/UI/TutorialButton.h:TutorialPageButton` | live in the shipped build | [B:FULL] |
| 363 | `TutorialButton` | `GameMenuButton` | `code/UI/TutorialButton.h:TutorialButton` | live in the shipped build | [B:FULL] |
| 364 | `UnlockFullGameButton` | `GameMenuButton` | `code/UI/UnlockFullGameButton.h:UnlockFullGameButton` | live in the shipped build | [B:FULL] |
| 365 | `WaitingButton` | `DarwiniaButton` | `code/UI/WaitingButton.h:WaitingButton` | live in the shipped build | [B:FULL] |
| 366 | `CreditsButton` | `GameMenuButton` | `code/UI/credits_button.h:CreditsButton` | live in the shipped build | [B:FULL] |
| 367 | `MapDetailsButton` | `DarwiniaButton` | `code/UI/mapDetailsButton.h:MapDetailsButton` | not compiled in any PC configuration | [B:DEAD] |
| 368 | `MOTDButton` | `ScrollingTextButton` | `code/UI/motdButton.h:MOTDButton` | live in the shipped build | [B:FULL] |
| 369 | `VolumeControl` | `DarwiniaButton` | `code/UI/soundsliderbutton.h:VolumeControl` | not compiled in any PC configuration | [B:DEAD] |
| 370 | `UpdatePageButton` | `GameMenuButton` | `code/UI/update_page_buttons.h:UpdatePageButton` | live in the shipped build | [B:FULL] |
| 371 | `RunAutoPatcherButton` | `GameMenuButton` | `code/UI/update_page_buttons.h:RunAutoPatcherButton` | live in the shipped build | [B:FULL] |
| 372 | `RunSparkleButton` | `GameMenuButton` | `code/UI/update_page_buttons.h:RunSparkleButton` | other product target / platform only | [B:DEAD] |
| 373 | `VisitWebsiteButton` | `GameMenuButton` | `code/UI/update_page_buttons.h:VisitWebsiteButton` | live in the shipped build | [B:FULL] |
| 374 | `PlayerNameString` | `NetworkUnicodeString` | `code/game_menu.h:PlayerNameString` | live in the shipped build | [B:FULL] |
| 375 | `GameMenu` | — | `code/game_menu.h:GameMenu` | live in the shipped build | [B:FULL] |
| 376 | `GameMenuTitleButton` | `DarwiniaButton` | `code/game_menu.h:GameMenuTitleButton` | live in the shipped build | [B:FULL] |
| 377 | `GameMenuButton` | `DarwiniaButton` | `code/game_menu.h:GameMenuButton` | live in the shipped build | [B:FULL] |
| 378 | `GameMenuDropDown` | `DropDownMenu` | `code/game_menu.h:GameMenuDropDown` | live in the shipped build | [B:FULL] |
| 379 | `GameMenuDropDownWindow` | `DropDownWindow` | `code/game_menu.h:GameMenuDropDownWindow` | live in the shipped build | [B:FULL] |
| 380 | `GameMenuDropDownMenuOption` | `DropDownMenuOption` | `code/game_menu.h:GameMenuDropDownMenuOption` | live in the shipped build | [B:FULL] |
| 381 | `GameMenuInputField` | `InputField` | `code/game_menu.h:GameMenuInputField` | live in the shipped build | [B:FULL] |
| 382 | `GameMenuInputScroller` | `InputScroller` | `code/game_menu.h:GameMenuInputScroller` | live in the shipped build | [B:FULL] |
| 383 | `BackPageButton` | `GameMenuButton` | `code/game_menu.h:BackPageButton` | live in the shipped build | [B:FULL] |
| 384 | `GameMenuCheckBox` | `CheckBox` | `code/game_menu.h:GameMenuCheckBox` | live in the shipped build | [B:FULL] |

Totals: not compiled in any PC configuration = 31; TARGET_DEBUG builds only = 75; compiled only in TARGET_DEBUG builds, never instantiated (no caller) = 3; LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code) = 77; compiled into FULL, only used by editor/debug tools = 7; live in the shipped build = 119; other product target / platform only = 2; compiled into FULL, never reached = 70; all declarations = 384.

---

## 13 Framework defects and edge cases (do not carry forward)

Eclipse-level quirks Q1–Q10 are in §2.13. Darwinia/front-end defects [C] unless marked:

| # | Defect | Effect | Ref |
|---|---|---|---|
| F1 | `InputField::RegisterBool` sets `m_type = TypeChar`; `InputScroller` then casts the `LocalBool` to `NetworkChar*` | works only because both adaptors have the same layout [I]; bool increments unbounded | `code/interface/input_field.cpp:InputField::RegisterBool`, `InputScroller::CheckInput` |
| F2 | `TypeChar` scroller ignores bounds | colour bytes wrap 255 → 0 | `InputScroller::CheckInput` |
| F3 | Esc commits instead of cancelling | no way to abandon an edit | `InputField::Keypress` |
| F4 | Typed numbers not clamped | out-of-range values reach data (local adaptors) | `NetworkValue::FromUnicodeString` |
| F5 | `DarwiniaWindow::Update` asks `m_buttons[m_currentButton]` (registration list) for `SpecialScroll` | wrong widget gets the scroll hook | `DarwiniaWindow::Update` |
| F6 | `MenuUp` loop never increments its counter | hang if every navigation entry is inactive | `DarwiniaWindow::Update` |
| F7 | Darwinia window title always "MULTIWINIA" | `m_title` / window phrase never shown | `DarwiniaWindow::Render` |
| F8 | `BorderlessButton::Render` calls `SetCurrentButton` | render-order-dependent focus | `BorderlessButton::Render` |
| F9 | `MessageDialog` fixed 20-line array | buffer overflow on longer messages | `MessageDialog::MessageDialog` |
| F10 | `MessageDialog` not controller-dismissable | pad soft-lock until a mouse is used | §4.7 |
| F11 | Darwinia drop-down open list shows raw phrase keys | untranslated list items | `DropDownMenu::CreateMenu` |
| F12 | `ScrollChangeButton` leaves additive blend and re-enables depth writes | later UI drawn additive until a text draw resets it | `ScrollChangeButton::Render` |
| F13 | Scroll repeat and float scrollers are frame-rate dependent | inconsistent feel | §4.2, §4.4 |
| F14 | `ScrollBarButton::Render` divides by `m_numRows` (0 for an empty list) | degenerate thumb | `ScrollBarButton::Render` |
| F15 | `SetMenuSize(screenW, screenH)` under large menus scales full-screen pages beyond the screen (then clamped to a negative origin) | page partly off-screen | `DarwiniaWindow::SetMenuSize`, `GameOptionsWindow::GameOptionsWindow` |
| F16 | `GameMenuInputField` roll-over guard compares with `"none"` (sentinel is `"None"`) | input fields never play `MenuRollOver` | `code/game_menu.cpp:GameMenuInputField::Render` |
| F17 | `m_disabled` is ignored by `GameMenuButton` (render and click) | e.g. "Host" stays clickable when hosting is not permitted [I consequence] | `GameMenuButton::Render`, `code/UI/NewServerButton.h`, `GameMenuWindow::SetupNewOrJoinPage` |
| F18 | `FancyColourButton` computes "taken" differently in `MouseUp` (counts unused slots and the local team) and `Render` (excludes them) and sets `m_inactive` from `Render` | clicking one's own current colour does nothing; navigation skips depend on the last rendered frame | `code/UI/FancyColourButton.h` |
| F19 | Pad Back of `GameMenuWindow` pages is polled inside `BackPageButton::Render` | Back does nothing whenever that button is not rendered (unfocused window, open drop-down) and lags one frame | `code/game_menu.cpp:BackPageButton::Render` |
| F20 | `GameServerButton::SpecialScroll` writes `m_scrollBar->m_currentValue` directly | bypasses clamping and dirtying | `code/UI/GameServerButton.h:GameServerButton::SpecialScroll` |
| F21 | Server rows inserted into `m_buttonOrder` at `Size()−1` but removed at `m_serverButtonOrderStartIndex + row` | wrong entries (possibly Back) removed when the list shrinks [I consequence] | `GameMenuWindow::UpdateJoinGamePage` |
| F22 | Browser scroll bar window size 10 for a 13-row list | paging/thumb proportions off by 3 rows | `GameMenuWindow::SetupJoinGamePage` |
| F23 | `AuthInputField::Render` draws its background at window-relative `m_x, m_y` | correct only because the front-end window is at (0,0) | `code/UI/AuthInputField.h` |
| F24 | `GameMenuDropDownMenuOption` caption x = `realX + realX × 0.05` | indent depends on screen position | `code/game_menu.cpp:GameMenuDropDownMenuOption::Render` |
| F25 | `DropDownWindow` registered before it is sized | no on-screen clamp for the list; inherited close box at x = −12 | `DropDownMenu::CreateMenu` |
| F26 | `MessageDialog` draws message lines through printf-style `DrawText2D` | `%` in a message is interpreted as a format directive | `MessageDialog::Render` |
| F27 | `TextButton(UnicodeString)` constructor leaves members uninitialised | garbage flags if ever used | `code/interface/helpandoptions_windows.cpp:TextButton::TextButton` |
| F28 | Missing-phrase results share one `m_notFound` buffer | the reference returned by one miss is overwritten by the next; not thread-safe | `LangTable::LookupPhrase` |
| F29 | `CheckBox::MouseUp` toggles without checking `m_disabled`/`m_inactive` | "inactive" check boxes still toggle | `CheckBox::MouseUp` |
| F30 | `SetupNewPage` indexes `s_pageNames[_page]` before validating `_page` | out-of-bounds read before the release assert | `GameMenuWindow::SetupNewPage` |
| F31 | `MultiwiniaFileDialog` constructor creates a `ScrollBar` that `Create` replaces | leak [B:EDITOR] | `MultiwiniaFileDialog::MultiwiniaFileDialog` |
| F32 | `DropDownMenu::GetSelectionName` bounds check uses `>` | off-by-one read | `DropDownMenu::GetSelectionName` |
| F33 | Window drag stops for any frame in which the cursor passes over a button (that button receives `MouseDown` instead) | jerky window drags over dense windows | `EclUpdateMouse` |

---

## 14 Constants table

All values [C] at the cited symbol.

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `SIZE_ECLWINDOW_NAME`, `SIZE_ECLWINDOW_TITLE`, `SIZE_ECLBUTTON_NAME` | 256 | `#define` | `contrib/eclipse/eclwindow.h`, `contrib/eclipse/eclbutton.h` |
| Tooltip delay | 1000 ms | literal | `contrib/eclipse/eclipse.cpp:EclUpdateMouse` |
| Resize grab band | ±4 px (exclusive) around bottom/right edge | literal | `EclUpdateMouse` |
| Minimum window size on drag-resize | 60 × 40 px | literal | `EclUpdateMouse` |
| Auto-grow margin | 10 px below the lowest button | literal | `EclWindow::RegisterButton` |
| `APP_RAND_MAX` (child placement) | 32767 | `#define` | `contrib/systemiv/lib/math/random_number.h` |
| Default button font size | 11.0 | ctor | `DarwiniaButton::DarwiniaButton` |
| Auto button size | `chars × 7 + 9` px wide, 15 px high | literal | `DarwiniaButton::SetShortProperties` |
| Title-bar height | `GetMenuSize(15)` px | literal | `DarwiniaWindow::GetClientRectY1` |
| Close box | 10 × 10 at `(w − 12, 2)` | literal | `DarwiniaWindow::Create` |
| Large-menu scale | `0.96 × screenH / 460` | literal | `DarwiniaWindow::GetMenuSize` |
| Large-menu preference | `LargeMenus == 2` | pref | `code/app.cpp:App::Initialise`, `code/interface/prefs_other_window.h:OTHER_LARGEMENUS` |
| Window body tint alpha | 0.96 | literal | `DarwiniaWindow::Render` |
| Body texture repeat | s × 8 (assumes 512×64) | literal | `DarwiniaWindow::Render` |
| Darwinia title text size / baseline | `GetMenuSize(12)` / `y + 9` | literal | `DarwiniaWindow::Render` |
| Value-control field width | `w − 37` (numeric); scrollers 15×15 at +5 and +22 | literal | `DarwiniaWindow::CreateValueControl` |
| `PIXELS_PER_CHAR` (Darwinia) | 7 | `#define` | `code/interface/input_field.cpp` |
| `PIXELS_PER_CHAR` (front end) | 25 | `#define` | `code/game_menu.cpp` |
| Input box width | `len × PIXELS_PER_CHAR + 7` | literal | `InputField::Keypress`, `InputField::Render` |
| Edit well | right 40 % of the field | literal | `InputField::Render` |
| Caret blink | 1 Hz, 50 % duty | literal | `InputField::Render`, `GameMenuInputField::Render`, `ChatInputField::Render`, `AuthInputField::Render` |
| `INTEGER_INCREMENT_PERIOD` | 0.1 s (10 steps/s) | `#define` | `code/interface/input_field.cpp`, `code/game_menu.cpp` |
| Float scroller rate | `change × elapsed × 0.2` per frame | literal | `InputScroller::CheckInput` |
| Scroll speed-up | × 5 (Shift) | literal | `InputScroller::CheckInput` |
| Default numeric bounds | [0, 1.0e4] | default args | `InputField::Register*` (`code/interface/input_field.h`) |
| `DEF_FONT_SIZE` | 12.0 | `#define` | `code/lib/text_renderer.h` |
| `GetTextWidth` default size | 13.0; horizontal scale 0.6 (CJK 1.0) | default arg / `App::SetLanguage` | `code/lib/text_renderer.h`, `code/app.cpp` |
| Font spacing factors | editor 0.91, square 1.03, speccy 1.0 | literal | `code/app.cpp:App::SetLanguage` |
| Text 2D offset | ortho −0.325 px; `DrawText2DSimple` x −3, y −7 | literal | `code/lib/text_renderer.cpp` |
| Scroll-bar arrow height | 18 px | literal | `ScrollBar::Create` |
| Drop-down column threshold (Darwinia) | `0.8 × screenH` | literal | `DropDownMenu::CreateMenu` |
| Drop-down column budget (front end) | `leftH − 3.3 × bH` | literal | `GameMenuDropDown::CreateMenu` |
| Drop-down option pitch (front end) | `1.05 × m_h`, width `0.95 × buttonW/columns` | literal | `GameMenuDropDown::CreateMenu` |
| `MessageDialog` | ≤ 20 lines; `w ≥ 100`; `h = 65 + 12·lines ≥ 85`; button 40×18 at `h − 30` | literal | `code/interface/message_dialog.cpp` |
| `FileDialog` rows | 13 px pitch, 12 px high; `rows = (h − 60)/13` | literal | `FileDialog::Create` |
| Double-click window | 0.2 s | literal | `code/interface/filedialog.cpp`, `code/interface/multiwinia_filedialog.cpp` |
| `MultiwiniaFileDialog` rows | 10, height `0.65 × bH` | literal | `MultiwiniaFileDialog::Create` |
| Check-box geometry | box `0.75 h`, tick inset `0.05 h`, tick `0.65 h`, box at `right − 1.5 h` | literal | `CheckBox::Render`, `GameMenuCheckBox::Render` |
| Layout fractions | `0.26 H`, `0.6 W`, gap `0.05 × vertSplit`, widescreen `W/H > 1.4` → `±0.08 W` | literal | `code/game_menu.cpp:GetPositions` |
| Front-end fonts | large `0.095 H`, med `0.04 H`, small `0.022 H` | literal | `code/game_menu.cpp:GetFontSizes` |
| Front-end buttons | `w = 0.9 × leftW`, `h = 1.3 × med`, `gap = 0.1 × med` | literal | `code/game_menu.cpp:GetButtonSizes` |
| Compact rows | `0.65 × bH` (options, advanced, player, filters, password); `0.66 × bH` (lobby small controls); `0.6 × bH` (browser rows) | literal | setup functions |
| `CreateMenuControl` field height | 30 (callers override) | literal | `GameOptionsWindow::CreateMenuControl` |
| Fill-box shadow width | `0.01 W` | literal | `code/game_menu.cpp:DrawFillBox` |
| Fill-box inner outline inset | 3 px | literal | `DrawFillBox` |
| Logo | aspect 277/194, height `0.8 titleH`, inset `0.05 titleW` | literal | `GameOptionsWindow::Render`, `GameMenuWindow::Render` |
| Title bitmap | width = text width × 1.1, aspect 5.4:1, top at `titleY + 0.1 titleH` | literal | `GameMenuWindow::Render` |
| Error dialogue | `W/3 × H/3`, wrap `1.5 × boxW` | literal | `GameOptionsWindow::RenderErrorDialogue` |
| Menu transition | duration 1.0 s, direction ±1; `fraction = 3.6 f`; inset `0.2 × fraction`; slide `fraction⁶ × W`; alpha `1 − 0.2 × fraction`; edge strip 20 px | literal | `code/renderer.cpp:Renderer::RenderMenuTransition`, `code/game_menu.cpp:GameMenuButton::MouseUp` |
| Front-end fade-in | 0.25 s | literal | `code/game_menu.cpp:GameMenu::CreateMenu` |
| Button birth cascade | slide until `0.6 × y/H` s; flash until `(5/numButtons) × y/H` s; slide `0.1 W` per remaining second; flash alpha ≤ 0.9; pop 0.1 s; ghost 1.3× | literal | `GameMenuButton::Render` |
| Level-row cascade | `1.0 × y/H` s, slide `0.1 W` per remaining second | literal | `code/UI/LevelSelectButtonBase.h:LevelSelectButtonBase::RenderMapName` |
| Scrolling text | hold 1.5 s; 30 px/s; gap `1.2 × font`; heading `1.25×`; MOTD extra hold 5.0 s | literal / member default | `code/UI/ScrollingTextButton.h`, `code/UI/motdButton.h` |
| Backdrop | sprite centre `(0.75 W, 0.7 H)`, half-size `0.3 W + 5 sin t`, rotation 0.2 rad, 10 echoes ×1.02 and (−6,−6) px with alpha `0.2(1 − i/10)`; gear half-size `0.6 W`, `0.2 rad/s`, alpha 0.06, 10 copies +0.02 rad; detail low < 20 fps, high ≥ 60 fps, `k = advanceTime × 0.5`, start 60 | literal | `GameMenuWindow::RenderBigDarwinian` |
| Blink | period 1.0 s, 50 % duty (alpha 1.0 / 0.5) | literal | `code/UI/GameMenuWindow.cpp:RenderRequestingEffect`, `code/UI/GameMenuWindow.cpp:RenderLoadingAchievementsEffect` |
| Connecting dots | 2 steps/s, 0–3 dots | literal | `code/UI/WaitingButton.h` |
| Gamer-tag flash | 1.0 s | literal | `code/UI/GamerTagButton.h` |
| `SERVERS_ON_SCREEN` | 13 | `#define` | `code/game_menu.h` |
| `MAX_GAME_TYPES` | 7 | `#define` | `code/game_menu.h` |
| `GAMETYPE_PROLOGUE`, `GAMETYPE_CAMPAIGN` | 254, 255 | `#define` | `code/game_menu.h` |
| `NumPages` | 38 | enum | `code/UI/GameMenuWindow.h` |
| `AUTHENTICATION_KEYLEN` | 32 (31 visible; dashes after 6, 13, 20, 27) | `#define` | `contrib/systemiv/lib/metaserver/authentication.h`, `code/UI/AuthInputField.h` |
| Player name / server password max | 16 characters | ctor default | `code/game_menu.h:PlayerNameString`, `code/UI/GameMenuWindow.h:ServerPasswordString` |
| Chat message buffer / rename | 255 / ≤ 15 characters | literal | `code/interface/chatinput_window.cpp` |
| In-game chat box | font `0.015 H`; width `0.25 W`; scale `W/1280` (`W/1024` at ≤ 800, 0.75 at ≤ 640) | literal | `ChatInputWindow::ChatInputWindow` |
| Language-table limits | 1024 output chars; 64 marker key; 96 final key | `#define` | `code/lib/language_table.cpp` |
| Old language table lifetime | 5 main-loop iterations | literal | `code/app.cpp:App::SetLanguage` |

Page-level constants of `GameMenuWindow` (routed from P18 §13; page behaviour in [09](09_meta_systems.md)) [C]:

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `MULTIWINIA_MAXPARAMS` | 64 | `#define` | `code/multiwinia.h` |
| `MULTIWINIA_NUM_TEAMCOLOURS` | 8 | `#define` | `code/multiwinia.h` |
| `ASSAULT_ATTACKER_COLOURID` / `ASSAULT_DEFENDER_COLOURID` | 1 / 0 | `#define` | `code/multiwinia.h` |
| Browser rebuild interval | 2.0 s (1.0 s when empty and the WAN list has been received) | literal | `GameMenuWindow::UpdateJoinGamePage` |
| WAN list request interval | 10.0 s (browser and quick match) | literal | `GameMenuWindow::UpdateJoinGamePage`, `GameMenuWindow::UpdateQuickMatchGamePage` |
| Quick-match rebuild interval | 1.0 s | literal | `GameMenuWindow::UpdateQuickMatchGamePage` |
| Manual server prefs | `Server1` … `Server30`; default port pref `NetworkServerPort` = 4000; synthetic max teams 4; mode `i mod 6` | literal | `GameMenuWindow::UpdateJoinGamePage` |
| Auth-box "recent change" window | 2.0 s | literal | `GameMenuWindow::ShowAuthBox` |
| Client lag threshold on leave | 5.0 s since the last server letter | literal | `code/UI/FinishMultiwiniaButton.h:FinishMultiwiniaButton::MouseUp` |
| Achievement row | height `(H/16) × 1.6` ("64 px images"); slide-in 750 px/s; 4 px per scroll step | literal | `GameMenuWindow::SetupAchievementsPage`, `code/UI/AchievementButton.h` |
| Thumbnail / mode image | `0.8 rW × 0.6 rW` (4:3) / `0.8 rW × (2/3)(0.8 rW)` | literal | `GameMenuWindow::SetupMultiplayerPage`, `GameMenuWindow::SetupGameSelectPage` |
| Lobby gamer-tag column | origin `leftY + 6 bH`; `w = 0.45 bW`; `h = 0.75 bH`; step `1.5 h`; kick button at `+1.05 w` | literal | `GameMenuWindow::SetupMultiplayerPage` |
| Lobby small controls | `0.66 × bH` (Advanced button, AI drop-down, map name) | literal | `GameMenuWindow::SetupMultiplayerPage` |
| Browser row | `0.6 × bH` high; `0.9 × titleW` wide; header `0.6 bH` | literal | `GameMenuWindow::SetupJoinGamePage` |
| Browser columns | host `0.05 w`, type `0.30 w`, map `0.40 w`, count `0.90 w`, header players right-aligned at `0.98 w`; player boxes from `count − 80 px`, `(0.66 (h − 8)) × (h − 8)`, 2 px gap | literal | `code/UI/GameServerButton.h:GameServerButton::Render`, `code/UI/ServerTitleInfoButton.h` |
| Map-list columns | name `0.025 w`, time `0.65 w`, players `0.83 w` | literal | `code/UI/LevelSelectButtonBase.h:LevelSelectButtonBase::GetColumnPositions` |
| Lobby chat box | fill alpha 200/255; wrap width `1.75 w`; stops 5 % from the top; line `1.1 × font` | literal | `code/UI/GameMenuWindow.cpp:GameMenuChatButton::Render` |
| Buy-now art | prices aspect 40/300 at `0.6 bW`; box art 149/394 at `0.8 rW` | literal | `GameMenuWindow::SetupBuyMePage` |
| Windowed fallback for the store | 1024 × 768 | literal | `code/UI/UnlockFullGameButton.h:UnlockFullGameButton::MouseUp` |
| Store URL | `http://store.introversion.co.uk` (IP item) | literal | `code/app.cpp:App::GetBuyNowURL` |
| Error overlay (dead) | `W/3 × H/3` centred | literal | `GameMenuWindow::RenderErrorDialogue` |
| Help image (dead) | 600 × 400 px | literal | `GameMenuWindow::SetupHelpPage` |
| Prefs read/written by the front end | `PlayerName`, `PlayerPreferedColour` (default −1), `FilterShowDemos`, `FilterShowIncompatible`, `FilterShowFullGames`, `FilterShowCustomMaps`, `FilterShowPassworded` (default 1), `Server1`…`Server30`, `NetworkServerPort`, `TextLanguage`, `ScreenWindowed`, `ScreenWidth`, `ScreenHeight` | pref keys | `GameMenuWindow::GameMenuWindow`, `SaveFiltersButton`, `ApplyNameButton`, `FancyColourButton`, `UpdateJoinGamePage`, `MOTDButton`, `UnlockFullGameButton` |
| Files touched by the front end | `<profile>authkey`, `<profile>authkey.dev`, `multiwinia_betatesters.txt`, `autopatch.exe`; (dead) `testbedconfig.txt` | literal | `code/UI/AuthInputField.h`, `GameMenuWindow::PasteFromClipboard`, `code/UI/PlayDemoAuthkeyButton.h`, `ReadMultiwiniaBetaTesters`, `code/UI/update_page_buttons.h` |

---

## Open questions and unknowns

| # | Item | Kind | What would resolve it |
|---|---|---|---|
| 28-U1 | Exact on-screen look of "ink" text (`SRC_ALPHA, ONE_MINUS_SRC_COLOR`) over the Darwinia highlight gradient and the fill boxes | [I] (blend arithmetic only) | render a captured frame on the target build (or run the GL path in a reference renderer) |
| 28-U2 | Content of the frame captured by `InitialiseMenuTransition` (`glCopyTexImage2D` from the read buffer during the update phase, i.e. after the previous `Flip`): previous frame or undefined back-buffer contents depending on the driver's swap behaviour | [U] | test on several GL drivers / read `WindowManagerWin32::Flip` pixel-format flags (swap copy vs exchange) |
| 28-U3 | Whether the shipped SKUs include the partial `japanese`/`korean`/`portuguese` language files (160–220 loaded phrases each); if they do, choosing them shows raw phrase ids for most of the UI | [I] design-critical for localisation QA | inspect the retail `language.dat` per SKU (`tools/build_language_dat.bat` + `tools/*language.list`, [31](31_platform_build_and_tools.md)) |
| 28-U4 | Whether committed IME text reaches `InputField` as `WM_CHAR` units (no `WM_IME_*` handling exists) | [I] | run the game with a Japanese/Korean IME |
| 28-U5 | Whether the server rejects lobby option requests from non-host clients (the UI only hides controls, §4.12) | [U] (shared with 09/22) | `code/network/server.cpp` handlers for `NET_DARWINIA_SETGAMEOPTION*` |
| 28-U6 | Consequence of F21 (server-row navigation entries removed at the wrong index) in real browsing sessions | [I] | instrument `UpdateJoinGamePage` or step through with a shrinking list |
| 28-U7 | Whether `EclUpdate` skipping a window when another window is removed during the same iteration (§2.9) ever drops a frame of navigation in practice | [I] | trace `EclUpdate` with drop-down windows that self-remove in `DropDownWindow::Update` |
| 28-U8 | Whether any window in a shipped flow is opened while the Eclipse screen size is stale (screen size is only refreshed in `AdjustWindowPositions`) | [I] | audit every screen-mode change path (`RestartWindowManagerAndRenderer`, `UnlockFullGameButton`, `SetWindowed`) |
| 28-U9 | Whether a demo-key player whose demo limitations forbid hosting can still host: `App::HostGamePermitted()` is consulted only to set the no-op `m_disabled` flag on the Host button (F17), so the UI does not block it | [I] design-critical (demo gating) | check `App::StartMultiPlayerServer` / `ClientToServer::GetDemoLimitations` and the metaserver side ([22](22_determinism_and_networking.md), [09](09_meta_systems.md)) |

## Docs-vs-code discrepancies

| Doc | Says | Code | Ref |
|---|---|---|---|
| `docs/multiwiniachanges-beta.txt` (Issue 650, listed as fixed) | "Game Menu: X360 B button in menus doesn't always go back" | Structural cases remain where B does nothing: pages without a `BackPageButton` (Main, AuthKeyEntry), any frame in which the `BackPageButton` is not rendered (drop-down list open, window unfocused), and `MessageDialog` (no back button) [C mechanism; I that these were the reported cases] | `code/game_menu.cpp:BackPageButton::Render`, `code/interface/message_dialog.cpp` |
| `docs/multiwiniachanges-beta.txt` (Issue 558, listed as fixed) | "Menu navigation in the pre-game with controller" | consistent: the lobby builds `m_buttonOrder` and starts the cursor on Play | `GameMenuWindow::SetupMultiplayerPage` |
| `docs/detailed schedule.txt` "Translation System → Missed out" (Sound Stats/Parameter/Editor/Profile windows, Profile Window, Pokey Window, Network Window, Input Logger, Grabber Window, Gesture Window, Debug Menu, Cheat Window, Camera Anim Window) | these windows are untranslated | consistent: literal English captions (§10.3) | [C] grep of the window captions (§10.3) |
| `docs/RussianFontRequest.txt` | only speccy had Cyrillic glyphs; they were copied to square and editor as a stop-gap | consistent: the three Cyrillic atlas pairs are byte-identical | `data/textures/unicode/*.0410.042f.bmp`, `*.0430.044f.bmp` |
| `docs/sound changes.txt` | "Sound editor will now use a scrollbar if there are too many events to fit" | consistent (`SoundEditorWindow` creates a `ScrollBar`), but the sound editor is [B:DEBUG] only | `code/interface/soundeditor_window.cpp` |

## IP items

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Eclipse" library name and author credit ("Christopher Delay") | name | `contrib/eclipse/*`; control names `EclipseLMouseDown`, `EclipseLMousePressed`, `EclipseRMousePressed` (`data/input/input_preferences.txt`) | `ui-core`; `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown` |
| Class/brand names `DarwiniaWindow`, `DarwiniaButton`, `GameMenuWindow::RenderBigDarwinian`, `MultiwiniaWindow`, `MultiwiniaFileDialog`, `MultiwiniaEditorButton`, `FinishMultiwiniaButton`, `dialog_multiwinia` | name | `code/interface/*`, `code/UI/*` | `UiWindow`, `UiButton`, `RenderBackdrop`, `MapEditorHubPage`, `MapFilePicker` |
| Title "MULTIWINIA" (`multiwinia_mainmenu_title`), subtitle "Survival of the flattest" (`multiwinia_mainmenu_subtitle`), `textures/mwtitle.bmp`, `icons/mwlogo.bmp` | brand / text / art | `GameOptionsWindow::Render`, `GameMenuWindow::Render`, `DarwiniaWindow::Render` | `app.title`, `app.tagline`, new logotype and logo |
| Backdrop art `sprites/darwinian.bmp` (giant Darwinian), `mwhelp/gear.bmp` | character art | `GameMenuWindow::RenderBigDarwinian` | own mascot / abstract backdrop |
| UI textures `textures/interface_red.bmp`, `textures/interface_blur.bmp`, `textures/cloudyglow.bmp`, `textures/menuhighlight.bmp`, `icons/selectionarrow.bmp`, `icons/tick.bmp` | art | §8.4 | new UI texture set |
| Controller glyphs `icons/button_a.bmp`, `icons/button_b.bmp`, `icons/button_lb.bmp`, `icons/button_rb.bmp`, `icons/button_lb_shadow.bmp`, `icons/button_rb_shadow.bmp` (Xbox 360 pad art) | third-party glyph art | `BackPageButton::Render`, `MenuCloseButton::Render`, `GameMenuWindow::PreloadTextures` | platform-guideline glyphs |
| Fonts "speccy", "editor", "square" (`data/textures/unicode/*`, incl. CJK/Hangul glyph art) | art (font) | `code/app.cpp:App::SetLanguage` | licensed open fonts (e.g. an OFL UI font + Noto CJK) via a glyph cache |
| Palettes (red glass panel + steel-blue highlight; green/red front-end captions on blue-glow boxes) | art direction | §8.3 | new palette (keep the token structure) |
| UI sound events `MenuSelect`, `MenuCancel`, `MenuRollOver`, `LobbyAmbience`, `ChatMessage` (`SoundSourceBlueprint::TypeMultiwiniaInterface`) | audio | `code/game_menu.cpp`, `code/interface/mainmenus.h`, `code/UI/GameMenuWindow.cpp` | `ui.select`, `ui.back`, `ui.hover`, `ui.lobbyLoop`, `ui.chat` |
| Framework phrase ids and their English text: `dialog_close`, `dialog_ok`, `dialog_cancel`, `dialog_back`, `dialog_multiplefiles`, `editor_coloureditor`, `editor_red`, `editor_green`, `editor_blue`, `editor_alpha`, `error`, `multiwinia_editor_success`, `multiwinia_editor_selectmap`, `multiwinia_menu_back`, `multiwinia_presstotalk`, `authentication_result`, `dialog_auth_status_demo_user` | text | `data/language/*.txt` | own string table (ids may be reused as neutral keys, texts not) |
| Literal English captions in framework code: "Close", "Close this window", "Value left", "Value right", "New Window", "New Button", "New Scrollbar", "Failed to set requested screen resolution of …", "Failed to set requested sound settings", "Restart Required" / "Please restart Darwinia …" (Linux) | text | `code/interface/darwinia_window.cpp`, `code/interface/input_field.cpp`, `code/game_menu.cpp`, `contrib/eclipse/*`, `code/interface/scrollbar.cpp`, `code/renderer.cpp`, `code/sound/soundsystem.cpp`, `code/interface/prefs_screen_window.cpp` | rewrite |
| Store URL `http://store.introversion.co.uk` (`App::GetBuyNowURL`), update URL / `autopatch.exe` launch | brand / URL | `code/app.cpp`, `code/UI/UnlockFullGameButton.h`, `code/UI/update_page_buttons.h` | own store/updater URLs from configuration |
| Menu page and button strings (every `multiwinia_*` id used by `code/UI`) and window names | text | enumerated in the IP register of [09](09_meta_systems.md); the text of every id is in `data/language/english.txt` | own strings |

## Source map

UI framework and front-end sources (every file in `contrib/eclipse/`, `code/interface/`, `code/UI/`) plus the files that feed them. "Framework" rows are specified in this document; "screen" rows are windows built on the framework and specified in the document named.

| File | Role |
|---|---|
| `contrib/eclipse/eclipse.h` | Eclipse public API (window management, focus, mouse/keyboard pump, dirty rects) — §2 |
| `contrib/eclipse/eclipse.cpp` | Eclipse implementation (`EclUpdateMouse` state machine, z-order, popups, maximise, render order) — §2 |
| `contrib/eclipse/eclwindow.h` | `EclWindow` class — §2.1 |
| `contrib/eclipse/eclwindow.cpp` | `EclWindow` (button registry, auto-grow, render loop, keypress forwarding) — §2 |
| `contrib/eclipse/eclbutton.h` | `EclButton` class — §2.1 |
| `contrib/eclipse/eclbutton.cpp` | `EclButton` (properties, empty virtuals) — §2.1 |
| `contrib/eclipse/eclipse.vcproj` | static-library project linked by `Darwinia.sln` (build evidence) — §1.2 |
| `contrib/eclipse/eclipse.dsp` | VC6 project of the same library, not used by the VS2005 build — §1.2 |
| `code/interface/darwinia_window.h` | `DarwiniaWindow`, `DarwiniaButton`, `BorderlessButton`, `CloseButton`, `GameExitButton`, `InvertedBox`, `LabelButton` — §3 |
| `code/interface/darwinia_window.cpp` | Darwinia layer implementation (chrome, navigation, highlight, value controls, menu scaling) — §3 |
| `code/interface/input_field.h` | `InputField`, `InputScroller`, `ColourWidget`, `ColourWindow` — §4.1–§4.3 |
| `code/interface/input_field.cpp` | text input, validation, scrollers, colour picker — §4.1–§4.3 |
| `code/interface/scrollbar.h` | `ScrollBar`, `ScrollBarButton`, `ScrollChangeButton` — §4.4 |
| `code/interface/scrollbar.cpp` | scroll bar model, thumb drag, paging, arrows — §4.4 |
| `code/interface/checkbox.h` | `CheckBox` — §4.5 |
| `code/interface/checkbox.cpp` | check-box binding and render — §4.5 |
| `code/interface/drop_down_menu.h` | `DropDownOptionData`, `DropDownWindow`, `DropDownMenu`, `DropDownMenuOption` — §4.6 |
| `code/interface/drop_down_menu.cpp` | drop-down options, singleton list window — §4.6 |
| `code/interface/message_dialog.h` | `MessageDialog` — §4.7 |
| `code/interface/message_dialog.cpp` | message box and `OKButton` — §4.7 |
| `code/interface/filedialog.h` | `FileDialog` — §4.8 |
| `code/interface/filedialog.cpp` | file picker rows, selection, double-click — §4.8 |
| `code/interface/multiwinia_filedialog.h` | `MultiwiniaFileDialog` [B:EDITOR] — §4.9 |
| `code/interface/multiwinia_filedialog.cpp` | front-end-styled file picker [B:EDITOR] — §4.9 |
| `code/interface/multiwinia_window.h` | `MultiwiniaWindow`, `NewMapWindow` [B:EDITOR] — §4.9 (functional: [08](08_levels_and_level_design.md)) |
| `code/interface/multiwinia_window.cpp` | editor hub pages using the front-end style [B:EDITOR] — §4.9 |
| `code/interface/mainmenus.h` | `GameOptionsWindow`, `MenuCloseButton`, `HelpAndOptionsButton`, legacy menu classes — §5 |
| `code/interface/mainmenus.cpp` | `GameOptionsWindow` render/update/error dialogue/`CreateMenuControl`; in-game and legacy menus — §5 (functional: [09](09_meta_systems.md)) |
| `code/interface/chatinput_window.h` | `ChatInputWindow`, `ChatInputField` — §4.10 |
| `code/interface/chatinput_window.cpp` | chat text entry, `/name` command — §4.10 |
| `code/interface/helpandoptions_windows.h` | help/option page windows, `TextButton`, `CrateButton` — §5.1, §6.11 (functional: [09](09_meta_systems.md)) |
| `code/interface/helpandoptions_windows.cpp` | help and options pages — screen ([09](09_meta_systems.md)) |
| `code/interface/prefs_screen_window.h` | `PrefsScreenWindow` — screen ([09](09_meta_systems.md)) |
| `code/interface/prefs_screen_window.cpp` | screen options; `RestartWindowManagerAndRenderer`, `AdjustWindowPositions` (resolution change, §7.3) |
| `code/interface/prefs_graphics_window.h` | `PrefsGraphicsWindow` — screen ([09](09_meta_systems.md)) |
| `code/interface/prefs_graphics_window.cpp` | graphics options — screen ([09](09_meta_systems.md)) |
| `code/interface/prefs_other_window.h` | `PrefsOtherWindow`; pref-key defines `OTHER_HELPENABLED`, `OTHER_CONTROLHELPENABLED`, `OTHER_BOOTLOADER`, `OTHER_CHRISTMASENABLED`, `OTHER_LANGUAGE`, `OTHER_DIFFICULTY`, `OTHER_LARGEMENUS`, `OTHER_AUTOMATICCAM`, `OTHER_USEPORTFORWARDING`, `OTHER_SERVERPORT`, `OTHER_CLIENTPORT` — §3.2, §10.7 |
| `code/interface/prefs_other_window.cpp` | other options; language switch and large-menu apply — §10.7 |
| `code/interface/prefs_sound_window.h` | `PrefsSoundWindow` — screen ([27](27_audio_engine.md)) |
| `code/interface/prefs_sound_window.cpp` | sound options — screen ([27](27_audio_engine.md)) |
| `code/interface/prefs_keybindings_window.h` | `PrefsKeybindingsWindow` [B:LEGACY] — screen ([09](09_meta_systems.md)) |
| `code/interface/prefs_keybindings_window.cpp` | key-binding window, `ControlMethodDropDownMenu` [B:LEGACY] |
| `code/interface/controllerunplugged_window.h` | `ControllerUnpluggedWindow` — screen ([06](06_player_control_and_input.md), [09](09_meta_systems.md)) |
| `code/interface/controllerunplugged_window.cpp` | controller-unplugged page |
| `code/interface/network_window.h` | `NetworkWindow` (F9) — screen ([32](32_debug_test_and_performance.md)) |
| `code/interface/network_window.cpp` | network statistics window (Darwinia style, uses `CloseButton`, `InvertedBox`) |
| `code/interface/debugmenu.h` | `DebugMenu`, `DebugKeyBindings` — screen ([32](32_debug_test_and_performance.md)) |
| `code/interface/debugmenu.cpp` | debug "Tools Menu" (unreachable) and hot-key openers |
| `code/interface/cheat_window.h` | `CheatWindow` [B:DEBUG] — screen ([32](32_debug_test_and_performance.md)) |
| `code/interface/cheat_window.cpp` | cheat buttons [B:DEBUG] |
| `code/interface/profilewindow.h` | `ProfileWindow` [B:DEBUG] — screen ([32](32_debug_test_and_performance.md)) |
| `code/interface/profilewindow.cpp` | CPU profiler window [B:DEBUG] |
| `code/interface/grabber_window.h` | `GrabberWindow` [B:DEAD] (DEBUG-only, never instantiated) — screen ([32](32_debug_test_and_performance.md)) |
| `code/interface/grabber_window.cpp` | AVI capture window [B:DEAD] (DEBUG-only, never instantiated) |
| `code/interface/gesture_window.h` | `GestureWindow` [B:DEAD] |
| `code/interface/gesture_window.cpp` | gesture editor [B:DEAD] |
| `code/interface/soundeditor_window.h` | sound editor windows [B:DEBUG] — screen ([27](27_audio_engine.md)) |
| `code/interface/soundeditor_window.cpp` | sound editor (uses `FileDialog`, `ScrollBar`, `DropDownMenu`) [B:DEBUG] |
| `code/interface/soundparameter_window.h` | `SoundParameterEditor` [B:DEBUG] — screen ([27](27_audio_engine.md)) |
| `code/interface/soundparameter_window.cpp` | sound parameter graph editor [B:DEBUG] |
| `code/interface/soundstats_window.h` | `SoundStatsWindow` [B:DEBUG] — screen ([27](27_audio_engine.md)) |
| `code/interface/soundstats_window.cpp` | sound statistics (uses `ScrollBar`) [B:DEBUG] |
| `code/interface/sound_profile_window.h` | `SoundProfileWindow` [B:DEBUG] — screen ([27](27_audio_engine.md)) |
| `code/interface/sound_profile_window.cpp` | sound profiler [B:DEBUG] |
| `code/interface/editor_window.h` | `MainEditWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/editor_window.cpp` | editor main window [B:EDITOR] |
| `code/interface/buildings_window.h` | building editor windows [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/buildings_window.cpp` | building editor controls [B:EDITOR] |
| `code/interface/camera_anim_window.h` | camera-animation editor windows [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/camera_anim_window.cpp` | camera-animation editor [B:EDITOR] |
| `code/interface/camera_mount_window.h` | `CameraMountEditWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/camera_mount_window.cpp` | camera-mount editor [B:EDITOR] |
| `code/interface/landscape_window.h` | landscape editor windows [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/landscape_window.cpp` | landscape editor (file dialogs, guide grid) [B:EDITOR] |
| `code/interface/lights_window.h` | `LightsEditWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/lights_window.cpp` | lights editor (`LabelButton`) [B:EDITOR] |
| `code/interface/instant_unit_window.h` | instant-unit editor windows [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/instant_unit_window.cpp` | instant-unit editor [B:EDITOR] |
| `code/interface/tree_window.h` | `TreeWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/tree_window.cpp` | tree editor [B:EDITOR] |
| `code/interface/invalidcrates_window.h` | `InvalidCratesWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/invalidcrates_window.cpp` | invalid-crate editor [B:EDITOR] |
| `code/interface/multiwiniaoptions_window.h` | `MultiwiniaOptionsWindow` [B:EDITOR] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/multiwiniaoptions_window.cpp` | editor map options [B:EDITOR] |
| `code/interface/globalworldeditor_window.h` | `GlobalWorldEditorWindow` [B:LEGACY] — screen ([08](08_levels_and_level_design.md)) |
| `code/interface/globalworldeditor_window.cpp` | Darwinia world editor [B:LEGACY] |
| `code/interface/leaderboards_window.h` | leaderboard windows [B:LEGACY] — screen ([09](09_meta_systems.md)) |
| `code/interface/leaderboards_window.cpp` | leaderboard stub [B:LEGACY] |
| `code/interface/userprofile_window.h` | profile windows [B:LEGACY] — screen ([09](09_meta_systems.md)) |
| `code/interface/userprofile_window.cpp` | profile windows [B:LEGACY] |
| `code/interface/reallyquit_window.h` | `ReallyQuitWindow` [B:LEGACY] |
| `code/interface/reallyquit_window.cpp` | quit confirmation (`GameExitButton`) [B:LEGACY] |
| `code/interface/save_on_quit_window.h` | `SaveOnQuitWindow` [B:LEGACY] |
| `code/interface/save_on_quit_window.cpp` | sound-editor save prompt [B:LEGACY] |
| `code/interface/demoend_window.h` | `DemoEndWindow` [B:LEGACY] |
| `code/interface/demoend_window.cpp` | Darwinia demo end page (needs `DEMO2`) [B:LEGACY] |
| `code/interface/mods_window.h` | mod windows [B:DEAD] |
| `code/interface/mods_window.cpp` | mod system UI (`USE_DARWINIA_MOD_SYSTEM`) [B:DEAD] |
| `code/interface/buynow_window.h` | `BuyNowWindow` (not in project) [B:DEAD] |
| `code/interface/buynow_window.cpp` | buy-now window (not in project) [B:DEAD] |
| `code/interface/updateavailable_window.h` | `UpdateAvailableWindow` (not in project) [B:DEAD] |
| `code/interface/updateavailable_window.cpp` | update window (not in project) [B:DEAD] |
| `code/game_menu.h` | `GameMenu`, `PlayerNameString`, `GameMenu*` widgets, layout helper declarations, `IS_DEMO`, `SERVERS_ON_SCREEN` — §5, §7 |
| `code/game_menu.cpp` | layout helpers, `DrawFillBox`, `RenderHighlightBeam`, `GameMenuButton`/`BackPageButton`/title/drop-down/input/check-box widgets, `s_pageNames`, `GameMenu` lifecycle — §5–§7 |
| `code/UI/GameMenuWindow.h` | `GameMenuWindow` (page enum, members), `ServerPasswordString` — §6 |
| `code/UI/GameMenuWindow.cpp` | page state machine, set-up/update/render of every page, `RenderBigDarwinian`, `GameMenuChatButton` — §6 (functional: [09](09_meta_systems.md)) |
| `code/UI/ImageButton.h` | `ImageButton` — §6.9 |
| `code/UI/ImageButton.cpp` | image widget render — §6.9 |
| `code/UI/MapData.h` | `MapData` record — §6.12 ([09](09_meta_systems.md)) |
| `code/UI/MapData.cpp` | `MapData` — §6.12 |
| `code/UI/ScrollingTextButton.h` | scrolling text widget — §6.8 |
| `code/UI/FancyColourButton.h` | colour/side picker — §6.10 |
| `code/UI/AuthInputField.h` | key-entry field — §4.11 |
| `code/UI/GameServerButton.h` | server-browser row, `SpecialScroll` — §6.11 |
| `code/UI/LevelSelectButtonBase.h` | map-row base, cascade — §6.7, §6.11 |
| `code/UI/LevelSelectButton.h` | map row — §6.12 |
| `code/UI/AnyLevelSelectButton.h` | "any map" row (unreached) — §6.12 |
| `code/UI/GamerTagButton.h` | lobby slot — §6.11 |
| `code/UI/KickPlayerButton.h` | kick button — §6.11 |
| `code/UI/WaitingButton.h` | connection status — §6.11 |
| `code/UI/GameTypeImageButton.h` | mode picture — §6.9 |
| `code/UI/MapImageButton.h` | map thumbnail — §6.9 |
| `code/UI/HelpImageButton.h` | help picture (unreached) — §6.9 |
| `code/UI/GameTypeInfoButton.h` | mode blurb (`ScrollingTextButton`) — §6.8 |
| `code/UI/MapInfoButton.h` | map description (`ScrollingTextButton`) — §6.8 |
| `code/UI/OptionDetailsButton.h` | option help (`ScrollingTextButton`) — §6.8 |
| `code/UI/motdButton.h` | `MOTDButton` (`ScrollingTextButton`) — §6.8 |
| `code/UI/MapListTitlesButton.h` | map-list header — §6.12 |
| `code/UI/ServerTitleInfoButton.h` | browser header — §6.12 |
| `code/UI/MapSelectModeButton.h` | lobby map name — §6.12 |
| `code/UI/PlayGameButton.h` | Play/Ready — §6.12 |
| `code/UI/GameTypeButton.h` | mode row — §6.12 |
| `code/UI/AdvancedOptionsModeButton.h` | → AdvancedOptions — §6.12 |
| `code/UI/ApplyNameButton.h` | save name, back — §6.12 |
| `code/UI/AuthPageButton.h` | → AuthKeyEntry — §6.12 |
| `code/UI/AuthStatusButton.h` | auth status box — §6.12 |
| `code/UI/BuyNowModeButton.h` | → BuyMeNow — §6.12 |
| `code/UI/BuyNowInfoButton.h` | upsell lines (unused) — §6.12 |
| `code/UI/ConnectButton.h` | `ServerConnectButton` — §6.12 |
| `code/UI/DarwiniaModeButton.h` | "Single Player" — §6.12 |
| `code/UI/EnterAuthKeyButton.h` | auth Done — §6.12 |
| `code/UI/FiltersPageButton.h` | → Filters — §6.12 |
| `code/UI/FinishMultiwiniaButton.h` | `FinishMultiwiniaButton`, `FinishSinglePlayerButton`, `SaveFiltersButton` — §6.12 |
| `code/UI/JoinGameButton.h` | → JoinGame — §6.12 |
| `code/UI/NewOrJoinButton.h` | "Multiplayer" — §6.12 |
| `code/UI/NewServerButton.h` | "Host" — §6.12 |
| `code/UI/PasteKeyButton.h` | paste key — §6.12 |
| `code/UI/PlayDemoAuthkeyButton.h` | demo key — §6.12 |
| `code/UI/QuitButton.h` | quit — §6.12 |
| `code/UI/TutorialButton.h` | `TutorialPageButton`, `TutorialButton` — §6.6, §6.12 |
| `code/UI/UnlockFullGameButton.h` | open store — §6.12 |
| `code/UI/credits_button.h` | → Credits — §6.12 |
| `code/UI/update_page_buttons.h` | update-page buttons — §6.12 |
| `code/UI/AchievementButton.h` | achievement row (unused) — §6.12 |
| `code/UI/CSpectatorButton.h` | spectator request (unused, `TESTBED`) — §6.12 |
| `code/UI/CampaignButton.h` | Darwinia campaign [B:DEBUG] — §6.12 |
| `code/UI/PrologueButton.h` | Darwinia prologue [B:DEBUG] — §6.12 |
| `code/UI/MultiwiniaVersusCpuButton.h` | versus CPU [B:DEBUG] — §6.12 |
| `code/UI/MultiwiniaEditorButton.h` | open editor [B:EDITOR] — §6.12 |
| `code/UI/DarwiniaPlusMPButton.h` | Darwinia+ MP (unused) — §6.12 |
| `code/UI/DarwiniaPlusSPButton.h` | Darwinia+ SP (unused) — §6.12 |
| `code/UI/GameTypeAnyButton.h` | "any mode" (unused) — §6.12 |
| `code/UI/HelpSelectButton.h` | help select (unused) — §6.12 |
| `code/UI/NextHelpButton.h` | next help image (unreached) — §6.12 |
| `code/UI/PrevHelpButton.h` | previous help image (unreached) — §6.12 |
| `code/UI/QuickMatchButton.h` | quick match (unused) — §6.12 |
| `code/UI/QuitAuthKeyButton.h` | quit from key page (other targets) — §6.12 |
| `code/UI/TestBedButtons.h` | `StartTestbedButton` [B:DEAD] — §6.12 |
| `code/UI/GameMenuOptionsButton.h` | options button (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/HelpModeButton.h` | help mode (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/MultiwiniaModeButton.h` | multiplayer mode (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/QuickStartButton.h` | quick start (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/RequestAIButton.h` | add CPU (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/ResearchModeButton.h` | research (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/RollingDemoButton.h` | rolling demo (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/mapDetailsButton.h` | map details panel (uncompiled) [B:DEAD] — §6.12 |
| `code/UI/ErrorDialog.h` | `ErrorDialog` (not in project) [B:DEAD] — §6.12 |
| `code/UI/ErrorDialog.cpp` | `ErrorDialog` (not in project) [B:DEAD] — §6.12 |
| `code/UI/soundsliderbutton.h` | `VolumeControl` (not in project) [B:DEAD] — §6.12 |
| `code/user_input.h` | `UserInput` — §1.4 |
| `code/user_input.cpp` | Eclipse pump (`AdvanceMenus`), UI render pass, F9/F11 openers — §1.4 |
| `code/lib/input/inputdriver_win32.cpp` | `WM_CHAR` → `EclUpdateKeyboard` — §1.4 |
| `code/lib/input/keydefs.h` | `KEY_BACKSPACE`, `KEY_ENTER`, `KEY_ESC`, `KEY_STOP`, `KEY_0..9`, `KEY_A..Z` used by text input — §4.1 |
| `code/lib/input/file_paths.cpp` | input-preference file selection (`MultiwiniaControlMethod`) — §9.1 |
| `code/lib/window_manager_win32.cpp` | Unicode window class (UTF-16 `WM_CHAR`) — §1.4 |
| `code/lib/text_renderer.h` | `TextRenderer` API, `DEF_FONT_SIZE` — §8.2 |
| `code/lib/text_renderer.cpp` | 2D ortho, text blend modes, width estimates — §8.2 |
| `code/lib/language_table.h` | `LANGUAGEPHRASE`, `ISLANGUAGEPHRASE`, `RAWLANGUAGEPHRASE` — §10.1 |
| `code/lib/language_table.cpp` | phrase lookup, key fallback, input-mode tables, markup — §10.1–§10.2 |
| `code/app.cpp` | fonts per language, `SetLanguage`, `InitLanguage`, large-menu flag — §3.2, §10 |
| `code/main.cpp` | `MainMenuLoop`, `RemoveAllWindows`, `WindowsOnScreen`, `Finalise` (`EclShutdown`) — §1.4, §6.1 |
| `code/renderer.cpp` | UI pass position, menu transition, `Clip`/`EndClip`, resolution-failure `MessageDialog` — §1.4, §6.7 |
| `code/network/networkvalue.h` | `NetworkValue` adaptor family — §4.12 |
| `code/network/networkvalue.cpp` | text conversions, networked option `Set` — §4.12 |
| `code/sound/soundsystem.cpp` | hardware-3D failure `MessageDialog` — §4.7 |
| `contrib/systemiv/lib/tosser/llist.h` | `LList` used by every window/button list — §1.2 |
| `contrib/systemiv/lib/tosser/llist.cpp` | `LList::GetItem` NULL on invalid index — §1.2 |
| `contrib/systemiv/lib/unicode/unicode_string.h` | `UnicodeString` — §10.4 |
| `contrib/systemiv/lib/unicode/unicode_string.cpp` | `Truncate`, `ReplaceStringFlag`, case mapping — §4.1, §10.4 |
| `contrib/systemiv/lib/math/random_number.h` | `AppRandom`, `APP_RAND_MAX` (window placement) — §2.2 |
| `contrib/systemiv/lib/math/random_number.cpp` | `AppRandom` implementation — §2.2 |
| `contrib/systemiv/lib/metaserver/authentication.h` | `AUTHENTICATION_KEYLEN`, key status — §4.11 |
| `contrib/systemiv/interface/components/checkbox.h` | SystemIV UI component — not compiled [B:DEAD] (§1.2) |
| `contrib/systemiv/interface/components/checkbox.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/core.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/core.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/drop_down_menu.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/drop_down_menu.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/filedialog.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/filedialog.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/inputfield.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/inputfield.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/message_dialog.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/message_dialog.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/scrollbar.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/components/scrollbar.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/options/screenoptions_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/options/screenoptions_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/options/soundoptions_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/options/soundoptions_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundeditor_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundeditor_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundparameter_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundparameter_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundstats_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/soundsystem/soundstats_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/styleeditor_window.h` | not compiled [B:DEAD] |
| `contrib/systemiv/interface/styleeditor_window.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/lib/eclipse/eclipse.h` | SystemIV copy of Eclipse — not compiled [B:DEAD] (§1.2) |
| `contrib/systemiv/lib/eclipse/eclipse.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/lib/eclipse/eclwindow.h` | not compiled [B:DEAD] |
| `contrib/systemiv/lib/eclipse/eclwindow.cpp` | not compiled [B:DEAD] |
| `contrib/systemiv/lib/eclipse/eclbutton.h` | not compiled [B:DEAD] |
| `contrib/systemiv/lib/eclipse/eclbutton.cpp` | not compiled [B:DEAD] |
| `targets/vs2005/Darwinia.sln` | links the `eclipse` project — §1.2 |
| `targets/vs2005/Darwinia.vcproj` | game project file list and include path (build evidence) — §1.2 |
| `data/input/input_preferences.txt` | menu/Eclipse bindings — §9.1 |
| `data/input/input_preferences_darwinia.txt` | alternative bindings with keyboard menu navigation (unused) — §9.1 |
| `data/language/english.txt` | phrase table (2425 keys) — §10 |
| `data/language/french.txt` | phrase table — §10.5 |
| `data/language/german.txt` | phrase table — §10.5 |
| `data/language/italian.txt` | phrase table — §10.5 |
| `data/language/spanish.txt` | phrase table — §10.5 |
| `data/language/russian.txt` | phrase table — §10.5 |
| `data/language/japanese.txt` | partial phrase table (221 keys) — §10.5 |
| `data/language/korean.txt` | partial phrase table (161 keys) — §10.5 |
| `data/language/portuguese.txt` | partial phrase table (161 keys) — §10.5 |
| `data/textures/interface_red.bmp` | Darwinia window body — §8.4 |
| `data/textures/interface_blur.bmp` | fill-box body — §8.4 |
| `data/textures/cloudyglow.bmp` | fill-box glow — §8.4 |
| `data/textures/menuhighlight.bmp` | highlight beam — §8.4 |
| `data/textures/mwtitle.bmp` | title bitmap — §8.4 |
| `data/icons/mwlogo.bmp` | title-bar logo — §8.4 |
| `data/icons/selectionarrow.bmp` | scroll arrows — §8.4 |
| `data/icons/tick.bmp` | check-box tick — §8.4 |
| `data/icons/button_a.bmp` | pad glyph (pre-loaded) — §8.4 |
| `data/icons/button_b.bmp` | pad "B" back hint — §8.4 |
| `data/icons/button_lb.bmp` | pad glyph (pre-loaded) — §6.1 |
| `data/icons/button_rb.bmp` | pad glyph (pre-loaded) — §6.1 |
| `data/icons/button_lb_shadow.bmp` | pad glyph shadow (pre-loaded) — §6.1 |
| `data/icons/button_rb_shadow.bmp` | pad glyph shadow (pre-loaded) — §6.1 |
| `data/sprites/darwinian.bmp` | backdrop sprite — §6.7 |
| `data/mwhelp/gear.bmp` | backdrop gear — §6.7 |
| `docs/RussianFontRequest.txt` | Cyrillic atlas note (evidence) — §10.5 |
| `docs/multiwiniachanges-beta.txt` | menu-navigation change notes (evidence) — discrepancies |
| `docs/detailed schedule.txt` | untranslated-window list (evidence) — discrepancies |

Glyph atlases (`data/textures/unicode/*`, 138 files) feed all UI text; they are catalogued one row per file in [24](24_rendering.md) / [25](25_textures_and_assets.md) and are listed below for completeness:

| File | Role |
|---|---|
| `data/textures/unicode/editor.0020.00ff.bmp` | glyph atlas `editor` U+0020–U+00FF |
| `data/textures/unicode/editor.0410.042f.bmp` | glyph atlas `editor` U+0410–U+042F |
| `data/textures/unicode/editor.0430.044f.bmp` | glyph atlas `editor` U+0430–U+044F |
| `data/textures/unicode/editor.20a0.20af.bmp` | glyph atlas `editor` U+20A0–U+20AF |
| `data/textures/unicode/speccy.0020.00ff.bmp` | glyph atlas `speccy` U+0020–U+00FF |
| `data/textures/unicode/speccy.0410.042f.bmp` | glyph atlas `speccy` U+0410–U+042F |
| `data/textures/unicode/speccy.0430.044f.bmp` | glyph atlas `speccy` U+0430–U+044F |
| `data/textures/unicode/speccy.20a0.20af.bmp` | glyph atlas `speccy` U+20A0–U+20AF |
| `data/textures/unicode/square.0020.00ff.bmp` | glyph atlas `square` U+0020–U+00FF |
| `data/textures/unicode/square.0410.042f.bmp` | glyph atlas `square` U+0410–U+042F |
| `data/textures/unicode/square.0430.044f.bmp` | glyph atlas `square` U+0430–U+044F |
| `data/textures/unicode/square.20a0.20af.bmp` | glyph atlas `square` U+20A0–U+20AF |
| `data/textures/unicode/square.3000.30ff.bmp` | glyph atlas `square` U+3000–U+30FF |
| `data/textures/unicode/square.4e00.4eff.bmp` | glyph atlas `square` U+4E00–U+4EFF |
| `data/textures/unicode/square.4f00.4fff.bmp` | glyph atlas `square` U+4F00–U+4FFF |
| `data/textures/unicode/square.5000.50ff.bmp` | glyph atlas `square` U+5000–U+50FF |
| `data/textures/unicode/square.5100.51ff.bmp` | glyph atlas `square` U+5100–U+51FF |
| `data/textures/unicode/square.5200.52ff.bmp` | glyph atlas `square` U+5200–U+52FF |
| `data/textures/unicode/square.5300.53ff.bmp` | glyph atlas `square` U+5300–U+53FF |
| `data/textures/unicode/square.5400.54ff.bmp` | glyph atlas `square` U+5400–U+54FF |
| `data/textures/unicode/square.5500.55ff.bmp` | glyph atlas `square` U+5500–U+55FF |
| `data/textures/unicode/square.5600.56ff.bmp` | glyph atlas `square` U+5600–U+56FF |
| `data/textures/unicode/square.5700.57ff.bmp` | glyph atlas `square` U+5700–U+57FF |
| `data/textures/unicode/square.5800.58ff.bmp` | glyph atlas `square` U+5800–U+58FF |
| `data/textures/unicode/square.5900.59ff.bmp` | glyph atlas `square` U+5900–U+59FF |
| `data/textures/unicode/square.5a00.5aff.bmp` | glyph atlas `square` U+5A00–U+5AFF |
| `data/textures/unicode/square.5b00.5bff.bmp` | glyph atlas `square` U+5B00–U+5BFF |
| `data/textures/unicode/square.5c00.5cff.bmp` | glyph atlas `square` U+5C00–U+5CFF |
| `data/textures/unicode/square.5d00.5dff.bmp` | glyph atlas `square` U+5D00–U+5DFF |
| `data/textures/unicode/square.5e00.5eff.bmp` | glyph atlas `square` U+5E00–U+5EFF |
| `data/textures/unicode/square.5f00.5fff.bmp` | glyph atlas `square` U+5F00–U+5FFF |
| `data/textures/unicode/square.6000.60ff.bmp` | glyph atlas `square` U+6000–U+60FF |
| `data/textures/unicode/square.6100.61ff.bmp` | glyph atlas `square` U+6100–U+61FF |
| `data/textures/unicode/square.6200.62ff.bmp` | glyph atlas `square` U+6200–U+62FF |
| `data/textures/unicode/square.6300.63ff.bmp` | glyph atlas `square` U+6300–U+63FF |
| `data/textures/unicode/square.6400.64ff.bmp` | glyph atlas `square` U+6400–U+64FF |
| `data/textures/unicode/square.6500.65ff.bmp` | glyph atlas `square` U+6500–U+65FF |
| `data/textures/unicode/square.6600.66ff.bmp` | glyph atlas `square` U+6600–U+66FF |
| `data/textures/unicode/square.6700.67ff.bmp` | glyph atlas `square` U+6700–U+67FF |
| `data/textures/unicode/square.6800.68ff.bmp` | glyph atlas `square` U+6800–U+68FF |
| `data/textures/unicode/square.6900.69ff.bmp` | glyph atlas `square` U+6900–U+69FF |
| `data/textures/unicode/square.6a00.6aff.bmp` | glyph atlas `square` U+6A00–U+6AFF |
| `data/textures/unicode/square.6b00.6bff.bmp` | glyph atlas `square` U+6B00–U+6BFF |
| `data/textures/unicode/square.6c00.6cff.bmp` | glyph atlas `square` U+6C00–U+6CFF |
| `data/textures/unicode/square.6d00.6dff.bmp` | glyph atlas `square` U+6D00–U+6DFF |
| `data/textures/unicode/square.6e00.6eff.bmp` | glyph atlas `square` U+6E00–U+6EFF |
| `data/textures/unicode/square.6f00.6fff.bmp` | glyph atlas `square` U+6F00–U+6FFF |
| `data/textures/unicode/square.7000.70ff.bmp` | glyph atlas `square` U+7000–U+70FF |
| `data/textures/unicode/square.7100.71ff.bmp` | glyph atlas `square` U+7100–U+71FF |
| `data/textures/unicode/square.7200.72ff.bmp` | glyph atlas `square` U+7200–U+72FF |
| `data/textures/unicode/square.7300.73ff.bmp` | glyph atlas `square` U+7300–U+73FF |
| `data/textures/unicode/square.7400.74ff.bmp` | glyph atlas `square` U+7400–U+74FF |
| `data/textures/unicode/square.7500.75ff.bmp` | glyph atlas `square` U+7500–U+75FF |
| `data/textures/unicode/square.7600.76ff.bmp` | glyph atlas `square` U+7600–U+76FF |
| `data/textures/unicode/square.7700.77ff.bmp` | glyph atlas `square` U+7700–U+77FF |
| `data/textures/unicode/square.7800.78ff.bmp` | glyph atlas `square` U+7800–U+78FF |
| `data/textures/unicode/square.7900.79ff.bmp` | glyph atlas `square` U+7900–U+79FF |
| `data/textures/unicode/square.7a00.7aff.bmp` | glyph atlas `square` U+7A00–U+7AFF |
| `data/textures/unicode/square.7b00.7bff.bmp` | glyph atlas `square` U+7B00–U+7BFF |
| `data/textures/unicode/square.7c00.7cff.bmp` | glyph atlas `square` U+7C00–U+7CFF |
| `data/textures/unicode/square.7d00.7dff.bmp` | glyph atlas `square` U+7D00–U+7DFF |
| `data/textures/unicode/square.7e00.7eff.bmp` | glyph atlas `square` U+7E00–U+7EFF |
| `data/textures/unicode/square.7f00.7fff.bmp` | glyph atlas `square` U+7F00–U+7FFF |
| `data/textures/unicode/square.8000.80ff.bmp` | glyph atlas `square` U+8000–U+80FF |
| `data/textures/unicode/square.8100.81ff.bmp` | glyph atlas `square` U+8100–U+81FF |
| `data/textures/unicode/square.8200.82ff.bmp` | glyph atlas `square` U+8200–U+82FF |
| `data/textures/unicode/square.8300.83ff.bmp` | glyph atlas `square` U+8300–U+83FF |
| `data/textures/unicode/square.8400.84ff.bmp` | glyph atlas `square` U+8400–U+84FF |
| `data/textures/unicode/square.8500.85ff.bmp` | glyph atlas `square` U+8500–U+85FF |
| `data/textures/unicode/square.8600.86ff.bmp` | glyph atlas `square` U+8600–U+86FF |
| `data/textures/unicode/square.8700.87ff.bmp` | glyph atlas `square` U+8700–U+87FF |
| `data/textures/unicode/square.8800.88ff.bmp` | glyph atlas `square` U+8800–U+88FF |
| `data/textures/unicode/square.8900.89ff.bmp` | glyph atlas `square` U+8900–U+89FF |
| `data/textures/unicode/square.8a00.8aff.bmp` | glyph atlas `square` U+8A00–U+8AFF |
| `data/textures/unicode/square.8b00.8bff.bmp` | glyph atlas `square` U+8B00–U+8BFF |
| `data/textures/unicode/square.8c00.8cff.bmp` | glyph atlas `square` U+8C00–U+8CFF |
| `data/textures/unicode/square.8d00.8dff.bmp` | glyph atlas `square` U+8D00–U+8DFF |
| `data/textures/unicode/square.8e00.8eff.bmp` | glyph atlas `square` U+8E00–U+8EFF |
| `data/textures/unicode/square.8f00.8fff.bmp` | glyph atlas `square` U+8F00–U+8FFF |
| `data/textures/unicode/square.9000.90ff.bmp` | glyph atlas `square` U+9000–U+90FF |
| `data/textures/unicode/square.9100.91ff.bmp` | glyph atlas `square` U+9100–U+91FF |
| `data/textures/unicode/square.9200.92ff.bmp` | glyph atlas `square` U+9200–U+92FF |
| `data/textures/unicode/square.9300.93ff.bmp` | glyph atlas `square` U+9300–U+93FF |
| `data/textures/unicode/square.9400.94ff.bmp` | glyph atlas `square` U+9400–U+94FF |
| `data/textures/unicode/square.9500.95ff.bmp` | glyph atlas `square` U+9500–U+95FF |
| `data/textures/unicode/square.9600.96ff.bmp` | glyph atlas `square` U+9600–U+96FF |
| `data/textures/unicode/square.9700.97ff.bmp` | glyph atlas `square` U+9700–U+97FF |
| `data/textures/unicode/square.9800.98ff.bmp` | glyph atlas `square` U+9800–U+98FF |
| `data/textures/unicode/square.9900.99ff.bmp` | glyph atlas `square` U+9900–U+99FF |
| `data/textures/unicode/square.9a00.9aff.bmp` | glyph atlas `square` U+9A00–U+9AFF |
| `data/textures/unicode/square.9b00.9bff.bmp` | glyph atlas `square` U+9B00–U+9BFF |
| `data/textures/unicode/square.9c00.9cff.bmp` | glyph atlas `square` U+9C00–U+9CFF |
| `data/textures/unicode/square.9d00.9dff.bmp` | glyph atlas `square` U+9D00–U+9DFF |
| `data/textures/unicode/square.9e00.9eff.bmp` | glyph atlas `square` U+9E00–U+9EFF |
| `data/textures/unicode/square.ac00.acff.bmp` | glyph atlas `square` U+AC00–U+ACFF |
| `data/textures/unicode/square.ad00.adff.bmp` | glyph atlas `square` U+AD00–U+ADFF |
| `data/textures/unicode/square.ae00.aeff.bmp` | glyph atlas `square` U+AE00–U+AEFF |
| `data/textures/unicode/square.af00.afff.bmp` | glyph atlas `square` U+AF00–U+AFFF |
| `data/textures/unicode/square.b000.b0ff.bmp` | glyph atlas `square` U+B000–U+B0FF |
| `data/textures/unicode/square.b100.b1ff.bmp` | glyph atlas `square` U+B100–U+B1FF |
| `data/textures/unicode/square.b200.b2ff.bmp` | glyph atlas `square` U+B200–U+B2FF |
| `data/textures/unicode/square.b300.b3ff.bmp` | glyph atlas `square` U+B300–U+B3FF |
| `data/textures/unicode/square.b400.b4ff.bmp` | glyph atlas `square` U+B400–U+B4FF |
| `data/textures/unicode/square.b500.b5ff.bmp` | glyph atlas `square` U+B500–U+B5FF |
| `data/textures/unicode/square.b600.b6ff.bmp` | glyph atlas `square` U+B600–U+B6FF |
| `data/textures/unicode/square.b700.b7ff.bmp` | glyph atlas `square` U+B700–U+B7FF |
| `data/textures/unicode/square.b800.b8ff.bmp` | glyph atlas `square` U+B800–U+B8FF |
| `data/textures/unicode/square.b900.b9ff.bmp` | glyph atlas `square` U+B900–U+B9FF |
| `data/textures/unicode/square.ba00.baff.bmp` | glyph atlas `square` U+BA00–U+BAFF |
| `data/textures/unicode/square.bb00.bfff.bmp` | glyph atlas `square` U+BB00–U+BFFF |
| `data/textures/unicode/square.bc00.bcff.bmp` | glyph atlas `square` U+BC00–U+BCFF |
| `data/textures/unicode/square.bd00.bdff.bmp` | glyph atlas `square` U+BD00–U+BDFF |
| `data/textures/unicode/square.be00.beff.bmp` | glyph atlas `square` U+BE00–U+BEFF |
| `data/textures/unicode/square.bf00.bfff.bmp` | glyph atlas `square` U+BF00–U+BFFF |
| `data/textures/unicode/square.c000.c0ff.bmp` | glyph atlas `square` U+C000–U+C0FF |
| `data/textures/unicode/square.c100.c1ff.bmp` | glyph atlas `square` U+C100–U+C1FF |
| `data/textures/unicode/square.c200.c2ff.bmp` | glyph atlas `square` U+C200–U+C2FF |
| `data/textures/unicode/square.c300.c3ff.bmp` | glyph atlas `square` U+C300–U+C3FF |
| `data/textures/unicode/square.c400.c4ff.bmp` | glyph atlas `square` U+C400–U+C4FF |
| `data/textures/unicode/square.c500.c5ff.bmp` | glyph atlas `square` U+C500–U+C5FF |
| `data/textures/unicode/square.c600.c6ff.bmp` | glyph atlas `square` U+C600–U+C6FF |
| `data/textures/unicode/square.c700.c7ff.bmp` | glyph atlas `square` U+C700–U+C7FF |
| `data/textures/unicode/square.c800.c8ff.bmp` | glyph atlas `square` U+C800–U+C8FF |
| `data/textures/unicode/square.c900.c9ff.bmp` | glyph atlas `square` U+C900–U+C9FF |
| `data/textures/unicode/square.ca00.caff.bmp` | glyph atlas `square` U+CA00–U+CAFF |
| `data/textures/unicode/square.cb00.cbff.bmp` | glyph atlas `square` U+CB00–U+CBFF |
| `data/textures/unicode/square.cc00.ccff.bmp` | glyph atlas `square` U+CC00–U+CCFF |
| `data/textures/unicode/square.cd00.cdff.bmp` | glyph atlas `square` U+CD00–U+CDFF |
| `data/textures/unicode/square.ce00.ceff.bmp` | glyph atlas `square` U+CE00–U+CEFF |
| `data/textures/unicode/square.cf00.cfff.bmp` | glyph atlas `square` U+CF00–U+CFFF |
| `data/textures/unicode/square.d000.d0ff.bmp` | glyph atlas `square` U+D000–U+D0FF |
| `data/textures/unicode/square.d100.d1ff.bmp` | glyph atlas `square` U+D100–U+D1FF |
| `data/textures/unicode/square.d200.d2ff.bmp` | glyph atlas `square` U+D200–U+D2FF |
| `data/textures/unicode/square.d300.d3ff.bmp` | glyph atlas `square` U+D300–U+D3FF |
| `data/textures/unicode/square.d400.d4ff.bmp` | glyph atlas `square` U+D400–U+D4FF |
| `data/textures/unicode/square.d500.d5ff.bmp` | glyph atlas `square` U+D500–U+D5FF |
| `data/textures/unicode/square.d600.d6ff.bmp` | glyph atlas `square` U+D600–U+D6FF |
| `data/textures/unicode/square.d700.d7ff.bmp` | glyph atlas `square` U+D700–U+D7FF |

(138 files.)

## Appendix — dead/legacy code

| Code | Why dead / legacy | Tag |
|---|---|---|
| `contrib/systemiv/interface/*` (SystemIV UI components, options and sound-editor windows, style editor) and `contrib/systemiv/lib/eclipse/*` (SystemIV copy of Eclipse) | not referenced by any project file; `eclipse.h` resolves to `contrib/eclipse` (§1.2) | [B:DEAD] |
| Unused Eclipse API: `EclMaximiseWindow`/`EclUnMaximise`, `EclRegisterPopup`, `EclRegisterTooltipCallback`, `EclRegisterClearFunction`, `EclResetDirtyRectangles`, `EclGetDirtyRects`, `EclGenerateUniqueWindowName`, `EclGetCurrentButton`, `EclButton::MouseMove`; non-Win32 branch of `EclGetAccurateTime` | compiled, never called | [B:FULL] unused |
| `GestureWindow` (`code/interface/gesture_window.*`) | needs `GESTURE_EDITOR` and `USE_SEPULVEDA_HELP_TUTORIAL` | [B:DEAD] |
| `GrabberWindow` | compiled only under `AVI_GENERATOR` (TARGET_DEBUG), and its only creator `DebugKeyBindings::GrabberButton` has no caller in any build (`code/interface/debugmenu.cpp`) | [B:DEAD] |
| `DebugMenu` ("Tools Menu") | opener (F1) commented out | [B:LEGACY] |
| `CheatWindow`, `ProfileWindow`, sound editor/stats/profile/parameter windows, `CrateHelpWindow` | `CHEATMENU_ENABLED` / `SOUND_EDITOR` / `INCLUDE_CRATE_HELP_WINDOW` (TARGET_DEBUG) | [B:DEBUG] |
| Map-editor windows, `MultiwiniaWindow`, `NewMapWindow`, `MultiwiniaFileDialog`, `InvalidCratesWindow`, `MultiwiniaOptionsWindow`, `MultiwiniaEditorButton` | `LOCATION_EDITOR` or only reachable from editor code | [B:EDITOR] |
| `MainMenuWindow` and its buttons, profiles, leaderboards, `ResetLocationWindow`, `PrefsKeybindingsWindow`, `ReallyQuitWindow` (with `GameExitButton`), `SaveOnQuitWindow`, `DemoEndWindow`, `GlobalWorldEditorWindow` | compiled; no reachable opener — the creators are the Darwinia loops `code/main.cpp:GlobalWorldGameLoop` (`MainMenuWindow`) and `GlobalWorldEditorLoop` (`GlobalWorldEditorWindow`), buttons of the legacy `code/interface/mainmenus.cpp` menus (`OptionsMenuWindow::Create` → `DemoEndWindow`, key bindings, profiles, reset location), `code/interface/debugmenu.cpp:DebugKeyBindings::ReallyQuitButton` and `code/sound/soundsystem.cpp:SoundSystem::AreBlueprintsModified` (`SaveOnQuitWindow`, sound editor) [C] grep `new <Class>` | [B:LEGACY] |
| `ModsWindow`, `NewModWindow` (method bodies inside `#ifdef USE_DARWINIA_MOD_SYSTEM`) | macro never defined | [B:DEAD] |
| `AboutDarwiniaWindow`, `AboutDarwiniaButton`, `ModsButton` | classes compiled; their only instantiations are inside `#ifdef OBERONBUILD` / `#ifdef USE_DARWINIA_MOD_SYSTEM` (Resolved: P17b tagged `AboutDarwiniaWindow` [B:DEAD]; the class itself is compiled, only its opener is not) | [B:LEGACY] |
| `BuyNowWindow`, `UpdateAvailableWindow`, `code/UI/ErrorDialog.*`, `code/UI/soundsliderbutton.h` (`VolumeControl`) | not in `Darwinia.vcproj` | [B:DEAD] |
| `code/UI` uncompiled headers: `GameMenuOptionsButton`, `HelpModeButton`, `MultiwiniaModeButton`, `QuickStartButton`, `RequestAIButton`, `ResearchModeButton`, `RollingDemoButton`, `MapDetailsButton` | header included nowhere | [B:DEAD] |
| `code/UI` compiled but never instantiated: `AchievementButton`, `BuyNowInfoButton`, `CSpectatorButton` (`#ifdef TESTBED`), `DarwiniaPlusMPButton`, `DarwiniaPlusSPButton`, `GameTypeAnyButton`, `HelpSelectButton`, `QuickMatchButton` | creation commented out or behind undefined macros | [B:LEGACY] |
| Unreachable pages 1, 5, 8, 10, 11, 14, 15, 17, 18, 35 and their widgets (`AnyLevelSelectButton`, `HelpImageButton`, `NextHelpButton`, `PrevHelpButton`, `CampaignButton`, `PrologueButton`, `MultiwiniaVersusCpuButton`) | no live code sets those `m_newPage` values in FULL | [B:LEGACY] / [B:DEBUG] |
| Gap page slots 13, 19–22, 26–30 (Xbox Live Arcade, PDLC, Darwinia+) and the `.vcproj` entries for absent `UI/XBLASelectSessionType.h`, `UI/CustomGameTypeSelection.h`, `UI/InvitePlayersButton.h`, `UI/DownloadPDLCButton.h`, `UI/PDLCOptionsButton.h` | removed products | [B:DEAD] |
| Test bed (`TESTBED_ENABLED`): `GameMenuWindow::UpdateTestBed/AdvanceTestBedGame/ReadTestBedConfig`, `StartTestbedButton` — automates the menus by calling buttons' `MouseUp` | macro never defined | [B:DEAD] |
| `GameMenuWindow::RenderErrorDialogue` / `m_showingErrorDialogue` overlay (errors go to `PageError` instead), `m_reloadPage`, `m_setupNewPage` | never set / never read | [B:FULL] unused |
| `RestartWindowManagerAndRenderer` Linux "Restart Required" `MessageDialog` | `TARGET_OS_LINUX` | [B:DEAD] platform |
| macOS-only paste (`Cmd-V`), `RunSparkleButton`, Mac language/keybinding variants; `TARGET_OS_VISTA` media-centre large menus | platform macros | [B:DEAD] platform |
| `MultiwiniaFileButton::SpecialScroll` (pad scrolling of file rows) | commented out | [B:EDITOR] dead |
