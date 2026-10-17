import sys, os, csv
src = sys.argv[1]; out = sys.argv[2]
S1="1. Cutscene script language (Script)"
S2="2. Multiwinia tutorials (the live tutorials)"
S3="3. In-game help (MultiwiniaHelp non-tutorial)"
S4="4. Dr Sepulveda dialogue system"
S5="5. Darwinia demo tutorials"
S6="6. Darwinia context-sensitive help (HelpSystem)"
S7="7. Constants table"
rows=[]
def add(p,d,r,s): rows.append((p,d,r,";".join(s)))
add("code/helpsystem.cpp","full","",[S6,S7])
add("code/helpsystem.h","full","",[S6,S7])
add("code/multiwiniahelp.cpp","full","",[S2,S3,S7])
add("code/multiwiniahelp.h","full","",[S2,S3])
add("code/script.cpp","full","",[S1,S7])
add("code/script.h","full","",[S1])
add("code/sepulveda.cpp","full","",[S4,S7])
add("code/sepulveda.h","full","",[S4,S7])
add("code/sepulveda_strings.cpp","full","",[S4])
add("code/tutorial.cpp","full","",[S5,S7])
add("code/tutorial.h","full","",[S5])
add("code/tutorial_demo1.cpp","full","",[S5])
add("code/tutorial_demo2.cpp","full","",[S5])
for f in sorted(os.listdir(os.path.join(src,"data/scripts"))):
    add("data/scripts/"+f,"full","",[S1])
for f in sorted(os.listdir(os.path.join(src,"data/mwhelp"))):
    add("data/mwhelp/"+f,"skimmed","binary BMP: header (dimensions/bpp) read and every code reference traced; pixel content not inspected",[S3])
for f in sorted(os.listdir(os.path.join(src,"data/sepulveda"))):
    add("data/sepulveda/"+f,"skimmed","binary BMP: header (256x256 8bpp) read; usage traced in Sepulveda::RenderFace; pixel content not inspected",[S4])
extra=[
("code/attract.cpp","full","",[S1]),
("code/UI/TutorialButton.h","full","",[S2]),
("code/interface/helpandoptions_windows.cpp","skimmed","read help/select/general/game-type help windows (L160-345, L640-1000); crate help window and text buttons not needed",[S3]),
("code/UI/GameMenuWindow.cpp","skimmed","read tutorial page/setup, main-page tutorial buttons, quick-start, help-page and mwhelp image sites only",[S2,S3]),
("code/UI/GameTypeImageButton.h","skimmed","mode image selection only",[S3]),
("code/main.cpp","skimmed","read location and global-world loops for Script/Sepulveda/Help call order",[S1,S3]),
("code/app.cpp","skimmed","read construction of script/help/tutorial objects, LoadProfile, server start AI override",[S1,S2,S5]),
("code/camera.cpp","skimmed","read camera functions used by script commands and shift-speed help",[S1,S3]),
("code/global_world.cpp","skimmed","read RunScript event action, EvaluateEvents, ParseTutorial",[S1,S5]),
("code/game_menu.cpp","skimmed","tutorial return-to-menu lines only",[S2]),
("code/location.cpp","skimmed","tutorial hooks (crates, team colours, team positions) only",[S2]),
("code/location_input.cpp","skimmed","deselect-help and notify trigger sites only",[S3]),
("code/multiwinia.cpp","skimmed","grace period, LoadBlueprints, SetGameOption, tutorial score/time display only",[S2,S3]),
("code/taskmanager.cpp","skimmed","tutorial TerminateTask/Stop hooks only",[S2]),
("code/renderer.cpp","skimmed","help/sepulveda/tutorial render call sites only",[S3,S4]),
("code/worldobject/ai.cpp","skimmed","tutorial hooks only",[S2]),
("code/worldobject/spawnpoint.cpp","skimmed","tutorial PopulationLocked hook only",[S2]),
("code/worldobject/crate.cpp","skimmed","tutorial crate decay/airstrike hooks only",[S2]),
("code/worldobject/officer.cpp","skimmed","tutorial formation hooks only",[S2]),
("code/worldobject/darwinian.cpp","skimmed","tutorial grenade hook only",[S2]),
("code/worldobject/radardish.cpp","skimmed","tutorial ValidReceiverDish hook only",[S2]),
("code/worldobject/rocket.cpp","skimmed","launchpad_victory trigger and GetStateId only",[S1]),
("code/worldobject/building.cpp","skimmed","BuildingIsBlocked list only",[S1]),
("code/mapcrc.h","skimmed","KOTH map ids only",[S2]),
("code/level_file.h","skimmed","MAGIC_MOUNT_NAME_START_POS only",[S1]),
("contrib/systemiv/lib/filesys/text_stream_readers.cpp","skimmed","separators, comment and line reading only",[S1]),
("data/input/input_preferences.txt","skimmed","SkipCutscene/SkipMessage/SkipTutorial/SepulvedaHelp bindings only",[S1,S4,S5,S6]),
("data/language/english.txt","skimmed","help/tutorial/Sepulveda phrase ids only (checked by P19_phrases.py)",[S2,S3,S4]),
("data/levels/mp_koth_2P_1.txt","skimmed","building list for tutorial 1 ids",[S2]),
("data/levels/mp_koth_3P_1.txt","skimmed","building list for tutorial 2 ids",[S2]),
("data/levels/map_mine.txt","skimmed","building types for Demo 1 ids",[S5]),
("data/levels/mission_mine_demo.txt","skimmed","camera mounts and script triggers",[S1,S5]),
("data/levels/map_launchpad.txt","skimmed","building types for Demo 2 ids",[S5]),
("data/levels/mission_launchpad.txt","skimmed","camera mounts and script triggers",[S1,S5]),
("data/game.txt","skimmed","Action RunScript lines",[S1]),
("data/multiwinia.txt","skimmed","KOTH blueprint defaults",[S2]),
("data/sounds.txt","skimmed","Music and Sepulveda event blocks only",[S1,S4]),
("tools/multiwinia_exclusions.txt","full","",[S1,S3]),
("tools/build_multiwinia_data.bat","skimmed","exclusion selection and mwhelp conversion steps",[S1,S3]),
("docs/help system.txt","full","",["Docs-vs-code discrepancies"]),
]
for e in extra: add(*e)
with open(out,"w",newline="") as fh:
    w=csv.writer(fh); w.writerow(["path","depth","reason","sections"])
    for r in rows: w.writerow(r)
print(len(rows))
