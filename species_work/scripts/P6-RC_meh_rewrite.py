"""Apply the P6-RC fixes to reference/08 map_editor_hooks.md (one owned file).

Usage: python3 -I P6-RC_meh_rewrite.py <map_editor_hooks.md> <expected sha256> <new section 4 file>

Every replacement must match exactly the number of times it states, or nothing is written.
"""
import hashlib
import re
import sys

INPUT_RANGES = {
  'terrain.outside_height_range': '−100 to 100 wu, step 1.0',
  'terrain.cell_size_range': '1 to 100 wu, step 1',
  'terrain.world_size_range': '1 to 1,000,000 wu, step 10',
  'terrain.max_height_range': '0 to 2,000 wu, step 1.0',
  'tile.fractal_dimension_range': '0 to 10, step 0.01',
  'tile.height_scale_range': '0 to 10, step 0.02',
  'tile.desired_height_range': '0 to 1,000 wu, step 5.0',
  'tile.generation_method_range': '0 to 2, step 1',
  'tile.lowland_smoothing_range': '0 to 3, step 0.02',
  'tile.position_xz_range': '−10,000 to 10,000 wu',
  'tile.position_y_range': '−1,000 to 1,000 wu, step 0.5',
  'tile.size_range': '0 to 10,000 wu, step 10',
  'tile.seed_range': '0 to 10,000,000,000, step 1 (§5 item 1)',
  'flatten_area.size_range': '0 to 1,000 wu, step 1',
  'flatten_area.height_range': '−1,000 to 1,000 wu, step 1',
  'flatten_area.threshold_range': '0 to 1,000 wu, step 1',
  'guide_grid.power_range': '0 to 5, step 1',
  'guide_grid.brush_size_range': '1 to 40 samples, step 1, initially 1',
  'guide_grid.write_multiplier_range': '0 to 1, step 0.01, initially 1.0',
  'light.front_y_range': '−20 to 20, step 0.01',
  'light.colour_range': '0 to 5, step 0.02',
  'field.barrier_emitter.scale': '0 to 100, step 0.01',
  'field.hive.ants_inside': '0 to 1,000 ants, step 1',
  'field.spitter_plant.size': '0 to 50, step 0.1',
  'field.spitter_plant.pitch': '−π to π rad, step 0.1',
  'field.spitter_plant.force': '0 to 1,000 wu/s, step 1',
  'field.spitter_plant.variance': '0 to π rad, step 0.01',
  'field.spitter_plant.reload': '0 to 10,000 ticks, step 10 ticks, edited in whole seconds (0 to 1,000 s in steps '
                                'of 1 s at `ref.sim.tick_period`)',
  'field.spitter_plant.trigger_offset': '−10,000 to 10,000 wu, step 1',
  'field.spitter_plant.trigger_radius': '0 to 1,000 wu, step 1',
  'field.population_limiter.search_radius': '0 to 10,000 wu, step 1',
  'field.population_limiter.max_population': '0 to 10,000, step 1',
  'field.scenery_mesh.scale': '0 to 100, step 0.1',
  'field.launch_vessel.fuel': '0 to 100, step 0.1',
  'field.launch_vessel.passengers': '0 to 100 Workers, step 1',
  'field.launch_vessel.linked_id': '0 to 9,999, step 1',
  'field.control_zone.size': '0 to 1,000 wu, step 1',
  'field.bot_nav_node.priority_modifier': '−1 to 1, step 0.05',
  'field.core_device.countdown': '300 to 1,800 mode seconds, step 60 (the play-time range × '
                                 '`ref.match.option.time_limit_unit`)',
  'field.exclusion_zone.size': '0 to 10,000 wu, step 1',
  'field.turret.mortar_target_offset': '−10,000 to 10,000 wu, step 1',
  'field.turret.mortar_radius_force': '0 to 1,000 wu (radius) and 0 to 1,000 (force), step 1',
  'field.jump_pad.angle': '−π to π rad, step 0.1',
  'field.jump_pad.force': '0 to 1,000 wu/s, step 1',
  'field.bot_objective_marker.scan_range': '1 to 500 wu, step 1',
  'field.bot_objective_marker.pickup_only': '−1 to 1, step 1',
  'starting_forces.count_range': '1 to 1,000, step 1',
  'starting_forces.spread_range': '0 to 1,000 wu, step 1',
  'options.player_count_range': '1 to 4, step 1 (§5 item 1)',
  'options.play_time_range': '5 to 30, step 1',
  'option.reinforcement_count_range': '0 to 300 Workers, step 10',
  'option.transport_cap_range': '0 to 5, step 1',
  'option.airstrike_threshold_range': '0 to 20, step 1',
  'option.visitor_position_range': '0 to 3, step 1',
}

# (old, new, expected count)
EDITS = [
  # §1
  ('lets an author change every capability of a map except routes, camera animations and primary objectives, and writes',
   'lets an author change every capability of a map except routes and camera animations, and writes', 1),
  ('every match uses a listed map of a content set.\n',
   'every match uses a listed map of a content set. Campaign content is not specified (DEC-052).\n', 1),
  ('the validation a map tool applies when it saves; and the defects and gaps of the reference editor.',
   'the validation a map tool applies when it saves; and the gaps of the reference editor.', 1),
  ('- **Input ranges.** "Range a to b, step s" is the reference editor\'s input range of a field and the change per press '
   'of the field\'s increment or decrement control. An input range is a property of the reference editor; the limits that '
   'the content build enforces are those of the field\'s owning document (§5 lists where the two differ).',
   '- **Input ranges.** "a to b, step s" is the reference editor\'s input range of a field and the change per press of '
   'the field\'s increment or decrement control. An input range is an informative property of the reference editor, not '
   'a tuning value; the limits that the content build enforces are those of the field\'s owning document (§5 lists where '
   'the two differ).', 1),
  ('the selection panels (the tile panel, the flatten-area panel, the structure panel and the record panel), the tree '
   'panel, the guide-grid painter and the two camera-animation panels: 16 panels in all.',
   'the selection panels (the tile panel, the flatten-area panel, the structure panel and the record panel), the tree '
   'panel and the guide-grid painter: 14 panels in all.', 1),
  # §2.1
  ('**R-MAP-300** (MUST) The values of the tuning table of this document are the reference tuning baseline (DEC-004), '
   'and this document',
   '**R-MAP-300** (MUST) The values of the tuning tables of this document are the reference tuning baseline (DEC-004); '
   'the input ranges of §2 and the layout and drawing values of table 4.5-A are informative properties of the reference '
   'editor. This document', 1),
  ('The editor\'s own panels are absent from player builds; the excluded-outcomes panel, the map-editor hub page and the '
   'map file picker are present but never opened. This is the reference default of DEC-045.',
   'No editor panel, map-editor hub page or map file picker can be opened in a player build. This is the reference '
   'default of DEC-045.', 1),
  ('except three: routes, for which it has no operation (§2.9); camera animations, whose authoring panels are never '
   'opened (§2.10, DEC-803); and primary objectives, which it neither edits nor writes (R-MAP-033).',
   'except two: routes, for which it has no operation (§2.9), and camera animations, for which it offers no operation '
   '(§2.10, DEC-803).', 1),
  # §2.2
  ('so the reference has no play test from the editor (table 2.16-A item 14).',
   'so the reference has no play test from the editor (table 2.16-A item 8).', 1),
  ('On its next update the hub page starts the editor when the chosen map is not the placeholder that names no map and a '
   'reader for that name exists in the content set\'s maps folder or in the player\'s map folder. The existence test '
   'succeeds for every name, so any name other than the placeholder starts the editor. Starting sets the requested map, '
   'clears the campaign mission (DEC-052), sets the editing state',
   'On its next update the hub page starts the editor for any chosen map other than the placeholder that names no map. '
   'Starting sets the requested map, sets the editing state', 1),
  ('The default map stores its file name in lower case as its name and names no campaign mission.',
   'The default map stores its file name in lower case as its name.', 1),
  ('and it has no undo (table 2.16-A item 14).', 'and it has no undo (table 2.16-A item 8).', 1),
  # §2.3
  ('| 6 | camera views | camera-view panel | add a camera view to a camera animation (unreachable, R-MAP-373) | §2.10 |',
   '| 6 | camera views | camera-view panel | none (R-MAP-373) | §2.10 |', 1),
  (' Deleting an entry shifts the indices of the later entries; the editor guards a selection only by closing the '
   'selection\'s panel. A second, finer selection, meant for the route points of a route, is declared and never used '
   '(§2.9).',
   ' Deleting an entry shifts the indices of the later entries; the editor guards a selection only by closing the '
   'selection\'s panel.', 1),
  (' and link, for structure records. A create tool is declared and never used.', ' and link, for structure records.', 1),
  ('the **crosshair point**, which the editor marks with a crosshair of half-length `ref.map.editor.crosshair_half_px` '
   '(30 px).',
   'the **crosshair point**, which the editor marks with a crosshair (table 4.5-A).', 1),
  ('**R-MAP-323** (MUST) The editor\'s panels have the reference sizes and placements of `ref.map.editor.panel.main`, '
   '`ref.map.editor.panel.mode`, `ref.map.editor.panel.selection`, `ref.map.editor.panel.tree`, '
   '`ref.map.editor.panel.guide_grid`, `ref.map.editor.panel.asset_picker`, `ref.map.editor.panel.colour_editor` and '
   '`ref.map.editor.panel.animation_nodes`.',
   '**R-MAP-323** (MUST) The editor\'s panels have the reference sizes and placements of table 4.5-A.', 1),
  # §2.4
  ('in the style of `ref.map.editor.guide_grid.canvas_style`.', 'in the style of table 4.5-A.', 1),
  ('| scale down, scale up | rescales the map (R-MAP-328) | `ref.map.editor.terrain.scale_factors` |',
   '| scale down, scale up | rescales the map (R-MAP-328) | — (the factors `ref.map.editor.terrain.scale_factors`) |', 1),
  # §2.6
  ('the no-team swatch in `ref.map.editor.position_selector.inactive_rgb`;',
   'the no-team swatch in the inactive grey of table 4.5-A;', 1),
  ('| campaign flag | marks the record as belonging to a campaign mission; the text writer writes only records whose '
   'campaign flag is clear (R-MAP-386), so setting it removes the structure from the saved map. Species maps hold no '
   'campaign record (DEC-052) |\n', '', 1),
  (' Every kind that the table does not list has only the common operations. The reference editor also holds per-kind '
   'operations for kinds that only campaign content uses; they can never be shown, because such kinds can be neither '
   'created nor loaded (DEC-052).',
   ' Every kind that the table does not list has only the common operations.', 1),
  ('(table 2.16-A item 8). A map tool draws its editor randomness', '(table 2.16-A item 7). A map tool draws its editor '
   'randomness', 1),
  # §2.9
  ('it cannot create, edit or delete a route or a route point, the finer selection meant for route points is never used '
   '(R-MAP-319), scaling moves no route (R-MAP-328),',
   'it cannot create, edit or delete a route or a route point, scaling moves no route (R-MAP-328),', 1),
  # §2.10
  ('| name (one per view) | edits the view\'s name in place, at most `ref.map.editor.camera_view.name_field_chars` (63) '
   'characters |',
   '| name (one per view) | edits the view\'s name in place, at most 63 characters (informative, §5 item 1) |', 1),
  # §2.11
  ('| basic | display name | a text field of at most `ref.map.editor.options.name_field_chars` (32) characters |',
   '| basic | display name | a text field of at most 32 characters (§5 item 1) |', 1),
  ('In the co-op and Siege sections a start position at or above the player count is inactive and is shown in '
   '`ref.map.editor.position_selector.inactive_rgb`.',
   'In the co-op and Siege sections a start position at or above the player count is inactive and is shown in the '
   'inactive grey of table 4.5-A.', 1),
  ('| Section | Field | Control and input range |', '| Section | Field | Control and input range (informative) |', 1),
  ('| # | Map option | Control and input range |', '| # | Map option | Control and input range (informative) |', 1),
  ('The reinforcement-power-option lists offer five entries: airstrike, firebomb run, flame turret, rocket turret and gun '
   'turret.',
   'The reinforcement-power-option lists offer five entries: airstrike, Firebomb Run, flame turret, rocket turret and gun '
   'turret.', 1),
  ('and so do firebomb run and rocket turret: the meaning', 'and so do Firebomb Run and rocket turret: the meaning', 1),
  # §2.12
  ('except a view within `ref.map.editor.camera_view.marker_skip_wu` (20 wu) of the camera.',
   'except a view within the skip distance of table 4.5-A of the camera.', 1),
  ('a label of the count and the actor kind\'s name, placed as `ref.map.editor.starting_forces.label`; a grid of dots, '
   '⌊√count⌋ columns by count ÷ columns rows (whole numbers), centred on the record and drawn in the team colour of its '
   'start position as `ref.map.editor.starting_forces.dot_grid`; and, in the starting-forces mode, a circle of the '
   'record\'s spread drawn as `ref.map.editor.starting_forces.spread_circle`.',
   'a label of the count and the actor kind\'s name; a grid of dots, ⌊√count⌋ columns by count ÷ columns rows (whole '
   'numbers), centred on the record and drawn in the team colour of its start position; and, in the starting-forces '
   'mode, a circle of the record\'s spread; each placed and drawn as table 4.5-A gives.', 1),
  ('5. **World axes,** drawn as `ref.map.editor.overlay.world_axes`.', '5. **World axes,** drawn as table 4.5-A gives.', 1),
  ('6. **Crosshair:** two white lines of half-length `ref.map.editor.crosshair_half_px` through the centre of the screen.',
   '6. **Crosshair:** two white lines through the centre of the screen (table 4.5-A).', 1),
  ('a sphere of radius `ref.bot.link.range` ([reference/07](../07_bots/01_navigation_graph.md)) in '
   '`ref.map.editor.gizmo.bot_nav_node_rgb` |',
   'a sphere of radius `ref.bot.link.range` ([reference/07](../07_bots/01_navigation_graph.md)) in the colour of table '
   '4.5-A |', 1),
  ('| bot objective | a sphere of `ref.map.editor.gizmo.bot_objective_radius`, red, or blue for a defence objective, and '
   'while selected a red sphere of `ref.map.editor.gizmo.bot_objective_selected_radius`;',
   '| bot objective | a red sphere, or a blue one for a defence objective, and while selected a larger red sphere, with '
   'the radii of table 4.5-A;', 1),
  ('| volcano vent, jump pad, exclusion zone, smoke column | a sphere of `ref.map.editor.gizmo.marker_radius`, and while '
   'selected a larger sphere, of `ref.map.editor.gizmo.volcano_vent_selected_radius` for a volcano vent |',
   '| volcano vent, jump pad, exclusion zone, smoke column | a sphere, and while selected a larger sphere for a volcano '
   'vent, with the radii of table 4.5-A |', 1),
  ('drawn as `ref.map.editor.gizmo.mortar_circle`, and a force indicator', 'drawn as table 4.5-A gives, and a force '
   'indicator', 1),
  ('a pending move as an outline at (pending x, `ref.map.editor.overlay.pending_move_height`, pending z);',
   'a pending move as an outline at (pending x, the pending-move outline height of table 4.5-A, pending z);', 1),
  ('offset by one cell; colours and passes as `ref.map.editor.overlay.tiles` |',
   'offset by one cell; colours and passes as table 4.5-A gives |', 1),
  ('of size (2 × size, centre y + `ref.map.editor.overlay.flatten_box_extra_height`, 2 × size), translucent, the '
   'selected area opaque, as `ref.map.editor.overlay.flatten_areas` |',
   'of size (2 × size, centre y + the extra height of table 4.5-A, 2 × size), translucent, the selected area opaque |', 1),
  ('a line from the record\'s position raised by `ref.map.editor.overlay.link_line_raise` to the world point, ending in a '
   'two-stroke arrowhead, as `ref.map.editor.overlay.link_line` |',
   'a line from the record\'s position, raised as table 4.5-A gives, to the world point, ending in a two-stroke '
   'arrowhead |', 1),
  ('the selected animation\'s arrows in a brighter colour, as `ref.map.editor.overlay.animation_arrows` |',
   'the selected animation\'s arrows in a brighter colour (table 4.5-A) |', 1),
  ('\nThe camera-view marker mesh, which the drawing and the picking test of table 2.3-B use, is not yet in the asset '
   'manifest.\n', '\n', 1),
  # §2.13
  ('The **container writer** (Publish) writes the custom-map container (§2.14). The text writer writes no campaign '
   'mission (DEC-052), and it writes no map at all when the map\'s file name contains the text `null`, the text with '
   'which the editor marks "no map", so a map whose name contains it is never saved.',
   'The **container writer** (Publish) writes the custom-map container (§2.14).', 1),
  ('the map lights (those of the open match world; none when no match world exists), the structure records (only those '
   'whose campaign flag is clear), the camera views and the starting-forces records. It writes no camera animation, no '
   'route, no primary objective and no campaign record.',
   'the map lights (those of the open match world; none when no match world exists), the structure records, the camera '
   'views and the starting-forces records. It writes no camera animation and no route.', 1),
  # §2.14
  (' Its file name is the map\'s file name with the last four characters replaced by the container\'s extension (the '
   'reference does this with an in-place text operation whose source and destination overlap, which is undefined).',
   ' Its file name is the map\'s file name with the last four characters replaced by the container\'s extension.', 1),
  # §3
  ('    HP->>ML: next update sets the requested map, no mission, the editing state and the custom-map world id, and closes '
   'every window',
   '    HP->>ML: next update sets the requested map, the editing state and the custom-map world id, and closes every '
   'window', 1),
  # §5
  ('   - a camera-view name field accepts `ref.map.editor.camera_view.name_field_chars` (63) characters, one more than',
   '   - a camera-view name field accepts 63 characters, one more than', 1),
  ('   - the display-name field accepts only `ref.map.editor.options.name_field_chars` (32) characters, far fewer than',
   '   - the display-name field accepts only 32 characters, far fewer than', 1),
  ('   - a camera-animation node accepts durations up to 100 s, while rules and conventions R-MAP-127 requires each '
   'duration to stay below `ref.map.camera.animation_duration_max_s` (60 s) (R-MAP-371);\n', '', 1),
  ('5. **Reinforcement option values.** The entries airstrike and gun turret write the same option value, and so do '
   'firebomb run and rocket turret;',
   '5. **Reinforcement option values.** The entries airstrike and gun turret write the same option value, and so do '
   'Firebomb Run and rocket turret;', 1),
  ('11. **Campaign flag.** Setting a structure\'s campaign flag removes the structure from the next save without any '
   'warning (R-MAP-347, R-MAP-386).\n'
   '12. **Names containing `null`.** The text writer writes nothing for a map whose file name contains `null`, and it '
   'reports nothing (R-MAP-385).\n'
   '13. **Short container names.** Publish replaces the last four characters of the file name with the container\'s '
   'extension; for a file name of four characters or fewer the result is not specified (R-MAP-392).\n'
   '14. **Regeneration cost.**',
   '11. **Regeneration cost.**', 1),
  ('15. **Links can only grow.**', '12. **Links can only grow.**', 1),
  ('16. **Tree height.**', '13. **Tree height.**', 1),
  ('17. **Create before a kind is chosen.**', '14. **Create before a kind is chosen.**', 1),
  ('18. **No play test.**', '15. **No play test.**', 1),
  # §6
  ('then choosing airstrike and choosing gun turret write the same value, and choosing firebomb run and choosing rocket '
   'turret write the same value.',
   'then choosing airstrike and choosing gun turret write the same value, and choosing Firebomb Run and choosing rocket '
   'turret write the same value.', 1),
  ('**AC-MEH-16 Reference text writer** (R-MAP-385, R-MAP-386, R-MAP-387). Given the reference text writer, a map named '
   '`null_test` writes no file; a structure at x = 1,234.5678 wu is written as 1,234.57 wu; a map option of value 0 is not '
   'written; and no camera animation or route is written.\n\n', '', 1),
  # §8
  ('- DEC-803 — camera animations without content: the unreachable camera-animation authoring and the developer fly mode '
   'of the editor session (R-MAP-303, R-MAP-309, R-MAP-371, R-MAP-373).',
   '- DEC-803 — camera animations without content: camera-animation authoring, which the reference editor does not offer, '
   'and the developer fly mode of the editor session (R-MAP-303, R-MAP-309, R-MAP-371, R-MAP-373).', 1),
  ('Dropped entries applied: DEC-052 (campaign missions, campaign structure kinds and the campaign world editor).',
   'Dropped entries applied: DEC-052 (campaign content is not specified).', 1),
  ('2. Undo, unsaved-change tracking and the save prompt (table 2.16-A item 14).',
   '2. Undo, unsaved-change tracking and the save prompt (table 2.16-A item 8).', 1),
  ('(table 2.16-A items 2, 3 and 5).', '(table 2.16-A items 1, 2 and 4).', 1),
  ('4. Play test of the real simulation from the editor (table 2.16-A item 14).',
   '4. Play test of the real simulation from the editor (table 2.16-A item 8).', 1),
  ('11. The camera-view marker mesh and the editor\'s drawing style (R-MAP-380).',
   '11. The editor\'s drawing style (R-MAP-380, table 4.5-A).', 1),
]

R371_OLD_START = '**R-MAP-371** (MUST) The **camera-animation panels**'
R371_NEW = ('**R-MAP-371** (MUST) The reference editor offers no camera-animation authoring: no operation opens a '
            'camera-animation panel, so an author can neither create, edit, delete nor preview a camera animation in it '
            '(DEC-803). Camera-animation authoring is a design-phase input (§8.1).')
R373_OLD_START = '**R-MAP-373** (MUST)'
R373_NEW = ('**R-MAP-373** (MUST) In the camera-views mode a select on a camera view has no effect. Camera animations exist '
            'in the map format with no content that uses them (rules and conventions R-MAP-127; DEC-803), and a map tool '
            'writes every camera animation that the loaded map holds (DEC-800, R-MAP-388).')
R387_OLD_START = '**R-MAP-387** (MUST)'
R387_NEW = ('**R-MAP-387** (MUST) The reference editor\'s writer is lossy: a load, save and load round trip moves '
            'structures, changes the generated terrain and changes the map\'s content identity (DEC-045, DEC-204), and a '
            'map option set to 0 reads back as absent (DEC-120). Every map tool writes by DEC-800 (R-MAP-388).')
R393_OLD_START = '**R-MAP-393** (MUST)'
R393_NEW = ('**R-MAP-393** (MUST) Reading a container restores the map text and registers every embedded image under its '
            'resource name, so that the terrain and the sea find it as they find a content asset. A file that is not a '
            'container, such as a text map stored under the container\'s extension, fails to load. A map tool and the '
            'custom-map loader check every length and every result, reject a container whose stream does not end where '
            'its length says, report a malformed file with a diagnostic and never fail fatally, and are fuzz-tested '
            '(DEC-015, DEC-045).')

DEFECTS_NEW = '''| # | Defect or gap | Rule | Corrected for every map tool by |
|---|---|---|---|
| 1 | Picking returns the first object hit in list order, not the nearest. | R-MAP-322 | — |
| 2 | While a structure is selected, a select on another structure does nothing; the selection must be cleared first. | R-MAP-346 | — |
| 3 | A click in the none mode on a flatten area enters the terrain-tiles mode. | R-MAP-318 | — |
| 4 | Terrain tiles, flatten areas, starting-forces records, map lights and camera animations are selected by list index, and a deletion shifts the indices of the later entries. | R-MAP-319 | — |
| 5 | The terrain regenerates on every value change, and on every frame while a decimal control is held. | R-MAP-325 | — |
| 6 | Scaling truncates the whole-number tile coordinates and does not scale flatten areas, map lights or routes. | R-MAP-328 | — |
| 7 | Clone with variation of a tree re-seeds the shared presentation generator from the wall-clock time. | R-MAP-355, R-MAP-356 | DEC-013 (R-SIM-608) |
| 8 | There is no undo, no record of unsaved changes and no save prompt when leaving; the simulation is stopped while editing, so there is no play test of the real simulation in the editor. | R-MAP-305, R-MAP-309, R-MAP-311 | — |
| 9 | The new-map path writes a plain-text map while the edit path lists only containers, and a text map under the container's extension cannot be read. | R-MAP-308, R-MAP-393 | DEC-015 |
| 10 | There is no operation for routes, route points or camera animations; mode semantics exist only through generic structure fields: ownership through the owner choice, and Flag post links and Core Device console lists can only be appended, one per select, with no clear-links operation for these kinds; no link target is checked. | R-MAP-349, R-MAP-365, R-MAP-371 | DEC-457, DEC-561 |
| 11 | The writer is lossy: a round trip changes values, and a map option of value 0 reads back as absent. | R-MAP-387 | DEC-800, DEC-120 |
| 12 | No rule of a mode is checked when a map is saved. | R-MAP-400 | DEC-561 |
| 13 | A malformed container is not rejected with a diagnostic. | R-MAP-393 | DEC-015 |'''


def replace_requirement(text, start, new):
  i = text.index(start)
  assert text.count(start) == 1, start
  j = text.index('\n\n', i)
  return text[:i] + new + text[j:]


def main():
  path, expected, s4_path = sys.argv[1:4]
  with open(path, 'rb') as fh:
    raw = fh.read()
  if hashlib.sha256(raw).hexdigest() != expected:
    sys.exit('refused: the file changed since it was read')
  text = raw.decode('utf-8')
  for old, new, n in EDITS:
    c = text.count(old)
    if c != n:
      sys.exit(f'refused: {c} matches for {old[:80]!r}')
    text = text.replace(old, new)
  # R-MAP-371: the requirement and its three bullets
  i = text.index(R371_OLD_START)
  j = text.index(R373_OLD_START, i)
  text = text[:i] + R371_NEW + '\n\n' + text[j:]
  text = replace_requirement(text, R373_OLD_START, R373_NEW)
  text = replace_requirement(text, R387_OLD_START, R387_NEW)
  text = replace_requirement(text, R393_OLD_START, R393_NEW)
  # table 2.16-A
  i = text.index('| # | Defect or gap | Rule | Corrected for every map tool by |')
  j = text.index('\n\n', i)
  text = text[:i] + DEFECTS_NEW + text[j:]
  # input ranges: every remaining backticked editor key that is an input range becomes its informative value
  for key, value in INPUT_RANGES.items():
    token = f'`ref.map.editor.{key}`'
    head, sep, tail = text.partition('## 4 Tuning table')
    if token not in head:
      sys.exit(f'refused: {token} not cited in §2')
    head = head.replace(token, value)
    text = head + sep + tail
  text = text.replace('| Operation | Effect | Input range |', '| Operation | Effect | Input range (informative) |')
  text = text.replace('| Kind | Operation or field | Input range | Field owner |',
                      '| Kind | Operation or field | Input range (informative) | Field owner |')
  # section 4
  with open(s4_path, encoding='utf-8') as fh:
    s4 = fh.read().rstrip('\n') + '\n\n'
  i = text.index('## 4 Tuning table')
  j = text.index('## 5 Edge cases and interactions')
  text = text[:i] + s4 + text[j:]
  # report editor keys still cited outside the new section 4
  head = text[:text.index('## 4 Tuning table')] + text[text.index('## 5 Edge cases and interactions'):]
  left = sorted(set(re.findall(r'`(ref\.map\.editor\.[a-z_.]+)`', head)))
  with open(path, 'w', encoding='utf-8', newline='') as fh:
    fh.write(text)
  print('written; editor keys cited outside section 4:', ', '.join(left))


if __name__ == '__main__':
  main()
