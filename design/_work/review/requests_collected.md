=== F1.md

1. **22** §14.11 L13 (Difficulty preference): not reachable (V1) — every level load resets `m_difficultyLevel` to 0 (`code/level_file.cpp:1488` → `code/app.cpp:632`), no `mp_*` level has `CreatedAsDifficulty`, and `GetStat` scales only `StatSpeed`. Same correction in §14.6 (ParseInstantUnits row), §14.12 (last row), §14.13 and the "Simulation-relevant preferences" row (L1743). 21 L1630 alr
2. **22** §14.11 L20 and §14.12 `Armour::SetDirectOrders` row: unreachable in an unmodified peer (V2): the only `RequestNextWeapon` call is commented out (`code/location_input.cpp:1055-1062`) and `RequestPreviousWeapon` has no caller. Keep only the `s_numMultiwiniaZones` half as live. §5.4 rows 25–26 also need "no live sender".
3. **40** C8, P5, P6, F29 and §"UI/render code" row (L452) and **41** R02, R04 list L13 and/or the armour debounce as live desyncs. Align with V1/V2 (00 §7 Q8 now does).
4. **03** §4.1 / §14 "exactly 1.0 game second per link" → 10h + 1 increments, first hop 11, then 10 (V8; 05 §3.4, 00 §3.2). R3 has the same request.
5. **07** §12.1 "retribution when eliminated" applies to every mode — 01 §2.9 shows Assault, Rocket Riot (and Tank Battle) never grant (R3 L125). For information; 01 unchanged.

=== F2.md

1. **26** (line ≈ 888): "ring 35→60 … 35→135 (Multiwinia values, [04])". These are *render* ring values (`Shockwave::Render` uses `life − predictionTime`, so the drawn ring starts at 35), but they are attributed to 04. Either label them as the render ring or use the damage ring of 04 §6.11 / 02 §2.7 (39→59 grenade, 39→63 rocket, 39→83 bomb, 39→131 nuke/meteor). Evidence `code/w
2. **03** (line ≈ 898): "The task self-destructs the turret if an enemy captures it" is unconditional. It holds only while the turret task runs, i.e. in modes that define `MaxTurrets` (RocketRiot, Blitzkrieg); elsewhere the task terminates at placement and the turret is ordinary (04 §5.11; `code/taskmanager.cpp:509-527`, `:785-799`).
3. **06** §2.3 (R4 row 113): for "how Darwinians then enter the dish", link [02 §4.8](../../02_units_and_entities.md#48-darwinian--movement-routes-and-orders) (`Darwinian::GiveOrders`: a dish/bridge within 5 of the order point → order = its entrance) and 02 §2.9 "Teleports" (`Entity::EnterTeleports`: entrance within 15, facing test).
4. **21** §4.2 "Dying" (R6 row 114): the truncating fade formula is now in 02 §2.8; 21 can adopt or link it.
5. **41**: mark 04-U2 and 04-U6 as resolved (rows ≈ 252 and 256); 02-U12 is resolved for `RunNuke` on the shipped maps (SpaceShip loops still [U]); 02-U1 now carries a new-game decision (speed stat 244).
6. **07**: optional link to the new AI parameter table in 02 §24 (main-timer periods 200 / 101 / 11 ticks, Assault 30; radar 101; the Blitzkrieg `minDistance = 20` that is computed but never used in `AI::Advance`).
7. **22**: no action needed from 02/04's side; 02 now states that NEXTWEAPON/PREVWEAPON have no live sender (consistent with V2 and R6 row 111).

=== F3.md

1. **01 §Conventions (L41):** "≈ 57 % of periods are 11 ticks" for `GetNetworkTime() > t + 1.0` timers is horizon-dependent. Exact replay (speed 1.0, time from 0): period is 10 ticks for re-arm times in [0.4, 3.0), [15.5, 63.2), [255.4, 1023.0), ≥ 4095.5 s, 11 otherwise (`+ 2.0`: 20 ticks in [0, 2.2), [14.7, 62.4), [254.6, 1022.2), ≥ 4094.7 s). See 03 §1.2. Tag can become [C arithmetic]; 0
2. **41:** mark 03-U1, 03-U2, 03-U15 (with 01-U8) and 07-U6 resolved (03 Open questions, 07 §7.2). 03-U4/U5/U6 are now also closed in 03, matching 41.
3. **26 L1020:** `Armour::ConvertToGunTurret` is tagged [B:FULL]; it is unreachable in Multiwinia → [B:LEGACY] (`code/location_input.cpp:1048-1053`; `code/location.cpp:3915-3965`).
4. **04 (GunTurret / crate turrets) and 02/24 if they describe turret line of sight:** `GunTurret::ClearLOS` samples from the target away from the turret (`code/worldobject/gunturret.cpp:251-276`); see 03 §5.1 and §15 #34.
5. **22 §14.5/§14.9 (RNG call-site census, order-dependent decisions):** a manned turret barrel draws 7 synced values per shot (03 §5.1); the stale `m_mostRecentEntity` path (07 §7.4) is another order-dependent AI behaviour.

=== F4.md

1. **26 §3 (L142)**: the animated-panel table says `GameTypeTankBattle` "(compiled out, [B:DEBUG])"; per V10 it is compiled in but not permitted in FULL (`code/app.cpp:571-573`, `code/lib/universal_include.h:94,140`). Suggest "(not permitted in FULL, [B:DEBUG])".
2. **41 L293 (06-U12)**: already marked Resolved there; 06 now closes it too — no change needed unless 41 mirrors 06's [U] tag text.
3. **02** (optional): §3/§4 could note that a Darwinian order targeted at a dish **centre** is re-targeted to the entrance by `Darwinian::GiveOrders` (`code/worldobject/darwinian.cpp:2579-2601`) — 06 §2.3 now links to 02 §4.8 / §2.9 for this.

=== F5.md

1. **41_risks_and_open_questions**:
   - mark **30-U6 Resolved [C]**: `GlobalWorldGameLoop` sets `m_atMainMenu = true` at `code/main.cpp:708` and breaks at L711.
   - mark **27-U2 Resolved [C]** (music is unreachable by code; see 27 §8).
   - mark **27-U1 / 10-U7** as "fidelity only, not blocking": the normative laws are now in 10 §8.2–§8.3.
   - at L168 ("Is music ever heard"), replace "triggers are Darwinia scripts or `USE_LOADERS` code" with "Darwinia scripts, the credits loader and `StartSequence` ([B:LEGACY]), or the seven `USE_LOADERS` loaders ([B:DEAD])".
2. **07_ai §17**: the "Formation officers" row describes `AI::AdvanceOfficerAI`, which promotes defensive officers (≥ 70 friends, no non-goto officer within 300 wu). Nothing in `ai.cpp` sets a formation (`code/worldobject/ai.cpp:1138-1203`; no `SetFormation` call), so suggest the label "defensive officers (`AdvanceOfficerAI`)". Low.
3. **20_architecture**: R5 §5 A flagged `m_startSequence` [B:DEAD]. The current 20 (L594) already says [B:LEGACY], so nothing is needed unless another row disagrees.
4. **29_data_formats_reference**: consistent with 30 now (37 of 43 opcodes over 43 files including `data/purity/buyme.txt`). No change.

=== F6.md

1. **40_carry_forward_assessment** F29 (line ≈560) lists "a hand-edited Difficulty preference reaches the sim (L13)" and "`Armour::SetDirectOrders` static debounce (L20)" as live desync paths. Per V1/V2 and 22 §14.11, L13 is resolved (not a desync), and only the `s_numMultiwiniaZones` half of L20 is live. Keep L1, L20 (zone counter) and L21.
2. **40_carry_forward_assessment** R17 (line ≈116), K25 (≈524), the tick-rate table (≈472) and the source-map row for `server.cpp` (≈758) recommend "latest-only control coalescing" without qualification. Per 22 §6.1 and §15 N11/N15, latest-only replacement drops one-shot events (clicks, fire, move) when two ALIVEs land in one tick. Coalesce only continuous state, and merge one-shot event
3. **41_risks_and_open_questions** R02 (line ≈120) cites L13 as a live leak: it is resolved (always 0 in Multiwinia). R04 (≈122): mark the `Armour::SetDirectOrders` debounce as latent (no live sender).
4. **06_player_control_and_input** §1.2 ALIVE row (line ≈152) says "server keeps only the latest per client per tick". Consider adding the consequence: an earlier ALIVE's one-shot flags are lost, and a tick with no ALIVE has no `UpdateTeam` (22 §6.1).
5. **31_platform_build_and_tools** (R8): the Grabber reachability is [B:DEAD] (V10); 20 now says so.

=== F8.md

1. **09 `09_meta_systems.md`**
   - L2127: align the first-run `default_preferences.txt` tag with 31 §8.9: [C] that a pure-`.dat` install never applies it (`tools/init.list`, `code/app.cpp:App::App`); a loose copy shipped by any SKU is [U] (31-U8 / 29-U2).
   - L2344–2345: `AVIMotionBlurFactor`/`AVIOptions` consumers should be [B:DEAD], because `AVIGenerator` is never created (`DebugKeyBindings::GrabberButton` has no caller).
   - R4 also asks 08/09 to state that the thumbnail fallback is unreachable (25 and 29 now say so).
2. **26 `26_animation_and_vfx.md` L142:** "GameTypeTankBattle (compiled out, [B:DEBUG])" should read "compiled in but not permitted (`INCLUDEGAMEMODE_TANKBATTLE` undefined), [B:DEBUG] availability" (V10; `code/app.cpp:571–573`).
3. **41 `41_risks_and_open_questions.md` L561:** 31-U11 is now **closed** — Grabber [B:DEAD]; the profiler is reachable only through the DEBUG cheat window.
4. **90 `90_glossary.md` L144–145, L349:** the AVI grabber / `AVIGenerator` should be [B:DEAD] (DEBUG-only code that nothing opens), not [B:DEBUG].
5. **23 `23_terrain_and_landscape.md` (R7 G5):** cross-link `redshirt2` to [29 §3.3](29_data_formats_reference.md#33-redshirt2-obfuscated-text), which holds the 16-entry offset table and the algorithm.
6. **08 `08_levels_and_level_design.md` (R4 row L139):** no request — 08 §2.2 L95, L374 and L2334 already state that a missing thumbnail draws nothing and that the `default.bmp` fallback is unreachable, which matches 25 and 29 now.
7. **24 `24_rendering.md` L1808 (optional):** note that `globalworld_*.shp` are loaded at every start-up (FULL, load-only), although they are never drawn in a match.

=== FIX_BRIEF.md
=== F7.md

1. **08 (owner of 08_levels_and_level_design.md)**, R7 X2: §6.1 (≈ L806) says "tiles are merged in file order (later tiles win where they overlap)". The code takes the per-sample **maximum**, so tile order matters only through flatten areas (`code/landscape.cpp:501-505`; 23 §7.5).
2. **22 (determinism)**, R7 X4: §14.8.9 T6 (≈ L1539) lists `Landscape::SphereHit` as a simulation consumer. It has no caller anywhere (`code/landscape.cpp:1063`; 23 §14).
3. **22**, R7 X5: ≈ L1346 says the `LevelFile` landscape `m_maxHeight` keeps full double precision. It is a `float` field assigned from `atof` (`code/level_file.h:151`, `code/level_file.cpp:787`; 23 §4.2). It is also used only by the colour ramp.
4. **07 (AI)**, R3 §4: §8 can now link `Location::IsVisible` to [23 §14](../../23_terrain_and_landscape.md#14-landscape-queries-used-by-gameplay), which has a new row for it.
5. **04 (F2)**: 04-U1's figure is **unchanged** after the V12 port fix. `C_nuke_sea_check.py SRC 25` still gives a minimum sea fraction of 0.2623 (`mp_assault_3p_3`), so no edit is needed. Optionally, 04-U1 could cite 23 §9.4 in place of `_work/drafts/C_nuke_sea_check.csv` to satisfy rule 5.
6. **Coordinator**, R7 G4: decide whether `design/_work/scripts/P11_terrain_ref.py` and `S_23_terrain_oracle.py` ship as a named deliverable. Doc 23 now inlines enough vectors (§8, §16.3, §16.5, §18) to test an implementation without them, but `dump`/`trace` remain the debugging tools.

