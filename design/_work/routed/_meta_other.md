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
