# Entity and overlay renderers

> Status: Engine baseline · Prefix: RND · ADRs: [ADR-0005](../../../adr/0005-pipeline-state-catalogue.md) · [ADR-0006](../../../adr/0006-root-signature-and-bindless-resource-access.md) · [ADR-0008](../../../adr/0008-gpu-memory-uploads-and-device-removal.md) · [ADR-0009](../../../adr/0009-instanced-crowds-sprite-cards-and-shadows.md) · [ADR-0010](../../../adr/0010-gpu-particles-and-shatter-debris.md) · [ADR-0013](../../../adr/0013-working-colour-space-and-blending.md) · [ADR-0016](../../../adr/0016-view-projection-and-depth-conventions.md) · [ADR-0018](../../../adr/0018-texture-import-manifest.md) · [ADR-0020](../../../adr/0020-flat-shading-and-team-tint.md) · [ADR-0021](../../../adr/0021-mesh-socket-schema.md) · [ADR-0031](../../../adr/0031-batched-ui-rendering.md) · [ADR-0033](../../../adr/0033-ui-layout-scaling-and-dpi.md) · [ADR-0035](../../../adr/0035-procedural-animation-runtime.md) · [ADR-0036](../../../adr/0036-effect-events-and-presentation-clock.md) · [ADR-0064](../../../adr/0064-accessibility-baseline-xag.md) · Related: [Renderer](README.md) · [Frame graph and passes](frame_graph_and_passes.md) · [World renderers](world_renderers.md) · [View, lighting and fog](view_lighting_and_fog.md) · [Shaders](shaders.md) · [Visual language](visual_language.md) · [Options and budgets](options_and_budgets.md) · [Animation and VFX](../animation_and_vfx.md)

This document specifies how the renderer draws everything that moves or points: the crowds of actors (Workers, Crawlers, Troopers, creatures and vehicles) as instanced sprite cards, billboards and lit meshes; their blob shadows; the effects pass with its shockwave rings and lightning; laser bolts; route lines; world markers; the world cursor with its highlight rings and placement previews; and the Power slots panel and match HUD overlays. It uses the pass numbers, catalogue blend states and depth-bias variants of [frame_graph_and_passes.md](frame_graph_and_passes.md) and [view_lighting_and_fog.md](view_lighting_and_fog.md), and defines the catalogue entries of its own passes (§3.3).

## 1 Responsibilities and boundaries

**RND-300** (MUST) This document's renderers record passes 7a, 7b, 7c, 7d, 10a, 10b, 12, 16, 20, 23 and 28 of the ordered pass table ([frame_graph_and_passes.md](frame_graph_and_passes.md) RND-070), with the catalogue entries of §3.3. Pass 11 (Essence orbs), pass 14 (shatter debris) and pass 15 (particles), and the Worker ground-shadow values of pass 7d, belong to [animation_and_vfx.md](../animation_and_vfx.md); passes 13, 17, 19 and 21 draw UI-framework content ([ui_framework/README.md](../ui_framework/README.md)).

**RND-301** (MUST) These renderers MUST NOT write simulation state (see DEC-007). Every value that the reference look computes while drawing and keeps from frame to frame is presentation state that the renderer owns, keyed by the object's handle (`SimObjectId`) or the route handle, and never stored in a simulated object (DEC-222 item 2). The values are listed in §3.2: the Worker colour cross-fade after a conversion, the fade value of each drawn route, the walkable flag of the route preview, the visibility timers of world markers, and the cursor animation phases.

**RND-302** (MUST) A renderer draws exactly the state that the presentation snapshot holds and never caps a list that the simulation tracks. In particular the energy-grid and fuel-chain renderers draw every energy surge that a fuel pump tracks; the one limit is `ref.bldg.fuel_pump.max_surges` (20) of [economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) R-BLDG-198, used by the simulation and the drawing alike (see DEC-463). When a frame has more instances than an instance buffer holds, the upload-ring overflow rule of the renderer applies (README RND-017, ADR-0008); no instance is dropped.

**RND-303** (MUST) Every draw of these passes names one complete catalogue entry (ADR-0005, RND-052). The reference look draws the actors of passes 7a and 7b one after another, and an actor that sets no state of its own inherits what the previous actor left; the Species renderer reproduces only the states that §4.4 lists for each actor kind, never a state left by an earlier draw.

**RND-304** (MUST) Every random choice these renderers make, such as the team colour of an Allegiance Scrambler muzzle flash, draws from the presentation generator (`FxRng`), never from the simulation generator (RND-005, DEC-013).

## 2 Interfaces

The interface below is a C++23 declaration sketch, not an implementation.

```cpp
namespace species::render {

enum class OrientationMode : std::uint8_t {   // the quad orientation modes of ADR-0009
  CameraFacing,     // corners along the camera right and up vectors
  UprightCard,      // turns about the world vertical axis towards the camera
  WorldOriented,    // explicit right and up axes, optionally blended into a billboard with distance
  Segment           // between two end points, widened perpendicular to the camera ray
};
enum class QuadAnchor : std::uint8_t { Centre, BottomCentre };

struct QuadInstance {                 // shared header layout (shaders.md RND-262), §3.1
  Vec3 position;                      // centre or anchor; first end point for Segment
  Vec3 endPoint;                      // Segment only
  Vec3 axisRight, axisUp;             // WorldOriented only
  float halfWidth = 0, halfHeight = 0;
  float blendStart = 0, blendLength = 0;   // WorldOriented: billboard blend; length 0 = never
  float rotation = 0;                 // radians about the view axis (CameraFacing)
  ColourRgba colour, endColour;       // endColour is used by Segment only
  std::uint32_t textureIndex = 0;     // descriptor-heap index (ADR-0006)
  std::uint32_t uvRect = 0;           // index into the UV-rectangle table (atlas cells, mirrored maps)
  OrientationMode mode = OrientationMode::CameraFacing;
  QuadAnchor anchor = QuadAnchor::Centre;
};

struct InstanceType {                 // one per drawn kind and catalogue entry, registered at start-up
  CatalogueKey entry;                 // §3.3
  float cullRadius = 0;               // sphere-test radius around each instance (RND-234)
  std::optional<float> maxDistance;   // per-type distance limit
};

class CrowdRenderer {                 // ADR-0009
public:
  InstanceTypeId RegisterType(const InstanceType& type);
  std::span<QuadInstance> BeginInstances(InstanceTypeId type, std::uint32_t count);  // upload ring
  void RecordCull(PassContext& pass) const;                                        // instance_cull
  void RecordDraws(PassContext& pass, std::span<const InstanceTypeId> types) const;
};

class ActorRenderer {
public:
  void RecordGroups(PassContext& pass, const PresentationSnapshot& snap, double t) const;    // 7a
  void RecordOthers(PassContext& pass, const PresentationSnapshot& snap, double t) const;    // 7b
  void RecordCrawlers(PassContext& pass, const PresentationSnapshot& snap, double t) const;  // 7c
  void RecordWorkers(PassContext& pass, const PresentationSnapshot& snap, double t) const;   // 7d
  void OnTeamChanged(SimObjectId worker, std::uint8_t previousColourId, double realNow);     // RND-317
};

class EffectRenderer {
public:
  void RecordEffects(PassContext& pass, const PresentationSnapshot& snap, double t) const;     // 10a
  void RecordLaserBolts(PassContext& pass, const PresentationSnapshot& snap, double t) const;  // 10b
};

class RouteLineRenderer {
public:
  void OnRouteHighlighted(RouteHandle route, std::uint8_t team);   // RND-350
  void OnRouteIntensity(RouteHandle route, float intensity);       // RND-351
  void Record(PassContext& pass, const RoutePreview& preview, double realNow);  // pass 12
  [[nodiscard]] bool PreviewWalkable() const;                      // read by the world cursor
};

class WorldMarkerRenderer   { public: void Record(PassContext& pass, const WorldMarkers& markers); };  // 16
class WorldCursorRenderer   { public: void Record(PassContext& pass, const WorldCursor& cursor); };   // 20
class PowerSlotsPanelRenderer { public: void Record(PassContext& pass, const PowerSlotsPanel& panel); };  // 23
class MatchHudRenderer      { public: void Record(PassContext& pass, const MatchHudView& hud); };     // 28

struct DevSphereMesh { std::array<Vec3, 42> vertices; std::array<std::uint16_t, 240> indices; };  // RND-336
DevSphereMesh BuildDevSphere();

}  // namespace species::render
```

**Events.** The renderers consume these presentation events (DEC-007, ADR-0036): a Worker's team change (for the colour cross-fade, RND-317); a route highlight and a route intensity request (RND-350, RND-351); the registration and removal of world markers ([reference/06_controls_camera_and_commands.md](../../../reference/06_controls_camera_and_commands.md)). They raise one request of their own: the camera shake of a laser bolt that passes near the camera, which they hand to the VFX screen effects (RND-347).

**Data contracts.** Per drawn actor the snapshot holds the fields of README §2 (position, velocity, front and up, team, kind, health) and the kind-specific fields named in §4.4; per route its points and its end point; per world marker its target, kind, icon, owner, persistence and progress fraction; per effect its kind, position, velocity, age and kind-specific fields.

**Threading contract.** All public calls run on the frame thread. Instance-buffer writes of different instance types MAY run in parallel where the threading model allows (DEC-224). The cull compute pass of every pass that draws instances runs before its draws, on the direct queue, in the same command list.

## 3 Data model

### 3.1 Quad conventions

**RND-305** (MUST) A quad's size in this document is its half-size: its corners lie at its centre ± half-width along its right axis and ± half-height along its up axis, or, with the anchor `BottomCentre`, from its anchor point up to 2 × half-height along its up axis. A quad is two triangles with the index pattern 0, 1, 2, 0, 2, 3 (RND-055), and every quad of this document is expanded in the vertex shader from one `QuadInstance` (ADR-0009).

**RND-306** (MUST) The vertex shader builds the four corners of each orientation mode as follows, with *R* and *U* the frame's camera right and up vectors:

```text
CameraFacing:   right = R rotated by rotation about the view axis; up = U rotated likewise
UprightCard:    up = (0, 1, 0); right = normalise(up × (camera position − position))
WorldOriented:  right = axisRight; up = axisUp
                if blendLength > 0 and d > blendStart:          # d = |position − camera position|
                    f = min((d − blendStart) ÷ blendLength, 1)
                    right = lerp(right, R, f); up = lerp(up, U, f)  # linear, not renormalised
Segment:        mid = (position + endPoint) ÷ 2
                side = normalise((mid − camera position) × (mid − endPoint)) × halfWidth
                corners = position − side, position + side, endPoint + side, endPoint − side
                colour at position = colour; colour at endPoint = endColour
```

The corners of the first three modes are centre − right·w − up·h, centre + right·w − up·h, centre + right·w + up·h, centre − right·w + up·h (half-sizes *w*, *h*), and for the anchor `BottomCentre` anchor − right·w, anchor + right·w, anchor + right·w + 2·up·h, anchor − right·w + 2·up·h.

### 3.2 Presentation state owned by these renderers

| State | Key | Content | Lifetime | Requirement |
|---|---|---|---|---|
| Worker colour cross-fade | `SimObjectId` | previous team colour id, start time on the real clock | from the team-change event until `ref.unit.worker.convert_fade` has passed | RND-317 |
| route fade | route handle | fade value in [0, 1]; under option B of DEC-651 a copy of the route's points | while the value is above 0 | RND-351, RND-352 |
| route-preview walkable flag | — | one flag per frame | one frame | RND-354 |
| world-marker visibility | marker | on-screen time, seen flag, flash start time | the marker's life | RND-358 to RND-360 |
| selection-circle phase | — | the rotation angle of the selection-circle particles | the session | RND-373 |
| Crawler level of detail | Crawler group | the detail level of the frame | one frame | RND-322 |

**RND-307** (MUST) The renderer creates and deletes these records only from presentation events and from the snapshot; a record whose object no longer exists in the snapshot is deleted at the end of the frame, and no record is read by any simulation rule (DEC-007).

### 3.3 Catalogue entries of the actor, effect and overlay passes

**RND-308** (MUST) These passes use exactly the catalogue entries below, each a full pipeline descriptor in the default-plus-differences form of RND-055. Blend states are those of RND-058; depth-bias variants are those of RND-219; "in-shader" depth means the test against `scene_depth_single` of RND-072.

| Entry | Shader ([shaders.md](shaders.md) or §3.4) | Blend | Depth | Cull | Cut-out or bias | Pass | Draws |
|---|---|---|---|---|---|---|---|
| `group_mesh_lit` | `mesh_lit` | `Alpha` | `TestWrite` | `Back` | cut-out `rnd.pso.strike_team_cutout` | 7a | centipede segments, Bombers and their bombs, ants of a Hive group |
| `trooper_mesh_lit` | `mesh_lit` | `Opaque` | `TestWrite` | `Back` | — | 7a | Troopers |
| `actor_mesh_lit` | `mesh_lit` | `Opaque` | `TestWrite` | `Back` | — | 7b | lit actor meshes, with the object lighting or the special lighting chosen per draw (RND-228, RND-229) |
| `actor_flicker_additive` | `mesh_lit` | `Additive` (facet alpha 1, so source and destination factors are one and one) | `TestWrite` | `Back` | — | 7a, 7b | the damage-flicker draw |
| `actor_flicker_darkening` | `mesh_lit` | `Darkening` | `TestWrite` | `Back` | — | 7b | the spider's damage-flicker draw |
| `actor_strip_lit` | `mesh_lit` | `Opaque` | `TestWrite` | `None` | — | 7b | Brood Drifter tail strips |
| `actor_glow` | `unlit_textured` | `Additive` | `Test` | `None` | — | 7b | Collector bubble, Crawler egg cards, barrage missile and launcher cards |
| `actor_glow_nodepth` | `unlit_textured` | `Additive` | `None` | `None` | — | 7b | meteor flares, the Reaper Serpent's carried Essence orbs, the barrage ground marker |
| `actor_ink_nodepth` | `unlit_textured` | `Ink` | `None` | `None` | — | 7b | the shadow under the barrage ground marker |
| `actor_ink` | `unlit_textured` | `Ink` | `Test` | `None` | — | 7b | the barrage launcher shadow |
| `actor_darkening` | `unlit_textured` | `Darkening` | `Test` | `None` | — | 7b | the Rogue faction's barrage missile cards |
| `actor_beam_alpha` | `unlit_textured` | `Alpha` | `Test` | `None` | — | 7b | the Collector's harvesting beam, the Abductor's beam rings |
| `actor_beam_additive` | `unlit_textured` | `Additive` | `Test` | `None` | — | 7b | barrage missile trails |
| `blob_shadow` | `unlit_textured` | `Darkening` | `Test` | `None` | `bias_blob_shadow` | 7b | blob shadows (RND-325) |
| `crawler_trail` | `unlit_textured` | `Additive` | `Test` | `None` | `bias_crawler` | 7c | Crawler worm and glow strips |
| `crawler_trail_rogue` | `unlit_textured` | `Ink` | `Test` | `None` | `bias_crawler` | 7c | the Rogue faction's Crawler strips |
| `worker_card` | `unlit_textured`, `CUTOUT` | `Alpha` | `TestWrite` | `None` | cut-out `rnd.pso.worker_cutout` | 7d | Worker cards, side cards, hurt cards, state overlays, seasonal hats |
| `worker_ground_shadow` | `unlit_textured`, `CUTOUT` | `Alpha` | `Test` | `None` | cut-out `rnd.pso.worker_cutout` | 7d | Worker ground shadows |
| `worker_glow` | `unlit_textured` | `Additive` | `Test` | `None` | — | 7d | Worker glows and bubbles (R-UNIT-147) |
| `effect_additive` | `unlit_textured` | `Additive` | `Test` | `None` | — | 10a | the effect batch, the lightning layers, the shockwave blast |
| `effect_gradient_nodepth` | `unlit_vertex_colour` | `Additive` | `None` | `None` | — | 10a | the shockwave ground ring |
| `laser_bolt` | `unlit_textured` | `Additive` | `Test` | `None` | `bias_laser` | 10b | laser bolts |
| `route_line` | `unlit_textured` | `Alpha` | `None` | `None` | — | 12 | route spots and arrows |
| `overlay_alpha` | `unlit_textured` | `Alpha` | `None` | `None` | — | 16, 20, 23, 28 | marker and panel shadows, the crosshair, panel art |
| `overlay_additive` | `unlit_textured` | `Additive` | `None` | `None` | — | 16, 20, 23 | icons, compass arrows, colour discs, cursors |
| `overlay_ink` | `unlit_textured` | `Ink` | `None` | `None` | — | 20 | blurred cursor and icon shadows, drawn with source alpha 0 |
| `overlay_gradient_alpha` | `unlit_vertex_colour` | `Alpha` | `None` | `None` | — | 23 | the pop-up box gradient |
| `cursor_ring` | `world_overlay`, `SOURCE = vertex_colour` | `Alpha` | in-shader, with `bias_cursor` | `None` | `bias_cursor` | 16, 20 | highlight rings and the glow cylinder |
| `cursor_area` | `world_overlay`, `SOURCE = vertex_colour` | `Alpha` | in-shader | `None` | — | 20 | the area cylinder |
| `cursor_dots` | `world_overlay`, `SOURCE = textured` | `Alpha` | in-shader, with `bias_cursor` | `None` | `bias_cursor` | 20 | formation-preview diamonds |
| `cursor_ghost` | `world_overlay`, `SOURCE = mesh` | `Ghost` | in-shader | `Back` | — | 20 | placement ghost models |

**RND-309** (MUST) An in-shader depth test of the `world_overlay` shader keeps a fragment only when its reversed-Z depth, multiplied by the factor *k* of its depth-bias variant (1 without one), is at least the value of `scene_depth_single` at the fragment's scene position; this is the in-shader form of the depth-bias rule of RND-220 and lets these 3-D overlays keep their place after the UI windows (RND-072). None of these entries writes depth.

### 3.4 Shaders owned by this document

**RND-310** (MUST) This document adds two shaders to the set of [shaders.md](shaders.md) RND-263, following its rules (RND-260, RND-261):

| Shader | Stages | Permutation axes | Catalogue entries | Pass |
|---|---|---|---|---|
| `instance_cull` | CS | none | — (compute) | before the draws of 7a to 7d, 10a, 10b and 11 |
| `world_overlay` | VS, PS | `SOURCE` (`vertex_colour`, `textured`, `mesh`) | `cursor_ring`, `cursor_area`, `cursor_dots`, `cursor_ghost` | 16, 20 |

Sprite cards, billboards, segments and strips are drawn by `unlit_textured` and `unlit_vertex_colour` with the `QuadInstance` stream; `world_overlay` with `SOURCE = mesh` draws a cooked mesh's facet colours unlit.

## 4 Behaviour and algorithms

### 4.1 Crowd rendering and instancing

**RND-311** (MUST) Crowds are drawn by GPU-driven instancing (ADR-0009). Each frame, for every instance type of a pass, the CPU writes the presentation view of every visible candidate (position, axes or facing, size, colour, team palette index, orientation mode, texture and UV cell) into the type's instance buffer in the upload ring (ADR-0008). The `instance_cull` compute pass then tests each instance against the view frustum with its type's cull radius and distance limit, compacts the survivors with wave operations and writes the indirect arguments, and one `ExecuteIndirect` call draws each type in each pass that draws it. The CPU cost of a crowd is therefore a fixed number of calls per type and pass, whatever the number of figures (RND-040).

```mermaid
flowchart LR
  S["Presentation snapshot: actors, effects, laser bolts"] --> W["CPU: write QuadInstance and MeshInstance records per type into the upload ring"]
  W --> C["instance_cull: frustum, cull radius and distance limit per type; compaction"]
  C --> A["Indirect arguments per type"]
  A --> D["ExecuteIndirect per type and pass: 7a, 7b, 7c, 7d, 10a, 10b"]
  D --> T["scene_colour and scene_depth"]
```

Caption: one frame of crowd drawing; the CPU writes instance data, the GPU culls, compacts and draws, and the number of calls depends only on the number of instance types.

**RND-312** (MUST) The GPU cull is conservative (RND-236): it never rejects an instance that the CPU view test of its kind would keep. Each kind's CPU view test is the one RND-234 and RND-235 name; the effect kinds drawn without a view test (RND-331) are written with an infinite cull radius.

**RND-313** (MUST) Instances of one type are drawn in the order the CPU writes them, which is the order of the owning pool, so that a pass that the reference draws in pool order (passes 7a and 7b) keeps that order; additive and darkening instance types are order-independent (RND-058) and MAY be compacted in any order.

### 4.2 Workers (pass 7d)

**RND-314** (MUST) Every in-view Worker is drawn as a card of `sprite.worker.front` with the `worker_card` entry, as a `WorldOriented` quad with the anchor `BottomCentre`, built from the Worker's values in [reference/02_units/worker.md](../../../reference/02_units/worker.md) R-UNIT-142 to R-UNIT-148 by this procedure, where *d* is the camera distance of the Worker's drawn position (RND-081), *E* the entity-detail setting ([options_and_budgets.md](options_and_budgets.md)), *k* the Worker distance scale factor setting and *g* the presentation frame time (R-SIM-570):

```text
D  = rnd.crowd.worker_detail_distance[E]                 # 99999.9, 1000 or 500 wu
hd = clamp(1 − d² ÷ D², 0, 1)                            # the high-detail factor
s  = ref.unit.worker.card_half_width
     × (1 + ref.unit.worker.card_variation × ((slot index × unit id) mod 10))
if k > 0 and d > ref.unit.worker.card_scale_start:
    s = s × (1 + k × (d − ref.unit.worker.card_scale_start) ÷ ref.unit.worker.card_scale_divisor)
if the Worker has Fury:
    s = s × (1 + |sin g| × rnd.crowd.worker_fury_pulse)
far = ref.unit.worker.card_face_start + ref.unit.worker.card_scale_divisor   # 1300 wu
up = terrain normal under the Worker   if hd > 0 and d < far
     (0, 1, 0)                         otherwise
up = up tilted by the sway or tumble of R-UNIT-143
right = Worker front × up
quad: halfWidth = s, halfHeight = s, axisRight = right, axisUp = up,
      blendStart = ref.unit.worker.card_face_start, blendLength = ref.unit.worker.card_scale_divisor
```

The card therefore faces the Worker's own direction near the camera and turns into a camera billboard between 300 wu and 1300 wu (RND-306), and its size grows with distance so crowds stay readable when the camera is far ([visual_language.md](visual_language.md) RND-414).

**RND-315** (MUST) The high-detail factor *hd* gates three things: the terrain-following orientation of RND-314, the hurt cards of R-UNIT-145, which are drawn only while *hd* is above 0, and the Worker's ground shadow, whose alpha is `ref.unit.worker.shadow_alpha` × *hd* (the "detail level" of R-UNIT-147), within `ref.unit.worker.shadow_range` and with the values of [animation_and_vfx.md](../animation_and_vfx.md) (RND-241, RND-221), drawn with `worker_ground_shadow`.

**RND-316** (MUST) The card texture is chosen per Worker, in this order of precedence: `sprite.visitor.front` for a Visitor faction Worker; `sprite.worker.side` when the card is seen edge-on (R-UNIT-144); `sprite.worker.hurt` variant 04 for a Burning Worker and every Rogue faction Worker, and variants 01 to 03 for the airborne corpses of R-UNIT-145; `sprite.worker.front` otherwise. The state overlay `sprite.worker.state_overlay` and, while the seasonal state is active, `sprite.worker.seasonal_hat` on every third Worker (R-UNIT-148) are further `worker_card` instances; the glows and the Shield bubble of R-UNIT-147 are `worker_glow` instances. The Visitor faction Worker's glow is `fx.glow_round` (R-UNIT-145).

**RND-317** (MUST) A Worker's card colour is the colour of R-UNIT-146: team colour × (`ref.unit.worker.colour_base` + `ref.unit.worker.colour_health_weight` × health fraction); a dead Worker's RGB × `ref.unit.worker.corpse_colour` with alpha `ref.unit.worker.corpse_alpha_per_health` × health. After a team change the renderer starts a cross-fade record (§3.2) on the presentation team-change event and draws the team colour as lerp(previous team colour, new team colour, min(elapsed real time ÷ `ref.unit.worker.convert_fade`, 1)); it deletes the record when the fade completes. The cross-fade never changes a field of the simulated Worker (RND-301).

**RND-318** (MUST) The Worker cards use the team palette through their vertex colour, not the mesh tint of ADR-0020: the card art is white or light grey and the instance colour multiplies it ([visual_language.md](visual_language.md) RND-404).

### 4.3 Crawlers (pass 7c)

**RND-319** (MUST) Every in-view Crawler group draws, for each Crawler, a worm strip and a glow strip along its trail samples, with the widths, colours and alphas of [reference/02_units/creatures.md](../../../reference/02_units/creatures.md) R-UNIT-539 and the trail geometry of [animation_and_vfx.md](../animation_and_vfx.md), as `Segment` instances with a colour at each end. Strips of every team except the Rogue faction use the `crawler_trail` entry; the Rogue faction's strips use `crawler_trail_rogue`, whose worm colour has alpha 0 and so darkens like the `Darkening` blend, while its glow (alpha 50 of 255) darkens in proportion to its alpha (RND-058).

**RND-320** (MUST) The worm strip samples the Crawler trail texture, a derived variant of `sprite.crawler.segment` produced by the cooker (ADR-0018): a column `rnd.crawler.trail_column_width` (32 px) wide and `rnd.crawler.trail_column_height` (512 px) tall, cleared to black, holding copies of the segment art stacked from the top, copy *i* (from 0) being (32 − ⌊*i* ÷ 2⌋ × `rnd.crawler.trail_copy_shrink` (1 px)) px square, while the next copy fits in the column; that is 18 copies, 504 px in all. The glow strip samples `fx.glow_round`.

**RND-321** (MUST) The worm and glow strips are drawn with no depth write, no culling and the `bias_crawler` variant (RND-219), so a trail lying on the terrain wins its depth ties against the terrain.

**RND-322** (MUST) A Crawler group's detail level *L*, which R-UNIT-539 maps to the number of drawn samples and to the glow strip, is chosen each frame from the entity-detail setting *E* and the camera distance *d* of the group:

| *E* | *L* = 1 | *L* = 2 | *L* = 3 | *L* = 4 |
|---|---|---|---|---|
| 1 | *d* ≤ `ref.unit.crawler.detail_far` (1000 wu) | *d* > 1000 wu | — | — |
| 2 | *d* ≤ `ref.unit.crawler.detail_near` (500 wu) | 500 < *d* ≤ 1000 wu | *d* > 1000 wu | — |
| 3 | *d* ≤ `rnd.crawler.lod_low_near` (300 wu) | 300 < *d* ≤ `rnd.crawler.lod_low_mid` (600 wu) | 600 < *d* ≤ 1000 wu | *d* > 1000 wu |

A higher level draws fewer samples, and level 4 draws no glow strip.

### 4.4 Actor groups (pass 7a) and other actors (pass 7b)

**RND-323** (MUST) Pass 7a draws, per team, every in-view member of the team's actor groups: the Troopers of its Strike teams, its centipede segments, the Bombers of its airstrike flights and the ants of its Hive groups. Pass 7b draws every other in-view actor of the team, neither Worker nor Crawler, in pool order: first the actors that the current tick has already advanced, then those it has not, with the slice-aware time of RND-082. Both passes draw with fog off.

**RND-324** (MUST) Each actor kind is drawn as the table states. "Object lighting" is RND-228; "special lighting" is RND-229; the tint of each mesh is table 2.12-A of [reference/02_units/README.md](../../../reference/02_units/README.md) (R-UNIT-046) applied by the tint function of RND-271; "flicker" is the damage-flicker draw of R-UNIT-051, which in a frame where the flicker condition holds replaces the body draw by a draw with one axis of the mesh transform scaled ([animation_and_vfx.md](../animation_and_vfx.md)) and the entry given; "shadow" is a blob shadow (RND-325).

| Actor kind (pass) | Body | Further draws, in order | Entries |
|---|---|---|---|
| Trooper (7a) | `mesh.unit.trooper_figure`, object lighting; flicker | — | `trooper_mesh_lit`; flicker `actor_flicker_additive` |
| centipede segment (7a) | while alive and linked into its chain: `mesh.creature.centipede_head_segment` for a head, `mesh.creature.centipede_segment` otherwise, object lighting, scaled uniformly by the segment's size, with the scaled normals of RND-326 | — | `group_mesh_lit` |
| Bomber (7a) | `mesh.unit.bomber_body`, object lighting, at the scale and colour of [reference/02_units/hazards_and_projectiles.md](../../../reference/02_units/hazards_and_projectiles.md) R-UNIT-720 | while armed: `mesh.projectile.thrown_weapon` below it (R-UNIT-720) | `group_mesh_lit` |
| ant (7a in a Hive group; 7b when spawned on its own) | a variant of `mesh.creature.ant_body` (R-UNIT-569), object lighting, scaled uniformly by the ant's scale | — | `group_mesh_lit` (7a), `actor_mesh_lit` (7b) |
| Leader (7b) | `mesh.unit.leader_figure`, object lighting, renormalised normals; flicker | its banner, a cloth flag with `marker.leader_banner` and the pointer `marker.flag_pointer` ([leader_and_formations.md](../../../reference/02_units/leader_and_formations.md) R-UNIT-191), drawn here only for a Leader of another team or while the interface is hidden; the local team's Leader banners are drawn by the world markers (RND-363) | `actor_mesh_lit`; flicker `actor_flicker_additive`; the banner entries of [animation_and_vfx.md](../animation_and_vfx.md) |
| Transport (7b) | `mesh.unit.transport_hull`, object lighting; flicker on its right or front axis | — | `actor_mesh_lit`; flicker `actor_flicker_additive` |
| Builder (7b) | `mesh.unit.builder_body`, object lighting | shadow of half-size `rnd.shadow.blob.builder_size` | `actor_mesh_lit`, `blob_shadow` |
| Collector (7b) | `mesh.unit.collector_body`, object lighting | the bubble `fx.bubble_sphere` as a `CameraFacing` card ([collector.md](../../../reference/02_units/collector.md) R-UNIT-444); while Harvesting, its beam (R-UNIT-444); shadow of half-size `rnd.shadow.blob.collector_size` | `actor_mesh_lit`, `actor_glow`, `actor_beam_alpha`, `blob_shadow` |
| spider (7b) | `mesh.creature.spider_body`, object lighting; flicker with the darkening blend | eight legs, each `mesh.creature.spider_thigh` and `mesh.creature.spider_shin`, object lighting, posed by the procedural legs ([creatures.md](../../../reference/02_units/creatures.md) R-UNIT-580, ADR-0035) | `actor_mesh_lit`; flicker `actor_flicker_darkening` |
| Brood Drifter (7b) | `mesh.creature.brood_drifter_body`, object lighting; flicker | four team-coloured tail strips with the object lighting and their vertex colours (R-UNIT-547); shadow of half-size `ref.unit.brood_drifter.shadow_size` | `actor_mesh_lit`, `actor_flicker_additive`, `actor_strip_lit`, `blob_shadow` |
| Reaper Serpent (7b) | `mesh.creature.reaper_serpent_head` and its `mesh.creature.reaper_serpent_tail` segments, object lighting, renormalised normals | shadows: the head `ref.unit.reaper_serpent.head_shadow`, each tail segment `ref.unit.reaper_serpent.tail_shadow` (R-UNIT-594); its carried Essence orbs as `CameraFacing` quads with no depth test | `actor_mesh_lit`, `blob_shadow`, `actor_glow_nodepth` |
| Seed Pod (7b) | `mesh.creature.seed_pod`, object lighting, renormalised normals | shadow of half-size the pod's size × `ref.unit.seed_pod.shadow_scale` (R-UNIT-521) | `actor_mesh_lit`, `blob_shadow` |
| Crawler egg (7b) | `sprite.crawler_egg.body` as a `CameraFacing` card (R-UNIT-528) | shadow of half-size `ref.unit.crawler_egg.card_size` ÷ 2 | `actor_glow`, `blob_shadow` |
| barrage missile (7b) | before launch: the ground marker `marker.barrage_target` over its blurred shadow (RND-243), with no depth test, and the launcher cards `sprite.missile_barrage.launcher` over `sprite.missile_barrage.launcher_shadow` (R-UNIT-729); in flight: the card `sprite.missile_barrage.missile`, then its trail of `fx.beam_short` | — | marker `actor_ink_nodepth` then `actor_glow_nodepth`; launcher `actor_ink` then `actor_glow`; missile `actor_glow`, Rogue faction `actor_darkening`; trail `actor_beam_additive` |
| meteor (7b) | `mesh.projectile.meteor_rock`, object lighting, renormalised normals | the `fx.starburst` flares of R-UNIT-735, with no depth test | `actor_mesh_lit`, `actor_glow_nodepth` |
| Abductor (7b) | `mesh.creature.abductor_hull`, special lighting | while Abducting or Dropping: its beam rings (R-UNIT-710) | `actor_mesh_lit`, `actor_beam_alpha` |
| bot controller (7b) | nothing in a match; the map editor draws its hook ([map_editor_hooks.md](../../../reference/08_maps_and_level_design/map_editor_hooks.md)) | — | — |

The Builder draws no beam in a match: none of its match states (R-UNIT-453) draws one. The Leader and the meteor are drawn with interpolated shading in the reference look; on meshes whose facets carry one normal and one colour at every corner (ADR-0020) that gives the image of flat shading, so the Species renderer draws them flat like every mesh.

**RND-325** (MUST) A blob shadow of half-size *s* under a caster at *p* is one quad of `fx.glow_round` whose four corners lie at (*p.x* ± *s*, *y*, *p.z* ± *s*), each at *y* = max(terrain height at the corner + `rnd.shadow.blob.lift` (0.9 wu), `rnd.shadow.blob.min_height` (1.0 wu)), in the colour of [reference/02_units/README.md](../../../reference/02_units/README.md) R-UNIT-050, with the `blob_shadow` entry (RND-242). The shadow is skipped when all four corner heights are above *p.y*.

**RND-326** (MUST) A centipede segment is lit with its mesh transform's uniform scale *σ* left in its normals: in the diffuse term of RND-233 its facet normal *n* is replaced by *n* ÷ *σ*, so a larger segment is lit darker and a smaller one brighter before the clamp. Every other scaled actor mesh is lit with unit facet normals (RND-055).

**RND-327** (MUST) An Essence orb whose team is the Rogue faction slot, which no producer creates (R-UNIT-305), would be drawn in `rnd.crowd.rogue_orb_colour` (100, 100, 100, alpha 0) in the first batch of pass 11, which darkens; every other orb is drawn in its team colour, or white with no team, with the sizes, alphas and timing of [essence_orbs.md](../../../reference/02_units/essence_orbs.md) R-UNIT-330 to R-UNIT-332 and the pass-11 drawing of [animation_and_vfx.md](../animation_and_vfx.md).

**RND-328** (MUST NOT) Passes 7a and 7b draw no actor kind that no match creates (see DEC-062): no Rift tentacle, no Vehicle Duel vehicle, no Summoner, no Tri-walker, and no campaign infantry, drop ship or lander. The Leader draws no shield of carried orbs, which no match shows.

### 4.5 Billboards and render utilities

**RND-329** (MUST) Most billboards (particles, Essence orbs, structure lights and ports, glow quads) are `CameraFacing` quads centred on their position. The bottom-anchored billboard is a `CameraFacing` quad with the anchor `BottomCentre`, of a given width and height, whose texture is mapped upright and mirrored left to right; its users are the dust vortex (`fx.glow_cloudy`) and the snowflake (`fx.starburst`) ([animation_and_vfx.md](../animation_and_vfx.md)). Its texture is sampled with linear filtering through all mip levels, and it is drawn with no culling, with the blend and colour its user names.

**RND-330** (MUST) The development sphere (`DevSphereMesh`) is an icosahedron of 12 vertices, built from the constants `rnd.sphere.ico_x` (0.5257311121) and `rnd.sphere.ico_z` (0.85065080835) as the twelve points (±*x*, 0, ±*z*), (0, ±*z*, ±*x*) and (±*z*, ±*x*, 0), with its 20 faces, subdivided `rnd.sphere.subdivisions` (1) time: each face becomes four by its edge midpoints pushed out to the unit sphere, which gives 80 triangles. It is built once, scaled and moved per draw, and drawn either as `rnd.sphere.line_width` (1 px) lines along its triangle edges or filled. Its users are the development drawing of pass 24 ([diagnostics_test_and_performance.md](../diagnostics_test_and_performance.md)) and the sphere hooks of the map editor (RND-174).

### 4.6 Effects (pass 10a)

**RND-331** (MUST) Pass 10a draws every effect object of the match world that passes the point-in-view test of RND-234, in effect-pool order, as one batch with additive blending, no culling and no depth write (`effect_additive`), each effect adding the draws its owner specifies. The Core Device pulse wave, Swarm streaks and lightning strikes are drawn without a view test.

**RND-332** (MUST) The effect kinds of a match, and where their visuals are specified, are:

| Effect kind | Species type | Visuals specified in |
|---|---|---|
| grenade | `GrenadeProjectile` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md), `mesh.projectile.thrown_weapon` |
| airstrike target marker | `AirstrikeTargetMarker` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| airstrike bomb | a thrown weapon (`ThrownWeapon`) | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md), `mesh.projectile.thrown_weapon` |
| turret round | `TurretRound` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md), `mesh.projectile.turret_round` |
| Swarm streak | `SwarmStreak` | [crates_and_powers.md](../../../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| snowflake | `SnowFlake` | [animation_and_vfx.md](../animation_and_vfx.md); the bottom-anchored billboard of RND-329 |
| rocket | `RocketProjectile` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| shockwave ring | `ShockwaveRing` | this document, RND-333 to RND-335 |
| muzzle flash | `MuzzleFlashEffect` | [animation_and_vfx.md](../animation_and_vfx.md) |
| order trail mark | `OrderTrailMark` | [leader_and_formations.md](../../../reference/02_units/leader_and_formations.md) R-UNIT-186 |
| Wisp | `Wisp` | [creatures.md](../../../reference/02_units/creatures.md) R-UNIT-597 |
| Allegiance Scrambler effect | `AllegianceScramblerEffect` | [animation_and_vfx.md](../animation_and_vfx.md) |
| dust vortex | `DustVortex` | [animation_and_vfx.md](../animation_and_vfx.md); the bottom-anchored billboard of RND-329 |
| lightning strike | `LightningStrike` | this document, RND-336 and RND-337 |
| Core Device pulse wave | `CorePulseWave` | [animation_and_vfx.md](../animation_and_vfx.md) |
| firebomb canister | `FirebombCanister` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| shell casing | `ShellCasing` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md), `mesh.projectile.shell_casing` |
| fireball | `FireballProjectile` | [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |

The order of this table carries no meaning. The spawn-network transit beams, the Beam Dish beams and the energy-grid power lines are structure layers of pass 8 ([world_renderers.md](world_renderers.md) RND-169, RND-171, RND-172). The full-screen flash and the camera shake of a shockwave are VFX screen effects (RND-246, [animation_and_vfx.md](../animation_and_vfx.md)).

**RND-333** (MUST) A shockwave ring draws, during its whole life, a ground ring: a triangle-fan disc at its height + `rnd.shockwave.ring_lift` (5 wu), of radius `rnd.shockwave.ring_base_radius` (35 wu) + `rnd.shockwave.ring_growth` (40 wu per s) × (size − *λ*), where size is the ring's size, which is also its life in seconds, and *λ* = its remaining life − the extrapolation time *t* (RND-081). The fan's centre vertex has the colour `rnd.shockwave.centre_colour` (1, 1, 0, alpha 0) and its rim vertices `rnd.shockwave.rim_colour` (1, 0.5, 0.5) with alpha `rnd.shockwave.rim_alpha` (0.6) × *λ* ÷ size, interpolated across each triangle, drawn with `effect_gradient_nodepth` (no depth test).

**RND-334** (MUST) During the first `rnd.shockwave.blast_duration` (1 s) of its life, at camera distance *D*, a shockwave ring also draws its blast: `rnd.shockwave.blast_layers` (5) stacked `CameraFacing` quads of `fx.starburst` whose first half-size is size × `rnd.shockwave.blast_star_scale` (500) ÷ √*D* and each further one × `rnd.shockwave.blast_layer_growth` (1.2), in `rnd.shockwave.blast_colour` (1, 0.5, 0.2) with alpha 1 − age in seconds; then 5 stacked quads of `fx.glow_round` whose first half-size is size × `rnd.shockwave.blast_glow_scale` (1000) ÷ √*D*, growing likewise, in the same colour, all with `effect_additive`.

**RND-335** (MUST) The ring's damaging radius is a simulation rule ([weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md)) and the drawn ring of RND-333 never feeds it; the drawn ring starts at radius 35 wu when the ring is created.

**RND-336** (MUST) A lightning strike that is not striking draws its cloud glow: one quad of `fx.glow_round` at its position with the half-size `rnd.lightning.cloud_size` (200 wu), right axis = the camera right vector and up axis = (0, 1, 0) × right, with alpha |sin(`rnd.lightning.cloud_pulse_rate` (2 rad per s) × *g*)|, *g* the presentation frame time.

**RND-337** (MUST) A striking lightning strike draws `rnd.lightning.bolt_layers` (3) `UprightCard` quads of `fx.lightning_strip` from its position down to the terrain under it (height = its y − the terrain height there), the first `rnd.lightning.bolt_width` (30 wu) wide and each further one × `rnd.lightning.bolt_width_growth` (1.2) wider, the first with alpha |sin(*λ* − *t*)|, where *λ* is the strike's remaining strike life in seconds and *t* the extrapolation time, and each further one with the previous alpha × `rnd.lightning.bolt_alpha_falloff` (0.5), all with `effect_additive`. The strike timing and its ground effects are simulation rules ([reference/08_maps_and_level_design/rules_and_conventions.md](../../../reference/08_maps_and_level_design/rules_and_conventions.md)).

### 4.7 Laser bolts (pass 10b)

**RND-338** (MUST) Pass 10b draws every laser bolt of the match world as one instanced draw with the `laser_bolt` entry: additive, no culling, no depth write and the `bias_laser` variant (RND-219). Each bolt is drawn as `rnd.laser.layers` (5) identical `Segment` instances, so its additive brightness is five times one layer's before the clamp of RND-058.

**RND-339** (MUST) A bolt's segment runs from its drawn position *p* (RND-081) to *p* + normalise(velocity) × `rnd.laser.length` (10 wu), with the half-width `rnd.laser.half_width` (0.8 wu) perpendicular to the camera ray (RND-306), in the team colour of the bolt's source team, or `rnd.laser.rogue_colour` (255, 255, 255) for the Rogue faction. A Convert bolt doubles the half-width (`rnd.laser.convert_width_factor` (2)) and samples `fx.convert_beam`; every other bolt samples `fx.beam_short`. Both textures are read through a view of mip level 0 only, with linear clamp-to-edge filtering, *u* along the bolt and *v* across it.

**RND-340** (MUST) A laser bolt that passes near the camera requests a camera shake through the VFX screen effects, with the distance and strength that [animation_and_vfx.md](../animation_and_vfx.md) specifies (RND-246); the request is presentation-only and local, and the renderer issues it while it writes the bolt's instances, never from the simulation.

**RND-341** (MUST) The laser-bolt lifetime, its ricochet and its hits are simulation rules ([reference/02_units/README.md](../../../reference/02_units/README.md) R-UNIT-052, [weapons_and_projectiles.md](../../../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md)); the drawing reads them and never feeds them.
