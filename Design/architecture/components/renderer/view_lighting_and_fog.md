# View, lighting and fog

> Status: Engine baseline · Prefix: RND · ADRs: [ADR-003](../../../ADR/ADR-003-shader-language-compiler-and-material-shaders.md) · [ADR-005](../../../ADR/ADR-005-pipeline-state-catalogue.md) · [ADR-009](../../../ADR/ADR-009-instanced-crowds-sprite-cards-and-shadows.md) · [ADR-012](../../../ADR/ADR-012-swap-chain-presentation-and-frame-pacing.md) · [ADR-013](../../../ADR/ADR-013-working-colour-space-and-blending.md) · [ADR-015](../../../ADR/ADR-015-scene-anti-aliasing.md) · [ADR-016](../../../ADR/ADR-016-view-projection-and-depth-conventions.md) · [ADR-036](../../../ADR/ADR-036-effect-events-and-presentation-clock.md) · [ADR-064](../../../ADR/ADR-064-accessibility-baseline-xag.md) · Related: [Renderer](README.md) · [Frame graph and passes](frame_graph_and_passes.md) · [World renderers](world_renderers.md) · [Shaders](shaders.md) · [Camera and sequences](../camera_and_sequences.md)

This document specifies how the renderer turns the camera's view parameters into a projection and a view, how coplanar layers are offset in depth, which view tests decide what is submitted, and the fog, lighting and shadow model of the reference look.

## 1 Responsibilities and boundaries

**RND-208** (MUST) The renderer builds the projection, the view, the depth-bias variants, the fog and the two-light lighting of every frame from the camera's view parameters and the map's lights. It MUST NOT move the camera, choose its field-of-view values or change map lights; those belong to the camera component ([camera_and_sequences.md](../camera_and_sequences.md), see DEC-024) and to the map data (RND-013).

**RND-209** (MUST) Lighting and fog are presentation: no simulation rule reads a light, a fog value or a shadow (R-SIM-640).

## 2 Interfaces

```cpp
namespace Engine
{

struct FrameView // built in pass 1
{
  Mat4 view; // row-major, row vectors (ADR-016)
  Mat4 projection;
  Mat4 viewProjection;
  Vec3 position;
  Vec3 front;
  Vec3 up;
  float fovYRadians;
  float nearPlane;
  float farPlane;
  std::uint32_t width; // render size in pixels
  std::uint32_t height;
};

struct DirectionalLight
{
  Vec3 towardsLight;
  Vec3 color; // also the specular colour
};
struct LightRig
{
  std::array<DirectionalLight, 2> lights; // a missing light is black
};

// In the shared header FrameConstants.h, with its type names (shaders.md RND-262).
struct LitMaterial
{
  Float3 specular; // diffuse and ambient follow the facet colour
  float shininess;
};

[[nodiscard]] bool PointInView(const FrameView& _view, const Vec3& _point);                  // RND-234
[[nodiscard]] bool SphereInView(const FrameView& _view, const Vec3& _center, float _radius); // RND-234

} // namespace Engine
```

## 3 Data model

| Data | Content | Source |
|---|---|---|
| frame view | matrices, camera vectors, field of view, planes, render size | the camera component and RND-210 to RND-213 |
| map light rig | the map's first two lights, direction towards the light and colour (RND-226) | map data (RND-013) |
| special light rig | one light and its material (RND-229) | this document |
| level fog | colour, start and end (RND-222) | this document and the terrain extent |
| per-frame constants | the frame view, the active light rig, the level fog | README RND-014 |

## 4 Behaviour and algorithms

### 4.1 Projection and view

**RND-210** (MUST) The projection is a perspective projection with the camera's vertical field of view and an aspect ratio of render width ÷ render height, so the horizontal field of view follows the aspect ratio (Hor+). It uses the conventions of ADR-016: right-handed world and view space with +y up, the camera looking along −z in view space, reversed Z (the near plane maps to depth 1 and the far plane to depth 0), a finite far plane, and row-major matrices used with row vectors.

**RND-211** (MUST) While a match world exists, the near and far planes are set at the start of every frame to `rnd.view.near_plane` (5 wu) and `rnd.view.far_plane` (15000 wu). Outside a match (the front end and the loading view) they are `rnd.view.menu_near_plane` (50 wu) and `rnd.view.menu_far_plane` (10000000 wu).

> **Amendment 2026-10-18 (WLD-001; MVP-010).** In the world game the match planes apply while a world is drawn and the front-end planes outside it. The far plane of 15,000 wu holds for the MVP's one region of 5,120 wu: a camera above the region at the settlement view's highest height (5,000 wu, R-CTRL-461) is at most 8,800 wu from any point of it. The strategic view (WLD-240, up to 46,080 wu high) and worlds of more than one region need a far plane of their own, and so does the fog end of RND-222, which a world extent of 71,680 wu (WLD-001) would put at 72,680 wu; both are DEC-247's, with the camera-relative positions of WLD-246.

> **Amendment 2026-10-19 (O59).** The fog end of RND-222 no longer grows with the world, so nothing is visible beyond 6,120 wu from the camera in a world of any size and the settlement view's far plane of 15,000 wu holds; only the strategic view's planes and fog remain DEC-247's.

**RND-212** (MUST) Before every projection set-up, the camera's vertical field of view is clamped to [`rnd.view.fov_min` (1°), `rnd.view.fov_max` (180°)]. The field-of-view values themselves (the free camera, its zoom, turret aiming and scripted views) are the camera component's; the main-menu value is RND-077.

**RND-213** (MUST) The view matrix looks from the camera position towards position + front, with the camera's up vector.

**RND-214** (MUST) In a poster tile (RND-111), with *N* tiles per side, tile index *i* and the vertical field of view *φ* in degrees, the scale is *k* = *φ* ÷ `rnd.view.poster_fov_divisor` (78). The whole poster frustum spans, at the near distance *n*, the horizontal range [−*k*·*n*, *k*·*n*] and the vertical range [−(*H* ÷ *W*)·*k*·*n*, (*H* ÷ *W*)·*k*·*n*], with *W* × *H* the render size; tile *i* is the off-centre perspective frustum of width 2·*k*·*n* ÷ *N* and height 2·(*H* ÷ *W*)·*k*·*n* ÷ *N* at column *i* mod *N* and row ⌊*i* ÷ *N*⌋, counting rows from the bottom, with the frame's near and far planes. The poster's field of view is therefore set by *φ* ÷ 78 rather than by the on-screen projection (see DEC-958 item 2).

**RND-215** (MUST) The 2-D overlay space is in pixels of the target it draws into, with the origin at the top-left corner and *y* growing downwards. Glyph placement within it is the UI framework's (ADR-034).

**RND-216** (MUST) `ProjectToScreen(p)` multiplies the world point by the frame's cached view-projection matrix; if the resulting *w* is at or below 0 it returns no position; otherwise it divides by *w* and maps the result to pixels of the 2-D overlay space, returning *w* as the third component.

**RND-217** (MUST) `ScissorFromWorld(a, b)` projects both points with `ProjectToScreen` and returns the scissor rectangle that spans them; drawing with it restores the full-target scissor afterwards.

**RND-218** (MUST) Screen-to-world rays for the cursor, selection and the camera are the camera's `PickRayAt` ([camera](../camera_and_sequences.md) CAM-090, CAM-093), built from the inverse of the frame's view-projection matrix with reversed-Z depth values, never from GPU state (ADR-016); the renderer has no ray helper of its own.

### 4.2 Depth-bias variants

**RND-219** (MUST) Draws that must win depth ties against coplanar or nearly coplanar surfaces use a depth-biased catalogue variant (ADR-005, ADR-016), never a change of the projection. Each variant has a bias factor *k* above 1, which sets how far its fragments move towards the camera (RND-220):

| Variant | Factor *k* | Used by |
|---|---|---|
| `bias_cursor` | `ref.ctrl.cursor.depth_bias` (1.03) of [cursor_and_markers.md](../../../reference/06_controls_camera_and_commands/cursor_and_markers.md) (R-CTRL-363, R-CTRL-374) | the world cursor's highlight rings and formation preview (pass 20), the ground highlights of order markers (pass 16) |
| `bias_crawler` | `rnd.depth_bias.crawler` (1.05) | Crawler trails (pass 7c) |
| `bias_blob_shadow` | `rnd.depth_bias.blob_shadow` (1.05) | blob shadows (RND-242) |
| `bias_laser` | `rnd.depth_bias.laser` (1.2) | laser bolts (pass 10b) and rockets |

**RND-220** (MUST) A variant with factor *k* sets `DepthBias` = round((*k* − 1) × `rnd.depth_bias.unit_scale` (2²⁴)), `SlopeScaledDepthBias` = 0 and `DepthBiasClamp` = 0. On a floating-point depth target the bias is `DepthBias` × 2^(*e* − 23), with *e* the exponent of the primitive's largest depth (the nearest point under reversed Z), so on the `D32_FLOAT` target this adds, towards the camera, between (*k* − 1) and 2 × (*k* − 1) times the depth of the primitive's nearest point: the draw wins depth ties against surfaces drawn without bias and loses against geometry clearly in front of it (T5).

**RND-221** (MUST) Workers and their ground shadows use no depth bias: the shadows lie slightly above the terrain instead ([README.md](../animation_and_vfx/README.md)).

### 4.3 Fog

**RND-222** (MUST) Level fog is linear on the fog coordinate *c*, the view-space depth of the pixel (its distance along the view axis), evaluated per pixel (ADR-003). With the fog end *e* = max(`rnd.fog.min_end` (4000 wu), *X* + `rnd.fog.edge_margin` (1000 wu), *Z* + `rnd.fog.edge_margin`), where *X* and *Z* are the world's extents, and the fog start `rnd.fog.start_fraction` (1/3) × *e*, the fogged colour is lerp(fog colour, colour, *f*) with *f* = clamp((*e* − *c*) ÷ (*e* − start), 0, 1). Fog changes the colour only, never the alpha, and is applied before blending. The fog colour is the background colour `rnd.frame.clear_colour` (black), or `rnd.frame.clear_colour_inverted` (white) in the inverted style (see DEC-040). Because the background is black, distant terrain and structures fade to black: objects emerge from darkness.

> **Amendment 2026-10-19 (O59).** In the world game *X* and *Z* are the side of one region, `wld.region.side` (5,120 wu), whatever the world's extent, so the fog end is *e* = 6,120 wu from the camera at every world size: distant terrain always fades to black, and the MVP's valley looks as the rule gives it today. The strategic view sets its own fog when it is designed (DEC-247).

**RND-223** (MUST) Level fog is enabled for the terrain (pass 4), the flat sea layer and the waves (passes 5a and 5b) and the structures (passes 6 and 8).

**RND-224** (MUST) The sky uses its own black fog with the per-layer ranges of [world_renderers.md](world_renderers.md) RND-151 and RND-153.

**RND-225** (MUST) No fog is applied to the actors (passes 7a to 7d), effects, laser bolts, Essence orbs, route lines, shatter debris, particles, UI or any overlay pass.

### 4.4 Lighting

**RND-226** (MUST) The per-frame constants carry two directional lights: the map's first two lights (RND-013), each with a unit direction towards the light and a colour that serves as both its diffuse and its specular colour. A map with one light leaves the second light black, with the direction `rnd.lighting.unused_light_direction` (0, 0, −1); lights beyond the second are not used. There is no ambient term. Light colours above 1, up to 2.0 per channel (ADR-003), over-brighten facets.

**RND-227** (MUST) During camera turns the map lights lag the view by one frame (see DEC-325): the world-space direction passed for frame *j* is *R*ⱼ⁻¹ × *R*ⱼ₋₁ × the light's direction, where *R* is the rotation part of a frame's view matrix and *R*₋₁ of the renderer's first frame is the identity. With a still camera this is the light's own direction. Under option B of DEC-325 the light's own direction is passed, so the lights set for a frame are used in that frame.

**RND-228** (MUST) The object lighting, used by lit meshes unless RND-229 applies, is the per-facet diffuse term of RND-233 for both map lights, with the facet colour as the diffuse colour and no specular term: lit meshes are diffuse only.

**RND-229** (MUST) The special lighting replaces the map light rig always for Relics and for the Abductor, and for every structure of pass 6 when the special-lighting setting is on ([options_and_budgets.md](options_and_budgets.md)), in every Launch Race match, and on maps whose map effects include special lighting ([reference/08_maps_and_level_design/rules_and_conventions.md](../../../reference/08_maps_and_level_design/rules_and_conventions.md)). It is one light with the direction Normalize(`rnd.lighting.special_direction` (0, 1, 0.5)) and the colour `rnd.lighting.special_colour` (2.0, 1.5, 0.75) as diffuse and specular, no ambient term, and the second light black; the material's diffuse colour is the facet colour, its specular colour `rnd.lighting.special_specular` (1, 1, 1) and its shininess `rnd.lighting.special_shininess` (10). The special light uses the view of its own frame, so it never lags.

**RND-230** (MUST) The terrain uses two materials in its single pass ([world_renderers.md](world_renderers.md) RND-136): the surface material (diffuse colour the facet colour, no specular term) and the grid material (diffuse colour the facet colour, specular `rnd.terrain.overlay_specular`, shininess `rnd.terrain.overlay_shininess`).

**RND-231** (MUST) While the match's seasonal state is active, the colours of both map lights are `rnd.light.seasonal_colour` (RND-013).

**RND-232** (MUST) The sea, the sky, sprites, particles, effects and the UI are unlit.

**RND-233** (MUST) A lit facet with unit facet normal *n*, facet colour *D* (with alpha), material specular colour *S* and shininess *p* gets, for each enabled light *i* with direction *l*ᵢ and colour *C*ᵢ:

- diffuse *d*ᵢ = max(*n* · *l*ᵢ, 0) × *C*ᵢ × *D*.rgb;
- specular *s*ᵢ = max(*n* · *h*ᵢ, 0)^*p* × *C*ᵢ × *S* when *n* · *l*ᵢ > 0, and 0 otherwise, with *h*ᵢ = Normalize(*l*ᵢ + *v*), where *v* is the unit vector from the scene towards the camera along the view axis (the negated camera front: the viewer is at an infinite distance).

The lit colour is clamp(Σᵢ (*d*ᵢ + *s*ᵢ), 0, 1) with alpha *D*.a. A texture, where the material has one, then multiplies the lit colour and alpha; fog is applied next; blending last. Lighting is evaluated on encoded colour values, as ADR-013 specifies.

### 4.5 View tests

**RND-234** (MUST) The CPU view tests that decide which objects are submitted are:

- point in view: dot(Normalize(*p* − camera position), camera front) ≥ cos(*φ*) − `rnd.frustum.point_tolerance` (0.2), a cone test with the full vertical field of view *φ*, not its half, and therefore generous;
- sphere in view: true when the camera is inside the sphere; otherwise true when acos(dot(Normalize(*c* − camera position), camera front)) − asin(*r* ÷ |*c* − camera position|) ≤ *θ* ÷ 2, where *θ* is the diagonal field of view: with the render size *W* × *H* in pixels, *a* = (*H* ÷ 2) ÷ sin(*φ* ÷ 2), *b* = √(*a*² + (*W* ÷ 2)²), *q* = √(*W*² + *H*²) ÷ 2 and *θ* = 2 × asin(*q* ÷ *b*).

Neither test rejects beyond the far plane; the far plane and the fog end bound what is seen.

**RND-235** (MUST) Each object kind decides its visibility with its own view test, usually the sphere test with its bounding sphere; owner documents name the exceptions, such as effects drawn without a view test (pass 10a) and Barrier Emitters during the grace period (RND-196).

**RND-236** (MUST) GPU culling of instances and terrain chunks (ADR-009, ADR-011) is conservative: it never culls an object whose bounds intersect the view frustum, so it changes no image compared with the CPU view tests.

### 4.6 Shadows

**RND-240** (MUST) The look has no shadow maps, no stencil shadows and no projected-geometry shadows. It has three kinds of decal shadow, each drawn as instanced quads that follow the terrain through `terrain_heights` (ADR-009). Adding shadow maps is a change of the look and needs an ADR.

**RND-241** (MUST) A **sprite ground shadow** is the actor's own sprite texture drawn as a black, flat quad lying on the terrain, offset along +*z* as if lit from a fixed direction, without depth write, with an alpha set by the actor's detail factor, skipped beyond a distance and while the actor is dead or abducted. The Worker's values are in [README.md](../animation_and_vfx/README.md) and its pass is 7d.

**RND-242** (MUST) A **blob shadow** is a quad of half-size *s* with `fx.glow_round`, drawn with the `Darkening` blend, no culling, no depth write and the `bias_blob_shadow` variant, following the terrain slightly above it and skipped when all its corners lie above the caster. Its colour is `ref.unit.shadow.colour` (0.6, 0.6, 0.6, alpha 0) of [reference/02_units/README.md](../../../reference/02_units/README.md) R-UNIT-050; each corner lies `rnd.shadow.blob.lift` (0.9 wu) above the terrain and at least at `rnd.shadow.blob.min_height` (1.0 wu); the per-actor half-sizes are the shadow column of [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md) RND-324, and the drawing is RND-325. Its users are Crawler eggs, Brood Drifters, Builders, Reaper Serpents (head and tail segments), Collectors and Spitter Plants.

**RND-243** (MUST) A **blurred-icon shadow** is a blurred copy of an icon, produced offline by the cooker as a derived texture variant (ADR-018) with the five-tap kernel of [asset_pipeline_and_resources.md](../asset_pipeline_and_resources.md) AST-073, whose weights the blur gain `rnd.shadow.icon_blur_radius` (10) multiplies; the kernel reaches ±2 px. It is drawn first with the `Ink` blend and source alpha 0, which darkens, and then followed by the icon drawn additively. The reference documents give the same value for each user's shadow blur (`ref.ctrl.cursor.shadow_blur`, `ref.crate.render.landing_marker_blur`, `ref.hazard.missile_barrage.marker_shadow_blur`). Its users are the shadowed cursors of the world cursor (pass 20; `cursor.world.shadow`, `cursor.highlight.shadow`, `cursor.place.shadow`, `cursor.unavailable.shadow`; R-CTRL-350), the Supply crate landing marker (`marker.crate_landing.shadow`; [crates_and_powers.md](../../../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-140) and the Missile Barrage ground marker (`marker.barrage_target.shadow`; [hazards_and_projectiles.md](../../../reference/02_units/hazards_and_projectiles.md) R-UNIT-729), as [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md) RND-324 and RND-366 draw them. The Missile Barrage launcher's shadow is not a derived variant: it is the authored darkening card `sprite.missile_barrage.launcher_shadow` (R-UNIT-729).

**RND-244** (MUST) Shadow quads are drawn unsorted in their owner's pass; darkening shadows are order-independent (RND-058).

### 4.7 Screen-space effects

**RND-246** (MUST) The look uses exactly these screen-space effects:

| Effect | Mechanism | Specified in |
|---|---|---|
| fade to and from black | a full-screen quad in pass 27 | [frame_graph_and_passes.md](frame_graph_and_passes.md) RND-085 |
| screen transition | the captured-frame slide in pass 34 | [frame_graph_and_passes.md](frame_graph_and_passes.md) RND-095 |
| shockwave flash | a full-screen additive white quad at the start of a shockwave | [README.md](../animation_and_vfx/README.md), ADR-036 |
| camera shake | an additive rotation offset on the view, requested by laser bolts that pass near the camera and by shockwaves; local only | [README.md](../animation_and_vfx/README.md), ADR-036 |
| brightness | a constant in the final pass; the system gamma ramp is never changed | ADR-012 |

**RND-247** (MUST NOT) The look has no bloom, no blur, no post-process anti-aliasing and no screen-space distortion. Multisampling follows ADR-015, and its Off setting is the reference appearance of polygon edges.

**RND-248** (MUST) Flash intensity and HDR highlight levels are capped by the photosensitivity rule, and camera shake is scaled by the player's motion setting (ADR-036, ADR-064), with the limits of [particles_shatter_and_screen_effects.md](../animation_and_vfx/particles_shatter_and_screen_effects.md) VFX-232 and VFX-235 (see DEC-964).

## 5 Concurrency and performance budget

**RND-250** (MUST) The frame view, the light rigs and the fog parameters are computed once per frame on the frame thread before pass 0 records; view tests run on the CPU for submission and on the GPU for instance culling. Their costs are part of the budgets in [options_and_budgets.md](options_and_budgets.md).

## 6 Failure modes, diagnostics and telemetry

| Failure | Behaviour | Requirement |
|---|---|---|
| the camera supplies a degenerate front or up vector | the previous frame's view is kept, and a warning is logged in Debug builds | RND-213 |
| a map has no lights | a content-build warning; at run time both lights are black, so facets lit by the map rig render black | RND-226 |
| a light direction has zero length | it is normalised to (0, 0, 1) | RND-012 |

A development view of Debug builds shows the light directions, the fog range and the culling counts of the frame.

## 7 Acceptance tests

The golden-image tests compare with the default tolerance and on the pinned reference configuration of the test harness ([diagnostics, test and performance](../diagnostics_test_and_performance/README.md) §4.8), unless a test states its own tolerance.

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 projection | unit | near maps to depth 1 and far to 0; the vertical field of view is kept at 4:3, 16:9 and 21:9 | RND-210, RND-211 |
| T2 clamp | unit | fields of view of 0.5° and 200° are clamped to 1° and 180° | RND-212 |
| T3 poster tiles | unit | the union of the *N* × *N* tile frusta equals the poster frustum of RND-214, tile 0 bottom-left | RND-214 |
| T4 projection helper | unit | known points map to known pixels; a point behind the camera returns no position | RND-216 |
| T5 depth bias | golden-image | a decal and a laser bolt coplanar with terrain win the depth tie at 50, 500 and 5000 wu, and lose against geometry 1 wu in front of them | RND-219, RND-220 |
| T6 fog ramp | golden-image | a flat terrain strip from 0 to 8000 wu reproduces the linear ramp to black between *e* ÷ 3 and *e* | RND-222, RND-223 |
| T7 lighting | golden-image | a reference two-light rig, (0.07, 0.35, 0.93) with colour (1, 1, 1) and (−0.04, 0.43, 0.90) with colour (2.0, 1.0, 0.4); over-bright lights (colour 2.0), one-light maps, the special rig with specular highlights and the grid specular term | RND-226, RND-228 to RND-230, RND-233 |
| T8 light lag | unit | after a 90° camera turn between two frames, the passed direction equals *R*ⱼ⁻¹ *R*ⱼ₋₁ × the light direction; with option B it equals the light direction | RND-227 |
| T9 view tests | property | the cone and sphere tests of RND-234 against recorded camera paths; the GPU cull never rejects an object that intersects the frustum | RND-234 to RND-236 |
| T10 shadows | golden-image | both decal kinds on flat and sloped terrain, and an icon shadow under its icon | RND-240 to RND-243 |

## 8 Open decisions

| Reference | Status | Where it applies |
|---|---|---|
| DEC-024 | Resolved | camera field-of-view values are the camera component's (RND-212) |
| DEC-040 | Open | the inverted style's fog colour (RND-222) |
| DEC-325 | Open | the one-frame light lag (RND-227) |
| DEC-562 | Open | map lighting presets (RND-226, RND-229) |
| DEC-958 | Open | the poster field of view of *φ* ÷ 78 (RND-214) |
| DEC-964 | Open | the accessibility limits for camera shake and flashes (RND-248) |
| ADR-016 | Proposed | reversed Z, row-major matrices and the depth-bias rule this document follows |

**Design-phase inputs.** The far plane, the fog end and the sky ranges for a world larger than an arena map (DEC-247); whether lights become named presets (DEC-562); whether Species adds shadow maps for its strategic zoom, which needs a new ADR (RND-240).

## 9 Tuning table

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `rnd.view.near_plane` | 5 | wu | in a match world, set every frame | RND-211 | |
| `rnd.view.far_plane` | 15000 | wu | in a match world | RND-211 | DEC-247 |
| `rnd.view.menu_near_plane` | 50 | wu | outside a match | RND-211 | |
| `rnd.view.menu_far_plane` | 10000000 | wu | outside a match | RND-211 | |
| `rnd.view.fov_min` | 1 | degrees | clamp of the vertical field of view | RND-212 | |
| `rnd.view.fov_max` | 180 | degrees | | RND-212 | |
| `rnd.view.poster_fov_divisor` | 78 | degrees | poster scale *k* = *φ* ÷ 78 | RND-214 | |
| `rnd.depth_bias.crawler` | 1.05 | bias factor *k* | | RND-219 | |
| `rnd.depth_bias.blob_shadow` | 1.05 | bias factor *k* | | RND-219, RND-242 | |
| `rnd.depth_bias.laser` | 1.2 | bias factor *k* | laser bolts and rockets | RND-219 | |
| `rnd.depth_bias.unit_scale` | 16777216 | 2²⁴ | converts a factor into `DepthBias` for `D32_FLOAT` | RND-220 | |
| `rnd.fog.min_end` | 4000 | wu, view depth | smallest fog end | RND-222 | DEC-247 |
| `rnd.fog.edge_margin` | 1000 | wu | added to each world extent | RND-222 | |
| `rnd.fog.start_fraction` | 1/3 | × fog end | fog start | RND-222 | |
| `rnd.lighting.unused_light_direction` | 0, 0, −1 | unit vector | direction of a black second light | RND-226 | |
| `rnd.lighting.special_direction` | 0, 1, 0.5 | vector, normalised at use | towards the light | RND-229 | |
| `rnd.lighting.special_colour` | 2.0, 1.5, 0.75 | RGB multiplier | diffuse and specular | RND-229 | |
| `rnd.lighting.special_specular` | 1, 1, 1 | per channel | material specular | RND-229 | |
| `rnd.lighting.special_shininess` | 10 | exponent | | RND-229 | |
| `rnd.frustum.point_tolerance` | 0.2 | cosine | point-in-view cone tolerance | RND-234 | |
| `rnd.shadow.icon_blur_radius` | 10 | × kernel weight | the blur gain of the derived shadow variants of icons: it multiplies the five-tap weights of AST-073, whose taps reach ±2 px; not a radius in pixels | RND-243, RND-422 | |
