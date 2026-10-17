# Collected "other" sections from all drafts

<!-- from P19_help_tutorial_script -->
## Summary of build reality for this partition

| Component | Files | Compiled in FULL? | Reachable in Multiwinia? | Tag |
|---|---|---|---|---|
| Script interpreter (`Script`) | `code/script.{h,cpp}` | Yes (only `Say`/`ShutUp`/`Highlight`/`ClearHighlights`/`DemoGesture`/`SetCutsceneMode`/`WaitPlayerNotBusy` bodies and `ShowDemoEndSequence` are hollowed out by `USE_SEPULVEDA_HELP_TUTORIAL`) | Ticked every frame in the location loop, but **no Multiwinia path ever loads a script** (see §1.5), and `scripts/*` is excluded from the PC data archive (`tools/multiwinia_exclusions.txt` L36) | **[B:FULL] engine, [B:LEGACY] content** |
| Multiwinia in-game help tabs + the two Multiwinia tutorials (`MultiwiniaHelp`) | `code/multiwiniahelp.{h,cpp}` | Yes; tutorial logic under `INCLUDE_TUTORIAL` (defined in FULLGAME) | Yes | **[B:FULL]** |
| Darwinia help system (`HelpSystem`) | `code/helpsystem.{h,cpp}` | Whole file inside `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` | No | **[B:DEAD]** |
| Dr Sepulveda dialogue (`Sepulveda`) | `code/sepulveda.{h,cpp}`, `code/sepulveda_strings.cpp` | Whole files inside `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` | No | **[B:DEAD]** |
| Darwinia demo tutorials (`Tutorial`, `Demo1Tutorial`, `Demo2Tutorial`) | `code/tutorial.{h,cpp}`, `code/tutorial_demo1.cpp`, `code/tutorial_demo2.cpp` | Whole files inside `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` | No | **[B:DEAD]** |

Evidence: [C] `code/tutorial.h` L5 `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` … L88 `#endif`; `code/tutorial.cpp` L3/L240; (the same whole-file wrapping for `sepulveda*`, `tutorial_demo*` and `helpsystem*` is cited in §4, §5 and §6 below). `INCLUDE_TUTORIAL` defined for FULLGAME: [C] `code/lib/universal_include.h` (L97 FULLGAME block).

**Design consequence.** The *Multiwinia* tutorial is not the Darwinia `Tutorial` class family in `tutorial*.cpp`; it is a context-sensitive "help tab" director implemented inside `MultiwiniaHelp` (`AttachTutorialTo3d` / `AttachTutorial2To3d`) and running on ordinary KOTH maps (`mp_koth_2P_1`, `mp_koth_3P_1`). There is no narrator character, no voice, no script file and no cutscene in the shipped Multiwinia tutorials. [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::Advance` (L1232–1244), `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetupTutorial`, `SetupTutorial2`.

---

<!-- from X1a_text_formats -->
## Format index

| Anchor | Format | Files | Section | Build |
|---|---|---|---|---|
| `fmt-textreader` | shared line reader / tokeniser | all text formats | §1 | [B:FULL] |
| `fmt-level-file` | level map / mission files | `data/levels/**`, `data/purity/map_*`, `mission_*` | §2 | mp: [B:FULL]; campaign: [B:LEGACY] |
| `fmt-level-enum` | achievement level ids | `data/levels/level_enum.txt` | §2.19 | [B:FULL] |
| `fmt-game-txt` | campaign world | `data/game*.txt`, `data/purity/game.txt` | §3 | [B:LEGACY]/[B:DEAD] |
| `fmt-locations-txt` | world-map positions | `data/locations.txt` | §3.4 | [B:LEGACY] |
| `fmt-multiwinia-txt` | game types & parameters | `data/multiwinia.txt`, `data/multiwinia-testbed.txt` | §4 | [B:FULL] / [B:DEAD] |
| `fmt-betatesters` | credit name lists | `data/betatesters.txt`, `data/multiwinia_betatesters.txt` | §5 | [B:LEGACY] / [B:FULL] |
| `fmt-preferences` | preferences | `<P>preferences.txt`, `data/default_preferences.txt` | §6 | [B:FULL] |
| `fmt-input-bindings` | key/mouse/pad bindings | `data/input/input_preferences*.txt`, `<P>user_input_preferences.txt` | §7 | [B:FULL] |
| `fmt-keyboard-layout` | layout overrides | `data/input/keyboards/*.txt` | §7.6 | [B:FULL] |
| `fmt-locales` | LANGID → layout | `data/input/locales.txt` | §7.7 | [B:FULL] |
| `fmt-language` | phrase tables | `data/language/*.txt`, `strings_*.txt` overlays | §8 | [B:FULL] |
| `fmt-script` | cutscene scripts | `data/scripts/*.txt` | §9 | [B:LEGACY] |
| `fmt-purity-misc` | purity data set | `data/purity/*` | §10 | [B:DEAD] |
| `fmt-testbedconfig` | test-bed sequence | `testbedconfig.txt` | §11 | [B:DEAD] |
| `fmt-support-ini` | crash-report file list | `MultiwiniaSupport.ini` | §12 | [B:FULL tool] |
| `fmt-runtime-files` | runtime-written files (index) | profile directory, cwd | §13 | various |
| `fmt-profile-save` | campaign save games | `<P>users/<profile>/*` | §13.3 | [B:LEGACY] |
| `fmt-authkey` | authentication key | `<P>authkey.dev`, `<P>authkey` | §13.4 | [B:FULL] |
| `fmt-sync-dump` | desync dumps | `<P>syncerror-*.txt`, `<P>syncdiff-*.txt` | §13.5 | [B:FULL] |
| `fmt-mwm` | custom map container | `<P>maps/*.mwm` | §13.6 | [B:EDITOR] |

Generated tables: `_work/drafts/X1a_level_field_usage.csv` (every block/field/value × file set, from `_work/scripts/X1a_level_survey.py`), `_work/drafts/X1a_encoding_survey.csv` (from `_work/scripts/X1a_text_survey.py`). Other helper scripts: `_work/scripts/X1a_language_survey.py`, `X1a_input_lines.py`, `X1a_script_check.py`, `X1a_extract_read_funcs.py`, `X1a_coverage.py` (all run with `python3 -I`, paths as arguments).

<a id="fmt-textreader"></a>
