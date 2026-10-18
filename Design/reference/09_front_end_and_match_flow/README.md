# Front end and match flow

> Status: Reference · Prefix: R-META · Related: [00 Reference overview](../00_reference_overview.md) · [01 Skirmish rules and modes](../01_skirmish_rules_and_modes/README.md) · [06 Controls, camera and commands](../06_controls_camera_and_commands/README.md) · [08 Maps and level design](../08_maps_and_level_design/README.md) · [10 Audio design](../10_audio_design/README.md) · [11 Tutorial and help](../11_tutorial_and_help.md) · [12 Simulation model](../12_simulation_model/README.md) · [UI framework](../../architecture/components/ui_framework/README.md)

This folder specifies the front end of the reference game and the way a match is set up, started and left: the one full-screen menu screen and its pages, the windows layered over it, the match setup and its options, the loading view, the explainer panels shown while the players ready up, the settings pages, localisation, and the hooks of the features that the design phase designs (a results screen, achievements, statistics and leaderboards). Pages are described by what they show and do; how a page is built, focused, navigated and drawn is the [UI framework](../../architecture/components/ui_framework/README.md), in particular its [page recipes](../../architecture/components/ui_framework/pages_and_screen_recipes.md).

## Files

| File | Content | Requirement ids |
|---|---|---|
| [README.md](README.md) | this index: the files, the front end at a glance, a reading guide and the decisions that shape the folder | none |
| [screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) | the page set and the full screen flow; the front-end lifecycle; navigation, Back and the front-end sound cues; every page (title, tutorials, host-or-join, mode select, map select, match setup, advanced options, player options, the session pages, credits); the help-and-options branch; the in-match menu, leave confirmation, chat input and gamepad-disconnected dialog; the match setup options and who can change them; the match setup state; single player, host and client; player slots, names, colours and sides; ready, start, kick, bots and open or closed slots; leaving the match setup; chat; the seasonal theme at match setup; the results-screen hooks | R-META-001 to R-META-105 |
| [front_end_presentation.md](front_end_presentation.md) | the layout that every page and options screen shares; the animated backdrop and the title bar; the loading view; the explainer panel shown during ready-up; the match start as the front end sees it; the version string | R-META-200 to R-META-235 |
| [options_localisation_and_hooks.md](options_localisation_and_hooks.md) | the settings store and every user option (screen, graphics, sound and other options), the option defaults and the front-end constants that the settings use; the localisation hooks (string tables, the language set, fallback); the in-match Launch Race status panel; the achievement, statistics and leaderboard hooks | R-META-400 to R-META-699 |

Each file owns the tuning keys of its own rules. The key families of [screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) and [front_end_presentation.md](front_end_presentation.md) are listed in §4.1 of the former; every other `ref.meta.*` key belongs to [options_localisation_and_hooks.md](options_localisation_and_hooks.md). R-META-106 to R-META-199 and R-META-236 to R-META-399 are unassigned.

## The front end at a glance

- **One screen, many pages.** The front end is one full-screen menu screen over an animated backdrop. The reference has 18 live pages; the library specifies 15 of them. The other three, two credential and trial-edition pages and an update notice, depend on entitlement and online services that the library does not specify (DEC-333, DEC-248, DEC-249).
- **One path for every match.** Single player against bots, hosting and the tutorials all pass through the same match setup page; a joining player reaches it through the session browser and the connecting page. A match starts at once in single player and, in multiplayer, when every human is ready.
- **Bots fill the match.** Every open player slot is occupied: at match start the host fills every empty open slot with a bot, and the host may close slots before the match (DEC-079).
- **Loading and ready-up.** A loading view leads into a frozen match. While the players ready up, an explainer panel for the mode loops at the bottom of the screen; a three-second countdown follows.
- **No results page.** A finished match stays under its game-over overlay until the player leaves through the in-match menu, which rebuilds the front end at its title page (DEC-048).
- **Settings everywhere.** Help and options opens from the title page and from the in-match menu, and leads to the help pages ([reference/11](../11_tutorial_and_help.md)) and to the screen, graphics, sound and other options.

## Reading guide

- **UI builders** read [screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) §2.1 to §2.16 and diagram 3-A and [front_end_presentation.md](front_end_presentation.md) §2.1 and §2.2, together with the [UI framework](../../architecture/components/ui_framework/README.md).
- **Match setup and session designers** read §2.17 to §2.25 of [screen_flow_and_match_setup.md](screen_flow_and_match_setup.md), the match options of [reference/01](../01_skirmish_rules_and_modes/README.md) §2.19, the match-setup commands of [reference/12](../12_simulation_model/06_teams_and_player_commands.md) R-SIM-451 and the deferred decisions DEC-243 to DEC-249.
- **Loading and presentation** start from [front_end_presentation.md](front_end_presentation.md) §2.3 and §2.4, with DEC-226 and DEC-750.
- **Settings and localisation** are in [options_localisation_and_hooks.md](options_localisation_and_hooks.md).
- **Feature hooks**: the results screen is §2.26 of [screen_flow_and_match_setup.md](screen_flow_and_match_setup.md); achievements, statistics and leaderboards are in [options_localisation_and_hooks.md](options_localisation_and_hooks.md).

## Decisions that shape this folder

| DEC | Status | Effect on this folder |
|---|---|---|
| DEC-007 | Resolved | presentation never writes simulation state: the front end changes the match setup only through player commands, and match rules emit presentation events |
| DEC-026 | Resolved | the single-window page flow and the one shared match setup page are kept; the loading view shows progress |
| DEC-048 | Open | a post-match results screen; the library records its hooks only |
| DEC-054 | Dropped | the start-up mini-programs, the first-time intro sequence and the attract mode are not specified |
| DEC-079 | Resolved | empty open slots are filled with bots by default, and the host may close slots before the match |
| DEC-243 | Open | who may change the match setup, and the match-setup state model |
| DEC-244 | Open | joining, admission, reconnection and spectators |
| DEC-245 | Open | session discovery and the session browser |
| DEC-248 | Open | player identity and entitlement; no credential page is specified |
| DEC-249 | Open | online services and quick match |
| DEC-047 | Open | achievements, statistics and leaderboards, as hooks |
| DEC-050 | Open | front-end features that the design phase designs anew: rebinding, control help, objective cards, spectating and quick match |
| DEC-061 | Dropped | user profiles and mods |
| DEC-099 | Resolved | a missing string falls back to the base language |

The full list of decisions that each file cites is in its §8.
