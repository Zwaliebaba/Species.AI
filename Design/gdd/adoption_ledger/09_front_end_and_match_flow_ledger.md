# Adoption ledger: 09 front end and match flow

> Status: Draft · Owner: area C2 · Reference: [09_front_end_and_match_flow](../../reference/09_front_end_and_match_flow/README.md) · Rules: `prompts/species-design-phase.md` §5.3

45 sections, 239 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `09_front_end_and_match_flow/front_end_presentation.md §2.1` | pending | | Shared page layout: 4 requirements (R-META-200 to R-META-203) |
| `09_front_end_and_match_flow/front_end_presentation.md §2.2` | pending | | Persistent background: 8 requirements (R-META-204 to R-META-210) |
| `09_front_end_and_match_flow/front_end_presentation.md §2.3` | pending | | Loading view: 9 requirements (R-META-211 to R-META-219) |
| `09_front_end_and_match_flow/front_end_presentation.md §2.4` | pending | | Explainer panel: 14 requirements (R-META-220 to R-META-233) |
| `09_front_end_and_match_flow/front_end_presentation.md §2.5` | pending | | The match start as the front end sees it: 1 requirement (R-META-234) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.1` | pending | | The settings store: 10 requirements (R-META-400 to R-META-409) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.2` | pending | | Settings navigation and the shared page pattern: 4 requirements (R-META-410 to R-META-413) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.3` | pending | | Screen options: 7 requirements (R-META-414 to R-META-424) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.4` | pending | | Graphics options: 3 requirements (R-META-425 to R-META-427) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.5` | pending | | Sound options: 10 requirements (R-META-429 to R-META-439) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.6` | pending | | Other options: 11 requirements (R-META-440 to R-META-450) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.7` | pending | | Settings kept outside the options pages: 7 requirements (R-META-451 to R-META-457) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.8` | pending | | Options the reference does not have: 3 requirements (R-META-458 to R-META-459) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.9` | pending | | Localisation: 17 requirements (R-META-460 to R-META-476) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.10` | pending | | Loading: 3 requirements (R-META-477 to R-META-479) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.11` | pending | | Achievement hooks: 13 requirements (R-META-480 to R-META-493) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.12` | pending | | Statistics hooks: 4 requirements (R-META-494 to R-META-497) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.13` | pending | | Leaderboard hooks: 1 requirement (R-META-498) |
| `09_front_end_and_match_flow/options_localisation_and_hooks.md §2.14` | pending | | Launch Vessel status panel: 6 requirements (R-META-505 to R-META-510) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.1` | pending | | Scope and the page set: 4 requirements (R-META-001 to R-META-004) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.2` | pending | | Front-end lifecycle: 9 requirements (R-META-005 to R-META-013) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.3` | pending | | Navigation, Back and the front-end sound cues: 6 requirements (R-META-014 to R-META-019) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.4` | pending | | Page specification conventions: 2 requirements (R-META-020 to R-META-021) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.5` | pending | | Title page: 3 requirements (R-META-022 to R-META-024) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.6` | pending | | Tutorials page: 2 requirements (R-META-025 to R-META-026) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.7` | pending | | Host-or-join page: 2 requirements (R-META-027 to R-META-028) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.8` | pending | | Mode-select page: 5 requirements (R-META-029 to R-META-033) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.9` | pending | | Map-select page: 9 requirements (R-META-034 to R-META-042) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.10` | pending | | Match setup page: 3 requirements (R-META-043 to R-META-045) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.11` | pending | | Advanced options page: 5 requirements (R-META-046 to R-META-050) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.12` | pending | | Player options page: 2 requirements (R-META-051 to R-META-052) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.13` | pending | | Session pages: 7 requirements (R-META-053 to R-META-059) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.14` | pending | | Credits page: 1 requirement (R-META-060) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.15` | pending | | Help-and-options branch: 1 requirement (R-META-061) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.16` | pending | | In-match menus and dialogs: 5 requirements (R-META-062 to R-META-066) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.17` | pending | | Match setup options: 3 requirements (R-META-067 to R-META-069) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.18` | pending | | Who can change what: 2 requirements (R-META-071 to R-META-072) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.19` | pending | | Match setup state: 3 requirements (R-META-073 to R-META-075) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.20` | pending | | Single player, host and client: 2 requirements (R-META-076 to R-META-077) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.21` | pending | | Player slots, names, colours and sides: 7 requirements (R-META-078 to R-META-084) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.22` | pending | | Ready, start, kick, bots and open or closed slots: 7 requirements (R-META-085 to R-META-091) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.23` | pending | | Leaving the match setup page: 3 requirements (R-META-092 to R-META-094) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.24` | pending | | Chat: 7 requirements (R-META-095 to R-META-101) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.25` | pending | | Seasonal theme at match setup: 2 requirements (R-META-102 to R-META-103) |
| `09_front_end_and_match_flow/screen_flow_and_match_setup.md §2.26` | pending | | Results-screen hooks: 2 requirements (R-META-104 to R-META-105) |
