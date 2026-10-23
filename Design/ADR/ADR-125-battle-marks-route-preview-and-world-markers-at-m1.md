---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T17), owner (O75, O76, O78)
---

# The battle marks, the route preview and the world markers at M1

## Context and Problem Statement

M1 task T17's second step, T17b, draws what the battle verbs of [ADR-124](ADR-124-battle-verbs-at-m1.md) leave on the screen (M1 plan table 2 row T17; MVP-101):
- the order trail marks of a Waypoint order (R-UNIT-186);
- the route preview of the selection (R-CTRL-023, RND-350 to RND-355);
- a Waypoint banner's flag pointer (R-UNIT-191, VFX-141);
- the world markers with their edge compass and team discs (R-CTRL-380 to R-CTRL-388, RND-356 to RND-358);
- the badge near the pointer (R-CTRL-366).

O78 settled three questions: the route lines show the local preview alone; the flag pointer's target is replicated; and the Leader markers keep R-CTRL-381's off-screen rule.

Seven things in the design do not hold as written at M1:
1. **No target on the client.** The pointer aims at the first waypoint after the start of the Leader's order route, which the server holds and ADR-111 left unreplicated.
2. **No depth for pass 16.** RND-358 draws the local team's banners in pass 16 with the in-shader depth test of RND-309. M1 has neither `scene_depth_single` nor the `WorldOverlay` shader.
3. **One marker type.** The fixed, Supply crate, Power, Relic and Transport order markers of R-CTRL-381 have no source at M1; only Leaders do.
4. **No 2-D quad path.** RND-357's entries are listed as `UnlitTextured` on the 3-D `QuadInstance` stream. The markers and the badge are masks placed in the overlay's pixels and turned on the screen.
5. **The pointer's order in pass 7b.** The pointer is a quad over the cloth, and pass 7b records its quads before its cloth triangles.
6. **The route preview's cost and shape.** R-CTRL-023 builds the preview every frame. The reference places neither the spots' spacing nor the arrows (DEC-959 item 4), and gives no rule for the arrows' filtering: `fx.particle_dot` is point-magnified (AST-027), while `marker.route_arrow` is drawn larger than its 32 px.
7. **Gaps in the design:**
   - which way the owner's masks point;
   - the pointer's size;
   - the units of the 2-D sizes;
   - the badge's count text, which needs the UI's text that M1 does not have.

## Decision Drivers

* The reference look as written, wherever M1 reaches it (MVP-101), with each deviation named.
* The presentation never writes simulation state (RND-301), and the preview never touches the simulation's route store (R-SIM-358).
* A fixed number of draw calls, whatever the number of marks (RND-455).
* Every value in data, in the client's own record (CONVENTIONS §4, ADR-116).

## Considered Options

1. **Replicate the target, keep the banners in pass 7b, and add a 2-D quad path for pass 16.** The server sends the pointer's target as a figure field. The local banners stay with the Leader renderer, and pass 16 draws the Leader markers' 2-D layers off screen.
2. **Pass 16 as written.** Build `scene_depth_single`, the `WorldOverlay` shader and its in-shader test, and draw the local team's banners there.
3. **No world markers at M1.** Draw the trail marks, the preview, the pointer and the badge, and leave the markers for M3's other types.

## Decision Outcome

Chosen option: 1, because every T17 mark is drawn without the in-shader depth machinery, which only the banners and the cursor rings need. The off-screen Leader marker, the case O78 kept, is a 2-D drawing. Option 2 builds a resolve of the depth buffer and a fourth shader family for one layer at M1. Option 3 drops the off-screen compass that the owner chose to keep.

* **The flag pointer's target** (O78; ADR-111 amendment):
  - **The field.** `ReplicatedFigure::target` is the first point after the start of a Leader's Waypoint order route (R-UNIT-191), and none for any other figure or order.
  - **On the wire.** Field bit 64 of the figure delta carries it, as a presence bit and, when present, a whole position. A whole figure record carries the presence bit too. `StateUpdate::VERSION` is 2.
  - **Every team's.** The server sends it for every team, since the Leader renderer draws every banner.
* **The flag pointer** (VFX-141; ADR-122 amendment):
  - **The quad.** A camera-facing quad of `marker.flag_pointer` at the centre of the cloth's vertices. Its half-size is `vfx.cloth.pointer_size_factor` (0.25, provisional) × the banner's size, and it is drawn white.
  - **The entry.** `cloth_pointer`: `UnlitTextured` with the mask pixel shader, `Alpha`, `Test`, `None`, and the bias variant `bias_flag_pointer`. The variant's 5 depth-bias units and slope-scaled bias of 1.0 are compiled in (`DepthBias.h`), since the catalogue is made before the record is read, and a test holds them to `vfx.cloth.pointer_constant_bias` and `vfx.cloth.pointer_slope_bias`. `PipelineEntry` gains the slope-scaled bias.
  - **Its place.** It is drawn at the end of pass 7b, after the cloth, as the scene pass `Pointers`.
  - **Its turn.** Its art points down, and it is turned to point towards the target on the screen (RND-216). The direction comes from the two points' projections when both lie ahead of the camera, and from their difference on the view plane otherwise.
* **The order trail marks** (R-UNIT-186):
  - **The quad.** A ground-aligned `marker.order_trail` in pass 10a with `effect_additive_nodepth` and the team's colour, since it lies on the terrain. It is lowered by `ref.unit.leader.trail_lift`. Its front is the replicated heading, and its right is the front × the terrain normal.
  - **Size and fade.** Its half-size is max(`ref.unit.leader.trail_size_min`, `ref.unit.leader.trail_size_scale` × the camera distance ÷ `ref.unit.leader.trail_size_divisor`). Its alpha is `ref.unit.leader.trail_alpha` × (1 − age). The age runs from the display tick at which the client first saw the mark, since the mark's creation tick is not replicated.
  - **Its team.** The server sends only the local team's marks (ADR-106).
* **The route preview** (O78; R-CTRL-023, RND-353, RND-354):
  - **From.** A selected Leader's position, unless the target is nearer than `ref.unit.leader.formation_toggle_radius`, where a press forms it up (R-UNIT-170). Otherwise the centre of the selected Workers that the frame's figures hold (R-SIM-402); with none, there is no preview. The selection is the one the verbs resolve with (ADR-124).
  - **To.** The order target (R-CTRL-057), straight while Ctrl is held.
  - **Its store.** The route is built with the region's land route graph into the preview's own store (R-SIM-358).
  - **Rebuilding.** It is built again only when an end moves more than `rnd.route.preview_rebuild_distance` (1 wu, provisional) or the straight flag changes. Otherwise the last route stands, so a still pointer costs no query.
  - **No fade.** It is shown at full strength while it exists and vanishes with it: R-CTRL-023 discards it each frame, so it has no fade record (RND-351, RND-352). ADR-127 keeps the preview of an order sent as an ordered route that fades.
  - **The spots.** Spot *i* has the half-size of its camera distance ÷ `rnd.route.spot_size_divisor`, and the next lies one spot's size, twice that half-size, further along the route (DEC-959 item 4). The step is at least 0.05 wu and a preview has at most 4,096 spots. The spots use `route_line` and `fx.particle_dot`.
  - **The colours.** The team's colour while walkable, otherwise `rnd.route.blocked_color`, plus `rnd.route.gray_offset` and the pulse of RND-353 on the real clock.
  - **The arrows.** One `marker.route_arrow` at each leg's middle, in the colour of the spot there. Its half-size is `rnd.route.arrow_size_factor` (2, provisional) × the spot half-size there, and a leg shorter than the arrow gets none. Its art points right, and it is turned along the leg on the screen.
  - **The arrows' entry.** The new entry `route_arrow`: `route_line`'s state with the linearly filtered mask shader, since the arrow is magnified.
  - **No route.** A preview that is not walkable shows the unavailable cursor (O76), which then outranks the highlight. As off the region (ADR-124), a secondary press then orders neither the crowd nor the selected Leader; a toggle still goes. The verbs read the last frame's preview.
* **The world markers** (R-CTRL-380 to R-CTRL-388):
  - **Which.** One Leader marker for each living Leader of the local team, made when the Leader is first seen rather than at its promotion, so a Leader present at the join has one. It is removed with its Leader.
  - **Tracking and order.** It tracks the Leader lifted by `ref.ctrl.marker.lift`, smoothed by min(1, `ref.ctrl.marker.smoothing` × the real step). One bubble-sort pass a frame puts the farthest first; a marker younger than `ref.ctrl.marker.new_age` takes its age as its key.
  - **On screen.** Its time on screen counts towards being seen. Nothing of it is drawn in pass 16: its icon is the Leader's banner, which pass 7b draws (RND-358 amended).
  - **Off screen.** Until seen, it is held at the screen's edge by R-CTRL-383, with `ref.ctrl.marker.edge_probe`, `edge_probe_ahead` and `edge_inset`.
  - **The layers.** In R-CTRL-386's order: the disc shadow and the compass shadow in black with `ref.ctrl.marker.shadow_alpha` (`overlay_alpha`); the compass, its art's arrow pointing up, turned outward towards the Leader; the team disc; and `icon.power.leader` in place of the banner (`overlay_additive`).
  - **The sizes.** R-CTRL-385's, each the full side of its square: the icon is the target's height ÷ `ref.ctrl.marker.size.icon_divisor`, the shadow adds `shadow_pad`, and the compass adds the height ÷ `compass_divisor`. They are × (2 − distance ÷ `near_distance`) while near, and × `hover_scale` while hovered or selected.
  - **Under the pointer.** The marker whose icon box holds the pointer is the marker under the pointer, and the next frame's verbs read it as an own Leader under the pointer (R-CTRL-387).
  - **Not built.** R-CTRL-384's rules 2 to 4 and the flash and spin of R-CTRL-381 do not reach a Leader marker drawn this way: seen, it never shows off screen, and on screen it draws nothing here. M1 does not build them.
* **The badge** (R-CTRL-366 rows 2 and 3):
  - **The layers.** While a Leader or Workers are selected, `icon.power.leader` or `icon.power.workers` on the team's disc over its shadow, in the world markers' layers.
  - **Its place and size.** Its centre lies `ref.ctrl.cursor.icon_offset` + `ref.ctrl.cursor.icon_center_offset` along Normalize(0.85, 1) from the pointer, and its side is the target's height ÷ `ref.ctrl.cursor.icon_divisor`.
  - **No count.** The selected Workers' count is not drawn at M1; ADR-127 draws it in GameFont. A route that cannot be walked shows O76's unavailable pointer, not the badge's red sprite.
* **Pixels.** The 2-D sizes and offsets are physical pixels of the back buffer, not scaled by the DPI, as O76's cursors are.
* **The overlay quads** (RND-356, RND-357):
  - **The quad.** `OverlayQuad` is a mask of the scene's textures, its centre, half-size and clockwise turn in physical pixels, and its blend.
  - **The shader.** The new `OverlayQuad` shader turns the square's corners and averages the mask over each pixel's footprint, as the cursor's does.
  - **The entries.** `overlay_alpha` blends the colour with the mask × alpha; `overlay_additive` adds the colour × the mask × alpha.
  - **Drawing.** The quad renderer draws them in pass 16 (`pass_16_world_markers`, after `final`) and in pass 20 before the cursor, one instanced call for each run of one texture and blend.
* **Pass 12** (`pass_12_route_lines`) lies between passes 11 and 15. `ScenePass` gains `Routes` and `Pointers`.
* **The masks' directions** were read from the owner's files by decoding them: the edge compass's arrow points up, the route arrow right and the flag pointer down; the order trail is symmetric.
* **The render check** (ADR-117). A Debug client's `--render-check` drives the verbs by script: it promotes a Worker, orders the new Leader towards the region's centre, and then holds a selection circle with the ground point beyond the Workers. The check's frames rise to 40, so the trail, the pointer, the preview and the badge reach the WARP adapter.

### Consequences

* Good, because the steward sees where each Leader heads (its pointer and trail), where the crowd would walk (the preview), what is selected (the badge) and where an unseen Leader went (the edge compass).
* Good, because the markers and the badge share one 2-D path, drawn with a call per run.
* Good, because the target costs one bit per whole figure record, and a position only when it changes.
* Bad, because a local banner is depth-tested in pass 7b rather than drawn over the markers' discs, and an on-screen Leader marker draws no disc behind it.
* Bad, because the preview kept within 1 wu of its ends is up to 1 wu stale.
* Neutral, because the provisional keys `vfx.cloth.pointer_size_factor`, `rnd.route.arrow_size_factor` and `rnd.route.preview_rebuild_distance` wait for the owner's T17 run.

### Confirmation

* GameSharedTests: a whole Leader record with a target, and the target's delta set and cleared (`StateUpdateTests`).
* GameLogicTests: a Waypoint Leader's replicated target is its route's first point after the start, and a Leader in formation has none (`CommandTests`).
* GameClientTests:
  - the trail mark's size, fade, lowering and heading (`EffectsLookTests`, R-UNIT-186);
  - the pointer after the cloth, its size, its turn and its compiled-in bias (`LeaderLookTests`, VFX-141, VFX-014);
  - the preview's spots, colours, pulse and arrows, a route into the sea not walkable, and its rebuilding (`RoutePreviewTests`);
  - the orders held back while the route cannot be walked (`BattleVerbsTests`);
  - the markers' making and removal, the edge placement, the layers and sizes, the marker under the pointer, the seen rule and the drawing order (`WorldMarkersTests`);
  - the badge (`WorldMarkersTests`, R-CTRL-366).
* CI's render check with the scripted verbs (`Build\CheckWorldFrame.py`).
* The owner's T17 run, with the checklist of the pull request.

## Pros and Cons of the Options

### 1 — replicate the target, banners in 7b, a 2-D path in pass 16

* Good, because every mark M1 needs is drawn with machinery that exists or is small.
* Bad, because the local banners do not move to pass 16 until its in-shader test exists.

### 2 — pass 16 as written

* Good, because RND-358 holds as written.
* Bad, because it builds a depth resolve, the `WorldOverlay` shader and its test for the banners alone at M1.

### 3 — no world markers at M1

* Good, because it is the least code.
* Bad, because it drops the off-screen compass that O78 kept.

## More Information

* Related: [ADR-106](ADR-106-leaders-formations-and-battle-commands.md), [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md), [ADR-122](ADR-122-leaders-and-banners-drawn-at-m1.md), [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md), [ADR-124](ADR-124-battle-verbs-at-m1.md).
* Owner decisions O75, O76 and O78; MVP-101; R-UNIT-186, R-UNIT-191; R-CTRL-023, R-CTRL-057, R-CTRL-366, R-CTRL-380 to R-CTRL-388; R-SIM-358, R-SIM-402; RND-216, RND-350 to RND-358; VFX-014, VFX-141; DEC-959.
* The code: `GameShared/ReplicatedState`; `GameLogic/StateSender`; `NeuronClient/QuadRenderer`, `FrameRenderer`, `PipelineCatalog`, `DepthBias.h`, `WorldScene.h` and `Shader/OverlayQuad*`; `GameClient/RoutePreview`, `WorldMarkers`, `LeaderLook`, `EffectsLook`, `SettlementView` and `WorldPresentation`.
* **Amendment 2026-10-21 (M1 task T17c, owner decision O82, [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md)).** The order target is the pointer ground point alone, and an own Leader under the pointer shows no route, since a primary press on it selects it. The preview of an order sent is kept as an ordered route with a fade record (RND-351, RND-352) and fades at `rnd.route.fade_rate`; the preview and each ordered route end in `marker.order_trail` on the ground; an arrow lies at its leg's middle on the screen; and the badge shows the selected Workers' count in GameFont. The render check orders its Leader with the primary press.
* **Amendment 2026-10-23 (M3 task T12, owner decision O98, [ADR-167](ADR-167-the-conflict-verbs-and-markers-at-m3.md)).** The world markers' other types arrive. The expedition's Leaders have Leader markers; while one is outside the steward's areas its marker stands at it on screen with no compass and stays at the edge off screen however long it has been seen. Each battle of the steward's settlement, attack on it, target of its own expedition and claim of or on it has a marker: the team disc with the empty compass ring on screen or the compass at the edge, drawn always and under the Leaders' markers, with its icon drawn by the HUD over the disc.
