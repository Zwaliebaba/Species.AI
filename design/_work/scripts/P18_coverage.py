import csv, sys, json
out = sys.argv[1]; parts = sys.argv[2]
d = json.load(open(parts))
files = d['P18_ui_gamemenu']['files']
S = {
 1:'1. Front-end lifecycle and global structure', 2:'2. Page enumeration', 3:'3. Screen-flow diagram',
 4:'4. Page-by-page specification', 5:'5. Start sequences', 6:'6. Lobby state model', 7:'7. Demo / full-game gating',
 8:'8. Game-setup options as data', 9:'9. Lobby chat', 10:'10. Metaserver-driven front-end content',
 11:'11. Map metadata, thumbnails and game-type presentation', 12:'12. How GameMenuWindow is built', 13:'13. Constants table',
 'A':'Appendix — dead/legacy code', 'I':'IP register', 'X':'Contradictions / open questions',
}
m = {
 'GameMenuWindow.cpp':[1,2,3,4,5,6,7,8,9,10,11,12,13,'X','I','A'],
 'GameMenuWindow.h':[2,6,12,13,'A'],
 'ImageButton.cpp':[4,12], 'ImageButton.h':[4,12],
 'MapData.cpp':[11], 'MapData.h':[11],
 'AchievementButton.h':[4,12,'A'], 'AdvancedOptionsModeButton.h':[4,7,12], 'AnyLevelSelectButton.h':[4,12,'A'],
 'ApplyNameButton.h':[4,12], 'AuthInputField.h':[4,12,13], 'AuthPageButton.h':[3,4,12], 'AuthStatusButton.h':[4,10,12],
 'BuyNowInfoButton.h':[4,12,'A'], 'BuyNowModeButton.h':[4,7,12], 'CSpectatorButton.h':[4,12,'A'], 'CampaignButton.h':[3,4,12,'A'],
 'ConnectButton.h':[4,12], 'DarwiniaModeButton.h':[3,4,12], 'DarwiniaPlusMPButton.h':[12,'A'], 'DarwiniaPlusSPButton.h':[12,'A'],
 'EnterAuthKeyButton.h':[4,12], 'FancyColourButton.h':[4,12], 'FiltersPageButton.h':[4,12],
 'FinishMultiwiniaButton.h':[4,12,'X'], 'GameMenuOptionsButton.h':[12,'A'], 'GameServerButton.h':[4,7,12,13,'X'],
 'GameTypeAnyButton.h':[12,'A'], 'GameTypeButton.h':[4,7,11,12], 'GameTypeImageButton.h':[4,11,12], 'GameTypeInfoButton.h':[4,11,12],
 'GamerTagButton.h':[4,12], 'HelpImageButton.h':[4,12,'A'], 'HelpModeButton.h':[12,'A'], 'HelpSelectButton.h':[12,'A'],
 'JoinGameButton.h':[4,12], 'KickPlayerButton.h':[4,12], 'LevelSelectButton.h':[4,8,11,12,'X'], 'LevelSelectButtonBase.h':[4,12,13],
 'MapImageButton.h':[4,11,12], 'MapInfoButton.h':[4,11,12], 'MapListTitlesButton.h':[4,12], 'MapSelectModeButton.h':[4,12],
 'MultiwiniaEditorButton.h':[4,12,'A'], 'MultiwiniaModeButton.h':[5,12,'A'], 'MultiwiniaVersusCpuButton.h':[4,12,'A'],
 'NewOrJoinButton.h':[4,7,12], 'NewServerButton.h':[4,12], 'NextHelpButton.h':[4,12,'A'], 'OptionDetailsButton.h':[4,12],
 'PasteKeyButton.h':[4,12], 'PlayDemoAuthkeyButton.h':[4,7,12], 'PlayGameButton.h':[4,12], 'PrevHelpButton.h':[4,12,'A'],
 'PrologueButton.h':[3,4,12,'A'], 'QuickMatchButton.h':[4,12,'A'], 'QuickStartButton.h':[5,12,'A'], 'QuitAuthKeyButton.h':[4,12,'A'],
 'QuitButton.h':[4,12], 'RequestAIButton.h':[5,12,'A'], 'ResearchModeButton.h':[4,12,'A'], 'RollingDemoButton.h':[5,12,'A'],
 'ScrollingTextButton.h':[12,13], 'ServerTitleInfoButton.h':[4,12], 'TestBedButtons.h':[12,'A'], 'TutorialButton.h':[4,5,12],
 'UnlockFullGameButton.h':[4,7,12], 'WaitingButton.h':[4,12], 'credits_button.h':[4,12], 'mapDetailsButton.h':[11,12,'A'],
 'motdButton.h':[10,12], 'update_page_buttons.h':[4,10,12],
}
rows=[]
for f in files:
    b=f.split('/')[-1]
    secs=m[b]
    rows.append([f,'full','',';'.join(S[x] for x in secs)])
extra = [
 ('code/UI/ErrorDialog.cpp','full','outside partition list; same directory; not in vcproj',[ 'A']),
 ('code/UI/ErrorDialog.h','full','outside partition list; same directory; not in vcproj',['A']),
 ('code/UI/soundsliderbutton.h','skimmed','outside partition list; never included (dead)',['A']),
 ('code/game_menu.cpp','skimmed','P17b-owned; read s_pageNames, base buttons, CreateMenu, AddMap, layout helpers only',[1,2,4,11,12,13]),
 ('code/game_menu.h','full','P17b-owned; constants and base class declarations',[12,13]),
 ('code/interface/darwinia_window.cpp','skimmed','P17a-owned; navigation/Remove/highlight only',[12]),
 ('code/interface/darwinia_window.h','skimmed','P17a-owned; navigation members',[12]),
 ('code/interface/chatinput_window.cpp','skimmed','P17b-owned; Keypress (/name) only',[9]),
 ('code/interface/chatinput_window.h','skimmed','P17b-owned; ChatInputField decl',[9,12]),
 ('code/interface/helpandoptions_windows.cpp','skimmed','P17b-owned; HelpAndOptionsWindow::Create only',[3,4]),
 ('code/interface/helpandoptions_windows.h','skimmed','P17b-owned; TextButton decl',[4,12]),
 ('code/interface/mainmenus.cpp','skimmed','P17b-owned; leave-game and HelpAndOptionsButton/Achievements/Leaderboard buttons',[1,4,'X']),
 ('code/interface/mainmenus.h','skimmed','P17b-owned; button decls',[4]),
 ('code/interface/leaderboards_window.cpp','skimmed','P17b-owned; grep for GameMenuWindow use',[2]),
 ('code/interface/scrollbar.h','skimmed','P17a-owned; Create signature',[4,12]),
 ('code/multiwinia.cpp','skimmed','P02-owned; LoadBlueprints, params, colours, GetDifficulty, RemoveTeams',[4,8,9]),
 ('code/multiwinia.h','skimmed','P02-owned; enums and constants',[8,13]),
 ('data/multiwinia.txt','full','option tables cross-check (P02 owns semantics)',[8]),
 ('code/app.cpp','skimmed','P01-owned; permission predicates, StartNetwork, ShutdownCurrentGame, GetBuyNowURL',[4,7]),
 ('code/main.cpp','skimmed','P01-owned; MainMenuLoop only',[1]),
 ('code/location_input.cpp','skimmed','game-over return path only',[1]),
 ('code/network/demo_limitations.cpp','full','demo defaults',[7]),
 ('code/network/demo_limitations.h','full','demo limitations fields',[7]),
 ('code/network/networkvalue.h','skimmed','P15-owned; NetworkGameOption classes',[6]),
 ('code/network/networkvalue.cpp','skimmed','P15-owned; Set implementations',[6,8]),
 ('code/network/clienttoserver.cpp','skimmed','P15-owned; menu-facing handlers (disconnect, chat, coop, start game, options request)',[1,4,6,9]),
 ('code/network/network_defines.h','skimmed','P15-owned; GAMEOPTION ids, password key',[4,6]),
 ('contrib/systemiv/lib/metaserver/metaserver_defines.h','skimmed','P15-owned; directory keys',[4,'X']),
 ('contrib/systemiv/lib/metaserver/authentication.h','skimmed','P15-owned; key length and status enum',[4,13]),
 ('contrib/systemiv/lib/metaserver/authentication.cpp','skimmed','P15-owned; status phrase mapping only',[4]),
 ('contrib/eclipse/eclwindow.cpp','skimmed','RegisterButton/GetButton ordering only',[12]),
 ('code/renderer.cpp','skimmed','P12-owned; menu transition entry points only',[12]),
 ('data/language/english.txt','skimmed','phrase-id existence checks via script P18_phrases.py',[8,'I']),
 ('code/lib/language_table.cpp','skimmed','LookupPhrase missing-key fallback only',[4,8]),
 ('docs/multiwinia-changes.txt','skimmed','docs-vs-code check',['X']),
 ('docs/multiwiniachanges-beta.txt','skimmed','docs-vs-code check',['X']),
]
S['D']='Docs-vs-code discrepancies'
for f,dep,why,secs in extra:
    secs=[ 'D' if (x=='X' and f.startswith('docs/')) else x for x in secs]
    rows.append([f,dep,why,';'.join(S[x] for x in secs)])
with open(out,'w',newline='') as fh:
    w=csv.writer(fh); w.writerow(['path','depth','reason','sections']); w.writerows(rows)
print(len(files), len(rows))
