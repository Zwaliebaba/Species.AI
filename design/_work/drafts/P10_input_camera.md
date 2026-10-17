# P10_input_camera — Input driver/filter graph, camera, control help

Scope: `code/lib/input/*` (all 65 partition files plus the 9 non-partition files in that directory — SDL drivers, `inputfilterspec.cpp`, `xbox_eventhandler.cpp` — read for the dead-code appendix), `code/camera.{h,cpp}`, `code/control_help.{h,cpp}`; supporting data `data/input/input_preferences.txt`, `data/input/input_preferences_darwinia.txt`, `data/input/input_preferences_mac.txt`, `data/input/keyboards/de_DE.txt`, `data/input/keyboards/fr_FR.txt`, `data/input/locales.txt`; supporting docs `docs/input_preferences_syntax.txt`, `docs/DarwiniaInput.rtf`. Build baseline: see build_matrix.md (`Release|Win32`, OpenGL, `TARGET_PC_FULLGAME`).

Generated companions (do not edit by hand; regenerate with `python3 -I OUT/_work/scripts/P10_bindings.py <SRC> OUT/_work/manifest.csv <csv> <md> <lines_csv>`):
- `OUT/_work/drafts/P10_input_camera_bindings.csv` — one row per `ControlType` (164 rows): type, default KB/M and XInput bindings, icon, Darwinia-scheme / Mac / de_DE / fr_FR differences, live consumer files.
- `OUT/_work/drafts/P10_input_camera_preflines.csv` — every non-blank line of the five shipped binding files with the parse outcome the engine would produce (re-implemented parser).

Tag legend as build_matrix §7. "Live consumer" = file in the `Release|Win32` vcproj build whose non-comment, non-`#if`-dead code references `Control<Name>`.

---

## [→ 06_player_control_and_input] 1. Input system architecture

### 1.1 Overview

The input layer is a small **query-driven binding graph** [C] `code/lib/input/input.h:InputManager`:

- Game code never reads hardware. It asks `g_inputManager->controlEvent(ControlX[, details])` — "did logical control X trigger this frame?" — and optionally receives an `InputDetails` payload (type + x/y integers + controller index).
- Each logical control (`ControlType`, 164 values + `ControlNull`, §2) owns an ordered list of `InputSpec`s (its bindings). A spec is a tiny compiled record: *which driver*, *which control inside the driver*, *which condition* (down/up/pressed/…), *driver-private handler id*, *output type*.
- **Drivers** both *parse* binding strings (each driver recognises its own syntax) and *evaluate* specs. Leaf drivers read hardware state (Win32 key/mouse, XInput pads, idle timer, preferences, constants). Composite drivers (`&&` conjunction, `++` chord, `not` inversion, `alias`, `->` pipe) hold sub-spec lists and evaluate them by recursing into the manager. This recursion is what makes the system a graph.
- Bindings come from text files (`data/input/*.txt`) loaded at start-up, then overridden by a keyboard-layout file and by the user's saved file.
- Polling model: there are no input events for game code; each frame the manager `Advance()`s every driver (snapshotting "this frame" vs "last frame" state), then any number of `controlEvent` queries are answered from those snapshots. Queries are side-effect free except for XInput "repeat" timers (§1.6.6).

### 1.2 Lifecycle and frame order [B:FULL]

| Step | When | What | Ref |
|---|---|---|---|
| 1 | start-up, before `App` exists | `g_eventHandler = new W32EventHandler()` | [C] `code/main.cpp:AppMain` (L1090) |
| 2 | start-up | `InitialiseInputManager()` creates `g_inputManager` and registers drivers **in this order** (index = `InputSpec::driver`): 0 `ConjoinInputDriver`, 1 `ChordInputDriver`, 2 `InvertInputDriver`, 3 `IdleInputDriver`, 4 `W32InputDriver`, 5 `XInputDriver` *only if* `LoadLibrary("XINPUT1_3")` succeeds, then `PrefsInputDriver`, `ValueInputDriver`, `AliasInputDriver` (indices 6–8, or 5–7 without XInput). `PipeInputDriver` is **never registered** and `g_inputFilterManager` is never created. | [C] `code/main.cpp:InitialiseInputManager` |
| 3 | start-up, after `new App()` | `ReadInputPreferences()`: system file → keyboard-layout file (replace) → user file (replace) (§1.10) | [C] `code/main.cpp:ReadInputPreferences` |
| 4 | start-up | `g_target = new TargetCursor()` (virtual cursor fed by `TargetMove`/`TargetMoveSecondary`/`TargetMoveZ`) | [C] `code/main.cpp:AppMain`; `code/lib/targetcursor.cpp:TargetCursor::Advance` |
| 5 | every render frame | `UserInput::Advance()` → `InputManager::Advance()`: (a) clear per-frame suppression flags; (b) for each driver in registration order: `driver->Advance()`, accumulate idle and input-mode; (c) `g_target->Advance()` | [C] `code/user_input.cpp:UserInput::Advance`; `code/lib/input/input.cpp:InputManager::Advance` |
| 6 | many times per frame (loading screens, main loops, D3D present) | `InputManager::PollForEvents()` → each driver's `PollForEvents()`; W32 pumps the Win32 message queue (`g_windowManager->NastyPollForMessages()`) | [C] `code/lib/input/input.cpp:InputManager::PollForEvents`; `code/lib/input/inputdriver_win32.cpp:W32InputDriver::PollForEvents` |
| 7 | any time after step 5 | `controlEvent()` queries from game/UI code | — |

`InputManager::Advance` also derives two global facts [C] `code/lib/input/input.cpp:InputManager::Advance`:
- **idle** = every driver reports `isIdle()`; consumed by the Idle driver (§1.6.4).
- **input mode** (`INPUT_MODE_KEYBOARD` = 1 or `INPUT_MODE_GAMEPAD` = 2): among non-idle drivers the highest `getInputMode()` wins ("prefers the Gamepad"); if no driver was active the previous mode is kept. Initial value `INPUT_MODE_KEYBOARD`. W32 reports KEYBOARD, XInput reports GAMEPAD, all others NONE. The mode switches UI hints, language-phrase variants (`_kbd`/`_xin` suffix lookup, [C] `code/lib/language_table.cpp:LangTable::RawDoesPhraseExist`), automatic camera tracking (§3.6) and control help (§4).

### 1.3 Core data types

`InputType` (bit flags) [C] `code/lib/input/input_types.h:InputType`:

| Name | Value | Meaning |
|---|---|---|
| `INPUT_TYPE_FAIL` | 0 | details invalid |
| `INPUT_TYPE_BOOL` | 1 | button-like |
| `INPUT_TYPE_ANALOG` | 2 | carries axis data (bit shared by 1D/2D) |
| `INPUT_TYPE_1D` | 6 | one axis in `x` (trigger, wheel, idle seconds); implies ANALOG |
| `INPUT_TYPE_2D` | 10 | two axes in `x`,`y` (mouse, thumbstick); implies ANALOG |
| `INPUT_TYPE_ANY` | 15 | wildcard (terminator row of the action table only) |

`InputDetails {type, int x, int y, int controller=-1}` [C] `code/lib/input/input_types.h:InputDetails`. `controller` is set only by XInput (index 0–3 of the pad that satisfied the spec).

`InputSpec {unsigned driver; inputtype_t type; int control_id; int handler_id; int condition}` [C] `code/lib/input/inputspec.h:InputSpec`. `handler_id` is driver-private (W32: 1=key, 2=mouse; XInput: sensitivity percent).

`InputDescription {noun, verb, pref, translated}` — human-readable description of a spec, used by the key-binding UI; `pref` is the re-serialised binding string; `translate()` replaces `noun`/`verb` with language phrases if such phrases exist [C] `code/lib/input/input_types.cpp:InputDescription::translate`.

`InputMode {INPUT_MODE_NONE=0, INPUT_MODE_KEYBOARD=1, INPUT_MODE_GAMEPAD=2}` [C] `code/lib/input/input_types.h:InputMode`.

`InputCondition` [C] `code/lib/input/inputdriver.h:InputCondition`, with the shared condition-word table [C] `code/lib/input/inputdriver.cpp:s_conditions`:

| Id | Enum | Word (prefs file) | Applies to | Semantics |
|---|---|---|---|---|
| 0 | `COND_DOWN` | `down` | BOOL | became pressed this frame (edge) |
| 1 | `COND_UP` | `up` | BOOL | became released this frame (edge) |
| 2 | `COND_PRESSED` | `pressed` | BOOL | currently held (level) |
| 3 | `COND_REPEAT` | `repeat` (XInput BOOL controls only, not in shared table) | BOOL | held, fires on first frame then every 0.3 s |
| 4 | `COND_ACTION` | (`action` commented out) | — | unused |
| 5 | `COND_RELEASED` | `release` | ANALOG | just returned to zero/dead zone |
| 6 | `COND_NONZERO` | `position` | ANALOG | outside dead zone; returns absolute position |
| 7 | `COND_MOVED` | `move` | ANALOG | changed since last frame; returns delta |
| 8 | `COND_ZERO` | `zero` | ANALOG | inside dead zone |
| 9 | `COND_READ` | `read` | ANALOG | always true; returns current value |

**Condition lookup bug (design-relevant)** [C] `code/lib/input/inputdriver.cpp:InputDriver::getDefaultConditionID`: the lookup loops until a word matches *and* `(controlType & rowType) == rowType`; if nothing matches it stops on the NULL terminator row (index 8) and, because the guard is `i < NumInputConditions` (=10) instead of "found", returns condition 0 (`COND_DOWN`) with type 0 (`INPUT_TYPE_FAIL`). Type 0 passes every bind-acceptance test (§1.4). Consequences in shipped data: `XInput dpadleft donw` (typo), `XInput lefttrigger repeat`, `XInput righttrigger repeat`, `XInput lefttrigger down` all silently become "down" edges (§2.3). A new implementation must reject unknown conditions — but must then deliberately map these four shipped bindings to "down" to preserve behaviour.

`InputParserState` (parse result; only `STATE_DONE` = success; when all drivers fail the manager reports the error of the driver that progressed furthest, i.e. highest state value) [C] `code/lib/input/inputdriver.h:InputParserState`, `code/lib/input/input.cpp:InputManager::parseInputSpecTokens`: `STATE_ERROR`=0, `STATE_WANT_DRIVER`=1, `STATE_WANT_CONTROL`=2, `STATE_WANT_COND`=3, `STATE_WANT_MODIFIER`=4, `STATE_WANT_OPTIONAL`=5, `STATE_BAD_EXTRA`=6, `STATE_OVERSTEP`=7, `STATE_CONJ_ERROR`=8, `STATE_DONE`=9.

> Note: the per-driver error-string arrays (`errors[]` in `inputdriver_simple.cpp`, `_alias.cpp`, `_prefs.cpp`, `_value.cpp`) have 9 entries ordered as if `STATE_WANT_OPTIONAL` did not exist, so messages for states ≥5 are off by one and state 9 indexes past the end [C]. Messages are only logged to stdout (`derr`) — cosmetic.

### 1.4 `ControlBindings` — storage and acceptance [C] `code/lib/input/control_bindings.{h,cpp}`

- `bindings[NumControlTypes]`: per control an ordered `auto_vector<const InputSpec>` (owning list). Order = order of successful bind calls; **index 0 is the "primary" binding** (shown in the key-binding UI, replaced by locale/user files).
- `icons[NumControlTypes]`: per control one icon path string (`~` lines; last one wins).
- `suppressed[NumControlTypes]` (byte flags): set by `InputManager::suppressEvent(type)`, cleared at the start of each `InputManager::Advance`. A suppressed control returns false for the rest of the frame (used e.g. so a click consumed by an Eclipse UI window does not also reach the game: [C] `code/user_input.cpp` — `EclipseLMouseDown` suppressed when a window is under the cursor).
- `getControlID(name)`: case-insensitive (`stricmp`) linear search over the names generated from `control_types.inc` (X-macro `DEF_CONTROL_TYPE(name,type)` → `"name"`) [C] `code/lib/input/control_bindings.cpp:s_actions`.
- **Type acceptance** `isAcceptibleInputType(control, specType)`: `valid = controlType; if (valid has BOOL) valid |= INPUT_TYPE_1D; accept iff (valid & specType) == specType`. Hence: BOOL controls accept BOOL, 1D (e.g. triggers) and ANALOG (2) specs; 1D controls accept 1D/ANALOG; 2D controls accept 2D/ANALOG but **not** BOOL; type-0 specs are accepted everywhere. Note the default condition lookup rewrites an analog spec's type to `INPUT_TYPE_ANALOG` (2) — the concrete 1D/2D distinction is only restored at evaluation time by the driver filling `details.type`.
- `bind(type, spec, replace)`: if accepted, copy the spec; with `replace && list non-empty` erase element 0 and insert the new spec at 0; otherwise append.
- `code/lib/input/control_types.cpp` defines a second, unused `static s_actions` table whose names are the literal string `"x"` (macro bug) [C] — dead.

### 1.5 `InputManager` API semantics [C] `code/lib/input/input.cpp`

| Method | Behaviour |
|---|---|
| `controlEvent(type, details)` | If `type` not suppressed: iterate its specs **in order**; return true at the **first** spec whose driver's `getInput` returns true (details from that spec). Otherwise `details.type = FAIL`, false. A control therefore behaves as the logical OR of its bindings; the payload comes from the first satisfied binding. |
| `checkInput(spec, details)` | `drivers[spec.driver]->getInput(spec, details)`; on false sets `details.type = FAIL`. Used by composite drivers. |
| `parseInputPrefs(reader, replace)` | Line parser (§1.9). |
| `parseInputSpecString/Tokens` | Whitespace tokenisation (`istringstream >>`); offer tokens to each driver in registration order; first `STATE_DONE` wins and sets `spec.driver`. |
| `replacePrimaryBinding(type, str)` | parse + `bind(type, spec, replace=true)`; silent on failure. Used by the key-binding window. |
| `getFirstActiveInput(spec, instant)` | Asks drivers in order; only W32 implements it: first VK `i` in `0..222` (`< KEY_TILDE`, so TILDE itself is excluded) with a "down" edge this frame whose key name contains no space → `key <name> down` if `instant` else `key <name> pressed`. Mouse capture code is commented out → **only keyboard keys can be captured for rebinding** [C] `code/lib/input/inputdriver_win32.cpp:W32InputDriver::getFirstActiveInput`. |
| `getBoundInputDescription(type, desc)` | Description of the first bound spec whose driver can describe it, then `translate()`. |
| `controlIcon(type)` | icon path. |
| `isIdle()`, `getInputMode()` | §1.2. |
| `Clear()` | remove all bindings (icons kept). |
| `printNumBindings()` | debug dump. |

### 1.6 Driver catalogue

#### 1.6.0 `InputDriver` base and `SimpleInputDriver` [C] `code/lib/input/inputdriver.{h,cpp}`, `code/lib/input/inputdriver_simple.{h,cpp}`

Base virtuals: `parseInputSpecification` (pure), `getInput` (pure), `Advance` (pure), `getLastParseError` (pure), `isIdle` (default true), `getInputMode` (default NONE), `getFirstActiveInput` (default false), `PollForEvents` (default no-op), `getInputDescription` (default false), `getName`. Helpers: condition lookup (§1.3), `getDefaultPrefsString` (condition → word, safe version that checks for the terminator), `getDefaultVerb` → `"control_verb_" + word` (no `control_verb_*` phrases exist in `data/language/english.txt` [C], so verbs stay as raw ids — [I] the UI only shows nouns).

`SimpleInputDriver` implements a 3-token state machine shared by Idle, W32 and XInput: `WANT_DRIVER --acceptDriver(tok)--> WANT_CONTROL --getControlID(tok)≥0, type:=getControlType--> WANT_COND --getConditionID(tok)≥0--> writeExtraSpecInfo()` which returns `DONE` or `WANT_OPTIONAL`/`WANT_MODIFIER`; optional tokens go to `parseExtraToken`. A token after `DONE` → `OVERSTEP` (fail). Ending in `WANT_OPTIONAL` counts as `DONE`. Any rejected token stops parsing with the current (non-DONE) state.

#### 1.6.1 Conjoin (`&&`) — driver 0 [C] `code/lib/input/inputdriver_conjoin.cpp`

- Parse: if no `&&` token → `STATE_ERROR` (not mine). Split at every `&&`; each part is parsed by the **full** manager (so parts may be chords, inversions, aliases, leaf specs; an empty part fails). At most one part may have type > BOOL ("complex"); the conjunction's type = that part's type, else BOOL. Sub-spec list stored in `m_specs`; `spec.control_id` = list index.
- Evaluate: all parts must be true (short-circuit on first false). Returned details: first part's details, overridden by the complex part's details if any.
- Description: noun/verb of the highest-typed part.
- Because Conjoin is driver 0, `&&` binds loosest: `a ++ b && c` = `(a ++ b) && c`.

#### 1.6.2 Chord (`++`) — driver 1 [C] `code/lib/input/inputdriver_chord.cpp`

- Parse: if no `++` → error. Split into N parts *without conditions* (e.g. `key control`, `key c`). Generate N conjunction strings; in string k part k gets ` down` and every other part ` pressed`: for `key control ++ key c` → `key control down && key c pressed` and `key control pressed && key c down`. Each must parse to type BOOL.
- Evaluate: **OR** over the N conjunctions — i.e. "all parts held, and at least one of them went down this frame" (order-independent chord edge).
- Description: nouns joined with `-` (e.g. `CTRL-C`).

#### 1.6.3 Invert (`not`/`!`) — driver 2 [C] `code/lib/input/inputdriver_invert.cpp`

- Parse: first token `not` (case-insensitive) or `!`; remaining tokens parsed by the manager; must be exactly `INPUT_TYPE_BOOL` (type-0 fallback specs are rejected). Evaluate: logical NOT of the sub-spec (details are whatever the sub-driver wrote). Description verb prefixed `not `. Unused in shipped data.

#### 1.6.4 Idle — driver 3 [C] `code/lib/input/inputdriver_idle.cpp`

Grammar `idle atleast <N>` | `idle reaches <N>` | `idle no longer` (N positive integer seconds, parsed with `istringstream >> int`).
- State: `m_idleTime` (s), `m_oldIdleTime`, `m_lastChecked`. `Advance`: `old = idle; if manager.isIdle() idle += now - lastChecked else idle = 0; lastChecked = now` (wall clock `GetHighResTime`). Because Idle's own `isIdle()` is the default `true` and the manager's idle flag is computed *during* the driver loop, Idle reads the **previous frame's** combined flag [I] (driver 3 advances before W32/XInput).
- Evaluate: `atleast N`: `int(idle) >= N` (1D, x = int idle seconds); `reaches N`: `old < N < idle` (one-frame edge); `no longer`: `idle == 0` (BOOL — true on every non-idle frame, not only on the transition).
- `getControlID` scans a 3-entry table without terminator → an unknown word reads past the array (UB) [C].
- Shipped use: `StopAttractMode = idle no longer`; `StartAttractMode = idle reaches 60` is commented out. Attract mode itself is `ATTRACTMODE_ENABLED`-gated → these bindings are [B:LEGACY].

#### 1.6.5 W32 key/mouse — driver 4 [C] `code/lib/input/inputdriver_win32.{h,cpp}`, `code/lib/input/keydefs.h`, `code/lib/input/keynames.cpp`

Grammar: `key <keyname> (down|up|pressed)` | `mouse (left|right|middle) (down|up|pressed)` | `mouse (axes|wheel) (move|read)` | `mouse any …` | `key any …`. Driver words are case-sensitive (`key`, `mouse`); key names case-insensitive; mouse control words case-sensitive.

State (per driver; key arrays are globals `g_keys[256]`, `g_keyDeltas[256]` also read directly by legacy code):

| Field | Written by | Meaning |
|---|---|---|
| `g_keys[vk]` | WndProc | 1 while held |
| `m_keyNewDeltas[vk]` | WndProc | +1 on press / -1 on release since last Advance (last event wins) |
| `g_keyDeltas[vk]` | Advance (`= m_keyNewDeltas`, then new deltas zeroed) | this frame's edge |
| `m_mb[3]` | WndProc (L=0, R=1, M=2) | button held |
| `m_mbDeltas[3]` | Advance: `m_mb - m_mbOld` | edge (+1/-1/0) |
| `m_mousePos[3]` | WndProc (`WM_MOUSEMOVE` absolute x,y; wheel z += `(short)HIWORD(wParam)/120`, integer division) | |
| `m_mouseVel[3]` | Advance: `pos - posOld` | per-frame delta |

Windows messages handled (`W32InputDriver::WndProc`): `WM_[LMR]BUTTONDOWN/UP` (also capture/uncapture mouse), `WM_MOUSEWHEEL` (0x020A), `WM_MOUSEMOVE`, `WM_HOTKEY` id 0 (Alt-Tab hot-key → synthesises a TAB press), `WM_SYSKEYDOWN` (ALT itself sets `g_keys[ALT]`; other keys with ALT held set a down-edge; **Alt+F4 sets `g_app->m_requestQuit`**), `WM_SYSKEYUP`, `WM_KEYDOWN` (edge only if not already held — auto-repeat ignored), `WM_KEYUP`, `WM_CHAR` (forwarded to Eclipse text input `EclUpdateKeyboard` with shift/ctrl/alt state). Unhandled → -1 (next processor).

Evaluation:
- key: `down` ⇔ `g_keyDeltas==+1`, `up` ⇔ `-1`, `pressed` ⇔ `g_keys==1`. A press+release inside one frame leaves delta -1 → the `down` edge is **lost** (only `up` seen) [C].
- mouse button: `down/up/pressed` from `m_mbDeltas`/`m_mb`; a click shorter than one frame is lost entirely [C].
- `mouse axes move` → 2D, `x,y = m_mouseVel`, true if non-zero; `read` → always true. `mouse wheel move` → 1D `x = wheel notches this frame`. Other conditions on axes/wheel never fire.
- `mouse any` and `key any` (VK 255, "TODO: Any key not implemented") parse but **never fire** [C].
- `isIdle` = all key/delta arrays zero, no button held, zero mouse velocity (wheel included). `getInputMode` = KEYBOARD.
- `SetMousePosNoVelocity(x,y)` warps without generating velocity.

Key-name table (`getKeyNames()`, index = Windows VK code; only names without spaces are bindable; lookup scans indices 0…223 so **`META` (224) is unbindable** — the Mac file's `key meta` lines fail) [C] `code/lib/input/keynames.cpp:getKeyNames`, `code/lib/input/inputdriver_win32.cpp:W32InputDriver::getKeyId`:

8=`BACKSPACE`, 9=`TAB`, 13=`ENTER`, 16=`SHIFT`, 17=`CONTROL`, 18=`ALT`, 19=`PAUSE`, 20=`CAPSLOCK`, 27=`ESC`, 32=`SPACE`, 33=`PGUP`, 34=`PGDN`, 35=`END`, 36=`HOME`, 37=`LEFT`, 38=`UP`, 39=`RIGHT`, 40=`DOWN`, 45=`INSERT`, 46=`DEL`, 48–57=`0`…`9`, 65–90=`A`…`Z`, 96–105=`0_PAD`…`9_PAD`, 106=`ASTERISK`, 107=`PLUS_PAD`, 108=`MINUS_PAD`, 109=`Unknown`, 110=`DEL_PAD`, 111=`SLASH_PAD`, 112–123=`F1`…`F12`, 144=`NUMLOCK`, 145=`SCRLOCK`, 186=`COLON`, 187=`EQUALS`, 188=`COMMA`, 189=`MINUS`, 190=`STOP`, 191=`SLASH`, 192=`AT`, 219=`OPENBRACE`, 220=`BACKSLASH`, 221=`CLOSEBRACE`, 222=`QUOTE`, 223=`TILDE`, 224=`META` (all other indices are `"unknown N"` / `"unknown 146-185"` / `"unknown 193-218"`).

> **Table defect** [C]: `keydefs.h` defines `KEY_MINUS_PAD 109` (= `VK_SUBTRACT`) but the name table has `MINUS_PAD` at 108 (`VK_SEPARATOR`) and `Unknown` at 109; so `key minus_pad` binds the separator key, and numpad-minus can only be named `key unknown`. A new game should derive names from a single VK→name table.

Description (`getInputDescription`) for the binding UI: noun = language phrase `control_<KEYNAME>` if it exists (case-insensitive hash lookup), else the raw key name; verb `control_verb_<cond>`; **`pref` is built from the *translated* noun** (`"key " + noun + " " + cond`). For keys whose English phrase differs from the bindable name (`control_control`=`CTRL`, `control_insert`=`Ins`, `control_equals`=`=`, `control_minus`=`-`, `control_slash`=`/`, `control_stop`=`.`, `control_asterisk`=`*`, `control_at`=`@`, `control_backslash`=`\`, `control_tilde`=`~`, `control_openbrace`=`{`, `control_closebrace`=`}`, `control_plus_pad`=`NUM_+`, `control_minus_pad`=`NUM_-`, `control_del_pad`=`NUM_DEL`, `control_slash_pad`=`NUM_/`) the re-serialised string cannot be parsed back, so **rebinding a control to one of these keys silently fails** (in-session via `replacePrimaryBinding` and on next start via the user file) [C] chain `code/lib/input/inputdriver_win32.cpp:W32InputDriver::getInputDescription` → `code/interface/prefs_keybindings_window.cpp:ApplyKeybindingsButton::MouseUp`; phrase values `data/language/english.txt`. [I] assumes `UnicodeString`→`std::string` assignment yields the ASCII text. Mouse nouns: `control_mouse_left_button`, `control_mouse_right_button`, `control_mouse_mid_button`, `control_mouse_wheel` (verb `control_verb_scroll`), `control_mouse`, `control_mouse_unknown`; none of these phrases exist in English data → raw ids.

#### 1.6.6 XInput — driver 5 (present only if `xinput1_3.dll` loads) [C] `code/lib/input/inputdriver_xinput.{h,cpp}`

Grammar: `XInput <control> <cond> [with sensitivity <1..200>]` (driver word case-sensitive). Controls (index, name, type):

| # | Name | Type | Evaluated as |
|---|---|---|---|
| 0 | `lefttrigger` | 1D | trigger value |
| 1 | `righttrigger` | 1D | trigger value |
| 2 | `leftbumper` | BOOL | button LB |
| 3 | `rightbumper` | BOOL | button RB |
| 4 | `leftthumbstick` | 2D\|BOOL | stick L (analog conds) / L3 click (bool conds) |
| 5 | `rightthumbstick` | 2D\|BOOL | stick R / R3 click |
| 6–9 | `dpadleft`, `dpadright`, `dpadup`, `dpaddown` | BOOL | d-pad |
| 10–13 | `a`, `b`, `x`, `y` | BOOL | face buttons |
| 14 | `start` | BOOL | |
| 15 | `back` | BOOL | |
| 16 | `plug` | BOOL | controller connected (`down` = plugged this frame, `up` = unplugged this frame, `pressed` = connected) — evaluated even when disconnected |
| 17–20 | `leftthumbstickleft/right/up/down` | BOOL | stick L as 4 virtual buttons |
| 21–24 | `rightthumbstickleft/right/up/down` | BOOL | stick R as 4 virtual buttons |
| 25 | `lefttriggerb` | BOOL | `bLeftTrigger != 0` (ignores condition; leaves `details.type` unset) |
| 26 | `righttriggerb` | BOOL | `bRightTrigger != 0` (same) |

Optional `with sensitivity N` is accepted only on controls 0, 1, 4, 5 (sets `handler_id = N`, default 100 → multiplier N/100); directional controls get sensitivity 100 and accept no extra tokens. `repeat` is accepted only for BOOL-typed controls (`lefttrigger repeat` therefore falls into the unknown-condition fallback = `down`).

Per-frame acquisition, for each of 4 controllers (`MaxControllers = 4`, all polled; queried only if bit set in `m_controllerMask`, default `0x0F`): copy state→old; `XInputGetState(id)`; if connected: **dead zones** — if both stick axes are inside ±`XINPUT_GAMEPAD_LEFT/RIGHT_THUMB_DEADZONE` (square test) zero both; triggers below `XINPUT_GAMEPAD_TRIGGER_THRESHOLD` → 0 (SDK constants, [I] 7849 / 8689 / 30); then **response curve** `stick' = floor((raw · S / 32768)^3)` with `S = 3.4` (left) / `2.7` (right), **Y negated** (up = negative), giving ranges ±39 (left) and ±19 (right); triggers `floor(raw · 6.3)` stored back into a `BYTE` (values >40 overflow — [U] compiler-defined wrap; effectively only zero/non-zero is robust) [C] `code/lib/input/inputdriver_xinput.cpp:Controller::correctDeadZones`, `Controller::scaleInputs`, `Controller::Advance`.

Evaluation rules [C] `Controller::getInput`, `getButtonInput`, `getThumbInput`, `getTriggerInput`, `getThumbDirection`:
- Disconnected pad: false (except `plug`). If `dwPacketNumber` unchanged since last frame, edge conditions (`down`, `up`, `release`, `move`) are false.
- Buttons: `down` = now && !before; `up`; `pressed`; `repeat` = now && (!before || now ≥ nextRepeat[control]); on fire nextRepeat = now + **0.3 s** (`REPEAT_FREQ`). The repeat timer table `s_nextrepeat[27]` is **static, shared by all four controllers, and mutated during queries** (two bindings on the same control share it).
- Stick analog: `position` (2D, x/y = round(v·sens), true if v≠0); `move` (2D delta·sens, true if |dx| or |dy| > 3.4); `zero`; `release` (now zero, before non-zero); `read` (always).
- Trigger analog: `position` (1D x = round(v·sens), v≠0), `zero`, `release`, `move` (1D delta), `down` (non-zero now, zero before), `read`, `pressed` (x = v>0).
- Stick-as-buttons: value = −y (up), y (down), x (right), −x (left), scaled by sensitivity; pressed ⇔ value > 3.4 (left-stick threshold used for both sticks); `down/pressed/up/repeat` as buttons; repeat timer keyed by the **stick** id (shared across its four directions).
- `getInput` over the masked controllers returns the first controller that satisfies the spec and stores its index in `details.controller`. `isIdle` = every masked connected pad reads all-zero gamepad state. `getInputMode` = GAMEPAD.
- `setControllerMask(mask)` exists; no live caller in `code/` (only an `extern` in `code/UI/GameMenuWindow.cpp`) [C] → all four pads always active.
- Description: noun `control_gamepad_<name>`, verbs `control_verb_squeeze` for trigger position/move/read; no such phrases in English data.

#### 1.6.7 Prefs (`pref [not|!] <key>` / `pref !<key>`) [C] `code/lib/input/inputdriver_prefs.cpp`

True iff `g_prefsManager->GetInt(key, 0) > 0` (or the negation). The key must exist at parse time. **Index bug**: `keyPosition` returns `m_keys.size()` (1-based) after appending a new key but `getInput` indexes 0-based and range-checks `< size`, so the first spec referencing each new key always reads the wrong slot (never true for the last-added key) [C]. Double negation (`not !x`, `!!x`) rejected. Unused in shipped data.

#### 1.6.8 Value (`value Pecan|Hummingbird`) [C] `code/lib/input/inputdriver_value.cpp`, `code/lib/input/inputdriver_values.inc`

Constants: `Pecan` = true; `Hummingbird` = pref `Hummingbird` > 0. The parser never sets `spec.type` (left uninitialised by the caller) → bind acceptance is undefined [C]. Unused in shipped data.

#### 1.6.9 Alias (`alias <ControlName>`) [C] `code/lib/input/inputdriver_alias.cpp`

BOOL spec whose evaluation is `controlEvent(otherControl, details)` (recursion; a self-referential alias would recurse without bound — no guard). Shipped use: `EclipseLMouseDown = alias ShowMouseCursor && XInput a down` and `EclipseLMousePressed = alias ShowMouseCursor && XInput a pressed` — holding LB turns the pad's A button into a UI left click.

#### 1.6.10 Pipe (`[spec, spec, …] -> filter`) [B:LEGACY — compiled, never registered] [C] `code/lib/input/inputdriver_pipe.cpp`

Intended to feed several sub-specs into an `InputFilter`. Never added to the manager; `g_inputFilterManager` is never instantiated (NULL deref if it were). Its arrow search starts at `tokens[tokens.length()]`, which throws `"Index out of bounds."` [C]. Not part of the shipped grammar.

### 1.7 Filters [B:LEGACY] [C] `code/lib/input/inputfilter*.{h,cpp}`, `code/lib/input/inputfilterspec.h`

`InputFilter` (abstract `filter`, `parseFilterSpecification`, `Advance`) → `InputFilterWithDelta` (registers per-spec current/old `InputDetails`, `getDelta` with appear/disappear semantics, `Advance` that swaps x and y by mistake) → `DirectionInputFilter` (`direction (up|down|left|right) (down|pressed)`; `filter()` returns false; `calcDetails` empty). `InputFilterManager` (list of filters, parse, apply) has its `Advance` body commented out. `InputFilterSpec {filter, id, type, min_inputs, max_inputs, mode, condition}`. Entire subsystem is a stub; `inputfilterspec.cpp` (`newFilterSpecID`) is not even in the vcproj [B:DEAD].

### 1.8 Helper abstractions [C]

- `Movement2D`/`Movement3D` (integer velocity + sign helpers), `DPadMovement` (6 BOOL controls → `v = (pos − neg) × sensitivity` on X/Y/Z), `AnalogMovement2D` (2D control × sensitivity, zero if not 2D), `PriorityMovement2D` (first source unless zero) [C] `code/lib/input/movement2d.{h,cpp}`. Live use: the camera debug mode's static `DPadMovement(CameraForwards, CameraBackwards, CameraLeft, CameraRight, CameraUp, CameraDown, 1)` [C] `code/camera.cpp:Camera::AdvanceDebugMode`.
- `InputTransform`/`ControlEventFunctor`/`ToggleInputTransform` (latch: `state = (state && !off) || (!state && on)`, re-evaluated on every query) [C] `code/lib/input/transform.{h,cpp}`. Only `ControlEventFunctor` is used outside the library — as base of `KeyboardShortcut` in the task-manager interface [C] `code/taskmanager_interface.h:KeyboardShortcut` (P09); `ToggleInputTransform`, `AnalogMovement2D`, `PriorityMovement2D` have no users.
- `TargetCursor` (outside scope, P12) is the single source of "mouse position" for game code: per frame `TargetMove` (or, if no Eclipse window and task manager hidden, `TargetMoveSecondary`) 2D delta is added to the virtual cursor; `TargetMoveZ` (1D, disabled while windows or chat are open) accumulates wheel Z; `dX/dY/dZ` = this frame's deltas [C] `code/lib/targetcursor.cpp:TargetCursor::Advance`.
- `InputPrefs` (static paths/locale) §1.10.

### 1.9 Event handling [C] `code/lib/input/eventhandler.{h,cpp}`, `win32_eventhandler.{h,cpp}`, `win32_eventproc.h`

`EventHandler` (abstract `WindowHasFocus`) ← `W32EventHandler` (also a `W32EventProcessor`). `W32EventHandler::WndProc` (called by the window manager for every message):
1. Focus: `g_windowHasFocus = (GetForegroundWindow() == game window)`; when focus is lost or was lost last message and ALT is held, force an ALT up-edge (avoids stuck ALT after Alt-Tab).
2. Returns -1 (let Windows handle) for `WM_CREATE`, `WM_DESTROY`, `WM_SIZING`, `WM_WINDOWPOSCHANGING`, `WM_WINDOWPOSCHANGED`, `WM_COMMAND`, `WM_NCHITTEST`; `WM_SETCURSOR` hides the OS cursor over the client area and then (missing `break`) falls through to `WM_MOVE` handling (`WindowMoved()`, return -1); `WM_CANCELMODE` → 0; `WM_CLOSE` → `m_requestQuit = true`; `WM_INPUTLANGCHANGE` only logged (bindings/locale are **not** reloaded on layout change).
3. Otherwise offers the message to registered processors in order (only `W32InputDriver`) until one returns ≠ -1.
`BindAltTab`/`UnbindAltTab` register hot-key id 0 = Alt+Tab (its only caller is commented out → Alt-Tab is never captured in the shipped build [C] `code/main.cpp`, `code/interface/prefs_keybindings_window.cpp`). `RemoveEventProcessor` uses `std::remove` without `erase` (no-op) [C]. The camera only advances while `g_eventHandler->WindowHasFocus()` [C] `code/main.cpp:LocationGameLoop`.

### 1.10 Binding files, layering, locales [C] `code/lib/input/file_paths.cpp`, `code/main.cpp:ReadInputPreferences`

| Layer | Path | Selection | Mode |
|---|---|---|---|
| System | `input/input_preferences.txt` (PC) or `input/input_preferences_darwinia.txt` | preference `MultiwiniaControlMethod` (default 1): 0 → Darwinia file, else main file (`input_preferences_mac.txt` replaces the main file under `TARGET_OS_MACOSX`) | append |
| Keyboard layout | `input/keyboards/<layout>.txt` | `<layout>` = preference `KeyboardLayout`, default = `locales.txt` lookup of `GetKeyboardLayout(0) & 0xFFFF` (LANGID); path cached on first call; missing file silently skipped | replace (primary) |
| User | `<profile dir>/user_input_preferences.txt` | written by Options → Controls (`PrefsManager` key = control name, value = binding string) | replace (primary) |

`locales.txt` [C] `data/input/locales.txt`: 1031→`de_DE`, 1033→`en_US`, 1036→`fr_FR`, 2057→`en_UK`, 4105→`en_CA`. Only `de_DE.txt` and `fr_FR.txt` exist; each re-states the primary camera keys (de: `CameraZoom = key y pressed` QWERTZ swap; fr: AZERTY — Left `q`, Up `a`, Forwards `z`, Zoom `w`). `InputPrefs::IsKeyboardFrenchLocale()` (layout == `fr_FR`) additionally swaps the camera-help image to `mwhelp/control_camera_french.bmp` [C] `code/multiwiniahelp.cpp`.

Replace semantics: each replace-line removes element 0 of the control's list and inserts the new spec at 0, so several replace-lines for the same control in one file overwrite each other (last wins) and never add bindings. "Restore defaults" in Options re-reads only the system file after `Clear()` — the locale layer is **not** re-applied [C] `code/interface/prefs_keybindings_window.cpp:RestoreDefaultsButton::MouseUp`. User-rebindable controls (Options UI): CameraLeft, CameraRight, CameraForwards, CameraBackwards, CameraUp, CameraDown, CameraZoom, UnitDeselect, SkipMessage, GesturesChatLog, IconsChatLog, IconsTaskManagerDisplay (also rewrites IconsTaskManagerHide to the `up` form), IconsTaskManagerEndTask (P17b owns the UI).

### 1.11 Binding-file grammar (formal) [C] `code/lib/input/input.cpp:InputManager::parseInputPrefs`, `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader`

```
file        := { line EOL }
line        := [ binding | icon ] [ '#' comment ]          ; '#' to end of line removed by TextReader
binding     := CONTROL sep '=' sep spec                     ; CONTROL: ControlType name, case-insensitive
icon        := CONTROL sep '~' sep PATH                     ; sets the control's icon (last wins)
sep         := one or more of  ' ' '\t' ':' ','              ; TextReader separators for the first two tokens
spec        := conj                                          ; rest of line, re-tokenised on whitespace only
conj        := chordOrUnary { '&&' chordOrUnary }           ; ≤1 complex (non-BOOL) operand
chordOrUnary:= chord | unary
chord       := partial '++' partial { '++' partial }        ; partial = leaf spec without its condition word
unary       := ('not' | '!') unary | leaf                    ; operand must be BOOL
leaf        := 'key' KEYNAME BCOND
             | 'mouse' ('left'|'right'|'middle'|'any') BCOND
             | 'mouse' ('axes'|'wheel') ('move'|'read')
             | 'XInput' XBUTTON (BCOND | 'repeat')
             | 'XInput' XANALOG (ACOND | BCOND) [ 'with' 'sensitivity' INT1_200 ]
             | 'idle' ('atleast'|'reaches') POSINT | 'idle' 'no' 'longer'
             | 'pref' ['not'|'!'] ['!']PREFKEY
             | 'value' ('Pecan'|'Hummingbird')
             | 'alias' CONTROL
BCOND       := 'down' | 'up' | 'pressed'
ACOND       := 'position' | 'move' | 'release' | 'zero' | 'read'
```
Unknown condition words are accepted as `down` (bug, §1.3). Lines whose second token is neither `=` nor `~` (e.g. `TaskManagerSwitchTask INPUT_TYPE_BOOL`), unknown control names (e.g. `UnitPrimaryFireDirected ~ …`) and unparsable specs (e.g. `SpaceNav …`) are skipped with a stdout message. Parse failures never abort loading.

### 1.12 Graph diagram

```mermaid
flowchart LR
  subgraph OS["Win32 / XInput"]
    MSG["Window messages<br/>WM_KEY*, WM_*BUTTON*, WM_MOUSEMOVE,<br/>WM_MOUSEWHEEL, WM_CHAR, WM_HOTKEY"]
    XGS["XInputGetState(0..3)"]
  end
  MSG --> EH["W32EventHandler::WndProc<br/>(focus, ALT fix, quit)"] --> W32["W32InputDriver<br/>key/mouse state + deltas"]
  MSG -. WM_CHAR .-> ECL["Eclipse text input"]
  XGS --> XI["XInputDriver<br/>4 × Controller<br/>deadzone + cubic curve"]
  IDLE["IdleInputDriver<br/>idle seconds"]
  PREF["PrefsInputDriver"]
  VAL["ValueInputDriver"]
  subgraph IM["InputManager (g_inputManager)"]
    CB["ControlBindings<br/>ControlType → [InputSpec…] (+icon, suppress)"]
    CE["controlEvent(ControlX, details)<br/>first satisfied spec wins"]
  end
  CONJ["ConjoinInputDriver &&"]
  CHORD["ChordInputDriver ++"]
  INV["InvertInputDriver not/!"]
  ALIAS["AliasInputDriver alias"]
  PIPE["PipeInputDriver -> (not registered)"]
  GAME["Game / UI code<br/>(UserInput, LocationInput, TeamControls,<br/>TaskManagerInterface, Camera, Eclipse)"]
  TC["TargetCursor (g_target)<br/>virtual cursor X/Y/Z"]
  GAME -->|query| CE
  CE --> CB
  CB -->|spec.driver| W32 & XI & IDLE & PREF & VAL & CONJ & CHORD & INV & ALIAS
  CONJ -->|checkInput(sub-spec)| CE
  CHORD -->|checkInput(conjunctions)| CE
  INV -->|checkInput| CE
  ALIAS -->|controlEvent(other)| CE
  TC -->|TargetMove / TargetMoveSecondary / TargetMoveZ| CE
  GAME --> TC
  PIPE -. dead .-> CE
```

```mermaid
sequenceDiagram
  participant Loop as Main loop (per render frame)
  participant UI as UserInput::Advance
  participant IM as InputManager
  participant D as Drivers (in order)
  participant TC as TargetCursor
  participant G as Game code (TeamControls, Camera, UI)
  Loop->>IM: PollForEvents() (n times: pumps Win32 queue)
  Loop->>UI: Advance()
  UI->>IM: Advance()
  IM->>IM: clear suppress[]
  loop each driver
    IM->>D: Advance() (snapshot cur/old, deltas)
    IM->>D: isIdle(), getInputMode()
  end
  IM->>TC: Advance() (controlEvent TargetMove…)
  G->>IM: controlEvent(ControlX, details)
  IM->>D: getInput(spec) for each bound spec until true
  D-->>IM: bool + details (composites recurse into IM)
  IM-->>G: bool + details
```

### 1.13 Class diagram

```mermaid
classDiagram
  class InputManager{-drivers: vector~InputDriver*~ -bindings: ControlBindings -m_idle -m_inputMode +controlEvent() +suppressEvent() +Advance() +PollForEvents() +parseInputPrefs() +checkInput() +getFirstActiveInput() +replacePrimaryBinding()}
  class ControlBindings{-bindings[NumControlTypes] -icons[] -suppressed[] +bind() +suppress() +isActive() +getControlID()}
  class InputDriver{<<abstract>> +parseInputSpecification() +getInput() +Advance() +isIdle() +getInputMode() +getInputDescription()}
  class SimpleInputDriver{<<abstract>> #acceptDriver() #getControlID() #getControlType() #writeExtraSpecInfo() #parseExtraToken()}
  InputDriver <|-- SimpleInputDriver
  SimpleInputDriver <|-- W32InputDriver
  SimpleInputDriver <|-- XInputDriver
  SimpleInputDriver <|-- IdleInputDriver
  InputDriver <|-- ConjoinInputDriver
  InputDriver <|-- ChordInputDriver
  InputDriver <|-- InvertInputDriver
  InputDriver <|-- AliasInputDriver
  InputDriver <|-- PrefsInputDriver
  InputDriver <|-- ValueInputDriver
  InputDriver <|-- PipeInputDriver
  W32EventProcessor <|-- W32InputDriver
  EventHandler <|-- W32EventHandler
  W32EventProcessor <|-- W32EventHandler
  InputManager o-- InputDriver
  InputManager *-- ControlBindings
  class InputFilter{<<abstract>>}
  InputFilter <|-- InputFilterWithDelta
  InputFilterWithDelta <|-- DirectionInputFilter
  InputFilterManager o-- InputFilter
  PipeInputDriver ..> InputFilterManager
  class Movement2D{<<abstract>>}
  Movement2D <|-- Movement3D
  Movement3D <|-- DPadMovement
  Movement2D <|-- AnalogMovement2D
  Movement2D <|-- PriorityMovement2D
  InputTransform <|-- ControlEventFunctor
  InputTransform <|-- ToggleInputTransform
```

## [→ 06_player_control_and_input] 2. Logical controls (`ControlType`) and default bindings

### 2.1 How to read the table

- Source of truth: `code/lib/input/control_types.inc` (164 `DEF_CONTROL_TYPE` entries; enum value = row #, `ControlNull` = 164 is never bound and never fires, `NumControlTypes` = 165) [C]; bindings: `data/input/input_preferences.txt` (the PC default, `MultiwiniaControlMethod` = 1) [C]. Generated by `OUT/_work/scripts/P10_bindings.py`, which re-implements the engine parser (§1.11) including its quirks; full per-line verdicts in `OUT/_work/drafts/P10_input_camera_preflines.csv`.
- Several bindings per cell are OR-ed in the listed order; the first is the *primary* (shown and rebindable in Options). `[=down: unknown-condition fallback]` marks a binding that only works because of the §1.3 bug.
- "Darwinia-scheme diff" = what `data/input/input_preferences_darwinia.txt` (selected by `MultiwiniaControlMethod = 0`, "Darwinia-like" left-click-to-order scheme) binds instead; "same" = identical, "(none)" = binding absent there.
- Pad column applies only when `xinput1_3.dll` is present (§1.2); otherwise those lines fail to parse and the control has only its KB/M bindings.
- "Live consumers" lists `code/` files of the baseline build that query the control (comment- and `#if`-filtered). An empty consumer cell means the binding exists but nothing in the FULL build reads it (dead control). `ShowMouseCursor` is consumed indirectly through the `alias` in `EclipseLMouseDown/Pressed`.
- Type column: BOOL/1D/2D per §1.3.

### 2.2 Control table

| # | ControlType | Type | Source comment | Keyboard / mouse default (input_preferences.txt) | Xbox (XInput) default | Darwinia-scheme diff (KB/M ; pad) | Live consumers (code/) |
|---|---|---|---|---|---|---|---|
| 0 | `MenuLeft` | BOOL | Select button to the left | — | XInput dpadleft repeat \| XInput leftthumbstickleft repeat \| XInput rightthumbstickleft repeat | KB/M: key left down ; pad: same | game_menu.cpp, shaman_interface.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 1 | `MenuRight` | BOOL | Select button to the right | — | XInput dpadright repeat \| XInput leftthumbstickright repeat \| XInput rightthumbstickright repeat | KB/M: key right down ; pad: same | game_menu.cpp, shaman_interface.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 2 | `MenuUp` | BOOL | Select button above | — | XInput dpadup repeat \| XInput leftthumbstickup repeat \| XInput rightthumbstickup repeat | KB/M: key up down ; pad: same | interface/darwinia_window.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 3 | `MenuDown` | BOOL | Select button below | — | XInput dpaddown repeat \| XInput leftthumbstickdown repeat \| XInput rightthumbstickdown repeat | KB/M: key down down ; pad: same | interface/darwinia_window.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 4 | `MenuActivate` | BOOL | Press menu button | — | XInput a down | KB/M: key enter down ; pad: same | interface/darwinia_window.cpp |
| 5 | `MenuEscape` | BOOL | Escape out of this menu | key esc down | XInput start down | — | interface/controllerunplugged_window.cpp, main.cpp |
| 6 | `MenuClose` | BOOL | close current window | — | XInput back down \| XInput b down | — | game_menu.cpp, interface/darwinia_window.cpp |
| 7 | `LeaderboardWeekly` | BOOL | Change to weekly leaderboards | — | XInput y down | — | interface/leaderboards_window.cpp |
| 8 | `LeaderboardLTrigger` | BOOL | Cycle leaderboard selections up | — | XInput lefttrigger repeat [=down: unknown-condition fallback] | — | interface/leaderboards_window.cpp |
| 9 | `LeaderboardRTrigger` | BOOL | Cycle leaderboard selections down | — | XInput righttrigger repeat [=down: unknown-condition fallback] | — | interface/leaderboards_window.cpp |
| 10 | `SkipMessage` | BOOL | Skip chat message | key enter down \| key esc down | XInput start down \| XInput a down \| XInput b down \| XInput y down | — | interface/prefs_keybindings_window.cpp, loaders/credits_loader.cpp, startsequence.cpp |
| 11 | `SkipCutscene` | BOOL | — | key enter down | XInput b down | — | script.cpp |
| 12 | `CameraLeft` | BOOL | Move camera left (boolean) | key a pressed \| key left pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp, multiwiniahelp.cpp |
| 13 | `CameraRight` | BOOL | Move camera right (boolean) | key d pressed \| key right pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp, multiwiniahelp.cpp |
| 14 | `CameraUp` | BOOL | Move camera up (boolean) | key q pressed | XInput righttrigger position | — | camera.cpp, interface/prefs_keybindings_window.cpp |
| 15 | `CameraDown` | BOOL | Move camera down (boolean) | key e pressed | XInput lefttrigger position | — | camera.cpp, interface/prefs_keybindings_window.cpp |
| 16 | `CameraForwards` | BOOL | Move camera forwards (boolean) | key w pressed \| key up pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp, multiwiniahelp.cpp, taskmanager_interface_multiwinia.cpp |
| 17 | `CameraBackwards` | BOOL | Move camera backwards (boolean) | key s pressed \| key down pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp, multiwiniahelp.cpp, taskmanager_interface_multiwinia.cpp |
| 18 | `CameraZoom` | BOOL | Zoom camera in (boolean) | key z pressed \| mouse middle pressed | XInput rightthumbstick pressed | — | camera.cpp, interface/prefs_keybindings_window.cpp |
| 19 | `CameraDeZoom` | BOOL | Zoom camera out (boolean) | key c pressed | — | — | — |
| 20 | `CameraMove` | 2D | Analog camera control | — | XInput leftthumbstick position | — | camera.cpp |
| 21 | `TargetMove` | 2D | Analog target control | mouse axes move | — | — | lib/targetcursor.cpp |
| 22 | `TargetMoveSecondary` | 2D | Analog target control | — | XInput rightthumbstick position | — | lib/targetcursor.cpp, team.cpp |
| 23 | `TargetMoveZ` | 1D | Analog target control | mouse wheel move | — | — | lib/targetcursor.cpp |
| 24 | `CameraZoomChange` | 1D | Analog camera zoom | — | — | — | — |
| 25 | `CameraSpeedup` | BOOL | Hold to go faster | key shift pressed | XInput leftthumbstick pressed | — | camera.cpp |
| 26 | `CameraSlowdown` | BOOL | Hold to go slower | mouse right pressed | — | — | camera.cpp |
| 27 | `CameraRotate` | BOOL | rotate the cam in entity track mode | — | XInput lefttrigger position | — | — |
| 28 | `CameraRotateLeft` | BOOL | — | — | XInput lefttrigger position | — | camera.cpp |
| 29 | `CameraRotateRight` | BOOL | — | — | XInput righttrigger position | — | camera.cpp |
| 30 | `CameraSwitchMode` | BOOL | Switch from free to track cam | — | — | — | — |
| 31 | `CameraDebugRotate` | BOOL | Hold for debug rotate mode | mouse right pressed | — | — | camera.cpp |
| 32 | `GestureActive` | BOOL | Hold to write a gesture | mouse left pressed | XInput a pressed | — | taskmanager_interface_gestures.cpp |
| 33 | `DarwinianPromote` | BOOL | — | mouse right down | XInput x down | — | location_input.cpp, multiwiniahelp.cpp |
| 34 | `BeginSelectionCircle` | BOOL | — | mouse left down | XInput a down | — | location_input.cpp |
| 35 | `EnlargeSelectionCircle` | BOOL | — | mouse left pressed | XInput a pressed | — | location_input.cpp, multiwiniahelp.cpp |
| 36 | `EndSelectionCircle` | BOOL | — | mouse left up | XInput a up | — | location_input.cpp |
| 37 | `IssueDarwinianOrders` | BOOL | — | mouse right down | XInput x down | KB/M: mouse left down ; pad: same | location_input.cpp, multiwiniahelp.cpp |
| 38 | `OrderDirectRoute` | BOOL | — | key control pressed | — | — | location_input.cpp |
| 39 | `OrderSpecialMove` | BOOL | — | key shift pressed | XInput x down | KB/M: same ; pad: (none) | location_input.cpp |
| 40 | `IssueSpecialOrders` | BOOL | — | mouse right down | XInput a down | KB/M: same ; pad: XInput x down | location_input.cpp, multiwiniahelp.cpp, team.cpp |
| 41 | `LeftButtonPressed` | BOOL | — | mouse left pressed | — | KB/M: (none) ; pad: same | team.cpp |
| 42 | `LeftButtonDown` | BOOL | — | mouse left down | — | KB/M: (none) ; pad: same | — |
| 43 | `UnitSelect` | BOOL | — | mouse left down | XInput a down | KB/M: same ; pad: XInput a pressed | location_input.cpp, multiwiniahelp.cpp |
| 44 | `UnitDeselect` | BOOL | Escape unit control mode | key space down | XInput b down | — | interface/prefs_keybindings_window.cpp, location_input.cpp |
| 45 | `UnitCycleLeft` | BOOL | Cycle left through tasks | key openbrace down | XInput leftbumper down | — | taskmanager_interface.cpp, taskmanager_interface_multiwinia.cpp |
| 46 | `UnitCycleRight` | BOOL | Cycle right through tasks | key closebrace down \| key tab down | XInput rightbumper down | — | taskmanager_interface.cpp, taskmanager_interface_multiwinia.cpp |
| 47 | `UnitEndSetTarget` | BOOL | — | mouse right up | XInput x up | KB/M: mouse left up ; pad: same | team.cpp |
| 48 | `UnitSetTarget` | BOOL | "Click to go to target" | mouse right pressed | XInput x pressed | KB/M: mouse left pressed ; pad: same | location_input.cpp, team.cpp |
| 49 | `UnitMove` | 2D | Analog unit movement | — | XInput leftthumbstick position | — | camera.cpp, team.cpp |
| 50 | `UnitMoveDirectionChange` | 2D | — | — | XInput leftthumbstick move | — | camera.cpp |
| 51 | `UnitPrimaryFireTarget` | BOOL | "Press button to fire at target" | mouse right pressed | XInput righttriggerb pressed | — | camera.cpp, control_help.cpp, gamecursor.cpp, multiwiniahelp.cpp, team.cpp |
| 52 | `UnitTargetDirected` | 2D | "Move joystick to fire" | — | XInput rightthumbstick position | — | team.cpp |
| 53 | `UnitSecondaryFireTarget` | BOOL | "Press button to fire at target" | mouse left down && mouse right pressed | XInput lefttrigger down [=down: unknown-condition fallback] | — | multiwiniahelp.cpp, team.cpp |
| 54 | `UnitSecondaryAim` | BOOL | — | — | — | — | — |
| 55 | `UnitStartSecondaryFireDirected` | 2D | Start moving power arc | — | — | — | — |
| 56 | `UnitSecondaryFireDirected` | 2D | Fire wherever power arc is | — | — | — | location_input.cpp, multiwiniahelp.cpp, team.cpp |
| 57 | `UnitCreate` | BOOL | — | mouse left down | XInput a down | — | location_input.cpp, multiwiniahelp.cpp |
| 58 | `WeaponCycleLeft` | BOOL | Cycle left through weapons or unit modes | — | XInput dpadleft down | — | — |
| 59 | `WeaponCycleRight` | BOOL | Cycle right through weapons or unit modes | — | XInput dpadright down | — | — |
| 60 | `XBoxMenuCycleUp` | BOOL | Cycle left through weapons or unit modes | — | XInput leftbumper repeat | — | interface/leaderboards_window.cpp, taskmanager_interface_icons.cpp |
| 61 | `XBoxMenuCycleDown` | BOOL | Cycle right through weapons or unit modes | — | XInput rightbumper repeat | — | interface/leaderboards_window.cpp, taskmanager_interface_icons.cpp |
| 62 | `TaskManagerSelectNextTask` | BOOL | Select Next active task | — | — | — | — |
| 63 | `TaskManagerDisplay` | BOOL | Show the task manager | — | — | — | — |
| 64 | `TaskManagerHide` | BOOL | Hide the task manager | — | — | — | — |
| 65 | `TaskManagerSwitchTask` | BOOL | Switch task | — | — | — | — |
| 66 | `TaskManagerEndTask` | BOOL | Kill task | — | — | — | taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 67 | `GesturesTaskManagerDisplay` | BOOL | Show the gestural task manager | key alt pressed | XInput x pressed | — | taskmanager_interface_gestures.cpp |
| 68 | `GesturesTaskManagerHide` | BOOL | Hide the gestural task manager | key alt up | XInput x up | — | — |
| 69 | `GesturesTaskManagerEndTask` | BOOL | Kill task | key control ++ key c | XInput y down | — | taskmanager_interface_gestures.cpp |
| 70 | `GesturesChatLog` | BOOL | — | key l pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp |
| 71 | `GesturesSwitchUnit` | BOOL | — | key tab down | — | — | taskmanager_interface.cpp |
| 72 | `GestureLeft` | BOOL | — | key a down \| key left down | XInput leftbumper down \| XInput dpadleft donw [=down: unknown-condition fallback] | — | taskmanager_interface_gestures.cpp |
| 73 | `GestureRight` | BOOL | — | key d down \| key right down | XInput rightbumper down \| XInput dpadright down | — | taskmanager_interface_gestures.cpp |
| 74 | `IconsTaskManagerDisplay` | BOOL | Show the iconic task manager | key tab down | — | — | interface/prefs_keybindings_window.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 75 | `IconsTaskManagerDisplayDown` | BOOL | When icon task manager key has been pressed down | — | XInput back down | — | taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 76 | `IconsTaskManagerDisplayPressed` | BOOL | When icon task manager key is pressed | — | XInput back pressed | — | taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 77 | `IconsTaskManagerDisplayUp` | BOOL | When icon task manager key is released | — | XInput back up | — | taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 78 | `IconsTaskManagerHide` | BOOL | Hide the iconic task manager | key tab up | XInput back down \| XInput b down | — | interface/prefs_keybindings_window.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 79 | `IconsTaskManagerEndTask` | BOOL | Kill task | key c down | XInput y down | — | interface/prefs_keybindings_window.cpp, multiwiniahelp.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 80 | `IconsChatLog` | BOOL | Show chat log | key l pressed | — | — | camera.cpp, interface/prefs_keybindings_window.cpp |
| 81 | `ActivateTMButton` | BOOL | Activate Task Manager buttons | mouse left up \| key enter down | XInput a up | — | shaman_interface.cpp, taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 82 | `EclipseLMouseDown` | BOOL | Eclipse left mouse button down | mouse left down | alias ShowMouseCursor && XInput a down | — | interface/input_field.cpp, user_input.cpp |
| 83 | `EclipseLMousePressed` | BOOL | Eclipse left mouse button is pressed | mouse left pressed | alias ShowMouseCursor && XInput a pressed | — | interface/input_field.cpp, interface/profilewindow.cpp, user_input.cpp |
| 84 | `EclipseRMousePressed` | BOOL | Eclipse right mouse button is pressed | mouse right down | — | — | user_input.cpp |
| 85 | `ScrollSpeedup` | BOOL | Hold to scroll faster | key shift pressed | — | — | interface/input_field.cpp |
| 86 | `SelectLocation` | BOOL | Select a location on the world screen | mouse left down | XInput a down | — | global_world.cpp, location_input.cpp |
| 87 | `LocationDragActive` | BOOL | Hold to drag a location | mouse left pressed | — | — | global_world.cpp |
| 88 | `DeselectLocation` | BOOL | Deselect a location after dragging | mouse left up | — | — | global_world.cpp |
| 89 | `TileSelect` | BOOL | Location editor tile select | mouse left down | — | — | — |
| 90 | `TileDrop` | BOOL | Location editor tile drop | mouse left up | — | — | — |
| 91 | `TileDrag` | BOOL | Location editor tile drag | mouse left pressed | — | — | — |
| 92 | `FileMultiSelect` | BOOL | Hold to select multiple files | key control pressed | — | — | interface/filedialog.cpp, interface/multiwinia_filedialog.cpp |
| 93 | `DebugDropPacket` | BOOL | For stress testing the network system | key f pressed | — | — | — |
| 94 | `TaskManagerSelectTask1` | BOOL | Go to task manager screenzone 0 | key 1 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 95 | `TaskManagerSelectTask2` | BOOL | Go to task manager screenzone 1 | key 2 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 96 | `TaskManagerSelectTask3` | BOOL | Go to task manager screenzone 2 | key 3 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 97 | `TaskManagerSelectTask4` | BOOL | Go to task manager screenzone 3 | key 4 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 98 | `TaskManagerSelectTask5` | BOOL | Go to task manager screenzone 4 | key 5 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 99 | `TaskManagerSelectTask6` | BOOL | Go to task manager screenzone 5 | key 6 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 100 | `TaskManagerSelectTask7` | BOOL | Go to task manager screenzone 6 | key 7 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 101 | `TaskManagerSelectTask8` | BOOL | Go to task manager screenzone 7 | key 8 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 102 | `TaskManagerSelectTask9` | BOOL | Go to task manager screenzone 8 | key 9 down | — | — | taskmanager_interface_gestures.cpp, taskmanager_interface_icons.cpp, taskmanager_interface_multiwinia.cpp |
| 103 | `IconsTaskManagerNewSquad` | BOOL | Make a new Insertion Squad | key f1 down | — | — | taskmanager_interface_icons.cpp |
| 104 | `IconsTaskManagerNewEngineer` | BOOL | Make a new Engineer | key f2 down | — | — | taskmanager_interface_icons.cpp |
| 105 | `IconsTaskManagerNewOfficer` | BOOL | Make a new Officer | key f3 down | — | — | taskmanager_interface_icons.cpp |
| 106 | `IconsTaskManagerNewArmour` | BOOL | Make a new Armour | key f4 down | — | — | taskmanager_interface_icons.cpp |
| 107 | `IconsTaskManagerSelectGrenade` | BOOL | Select Grenade as the current secondary weapon | key f5 down | — | — | taskmanager_interface_icons.cpp |
| 108 | `IconsTaskManagerSelectRocket` | BOOL | Select Rocket as the current secondary weapon | key f6 down | — | — | taskmanager_interface_icons.cpp |
| 109 | `IconsTaskManagerSelectAirStrike` | BOOL | Select AirStrike as the current secondary weapon | key f7 down | — | — | taskmanager_interface_icons.cpp |
| 110 | `IconsTaskManagerSelectController` | BOOL | Select Controller as the current secondary weapon | key f8 down | — | — | taskmanager_interface_icons.cpp |
| 111 | `LoaderHelp` | BOOL | Hold for a help screen | key h pressed | — | — | — |
| 112 | `SepulvedaHelp` | BOOL | Activate Sepulveda | key h down | — | — | — |
| 113 | `GOLLoaderSpeedup` | BOOL | Game of Life Loader keys | key p pressed | — | — | — |
| 114 | `GOLLoaderSlowdown` | BOOL | — | key o pressed | — | — | — |
| 115 | `GOLLoaderReset` | BOOL | — | key g down | — | — | — |
| 116 | `RTLoaderReflectivenessIncrease` | BOOL | Ray Tracer Loader keys | key p pressed | — | — | — |
| 117 | `RTLoaderReflectivenessDecrease` | BOOL | — | key o pressed | — | — | — |
| 118 | `RTLoaderPixelWaveIncrease` | BOOL | — | key l pressed | — | — | — |
| 119 | `RTLoaderPixelWaveDecrease` | BOOL | — | key k pressed | — | — | — |
| 120 | `RTLoaderMotionBlurIncrease` | BOOL | — | key m pressed | — | — | — |
| 121 | `RTLoaderMotionBlurDecrease` | BOOL | — | — | — | — | — |
| 122 | `RTLoaderPixelBlurIncrease` | BOOL | — | key i down | — | — | — |
| 123 | `RTLoaderPixelBlurDecrease` | BOOL | — | key u down | — | — | — |
| 124 | `RTLoaderBrightnessIncrease` | BOOL | — | key b pressed | — | — | — |
| 125 | `RTLoaderBrightnessDecrease` | BOOL | — | key v pressed | — | — | — |
| 126 | `RTLoaderResolutionIncrease` | BOOL | — | key x down | — | — | — |
| 127 | `RTLoaderResolutionDecrease` | BOOL | — | key c down | — | — | — |
| 128 | `RTLoaderCameraMode1` | BOOL | — | key 1 pressed | — | — | — |
| 129 | `RTLoaderCameraMode2` | BOOL | — | key 2 pressed | — | — | — |
| 130 | `RTLoaderCameraMode3` | BOOL | — | key 3 pressed | — | — | — |
| 131 | `RTLoaderLMB` | BOOL | — | mouse left pressed \| mouse right pressed | — | — | — |
| 132 | `RTLoaderRMB` | BOOL | — | — | — | — | — |
| 133 | `StartAttractMode` | BOOL | Activate Attract Mode | — | — | — | attract.cpp |
| 134 | `StopAttractMode` | BOOL | Deactivate Attract Mode | key space down \| key enter down \| key esc down ; other: idle no longer | XInput start down \| XInput b down | — | attract.cpp |
| 135 | `CameraFreeMovement` | BOOL | Change to free from first person | key f2 pressed | XInput rightthumbstick pressed | — | camera.cpp |
| 136 | `DebugCameraFixUp` | BOOL | — | key g pressed | — | — | — |
| 137 | `SkipTutorial` | BOOL | — | key t pressed | — | — | — |
| 138 | `GamePause` | BOOL | — | key pause down | — | — | user_input.cpp |
| 139 | `ToggleEditor` | BOOL | — | key f9 down | — | — | — |
| 140 | `CreateScreenShot` | BOOL | — | key p down | — | — | renderer.cpp |
| 141 | `ToggleCheatMenu` | BOOL | — | key f11 down | — | — | — |
| 142 | `MouseMovement` | 2D | — | — | — | — | — |
| 143 | `IconsTaskManagerQuickUnit` | BOOL | — | — | — | — | — |
| 144 | `IconsTaskManagerQuickUnitCreate` | BOOL | — | — | — | — | — |
| 145 | `IconsTaskManagerQuickUnitLeft` | BOOL | — | — | XInput dpadleft down | — | location_input.cpp |
| 146 | `IconsTaskManagerQuickUnitRight` | BOOL | — | — | XInput dpadright down | — | location_input.cpp |
| 147 | `IconsTaskManagerQuickUnitUp` | BOOL | — | — | XInput dpadup down | — | location_input.cpp |
| 148 | `IconsTaskManagerQuickUnitDown` | BOOL | — | — | XInput dpaddown down | — | location_input.cpp |
| 149 | `CameraSquadZoomIn` | BOOL | — | — | XInput dpadup pressed | — | camera.cpp |
| 150 | `CameraSquadZoomOut` | BOOL | — | — | XInput dpaddown pressed | — | camera.cpp |
| 151 | `ControllerUnplugged` | BOOL | — | — | XInput plug up | — | main.cpp |
| 152 | `ControllerPlugged` | BOOL | — | — | XInput plug down | — | main.cpp |
| 153 | `ToggleNetworkStats` | BOOL | — | key f9 down | — | — | user_input.cpp |
| 154 | `ToggleInterface` | BOOL | — | key alt pressed && key x down | — | — | main.cpp |
| 155 | `GameDebugQuit` | BOOL | — | — | — | — | main.cpp |
| 156 | `ToggleReady` | BOOL | — | key space down | XInput a down \| XInput b down | — | location_input.cpp |
| 157 | `ShowMouseCursor` | BOOL | — | — | XInput leftbumper pressed | — | — |
| 158 | `MainMenuActivate` | BOOL | — | — | XInput start down | — | interface/darwinia_window.cpp |
| 159 | `CloseTaskHelp` | BOOL | — | mouse left down \| key space down \| key enter down \| key esc down | XInput a down | — | UI/GameMenuWindow.cpp, interface/mainmenus.cpp, taskmanager_interface_multiwinia.cpp |
| 160 | `TaskManagerScrollUp` | BOOL | — | key w down | — | — | taskmanager_interface_icons.cpp |
| 161 | `TaskManagerScrollDown` | BOOL | — | key s down | — | — | taskmanager_interface_icons.cpp |
| 162 | `ToggleRendering` | BOOL | — | key r down | — | KB/M: (none) ; pad: same | — |
| 163 | `LobbyChat` | BOOL | — | key enter down | — | KB/M: (none) ; pad: same | UI/GameMenuWindow.cpp, location_input.cpp |


#### Icons (`~` lines) in input_preferences.txt (last per control wins)

| Control | Icon |
|---|---|
| `MenuLeft` | `icons/button_dpad_lr.bmp` |
| `MenuActivate` | `icons/button_a.bmp` |
| `CameraMove` | `icons/button_la.bmp` |
| `CameraUp` | `icons/button_la.bmp` |
| `CameraDown` | `icons/button_ra.bmp` |
| `CameraRotate` | `icons/button_lt_ra.bmp` |
| `TargetMove` | `icons/button_ra.bmp` |
| `UnitSelect` | `icons/button_a.bmp` |
| `UnitDeselect` | `icons/button_b.bmp` |
| `UnitCycleLeft` | `icons/button_lb.bmp` |
| `UnitCycleRight` | `icons/button_rb.bmp` |
| `UnitSetTarget` | `icons/button_x.bmp` |
| `UnitMove` | `icons/button_la.bmp` |
| `UnitStartSecondaryFireDirected` | `icons/button_rt.bmp` |
| `UnitSecondaryFireDirected` | `icons/button_lt.bmp` |
| `UnitSecondaryAim` | `icons/button_rt_ra.bmp` |
| `IconsTaskManagerEndTask` | `icons/button_y.bmp` |
| `IconsTaskManagerDisplay` | `icons/button_x.bmp` |

### Lines of the shipped preference files that do not create a binding

| File:line | Control | Op | Spec | Status | Reason |
|---|---|---|---|---|---|
| `input_preferences.txt:16` | `LeaderboardLTrigger` | `=` | `XInput lefttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences.txt:17` | `LeaderboardRTrigger` | `=` | `XInput righttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences.txt:106` | `UnitDeselect` | `=` | `SpaceNav rightbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences.txt:110` | `UnitCycleRight` | `=` | `SpaceNav leftbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences.txt:137` | `UnitSecondaryFireTarget` | `=` | `XInput lefttrigger down` | BOUND | unknown condition "down" silently parsed as COND_DOWN with type 0 |
| `input_preferences.txt:142` | `UnitPrimaryFireDirected` | `~` | `icons/button_ra.bmp` | IGNORED | Control ID not found |
| `input_preferences.txt:165` | `TaskManagerSwitchTask` | `INPUT_TYPE_BOOL` | `` | IGNORED | Assignment not found (no "=" or "~") |
| `input_preferences.txt:169` | `GesturesTaskManagerEndTask` | `=` | `key control ++ key c` | BOUND | expanded to 2 conjunctions (one part "down", others "pressed") |
| `input_preferences.txt:178` | `GestureLeft` | `=` | `XInput dpadleft donw` | BOUND | unknown condition "donw" silently parsed as COND_DOWN with type 0 |
| `input_preferences.txt:309` | `ToggleReady` | `=` | `SpaceNav rightbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences.txt:310` | `ToggleReady` | `=` | `SpaceNav leftbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences.txt:317` | `EclipseLMouseDown` | `=` | `alias ShowMouseCursor && XInput a down` | BOUND | alias of ShowMouseCursor |
| `input_preferences.txt:318` | `EclipseLMousePressed` | `=` | `alias ShowMouseCursor && XInput a pressed` | BOUND | alias of ShowMouseCursor |
| `input_preferences_darwinia.txt:21` | `LeaderboardLTrigger` | `=` | `XInput lefttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences_darwinia.txt:22` | `LeaderboardRTrigger` | `=` | `XInput righttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences_darwinia.txt:136` | `UnitSecondaryFireTarget` | `=` | `XInput lefttrigger down` | BOUND | unknown condition "down" silently parsed as COND_DOWN with type 0 |
| `input_preferences_darwinia.txt:141` | `UnitPrimaryFireDirected` | `~` | `icons/button_ra.bmp` | IGNORED | Control ID not found |
| `input_preferences_darwinia.txt:164` | `TaskManagerSwitchTask` | `INPUT_TYPE_BOOL` | `` | IGNORED | Assignment not found (no "=" or "~") |
| `input_preferences_darwinia.txt:168` | `GesturesTaskManagerEndTask` | `=` | `key control ++ key c` | BOUND | expanded to 2 conjunctions (one part "down", others "pressed") |
| `input_preferences_darwinia.txt:177` | `GestureLeft` | `=` | `XInput dpadleft donw` | BOUND | unknown condition "donw" silently parsed as COND_DOWN with type 0 |
| `input_preferences_darwinia.txt:312` | `EclipseLMouseDown` | `=` | `alias ShowMouseCursor && XInput a down` | BOUND | alias of ShowMouseCursor |
| `input_preferences_darwinia.txt:313` | `EclipseLMousePressed` | `=` | `alias ShowMouseCursor && XInput a pressed` | BOUND | alias of ShowMouseCursor |
| `input_preferences_mac.txt:16` | `LeaderboardLTrigger` | `=` | `XInput lefttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences_mac.txt:17` | `LeaderboardRTrigger` | `=` | `XInput righttrigger repeat` | BOUND | unknown condition "repeat" silently parsed as COND_DOWN with type 0 |
| `input_preferences_mac.txt:99` | `OrderDirectRoute` | `=` | `key meta pressed` | PARSE_FAIL | unknown key "meta" |
| `input_preferences_mac.txt:108` | `UnitDeselect` | `=` | `SpaceNav rightbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences_mac.txt:112` | `UnitCycleRight` | `=` | `SpaceNav leftbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences_mac.txt:143` | `UnitSecondaryFireTarget` | `=` | `key meta down && mouse left pressed` | PARSE_FAIL | part "key meta down" failed: unknown key "meta" |
| `input_preferences_mac.txt:144` | `UnitSecondaryFireTarget` | `=` | `XInput lefttrigger down` | BOUND | unknown condition "down" silently parsed as COND_DOWN with type 0 |
| `input_preferences_mac.txt:149` | `UnitPrimaryFireDirected` | `~` | `icons/button_ra.bmp` | IGNORED | Control ID not found |
| `input_preferences_mac.txt:172` | `TaskManagerSwitchTask` | `INPUT_TYPE_BOOL` | `` | IGNORED | Assignment not found (no "=" or "~") |
| `input_preferences_mac.txt:176` | `GesturesTaskManagerEndTask` | `=` | `key control ++ key c` | BOUND | expanded to 2 conjunctions (one part "down", others "pressed") |
| `input_preferences_mac.txt:185` | `GestureLeft` | `=` | `XInput dpadleft donw` | BOUND | unknown condition "donw" silently parsed as COND_DOWN with type 0 |
| `input_preferences_mac.txt:316` | `ToggleReady` | `=` | `SpaceNav rightbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences_mac.txt:317` | `ToggleReady` | `=` | `SpaceNav leftbutton down` | PARSE_FAIL | no driver accepts "SpaceNav" |
| `input_preferences_mac.txt:324` | `EclipseLMouseDown` | `=` | `alias ShowMouseCursor && XInput a down` | BOUND | alias of ShowMouseCursor |
| `input_preferences_mac.txt:325` | `EclipseLMousePressed` | `=` | `alias ShowMouseCursor && XInput a pressed` | BOUND | alias of ShowMouseCursor |

### 2.3 Notes on the table [C unless marked]

1. **Control schemes.** Two orthogonal preferences select controls: `MultiwiniaControlMethod` (0 = Darwinia-like: left mouse sets unit targets/orders; 1 = default RTS-like: right mouse) picks the system binding file [C] `code/lib/input/file_paths.cpp:InputPrefs::GetSystemPrefsPath`; `ControlMethod` (0 = gestures task manager, 1 = icon task manager; default 1 [C] `data/default_preferences.txt`) chooses which of the parallel `Gestures*`/`Icons*` controls are consulted, e.g. the chat log key [C] `code/camera.cpp:Camera::ChatLogVisible`.
2. **Darwinia-only bindings** (present only in `input_preferences_darwinia.txt`): `MenuLeft = key left down`, `MenuRight = key right down`, `MenuUp = key up down`, `MenuDown = key down down`, `MenuActivate = key enter down`; `UnitSetTarget = mouse left pressed`, `UnitEndSetTarget = mouse left up`, `IssueDarwinianOrders = mouse left down` (instead of the right-button versions); `IssueSpecialOrders = XInput x down` (instead of `a`); `UnitSelect = XInput a pressed` (instead of `down`). **Absent** in the Darwinia file: `OrderSpecialMove = XInput x down`, `LeftButtonPressed`, `LeftButtonDown`, `LobbyChat`, `ToggleRendering`, and the four `SpaceNav` lines.
3. **Multiwinia-default-only bindings** (main file, not Darwinia file): `LeftButtonPressed = mouse left pressed` (squad direct-move in `TeamControls`, [C] `code/team.cpp`), `LeftButtonDown`, `LobbyChat = key enter down`, `ToggleRendering = key r down` (`TESTBED_ENABLED` only), `OrderSpecialMove = XInput x down`.
4. **Mac file** (`input_preferences_mac.txt`, [B:DEAD] on PC) adds Ctrl+left-click equivalents for every right-button action (`DarwinianPromote`, `IssueSpecialOrders`, `UnitSetTarget`, `UnitEndSetTarget`, `IssueDarwinianOrders`, `UnitPrimaryFireTarget`) and moves `OrderDirectRoute` to `key meta pressed` and `UnitSecondaryFireTarget` to `key meta down && mouse left pressed` — both of which **fail to parse** because `META` is outside the key-lookup range (§1.6.5).
5. **Keyboard-layout overrides** (primary binding replaced): de_DE: `CameraZoom = key y pressed` (others restated unchanged); fr_FR: `CameraLeft = key q pressed`, `CameraUp = key a pressed`, `CameraForwards = key z pressed`, `CameraZoom = key w pressed` (others restated). The secondary arrow-key bindings remain.
6. **Duplicate physical inputs** (simultaneous meanings are resolved by consumers, not by the input layer): F9 → `ToggleEditor` (dead) and `ToggleNetworkStats`; Tab → `UnitCycleRight`, `GesturesSwitchUnit`, `IconsTaskManagerDisplay`; Space → `UnitDeselect`, `CloseTaskHelp`, `ToggleReady`, `StopAttractMode`; right mouse → `CameraSlowdown`, `CameraDebugRotate`, `DarwinianPromote`, `IssueSpecialOrders`, `UnitSetTarget`, `IssueDarwinianOrders`, `UnitPrimaryFireTarget`, `EclipseRMousePressed`; Shift → `CameraSpeedup`, `OrderSpecialMove`, `ScrollSpeedup`; P → `CreateScreenShot` (and loader keys); left trigger → `CameraDown`, `CameraRotate`, `CameraRotateLeft`, `UnitSecondaryFireTarget`; right trigger → `CameraUp`, `CameraRotateRight`; X button → `DarwinianPromote`, `IssueDarwinianOrders`, `OrderSpecialMove`, `UnitSetTarget`, `GesturesTaskManagerDisplay`; right-stick click → `CameraZoom`, `CameraFreeMovement`.
7. **Failed / ignored lines in the PC file**: `UnitDeselect = SpaceNav rightbutton down`, `UnitCycleRight = SpaceNav leftbutton down`, `ToggleReady = SpaceNav rightbutton down`, `ToggleReady = SpaceNav leftbutton down` (no SpaceNav driver exists in this tree — a 3D-mouse driver evidently removed); `UnitPrimaryFireDirected ~ icons/button_ra.bmp` (no such control); `TaskManagerSwitchTask INPUT_TYPE_BOOL` (no `=`).
8. **Controls with no binding** in the PC file: `CameraZoomChange`, `CameraSwitchMode` (commented out), `UnitSecondaryAim`, `UnitStartSecondaryFireDirected`, `UnitSecondaryFireDirected` (icons only), `TaskManagerSelectNextTask`, `TaskManagerDisplay`, `TaskManagerHide`, `TaskManagerSwitchTask`, `TaskManagerEndTask`, `RTLoaderMotionBlurDecrease`, `RTLoaderRMB`, `StartAttractMode`, `MouseMovement`, `IconsTaskManagerQuickUnit`, `IconsTaskManagerQuickUnitCreate`, `GameDebugQuit` (17).
9. **Bound but unread in the FULL build** (dead controls): `CameraDeZoom`, `CameraRotate`, `LeftButtonDown`, `WeaponCycleLeft`, `WeaponCycleRight`, `GesturesTaskManagerHide`, `TileSelect`/`TileDrop`/`TileDrag` (`LOCATION_EDITOR`), `DebugDropPacket` (`TARGET_DEBUG`), `LoaderHelp` + all `GOLLoader*`/`RTLoader*` (`USE_LOADERS`), `SepulvedaHelp` (`USE_SEPULVEDA_HELP_TUTORIAL`), `DebugCameraFixUp` (`_DEBUG`), `SkipTutorial` (`USE_SEPULVEDA_HELP_TUTORIAL`), `ToggleEditor` (commented out under `LOCATION_EDITOR`), `ToggleCheatMenu` (`CHEATMENU_ENABLED`), `ToggleRendering` (`TESTBED_ENABLED`), `StopAttractMode`/`StartAttractMode` (read only in `AttractMode::Advance`, which the main loop calls only under `ATTRACTMODE_ENABLED` [C] `code/main.cpp`, `code/attract.cpp:AttractMode::Advance`) — plus the unbound ones in note 8; of those, `UnitSecondaryFireDirected`, `TaskManagerEndTask` and `GameDebugQuit` are read by live code but can never be satisfied with the shipped files.
10. **Gamepad-only controls** (no KB/M binding): all `Menu*` navigation except `MenuEscape`, `MenuClose`, `MainMenuActivate`, `Leaderboard*`, `XBoxMenuCycle*`, `CameraMove`, `TargetMoveSecondary`, `CameraRotate*`, `UnitMove`, `UnitMoveDirectionChange`, `UnitTargetDirected`, `WeaponCycle*`, `IconsTaskManagerDisplayDown/Pressed/Up`, `IconsTaskManagerQuickUnit{Left,Right,Up,Down}`, `CameraSquadZoomIn/Out`, `ControllerPlugged/Unplugged`, `ShowMouseCursor`. On keyboard, menu navigation is mouse-driven (Eclipse windows), and the camera/unit analogue controls have digital equivalents (`CameraLeft`…).
11. **Controller hot-plug** [C] `code/main.cpp:HandleCommonConditions`: `ControllerUnplugged` (`XInput plug up`) opens the `controller_unplugged` dialog once; `ControllerPlugged` (`XInput plug down`) closes it; while unplugged and the input mode is still GAMEPAD the app treats itself as unfocused (`m_lostFocusPause`). `ControlGameDebugQuit` would set `m_requestQuit` but is unbound.
12. **Analogue payload conventions** consumed elsewhere: `CameraMove`/`UnitMove`/`TargetMoveSecondary`/`UnitTargetDirected` deliver the scaled left/right stick (x right-positive, **y up-negative**, ±39 left stick, ±19 right stick at sensitivity 100); `TargetMove` delivers mouse pixels/frame; `TargetMoveZ` wheel notches/frame (forward = +1). `CameraUp`/`CameraDown` on triggers are BOOL controls fed by 1D specs (true while the trigger is outside its threshold).

## [→ 06_player_control_and_input] 3. Camera

### 3.1 Model and conventions [C] `code/camera.h:Camera`, `code/camera.cpp`

- One global camera `g_app->m_camera` (local, per machine, never networked). World is Y-up; sea level y = 0; units = world units ("metres" in comments).
- Orientation: `m_front` (unit view direction) and `m_up`; re-orthonormalised every frame by `Normalise()` (`right = up × front`, `up = front × right`). **`GetRight()` returns `m_up × m_front`, which with this handedness points to screen-left**; this explains the sign conventions below (e.g. "CameraLeft adds `GetRight()`"). `GetUp()` returns world up if `m_up` is zero.
- Key state: `m_pos`, `m_targetPos` (mode-specific goal point), `m_targetFront/m_targetUp` (goal orientation for cuts/moves), `m_height` (desired absolute y in free movement / height parameter in aim modes), `m_fov`/`m_targetFov` (vertical FOV, degrees), `m_mode`, `m_debugMode`, `m_framesInThisMode`, `m_vel`, `m_cameraShake`, animation cursor (`m_anim`, `m_animCurrentNode`, `m_animNodeStartTime`, `m_modeBeforeAnim`, `m_pos/front/upBeforeAnim`), entity-tracking state (`m_trackingEntity(Id)`, `m_trackingCameraDist`, `m_currentEntityTrackPos`, `m_lastEntityTrackPos`, `m_wantedPos`, `m_lastSquadieId`, `m_cameraTarget`, `m_predictedEntityPos`, `m_resettingCameraPos`, `m_controlVector`, `m_skipDirectionCalculation`), flags `m_entityTrack`, `m_camLookAround`.
- Constructor defaults [C] `code/camera.cpp:Camera::Camera`: fov = targetFov = 60°, height 50, pos (1000, 500, 1000), front = normalise(0, −0.5, −1), up orthogonalised from world up, mode `ModeDoNothing`, debug mode `DebugModeAuto`, `m_distFromEntity` 100, `m_trackingCameraDist` 250 (**int**), `m_heightMultiplier` 1, `m_targettingFOVAngle` 0.939692621 (cos 20°, "40° FOV" cone), bounds ±1e6.
- Timing: all camera motion uses the **render-frame** delta `g_advanceTime` (wall clock, capped at 0.25 s [C] `code/main.cpp:UpdateAdvanceTime`) or direct `GetHighResTime()` deltas; `g_gameTime` (= wall clock in normal play) for animations and oscillations. The camera is advanced once per rendered frame, only while the window has focus [C] `code/main.cpp:LocationGameLoop`, plus an extra `Advance()` when the icon task manager closes [C] `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::HideTaskManager`. Not advanced in the main-menu loop (call commented out) [C] `code/main.cpp:MainMenuLoop`.
- Most "smoothing" is first-order exponential: `x = x·(1−k·dt) + target·k·dt` with no clamp of `k·dt` (see constants) — at low frame rates `k·dt` can exceed 1 and overshoot.

### 3.2 Per-frame `Camera::Advance` [C] `code/camera.cpp:Camera::Advance`

```mermaid
flowchart TD
  A[Advance] --> B{m_anim?}
  B -- yes --> B1[AdvanceAnim: node timing / transitions]
  B --> C{!m_camLookAround && mode != EntityTrack}
  B1 --> C
  C -- yes --> C1[AdvanceComponentZoom: FOV easing]
  C --> D{mode in SphereWorld, FreeMovement, BuildingFocus,<br/>EntityTrack, RadarAim, FirstPerson, TurretAim}
  C1 --> D
  D -- yes --> D1[AdvanceComponentMouseWheelHeight: m_height]
  D --> E{no anim AND (debug==Always OR (debug==Auto AND any Eclipse window open) OR framesInThisMode < 2)}
  D1 --> E
  E -- yes --> E1[uncapture mouse; AdvanceDebugMode]
  E -- no --> E2[capture mouse (uncapture if GameModeNone); per-mode advancer;<br/>location modes only if landscape heightmap exists]
  E1 --> F[camera shake]
  E2 --> F
  F --> G[Normalise; assert sane]
  G --> H[UserInput::RecalcMousePos3d]
  H --> I[framesInThisMode++]
```

Every mode request resets `m_framesInThisMode = 0`, so **the first two frames after any mode change run the debug advancer** (which, with no window open, lets the camera keys fly the camera at world-size/30 per second) [C].

### 3.3 Mode enumeration [C] `code/camera.h:Camera::Mode`

| # | Mode | Advancer | Entered by (live FULL code) | Build / reachability | `IsInteractive` | Wheel-height component |
|---|---|---|---|---|---|---|
| 0 | `ModeReplay` | none (no `case`) | only `code/lib/input_sdl.cpp` (not built) | [B:DEAD] | no | no |
| 1 | `ModeSphereWorld` | `AdvanceSphereWorldMode` | `EnterGlobalWorld`, script `CamReset` outside a location, end of outro | [B:LEGACY] Darwinia global world | yes | yes |
| 2 | `ModeFreeMovement` | `AdvanceFreeMovementMode` | `EnterLocation`; deselect (`UnitDeselect`), terminate turret/officer, task switch out of aim modes, entity-track exit, turret lost, script `CamReset` | [B:FULL] **the normal Multiwinia camera** | yes | yes |
| 3 | `ModeBuildingFocus` | `AdvanceBuildingFocusMode` | script `CamBuildingFocus`; task-manager hooks under `USE_SEPULVEDA_HELP_TUTORIAL` | [B:LEGACY] | no | yes |
| 4 | `ModeEntityTrack` | `AdvanceEntityTrackMode` | automatic from free movement (squad selected + tracking on); `Task::Start` when `AutomaticCamera` = 2 or gamepad ([C] `code/taskmanager.cpp`) | [B:FULL] (gamepad default / pref) | yes | yes |
| 5 | `ModeRadarAim` | `AdvanceRadarAimMode` | selecting own radar dish (`RequestRadarAimMode`, unless level option `NoRadarControl`) [C] `code/location_input.cpp` | [B:FULL] | yes | yes (clamped 50–120) |
| 6 | `ModeFirstPerson` | `AdvanceFirstPersonMode` | nobody | [B:LEGACY] unreachable | no | yes |
| 7 | `ModeMoveToTarget` | `AdvanceMoveToTargetMode` | camera animations, `RestoreCameraPosition(false)`, script `CamMove`/`CamBuildingApproach` | [B:LEGACY] (no shipped animation/script use in Multiwinia, §3.8) | no | no |
| 8 | `ModeDoNothing` | none (static) | constructor; end of MoveToTarget; `Cut` animation nodes | [B:FULL] (initial) | no | no |
| 9 | `ModeEntityFollow` | `AdvanceEntityFollowMode` | nobody (`RequestEntityFollowMode` has no caller) | [B:LEGACY] unreachable | no | no |
| 10 | `ModeTurretAim` | `AdvanceTurretAimMode` | selecting own gun turret / chess piece [C] `code/location_input.cpp`; `Team` when current building is a turret [C] `code/team.cpp`; task-manager | [B:FULL] | yes | yes (clamped 10–50) |
| 11 | `ModeSphereWorldScripted` | `AdvanceSphereWorldScriptedMode` | script `CamLocationFocus` | [B:LEGACY] | no | no |
| 12 | `ModeSphereWorldIntro` | `AdvanceSphereWorldIntroMode` | `StartSequence::Advance` [C] `code/startsequence.cpp`; the sequence is only created by `RunBootloader`, which is compiled only under `USE_LOADERS` (off) [C] `code/main.cpp:RunBootloader` | [B:LEGACY] unreachable | no | no |
| 13 | `ModeSphereWorldOutro` | `AdvanceSphereWorldOutroMode` | script (`code/script.cpp`) | [B:LEGACY] | no | no |
| 14 | `ModeSphereWorldFocus` | `AdvanceSphereWorldFocusMode` | main menu (`GameMenu::CreateMenu` via plain `RequestMode`, then `SetTarget`+`CutToTarget`; but the menu loop never advances the camera), script `CamGlobalWorldFocus` | [B:LEGACY] | no | no |

`IsMoving()` ⇔ MoveToTarget; `IsAnimPlaying()` ⇔ animation active or MoveToTarget.

Debug sub-modes (`m_debugMode`): `DebugModeNever` 0, `DebugModeAlways` 1 (editors), `DebugModeAuto` 2 (game: debug advancer only while an Eclipse window is open). `SetNextDebugMode` cycles 0→1→2→0 (debug menu) [C].

### 3.4 Common components

**Zoom (FOV)** [C] `Camera::AdvanceComponentZoom` — skipped while the task manager is visible, in EntityTrack, or while `m_camLookAround`:
- `target = m_targetFov`; rate `change = 30`.
- While `CameraZoom` held and no Eclipse window open: `target /= 4` (60° → 15°) and `change = 100`.
- `change = 10` in SphereWorldScripted; `change = 1` in MoveToTarget, DoNothing, BuildingFocus.
- `k = dt · change · 0.1`; `fov = fov·(1−k) + target·k` (time constant 1/3 s normal, 0.1 s zooming).
- `SetFOV` sets both fov and target; `SetTargetFOV` only the target. Mode targets: FreeMovement 60 (set on request), TurretAim 50 (every frame), SphereWorld 100, SphereWorldFocus 100, Intro/Outro 70, editing 60.
- `CameraDeZoom` (key C) is bound but never read: there is no zoom-out beyond the base FOV.
- Projection clamps fov to [1, 180] (§5).

**Height (mouse wheel / camera up-down)** [C] `Camera::AdvanceComponentMouseWheelHeight`:
- `delta = TargetCursor.dZ` (wheel notches this frame, forward positive). If no Eclipse window is open: `CameraUp` (Q / right trigger) adds `7·dt`, `CameraDown` (E / left trigger) subtracts `7·dt`, both suppressed while the current building is a gun turret or radar dish (`AimingBuilding()`).
- In a location: `land = max(heightmap(camera.xz), MIN_HEIGHT=10)`; `m_height += delta · 2 · sqrt(|m_height − land|)` (speed proportional to √altitude). Then: TurretAim → clamp [10, 50]; RadarAim → clamp [50, 120]; other modes → `m_height ≥ land + MIN_GROUND_CLEARANCE(10)`. All: `m_height ≤ MAX_HEIGHT = 5000`.
- Outside a location (global world): `m_height += delta·(0.1·m_height + 5)`; `<0 → 0.1`, `>1000 → 1000`.
- So holding Q at 60 fps rises `14·√alt·dt` per second, e.g. ≈ 140 u/s at 100 u altitude; one wheel notch at 100 u altitude = +20 u.

**Camera shake** [C] `Camera::CreateCameraShake`, `Camera::Advance`: `shake = max(shake, intensity)`; per frame while > 0: yaw `front` by `sfrand(0.05·shake)` rad (uniform in ±0.05·shake), build `up` = world up rolled about front by `0.3·sfrand(shake)` rad, re-orthogonalise, `shake −= dt` (linear decay, 1 unit/s). Sources (other partitions): portal 1.0, rocket (distance-scaled), weapons 0.5 and alpha-scaled [C] `code/worldobject/portal.cpp`, `rocket.cpp`, `weapons.cpp`. Uses the **unsynchronised** RNG `sfrand` [C] `contrib/systemiv/lib/math/random_number.h`.

**Cursor lock (shared by free movement, radar/turret aim, sphere world)**: before moving, project the 3D point under the cursor (`UserInput::GetMousePos3d`) to screen; after moving, re-project it; add the pixel difference (accumulated in static sub-pixel remainders, floored) to the virtual cursor and clamp to [0, W−1]×[0, H−1] — the cursor stays glued to the same ground point while the camera moves [C] `code/camera.cpp:Camera::AdvanceFreeMovementMode`.

### 3.5 `ModeFreeMovement` — the Multiwinia camera [B:FULL] [C] `Camera::AdvanceFreeMovementMode`, `Camera::RequestMode`

On request: `m_targetPos = m_pos`, `m_height = m_pos.y`, `m_targetFov = 60`, cursor to screen centre.

Each frame (skipped while rendering a poster):
1. `UpdateEntityTrackingMode()` (§3.6). If tracking is on and `GetEntityToTrack` yields an insertion-squad member → `RequestEntityTrackMode(id)` and return.
2. Set up matrices; record cursor ground point.
3. If neither task manager nor shaman interface is visible:
   - `moveRate = 250` u/s, ×4 (= 1000) while `CameraSpeedup` (Shift / L3). Basis vectors: `fwd` = front projected to XZ, normalised; `left` = `GetRight()` projected to XZ.
   - Local timer `dt' = now − last` (own static, clamped ≤ 1.0 s — independent of `g_advanceTime`).
   - Keys: `CameraLeft` → `target += left·dt'·rate`; `CameraRight` → `−left`; `CameraForwards` → `+fwd`; `CameraBackwards` → `−fwd`.
   - Pad: `CameraMove` (left stick, ±39) → `target −= left·dt'·x·10; target −= fwd·dt'·y·10` (stick up = negative y = forward; max ≈ 393 u/s; no speed-up multiplier) and records `CondMoveCameraOrUnit` for control help.
   - If only Forwards has been held (no other direction, no Shift) for > 4 s → `MultiwiniaHelp::ShowShiftSpeedupHelp()` (3 s hint) [C] `code/multiwiniahelp.cpp`.
   - **Cliff avoidance**: `dir` = horizontal direction to target; `d = DistanceToBlockage(dir, 100)` = first of 40 samples (2.5 u steps) where `land + 10 > m_height` (else 100). If `d < 100`: `s = max(0, (d − 30)/70)`; `target = pos + (target − pos)·s`; `m_height += (1 − s)·|horizontal(target − pos)|` (the camera climbs instead of hitting the slope).
   - **Ground clearance**: `land` = max heightmap at target and at (±10, ±10) around it, ≥ 10. If `land + 10 > m_height`: `target.y = m_height = land + 10`; else `target.y = m_height`.
   - **Position**: unless `m_camLookAround`, `pos = pos·(1−4dt') + target·4dt'`; `m_vel = Δpos/dt'`.
   - **Orientation (mouse-look)**: if task manager hidden and chat log not held: if the cursor ground point exists, rotate `front` towards `normalise(cursor3D − pos)` about axis `front × desired` by `|front × desired|·min(1, √dt')` radians; rebuild `up` from world up. **The camera always turns to face the point under the cursor** — moving the mouse (or right stick via `TargetMoveSecondary`) aims the camera ("CondCameraAim"); the cursor-lock then keeps the cursor on that ground point. There is **no edge-scrolling and no rotate/orbit key** in this mode.
   - Cursor lock (§3.4).
4. Bounds: `SetBounds(−200, sizeX+200, −200, sizeX+200)` is called on level load [C] `code/main.cpp:DoEnterLocation` (note: X size reused for Z), but the clamp code is commented out — **horizontal movement is unbounded** [C].

Chat log (`ChatLogVisible()`): true while `IconsChatLog` (ControlMethod 1) or `GesturesChatLog` (ControlMethod 0) — key L — is held and no Eclipse window is open; it freezes mouse-look (and the sphere-world camera) [C] `Camera::ChatLogVisible`.

Holding `CameraFreeMovement` (F2 / R3) without `UnitPrimaryFireTarget` sets `m_camLookAround`: position frozen, zoom component skipped, orientation still follows the cursor, and entity tracking is suspended [C] `Camera::SwitchEntityTracking`.

### 3.6 `ModeEntityTrack` and automatic tracking [B:FULL] [C] `Camera::AdvanceEntityTrackMode`, `AdvanceAutomaticTracking`, `AdvanceCameraMoveBehind`, `RotateTowardsEntity`, `UpdateControlVector`, `UpdateEntityTrackingMode`, `GetEntityToTrack`

Tracking switch: preference `AutomaticCamera` (`OTHER_AUTOMATICCAM`, default 0; no widget sets it in the shipped options window [C] `code/interface/prefs_other_window.cpp`): 1 = never, 2 = always, other = automatic → on iff input mode is GAMEPAD. `m_entityTrack = (value==2) && !m_camLookAround`.

`GetEntityToTrack`: false while `CameraFreeMovement` held or no location/team; else my team's selected entity (`GetMyEntity`, e.g. officer/armour) if any; else, if the current task is not stopping and the selected unit is an insertion squad, its point man.

`RequestEntityTrackMode(id)`: `m_trackingCameraDist = 250`, `m_distFromEntity = 200`, clears targeting state, `m_cameraTarget = m_pos`, `m_targetPos = entity.pos`.

Per frame (returns immediately while the task manager is visible):
1. Re-evaluate tracking; exit to FreeMovement if tracking off, entity missing/dead, current task not Running, no task and entity not an officer, or invalid id.
2. `predicted = pos + g_predictionTime·vel`.
3. `AdvanceAutomaticTracking`:
   - `CameraZoom` (R3 / Z / middle mouse) latches `m_resettingCameraPos` (snap behind).
   - `CameraSquadZoomIn/Out` (d-pad up/down held): `dist = clamp(dist ∓ 10·dt, 50, 500)` — but `m_trackingCameraDist` is an **int**, so the float result truncates: zoom-in moves 1 unit per frame (frame-rate dependent) and **zoom-out never changes the value unless dt ≥ 0.1 s** [C].
   - `k = min(1, 4·dt·(3 if resetting else 1))`.
   - Move-behind (only when the tracked entity is an insertion-squad member): on a new squad member, `last = pos` (y=0). `cur = member.pos` (y=0). If `|cur − last|² > 10`: let `c = dot(norm(cur − last), norm(wanted − last))`; if not resetting and `c > 0.5` (unit moving towards the camera within ±60°) the camera translates with the unit (`target = wanted + (cur − last)`); otherwise push `cur − last` into a static 10-sample history (initially filled with copies), average it, and place the camera `dist` behind the average direction of travel (`target = cur − avĝ·dist`); `target.y = dist`; `last = cur`. `wanted = target`.
   - `cameraTarget = lerp(cameraTarget, target (or pos if no input), k)`; then snap it onto the circle of radius `dist` around `cur` (horizontal) at absolute height **y = dist** (not terrain-relative); `pos = lerp(pos, cameraTarget, k)`.
   - Reset ends when `pos − wanted < 5` on each axis (signed comparison).
   - `RotateTowardsEntity`: `k2 = min(1, 2·dt + √(ex²+ey²)/10)` where e = entity's normalised screen offset from centre; `m_targetPos` eased to entity pos; `front` eased toward it; up rebuilt.
   - `UpdateControlVector`: refresh `m_controlVector = GetRight()` unless the stick is held, no `UnitMoveDirectionChange`, and `|cos(worldUp·camUp)| ≥ 0.98` (the code takes `cos` of a dot product — effectively only when the camera looks almost straight down) — keeps stick-relative unit movement stable while the camera swings.
   - For non-squad entities (officer/armour) move-behind is skipped and the circle centre `m_currentEntityTrackPos` is stale (last squad position or origin) [C]; [I] probably untested because free movement only auto-switches for squads.
4. Virtual cursor forced to screen centre every frame.
5. `m_targetVector += (m_wantedTargetVector − m_targetVector)/4` (cosmetic aim helper).

Zoom-FOV component is disabled in this mode; wheel height runs but has no effect (position uses `dist`).

### 3.7 Building-aim modes [B:FULL]

**Radar aim** [C] `Camera::RequestRadarAimMode`, `AdvanceRadarAimMode`: target = dish position (`RadarDish::GetDishPos(0)`), `m_height = 80`. Each frame: `ground = target` with y = heightmap; `k = 4·dt`; front eased toward cursor ground point; `ideal = ground + (0, h, 0) − horiz(front)·1.5h`; `pos = lerp(pos, ideal, k)`; cursor lock. Height adjustable within [50, 120]. FOV unchanged.

**Turret aim** [C] `Camera::RequestTurretAimMode`, `AdvanceTurretAimMode`: target = turret position, `m_height = 20`, `m_objectId` = building. Each frame: exit to FreeMovement unless my team's current building is my own `TypeGunTurret`; `SetTargetFOV(50)`; `ground = target + (0,20,0) − front·h`; front eased toward cursor point (k = 4·dt); `ideal = ground + (0,h,0) − horiz(front)·0.8h`; position eased; cursor lock. Height within [10, 50]. Camera up/down keys and triggers are blocked while aiming a building (`AimingBuilding`). `Task::SwitchTo` drops back to free movement from either aim mode [C] `code/taskmanager.cpp:Task::SwitchTo`.

### 3.8 Scripted / cinematic camera [B:LEGACY in Multiwinia]

**Camera mounts** (level file `CameraMounts_StartDefinition … _EndDefinition`, one line per mount: `name px py pz fx fy fz ux uy uz`; front and up normalised on load) [C] `code/level_file.cpp:LevelFile::ParseCameraMounts`. `SetTarget(mountName)` looks a mount up case-insensitively; the magic name `CamPosBefore` (`MAGIC_MOUNT_NAME_START_POS`) means "the pose recorded before the animation started" [C] `code/level_file.h`, `code/camera.cpp:Camera::SetTarget(char const*)`.

**Multiwinia use of mounts** [B:FULL] [C] `code/main.cpp:DoEnterLocation`, `LocationGameLoop`: on load, `SetTarget((maxX,1000,maxX), front (−1,−0.7,−1))` as fallback, `SetTarget("start")`, `CutToTarget()`, `RequestMode(FreeMovement)`; then, as soon as the local team exists, `SetTarget("player<N>")` with `N = Location::GetTeamPosition(myTeamId)` and `CutToTarget()` (repeated after a level reset). Census of the 57 `data/levels/mp_*.txt`: 51 define `player0`/`player1`, 31 `player2`, 13 `player3`, 7 `start`; 6 define no player mounts (`mp_assault_3p_3`, `mp_assault_stress`, `mp_sandbox`, `mp_sandbox_all`, `mp_tankbattle_3P`, `mp_tankbattle_4P`) and keep the `start`/fallback pose [C] (script census). Level designers therefore must author one mount per player slot.

**Camera animations** (level file `CameraAnimations_StartDefinition`: `animName` then node lines `Move|Cut <mountName|CamPosBefore> <duration 0..60 s>` until `End`) [C] `code/level_file.cpp:LevelFile::ParseCameraAnims`. **No shipped level contains a non-empty animation block** [C] (census of all 88 `data/levels/*.txt`); playback is reachable only through script `CamAnim` (Darwinia scripts, P19) and the editor's camera-animation window (`LOCATION_EDITOR`).

Playback state machine [C] `Camera::PlayAnimation`, `AdvanceAnim`, `StopAnimation`:

```mermaid
stateDiagram-v2
  [*] --> Idle
  Idle --> NodeActive: PlayAnimation(anim) [nodes>0] / save mode+pose; SetTarget(node0.mount); Move→RequestMode(MoveToTarget)+SetMoveDuration(d0); Cut→RequestMode(DoNothing)
  NodeActive --> NodeActive: gameTime > start+duration AND next node exists / (if current node is Cut: CutToTarget); SetTarget(next.mount); start=now; Move→MoveToTarget(d) | Cut→DoNothing
  NodeActive --> Idle: gameTime > start+duration AND last node / (Cut: CutToTarget); RequestMode(modeBefore); m_anim=NULL
  NodeActive --> Idle: StopAnimation() / RequestMode(modeBefore)
```

Note the cut is applied when a Cut node *ends* (the camera holds its previous pose for the node's duration, then jumps) [C].

**`ModeMoveToTarget`** [C] `Camera::AdvanceMoveToTargetMode`: on request store start pos/front/up and `m_startTime = g_gameTime`. `f = max(0.2, (gameTime − start)/duration)`; `dir`,`dist` from start to target; `vmax = 2·dist/duration`; position integrates a triangular speed profile (`v = 2·vmax·f` for f < 0.5, `2·vmax·(1−f)` for f < 1). Orientation: `mid` = start front rotated about `start×target` by `0.5·|start×target|` rad (= ½·sin θ, an approximation of half the angle); blend start→mid then mid→target with `RampUpAndDown` (quadratic ease-in/out) [C] `code/lib/math_utils.cpp:RampUpAndDown`. Up forced to world up. At f ≥ 1 → `ModeDoNothing`, `front = target front` (position is not snapped). `RestoreCameraPosition(cut)` returns to the recorded pose by cut or 3 s move.

**`ModeBuildingFocus`** [C] `Camera::RequestBuildingFocusMode`, `AdvanceBuildingFocusMode`: orbit `m_targetPos = building.centrePos`, radius `|r|·(1 + 0.3 sin 0.3t)`, height `h·(1 + 0.5 sin 0.3t)` above the centre, orbit angular velocity −0.15 rad/s; ramp factor `mf = min(1, t_since_start)`; during the first 2 s lift `y += 0.75·distance·(2 − t)` capped at y 1000; `pos` eased with `0.5·mf·dt`, `front` with `mf·dt`; negative range flips the view horizontally; y ≥ land + 10.

**Sphere-world modes** (Darwinia global world; [B:LEGACY]) [C]: `SphereWorld` — fov 100; focus (0, −400h, 0); front eased (2·dt) toward cursor point; pos eased (dt) to `focus − front·30000` (|ideal| ≤ 35000); `CameraMove` stick moves the cursor; cursor lock. `SphereWorldScripted` — height 50, focus wobble (±4000 x, ±2000 y, ±4000 z at 0.5/0.4/0.3 rad/s), front eased to `m_targetPos`. `SphereWorldIntro` — fov 70, fixed start pose (−965852, 1720000, 2600000), flies toward (−1e6, 4e6, 397000) for 30 s then to (1000, 500, 1000), speed 3000×62 u/s scaled near the end, rolling. `SphereWorldOutro` — fov 70, from origin outward (speed `clamp(4√|pos|, 1000, 2000)`×62), after 70 s runs the `credits` loader then `ModeSphereWorld` at (0,0,100). `SphereWorldFocus` — fov 100, orbit radius 100000·(1+0.4 sin 0.3t) at y −110000 + 50000 sin 0.2t, look at (0, 10000, 0), up (0,−1,0)-derived. Intro/Outro `_DEBUG` reset key `DebugCameraFixUp`.

**Unreachable modes** [B:LEGACY]: `ModeFirstPerson` — exits on `CameraFreeMovement`; fires `Location::FireLaser` from (right·−2, up·−3) along `front·200` every 0.1 s while `UnitPrimaryFireTarget`; `vel *= 0.9993` per frame; mouse yaw −0.01 rad/px, pitch 0.01 rad/px; y = land + 3. `ModeEntityFollow` — cursor recentred each frame, yaw −0.015 rad/px, pitch −0.01 rad/px (pitch limited so up.y > 0.1), distance `×(1 − 0.1·dZ)` clamped [15, 5000], target eased (5·dt) to predicted entity pos, `pos = target − front·dist`.

**Debug mode** [C] `Camera::AdvanceDebugMode`: speed = `worldSizeX/30` u/s (global world: `size/30`), ×10 with `CameraSpeedup`, ×0.1 with `CameraSlowdown` (right mouse); `DPadMovement` of the six camera keys moves along `−right`, `up`, `front` only when no Eclipse window is open (or editing); `CameraDebugRotate` (right mouse) yaw `−0.005·dx` and pitch `−0.005·dy` rad. Editing forces target fov 60.

### 3.9 Unused camera helpers [C]
`AdvanceNotTooLow` (min tracking height `MIN_TRACKING_HEIGHT 200 × m_heightMultiplier`), `AdvanceCanSeeUnits` (line-of-sight via `DirectDistanceToBlockage` + `GetHighestTangentPoint`), `AdvanceNotTooFarAway` (1.2 × `m_distFromEntity`), `AdvanceManualRotateCamera` (`CameraRotateLeft/Right` ±100 px equivalent, 0.015 rad/px), `AdvanceManualCameraHeight` (`m_heightMultiplier ±0.05`, clamp [0.25, 2]) are only referenced from a commented-out block in `AdvanceAutomaticTracking`; `TargetEntity` (auto-target enemies within 225 u in a 40° cone) and `GetHighestPoint` have no live callers; `Render()` is empty; `SetCubemapSide` is declared but never defined. Consequently **`CameraRotateLeft/Right` (triggers) do nothing in the shipped build** even though they are bound and help text exists.

### 3.10 Other public services [C]
- `GetClickRay(x, y)`: `gluUnProject` at window depth 0 and 1 with the current GL matrices (y flipped: `viewport[3] − y − 1`); if the modelview looks degenerate it is reset to identity first. `Get2DScreenPos` = `gluProject`.
- `GetBestBuildingInView()`: if camera speed > 5 u/s or no location → none; otherwise at most once per 1.0 s cast the centre-screen ray and choose the nearest building hit within 200 u (static cache) [C] `Camera::GetBestBuildingInView`.
- `WaterReflect()`: negate y of pos, front, up (applied twice around the reflection pass, P11).
- `RecordCameraPosition`/`RestoreCameraPosition`, `SetTarget(focus, distance, height)` (place on the current bearing at `distance`, `height` above the focus, looking at it), `CutToTarget` (pose = target, `m_height = target.y`).

## [→ 24_rendering] Camera projection, view and frustum

All [B:FULL] (identical under [B:D3D], where the GL calls are emulated).

| Parameter | Value | Ref |
|---|---|---|
| Projection | `gluPerspective(fovY = camera.m_fov degrees, aspect = screenW/screenH, near, far)` — FOV is **vertical**; horizontal FOV follows aspect | [C] `code/renderer.cpp:Renderer::SetupProjMatrixFor3D` |
| FOV clamp | `clamp(m_fov, 1, 180)` (macro modifies in place) before every projection set-up by the camera | [C] `code/camera.cpp:Camera::SetupProjectionMatrix`; `code/lib/math_utils.h:clamp` |
| Default FOV | 60° (free movement); 15° while zooming (60/4); 50° turret aim; 100° sphere world; 70° intro/outro; 90° set by main menu renderer | §3.4; [C] `code/game_menu.cpp` |
| Near / far in a location | **5 / 15 000** world units, re-set every frame in `RenderFrame` | [C] `code/renderer.cpp:Renderer::RenderFrame` |
| Near / far outside a location (global world / menu) | 50 / 10 000 000 | same |
| Initial near / far | 5 / 150 000 (constructor) | [C] `code/renderer.cpp:Renderer::Renderer` |
| Poster tiling | when `m_renderingPoster == PosterMakerTiling`: `scale = fov/78`, `left = −scale·near`, `top = −(H/W)·scale·near`, split into `RenderPosterResolution`² tiles, `glFrustum` per tile | [C] `code/renderer.cpp:Renderer::SetupProjMatrixFor3D` |
| View matrix | `gluLookAt(pos, pos + front·|pos|, up·|pos|)` (vectors scaled by distance from origin, presumably for precision) | [C] `code/camera.cpp:Camera::SetupModelviewMatrix` |
| 2D overlay | `gluOrtho2D(viewport)` with identity modelview | [C] `code/renderer.cpp:Renderer::SetupMatricesFor2D` |

**Depth-bias by near-plane nudging** [C] (pattern used by other renderers via `Camera::SetupProjectionMatrix(near·k, far)` then restoring `near`): lasers ×1.2 (`code/location.cpp`), team "others" (Darwinians, virii) ×1.05 (`code/team.cpp`), entities ×1.05 (`code/worldobject/entity.cpp`, `virii.cpp`), spirit receivers ×1.1, mines ×1.1, rockets ×1.2, portals ×1.5, Sepulveda ×1.5 (`code/sepulveda.cpp`, [B:DEAD]). [I] Increasing the near plane shrinks window depth for the same eye distance (depth ≈ 1 − near/d for far ≫ near), so these passes win depth tests against coplanar terrain — a substitute for `glPolygonOffset`.

**Frustum tests** (CPU-side culling helpers) [C] `code/camera.cpp`:
- `SetupProjectionMatrix` also computes `m_cosFov = cos(fovY)` and the **diagonal half-angle** `m_maxFovRadians`: with screen half sizes `W/2`, `H/2` in pixels, `dtc = (H/2)/sin(fovY/2)` (distance to top-centre), `dc = √(dtc² + (W/2)²)` (distance to a corner), `ddHalf = √(W²+H²)/2`, `m_maxFovRadians = 2·asin(ddHalf/dc)`.
- `PosInViewFrustum(p)`: `dot(norm(p − pos), front) ≥ cos(fovY) − 0.2` (a cone test on the cosine with tolerance 0.2; note it uses the full vertical FOV angle, not the half-angle, so it is generous).
- `SphereInViewFrustum(c, r)`: true if camera inside sphere; else `acos(dot(norm(c − pos), front)) − asin(r/|c − pos|) ≤ m_maxFovRadians/2` (cone around the screen diagonal).
- No plane-based frustum and no far-plane rejection in these helpers.

**Picking**: `GetClickRay` / `Get2DScreenPos` (§3.10) read back the live GL matrices; callers must have the 3D matrices set (the camera's advancers push/pop around their own use).

**Water reflection**: the reflection pass mirrors the camera with `WaterReflect()` (y → −y on pos/front/up) and calls it again to restore [C] `code/water_reflection.cpp` (P11/P12).

## [→ 06_player_control_and_input] 4. Control help (`ControlHelpSystem`)

### 4.1 Reachability summary [C]

`g_app->m_controlHelpSystem` is created by `App`, advanced every location frame and rendered every frame [C] `code/main.cpp:LocationGameLoop`, `code/renderer.cpp:Renderer::RenderFrame`, but in the Multiwinia FULL build it draws nothing:
- `Render()` returns immediately if `g_app->Multiplayer()` (game mode Multiwinia — set for every game started through the client's `NET_DARWINIA_START_GAME` handling, tutorial included [C] `code/network/clienttoserver.cpp`) [C] `code/control_help.cpp:ControlHelpSystem::Render`, `code/app.cpp:App::Multiplayer`;
- otherwise it also requires a location, `INPUT_MODE_GAMEPAD`, preference `ControlHelpEnabled` (`OTHER_CONTROLHELPENABLED`, default 1) ≠ 0, no running script, no `dialog_locationmenu` window; `RenderHelp` additionally requires `IsSinglePlayer()` (Prologue/Campaign), modes that only the `INCLUDEGAMEMODE_PROLOGUE/CAMPAIGN` buttons can set (not compiled) [C] `code/UI/PrologueButton.h`, `code/UI/CampaignButton.h`; build_matrix §5.
- `AddOnScreenAid` begins with `return;` and its only callers are in `code/tutorial_demo2.cpp` (`USE_SEPULVEDA_HELP_TUTORIAL`) [C].
- `CheckCondition` returns `false` (body commented out) and the Advance loop that would use it is commented out [C].
Therefore the whole system is **[B:LEGACY]** (Darwinia-campaign gamepad help). Only `RecordCondUsed` bookkeeping runs (from camera, target cursor and team input). The live Multiwinia contextual hints are in `MultiwiniaHelp` (P19), e.g. the Shift-speed-up hint (§3.5) and the French camera image.

`Advance()` early-out: returns when input mode ≠ GAMEPAD **and** `ControlHelpEnabled` ≠ 0 (so on keyboard with help *disabled* it does run — inverted logic, harmless since aids are never added) [C]. Its aid loop only examines the first aid (unconditional `break`) and removes aids from index `Size()` downwards (first index out of range) [C].

### 4.2 Icons and sets (old corner layout, rendered only by commented-out code) [C] `ControlHelpSystem::InitialiseIcons`

| Icon id | Texture / shadow | Set-relative pos | Text anchors (x,y direction) |
|---|---|---|---|
| A | `icons/button_a.bmp` / `icons/button_abxy_shadow.bmp` | (50, 100) | (0, 1) |
| B | `icons/button_b.bmp` / abxy shadow | (100, 50) | (1, 0) |
| X | `icons/button_x.bmp` / abxy shadow | (0, 50) | (−1, 0) |
| Y | `icons/button_y.bmp` / abxy shadow | (50, 0) | (0, −1) |
| DPAD | `icons/button_dpad.bmp` / `icons/button_control_shadow.bmp` | (50, 0) | (1,0), (0,−1), (0,1) |
| LB | `icons/button_lb.bmp` / `icons/button_lb_shadow.bmp` | (0, 0) | (−1, 0) |
| RB | `icons/button_rb.bmp` / `icons/button_rb_shadow.bmp` | (100, 0) | (1, 0) |
| LA | `icons/button_la.bmp` / control shadow | (25, 0) | (1, 0) |
| LT | `icons/button_lt.bmp` / `icons/button_trigger_shadow.bmp` | (−25, 0) | (0, 1) |
| RA | `icons/button_ra.bmp` / control shadow | (75, 50) | (−1, 0) |
| RT | `icons/button_rt.bmp` / trigger shadow | (125, 50) | (0, 1) |

Sets (stacked vertically, set height in px): 0 = {A,B,X,Y} 175; 1 = {LB,RB} 75; 2 = {LA,RA,LT,RT} 150; 3 = {DPAD} 75. `HelpIcon::Render`: 50 px icon, 10 px gap, 12 px outlined text; icon alpha = max text alpha (min 0.3); blinks `0.75a + 0.25a·sin(8t)` for 1.5 s after its text changes; shadow drawn with `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR`, icon additive. `HelpIconSet::Render`: 1.0 s fade in/out when any icon becomes (un)used.

### 4.3 Condition → hint table [C] `ControlHelpSystem::InitialiseConditions`, intended triggers from the commented-out `ControlHelpSystem::CheckCondition`

`TextIndicator(icon, textSlot, langId, minTime, maxTime, maxUsage, decayRate)`; usage bookkeeping: `RecordCondUsed` adds `2·dt` to `timeUsed` (capped at `maxUsage` if non-zero; `timeUsed` is never initialised); `DecayCond` subtracts `decayRate·dt`; `SetCondIcon` alpha = 1 below `minTime`, else `(maxTime − used)/(maxTime − minTime)`, hidden when < 0 (hints fade out once the player has used the control enough).

| # | Condition | Icon | Text id (English) | min/max/maxUsage/decay | Intended trigger (commented code) | `RecordCondUsed` callers (live) |
|---|---|---|---|---|---|---|
| 0 | `CondDestroyUnit` | Y | `gc_destroy` (Destroy) | 0/0/0/1 | a program running or unit selected | — |
| 1 | `CondTaskManagerCreateBlue` | X | `gc_create` (Create) | 0/0/0/1 | task manager hidden, or TM advises create | — |
| 2 | `CondTaskManagerCloseBlue` | X | `gc_close` (Close) | 0/0/0/1 | never (too cluttered) | — |
| 3 | `CondTaskManagerCloseRed` | B | `gc_close` | 0/0/0/1 | task manager visible | — |
| 4 | `CondDeselectUnit` | B | `gc_deselect` (Deselect) | 0/0/0/1 | unit or building selected | — |
| 5 | `CondMoveUnit` | A | `gc_move` (Move) | 0/0/0/1 | TM hidden and cursor advises movable entity | — |
| 6 | `CondSelectUnit` | A | `gc_select` (Select) | 0/0/0/1 | TM hidden and cursor highlights something | — |
| 7 | `CondTaskManagerCreateGreen` | A | `gc_create` | 0/0/0/1 | TM visible and advises create | — |
| 8 | `CondTaskManagerSelect` | A | `gc_select` | 0/0/0/1 | TM visible over selectable zone | — |
| 9 | `CondPlaceUnit` | A | `gc_place` (Place) | 0/0/0/1 | TM hidden and placement opportunity | — |
| 10 | `CondPromoteOfficer` | A | `gc_promote` (Promote) | 0/0/0/1 | highlighting + officer program being placed | — |
| 11 | `CondMoveCameraOrUnit` | LA | `gc_move` | 2/3/9/0.3 | free-movement or entity-track camera | camera stick move [C] `code/camera.cpp`; unit stick move [C] `code/team.cpp` |
| 12 | `CondZoom` | LT | `gc_zoom` (Zoom) | 0/0/0/1 | radar-aim camera | — |
| 13 | `CondCameraAim` | RA | `gc_aim` (Look) | 2/3/9/0.3 | free-movement camera | `TargetCursor` moved in free movement [C] `code/lib/targetcursor.cpp` |
| 14 | `CondSquaddieFire` | RA | `gc_fire` (Fire) | 2/3/9/1 | squad selected (entity-track) | (commented) |
| 15 | `CondChangeWeapon` | DPAD | `gc_changeweapon` (Change Weapon) | 1/2/8/0.3 | squad selected with >1 secondary weapon researched | — |
| 16 | `CondChangeOrders` | DPAD | `gc_changeorders` (Change Orders) | 0/0/0/1 | officer or armour selected | — |
| 17 | `CondCameraUp` | RT | `gc_cameraup` (Up) | 1/2/3/0.3 | no LA/RA hint, TM hidden, no squad | camera up/down used [C] `code/camera.cpp` |
| 18 | `CondCameraDown` | LT | `gc_cameradown` (Down) | 1/2/3/0.3 | same | same |
| 19 | `CondFireGrenades` | RT | `gc_grenade` (Grenade) | 1/2/8/0.3 | `UnitPrimaryFireDirected` (non-existent control) + grenade selected | [C] `code/team.cpp` |
| 20 | `CondFireRocket` | RT | `gc_rocket` (Rocket) | 1/2/8/0.3 | same, rocket | same |
| 21 | `CondFireAirstrike` | RT | `gc_airstrike` (Airstrike) | 1/2/8/0.3 | same, airstrike | same |
| 22 | `CondOfficerSetGoto` | RT | `gc_setgoto` (Set Goto) | 0/0/0/1 | officer selected | — |
| 23 | `CondArmourSetTurret` | RT | `gc_setturret` (Set Turret) | 0/0/0/1 | armour selected | — |
| 24 | `CondSwitchPrevUnit` | LB | `gc_prev` (Prev) | 0/0/0/1 | >1 task | — |
| 25 | `CondSwitchNextUnit` | RB | `gc_next` (Next) | 0/0/0/1 | >1 task | — |
| 26 | `CondRadarAim` | A | `control_help_radar` (Aim Radar) | 0/0/0/1 | radar dish selected | — |
| 27 | `CondRotateCameraLeft` | LT | `control_help_camerarotateleft` (Look Left) | 0/0/0/1 | squad selected and not firing | — |
| 28 | `CondRotateCameraRight` | RT | `control_help_camerarotateright` (Look Right) | 0/0/0/1 | squad selected and not firing | — |

### 4.4 Selection-dependent help panel (`RenderHelp`, the replacement layout) [C] `ControlHelpSystem::RenderHelp`, `RenderControlHelp`, `RenderControlHelpDPad`, `RenderControlHelpDPadIcon`

Layout: d-pad icon (`icons/button_dpad.bmp` + `icons/button_control_shadow.bmp`, 50 px) at `(W/2 − W/6, 0.75H + 12.5)` with d-pad captions around it (positions 1 = up, 2 = left, 3 = right, 4 = down; offsets ±1.5 box units); four button boxes mirrored to `(W/2 + W/6)` in a 2×2 grid (1 top-left, 2 top-right, 3 bottom-left, 4 bottom-right). Box `w = 0.15H`, `h = 0.3w`, base y `0.75H`, background black α 0.75·a, outline white α 0.2·a; optional highlight in team colour ×0.7 (timer decays at 0.5/s); icon = `icons/<name>.bmp` scaled from 64-px units to `0.25w`; caption = upper-cased language phrase, outlined, font `0.4h`. The keyboard icon variant (`mouse_lmb`) is chosen when the input mode is KEYBOARD (never true here because rendering requires GAMEPAD).

| Selected (team's entity type, else unit troop type) | D-pad captions (position: text id [flag icon]) | Buttons (position: text id, pad icon) |
|---|---|---|
| `Entity::TypeOfficer` | 1: `multiwinia_control_order` [`banner_goto`]; 2: `multiwinia_control_follow` [`banner_follow`]; 4: `multiwinia_control_noorder` [`banner_none`] | 1: `multiwinia_control_follow` if the cursor is within 50 u (√2500) of the officer else `multiwinia_control_order`, `button_x`; 2: `gc_destroy`, `button_y`; 3: `gc_move`, `button_a`; 4: `gc_deselect`, `button_b` |
| `Entity::TypeInsertionSquadie` | 1: `gc_zoomin`; 4: `gc_zoomout` | 1: `gc_changeweapon`, `button_x`; 2: `gc_destroy`, `button_y`; 4: `gc_deselect`, `button_b` |
| `Entity::TypeArmour` | 1: `gc_setturret` | 1: `multiwinia_control_unload` if passengers > 0 else `multiwinia_control_load`, `button_x`; 2: `gc_destroy`, `button_y`; 3: `gc_setturret` if awaiting deployment else `gc_move`, `button_a`; 4: `gc_cancel` if awaiting deployment else `gc_deselect`, `button_b` |
| nothing selected | 1: `researchname_squad`; 2: `researchname_engineer`; 3: `researchname_armour` (only if armour research level > 0) | 1: `multiwinia_control_promote`, `button_x`; 3: `multiwinia_control_select`, `button_a` |

`GetOrderNameAndBanner(order)` (unused helper): `OrderNone` → `multiwinia_control_noorder`/`banner_none`; `OrderFollow` → `multiwinia_control_follow`/`banner_follow`; `OrderGoto` and `OrderPrepareGoto` → `multiwinia_control_order`/`banner_goto`.

On-screen aids (disabled, §4.1): centred text at `(W/2, H/3)` with a `W/40`-sized icon pair to its left, alpha 0.8 fading 1/s after the aid's control fires (or its `CheckRunner` predicate, e.g. `BoolChecker<ControlHelpSystem>(&HasSquadRunning)`), removed 1 s after completion. Text ids passed by the dead tutorial: `control_help_cameramove` (CameraMove), `control_help_cameraturn` (TargetMoveSecondary), `control_help_cameraup` (CameraUp), `control_help_cameradown` (CameraDown), `control_help_quickunitstart` (IconsTaskManagerDisplayPressed), `control_help_quickunitcreate` (UnitCreate / HasSquadRunning), `control_help_quickunitplace` (UnitCreate), `control_help_move` (UnitMove), `control_help_camerarotateleft` (CameraRotateLeft), `control_help_camerarotateright` (CameraRotateRight), `darwinia_tutorial_engineer` (MenuDown), `darwinia_tutorial_grenade` (UnitPrimaryFireTarget), `darwinia_tutorial_taskmanager` (IconsTaskManagerDisplayPressed), `darwinia_tutorial_createunit` (UnitCreate) [C] `code/tutorial_demo2.cpp` [B:DEAD].

