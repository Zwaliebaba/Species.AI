# Phase 6 round 2 queue (coordinator)

Requests addressed to a packet that had already finished when the request was made, and coordinator follow-ups. Round 2 re-spawns the owners with these and with every finding still open.

| From | To | Item |
|---|---|---|
| P6-RD | P6-RB | requests 17 (defences_and_props.md table 2.10-A, `turret.fire_grenade` row) and 18 (02 README R-UNIT-008, optional) of reports/P6-RD.md |
| P6-RD | P6-RA | request 19 (01 README R-MODE-082 table, "research levels" row, optional) of reports/P6-RD.md |
| P6-RD | coordinator | regenerate the requirements index; then delete the temporary anchor `<a id="21-music-hooks-kh-music"></a>` in reference/10_audio_design/music_hooks_and_sound_design_brief.md (the label is a conversion internal) and repoint any link to it |
| P6-RD | coordinator | removed requirements kept as "(removed: …)": after the index regeneration, delete those that only the index cites |
| P6-RA | coordinator | regenerate the requirements index |
| P6-RE | P6-RB | request 8 of reports/P6-RE.md (optional wording) |
| P6-RE | P6-RA | request 9 of reports/P6-RE.md (R-MODE-057 step 2 "issue the score report" against R-SIM-454) |
| P6-RE | coordinator | regenerate the index; optional: rename reference/03 §2.3.6 "Render hooks" to "Drawing" in the same commit |
| P6-DAT | coordinator | requests 5 (R-AUD-052 link names) and 6 (R-BLDG-340 spawn structure and spawn-completed flag, R1b-3): **applied by the coordinator** |
