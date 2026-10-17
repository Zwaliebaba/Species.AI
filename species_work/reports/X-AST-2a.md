# X-AST-2a report: asset pass 2, part a (Phase 4)

Status: done. Date 2026-10-17. I wrote only my own files. I ran no git write command and used no publishing or upload tool, so nothing was published. Consumers are still document paths, as this pass requires; part b replaces them with requirement ids.

## 1 Files written

| File | Size (bytes) | Change |
|---|---:|---|
| `design_next/assets/asset_manifest.csv` | 972,826 | 1,412 rows (was 1,311) and 453 asset ids (was 401). Every consumer path now exists. Fixes are in §3. |
| `design_next/assets/GameAssets.md` | 199,596 | Was 149,179. It is the owner's single document, so it stays exempt from the 120 KB gate (coordinator decision at Phase 3; `check_design.py` `size_exempt`). It grew by the new rows, the new §11.1 (collision assets) and the new §18.1 (string ids). |
| `design_next/assets/README.md` | 8,297 | Counts, constraints and the placeholder rule. |
| `design_next/assets/art_and_audio_briefs.md` | 23,819 | Style guide and category briefs aligned with ADR-017, ADR-018 and ADR-020. |
| `species_work/asset_origin_map.csv` | 238,050 | 2,227 rows. 30 unbound reference takes are now mapped to new groups, and 71 rows have no reference file. |
| `species_work/p4_new_assets.csv` | 22,197 | 174 rows over 53 ids: the 52 new ids, plus `fx.glow_round` because it is now reused for a new purpose. |

Helper scripts are in `species_work/scripts/`. Each runs with `python3 -I`, with paths as arguments:
- `X-AST-2a_extract.py`: report sections;
- `X-AST-2a_req.py`: requirement lookup;
- `X-AST-2a_strings.py`: string-id census and the markdown of §18.1;
- `X-AST-2a_apply.py`: manifest, origin map and `p4_new_assets.csv`;
- `X-AST-2a_docs.py`: the GameAssets, README and briefs edits, each checked by how often its old text occurs;
- `X-AST-2a_patch.py`: routes RA-263, RA-285 and RB-326;
- `X-AST-2a_roles.py`: roles that the documents now settle.

The pass-1 checker `X-AST_check.py` exits 0. It finds no stem collisions and no near matches, and 1,460 in-scope originals, each mapped to exactly one Species file.

Inputs from the reference were only the X-AST package CSVs: `X2b_samples.csv`, `X2b_sound_events.csv` and `X2a_textures.csv`. I used them to size the new sound groups from the reference's unbound takes, and to confirm that no reference texture exists for the route arrow, the empty compass or the beam ring. Original names appear only in the origin map.

## 2 Row-count reconciliation (task 4)

- **Pass 1: 1,311 manifest rows against 1,327 origin-map rows.**
  - The 16 extra rows are text-category rows. The origin map maps each of the 16 reference text categories (221 entries) onto `content/strings/en_gb.json`, so that one file has 17 rows.
  - 1,311 + 16 = 1,327. No file is missing and none is duplicated.
- **Pass 2a: 1,412 manifest rows** (+101). The new rows are:
  - 32 sound-effect files and 2 UI-sound files;
  - 6 application-icon files;
  - 6 glyphs;
  - the route arrow, the empty compass and the Abductor beam ring (one each);
  - 10 derived variants;
  - 8 tint masks;
  - 1 editor mesh;
  - 32 collision files;
  - 1 development string table.
- **Origin map now:** 1,428 rows name a Species file. That is 1,412 distinct files + the same 16 text-category rows. The other 799 rows are excluded originals.
- **Excluded reference files:** 817 → 787. The 30 unbound takes moved out of "unused (bound to no sound event)", which goes from 192 to 162. The in-scope originals go from 1,430 to 1,460.
- **Reference text entries** stay 281 not created and 221 in scope.
- GameAssets §3 and §20 and the README now show these totals. They are computed by the script from the manifest and the origin map.

## 3 Item outcomes (phase4_dec_fixes.md)

| Item | Outcome |
|---|---|
| 23 | Added `sfx.ui.match_ui.game_over`: 2 members, sized from the reference's two unbound game-over takes, announcer bus (R-AUD-307, DEC-097). Victory and defeat play the existing `music.match.sting_victory` and `music.match.sting_defeat` (G10). No victory or defeat UI rows. |
| 27 | **Builder placement sound:** this is `sfx.ui.power_input.confirm` (G10), so no new row; `reference/02_units/builder.md` is added as its consumer. **Unit toggle:** `match_ui.toggle_state` plays `sfx.ui.match_ui.unit_select`; `transport.md` is added as its consumer. **`icon.power.rocket`:** `strike_team.md` is removed as a consumer; reference/04 still cites the icon. |
| 32, 36, 79, 90 | Every stale consumer path is replaced by the files of the new folder or split that cite the asset id: 1,232 occurrences on 1,232 rows, and every id had at least one citing file. The stale paths were `reference/10_audio_design.md` (904 rows), `creatures_and_hazards.md` (258), `animation_and_vfx.md` (46) and `reference/06_controls_camera_and_commands.md` (24). |
| 35 | Added `fx.abductor_beam_ring`: an alpha mask in the Visitor colour, drawn with `actor_beam_alpha` (R-UNIT-710). **Eaten-orb card:** no new row. VFX-122 draws it as diamond billboards and names no texture, so it reuses `fx.glow_round`. `creatures.md` and the VFX explainers file are added as consumers, and `p4_new_assets.csv` asks the fixers to cite it. |
| 41 | `fx.beam_short`: the sky grid samples it with linear clamp-to-edge through all mips (RND-151), and the laser bolts read its `level0` view (RND-339). `mesh.structure.structure_default` is now the port pad (R-BLDG-017, RND-168), also the missing-mesh fallback (RND-163). Its priority moves from post-launch to first-playable, because every port draws it. |
| 44 | **Refuelling loop:** confirmed silent (R-AUD-141 table, "silent"), so no row. **Port pad role:** as item 41. **Fuel-chain consumers:** `mode_objects.md` is replaced by `economy_and_spawn_objects.md` on the 22 rows where `mode_objects.md` does not cite the id. |
| 58 | `match_ui.order_given` and `power_slots.select` are silent under DEC-565 (R-AUD-302), so no rows. |
| 66 | Added `marker.route_arrow` (RND-353) and `marker.edge_compass_empty` (RND-357). Added the derived Crawler trail `sprite.crawler.segment.trail` (RND-320: 32×512, 18 copies). |
| 69 | **`uitex.logo_banner`:** no rule draws it and no document cites it. It is kept, but moved to post-launch with that note in its role; part b may drop it. **Explainer picture lists:** see route RA-263. |
| 73 | The 16 objective-card `help.hint.*` rows are now post-launch hook art (DEC-050, R-TUT-013). This is the X-AST option of the item. If the coordinator re-routes the objective cards as KEEP, the priority can return to launch. |
| 82 | Added `sfx.weapon.laser_bolt.spawn` (10), `.hit_structure` (10) and `.kill` (8). Each is sized from the reference's unbound charging, structure-hit and actor-hit takes (DEC-564, R-AUD-142). Added `sfx.unit.trooper.throw_grenade` (4) and the game-over group (item 23). The reference has no take for the grenade throw, so its 4 variants are a proposal. |
| 83 | ADR-020 is applied in the manifest, GameAssets §2.4, §8 and §11, the README and briefs §2.5, §7 and §10. Mesh team regions now carry explicit tint weights (`_SPECIES_TINT` or `species_team_tint`), and an authoring helper is available for key-colour art. Cloth carries the team area as grey levels plus companion tint masks `marker.leader_banner_tint` and `marker.flag_post_state_tint` (4 each, AST-022). Nothing is recoloured at load. The look is unchanged. |
| 85 | The five ADR-017 rows now have authored sizes in whole 4×4 blocks (13 files). Sizes are rounded up to the next multiple of 4 and no document cites the old sizes: <br>• `help.explainer_picture`: 9 members, for example 181×256 → 184×256 and 512×310 → 512×312; <br>• `help.general_page` member 03: 300×300; <br>• `uitex.logo_banner`: 788×300; <br>• `uitex.logo_emblem`: 556×388; <br>• `uitex.title_wordmark`: 1320×244. <br>§2.4 now says the cooker never pads, and the "reduced by the cooker" wording is replaced by AST-024. A script check confirms that every BC row is a multiple of 4. |
| 86 | **Derived shadows:** 9 rows `<parent>.shadow`, for the 7 shadowed cursors of R-CTRL-350 table 2.1-A (all except the turret-aim crosshair), `marker.crate_landing` and `marker.barrage_target` (AST-029). **Crawler trail:** see item 66. **Tint masks:** see item 83. **Collision assets:** 23 ids and 32 files, `mesh.collision.<name>` (AST-044). They cover every structure mesh that table 2.12-A or the default tests read: <br>• Spawn Node and its alternative body, Source Node, Relay, Transit Gate and its alternative body; <br>• both Command Consoles and the Core Device; <br>• fuel pump housing, fuel line foot and fuel depot; <br>• Solar Array, grid pylon and Beam Dish; <br>• Barrier Emitter, turret head and the 4 turret barrels, wall; <br>• Hive and Spitter Plant; <br>• the 7 scenery pieces, and the Abductor hull where a map places it as scenery. <br>The proposed `collision.` stem is not a registered stem, so I used the `mesh.` stem with the group `collision`. The 15 cross-document requests of C-AST belong to the routers. |
| 97 | Added `glyph.pad.stick_left`, `glyph.pad.stick_right` and `glyph.pad.dpad_horizontal`, each with its own shadow, as C-INP asked: 6 rows, consumer `input.md`. |
| 104 | Remainder of the coordinator's rename: the §2.3 id-root table now says `uitex.` and `mapfile.`. No `ui.` or `map.` asset id remains. |
| 109 | `ui.repair.passed` and `ui.repair.failed` are listed in GameAssets §18.1. Both are already cited by BLD and PLT. |
| 114 | `ui.match.cheats_used` is listed in §18.1; DBG cites it. Added `strings.dev_en_gb`, the development string table for the `ui.dev.*` captions: development editions only, never a player edition. |
| 117 | Added `uitex.app_icon`, 6 members (16, 24, 32, 48, 64 and 256 px). It becomes a Windows icon resource in the executable (PLT-057) and is in no pack. The five `ui.error.*` ids are listed in §18.1; all are cited by PLT. |
| 120 | GameAssets §2.1 now has `content/records/<family>/` for `modes`, `input`, `actors`, `crates`, `audio`, `vfx`, `timelines`, `ui`, `editions` and `test` (DAT-006 table 1-A). It also notes that `sound.groups` holds the member weights (DAT-275) and that explainers are found by their `mode` field (DAT-284). |

## 4 Report requests not covered by an item

| Source | Outcome |
|---|---|
| C-AST 14 (route RB-115) | All parts are applied except one (part i is information only): <br>• (a) the padding rows: item 85; <br>• (b) cloth `target_format` and `alpha_mode` name the tint mask; <br>• (c) all 71 render-mesh rows now say "colours attenuated at cook" and use explicit tint weights; <br>• (d) the `cursor.world` shadow is a derived variant; <br>• (e) `mesh.creature.spider_body` sockets are now `leg_01` to `leg_08`; <br>• (f) `sprite.visitor.front` is a cut-out at 0.04, because RND-316 draws it as a `worker_card` (RND-060); <br>• (g) GameAssets §2.4 wording; <br>• (h) AST-085 is added to the placeholder rule in GameAssets §2.6 and the README. <br>The README also loses "logged, never fatal": a manifest gap is a build error (AST-049); a missing file at run time is a diagnostic. |
| ADR-018 (X-ADR-P4 items 39, 84) | `fx.particle_dot` and `sprite.worker.front` name the point-magnify clamp-to-edge sampler. `fx.sky_cloud` follows RND-153 and RND-154. GameAssets §2.4 lists the closed sampler set and the straight-alpha with additive pairing. Briefs §2.7 follow RND-432; I dropped "full-screen pixel passes", because DEC-059 leaves that effect unspecified (C-RND-b). |
| C-RND-b (manifest corrections) | `cursor.place` is also the entering marker (R-CTRL-371). |
| C-UI-b | `cursor.pointer` is the image of the scroll-bar arrows and the selection arrows (UI-465). The world cursor draws the pointer. |
| C-VFX | `fx.shard_edges` is an art reference, not sampled at run time (VFX-229, ADR-010). `help.explainer_picture` now has full mips (R-META-231). |
| C-TER | `terrain.ramp`: orientation (TER-040) and the 64×64 rejection (TER-250). `terrain.shore_wave_gradient`: row 1 counted from the bottom (TER-041). Both are in the manifest, GameAssets §10 and briefs §9. |
| G11 | `help.hint.camera_controls` is 304×128 display (608×256 authored), matching R-TUT-021. The 16 objective-card rows: see item 73. |
| G08b | Added `mesh.dev.camera_view` (post-launch, editor only) and the mesh group `dev`. |
| G02a1 | `marker.leader_banner` is kept with 4 members. Its role notes that no in-scope rule draws `gather`. |
| G03a | **Alternative bodies:** `spawn_node_alt` and `transit_gate_alt` have no in-scope consumer and stay post-launch, together with their collision assets; part b may drop them. **Energy-surge star and grid-line beam:** left to the renderer, which has not named them. |
| G03b, G04 | **Silent events:** `turret.fire_grenade` and `tree.damaged` are silent (R-AUD-141). **Kills:** a turret-round kill is covered by `sfx.weapon.laser_bolt.kill`. **Crate expiry:** a sound waits on DEC-565, so no row. |
| C-AEN (RB-150) | GameAssets §2.4, §13 and §15, the README and briefs §14 state streaming per group: a group streams when its longest member is over 20 s, and every music group streams (AEN-072). The resident groups hold about 173 MiB (AEN-304). The manifest was already consistent per group, which a script checked. |
| G09a, G09b, C-RND-a, C-RND-b, C-UI-a, C-UI-b, G02b, G03a (add my file as consumer) | Every reference and component document that cites an asset id in backticks is now a consumer of that row. 535 rows gained a consumer this way. Two-segment wildcards such as `icon.power.*` count; stem-wide wildcards such as `sfx.*` do not, as the README says. |
| Pass-1 open roles | **Settled from the documents:** <br>• `sprite.worker.state_overlay`: drawn while a Worker has a Convert link (R-UNIT-147); <br>• `mesh.structure.turret_barrel`: member per turret type (gun barrel for gun and mortar, rocket, flame, cannon for the Convert Turret); <br>• `mesh.structure.bot_nav_node`: map editor only; <br>• font roles (UI-195): `font.ui.primary` is the title face, `font.ui.stylised` the caption face and `font.ui.compact` the body face. <br>**Still open:** `icon.power.laser` and `icon.power.slots_upgrade`. reference/04 lists them without a rule. |

## 5 Routes addressed to X-AST (p4_routes)

| Route | Outcome |
|---|---|
| RA-095, RA-284, RB-119 | Done; see items 32, 36, 79 and 90. The audio rows now name the folder files that cite each group (`event_catalogue.md` for every bound group). |
| RA-254 | Done (G08b). |
| RA-263 | **(a)** The "used by" notes of all 25 explainer pictures now follow R-META-225; 5 notes changed. **(b)** `front_end_presentation.md` is now a consumer of the 11 ids. **(c)** Its string ids are in the §18.1 census. |
| RA-282, RA-303 | **Consumers:** done. **String ids:** reference/09 and reference/11 are in the §18.1 census (`ui.options.*` 42, `ui.language.*` 9, `tutorial.tab.*` 60, `ui.help.*` 51 and the rest). **Aspect and the 16 hint rows:** done. |
| RA-285 | Renamed to the event ids of reference/10 in manifest roles and GameAssets "Also bound to": `collector.spawn`, `meteor.spawn (layer 2)`, `abductor.spawn (layer 4)` and `core_device.spawn`. Asset ids are unchanged. Members 07 to 13 of `sfx.unit.worker.burning` are now marked as silent samples by design, with placeholder silence (R-AUD-066; the route said R-AUD-065). The five new groups are in GameAssets §13 and §14. |
| RB-031, RB-167, RB-249, RB-259, RB-326 | Done in §18.1. The family table lists 579 cited ids or id patterns in 63 families. A second table lists the ids that the reports named, with their main owner. GameAssets §18 now names the ledger prefixes `team.colour.*` and `faction.*` instead of `match.team.*`. Two ids are still cited by no document: `ui.dialog.sound_settings_failed` (C-AEN to cite it) and `match.kill_milestone` (a proposal for the match-rules owner). |
| RB-133, RB-150, RB-185, RB-199, RB-218 | Done; see §4 and items 97 and 120. |

## 6 For the document fixers and the coordinator

1. **Citations of the new ids.** `p4_new_assets.csv` lists every new id with its consumer files, and the fixers cite them. These sentences say the asset is missing and become stale now:
   - R-AUD-142 and R-AUD-413 ("not yet in the asset manifest");
   - INP-081 ("glyph assets that the asset manifest does not hold yet"); INP-222 can now admit the stick and d-pad overrides;
   - G08b's note that `mesh.dev.camera_view` is not in the manifest;
   - DAT-006 ("folders … not yet listed in GameAssets §2.1 are proposed"), because GameAssets §2.1 now lists them.
2. **Component fixes that match my rows (X-ADR-P4 F3, F5 and F6):**
   - AST-027 should name the point-magnify sampler;
   - RND-162 and RND-406 still describe red key colours and refer to briefs §2.5, which now describes tint weights and masks;
   - RND-393 should give the colour space of the single-channel families as linear.
   - VFX-122 should name `fx.glow_round` as the eaten-orb card texture.
3. **New ids, no new stems.** The new ids use only registered stems. The collision assets are `mesh.collision.*`, and the derived variants are `<parent id>.<variant id>` as AST-028 requires. CONVENTIONS §8 needs no change for them.
4. **Proposals the owner may revisit:**
   - the 4 variants of the grenade throw;
   - the mapping of the reference's unbound takes onto the three laser groups (charging → fire, structure hits → structure hit, actor hits → kill). The ground-hit and ricochet takes have no event;
   - first-playable priority for the application icon, the route arrow and the development string table.
5. **Item 73** still needs the coordinator's choice between re-routing the objective cards as KEEP and leaving them as post-launch hook art, which this pass applied.

## 7 Checks

- **Pattern scans** on `design_next/assets/`: `scan_patterns.py forbidden_patterns.tsv` prints nothing, and `scan_patterns.py soft_patterns.tsv` prints nothing.
- **Gates.** I ran `check_design.py --skip-mermaid` with its report in my scratchpad. Gates 1, 2, 3, 7, 9 and 10 pass:
  - gate 7: 453 asset ids, 1,412 rows and 2,273 citations, all resolved; every consumer path exists;
  - gate 3: links pass, including the 73 new links in §18.1.
  Gates 5, 6 and 8 fail, but none of their failures names an `assets/` file; they belong to other packages. Gate 4 was skipped, and my files contain no Mermaid.
- **Size.** GameAssets.md is 199.6 KB. It is exempt as the owner's single document. If that exemption is withdrawn, §11.1, §18.1 and the audio tables are the natural split points.
