# Adoption ledger: 06 controls camera and commands

> Status: Draft · Owner: area C1 · Reference: [06_controls_camera_and_commands](../../reference/06_controls_camera_and_commands/README.md) · Rules: `prompts/species-design-phase.md` §5.3

44 sections, 173 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `06_controls_camera_and_commands/README.md §2.1` | pending | | General: 6 requirements (R-CTRL-001 to R-CTRL-006) |
| `06_controls_camera_and_commands/README.md §2.2` | pending | | Input conventions and global gates: 5 requirements (R-CTRL-010 to R-CTRL-014) |
| `06_controls_camera_and_commands/README.md §2.3` | pending | | Input pipeline and authority: 6 requirements (R-CTRL-020 to R-CTRL-025) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.1` | pending | | Action ids and default bindings: 6 requirements (R-CTRL-030 to R-CTRL-035) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.2` | pending | | Player actions: 9 requirements (R-CTRL-040 to R-CTRL-048) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.3` | pending | | The secondary-button resolver: 2 requirements (R-CTRL-050 to R-CTRL-051) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.4` | pending | | The object under the pointer: 5 requirements (R-CTRL-055 to R-CTRL-059) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.5` | pending | | Selecting with the pointer: 2 requirements (R-CTRL-062 to R-CTRL-063) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.6` | pending | | Leaders as signposts and formations: 6 requirements (R-CTRL-070 to R-CTRL-075) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.7` | pending | | Transport orders: 4 requirements (R-CTRL-080 to R-CTRL-083) |
| `06_controls_camera_and_commands/actions_and_bindings.md §2.8` | pending | | Direct control and the team command state: 10 requirements (R-CTRL-100 to R-CTRL-109) |
| `06_controls_camera_and_commands/camera.md §2.1` | pending | | Model and conventions: 5 requirements (R-CTRL-450 to R-CTRL-454) |
| `06_controls_camera_and_commands/camera.md §2.2` | pending | | The per-frame update: 2 requirements (R-CTRL-456 to R-CTRL-457) |
| `06_controls_camera_and_commands/camera.md §2.3` | pending | | Common components: 4 requirements (R-CTRL-460 to R-CTRL-464) |
| `06_controls_camera_and_commands/camera.md §2.4` | pending | | Free movement: 4 requirements (R-CTRL-470 to R-CTRL-473) |
| `06_controls_camera_and_commands/camera.md §2.5` | pending | | Tracking: 9 requirements (R-CTRL-480 to R-CTRL-488) |
| `06_controls_camera_and_commands/camera.md §2.6` | pending | | Structure aim: 3 requirements (R-CTRL-490 to R-CTRL-492) |
| `06_controls_camera_and_commands/camera.md §2.7` | pending | | Start-of-match camera views: 3 requirements (R-CTRL-495 to R-CTRL-497) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.1` | pending | | The cursor set: 3 requirements (R-CTRL-350 to R-CTRL-352) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.2` | pending | | Frame flow: 3 requirements (R-CTRL-355 to R-CTRL-357) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.3` | pending | | Highlight resolution: 2 requirements (R-CTRL-360 to R-CTRL-361) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.4` | pending | | The highlight effect: 1 requirement (R-CTRL-363) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.5` | pending | | The icon near the pointer: 1 requirement (R-CTRL-366) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.6` | pending | | The selection circle: 1 requirement (R-CTRL-370) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.7` | pending | | The entering effect: 1 requirement (R-CTRL-371) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.8` | pending | | The crosshair: 1 requirement (R-CTRL-372) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.9` | pending | | The formation preview: 1 requirement (R-CTRL-374) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.10` | pending | | Selection arrows and move markers: 1 requirement (R-CTRL-375) |
| `06_controls_camera_and_commands/cursor_and_markers.md §2.11` | pending | | World markers: 9 requirements (R-CTRL-380 to R-CTRL-388) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.1` | pending | | Selection state: 3 requirements (R-CTRL-150 to R-CTRL-152) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.2` | pending | | Selectable objects: 2 requirements (R-CTRL-153 to R-CTRL-154) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.3` | pending | | The selection circle and group selection: 4 requirements (R-CTRL-155 to R-CTRL-158) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.4` | pending | | Crowd orders: 2 requirements (R-CTRL-160 to R-CTRL-161) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.5` | pending | | Selection lifecycle: 1 requirement (R-CTRL-162) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.6` | pending | | The Power record: 5 requirements (R-CTRL-170 to R-CTRL-174) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.7` | pending | | Power kinds: 3 requirements (R-CTRL-175 to R-CTRL-177) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.8` | pending | | Power states: 2 requirements (R-CTRL-180 to R-CTRL-181) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.9` | pending | | Capacity and slot order: 6 requirements (R-CTRL-185 to R-CTRL-190) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.10` | pending | | Targeting: 4 requirements (R-CTRL-191 to R-CTRL-194) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.11` | pending | | Selection and cycling: 6 requirements (R-CTRL-200 to R-CTRL-205) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.12` | pending | | Cancellation and stopping: 2 requirements (R-CTRL-210 to R-CTRL-211) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.13` | pending | | The Power slots panel: 8 requirements (R-CTRL-220 to R-CTRL-227) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.14` | pending | | The power bar: 8 requirements (R-CTRL-230 to R-CTRL-237) |
| `06_controls_camera_and_commands/selection_and_power_slots.md §2.15` | pending | | The Power-received pop-up: 2 requirements (R-CTRL-238 to R-CTRL-239) |
