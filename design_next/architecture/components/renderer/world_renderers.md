# World renderers

> Status: Engine baseline · Prefix: RND · ADRs: [ADR-009](../../../ADR/ADR-009-instanced-crowds-sprite-cards-and-shadows.md) · [ADR-010](../../../ADR/ADR-010-gpu-particles-and-shatter-debris.md) · [ADR-011](../../../ADR/ADR-011-world-surface-rendering.md) · [ADR-013](../../../ADR/ADR-013-working-colour-space-and-blending.md) · [ADR-018](../../../ADR/ADR-018-texture-import-manifest.md) · [ADR-019](../../../ADR/ADR-019-mesh-source-format-and-cooked-meshes.md) · [ADR-020](../../../ADR/ADR-020-flat-shading-and-team-tint.md) · [ADR-021](../../../ADR/ADR-021-mesh-socket-schema.md) · Related: [Renderer](README.md) · [Frame graph and passes](frame_graph_and_passes.md) · [View, lighting and fog](view_lighting_and_fog.md) · [Shaders](shaders.md) · [Terrain](../terrain/README.md)

This document specifies the renderers of the static world: the terrain (`TerrainRenderer`), the sea (`SeaRenderer`), the sky and clouds (`SkyRenderer`), meshes and their team tint (`MeshAsset`), structures in their solid and alpha passes, trees (`TreeObject`) and Barrier Emitters (`BarrierEmitter`). Actors, crowds, effects and overlays are in [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md).

## 1 Responsibilities and boundaries

**RND-130** (MUST) The world renderers record passes 3, 4, 5a, 5b, 6 and 8 of the ordered pass table ([frame_graph_and_passes.md](frame_graph_and_passes.md) §4.3) with the catalogue entries listed there, and nothing else.

**RND-131** (MUST) The world renderers read the terrain component's data ([terrain](../terrain/README.md)) and never change it: the height field, the outside height and the cell size; the per-triangle colour-ramp look-up; the sea shore mask, depth map and flat-water tile mask; the coastline wave formulae. The terrain colours, the sea and the sky are presentation only and never feed the simulation (R-SIM-640).

## 2 Interfaces

The interface below is a C++23 declaration sketch, not an implementation. The terrain, sea and sky renderers, the tree generator and the mesh batch (`MeshRenderer`) are engine code in namespace `Engine`. The structure renderer reads the game's presentation snapshot and knows game concepts by name, so it is game-layer presentation code in `Species`, built on the mesh batch (`AGENTS.md` R9).

```cpp
namespace Engine
{

class TerrainRenderer
{
public:
  void Build(const TerrainField& _field, const TerrainColorRamp& _ramp); // RND-132 to RND-134
  void RecordTerrainPass(PassContext& _pass) const;                      // pass 4
};

class SeaRenderer
{
public:
  void Build(const TerrainField& _field, const SeaMasks& _masks, const SeaLook& _look); // RND-140 to RND-144
  void RecordWaveUpdate(PassContext& _pass, double _smoothPresentationTime) const;      // RND-145
  void RecordFlatLayer(PassContext& _pass) const;                                       // pass 5a
  void RecordWaves(PassContext& _pass) const;                                           // pass 5b
};

class SkyRenderer
{
public:
  void AdvanceTick(double _gameTimeStep);                                                 // RND-152, slice ref.sim.slice.sky
  void Record(PassContext& _pass, double _skyExtrapolationTime, SkyDetail _detail) const; // pass 3
};

struct TreeGeometry // RND-180 to RND-184, unit space, trunk (0,0,0)-(0,1,0)
{
  std::vector<TreeQuad> branchQuads;
  std::vector<TreeQuad> leafQuads;
};
TreeGeometry GenerateTreeGeometry(const TreeParameters& _tree, std::uint32_t _leafAlphaBoostLevel);

class MeshRenderer // lit, tinted mesh instances (ADR-009, ADR-020): the mesh batch that the game's renderers fill
{
public:
  std::span<MeshInstance> BeginInstances(const MeshHandle& _mesh, CatalogKey _entry, std::uint32_t _count); // upload ring
  void RecordDraws(PassContext& _pass, std::span<const CatalogKey> _entries) const;
};

} // namespace Engine

namespace Species
{

class StructureRenderer // game-layer presentation code (AGENTS.md R9), built on Engine::MeshRenderer
{
public:
  void RecordSolid(Engine::PassContext& _pass, const PresentationSnapshot& _snapshot, double _t) const; // pass 6
  void RecordAlpha(Engine::PassContext& _pass, const PresentationSnapshot& _snapshot, double _t) const; // pass 8
};

} // namespace Species
```

## 3 Data model

| Data | Content | Lifetime |
|---|---|---|
| terrain chunk | vertices with position, facet colour, facet normal, grid coordinates (one repeat per height-field cell) and barycentric coordinates; indices; a bounding box | the match; rebuilt after device removal |
| sea flat layer | one quad per set flag of the flat-water tile mask; the sea tile and the shore mask textures | the match |
| wave mesh | the vertex pairs of the coastline wave mesh in build order, each with its depth, shore noise and the index of its predecessor pair; a per-frame vertex buffer written by the wave update | static part: the match; dynamic part: one frame in flight |
| sky instance list | per layer: geometry, texture view, sampler, colour, UV transform and fog range | the renderer |
| sky state | the cloud offset and velocity (RND-152) | the match world, or the front end for the menu sky |
| mesh instance data | per instance: transform, palette index, tint flags, per-node transforms | one frame in flight (ADR-009) |
| tree geometry | branch and leaf quads per tree, in unit space | the match |

## 4 Behaviour and algorithms

### 4.1 Terrain

**RND-132** (MUST) The terrain mesh is built from the height field *h*(*x*, *z*) of [terrain](../terrain/README.md) (TER-012) with *N*ₓ sample columns, *N*_z sample rows and cell size *cs*; a sample read at a column or row index outside the grid returns the outside height. For every row *z* from 0 to *N*_z − 1 and column *x* from 0 to *N*ₓ − 1 there is a vertex pair *v1* = (*x*·*cs*, *h*(*x*, *z*), *z*·*cs*) and *v2* = (*x*·*cs*, *h*(*x*, *z* + 1), (*z* + 1)·*cs*); the last row's *v2* therefore lies at the outside height. A pair is **needed** when any of *h*(*x* − 1, *z*), *h*(*x* − 1, *z* + 1), *h*(*x*, *z*), *h*(*x*, *z* + 1), *h*(*x* + 1, *z*), *h*(*x* + 1, *z* + 1) is above 0. Taking the pairs in row-major order (columns within a row, then the next row), every two consecutive pairs *P* = (*p1*, *p2*) and *Q* = (*q1*, *q2*) that are both needed form the two triangles (*p1*, *p2*, *q1*) and (*q1*, *p2*, *q2*); within a row, where *P* is at column *x* and *Q* at column *x* + 1, both are counter-clockwise seen from above and their shared diagonal runs from (*x*, *z* + 1) to (*x* + 1, *z*). A pair that is not needed ends the run, so no triangle touches it. When *P* is the last pair of a row and *Q* the first pair of the next row, the two triangles span the map from one edge to the other; every reference map has sea along its whole border, so this never happens on the reference maps.

**RND-133** (MUST) Before facet normals and colours are computed, every vertex whose height is below `rnd.terrain.sink_threshold` (0.3) is moved to the height `rnd.terrain.sink_height` (−10), so that the coast is a crisp cliff into the sea and never meets the sea surface at the same depth.

**RND-134** (MUST) Each triangle has one facet normal and one facet colour (ADR-020). The facet normal of (*p1*, *p2*, *q1*) is normalise((*q1* − *p2*) × (*p1* − *p2*)); the facet normal of (*q1*, *p2*, *q2*) is normalise((*q2* − *p2*) × (*q1* − *p2*)). The facet colour is the colour-ramp look-up of [terrain](../terrain/edges_normals_colours_and_sea.md#412-colour-ramp-look-up) TER-181 for that triangle (slope from the facet normal, height from the triangle's centre, with its colour variation), read from the map's ramp `terrain.ramp`. The centre is the mean of the triangle's corners in strip order, as TER-181 gives it: *p1*, *p2* and *q1* for the first triangle and *p2*, *q1* and *q2* for the second, while the facet normal keeps the vertex order (*q1*, *p2*, *q2*) above; while the match's seasonal state is active the ramp is file 6 of `terrain.ramp`, the seasonal ramp (see DEC-118). In the inverted style every facet colour has alpha 0 (see DEC-040).

**RND-135** (MUST) The wire-grid overlay is drawn in the same pass as the surface (ADR-011). Its pattern is the stylised grid texture `fx.terrain_grid` (a 2 × 2 block of sub-cells, three of them with a diagonal), mapped once onto each height-field cell with grid coordinates (*u*, *v*) = (world *x* ÷ *cs*, world *z* ÷ *cs*), sampled with the linear-wrap sampler through all mip levels, so its lines do not follow the triangle edges (see DEC-326). The barycentric edge term of ADR-011 is the terrain shader's alternative permutation for DEC-326 option B ([shaders.md](shaders.md) RND-266).

**RND-136** (MUST) Pass 4 uses the `terrain` entry (`Opaque`, `TestWrite`, `Back`), level fog (RND-222) and the two directional lights (RND-226). The surface term is lit with the facet colour as its ambient and diffuse colour and no specular term. The overlay term is lit with the facet colour as its ambient and diffuse colour and a specular term of `rnd.terrain.overlay_specular` (0.5) per channel with a shininess of `rnd.terrain.overlay_shininess` (40), whose light specular colours equal the light colours, so grid lines catch highlights; it is multiplied by the grid texel and added to the surface term. The exact combination, including fog and clamping, is RND-266 in [shaders.md](shaders.md).

**RND-137** (MUST) In the inverted style (see DEC-040) pass 4 uses the `terrain_inverted` entry: both terms darken the destination (the `Darkening` blend), the surface term with depth write on, and fog runs towards the inverted fog colour (RND-222).

**RND-138** (MUST) The mesh is split into chunks of a fixed grid size, each with a bounding box; chunks outside the view frustum issue no draw, and the visible chunks are drawn with one instanced or indirect draw (ADR-011). Every chunk is drawn at full resolution: the reference terrain has no level of detail. A level-of-detail scheme for the strategic zoom and the large-world terrain inputs are decided by the design phase (see DEC-247).

**RND-139** (MUST) The terrain mesh is built when the match world initialises and rebuilt only after device removal (RND-034) or when the terrain data changes; option changes do not rebuild it (RND-031). The renderer also keeps the heights in the `terrain_heights` texture (ADR-011).

### 4.2 Sea

**RND-140** (MUST) The sea covers the square plane from (−*X*/2, −*Z*/2) to (1.5·*X*, 1.5·*Z*), twice the land size, over which [terrain](../terrain/edges_normals_colours_and_sea.md#414-sea-derivations) defines the shore mask, the depth map and the flat-water tile mask. It is drawn as two layers: a flat textured layer at the height `rnd.sea.flat_height` (−9) near land, and the coastline wave mesh near height 0, which exists only near the coast. Open sea is not drawn at all: the background colour and the fog show through.

**RND-141** (MUST) The flat layer is one quad per set flag of the flat-water tile mask, each tile covering its block of the plane at the height `rnd.sea.flat_height`; tiles whose depth samples are all open sea are not drawn.

**RND-142** (MUST) The flat layer combines two textures with the `modulate` combine mode ([shaders.md](shaders.md) RND-268) and a vertex colour of white (alpha 0 in the inverted style): texture 0 is the map's sea tile `terrain.sea_tile` (file 2 while the seasonal state is active), sampled with the linear-wrap sampler through all mip levels, with `rnd.sea.tile_repeats` (30) repeats across the plane on both axes, scrolled on both axes by *s* ÷ `rnd.sea.tile_scroll_period` (30 s), where *s* is the smooth presentation time (R-SIM-570); texture 1 is the shore mask, sampled with the linear clamp-to-edge sampler through all mip levels, with coordinates running from 0 to 1 across the plane and *v* = 0 at the edge *z* = 1.5·*Z*, in the orientation in which the terrain component lays out the mask.

**RND-143** (MUST) Pass 5a draws the flat layer with the `sea_flat` entry (`Opaque`, `Test`, `None`), unlit, with level fog; in the inverted style with `sea_flat_inverted` (`Darkening`).

**RND-144** (MUST) Match-world initialisation builds one coastline wave mesh for each of the sea-detail levels 1, 2 and 3, from the vertex pairs that [terrain](../terrain/edges_normals_colours_and_sea.md#414-sea-derivations) selects for that level (TER-201), in their build order, with the triangles of RND-132 between consecutive needed pairs. The sea-detail setting selects which of the meshes is drawn, so a change of the setting rebuilds nothing ([options_and_budgets.md](options_and_budgets.md) RND-448, RND-031). The colour table is row `rnd.sea.wave_table_row` (1) of the map's shore-wave gradient `terrain.shore_wave_gradient`, counted from the bottom row as viewed (TER-041) (file 4 while the seasonal state is active): one colour per pixel column of the gradient, each with alpha 1 (alpha 0 in the inverted style). With the sea detail at 0, no wave mesh is drawn and only the flat layer is.

**RND-145** (MUST) Once per rendered frame, never from the simulation, a compute pass at the start of pass 5b evaluates the wave heights and colours of every wave-mesh vertex with the coastline-wave formulae of [terrain](../terrain/edges_normals_colours_and_sea.md#414-sea-derivations) (TER-204, TER-205) at the smooth presentation time (R-SIM-570), into the wave vertex buffer of the current frame in flight (ADR-011). The colour of each vertex pair depends on the heights of its predecessor pair in build order, which are 0 before the first pair of the mesh; each pair evaluates its predecessor's heights from the same formulae, so pairs are independent of one another, and every duplicated join vertex takes the height of the vertex it duplicates. Each wave triangle takes as its facet colour the colour of its last vertex in build order: *q1* for (*p1*, *p2*, *q1*) and *q2* for (*q1*, *p2*, *q2*). The smooth presentation time is the Species reading of the smooth game-timer clock that the reference reads on this render path; the presentation clock provides it as the scaled presentation time ([animation_and_vfx/README.md](../animation_and_vfx/README.md) VFX-009).

**RND-146** (MUST) Pass 5b draws the wave mesh with the `sea_waves` entry (`Additive`, `TestWrite`, `Back`), unlit, with level fog. The visible result: near islands the shore-masked flat layer glows, and above it the additive waves brighten towards the coast and with wave height.

**RND-147** (MUST) The flat layer is drawn before the wave layer, and both after the terrain (pass 4), so the sunk coastal facets of RND-133 lie below both layers.

**RND-148** (MUST) In the map editor only, the sea is replaced by two quads over the land rectangle at the height `rnd.sea.editor_height` (−10): first `fx.terrain_grid` with `rnd.sea.editor_grid_repeats` (100) repeats across the rectangle in the colour `rnd.sea.editor_grid_colour` (250, 250, 250, 100 of 255) with the `Alpha` blend, then an untextured quad of alpha `rnd.sea.editor_fill_alpha` (30 of 255). This is a map-editor hook ([reference/08_maps_and_level_design/map_editor_hooks.md](../../../reference/08_maps_and_level_design/map_editor_hooks.md)).

### 4.3 Sky and clouds

**RND-150** (MUST) The sky is one instanced draw with the `sky` entry (`Additive`, `None`, `Back`): the sky grid, then the blobby cloud layers, then the flat cloud layers, each layer with its own texture view, sampler, colour, texture-coordinate transform and fog range. Every layer faces downwards, so it is visible only from below and culled from above. The sky fog colour is always black, also in the inverted style. The sky is drawn in pass 3 of a match frame, as the main-menu sky (RND-077) and behind the loading view (RND-118).

**RND-151** (MUST) The sky grid is a set of straight line quads at the height `rnd.sky.grid_height` (1200), spaced `rnd.sky.grid_spacing` (80 wu) apart over *x* and *z* from `rnd.sky.grid_min` (−4000) to `rnd.sky.grid_max` (6000), each `rnd.sky.grid_line_half_width` (8 wu) wide on either side of its centre line, in both directions: 125 lines along *x* and 125 along *z*. Each line carries `fx.beam_short` once, *u* along its length and *v* across its width, sampled with the linear clamp-to-edge sampler through all mip levels, in the colour `rnd.sky.grid_colour` (0.5, 0.5, 1.0, alpha 0.3), with black linear fog from `rnd.sky.grid_fog_start` (2000) to `rnd.sky.grid_fog_end` (4000).

**RND-152** (MUST) The sky has a cloud offset *O* (starting at 0) and a velocity *V* (starting at `rnd.sky.initial_velocity` (0.04, 0, 0)). Once per tick, in update slice `ref.sim.slice.sky` (R-SIM-513), the sky update sets *V* to `rnd.sky.velocity` (0.03, 0, −0.01) and adds *V* × the per-tick game-time step (`ref.sim.tick_period` × *s*, R-SIM-570) to *O*; the update is cosmetic and makes no random draw. The draw offset is *o* = *O* + *V* × the sky extrapolation time of RND-083. The front end's menu sky is never updated: its draw offset is (0.04, 0, 0) × (−real clock), so its clouds drift with real time.

**RND-153** (MUST) The cloud layers are quads over *x* and *z* from `rnd.sky.cloud_min` (−8000) to `rnd.sky.cloud_max` (9000), with black linear fog from `rnd.sky.cloud_fog_start` (2000) to `rnd.sky.cloud_fog_end` (5000), sampling `fx.sky_cloud` through a view of its mip level 0 only. Their heights, colours, texture coordinates across the quad and conditions on the sky-detail setting ([options_and_budgets.md](options_and_budgets.md)) are:

| Layer | Height | Colour (RGB, alpha) | Coordinate along *x* | Coordinate along *z* | Drawn when |
|---|---|---|---|---|---|
| blobby 1 | `rnd.sky.grid_height` (1200) | `rnd.sky.blobby_colour` (0.7, 0.7, 0.9, 0.6) | *o.x* to *o.x* + `rnd.sky.layer1_repeats` (9) | *o.z* to *o.z* + 9 | sky detail 1 or 2 |
| blobby 2 | `rnd.sky.layer2_height` (1000) | blobby colour | *o.x* ÷ 2 to *o.x* ÷ 2 + `rnd.sky.layer2_repeats` (4.5) | *o.x* ÷ 2 to *o.x* ÷ 2 + 4.5 | always |
| blobby 3 | `rnd.sky.layer3_height` (800) | blobby colour | *o.z* to *o.z* + `rnd.sky.layer3_repeats` (1.8) | *o.x* to *o.x* + 1.8 | sky detail 1 |
| flat 1 | 1200 | `rnd.sky.flat_colour` (0.7, 0.7, 0.9, 0.3); alpha `rnd.sky.flat_alpha_low_detail` (0.5) at sky detail 3 | as blobby 1 | as blobby 1 | always |
| flat 2 | 1000 | flat colour | as blobby 2 | as blobby 2 | sky detail 1 |

**RND-154** (MUST) The blobby layers sample `fx.sky_cloud` with the linear-wrap sampler. The flat layers sample it with point filtering and wrapping: the shader wraps the coordinates and samples through the point clamp-to-edge sampler, so each texel of the small tile is magnified into a hard-edged block.

**RND-155** (MUST NOT) The sky draws no picture layer: its only layers are the grid and the cloud layers of RND-151 and RND-153.

### 4.4 Meshes and team tint

**RND-156** (MUST) The cooked mesh format and the renderer support at least `rnd.mesh.max_nodes` (100) nodes and `rnd.mesh.max_sockets` (100) sockets per mesh.

For budgeting, the reference mesh set has 139 meshes with up to 20 nodes per mesh (440 in total), a hierarchy depth of up to 6, up to 38 sockets per mesh (425 in total), and 150 820 triangles in total, with up to 24 366 and a mean of 1085 per mesh; two of its meshes carry sockets only.

**RND-157** (MUST) A cooked mesh (`MeshAsset`, ADR-019) is a hierarchy of mesh nodes (`MeshNode`) under an implicit root node with the identity transform and no geometry. Each node has a rest transform relative to its parent, a draw range, per-triangle facet colours and facet normals, an axis-aligned bounding box, its vertical extent (lowest and highest *y*), and a bounding sphere whose centre is the box centre and whose radius is the largest distance from that centre to any of the node's vertices. Each socket (`MeshSocket`, ADR-021) names its parent node and keeps the chain of nodes from the root to that parent.

**RND-158** (MUST) Transforms use row vectors (ADR-016): a node's world transform is its local transform × its parent's world transform, and a socket's world transform is its own transform × the transforms of its parent chain from the deepest node up to the root × the instance transform.

**RND-159** (MUST) An animated node is drawn with its local transform advanced by the extrapolation of RND-081: its orientation rotated by its angular velocity × *t* and its position moved by its velocity × *t*, both in its parent's space. Owners animate parts only by setting node velocities and orientations in the presentation snapshot (ADR-035).

**RND-160** (MUST) Every authored mesh colour is multiplied by `rnd.mesh.colour_attenuation` (0.85) once, by the mesh cooker, so that lit 3-D colours match their 2-D equivalents ([asset_pipeline_and_resources.md](../asset_pipeline_and_resources.md)).

**RND-161** (MUST) One cooked mesh serves every team: instances carry a team palette index (RND-016), and the material shader tints by the facet's tint weight *w* (ADR-020). Each mesh has a tint scale *f*, a value of its import record that the shader reads from the per-draw material parameters: `rnd.tint.factor` (0.7) unless the mesh's owner names another, as the Transport hull does with `ref.unit.transport.hull_tint` (0.1, [transport.md](../../../reference/02_units/transport.md) R-UNIT-408) and the Collector body with `ref.unit.collector.tint_scale` (0.7, [collector.md](../../../reference/02_units/collector.md) R-UNIT-443). With the team colour *T* and the facet colour *c* (both in 0 to 1), the tinted colour is *T* × (*c.r* + *c.g* + *c.b*) ÷ (*T.r* + *T.g* + *T.b*) × *f*, which keeps the facet's brightness; on a mesh marked convert-all it is *T* × *f*. The drawn colour is *c* + *w* × (tinted − *c*), clamped to [0, 1]. Instances of no team are never tinted. Only the root node and its direct children are tinted; deeper nodes keep their facet colours.

**RND-162** (MUST) These meshes follow special tint rules: on the Worker figure `mesh.unit.worker_figure`, the colour of palette entry 0 is multiplied by `rnd.tint.worker_figure_entry0_factor` (0.5) before tinting; the Leader figure `mesh.unit.leader_figure` is convert-all; the Transport hull `mesh.unit.transport_hull` is convert-all with every facet at tint weight 1 and the tint scale `ref.unit.transport.hull_tint`, so every facet takes the team colour (its co-op shade where the team has one, R-SIM-421) × 0.1; the Collector body `mesh.unit.collector_body` is convert-all with every facet at tint weight 1 and the tint scale `ref.unit.collector.tint_scale`; turret heads `mesh.structure.turret_head` tint with the team colour and co-op shade; the Beam Dish `mesh.structure.beam_dish` re-tints when its team changes. The team-tinted meshes are those of centipede segments `mesh.creature.centipede_segment`, ants `mesh.creature.ant_body`, the Leader, the Reaper Serpent, turrets, the Beam Dish, the Transport and every actor that draws a team mesh ([entity_and_overlay_renderers.md](entity_and_overlay_renderers.md)). Team regions are marked explicitly, by the tint attribute `_SPECIES_TINT` or the material flag `species_team_tint` ([asset_pipeline_and_resources.md](../asset_pipeline_and_resources.md) AST-034, ADR-020); art made in the key-colour style reaches the mesh cooker only as explicit weights written by the authoring helper, which the artist reviews. The look is that of RND-406.

Mesh queries used by the simulation are not renderer functions (RND-004). For the collision assets that serve them, the reference query contract is:

| Query | Rule |
|---|---|
| ray | per node: the ray against the node's world-space bounding sphere (radius above 0 only); on a hit, true for an approximate query; for an exact query, every triangle of the node in world space; otherwise the node's children |
| sphere | the same with sphere-against-sphere and sphere-against-triangle tests |
| mesh against mesh | each node's bounding sphere of one mesh tested with the sphere query of the other |
| centre | the mean of all node centres, the root included |
| radius | the largest, over nodes, of the distance from the centre to the node's centre plus the node's radius |
| radius scale | multiplies every node radius by a factor |

### 4.5 Structures

**RND-163** (MUST) Pass 6 draws, for every structure in view, its mesh with the transform built from the structure's front, up and position at the slice-aware time of RND-082, with level fog and the object lighting, or the special lighting where RND-229 applies. Structures add these solid parts: team-tinted turret heads on the turret mount's sockets; Barrier Emitter posts scaled uniformly by the emitter's scale; scenery meshes (`SceneryMesh`) scaled by their scale; in the map editor, a direction arrow (a hook). The structure-detail setting ([options_and_budgets.md](options_and_budgets.md)) switches the drawing branches of ports, of Barrier Emitter translucent layers and of spawn-network translucent layers at detail level 1 and at levels below 3, as the rules below state; whether Species keeps these branches or defines its own quality levels is decided by the design phase (see DEC-327).

**RND-164** (MUST) Pass 8 is fogged with level fog for its whole duration. It first draws, in structure-pool order, every structure that does not ask for a depth sort, and then collects the structures that do ask for one at their sort centre.

**RND-165** (MUST) The default alpha content of a structure is its ownership lights (RND-167) and its ports (RND-168); a structure kind adds or replaces content as RND-169 to RND-176 state.

**RND-166** (MUST) The collected structures are sorted by the squared distance from the camera position to their sort centre and drawn from the farthest to the nearest; equal distances keep structure-pool order.

**RND-167** (MUST) Ownership lights, the structure light flashes of [reference/03_buildings_and_world_objects/README.md](../../../reference/03_buildings_and_world_objects/README.md) R-BLDG-045, are drawn at every socket of a structure's mesh that is of the light kind (ADR-021), for structures that belong to a team, only in the command frames whose number *n*, the latest command frame received (R-SIM-531), satisfies ⌊(*n* mod `ref.bldg.structure.light_cycle_ticks` (10)) ÷ `ref.bldg.structure.light_ticks_per_team` (2)⌋ = the team's index, and always in the map editor. Each team's lights are therefore on for 2 of every 10 command frames, 0.2 s on in a 1.0 s period at the reference tick rate, in a staggered order; because the quotient ranges only from 0 to 4, the team in slot 5 of the `ref.sim.max_teams` slots, the Visitor faction's slot (DEC-082), never shows ownership lights (see DEC-958 item 1). Each light is `ref.bldg.structure.light_quads` (10) nested camera-facing quads of half-size `ref.bldg.structure.light_size_wu` (6 wu) × *i* ÷ 10 for *i* from 0 to 9, with `fx.starburst` in the team colour, using the `glow_additive_fog` entry.

**RND-168** (MUST) For each of a structure's ports, as [reference/03_buildings_and_world_objects/README.md](../../../reference/03_buildings_and_world_objects/README.md) R-BLDG-046 specifies: while the structure-detail setting is below `ref.bldg.port.pad_detail_limit` (3), the port pad mesh `mesh.structure.structure_default` is drawn at the terrain height + `ref.bldg.port.pad_height_wu` (0.5 wu) with the object lighting, using the `port_pad` entry; then a camera-facing `fx.starburst` of size `ref.bldg.port.status_star_size_wu` (6 wu) at the pad's status socket, in `ref.bldg.port.status_occupied_rgb` (0.3, 1.0, 0.3) when a Worker occupies the port and `ref.bldg.port.status_empty_rgb` (1.0, 0.3, 0.3) when it is empty, using `glow_additive_fog`. The structures that R-BLDG-046 names as drawing their ports in their own way follow their own rules (RND-169, RND-172).

**RND-169** (MUST) The spawn-network structures (Spawn Nodes, Relays and Source Nodes, [reference/03_buildings_and_world_objects/economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md)) draw: in pass 6 their meshes `mesh.structure.spawn_node`, `mesh.structure.relay_mast` and `mesh.structure.source_node`, where the Source Node's mesh is drawn only in the map editor or when the structure's map record sets its global flag (see DEC-309); in pass 8, with `glow_additive_fog`:

1. transit beams between linked structures' link sockets with `fx.beam_short`, an inner beam of half-width `rnd.structure.spawn.beam_inner_half_width` (1 wu) and an outer beam of half-width `rnd.structure.spawn.beam_outer_half_width` (3 wu); the outer beam glow only at structure detail 1;
2. every transit orb (`TransitOrb`) on an edge as a diamond with an inner size of `rnd.structure.spawn.orb_inner_size` (2 wu) and an outer size of `rnd.structure.spawn.orb_outer_size` (6 wu), in the team colour of the Spawn Node it travels to, or grey `rnd.structure.spawn.orb_neutral_grey` (120 of 255) when that Spawn Node has no team;
3. a status star per port, grey `rnd.structure.spawn.empty_port_grey` (150 of 255) when empty, otherwise in the occupant's team colour;
4. glow blobs above the structure with `fx.glow_cloudy`: `ref.bldg.spawn_node.glow_blobs_by_detail` (20, 10 and 3 at structure detail 1, 2 and 3; [economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) R-BLDG-131), with alpha = occupied ports ÷ `ref.econ.spawn_node.port_count` (5), and a colour that moves linearly from `ref.econ.spawn_node.glow_start_rgba` (100, 100, 100, 200) to the team colour as the remaining activation delay runs from `ref.econ.spawn_node.activation_ticks` (300 ticks) down to 0 ([reference/05](../../../reference/05_population_flow.md) R-ECON-025).

**RND-170** (MUST) A turret draws its mount `mesh.structure.turret_mount` (sockets only) and its team-tinted head `mesh.structure.turret_head` on the mount's sockets in pass 6, and its lights and ports in pass 8; it draws its ports as RND-168 states, except that the status star of port i (1 to 4) sits at the mount's socket `status_0i` instead of the pad's status socket ([reference/03_buildings_and_world_objects/README.md](../../../reference/03_buildings_and_world_objects/README.md) R-BLDG-046; see DEC-860). Its barrel smoke is particles, and its ejected shell casings (`ShellCasing`) and rounds (`TurretRound`) are effects ([README.md](../animation_and_vfx/README.md), [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md)).

**RND-171** (MUST) A Beam Dish draws its tinted dish mesh `mesh.structure.beam_dish` in pass 6. In pass 8 it draws, along each active link, four nested beam tubes of radii `rnd.structure.dish.tube_radii` (10, 9, 8 and 4 wu) with alphas `rnd.structure.dish.tube_alphas` (0.4, 0.2, 0.2 and 0.5), textured with `fx.barrier_field` × `fx.dish_signal` in the `modulate` combine mode, clipped against the two planes of the linked dishes through two clip distances, with the `dual_additive_clip_fog` entry; while the dish is hovered or selected, it draws preview beams to every valid link target.

**RND-172** (MUST) The energy-grid structures ([reference/03_buildings_and_world_objects/mode_objects.md](../../../reference/03_buildings_and_world_objects/mode_objects.md)) draw: a power-line beam with `fx.beam_short` between the power-line sockets of linked structures; each energy surge as a star of size `ref.bldg.grid.surge_sprite_size_wu` (20 wu) with `fx.starburst` in the surge's team colour, followed by a trail of `ref.bldg.grid.trail_sprites` (3) sprites, except that surges of the Rogue and Visitor factions draw no trail ([economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) R-BLDG-173); at a Solar Array, status lights in the occupied and empty colours of RND-168 and, while it is owned and occupied, `rnd.structure.grid.solar_glow_count` (4) glow quads of `rnd.structure.grid.solar_glow_size` (60 × 40 wu) with `fx.glow_round` in the team colour, with brightness `rnd.structure.grid.solar_glow_base` (0.5) + `rnd.structure.grid.solar_glow_pulse` (0.5) × |sin *g*|, where *g* is the presentation frame time in seconds (R-SIM-570; see DEC-959 item 2). A grid terminal draws nothing. All of these use `glow_additive_fog`, the power lines `line_additive_fog`.

**RND-173** (MUST) A jump pad draws in pass 8 an animated arrow strip with `marker.jump_pad`, `rnd.structure.jump_pad.arrow_length` (80 wu) long and `rnd.structure.jump_pad.arrow_half_width` (20 wu) wide on either side, with brightness `rnd.structure.jump_pad.base_brightness` (0.3) + `rnd.structure.jump_pad.pulse_brightness` (0.7) × |sin *g*|, with *g* the presentation frame time (see DEC-959 item 2), using `glow_additive_fog`.

**RND-174** (MUST) These structures draw only their solid parts or nothing:

| Structure | Pass 6 | Pass 8 |
|---|---|---|
| Relic | one of the variants of `mesh.relic.variant`, chosen by the simulation draw of DEC-012, drawn with the special lighting; when it appears, a shatter effect with fraction `rnd.structure.relic.shatter_fraction` (0.5) ([README.md](../animation_and_vfx/README.md)) | lights and ports |
| wall | `mesh.structure.wall_section` at its position lowered by its fall speed × *t*, scaled; nothing once destroyed; its destruction is a shatter effect with the parameter `rnd.structure.wall.shatter_parameter` (3.0) | nothing |
| scenery mesh | its named mesh, scaled; without a mesh, a placeholder sphere of radius `rnd.structure.scenery.placeholder_radius` (40 wu) | nothing |
| exclusion zone, volcano vent | nothing in a match; spheres in the map editor (a hook) | nothing; a volcano vent's visuals are particles |
| smoke column | nothing | nothing; its visuals are particles |
| Allegiance Scrambler | nothing | nothing; its visuals are muzzle-flash particles in random team colours from a presentation generator ([README.md](../animation_and_vfx/README.md)) |

**RND-175** (MUST) Trees draw nothing in pass 6; Barrier Emitters draw their posts in pass 6. Both draw their translucent parts in pass 8 (§4.6, §4.7) and ask for a depth sort.

**RND-176** (MUST) Pass 8 draws the three-dimensional countdown screens of reference/03 as camera-facing quads of `uitex.panel_neutral` with the `alpha_fog` entry, each with its number drawn over it in the UI framework's glyphs (ADR-034; see DEC-959 item 5):

1. the Core Device's screen, in its team colour, `ref.bldg.core_device.countdown_screen_height_wu` (50 wu) above it, while its time remaining is at most `ref.bldg.core_device.countdown_display_mode_seconds` (10 s) ([mode_objects.md](../../../reference/03_buildings_and_world_objects/mode_objects.md) R-BLDG-316);
2. each fuel depot's screen of `ref.bldg.fuel_depot.countdown_screen_wu` (60 × 60 wu), `ref.bldg.fuel_depot.countdown_screen_height_wu` (150 wu) above the depot, while its Launch Vessel is in the Countdown state or in Flight: ⌊countdown⌋ + 1 during the Countdown and a blinking 0 during Flight ([economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) R-BLDG-205).

### 4.6 Trees

**RND-180** (MUST) Each tree's geometry is generated once, at map load, in unit space with the trunk from (0, 0, 0) to (0, 1, 0), as data (ADR-009), from the tree's parameters ([reference/03_buildings_and_world_objects/defences_and_props.md](../../../reference/03_buildings_and_world_objects/defences_and_props.md)): its seed, iteration depth *N*, bud size, push-out and push-up factors, branch and leaf colours. It is regenerated after device removal. The generator is a local tree generator, never a shared presentation generator (RND-005, R-SIM-651).

**RND-181** (MUST) The tree generator's state is a 32-bit integer that wraps modulo 2³². `Next()` sets state = state × `rnd.tree.generator.multiplier` (214013) + `rnd.tree.generator.increment` (2531011) and returns (state ÷ 2^`rnd.tree.generator.shift` (16)) AND `rnd.tree.generator.mask` (0x7FFF); `NextScaled(r)` returns *r* × `Next()` ÷ `rnd.tree.generator.mask` (32767). Seeding sets the state to the seed.

**RND-182** (MUST) A branch segment is generated by this procedure, with the width factor `rnd.tree.width_factor` (0.1), the child offset `rnd.tree.child_offset_min` (0.3) + `NextScaled`(`rnd.tree.child_offset_range` (0.6)) and the child length factor `rnd.tree.child_length` (0.4):

```text
Branch(from, to, n, pass):
  if n == 0: return
  n = n - 1
  if n == 0:                                   # a leaf segment
    if pass == RADIUS: crownRadius = max(crownRadius, |to - crownCenter|)
    elif pass == LEAF: crownCenter += to; leafCount += 1
  d = to - from
  A = Normalize(d x to)                        # Normalize of a zero vector gives (0, 0, 1)
  B = Normalize(A x d)
  len = |d|
  w = widthFactor * (budSize if n == 0 else 1.0)
  if (n == 0 and pass == LEAF) or (n != 0 and pass == BRANCH):
    emit quad (from - A*len*w, uv 0,0) (from + A*len*w, uv 0,1) (to + A*len*w, uv 1,1) (to - A*len*w, uv 1,0)
    emit the same quad with B in place of A    # two crossed planes per segment
  for i in 0 .. 3:                             # four children, also below leaf segments
    dir = [A, -A, B, -B][i]
    t = childOffsetMin + NextScaled(childOffsetRange)
    childFrom = from + d * t
    childTo = childFrom + dir * len * childLength * pushOut + d * (1 - t) * pushUp
    Branch(childFrom, childTo, n, pass)
```

**RND-183** (MUST) Generation runs: crown centre = 0, crown radius = 0, leaf count = 0, unpack the colours (with the leaf-alpha boost of RND-191); seed with the tree's seed and run `Branch((0,0,0), (0,1,0), N, BRANCH)` for the branch quads; seed again and run it with `LEAF` for the leaf quads; divide the crown centre by the leaf count; seed again and run it with `RADIUS`, which emits nothing. The crown volume this computes is the content build's derivation of the baked crown volume, with the radius multiplied by `ref.sim.integrity.tree_crown_radius_factor` (R-SIM-651); the renderer never writes it (RND-004, see DEC-012).

**RND-184** (MUST) A tree of depth *N* has 4^*k* segments at level *k*, 4^(*N*−1) leaf segments and (4^(*N*−1) − 1) ÷ 3 branch segments, each with two quads; for *N* = 6 that is 1024 leaf and 341 branch segments.

**RND-185** (MUST) A tree is drawn in pass 8 only, sorted at its sort centre: its position + its crown centre × its height (the crown centre ignores the tree's rotation). The transform is the tree's front, up and position × a uniform scale of its drawn height. The branch quads are drawn in the branch colour, then, unless the seasonal state is active or the tree is blighted, the leaf quads in the leaf colour, both with `fx.tree_canopy` (white with its mask as alpha, tinted by the draw colour) through the linear clamp-to-edge sampler with all mip levels, using `glow_additive_fog` unless a rule below names `alpha_fog`.

**RND-186** (MUST) The drawn height is the tree's height × (1 + sin((real clock + object serial × `rnd.tree.sway.serial_phase` (2)) × `rnd.tree.sway.rate` (2)) × `rnd.tree.sway.amplitude` (0.003)), a breathing scale; a blighted tree uses the amplitude `rnd.tree.sway.blighted_amplitude` (0.01). The object serial is the tree's simulated-object serial (`SimObjectId`).

**RND-187** (MUST) A tree of an Essence Grove (a tree whose Essence-orb drop rate is above 0) pulses its leaf colour's blue channel: *B* = *B* × *f* + 255 × (1 − *f*) with *f* = |sin(real clock + object serial × `rnd.tree.essence_pulse.serial_phase` (10))|.

**RND-188** (MUST) A blighted tree (one of a Blight Grove) draws its branches in `rnd.tree.blighted.branch_colour` (0, 0, 0, alpha 200 of 255), no leaves, with the `alpha_fog` entry: a dark silhouette.

**RND-189** (MUST) A tree that is turning blighted (its blight timer is at or below 0 and it is not yet blighted) is drawn twice, with *k* = (`rnd.tree.blighting.span` (10) + blight timer + *t*) ÷ `rnd.tree.blighting.span`: first with its branch colour's RGB and alpha multiplied by *k* and its leaf alpha multiplied by (1 − *k*); then as a black silhouette with branch alpha `rnd.tree.blighted.silhouette_alpha` (200) × *k* of 255.

**RND-190** (MUST) A burning tree (burn level *b* above 0) uses `alpha_fog`: its branch RGB is scaled towards black by min(*b* × `rnd.tree.burning.darken_gain` (10), 1) with alpha 1; its leaves are black with alpha × (1 − *b*). It also draws `rnd.tree.burning.flame_count` (4) camera-facing `fx.starburst` billboards with `glow_additive_fog_nodepth`, in `rnd.tree.burning.flame_colour` (0.8, 0.2, 0.1) with alpha `rnd.tree.burning.flame_alpha` (0.5) × drawn height ÷ height, flame *i* (0 to 3) centred at the position + (sin(*ω* *g* + *i*) × *i* × `rnd.tree.burning.flame_spread` (1.7), `rnd.tree.burning.flame_height` (0.8) × drawn height + cos(*ω* *g* + *i*), cos(*ω* *g* + *i*) × *i* × 1.7), with *g* the presentation frame time and *ω* = `rnd.tree.burning.flame_phase_rate` (2 rad per s), each of half-size max(`rnd.tree.burning.flame_size_gain` (2) × drawn height, `rnd.tree.burning.flame_min_size` (5 wu)). Each tick it emits ⌊drawn height ÷ `rnd.tree.burning.particle_height_step` (12)⌋ ÷ the structure-detail level fire particles through effect events ([README.md](../animation_and_vfx/README.md), ADR-036). A regrowing tree (burn level below 0) hides its leaves while |*b*| > `rnd.tree.regrow.hide_threshold` (0.5) and otherwise multiplies their alpha by (1 − `rnd.tree.regrow.fade_gain` (2) × |*b*|).

**RND-191** (MUST) While the seasonal state is active, branches are drawn in `rnd.tree.seasonal_branch_colour` (180, 100, 50 of 255) and no leaves are drawn (see DEC-118). The tree-detail setting (no user interface, default 1; [options_and_budgets.md](options_and_budgets.md)) multiplies the leaf alpha by `rnd.tree.leaf_alpha_boost` (1.3) to the power of the detail level, capped at 255, at generation, when the level is above 1; it changes no geometry.

### 4.7 Barrier Emitters

**RND-195** (MUST) A Barrier Emitter's post is drawn in pass 6 with `mesh.structure.barrier_emitter`, scaled uniformly by its scale, with unit normals.

**RND-196** (MUST) The curtain to the linked emitter is drawn in pass 8 while the emitter's mode is neither disabled nor never-on and its link resolves to a Barrier Emitter (RND-006), with no culling and no depth write. An emitter is treated as in view throughout the grace period. It asks for a depth sort at its centre position.

**RND-197** (MUST) The curtain colour is `rnd.barrier.neutral_colour` (100, 100, 100, alpha 150 of 255) for an emitter of no team; for a solar-linked emitter it moves between `rnd.barrier.solar_dark` (20, 20, 20) and `rnd.barrier.solar_team_factor` (0.9) × the team colour by |sin(`rnd.barrier.solar_pulse_rate` (4) × real clock)|, with alpha 150; otherwise it is the team colour with alpha `rnd.barrier.alpha` (150 of 255).

**RND-198** (MUST) The predicted status is the emitter's status moved towards its target by `rnd.barrier.status_rate` (0.3) × *t* (1 in the map editor). With *h* and *h*′ the full heights of this post and the linked post × their predicted statuses, the curtain quad has the corners: bottom-left = this post − (0, *h* × `rnd.barrier.skirt_fraction` (1/3), 0), top-left = this post + (0, *h*, 0), top-right = linked post + (0, *h*′, 0), bottom-right = linked post − (0, *h*′ × 1/3, 0); the skirt reaches a third of the height below the ground.

**RND-199** (MUST) While the structure-detail setting is below 3, the curtain is drawn with the `dual_alpha_fog` entry in the `replace_then_modulate` combine mode ([shaders.md](shaders.md) RND-268): texture 0 is `fx.barrier_field` with coordinates *s* from *s₀* to *s₀* + *dx* and *t* from 0 to *dz*, where *dx* = the distance between the posts ÷ (`rnd.barrier.repeat_factor` (2) × the full height), *dz* = the status and *s₀* = the presentation frame time ÷ `rnd.barrier.scroll_period` (15 s); texture 1 is `fx.barrier_field_wide` with *s* from 0 to 1 and *t* from *t₀* to *t₀* + *th*; both use the linear-wrap sampler with all mip levels. A solar-linked emitter uses *t₀* = the presentation frame time ÷ `rnd.barrier.solar_scroll_period` (1.5 s), *th* = `rnd.barrier.solar_repeat` (7), *s₀* = 0 and `fx.barrier_field` also as texture 1; any other emitter uses *t₀* = 0 and *th* = 1 (see DEC-959 item 1). The scroll reads the presentation frame time, the clock of render-time animation (see DEC-959 item 2).

**RND-200** (MUST) At every structure-detail level, the curtain is then drawn once more with `fx.barrier_field_wide` (*s* from 0 to 1, *t* from *t₀* to *t₀* + *th*) in the curtain colour with the `glow_additive_fog` entry, the burn layer.

**RND-201** (MUST) Finally a smoothed line is drawn along the top edge from the top-left to the top-right corner, `rnd.barrier.edge_width` (2.0 px) wide (`rnd.barrier.solar_edge_width` (0.5 px) for a solar-linked emitter), in the curtain colour, with the `line_additive_fog` entry.

**RND-202** (MUST) A Barrier Emitter draws no ownership lights.

## 5 Concurrency and performance budget

**RND-205** (MUST) Terrain, flat-sea, tree and structure geometry are static default-heap buffers built at load (ADR-008); per frame the CPU only culls chunks, writes structure and tree instance data and dispatches the wave update. The wave update and the sky draw are the only per-frame world work on the GPU besides the draws. GPU budgets are in [options_and_budgets.md](options_and_budgets.md).

## 6 Failure modes, diagnostics and telemetry

| Failure | Behaviour | Requirement |
|---|---|---|
| a map names a ramp, sea tile or gradient that does not exist | the map default (file 1 of each asset) and a diagnostic | RND-134, RND-142, RND-144 |
| a structure's mesh is missing | the default structure body `mesh.structure.structure_default` and a diagnostic (DEC-015) | RND-163 |
| a link names a missing or wrong-kind structure | drawn as unlinked; nothing is written | RND-006, RND-196 |
| a tree's parameters give zero leaves | no leaf quads; the crown data comes from the content build | RND-183 |

## 7 Acceptance tests

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 terrain mesh | unit | a small height field gives exactly the needed pairs, triangles, diagonals and sunk vertices of RND-132 and RND-133, including the spanning triangles of a field with land at its border | RND-132, RND-133 |
| T2 terrain normals | property | every facet normal of a random field has unit length and a positive *y* on flat ground | RND-134 |
| T3 terrain look | golden-image | the single-pass surface and grid at several distances and angles match the reference images; the grid lines do not follow the triangle diagonals | RND-135, RND-136 |
| T4 chunk culling | unit | chunks outside the frustum issue no draws | RND-138 |
| T5 sea | golden-image | flat layer and waves at three sea-detail levels match reference images at fixed smooth presentation times | RND-140 to RND-146 |
| T6 wave order | unit | the GPU wave colours equal a serial CPU evaluation in build order for a recorded mesh | RND-145 |
| T7 sky | golden-image | the sky at the three sky-detail levels, from below and from above the grid height | RND-150 to RND-154 |
| T8 sky offset | unit | after *k* ticks the offset is *k* × (0.03, 0, −0.01) × 0.1 at speed 1; the menu sky drifts at 0.04 per second of real time | RND-152 |
| T9 tint | golden-image | a reference mesh for every palette entry, with convert-all, Transport and Worker-figure rules; deeper nodes keep their colours | RND-161, RND-162 |
| T10 sort | unit | sorted structures draw farthest first; equal distances keep pool order | RND-164, RND-166 |
| T11 ownership lights | unit | for command frames 0 to 19 the lights of teams 0 to 4 are on in pairs of frames and team 5 never | RND-167 |
| T12 tree geometry | unit | for recorded seeds and parameters the quad lists and crown values equal the reference vectors; *N* = 6 gives 1024 and 341 segments | RND-181 to RND-184 |
| T13 tree states | golden-image | normal, Essence Grove, blighted, turning, burning, regrowing and seasonal trees | RND-185 to RND-191 |
| T14 Barrier Emitter | golden-image | curtains at detail 1 and 3, solar-linked and not, during status changes | RND-196 to RND-201 |

## 8 Open decisions

| Reference | Status | Where it applies |
|---|---|---|
| DEC-012 | Resolved | the Relic mesh draw and baked crown volumes (RND-174, RND-183) |
| DEC-958 | Open | the ownership-light blink formula, which leaves the Visitor faction's slot dark (RND-167) |
| DEC-959 | Open | the Barrier Emitter burn layer of an emitter that is not solar-linked, the clock of the cosmetic pulses and the look of the countdown screens (RND-172, RND-173, RND-176, RND-199, RND-200) |
| DEC-040 | Open | the inverted style (RND-134, RND-137) |
| DEC-118, DEC-101 | Resolved, Open | the seasonal ramp, sea tile, gradient and trees (RND-134, RND-142, RND-144, RND-191) |
| DEC-247 | Open | terrain level of detail and world scale (RND-138) |
| DEC-326 | Open | the terrain grid overlay pattern (RND-135) |
| DEC-309 | Open | the Source Node's global flag, which only changes its drawing (RND-169) |
| DEC-327 | Open | structure render-detail variants (RND-163) |

**Design-phase inputs.** The terrain's level-of-detail scheme for the strategic zoom and its streaming for a large world (DEC-247, ADR-011); whether the sea, the sky grid and the cloud layers extend with the world or follow the camera; whether the reference sky grid's fixed extent of −4000 to 6000 wu is kept for larger maps.

## 9 Tuning table

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `rnd.terrain.sink_threshold` | 0.3 | wu | vertices below this are sunk | RND-133 | |
| `rnd.terrain.sink_height` | −10 | wu | height of sunk vertices | RND-133 | |
| `rnd.terrain.overlay_specular` | 0.5 | per channel | specular colour of the grid term | RND-136 | DEC-326 |
| `rnd.terrain.overlay_shininess` | 40 | exponent | | RND-136 | |
| `rnd.sea.flat_height` | −9 | wu | flat sea layer | RND-140, RND-141 | |
| `rnd.sea.tile_repeats` | 30 | repeats across the plane | sea tile | RND-142 | |
| `rnd.sea.tile_scroll_period` | 30 | s of smooth presentation time | scroll = time ÷ 30 | RND-142 | |
| `rnd.sea.wave_table_row` | 1 | row index | of the shore-wave gradient | RND-144 | |
| `rnd.sea.editor_height` | −10 | wu | map-editor sea | RND-148 | |
| `rnd.sea.editor_grid_repeats` | 100 | repeats | map-editor sea | RND-148 | |
| `rnd.sea.editor_grid_colour` | 250, 250, 250, 100 | of 255 | map-editor sea | RND-148 | |
| `rnd.sea.editor_fill_alpha` | 30 | of 255 | map-editor sea | RND-148 | |
| `rnd.sky.grid_height` | 1200 | wu | sky grid, blobby 1, flat 1 | RND-151, RND-153 | |
| `rnd.sky.grid_spacing` | 80 | wu | | RND-151 | |
| `rnd.sky.grid_min` | −4000 | wu | | RND-151 | |
| `rnd.sky.grid_max` | 6000 | wu | 125 lines per axis | RND-151 | |
| `rnd.sky.grid_line_half_width` | 8 | wu | | RND-151 | |
| `rnd.sky.grid_colour` | 0.5, 0.5, 1.0, 0.3 | RGBA | | RND-151 | |
| `rnd.sky.grid_fog_start` | 2000 | wu, view depth | black fog | RND-151 | |
| `rnd.sky.grid_fog_end` | 4000 | wu, view depth | | RND-151 | |
| `rnd.sky.initial_velocity` | 0.04, 0, 0 | offset units per s of game time | until the first update; the menu sky keeps it | RND-152 | |
| `rnd.sky.velocity` | 0.03, 0, −0.01 | offset units per s of game time | set by every update | RND-152 | |
| `rnd.sky.cloud_min` | −8000 | wu | cloud quads | RND-153 | |
| `rnd.sky.cloud_max` | 9000 | wu | | RND-153 | |
| `rnd.sky.cloud_fog_start` | 2000 | wu, view depth | | RND-153 | |
| `rnd.sky.cloud_fog_end` | 5000 | wu, view depth | | RND-153 | |
| `rnd.sky.layer2_height` | 1000 | wu | blobby 2, flat 2 | RND-153 | |
| `rnd.sky.layer3_height` | 800 | wu | blobby 3 | RND-153 | |
| `rnd.sky.blobby_colour` | 0.7, 0.7, 0.9, 0.6 | RGBA | | RND-153 | |
| `rnd.sky.flat_colour` | 0.7, 0.7, 0.9, 0.3 | RGBA | | RND-153 | |
| `rnd.sky.flat_alpha_low_detail` | 0.5 | alpha | flat 1 at sky detail 3 | RND-153 | |
| `rnd.sky.layer1_repeats` | 9 | repeats | blobby 1, flat 1 | RND-153 | |
| `rnd.sky.layer2_repeats` | 4.5 | repeats | blobby 2, flat 2 | RND-153 | |
| `rnd.sky.layer3_repeats` | 1.8 | repeats | blobby 3 | RND-153 | |
| `rnd.mesh.max_nodes` | 100 | nodes per mesh | minimum supported | RND-156 | |
| `rnd.mesh.max_sockets` | 100 | sockets per mesh | minimum supported | RND-156 | |
| `rnd.mesh.colour_attenuation` | 0.85 | × | applied by the mesh cooker | RND-160 | |
| `rnd.tint.factor` | 0.7 | × | the default tint scale of a mesh; a mesh's owner may name another (RND-161) | RND-161 | |
| `rnd.tint.worker_figure_entry0_factor` | 0.5 | × palette entry 0 | Worker figure mesh only | RND-162 | |
| `rnd.structure.spawn.beam_inner_half_width` | 1 | wu | | RND-169 | |
| `rnd.structure.spawn.beam_outer_half_width` | 3 | wu | detail 1 only | RND-169 | DEC-327 |
| `rnd.structure.spawn.orb_inner_size` | 2 | wu | | RND-169 | |
| `rnd.structure.spawn.orb_outer_size` | 6 | wu | | RND-169 | |
| `rnd.structure.spawn.orb_neutral_grey` | 120 | of 255 | | RND-169 | |
| `rnd.structure.spawn.empty_port_grey` | 150 | of 255 | | RND-169 | |
| `rnd.structure.dish.tube_radii` | 10, 9, 8, 4 | wu | | RND-171 | |
| `rnd.structure.dish.tube_alphas` | 0.4, 0.2, 0.2, 0.5 | alpha | | RND-171 | |
| `rnd.structure.grid.solar_glow_count` | 4 | quads | | RND-172 | |
| `rnd.structure.grid.solar_glow_size` | 60 × 40 | wu | | RND-172 | |
| `rnd.structure.grid.solar_glow_base` | 0.5 | brightness | | RND-172 | |
| `rnd.structure.grid.solar_glow_pulse` | 0.5 | brightness × \|sin *g*\| | | RND-172 | |
| `rnd.structure.jump_pad.arrow_length` | 80 | wu | | RND-173 | |
| `rnd.structure.jump_pad.arrow_half_width` | 20 | wu | | RND-173 | |
| `rnd.structure.jump_pad.base_brightness` | 0.3 | × | | RND-173 | |
| `rnd.structure.jump_pad.pulse_brightness` | 0.7 | × | | RND-173 | |
| `rnd.structure.relic.shatter_fraction` | 0.5 | fraction | when a Relic appears | RND-174 | |
| `rnd.structure.wall.shatter_parameter` | 3.0 | shatter parameter | when a wall is destroyed | RND-174 | |
| `rnd.structure.scenery.placeholder_radius` | 40 | wu | scenery without a mesh | RND-174 | |
| `rnd.tree.generator.multiplier` | 214013 | | linear congruential step | RND-181 | |
| `rnd.tree.generator.increment` | 2531011 | | | RND-181 | |
| `rnd.tree.generator.shift` | 16 | bits | | RND-181 | |
| `rnd.tree.generator.mask` | 0x7FFF | | 32767 | RND-181 | |
| `rnd.tree.width_factor` | 0.1 | × segment length | × bud size for leaf segments | RND-182 | |
| `rnd.tree.child_offset_min` | 0.3 | fraction of the parent segment | | RND-182 | |
| `rnd.tree.child_offset_range` | 0.6 | fraction | uniform draw added to the minimum | RND-182 | |
| `rnd.tree.child_length` | 0.4 | × parent length | before the push-out factor | RND-182 | |
| `rnd.tree.sway.serial_phase` | 2 | s per serial | | RND-186 | |
| `rnd.tree.sway.rate` | 2 | rad per s | | RND-186 | |
| `rnd.tree.sway.amplitude` | 0.003 | × height | | RND-186 | |
| `rnd.tree.sway.blighted_amplitude` | 0.01 | × height | | RND-186 | |
| `rnd.tree.essence_pulse.serial_phase` | 10 | s per serial | | RND-187 | |
| `rnd.tree.blighted.branch_colour` | 0, 0, 0, 200 | of 255 | | RND-188 | |
| `rnd.tree.blighting.span` | 10 | blight-timer units | | RND-189 | |
| `rnd.tree.blighted.silhouette_alpha` | 200 | of 255, × *k* | | RND-189 | |
| `rnd.tree.burning.darken_gain` | 10 | × burn level | | RND-190 | |
| `rnd.tree.burning.flame_count` | 4 | billboards | | RND-190 | |
| `rnd.tree.burning.flame_colour` | 0.8, 0.2, 0.1 | RGB | | RND-190 | |
| `rnd.tree.burning.flame_alpha` | 0.5 | × drawn height ÷ height | | RND-190 | |
| `rnd.tree.burning.flame_spread` | 1.7 | wu per index | | RND-190 | |
| `rnd.tree.burning.flame_height` | 0.8 | × drawn height | | RND-190 | |
| `rnd.tree.burning.flame_phase_rate` | 2 | rad per s of presentation frame time | | RND-190 | |
| `rnd.tree.burning.flame_size_gain` | 2 | × drawn height | half-size | RND-190 | |
| `rnd.tree.burning.flame_min_size` | 5 | wu | half-size floor | RND-190 | |
| `rnd.tree.burning.particle_height_step` | 12 | wu per particle | per tick, ÷ structure detail | RND-190 | |
| `rnd.tree.regrow.hide_threshold` | 0.5 | burn level | | RND-190 | |
| `rnd.tree.regrow.fade_gain` | 2 | × \|burn level\| | leaf alpha × (1 − 2\|*b*\|) | RND-190 | |
| `rnd.tree.seasonal_branch_colour` | 180, 100, 50 | of 255 | | RND-191 | DEC-118 |
| `rnd.tree.leaf_alpha_boost` | 1.3 | × per detail level | above level 1, capped at 255 | RND-191 | |
| `rnd.barrier.neutral_colour` | 100, 100, 100, 150 | of 255 | no team | RND-197 | |
| `rnd.barrier.solar_dark` | 20, 20, 20 | of 255 | | RND-197 | |
| `rnd.barrier.solar_team_factor` | 0.9 | × team colour | | RND-197 | |
| `rnd.barrier.solar_pulse_rate` | 4 | rad per s of real time | | RND-197 | |
| `rnd.barrier.alpha` | 150 | of 255 | | RND-197 | |
| `rnd.barrier.status_rate` | 0.3 | status per s | prediction | RND-198 | |
| `rnd.barrier.skirt_fraction` | 1/3 | × post height | below the ground | RND-198 | |
| `rnd.barrier.repeat_factor` | 2 | × full height per repeat | | RND-199 | |
| `rnd.barrier.scroll_period` | 15 | s of presentation frame time | | RND-199 | |
| `rnd.barrier.solar_scroll_period` | 1.5 | s | | RND-199 | |
| `rnd.barrier.solar_repeat` | 7 | repeats | | RND-199 | |
| `rnd.barrier.edge_width` | 2.0 | px | | RND-201 | |
| `rnd.barrier.solar_edge_width` | 0.5 | px | | RND-201 | |
