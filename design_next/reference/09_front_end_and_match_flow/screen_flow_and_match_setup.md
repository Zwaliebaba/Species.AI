# Screen flow and match setup

> Status: Reference · Prefix: R-META · Related: [Folder index](README.md) · [Options, localisation and hooks](options_localisation_and_hooks.md) · [01 Skirmish rules and modes](../01_skirmish_rules_and_modes/README.md) · [06 Controls, camera and commands](../06_controls_camera_and_commands/README.md) · [07 Bots](../07_bots/README.md) · [08 Maps and level design](../08_maps_and_level_design/README.md) · [10 Audio design](../10_audio_design.md) · [11 Tutorial and help](../11_tutorial_and_help.md) · [12 Simulation model](../12_simulation_model/README.md) · [UI framework](../../architecture/components/ui_framework/README.md)

## 1 Purpose and player experience

This document specifies the front end of the reference game: the one full-screen menu screen and its pages, the windows layered over it, the way from the title page through mode, map and match setup into a match and back, the match setup options and which participant sees which control, the loading view, the explainer panel shown while the players ready up, and the start of a match as the front end sees it. The settings pages, localisation and the achievement and leaderboard hooks are in [options_localisation_and_hooks.md](options_localisation_and_hooks.md). Pages are described by what they show and do. How a page is built, how focus moves, how widgets look and how the screen is drawn belong to the UI framework ([README](../../architecture/components/ui_framework/README.md), [core and widgets](../../architecture/components/ui_framework/core_and_widgets.md), [layout, rendering, input and localisation](../../architecture/components/ui_framework/layout_rendering_input_and_localisation.md), [pages and screen recipes](../../architecture/components/ui_framework/pages_and_screen_recipes.md)). The lobby protocol, session discovery, admission and identity are decided by the design phase (DEC-243, DEC-244, DEC-245, DEC-248, DEC-249): this document states what the screens show and what the match setup needs, never how it travels between participants.

**Player experience.** The game opens on a loading view that shows a studio mark, then on a title page over an animated backdrop: a giant breathing Worker figure in a red glow, a slowly turning wheel behind it and the title wordmark above. From the title page the player goes to single-player play against bots, to multiplayer (host or join), to the two tutorials, to help and options or to the credits, or quits. Single player and hosting share one path: choose a mode, choose a map, then the match setup page, where the host sets the bot difficulty and the advanced options and every player picks a name and a colour, or a side in Siege. The match starts at once in single player, and in multiplayer when every human is ready. A loading view leads into a frozen match in which the mode's explainer panel loops at the bottom of the screen until every human has pressed Ready; a three-second countdown follows. When the match ends, the game-over overlay stays until the player leaves through the in-match menu, which rebuilds the front end at its title page.

### 1.1 What the front end contains

The player-visible front end consists of:

1. one full-screen front-end screen (`FrontEndScreen`), a page state machine with the pages of §2.1;
2. help and options, opened over the front end or over the in-match menu, which leads to the help pages ([reference/11](../11_tutorial_and_help.md)) and to the settings with their screen, graphics, sound and other options ([options_localisation_and_hooks.md](options_localisation_and_hooks.md));
3. the in-match menu, the leave confirmation, the in-match chat input and the gamepad-disconnected dialog (§2.18);
4. the in-match HUD: the ready prompt, the countdown, the scoreboard, the match clock and the game-over overlay ([reference/01](../01_skirmish_rules_and_modes/README.md) §2.9 and §2.16), whose results hooks are §2.31;
5. the loading view (§2.28) and the explainer panels (§2.29);
6. two tutorials, launched from the tutorials page (§2.8; content in [reference/11](../11_tutorial_and_help.md)).

These exist only as hooks or not at all: the achievements page, the leaderboards and the Skill rating (DEC-047, [options_localisation_and_hooks.md](options_localisation_and_hooks.md)); quick match (DEC-249, DEC-050); user profiles and mods (DEC-061); key rebinding (DEC-050); the first-time intro sequence, the start-up mini-programs and the attract mode (DEC-054).

### 1.2 Conventions used in this document

- **Screen geometry.** W and H are the screen width and height in pixels. bx, bw and bh are the x position, width and height of a standard button; the left box is lX, lY, lW, lH and the right box rX, rY, rW, rH; small, medium and large are the three font sizes (§2.3). Positions are (x, y) with y downwards.
- **Time.** Everything the front end animates runs on the presentation clock in real seconds (ADR-0036); t is the presentation time in seconds. Simulation durations, such as the grace countdown, are integer ticks owned by [reference/01](../01_skirmish_rules_and_modes/README.md) and [reference/12](../12_simulation_model/01_tick_timing_and_main_loop.md).
- **Page tables.** "Nav" is the widget's position in the page's navigation order, and "—" marks a widget that focus never reaches (a title, a picture or an information panel). "Caption" is a string id.
- **Text.** Every player-facing text is a string id; the new text of each id that this document defines is in §4.2.
- **Commands.** The front end changes the match setup only through the player commands of [reference/12](../12_simulation_model/06_teams_and_player_commands.md) R-SIM-450 to R-SIM-452. A "match-setup change" is one of the commands of R-SIM-451.

## 2 Rules

### 2.1 Scope and the page set

**R-META-001** (MUST) The values of §4 are the reference tuning baseline, and this document is the specification of record for the front end and the match setup (DEC-004, DEC-005). A reference behaviour that looks accidental is kept as an explicit, named rule unless a decision corrects it (DEC-006).

**R-META-002** (MUST) The front end never writes simulation state. It changes the match setup only by issuing player commands, and it reads match state only from the presentation events and the read-only state that the simulation publishes (DEC-007).

**R-META-003** (MUST) The front end is one full-screen screen whose content is one page at a time. The reference has `ref.meta.front_end.live_pages` (18) live pages; this document specifies the `ref.meta.front_end.specified_pages` (15) pages below. The other three, two credential and trial-edition pages and an update notice, are outside the library (DEC-333, DEC-248, DEC-249).

| Page | Reached from | Specified in |
|---|---|---|
| title page | start-up; every return from a match | §2.7 |
| tutorials page | the title page; every return from a tutorial | §2.8 |
| host-or-join page | the title page | §2.9 |
| mode-select page | the title page (single player); the host-or-join page (host) | §2.10 |
| map-select page | the mode-select page | §2.11 |
| match setup page | the map-select page; the connecting page | §2.12 |
| advanced options page | the match setup page (host) | §2.13 |
| player options page | the match setup page (the player's own slot row) | §2.14 |
| session browser page | the host-or-join page (join) | §2.15, DEC-245 |
| browser filter page | the session browser page | §2.15, DEC-245 |
| password page | the session browser page (a protected session) | §2.15, DEC-244 |
| connecting page | the session browser page; the password page | §2.15 |
| wait-for-start page | the match setup page (host) | §2.15 |
| error page | the session browser page; the connecting page; a removal from a session | §2.15 |
| credits page | the title page | §2.16 |

The order of this table carries no meaning.

**R-META-004** (MUST) These windows open over the page underneath, each as a separate window on top of the window stack (R-META-026): help and options, help selection, general help, mode help, the settings screen and its four option screens (§2.17); the in-match menu, the leave confirmation, the in-match chat input and the gamepad-disconnected dialog (§2.18). The gamepad-disconnected dialog can appear over any screen.

### 2.2 Front-end lifecycle

**R-META-005** (MUST) The front-end controller (`FrontEndController`) lives for the process ([reference/12](../12_simulation_model/02_match_lifecycle_and_state_reset.md) §2.1). It is not a window. It owns the map catalogue ([reference/08](../08_maps_and_level_design/rules_and_conventions.md) R-MAP-011), the 3-D backdrop (R-META-024) and the creation and destruction of the one front-end screen (`FrontEndScreen`). Every page, every page transition and the logic of the match setup page belong to the front-end screen. How the application composes its services and states is DEC-221; the screen-level flow of diagram 3-A is reference behaviour.

**R-META-006** (MUST) While the application is at the front end, the front-end loop creates the menu whenever the menu does not exist and the screen fade has completed.

**R-META-007** (MUST) Creating the menu runs these steps in order:

1. start a screen fade-in of `ref.meta.front_end.fade_in` (0.25 s);
2. remove every UI window;
3. reset the match rules ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-033);
4. create and register a new front-end screen;
5. cut the camera to the front-end view, at `ref.sim.lifetime.front_end_camera_position` looking along `ref.sim.lifetime.front_end_camera_direction` ([reference/12](../12_simulation_model/02_match_lifecycle_and_state_reset.md)), in the camera's front-end focus mode with no debug mode; [reference/06](../06_controls_camera_and_commands/camera.md) does not specify that mode (DEC-024);
6. set the application's game mode to none;
7. if the player is returning from a tutorial, set the new screen's requested page to the tutorials page;
8. clear the returning-from-tutorial flag;
9. mark the menu as created.

**R-META-008** (MUST) A new front-end screen covers the whole screen, (0, 0) to (W, H), and cannot be resized, moved or closed. Its current page is none and its requested page is the title page, unless step 7 of R-META-007 applies. It reads the player name from the player-name setting ([options_localisation_and_hooks.md](options_localisation_and_hooks.md)), whose default is `ui.player.default_name`, replaces every `%` character of it with a space and stores the result through the name rule (R-META-075). No credential or entitlement check runs at any point of the front end (DEC-248).

**R-META-009** (MUST) On its first update the front-end screen sets up its requested page. On the first update after the audio engine is ready, it starts the lobby ambience (R-META-030).

**R-META-010** (MUST) When the match-start command executes (`StartMatch`, [reference/12](../12_simulation_model/06_teams_and_player_commands.md) R-SIM-460), the participant sets its match-loading flag, requests the match, sets the game mode to match, leaves the front end, shows the interface, clears its at-match-setup flag and resets the game timer. The front-end loop then exits, stops the lobby ambience and destroys the menu: the front-end screen is removed and the screen fades out.

**R-META-011** (MUST) The in-match leave action (R-META-113) clears the requested match and returns the application to the front end. The front-end loop then builds a fresh front-end screen at the title page, or at the tutorials page after a tutorial. No match setup state survives a match; only the settings store persists ([reference/12](../12_simulation_model/02_match_lifecycle_and_state_reset.md) R-SIM-031).

**R-META-012** (MUST) The front end has no results page. A match result is shown only by the game-over overlay of the match HUD ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-035, R-MODE-068); its hooks are §2.31 (DEC-048).

**R-META-013** (MUST) After game over the only way out of a match is the in-match menu's leave action, which in a finished match leaves without a confirmation (R-META-111). The ready action has no effect at game over. The game-over prompt `match.game_over.leave_prompt_keyboard` or `match.game_over.leave_prompt_pad` names this path ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-068).

### 2.3 Shared page layout

**R-META-014** (MUST) Every front-end page and every options screen (`OptionsScreen`) uses one layout, computed from W and H:

| Quantity | Formula |
|---|---|
| horizontal split | `ref.meta.layout.horizontal_split` (0.26) × H |
| vertical split | `ref.meta.layout.vertical_split` (0.6) × W |
| gap | `ref.meta.layout.gap` (0.05) × vertical split, that is 0.03 × W |
| outer edges | left = top = gap; right = W − gap; bottom = H − gap. On a wide screen, W ÷ H > `ref.meta.layout.widescreen_aspect` (1.4): left += `ref.meta.layout.widescreen_inset` (0.08) × W and right −= `ref.meta.layout.widescreen_inset` × W |
| title bar | x = left; y = top; width = right − left; height = horizontal split − gap ÷ 2 − top |
| left box | x = left; y = horizontal split + gap ÷ 2; width = vertical split − gap ÷ 2 − left; height = bottom − y |
| right box | x = vertical split + gap ÷ 2; y = horizontal split + gap ÷ 2; width = right − x; height = bottom − y |
| fonts | large = `ref.meta.layout.font_large` (0.095) × H; medium = `ref.meta.layout.font_medium` (0.04) × H; small = `ref.meta.layout.font_small` (0.022) × H |
| standard button | width bw = `ref.meta.layout.button_width` (0.9) × lW; height bh = `ref.meta.layout.button_height` (1.3) × medium, that is 0.052 × H; button gap = `ref.meta.layout.button_gap` (0.1) × medium, that is 0.004 × H |

**R-META-015** (MUST) The standard page skeleton places its buttons centred in the left box at bx = lX + (lW − bw) ÷ 2. The page title sits at (lX + `ref.meta.layout.title_inset` (10 px), lY + 10 px, lW − 2 × 10 px, `ref.meta.layout.title_height` (1.5) × bh) in the medium font; it is drawn upper-cased, shrunk to fit, over the highlight beam (`ui.menu_highlight`). The first stacked button sits at y = lY + `ref.meta.layout.first_button_offset` (0.5 × bh + 1.3 × bh + one button step), where one button step is bh + button gap, and each further button one button step lower. The Back button sits at y = lY + lH − `ref.meta.layout.back_offset` (2) × bh. The right box carries an information panel (a picture, a description or the chat), which is outside the navigation order.

**R-META-016** (MUST) Page building, page transitions, the Back semantics and controller navigation follow the UI framework ([pages and screen recipes](../../architecture/components/ui_framework/pages_and_screen_recipes.md)). A Back button marked "previous page" returns to whichever page was showing before the current one.

**R-META-017** (MUST) The widgets of these pages (page buttons, page titles, text fields with their increase and decrease glyphs, drop-down lists in cycle mode or as a full-screen list, check boxes, Back and close buttons, text labels, scrolling text and pictures) render, take their palette and handle input as the UI framework specifies ([core and widgets](../../architecture/components/ui_framework/core_and_widgets.md)).

### 2.4 Persistent background

**R-META-018** (MUST) Every frame the front-end screen draws, in this order: the backdrop figure layer (R-META-019); the title bar (R-META-021); the content boxes of the current page (R-META-022); the page overlays (R-META-023); the widgets. While a drop-down list is open, only the widget that owns the list is drawn of the page's widgets.

**R-META-019** (MUST) The backdrop figure layer, which the options screens and the drop-down menus also draw, consists of:

1. a full-screen additive quad, black at three corners and `ref.meta.backdrop.glow_colour` (RGB 1.0, 0.5, 0.1) at the bottom-left corner, smoothly shaded;
2. the giant Worker figure, `sprite.worker.front`, with nearest filtering and tint `ref.meta.backdrop.figure_tint` (RGBA 1, 0, 0, 1), centred at `ref.meta.backdrop.figure_centre` (0.75 × W, 0.7 × H), of half-size `ref.meta.backdrop.figure_half_size` (0.3 × W) + `ref.meta.backdrop.figure_breath` (5 px) × sin(t), so that it breathes, and rotated by `ref.meta.backdrop.figure_angle` (0.2 rad);
3. in high detail only, `ref.meta.backdrop.echo_count` (10) echo copies of the figure, copy i from 0 to 9: each copy's size is the previous size × `ref.meta.backdrop.echo_scale` (1.02) and its centre the previous centre shifted by `ref.meta.backdrop.echo_offset` (−6, −6) px, tinted `ref.meta.backdrop.echo_tint` (RGB 1, 0.2, 0.2) at alpha `ref.meta.backdrop.echo_alpha` (0.2) × (1 − i ÷ 10);
4. the wheel, `ui.backdrop_wheel`, of half-size `ref.meta.backdrop.wheel_half_size` (0.6 × W), centred on the screen, at angle `ref.meta.backdrop.wheel_spin` (0.2 rad per second) × t, additive white at alpha `ref.meta.backdrop.wheel_alpha` (0.06), drawn `ref.meta.backdrop.wheel_copies` (10) times in high detail, each copy rotated a further `ref.meta.backdrop.wheel_copy_step` (0.02 rad), and not at all in low detail.

**R-META-020** (MUST) The detail level of R-META-019 changes only while the participant is at the front end. Each frame the smoothed frame rate becomes smoothed × (1 − k) + rate × k, with k = frame advance time × `ref.meta.backdrop.fps_smoothing` (0.5). The layer switches to low detail when the smoothed rate falls below `ref.meta.backdrop.low_detail_fps` (20) and back to high detail when it reaches `ref.meta.backdrop.high_detail_fps` (60). The smoothed rate starts at `ref.meta.backdrop.initial_fps` (60).

**R-META-021** (MUST) The title bar is a fill box. When the title string `ui.title.game_name` of the current language is the unchanged game name, the title wordmark `ui.title_wordmark` is drawn: its width is the width of that many characters of the title font at the large size × `ref.meta.title.wordmark_width_factor` (1.1), its height is that width ÷ `ref.meta.title.wordmark_aspect` (5.4), and its top is at the title bar's y + `ref.meta.title.wordmark_top` (0.1) × the title bar's height. Otherwise the title text is drawn with an outline. The tagline `ui.title.tagline` is drawn at the medium size, one large-font line below. The logo emblem `ui.logo_emblem`, of aspect `ref.meta.title.emblem_aspect` (277 ÷ 194) and height `ref.meta.title.emblem_height` (0.8) × the title bar's height, is drawn at x = the title bar's x + `ref.meta.title.emblem_inset` (0.05) × its width, and mirrored at the right end.

**R-META-022** (MUST) The content boxes are shadowed fill boxes (`ui.fill_blur`, `fx.glow_cloudy`; [pages and screen recipes](../../architecture/components/ui_framework/pages_and_screen_recipes.md)), by page: the match setup page has the left and the right box; the session browser page has one box of the title bar's width; the connecting page has the left box; the mode-select, map-select and advanced options pages have the left and the right box; every other page, the title page included, has the left box only.

**R-META-023** (MUST) The page overlays are: on the session browser page with an empty list, the blinking caption `ui.browser.requesting` while the list is being requested and `ui.browser.none_found` when no session was found (DEC-245). A blinking caption has alpha `ref.meta.blink.on_alpha` (1.0) for the first `ref.meta.blink.on_time` (0.5 s) of each second and `ref.meta.blink.off_alpha` (0.5) for the rest.

**R-META-024** (MUST) The 3-D backdrop behind the screen is drawn by the front-end controller whenever the participant is at the front end and the menu exists, and also by the loading view (R-META-171). It advances the camera, places it at the origin with a field of view of `ref.meta.backdrop.camera_fov` (90°), and builds the view from the eye `ref.meta.backdrop.camera_eye` (0, 600, 0) wu, the front (0, 0, 1) and the up vector (0, 1, 0), each rotated about the y axis by `ref.meta.backdrop.camera_yaw` (−1.899 rad) and then about the z axis by `ref.meta.backdrop.camera_roll` (−1.079 rad); the focus point is eye + front × `ref.meta.backdrop.camera_focus_distance` (100 wu). It then draws the sky and its cloud layers (`fx.sky_cloud`; the sky renderer `SkyRenderer`, [world renderers](../../architecture/components/renderer/world_renderers.md)) at the time −(real time), so the clouds drift backwards; the sky-detail setting applies ([options_localisation_and_hooks.md](options_localisation_and_hooks.md)).

### 2.5 Navigation, Back and the front-end sound cues

**R-META-025** (MUST) Every full-screen window registers a Back button as its back action, and the UI framework invokes it on the menu-close action of a gamepad (Back or the east face button) ([core and widgets](../../architecture/components/ui_framework/core_and_widgets.md)). In a match the menu action `action.match.menu` closes every open window at once and shows the interface again; it does not go back one level ([reference/06](../06_controls_camera_and_commands/actions_and_bindings.md) row M4).

**R-META-026** (MUST) Windows stack: each new window opens on top of the open ones, and closing a window reveals the one beneath it.

**R-META-027** (MUST) The **base press** of a page button plays the menu-select cue, or the menu-back cue when the button's transition direction is backwards, and starts a menu transition of `ref.meta.nav.transition_time` (1.0 s) in that direction.

**R-META-028** (MUST) A Back button plays the menu-select cue through the base press, then the menu-back cue, and restarts the transition in the backwards direction.

**R-META-029** (MUST) The front-end sound cues are events of the match UI sound source ([reference/10](../10_audio_design.md) owns the event ids):

| Cue | Asset | When |
|---|---|---|
| lobby bed | `amb.lobby_bed` | starts on the first front-end update after the audio engine is ready; stops when the front-end loop exits |
| menu select | `sfx.ui.match_ui.menu_select` | a forward button press through the base press; a map-row press |
| menu back | `sfx.ui.match_ui.menu_back` | a press of a button whose transition direction is backwards and that uses the base press; every Back button, after its menu-select cue |
| menu hover | `sfx.ui.power_slots.icon_hover` | focus moves onto a new button, map row or colour row |
| chat message | `sfx.ui.match_ui.chat_message` | a chat line arrives on the match setup page; join and leave notices are silent |

**R-META-030** (MUST) The lobby bed plays only at the front end. It does not play during a loading view, because its trigger runs only from the front-end loop (DEC-319).

### 2.6 Page specification conventions

**R-META-031** (MUST) Every widget of a page has a widget id that is unique within the page. The help text of a focused option control is keyed by its option (R-META-071).

**R-META-032** (MUST) Every caption is a string id. A string that the current language lacks falls back to the base language, so no page shows a string id as text (DEC-099; [options_localisation_and_hooks.md](options_localisation_and_hooks.md)).

### 2.7 Title page

**R-META-033** (MUST) Setting up the title page clears the single-player flag and the local map choice. The page holds:

| Nav | Widget | Caption | Shown when | Action |
|---|---|---|---|---|
| — | title | `ui.main.title` | always | — |
| 1 | single player | `ui.main.single_player` | always | base press; sets the single-player flag; → mode-select page |
| 2 | multiplayer | `ui.main.multiplayer` | always | base press; → host-or-join page |
| 3 | tutorials | `ui.main.tutorials` | always | base press; → tutorials page |
| 4 | help and options | `ui.main.help_and_options` | always | base press; opens the help-and-options window as a child of the screen and requests a menu transition |
| 5 | credits | `ui.main.credits` | always | base press; → credits page |
| 6 | quit | `ui.main.quit` | always, at y = lY + lH − 2 × bh | requests the application to quit, with no cue and no transition |

**R-META-034** (MUST) The per-frame update of the title page, which also runs on the tutorials page, starts the private local session when the single-player flag is set and no local session exists (R-META-040). This is how a tutorial launch obtains its session (diagram 3-F).

**R-META-035** (MUST) The title page offers no update check, no message of the day and no credential status; those depend on online services that the library does not specify (DEC-249, DEC-248).

### 2.8 Tutorials page

**R-META-036** (MUST) Setting up the tutorials page clears the current tutorial instruction ([reference/11](../11_tutorial_and_help.md)). The page holds:

| Nav | Widget | Caption | Condition | Action |
|---|---|---|---|---|
| — | title | `ui.tutorials.title` | | |
| 1 | tutorial 1 | `ui.tutorials.part_1` | inactive while the first tutorial's map ([reference/11](../11_tutorial_and_help.md)) is not in the Zone Control map list | ignored while inactive or while a quick start runs; otherwise the base press, then: set the single-player flag, the quick-start flag, the tutorial flag and the tutorial type, and lock the page navigation |
| 2 | tutorial 2 | `ui.tutorials.part_2` | inactive while the second tutorial's map is not in the Zone Control map list | as tutorial 1, for the second tutorial |
| 3 | Back | `ui.common.back` | | → title page |

**R-META-037** (MUST) While the navigation is locked, focus does not move and no page widget reacts. The tutorial launch then runs as diagram 3-F: the title-page update starts the private local session (R-META-034); once the session exists, the match setup applies the tutorial's set-up of [reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-083 (Zone Control, its tutorial map, every Zone Control option at its default, Easy bots, requested colour 0, and for the second tutorial the slot colours 0, 1 and 2 on slots 0, 1 and 2), marks the tutorial as starting and fades the screen out; when the local team slot arrives and the audio engine is ready, the front end sends the player's name and colour, requests (player count − 1) bot teams, issues `StartMatch` and clears the quick-start flag. After the tutorial match, the front end opens at the tutorials page (R-META-007).

### 2.9 Host-or-join page

**R-META-038** (MUST) Setting up the host-or-join page clears the single-player flag. The page holds:

| Nav | Widget | Caption | Action |
|---|---|---|---|
| — | title | `ui.host_or_join.title` | |
| 1 | host | `ui.host_or_join.host` | base press; → mode-select page, whose set-up starts the hosted session (R-META-040) |
| 2 | join | `ui.host_or_join.join` | base press; → session browser page (§2.15) |
| 3 | Back | `ui.common.back` | → title page |

**R-META-039** (MUST) No page leads to a quick match. Quick match, which joins the first compatible session with room, is DEC-249, and whether Species designs it is DEC-050.

### 2.10 Mode-select page

**R-META-040** (MUST) Setting up the mode-select page starts the match session when none exists. In single player it is a **private local session** that no other participant can join; when hosting it is a **hosted session** that others can join, which is not listed for joining until a map has been chosen (DEC-245 item 2). Both set the bot difficulty to Standard, unless a tutorial is running (R-MODE-030). The local participant joins its own session through the same path as any other participant (DEC-243 item 12); the session model is decided by the design phase (DEC-200, DEC-243).

**R-META-041** (MUST) The page holds one mode button per mode, in the order of the mode catalogue ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-029), six in all, each captioned `mode.<mode>.name`. A mode button is drawn only once the local participant holds a team slot or is a spectator, so the list appears once the new session has accepted the local player. Each button shows the mode's complexity badge (`ui.difficulty_badge`, labelled `mode.complexity.simple`, `mode.complexity.medium` or `mode.complexity.hard`) at its right edge, of size `ref.meta.mode_select.badge_size` (0.1 × w by 0.05 × w, w the button width). Moving focus onto a mode button makes it the highlighted mode.

**R-META-042** (MUST) Pressing a mode button does nothing while the button is inactive. Otherwise, once the local participant holds a team slot, it goes to the map-select page, sets the mode in the match setup, clears the mode that the current page was built for, remembers the pressed mode, and sets every option of the mode to its default, as one update that no participant sees half-applied ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-078, DEC-243).

**R-META-043** (MUST) The page holds:

| Nav | Widget | Caption | Notes |
|---|---|---|---|
| — | title | `ui.mode_select.title` | |
| 1–6 | mode buttons | `mode.<mode>.name` | R-META-041, R-META-042 |
| 7 | Back | `ui.common.back` | → title page in single player, else → host-or-join page; ends the local session (R-META-150) |
| — | mode card | — | in the right box at (rX + `ref.meta.mode_select.card_inset` (0.1) × rW, rY + 0.1 × rW), `ref.meta.mode_select.card_width` (0.8) × rW wide and `ref.meta.mode_select.card_aspect` (2 ÷ 3) of that width high: the `help.mode_card` of the highlighted mode at alpha `ref.meta.mode_select.card_alpha` (0.8), with an outline at alpha `ref.meta.mode_select.card_outline_alpha` (0.5) |
| — | mode description | — | below the card, `ref.meta.mode_select.info_height` (0.85) × rH − the card height high, in the small font: the highlighted mode's `mode.<mode>.objective`, scrolling automatically |

**R-META-044** (MUST) The page loads every mode card at set-up, so a focus change never waits for a load.

### 2.11 Map-select page

**R-META-045** (MUST) Setting up the map-select page loads the thumbnails of the current mode's maps. The reference loads them on the loading view's loader, with the loading view's spinner drawn over the menu while they load (R-META-171); in Species previews load in the background (DEC-226 item 9).

**R-META-046** (MUST) While a mode press is pending and the match setup's mode differs from the pressed mode, setting up the page builds an empty page, and the page is set up again every frame until the two match.

**R-META-047** (MUST) The page holds one row per map in the current mode's map list, in the order of the map catalogue ([reference/08](../08_maps_and_level_design/rules_and_conventions.md) R-MAP-016; the order of tied maps is DEC-314). Only maps that list the mode appear (R-MAP-017; maps that list no mode are DEC-802). A row is `ref.meta.map_select.row_height` (0.7) × bh high and bw wide, rows are 0.7 × one button step apart, and the row text uses the small font. A row's widget id is its map id.

**R-META-048** (MUST) The page holds:

| Nav | Widget | Caption | Notes |
|---|---|---|---|
| — | title | `mode.<mode>.name` | |
| — | column header | `ui.map_select.column.name`, `ui.map_select.column.play_time`, `ui.map_select.column.players` | a strip at (bx, lY + `ref.meta.map_select.header_offset` (10 px + 1.8 × bh), bw, 0.7 × bh), black at alpha `ref.meta.map_select.header_alpha` (0.5), its captions in grey `ref.meta.map_select.header_grey` (0.6) at the row columns (R-META-049) |
| 1 to n | map rows | R-META-049 | |
| n + 1 | Back | `ui.common.back` | → mode-select page |
| — | thumbnail | — | in the right box at (rX + 0.1 × rW, rY + 0.1 × rW), of the size of [reference/08](../08_maps_and_level_design/rules_and_conventions.md) R-MAP-022 (`ref.map.thumbnail.width_fraction` × rW wide, `ref.map.thumbnail.aspect` of that width high) |
| — | map description | — | below the thumbnail, `ref.meta.mode_select.info_height` × rH − the thumbnail height high, in the small font × `ref.meta.map_select.info_font_scale` (1.1) |

**R-META-049** (MUST) A map row shows three columns, at x + `ref.meta.map_select.column_name` (0.025) × w, x + `ref.meta.map_select.column_play_time` (0.65) × w and x + `ref.meta.map_select.column_players` (0.83) × w, w the row width:

1. **Name.** `map.<map id>.name`; a custom map shows the display name of its own header (R-MAP-020). The name is drawn in `ref.meta.map_select.name_colour` (RGB 0.65, 1, 0.4), or `ref.meta.map_select.name_focus_colour` (RGB 1, 0.3, 0.3) while focused; the row of the map that the match setup has chosen is drawn pressed.
2. **Play time.** The map's play time in minutes or, when the map sets none, the mode's default time limit, shown by `ui.map_select.play_time` as "0:" followed by the minutes in two digits, so that 10 reads 0:10 (R-MAP-021). When neither exists, because the mode has no time-limit option or its default is "no limit", the column shows `ui.map_select.no_play_time`.
3. **Players.** One Worker badge (`ui.worker_badge`) per player slot of the map, at most `ref.sim.team.player_slots` (4), each `ref.meta.map_select.badge_height` (0.9) × the row height high, `ref.meta.map_select.badge_spacing` (0.55) × the badge width apart and rotated by `ref.meta.map_select.badge_angle` (−π ÷ 8), coloured with team colour i for badge i ([reference/12](../12_simulation_model/06_teams_and_player_commands.md) R-SIM-415). On a map that forces co-op, badge i takes colour i ÷ 2, except that on a 3-player map badge 1 takes colour 1.

**R-META-050** (MUST) Each row slides in after the page is built: it appears `ref.meta.map_select.slide_delay` (1.0 s) × (the row's y ÷ H) after the page is built, starts −`ref.meta.map_select.slide_distance` (0.1 × W) × the remaining seconds to the left of its place, and carries an additive white flash whose alpha is the remaining seconds.

**R-META-051** (MUST) Moving focus onto a row plays the menu-hover cue and makes the row's map the highlighted map, which drives the thumbnail and the description.

**R-META-052** (MUST) Pressing a map row starts a forward transition, ignores an index outside the list, plays the menu-select cue, chooses the map in the match setup by its map id (`SetMatchOptionText`, R-SIM-451), remembers it as the local map choice and goes to the match setup page. The host applies the map's defaults once, authoritatively ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-079, DEC-067): co-op is cleared on a map that does not allow co-op and set on a map that forces it, and, when the mode has a time-limit option, the time limit becomes the map's play time, or the mode default when the map sets none.

**R-META-053** (MUST) The thumbnail shows the map of the focused row, or the chosen map when no row has focus, inside a white outline; a map without a thumbnail, which only a custom map can be, shows the outline alone (R-MAP-022). The description shows a custom map's header description, or `map.<map id>.description` for a listed map, scrolling automatically.

### 2.12 Match setup page

The match setup page is one page for single-player set-up, the host's lobby and a joining player's lobby (DEC-026); the differences are in §2.22.

**R-META-054** (MUST) Setting up the match setup page applies these steps in order:

1. When the match setup has no mode, nothing is built.
2. The map is the local map choice or, when there is none, the match setup's chosen map. When there is no map, or the local participant holds no team slot and is not a spectator, the page sets its waiting-for-team flag, remembers the mode and builds an empty page; it is set up again once a team slot, or the spectator flag, arrives.
3. When the page is entered from the map-select page while the mode that page was built for differs from the match setup's mode, the map-select page is built again instead. When it is entered from any page other than the map-select, advanced options and player options pages, the page records the match setup's mode as the mode it is built for and, on the host, chooses the first map of the list. Every ordinary entry comes from the map-select page, the player options page, the advanced options page or, for a joining player, the connecting page, so this branch is reached only when the mode changes while the page is shown.
4. The participant's at-match-setup flag is set.
5. On a return from the player options page with an empty name, the name becomes `ui.player.default_name`.
6. Unless the page is entered from the player options or advanced options page, the requested colour is initialised: in Siege or co-op the current request is remembered as the previous request and the request becomes none, because the side or alliance is chosen later; otherwise the request is the preferred-colour setting when that colour is free, else the next available colour, the lowest colour id from 0 to 7 that no used slot holds ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-015).
7. On the host, unless the page is entered from the advanced options or player options page, every option of the mode takes its default, keeping a time limit that the map's play time set ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-080, DEC-067).

**R-META-055** (MUST) The page holds the widgets below. Stacked controls use the small font; the title, the primary button and Back use the medium font.

| Nav | Widget | Caption | Who sees it | Geometry | Action |
|---|---|---|---|---|---|
| — | title | `mode.<mode>.name` | everyone | standard | — |
| 1, initial focus | primary button | `ui.lobby.ready` in multiplayer, `ui.lobby.play` in single player | everyone | (bx, lY + bh + `ref.meta.setup.primary_offset` (1.8) × bh, bw, bh) | §2.24 |
| 2 | advanced options | `ui.lobby.advanced_options` | the host | `ref.meta.setup.small_control_height` (0.66) × bh high | base press; → advanced options page |
| 3 | bot difficulty | `match.options.bot_difficulty`, values `match.options.bot_difficulty.easy`, `.standard`, `.hard` | the host | 0.66 × bh high; no background; cycle mode, in which left and right step through the values | the values 0, 1 and 2 are `BotDifficulty::Easy`, `BotDifficulty::Standard` and `BotDifficulty::Hard`, bound to the match setup's bot difficulty ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-082); only these values are accepted (DEC-077) |
| 4 onwards | player slot rows, one per player slot of the map | §2.23 | everyone | a column at (bx, lY + `ref.meta.setup.slot_column_y` (6) × bh), each row `ref.meta.setup.slot_width` (0.45) × bw wide and `ref.meta.setup.slot_height` (0.75) × bh high, rows `ref.meta.setup.slot_step` (1.125) × bh apart | the player's own row: base press, → player options page; any other row: no action |
| beside the rows | slot control: kick, or open and close | — | the host (§2.24) | at (bx + `ref.meta.setup.slot_control_x` (1.05) × 0.45 × bw, the row's y), `ref.meta.setup.slot_control_size` (0.75) × bh square | R-META-142, R-META-144 |
| last | Back | `ui.common.back` | everyone | (bx, lY + lH − 2 × bh) | §2.25 |
| — | mode card | — | everyone | (lX + lW − 0.45 × bw − (lW − bw) ÷ 2, lY + 6 × bh), 0.45 × bw wide and `ref.meta.setup.mode_card_height` (0.3) × bw high, beside the slot rows | the mode's `help.mode_card` |
| — | thumbnail | — | everyone | as on the map-select page (R-META-048) | the chosen map's thumbnail |
| — | map name | — | everyone | under the thumbnail, 0.66 × bh high, small font, over the highlight beam | the chosen map's name; a press does nothing, because the map cannot change on this page |
| — | map description or chat log | — | everyone | under the map name, rH − the thumbnail height − small ÷ 2 − 2 × one button step − button gap high | single player: the map description; multiplayer: the chat log in the small font × `ref.meta.chat.log_font_scale` (0.8) (§2.26) |
| — | chat input | — | multiplayer | (rX + 0.1 × rW, rY + `ref.meta.chat.input_y` (0.95) × rH, 0.8 × rW, `ref.meta.chat.input_height` (1.2) × small) | §2.26 |

**R-META-056** (MUST) No spectator control and no add-bot control exists on the page: bots fill the empty open slots when the match starts (R-META-140), and no player-facing path makes a participant a spectator (R-META-146).

### 2.13 Advanced options page

**R-META-057** (MUST) The advanced options page is reached only by the host. Its controls are `ref.meta.options.control_height` (0.65) × bh high and use the small font:

| Nav | Widget | Caption | Created when | Bound to |
|---|---|---|---|---|
| — | title | `ui.advanced_options.title` | always | |
| 1 to k | one control per option of the mode, in display order ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-073), of the control type and with the label of R-MODE-074 | the match setup has a mode | that option |
| k + 1 | basic crates only switch | `match.options.basic_crates` | always | the basic-crates-only flag |
| k + 2 | co-op switch | `match.options.coop` | the map allows co-op (R-MODE-018) | the co-op flag |
| k + 3 | password field, after one empty row | `match.options.password` | the host of a multiplayer session | the session password, at most `ref.match.option.password_max_length` (16) characters; setting it on the host sets the password that joining players must give (DEC-244 item 9) |
| last | Back | `ui.common.back` | always | → match setup page |
| — | option details | — | always | in the right box, inset `ref.meta.options.details_inset` (5 %) and `ref.meta.options.details_size` (90 %) of the box, small font: the help text of the focused control (R-META-061), scrolling automatically |

**R-META-058** (MUST) An option's control type selects its widget:

| Control | Widget | Values |
|---|---|---|
| switch | check box | off, on |
| list | drop-down list in cycle mode | one entry per value from the minimum to the maximum, each labelled by the value's string id (R-MODE-074) |
| spinner | integer field with a decrease and an increase glyph | clamped to the option's range; each glyph steps by the option's step; holding a glyph repeats the step every `ref.meta.options.repeat_period` (0.1 s), × `ref.meta.options.repeat_speedup` (5) while the UI framework's scroll speed-up action is held |

**R-META-059** (MUST) Every option value is an integer within the option's stated inclusive bounds, validated once by the match setup's authority (DEC-241 item 13, DEC-243). Under DEC-120 the lowest step of the time-limit spinner and of the crate-drop-period spinner is an explicit value, "no limit" or "none", which the spinner shows as `match.options.time_limit.none` or `match.options.crate_drop_period.none` and never as the number 0.

**R-META-060** (MUST) Every frame, the sudden-death control is inactive while the time limit is "no limit". When that state changes, the control's value is forced: off when it becomes inactive, on when it becomes active ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-056).

**R-META-061** (MUST) The help text of the focused control is `ui.advanced_options.help.<option>.<mode>` when that string exists, else `ui.advanced_options.help.<option>`.

### 2.14 Player options page

**R-META-062** (MUST) The player options page is reached from the player's own slot row. Its controls, its title included, are 0.65 × bh high and use the small font:

| Nav | Widget | Caption | When | Action |
|---|---|---|---|---|
| — | title | `ui.player_options.title` | | |
| 1 | name field | `ui.player_options.name` | always | the player's name, at most `ref.meta.player.name_max_length` (16) characters (R-META-063) |
| 2 to 4 | side rows: random, attackers, defenders | `ui.player_options.side.random`, `ui.player_options.side.attackers`, `ui.player_options.side.defenders` | Siege | request the colour none, the attacker colour `ref.sim.team.siege_attacker_colour_id` (1) or the defender colour `ref.sim.team.siege_defender_colour_id` (0) ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-026) |
| 2 to 9 | colour rows, colour ids 0 to 7 in id order | `team.colour.green`, `team.colour.red`, `team.colour.yellow`, `team.colour.blue`, `team.colour.orange`, `team.colour.purple`, `team.colour.cyan`, `team.colour.pink` | every other mode | request that colour (§2.23) |
| last | Back | `ui.common.back` | | applies the name (R-META-063), saves the settings file and returns to the match setup page |

**R-META-063** (MUST) The **name rule**: setting the player name truncates it to `ref.meta.player.name_max_length` (16) characters, stores the truncated name, or `ui.player.default_name` when it is empty, in the player-name setting, saves the settings file and issues `SetTeamName` for the player's slot (R-SIM-451). The stored name is always the truncated one.

### 2.15 Session pages

**R-META-064** (MUST) The session browser page, the browser filter page and the password page follow the reference default of DEC-245 (the merged session list, its filters, its sorting, its rows and the rule order for choosing a row) and DEC-244 (the password). This document specifies where they lead (diagram 3-A): choosing a row leads, by the first rule that applies, to the error page for an incompatible session, a full session or a map that is not installed, to the password page for a protected session, and otherwise to the connecting page; a protected session's row shows `ui.icon_locked`. The browser filter page saves its filter settings and returns to the browser. The password page leads to the connecting page when the typed password is not empty, and back to the browser otherwise.

**R-META-065** (MUST) The connecting page holds the title `ui.connecting.title`, a status widget in the medium font, and a Back button that returns to the session browser page when the player came from it and to the host-or-join page otherwise, for example from the password page.

**R-META-066** (MUST) The status widget shows, by the participant's connection state (the states are DEC-205 item 7):

| State | First line | Second line, at `ref.meta.connect.second_line_scale` (0.6) × the font |
|---|---|---|
| connecting | `ui.connecting.connecting` followed by 0 to `ref.meta.connect.max_dots` (3) dots, the count being ⌊t × 2⌋ mod 4, one more dot every `ref.meta.connect.dot_period` (0.5 s) | `ui.connecting.attempts` with the number of connection attempts |
| handshaking, connected | `ui.connecting.connected` | `ui.connecting.catching_up` with 100 × processed ÷ total command frames, without decimals: the share of the session history caught up so far (DEC-244 item 2) |
| disconnected | blank | blank |

When the player chose a session in the browser, a details block follows at `ref.meta.connect.details_scale` (0.5) × the font, its rows `ref.meta.connect.details_row_step` (0.7) × the font apart and its values `ref.meta.connect.details_value_x` (200 px) right of the labels: `ui.connecting.host` with the session name, `ui.connecting.address` with `ui.connecting.address_value` (the address and port actually contacted), `ui.connecting.mode` with the mode's name, `ui.connecting.map` with the map's name and `ui.connecting.players` with "n/max". All text is drawn twice, the outline first and then the fill.

**R-META-067** (MUST) The connecting page goes to the match setup page once the state is connected and the match setup's mode is known; the match setup page may then wait for a team slot (R-META-054 step 2). The page has no timeout: it waits until the player presses Back or the participant is removed (DEC-205).

**R-META-068** (MUST) The wait-for-start page is an empty page with the default left box. The host shows it from the moment it issues `StartMatch` until the match-start command executes, so that the automatic start (R-META-139) issues the command only once.

**R-META-069** (MUST) Creating an error stores its message and its back page (by default the title page), requests the error page, clears the at-match-setup flag and clears the chat log. The error page holds the title `ui.error.title`, the message in the small font, centred, at y = lY + `ref.meta.error.message_y` (0.5 × bh + 2 × bh), and a Back button to the stored back page. The stored back page is cleared once the page is built.

**R-META-070** (MUST) The errors that the front end reports, with their back pages, are (Species defines its own removal reasons, DEC-244 item 10):

| Trigger | Message | Back page |
|---|---|---|
| a browser row of an incompatible session | `ui.error.incompatible` | session browser |
| a browser row of a full session | `ui.error.session_full` | session browser |
| a browser row whose map is not installed | `ui.error.map_missing` | session browser |
| removed from the session: full | `ui.error.session_full` | session browser |
| removed from the session: a duplicate identity | `ui.error.duplicate_identity` | title |
| removed from the session: kicked by the host | `ui.error.kicked` | title |
| removed from the session: a wrong password | `ui.error.wrong_password` | session browser |
| the participant id arrives twice | `ui.error.connection` | title |
| joined as a spectator where spectators are not allowed | `ui.error.session_full` | session browser |
| the session removes the participant's own id | `ui.error.session_full` | title |
| any other removal reason | none: the front end goes straight to the title page | — |

### 2.16 Credits page

**R-META-071** (MUST) The credits page holds the title `ui.credits.title`, one scrolling text widget in the small font at (bx, lY + 1.8 × bh), bw wide and lH − 1.5 × bh − 1.3 × bh − bh − 2 × button gap high, and a Back button to the previous page. The text is the strings `ui.credits.line_1`, `ui.credits.line_2` and onwards, each followed by a space and a line break, up to the first index that has no string. A line that starts with "-" is a section heading, drawn `ref.meta.credits.heading_scale` (1.25) × larger without the dash. The scrolling follows the UI framework's scrolling text widget. The credit text is new (`ui.credits`).

### 2.17 Help-and-options branch

**R-META-072** (MUST) The windows of this branch are full-screen options screens laid out in the left box (R-META-014), with their buttons from top to bottom and Back at y = lY + lH − 2 × bh:

| Window | Content |
|---|---|
| help and options | title `ui.help_and_options.title`; how to play, `ui.help_and_options.how_to_play`, → help selection; settings, `ui.help_and_options.settings`, → the settings screen, unless it is already open; Back |
| help selection | title `ui.help_select.title`; general help, `ui.help_select.general`, → general help; one button per mode in the order of the mode catalogue, captioned `mode.<mode>.name`, → that mode's help; Back |
| general help | full width; page 1 to `ref.meta.help.general_pages` (5); previous, `ui.help.previous`, and next, `ui.help.next`, each half the width, change the page by −1 and +1 by rebuilding the window; leaving the range 1 to 5 closes the window; Back |
| mode help | the mode's help pages; a page exists only while its text string exists, and the window closes itself when it reaches a page that does not |

The content and the string ids of the help pages are [reference/11](../11_tutorial_and_help.md). The settings screen, its screen, graphics, sound and other options, and where each option screen's apply action returns to are [options_localisation_and_hooks.md](options_localisation_and_hooks.md).

### 2.18 In-match menus and dialogs

**R-META-073** (MUST) The menu action `action.match.menu` opens the in-match menu once the match's fade-in has finished and no tutorial script runs, when no UI window is open and the Power slots panel is hidden ([reference/06](../06_controls_camera_and_commands/actions_and_bindings.md) row M4, [reference/12](../12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-581 step 5). Opening it sets the camera's debug mode to automatic, a camera mode that [reference/06](../06_controls_camera_and_commands/camera.md) does not specify (DEC-024), and hides the interface. The menu does not pause the match, and a lost window focus does not hold the world either (R-SIM-581 step 7).

**R-META-074** (MUST) The in-match menu holds:

| Widget | Caption | Action |
|---|---|---|
| title | `ui.match_menu.title` | — |
| help and options | `ui.match_menu.help_and_options` | opens the help-and-options window (R-META-072) |
| leave | `ui.match_menu.leave` | when the match is over, the leave action at once (R-META-076); otherwise → the leave confirmation |
| back | `ui.match_menu.back` | in a match, moves the pointer to the screen centre; removes the window; shows the interface |

**R-META-075** (MUST) The leave confirmation holds the title `ui.confirm_leave.title`, a yes button `ui.confirm_leave.yes`, which runs the leave action, and a no button `ui.confirm_leave.no`, which closes the confirmation.

**R-META-076** (MUST) The **leave action** removes every window, clears the requested match and returns the application to the front end. The match and its session are torn down when the main loop sees the request ([reference/12](../12_simulation_model/02_match_lifecycle_and_state_reset.md) R-SIM-028; DEC-221).

**R-META-077** (MUST) The gamepad-disconnected dialog opens on `action.device.pad_disconnected` while the gamepad is known to be connected, and that knowledge is then cleared; it closes on `action.device.pad_connected`, which sets it again ([reference/06](../06_controls_camera_and_commands/actions_and_bindings.md) row M7). While the gamepad is disconnected and the input mode is gamepad, the application treats its window as unfocused. On its first update the dialog shows the message `ui.dialog.pad_disconnected` in a dialog box, which is all it draws; it closes itself when the message is dismissed (the UI framework's dismiss action: the primary pointer button, Space, Enter, Esc or the south face button) or on `action.match.menu`.

### 2.19 Match setup options

**R-META-078** (MUST) The mode options are the typed per-mode schemas of [reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-073 to R-MODE-077, with explicit defaults per mode (DEC-073); that document owns their labels, ranges, steps, defaults and meanings. The reference match setup holds the options of the current mode as an array of `ref.meta.setup.mode_option_slots` (64) integer entries, indexed by the option's position in the mode's display order (DEC-243 item 10). The advanced options page shows them in that order (R-META-057). For readability, the values of R-MODE-074 are repeated here:

| Mode | Options in display order: control, range and step, default |
|---|---|
| Spawn Control | score mode: list, 0 to 1, 0; time limit: spinner, no limit or 1 to 60 minutes in steps of 1, 10; Catch-up bonus: spinner, 0 to 90 % in steps of 10, 90; crate drop period: spinner, none or 10 to 300 s in steps of 10, 40; starting powers: spinner, 0 to 3, 0; Comeback Bonus: switch, on; sudden death: switch, on; crate placement: list, random or weighted, weighted |
| Zone Control | score mode: list, 0 to 3, 0; time limit: spinner, 1 to 60 minutes, 5; Catch-up bonus: 90; crate drop period: 40; starting powers: 0; Comeback Bonus: on; sudden death: on; crate placement: weighted |
| Relic Run | time limit: 1 to 60 minutes, 10; crate drop period: 40; Comeback Bonus: on; sudden death: on; crate placement: weighted |
| Siege | Catch-up bonus: 90; crate drop period: 40 |
| Launch Race | crate drop period: 40; reinforcement interval: spinner, 10 to 300 mode seconds in steps of 10, 60; reinforcement count: spinner, 1 to 100, 40; Transport cap: spinner, 1 to 10, 2; turret frequency: spinner, 0 to 10, 7; turret cap: spinner, 0 to 5, 3 |
| Flag Chain | time limit: no limit or 1 to 60 minutes, 15; crate drop period: 40; reinforcement interval: 60; reinforcement count: 40; Transport cap: 2; turret frequency: 2; Comeback Bonus: on; sudden death: on; turret cap: 2 |

Siege has no time limit, so a map's play time does not apply to it; its player options page offers sides instead of colours (R-META-062), and the single-round Siege flag has no control (DEC-066).

**R-META-079** (MUST) Defaults are applied to every option of the mode when a mode is pressed (R-META-042), when the host enters the match setup page (R-META-054 step 7) and by the tutorial set-up (R-META-037).

**R-META-080** (MUST) These match setup settings lie outside the mode schemas:

| Setting | Where it is changed | Values and default | Who, in the user interface |
|---|---|---|---|
| bot difficulty | match setup page list | Easy, Standard or Hard; Standard, set when the session starts; Easy in a tutorial (R-MODE-082) | the host |
| basic crates only | advanced options switch | off or on; off | the host |
| co-op | advanced options switch, on maps that allow co-op | off or on; switched by the map choice (R-META-052) | the host |
| password | advanced options text field | at most 16 characters; empty | the host of a multiplayer session |
| single-round Siege | no control | on (DEC-066) | nobody |
| map | map-select page; it cannot change on the match setup page | the maps listed for the mode | the host |
| mode | mode-select page | the six modes | the host |
| player name | player options page; the chat rename (R-META-157) | at most 16 characters, at most `ref.meta.chat.rename_max_length` (15) through the chat | each player |
| colour or side | player options page | the eight colours, or random, attackers and defenders in Siege (§2.23) | each player |
| open or closed slot | match setup page slot control (R-META-144) | open by default (DEC-079) | the host |
| seasonal theme | no control (§2.27) | the host's date rule (DEC-101) | nobody |
| research levels | no page | overwritten at every match start (DEC-115) | nobody |

**R-META-081** (MUST) The match setup holds research levels, which no page offers. Match start overwrites them with the fixed match levels ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-082; DEC-115), so they have no effect.

### 2.20 Who can change what

**R-META-082** (MUST) The user interface offers these controls:

| Setting | Host | Joining player |
|---|---|---|
| mode and map | the mode-select and map-select pages, before the match setup page | no page |
| bot difficulty | the match setup page list | the control is not created |
| mode options, basic crates only, co-op, password | the advanced options page | the button to the page is not created |
| own name, colour or side | the player options page | the player options page |
| open or closed slots | the slot controls | not created |
| ready | yes, and the host must also ready up | yes |
| kick | yes | no |
| start | automatic once every human is ready (R-META-139) | no |

**R-META-083** (MUST) Under the reference default the restriction of R-META-082 lies in the user interface only: every participant applies every match-setup change, start and ready command that any participant issues, and only kicking acts through the host alone. Which participant may change each setting, and how the authority validates each command, are DEC-243 and DEC-202.

### 2.21 Match setup state

**R-META-084** (MUST) The front-end screen keeps this local state, which no other participant sees:

| State | Meaning | Initial |
|---|---|---|
| current page, requested page | the page shown and the page to show next | none, the title page (R-META-008) |
| single-player flag | the single-player path: a private local session, Play instead of Ready, no chat and no kick | clear |
| quick-start flag, tutorial-starting flag | a quick start is running; a tutorial has been set up | clear |
| navigation lock | the page navigation is frozen during a tutorial launch | clear |
| local map choice | the map chosen on this participant, used so that nothing starts before the match setup holds the same map | none |
| page-built-for mode, pressed mode | the mode the current page was built for; the mode pressed and not yet applied | none; written before every use |
| highlighted map, highlighted mode | the focused map and mode, which drive the preview panels | none, the first mode |
| requested colour, previous requested colour | the colour or side request | none, none |
| player name | at most 16 characters | the player-name setting |
| session password | at most 16 characters | empty |
| waiting-for-team flag, waiting mode | the match setup page is deferred until a team slot arrives | clear, none |
| leaving-while-ready flag | the player is leaving while ready and waits until readiness is withdrawn (R-META-150) | clear |
| chat log, current chat line, chat-toggled-this-update flag | §2.26 | empty |
| error message, error back page | §2.15 | none, none |
| session browser state | DEC-245 | — |

**R-META-085** (MUST) The match setup that every participant shares holds what a match needs (DEC-243 item 10): the mode; the map, by map id; the mode options; the research levels; the bot difficulty; the co-op flag; the single-round Siege flag; the basic-crates-only flag; for each player slot, its owner, kind, name, colour, ready flag and whether it is open (DEC-079); the chat; the achievement masks ([options_localisation_and_hooks.md](options_localisation_and_hooks.md)); the seasonal state (DEC-118); the password; and the start. Choosing a mode sets every mode option to its default, and choosing a map sets the time limit (R-META-042, R-META-052).

**R-META-086** (MUST) Under the reference default every option, and every element of the option arrays, carries a version number. A change carries the version that the sender's copy holds + 1, and a participant applies it only when that version is greater than the stored one, and then stores it (DEC-243 item 5). Whether Species keeps this model or an authority-owned snapshot is DEC-243.

### 2.22 Single player, host and client

**R-META-087** (MUST) The match setup page differs by role:

| Aspect | Single player | Host | Joining player |
|---|---|---|---|
| session | private and local | hosted, listed for joining once a map is chosen | the host's |
| primary button | Play, which starts the match at once | Ready, a toggle; the match starts automatically once every human is ready | Ready, a toggle |
| bot difficulty and advanced options | yes | yes | no |
| slot controls | open and close | kick, open and close | no |
| chat | no; the map description instead | yes | yes |
| Back goes to | the map-select page, and the session stays up | the mode-select page; the session ends and every joined player is dropped | the host-or-join page, or the title page when leaving while ready (R-META-150) |

**R-META-088** (MUST) The host cannot change the map or the mode without ending its session, which drops every joined player.
