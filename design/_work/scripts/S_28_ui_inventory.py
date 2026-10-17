#!/usr/bin/env python3
"""S_28 - build-tag every UI class found by S_28_ui_classes.py and emit
(1) a Markdown inventory table (every declaration) and
(2) the master Mermaid class diagram for document 28.

Usage: python3 -I S_28_ui_inventory.py <classes_csv> <out_table_md> <out_mermaid_md>

The status of each class was determined by reading the code (instantiation sites, #ifdef guards,
vcproj membership); the mapping below is the evidence-bearing part and is documented in
28_ui_framework.md section 12.
"""
import csv
import sys

# status codes -> (tag, meaning)
STATUS = {
    "LIVE":    ("[B:FULL]", "live in the shipped build"),
    "UNREACH": ("[B:LEGACY]", "compiled into FULL, never reached"),
    "EDUSE":   ("[B:FULL]*", "compiled into FULL, only used by editor/debug tools"),
    "EDITOR":  ("[B:EDITOR]", "LOCATION_EDITOR builds only (or compiled-in but only reachable from editor code)"),
    "DEBUG":   ("[B:DEBUG]", "TARGET_DEBUG builds only"),
    "OTHER":   ("[B:DEAD]", "other product target / platform only"),
    "DEAD":    ("[B:DEAD]", "not compiled in any PC configuration"),
}

# file-level default status
FILE_STATUS = {
    "contrib/eclipse/eclbutton.h": "LIVE", "contrib/eclipse/eclipse.h": "LIVE", "contrib/eclipse/eclwindow.h": "LIVE",
    "code/game_menu.h": "LIVE", "code/game_menu.cpp": "LIVE",
    "code/interface/buildings_window.cpp": "EDITOR", "code/interface/buildings_window.h": "EDITOR",
    "code/interface/buynow_window.cpp": "DEAD", "code/interface/buynow_window.h": "DEAD",
    "code/interface/camera_anim_window.cpp": "EDITOR", "code/interface/camera_anim_window.h": "EDITOR",
    "code/interface/camera_mount_window.cpp": "EDITOR", "code/interface/camera_mount_window.h": "EDITOR",
    "code/interface/chatinput_window.h": "LIVE", "code/interface/chatinput_window.cpp": "LIVE",
    "code/interface/cheat_window.cpp": "DEBUG", "code/interface/cheat_window.h": "DEBUG",
    "code/interface/checkbox.h": "LIVE",
    "code/interface/controllerunplugged_window.h": "LIVE",
    "code/interface/darwinia_window.h": "LIVE",
    "code/interface/debugmenu.cpp": "UNREACH", "code/interface/debugmenu.h": "UNREACH",
    "code/interface/demoend_window.cpp": "UNREACH", "code/interface/demoend_window.h": "UNREACH",
    "code/interface/drop_down_menu.h": "LIVE",
    "code/interface/editor_window.cpp": "EDITOR", "code/interface/editor_window.h": "EDITOR",
    "code/interface/filedialog.cpp": "EDUSE", "code/interface/filedialog.h": "EDUSE",
    "code/interface/gesture_window.cpp": "DEAD", "code/interface/gesture_window.h": "DEAD",
    "code/interface/globalworldeditor_window.cpp": "UNREACH", "code/interface/globalworldeditor_window.h": "UNREACH",
    "code/interface/grabber_window.cpp": "DEBUG", "code/interface/grabber_window.h": "DEBUG",
    "code/interface/helpandoptions_windows.cpp": "LIVE", "code/interface/helpandoptions_windows.h": "LIVE",
    "code/interface/input_field.h": "LIVE",
    "code/interface/instant_unit_window.cpp": "EDITOR", "code/interface/instant_unit_window.h": "EDITOR",
    "code/interface/invalidcrates_window.h": "EDITOR",
    "code/interface/landscape_window.cpp": "EDITOR", "code/interface/landscape_window.h": "EDITOR",
    "code/interface/leaderboards_window.cpp": "UNREACH", "code/interface/leaderboards_window.h": "UNREACH",
    "code/interface/lights_window.cpp": "EDITOR", "code/interface/lights_window.h": "EDITOR",
    "code/interface/mainmenus.cpp": "LIVE", "code/interface/mainmenus.h": "LIVE",
    "code/interface/message_dialog.cpp": "LIVE", "code/interface/message_dialog.h": "LIVE",
    "code/interface/mods_window.cpp": "DEAD", "code/interface/mods_window.h": "DEAD",
    "code/interface/multiwinia_filedialog.cpp": "EDITOR", "code/interface/multiwinia_filedialog.h": "EDITOR",
    "code/interface/multiwinia_window.cpp": "EDITOR", "code/interface/multiwinia_window.h": "EDITOR",
    "code/interface/multiwiniaoptions_window.cpp": "EDITOR", "code/interface/multiwiniaoptions_window.h": "EDITOR",
    "code/interface/network_window.h": "LIVE",
    "code/interface/prefs_graphics_window.cpp": "LIVE", "code/interface/prefs_graphics_window.h": "LIVE",
    "code/interface/prefs_keybindings_window.cpp": "UNREACH", "code/interface/prefs_keybindings_window.h": "UNREACH",
    "code/interface/prefs_other_window.cpp": "LIVE", "code/interface/prefs_other_window.h": "LIVE",
    "code/interface/prefs_screen_window.cpp": "LIVE", "code/interface/prefs_screen_window.h": "LIVE",
    "code/interface/prefs_sound_window.cpp": "LIVE", "code/interface/prefs_sound_window.h": "LIVE",
    "code/interface/profilewindow.cpp": "DEBUG", "code/interface/profilewindow.h": "DEBUG",
    "code/interface/reallyquit_window.h": "UNREACH",
    "code/interface/save_on_quit_window.cpp": "UNREACH", "code/interface/save_on_quit_window.h": "UNREACH",
    "code/interface/scrollbar.h": "LIVE",
    "code/interface/sound_profile_window.cpp": "DEBUG", "code/interface/sound_profile_window.h": "DEBUG",
    "code/interface/soundeditor_window.cpp": "DEBUG", "code/interface/soundeditor_window.h": "DEBUG",
    "code/interface/soundparameter_window.cpp": "DEBUG", "code/interface/soundparameter_window.h": "DEBUG",
    "code/interface/soundstats_window.h": "DEBUG",
    "code/interface/tree_window.cpp": "EDITOR", "code/interface/tree_window.h": "EDITOR",
    "code/interface/updateavailable_window.cpp": "DEAD", "code/interface/updateavailable_window.h": "DEAD",
    "code/interface/userprofile_window.cpp": "UNREACH", "code/interface/userprofile_window.h": "UNREACH",
}

# per-class overrides (class -> status)
CLASS_STATUS = {
    # darwinia_window.h
    "GameExitButton": "UNREACH",          # only ReallyQuitWindow
    "LabelButton": "EDITOR",              # only lights_window
    # input_field.h
    "ColourWidget": "EDUSE", "ColourWindow": "EDUSE",
    # debugmenu
    "DebugKeyBindings": "LIVE",           # NetworkButton (F9) is live
    # helpandoptions
    "CrateHelpWindowButton": "DEBUG", "CrateButton": "DEBUG", "CrateHelpWindow": "DEBUG",
    # mainmenus.h / .cpp
    "MainMenuWindow": "UNREACH", "ResetLocationWindow": "UNREACH", "AboutDarwiniaWindow": "DEAD",
    "ConfirmResetWindow": "UNREACH", "MenuGameExitButton": "UNREACH", "AchievementsButton": "UNREACH",
    "LeaderBoardButton": "UNREACH", "ConfirmResetButton": "UNREACH", "AboutDarwiniaButton": "DEAD",
    "MainMenuUserProfileButton": "UNREACH", "ModsButton": "DEAD", "MultiwiniaButton": "UNREACH",
    "KeybindingsOptionsButton": "UNREACH", "ResetLevelButton": "UNREACH", "WebsiteButton": "UNREACH",
    "ResetLocationButton": "UNREACH",
    # code/UI (P18 inventory, verified by include/instantiation grep)
    "AnyLevelSelectButton": "UNREACH", "HelpImageButton": "UNREACH", "NextHelpButton": "UNREACH",
    "PrevHelpButton": "UNREACH", "CampaignButton": "DEBUG", "PrologueButton": "DEBUG",
    "MultiwiniaVersusCpuButton": "DEBUG",
    "AchievementButton": "UNREACH", "BuyNowInfoButton": "UNREACH", "CSpectatorButton": "DEAD",
    "DarwiniaPlusMPButton": "UNREACH", "DarwiniaPlusSPButton": "UNREACH", "GameTypeAnyButton": "UNREACH",
    "HelpSelectButton": "UNREACH", "QuickMatchButton": "UNREACH",
    "GameMenuOptionsButton": "DEAD", "HelpModeButton": "DEAD", "MultiwiniaModeButton": "DEAD",
    "QuickStartButton": "DEAD", "RequestAIButton": "DEAD", "ResearchModeButton": "DEAD",
    "RollingDemoButton": "DEAD", "MapDetailsButton": "DEAD",
    "QuitAuthkeyButton": "OTHER", "StartTestbedButton": "DEAD", "MultiwiniaEditorButton": "EDITOR",
    "RunSparkleButton": "OTHER", "ErrorDialog": "DEAD", "VolumeControl": "DEAD",
}
UI_DEFAULT = "LIVE"   # every other code/UI class is live (P18 inventory)

FRAMEWORK_FILES = {
    "contrib/eclipse/eclbutton.h", "contrib/eclipse/eclipse.h", "contrib/eclipse/eclwindow.h",
    "code/interface/darwinia_window.h", "code/interface/input_field.h", "code/interface/scrollbar.h",
    "code/interface/checkbox.h", "code/interface/drop_down_menu.h", "code/interface/message_dialog.cpp",
    "code/interface/message_dialog.h", "code/interface/filedialog.h", "code/interface/filedialog.cpp",
    "code/game_menu.h", "code/interface/mainmenus.h", "code/interface/chatinput_window.h",
    "code/interface/helpandoptions_windows.h", "code/interface/multiwinia_filedialog.cpp",
}
WINDOW_BASES = {"EclWindow", "DarwiniaWindow", "GameOptionsWindow", "FileDialog", "MessageDialog",
                "DropDownWindow"}


def status_of(cls, path):
    if cls in CLASS_STATUS:
        return CLASS_STATUS[cls]
    if path.startswith("code/UI/"):
        return UI_DEFAULT
    return FILE_STATUS.get(path, "LIVE")


def main():
    src, out_table, out_mmd = sys.argv[1], sys.argv[2], sys.argv[3]
    rows = list(csv.DictReader(open(src)))
    for r in rows:
        r["status"] = status_of(r["class"], r["file"])

    # ---- table
    with open(out_table, "w") as fh:
        fh.write("| # | Class | Base | Declared in | Status | Tag |\n|---|---|---|---|---|---|\n")
        for i, r in enumerate(rows, 1):
            tag, meaning = STATUS[r["status"]]
            base = f"`{r['base']}`" if r["base"] else "—"
            fh.write(f"| {i} | `{r['class']}` | {base} | `{r['file']}:{r['class']}` | {meaning} | {tag} |\n")
        counts = {}
        for r in rows:
            counts[r["status"]] = counts.get(r["status"], 0) + 1
        fh.write("\nTotals: " + "; ".join(f"{STATUS[k][1]} = {v}" for k, v in sorted(counts.items())) +
                 f"; all declarations = {len(rows)}.\n")

    # ---- mermaid (framework + windows + code/UI + game_menu; screen-private buttons are table-only)
    chosen, seen = [], set()
    for r in rows:
        if r["class"] in seen:
            continue
        in_diag = (r["file"] in FRAMEWORK_FILES or r["file"].startswith("code/UI/")
                   or r["base"] in WINDOW_BASES or r["file"] == "code/game_menu.h")
        if in_diag:
            chosen.append(r)
            seen.add(r["class"])
    ann = {"LIVE": None, "EDUSE": "tooluse", "UNREACH": "legacy", "EDITOR": "editor",
           "DEBUG": "debug", "OTHER": "othertarget", "DEAD": "dead"}
    lines = ["classDiagram", "  direction LR"]
    externals = set()
    for r in chosen:
        if r["base"] and r["base"] not in seen:
            externals.add(r["base"])
    for e in sorted(externals):
        lines.append(f"  class {e}")
        lines.append(f"  <<external>> {e}")
    for r in chosen:
        lines.append(f"  class {r['class']}")
        a = ann[r["status"]]
        if a:
            lines.append(f"  <<{a}>> {r['class']}")
    for r in chosen:
        if r["base"]:
            lines.append(f"  {r['base']} <|-- {r['class']}")
    # composition / ownership relations of the core
    lines += ["  EclWindow \"1\" o-- \"*\" EclButton : m_buttons",
              "  ScrollBar ..> ScrollBarButton : registers in parent",
              "  ScrollBar ..> ScrollChangeButton : registers in parent",
              "  DropDownMenu ..> DropDownWindow : opens singleton",
              "  GameMenuDropDown ..> GameMenuDropDownWindow : opens singleton",
              "  InputScroller ..> InputField : m_inputField",
              "  GameMenu ..> GameMenuWindow : creates and destroys",
              "  GameMenuWindow ..> ScrollBar : m_scrollBar",
              "  GameMenuWindow ..> MapData : via GameMenu m_maps"]
    with open(out_mmd, "w") as fh:
        fh.write("```mermaid\n" + "\n".join(lines) + "\n```\n")
    print(len(rows), "rows;", len(chosen), "diagram classes;", len(externals), "external bases")


if __name__ == "__main__":
    main()
