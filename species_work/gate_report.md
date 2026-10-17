# Gate report

| Gate | Result | Failures | Summary |
|---|---|---|---|
| 1 Hard-fail patterns | PASS | 0 | 0 hard-fail hits |
| 2 Soft-flag patterns | PASS | 0 | 37 soft hits, 37 allowed by CONVENTIONS §9, 0 not allowed |
| 3 Links and anchors | FAIL | 1 | 7064 relative links checked |
| 4 Mermaid | PASS | 0 | blocks=266 failed=0 |
| 5 Requirement ids | FAIL | 2 | 4918 requirement ids defined, 20859 citations, 25 prefixes registered |
| 6 DEC and ADR references | PASS | 0 | 246 DECs (113 Open, 113 of them cited by a requirement), 63 ADRs |
| 7 Asset closure | PASS | 0 | 401 manifest rows, 2273 asset citations in reference and component documents |
| 8 Tuning keys | FAIL | 17 | 3869 tuning keys, 9 cited by no requirement |
| 9 Truncation words | PASS | 0 | 0 truncation words |
| 10 Sizes, READMEs, templates | PASS | 0 | 177 documents, 76 checked for template headings |
| 11 Requirements index | PASS | 0 | REQUIREMENTS_INDEX.md and 25 index pages written (largest 99 KB) |

## Gate 3 Links and anchors

- reference/README.md:3 broken link: ../REQUIREMENTS_INDEX.md

## Gate 5 Requirement ids

- reference/07_bots/README.md:132 cites R-BLDG-510, which is not defined
- reference/10_audio_design/event_model_and_sound_data.md:123 cites R-META-120, which is not defined

## Gate 8 Tuning keys

- tuning key ref.unit.trooper.health defined 2 times: reference/02_units/README.md:462, reference/02_units/strike_team.md:192
- tuning key ref.unit.trooper.speed defined 2 times: reference/02_units/README.md:463, reference/02_units/strike_team.md:193
- tuning key ref.unit.transport.health defined 2 times: reference/02_units/README.md:466, reference/02_units/transport.md:248
- tuning key ref.unit.collector.health defined 2 times: reference/02_units/README.md:469, reference/02_units/collector.md:180
- tuning key ref.unit.collector.speed defined 2 times: reference/02_units/README.md:470, reference/02_units/collector.md:181
- tuning key ref.unit.builder.health defined 2 times: reference/02_units/README.md:472, reference/02_units/builder.md:136
- tuning key ref.unit.builder.speed defined 2 times: reference/02_units/README.md:473, reference/02_units/builder.md:137
- tuning key ref.unit.trooper.slope_divisor defined 2 times: reference/02_units/README.md:531, reference/02_units/strike_team.md:196
- tuning key cam.focus.radius_wave (architecture/components/camera_and_sequences.md:601) is cited by no requirement
- tuning key cam.focus.lift_factor (architecture/components/camera_and_sequences.md:605) is cited by no requirement
- tuning key rnd.crowd.worker_fury_pulse (architecture/components/renderer/entity_and_overlay_renderers.md:501) is cited by no requirement
- tuning key ter.generator.noise.envelope_gain (architecture/components/terrain.md:794) is cited by no requirement
- tuning key ref.bldg.carryable.base_speed_wu_s (reference/03_buildings_and_world_objects/mode_objects.md:451) is cited by no requirement
- tuning key ref.bldg.tree.regrow_leaf_threshold (reference/03_buildings_and_world_objects/trees_and_crate_structures.md:455) is cited by no requirement
- tuning key ref.ctrl.camera.mouse_look_rate (reference/06_controls_camera_and_commands/camera.md:212) is cited by no requirement
- tuning key ref.bot.marker.phase_ticks (reference/07_bots/05_mode_strategies_and_tutorials.md:205) is cited by no requirement
- tuning key ref.bot.marker.scan_range_default (reference/07_bots/05_mode_strategies_and_tutorials.md:206) is cited by no requirement
