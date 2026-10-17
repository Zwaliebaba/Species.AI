# Edges, normals, colours and sea

> Status: Engine baseline · Prefix: TER · ADRs: [ADR-011](../../../ADR/ADR-011-world-surface-rendering.md) · [ADR-060](../../../ADR/ADR-060-content-identity-by-hash.md) · [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md) · Related: [Terrain](README.md) · [Height-field generation](height_field_generation.md) · [World renderers](../renderer/world_renderers.md) · [Options and budgets](../renderer/options_and_budgets.md) · [Space, routing and physics](../../../reference/12_simulation_model/05_space_routing_and_physics.md)

This document specifies the world grid of the generated height field and its edge semantics, the simulation normal grid, the per-triangle colour-ramp look-up and its presentation generators, the sea derivations (shore mask, depth map, flat-water tile mask, wave meshes and coastline waves), the terrain lifecycle, and the conversions that the output depends on. Section numbers are shared by the files of this folder.

## 1 Responsibilities and boundaries

The responsibilities and boundaries of the terrain component are [README](README.md) §1 (TER-001 to TER-009). The world extent, the edge semantics and the normal grid are simulation data; the colour look-up and the sea derivations are presentation data that never feed the simulation (TER-007).

## 2 Interfaces

The functions specified here are those of the interface sketch of [README](README.md) §2.1: `TerrainField::Height`, `TerrainField::Normal` and the presentation derivations `FacetColor`, `BuildSeaMasks` and `SelectWaveVertexPairs`.

## 3 Data model

The rendering inputs and data products are [README](README.md) §3.6 and §3.7 (table 3.7-A).

## 4 Behaviour and algorithms

### 4.10 World grid, outside height and edge semantics

**TER-150** (MUST) The world grid has the sizes of table 4.10-A.

Table 4.10-A World-grid sizes.

| Quantity | Rule |
|---|---|
| sample spacing | the cell size, rounded to binary32 (TER-066), on both axes |
| grid size | columns = ⌈world size x ÷ cell size⌉, rows = ⌈world size z ÷ cell size⌉ (TER-038) |
| world extent | *X* = cell size × columns, *Z* = cell size × rows, with the origin at (0, 0); vector set A: 2700 ÷ 15.760000228881836 = 171.32, so 172 samples and *X* = 2710.720039367676 wu |
| sample positions | sample (*i*, *j*) lies at world (*i* × cell size, *j* × cell size); the last sample lies at (columns − 1) × cell size, one cell short of the extent |

**TER-151** (MUST) The world extent sizes the simulation's spatial grids and route graphs ([reference/12](../../../reference/12_simulation_model/05_space_routing_and_physics.md) R-SIM-304) and bounds the world clamp (R-SIM-366). It also sets the camera bounds, −`ref.sim.match_entry.camera_border` to *X* + `ref.sim.match_entry.camera_border` on both horizontal axes, the x extent serving for z as well ([camera](../../../reference/06_controls_camera_and_commands/camera.md)); the fog range, from `rnd.fog.min_end` and `rnd.fog.edge_margin` ([view, lighting and fog](../renderer/view_lighting_and_fog.md) RND-222); and the sea plane from (−*X*/2, −*Z*/2) to (1.5 *X*, 1.5 *Z*) (RND-140).

**TER-152** (MUST) The outside height plays the four roles of TER-031: both grids carry it as their outside value, the normal grid with `ter.normal.outside`.

**TER-153** (MUST) Because of TER-073, an interpolated read near or beyond the world grid's edges does **not** return the outside height. For the world grid (origin 0) with *fx* = *x* ÷ cell size, the read gives the results of table 4.10-B, and the same rules apply independently on z (rows).

Table 4.10-B Edge semantics of `Height` and `Normal`.

| Region of *fx* | Result |
|---|---|
| 0 ≤ *fx* < columns − 1 | bilinear inside the grid |
| columns − 1 ≤ *fx* < columns, the last cell | a blend of column columns − 1 and **column 0** (*i*2 is reset to 0) |
| columns ≤ *fx* < 65 536 | column 0 only (both indices are reset to 0) |
| *fx* ≥ 65 536 | the column given by the low 16 bits of the truncated index, an aliasing that needs coordinates beyond 589 000 wu at the smallest reference cell size |
| −1 < *fx* < 0 | *i*1 = 0, *i*2 = 1, with the weight *fx* + 1 on column 1; the result is discontinuous at *x* = 0 and approaches column 1 as *x* approaches 0 from below |
| *fx* ≤ −1 | column 0 only |

Off-map reads therefore return the terrain profile of row 0 and column 0. In every reference terrain every border sample is at or below sea level, so these rules alter only depths under water (for example a border maximum of −9.418 wu against an outside height of −27.89 wu) and never create land at the edge; every negative coordinate reads as sea, which is what restricts the Abductor's wander area (DEC-113). This reference behaviour is the default of DEC-246, which lets Species define off-map heights explicitly (item 7).

**TER-154** (MUST) The sea surface lies at `ref.sim.physics.sea_level` (TER-005) for every consumer: walkability rejects water at it ([reference/12](../../../reference/12_simulation_model/05_space_routing_and_physics.md) R-SIM-335), actors swim and drown at it (R-SIM-365, R-SIM-370), the renderer sinks every terrain vertex below `rnd.terrain.sink_threshold` to `rnd.terrain.sink_height` (RND-133), the shore mask counts a height above it as land (TER-196), and the wave mesh rests at it (TER-201).

**TER-155** (MUST) Every reference terrain has sea along its whole border and on a large share of its samples, with the figures of [rules and conventions](../../../reference/08_maps_and_level_design/rules_and_conventions.md) R-MAP-085 and DEC-247 item 6, so every bounded search for a sea or land position terminates there (DEC-014).

### 4.11 Simulation normals

**TER-170** (MUST) The simulation normal grid is a `SurfaceGrid<Vec3>` on the height grid's columns, rows, origin and cell size, with the outside value `ter.normal.outside`. It is computed once per generation, after the flatten areas, in binary64, with *c* the cell size and *h*(*i*, *j*) = `At`(*i*, *j*) of the height grid (off-grid reads give the outside height):

```text
for j := 0 .. rows - 1:                       # z outer
    for i := 0 .. columns - 1:                # x inner
        hN := h(i, j - 1) ;  hW := h(i + 1, j) ;  hE := h(i - 1, j) ;  hS := h(i, j + 1)
        if hN == hW and hE == hS and hN == hE:          # the centre is not compared
            normal(i, j) := ter.normal.outside           # (0, 1, 0)
            continue
        ctr := h(i, j)
        vN := (0, ctr - hN, c) ;   vW := (-c, ctr - hW, 0)
        vS := (0, ctr - hS, -c) ;  vE := ( c, ctr - hE, 0)
        nA := Normalise(vW x vN)               # TER-077, TER-078
        nB := Normalise(vE x vS)
        normal(i, j) := Normalise(nA + nB)     # component-wise sum
```

**TER-171** (MUST) The labels N, W, E and S name neighbours, not compass directions (W is the +x neighbour and E the −x neighbour). For a plane *h* = *a* × *x* + *b* × *z* both half-normals equal (−*a*, 1, −*b*) ÷ \|(−*a*, 1, −*b*)\|: the true upward normal, leaning downhill. Each half is normalised before the sum, so on curved ground the result weights the (+x, −z) and (−x, +z) one-sided gradients equally, whatever their magnitudes. An isolated spike or pit whose four neighbours are equal gets the up vector. `Normal`(*x*, *z*) blends four grid normals without renormalising (TER-013).

**TER-172** (MUST) The render facet normals are the renderer's (RND-134) and unrelated to the simulation normals, as table 4.11-A summarises; only the simulation normals feed the simulation.

Table 4.11-A Simulation normals and render normals.

| Aspect | Simulation normal grid | Render facet normals |
|---|---|---|
| defined at | every height sample | every terrain triangle |
| precision | binary64 | binary32 |
| input heights | the true heights | heights with vertices below `rnd.terrain.sink_threshold` sunk (RND-133) |
| smoothing | two normalised one-sided cross products | none: the facet normal |
| consumers | bounce physics, the centipede, impacts, particles, orientation to the ground | lighting and the slope axis of the colour ramp (TER-181) |
| simulation relevance | yes | no |

### 4.12 Colour-ramp look-up

**TER-180** (MUST) The colour look-up's height normaliser *highest* (binary32) is the maximum height when it is above 0, and otherwise `Highest`() of the height grid (TER-076). With a maximum height below the real peak, every height above the maximum height uses ramp row 0; in the reference content one map does this, with a maximum height of 200 wu against a peak of 238.0 wu.

**TER-181** (MUST) `FacetColour` gives each terrain triangle of the renderer's mesh (RND-132), in the mesh's build order, one colour from the map's land colour ramp (TER-040), using the render vertex positions after the sink of RND-133 and the triangle's facet normal of RND-134:

```text
(a, b, c) := the triangle's vertices: (p1, p2, q1) for the first triangle of a pair step,
             (p2, q1, q2) for the second                           # RND-132 notation
centre := (a + b + c) * ter.colour.centre_factor          # binary64, summed a, b, c; 0.33333, not 1/3
g      := binary32(facet normal y)                         # 1 = flat, 0 = vertical
u      := binary32(pow32(1 - g, ter.colour.slope_exponent))       # 0.4
hgt    := binary32(centre.y)
v      := binary32(1 - hgt / highest)
r0     := colourVariation.NextUint()                       # TER-185
colourVariation.Seed(uint32(trunc(centre.x)) OR (uint32(trunc(centre.z)) + r0))
v      := binary32(v + colourVariation.NextSigned(ter.colour.jitter_numerator / (max(hgt, 0) + ter.colour.jitter_offset)))
px     := clamp(trunc(u * ter.colour.ramp_size), 0, ter.colour.ramp_size - 1)
py     := clamp(trunc(v * ter.colour.ramp_size), 0, ter.colour.ramp_size - 1)
colour := ramp texel (px, py), py counted from the bottom row as viewed      # TER-040
```

The jitter is one draw, uniform over ±0.225 ÷ (max(hgt, 0) + 2). The sum inside the seed is taken modulo 2³². The renderer lights the facet with this colour and, in the inverted style, gives it alpha 0 (RND-134, RND-136, DEC-040).

**TER-182** (SHOULD) A ramp reads as follows, which artists of `terrain.ramp` follow ([art and audio briefs](../../../assets/art_and_audio_briefs.md)):

- **x axis, slope.** Column 0 is flat ground and column 63 vertical; the exponent 0.4 stretches gentle slopes, with *u* = (1 − cos θ)^0.4 for a facet tilted by θ: 10° uses column 11, 30° column 28, 45° column 39, 60° column 48 and a vertical facet column 63.
- **y axis, height.** Row 0, the bottom row as viewed, is the highest terrain; row 63, the top row, is sea level and below (*v* ≥ 1 clamps there). Heights map linearly onto rows: row = trunc(64 × (1 − *h* ÷ *highest* + jitter)).
- **Jitter.** The colour variation blurs the bands vertically by up to ±0.1125 of the ramp (±7.2 rows) at sea level, decaying to ±0.0022 (±0.14 rows) at a height of 100 wu.

### 4.13 Colour-variation and shore-noise generators

**TER-185** (MUST) The colour pass of one renderer build owns one presentation generator instance (`FxRng`), never shared with another consumer or thread and unreachable from simulation code (R-SIM-608, DEC-013). It uses the algorithm of TER-080, with `FxRng::NextUint` = `Next`() and `FxRng::NextSigned`(*R*) = `Signed`(*R*), and is seeded with `ref.sim.match.presentation_seed` (1, [match lifecycle](../../../reference/12_simulation_model/02_match_lifecycle_and_state_reset.md)) at the start of every build. Each triangle draws *r*0, re-seeds the instance from its centre and *r*0, and draws its jitter (TER-181), so the colours form a chain in build order. Every build of the same field, after device removal included, therefore gives the same colours. The colours are presentation and need not equal the reference's bit for bit (R-SIM-601); their distribution is the reference's. In the reference one shared generator serves the colours, audio, tree geometry and a service thread, so concurrent draws can change the colours, and a mid-match regeneration redraws them without re-seeding; neither happens here (TER-221).

**TER-186** (MUST) The shore noise of the wave mesh of each sea-detail level (TER-202) draws from its own copy of the colour instance, taken at the end of the colour pass, one draw per vertex in build order, as the reference's sea build continues the colour chain.

**TER-187** (MAY) A stateless function of the triangle or vertex position and the presentation seed may replace these chains if the design phase so decides (DEC-246 item 11); the distributions stay those of TER-181 and TER-202.

### 4.14 Sea derivations

All of this section is presentation and is rebuilt from the height field (TER-007); the renderer's passes are RND-140 to RND-148.

**TER-195** (MUST) The sea is built after the terrain mesh, in this order: (1) the sea masks: the shore mask (TER-196, TER-197), then the depth map (TER-198), then the flat-water tile mask (TER-199); (2) for each sea-detail level 1, 2 and 3 that the renderer builds (RND-448): the water cell (TER-200) and the wave-mesh vertex pairs with their depths and shore noise (TER-201, TER-202); then the depth map is released; (3) the wave colour table from the map's shore-wave gradient (TER-041). The wave heights and colours are evaluated once per rendered frame, never by the simulation (TER-204, TER-205, RND-145). At sea detail 0 no wave mesh exists and only the flat layer is drawn.

**TER-196** (MUST) The shore mask has `ter.sea.mask_size` (*S* = 128) × *S* pixels over the sea plane. With the pixel sizes *sX* = 2 *X* ÷ *S* and *sZ* = 2 *Z* ÷ *S* (binary32, exact because the division is by a power of two) and the offset *o* = `ter.sea.mask_land_offset` (32 pixels, the plane's margin of *X*/2), pixel (*x*, *z*) corresponds to the world point (*x* × *sX* − *o* × *sX*, *z* × *sZ* − *o* × *sZ*), so the plane [−*X*/2, 1.5 *X*) × [−*Z*/2, 1.5 *Z*) is covered. The land value of a pixel is 1 when *o* ≤ *x* < *S* − *o*, *o* ≤ *z* < *S* − *o* and `Height` at its world point is above `ref.sim.physics.sea_level`, and 0 otherwise. The mask is computed in binary32.

**TER-197** (MUST) The land values are blurred by the kernel *k*[0 to 10] = `ter.sea.blur_taps` (0.2, 0.3, 0.4, 0.5, 0.8, 1.0, 0.8, 0.5, 0.4, 0.3, 0.2), each divided by `ter.sea.blur_normaliser` (5.4, the sum of the taps), in scatter form: first horizontally, every pixel whose value is not within `ter.sea.blur_skip_tolerance` (10⁻⁶) of 0 adds value × *k*[*i*] to pixel (*x* + *i* − 5, *z*) for *i* from 0 to 10, contributions outside the image being dropped; then vertically, in the same form along z, on the result of the horizontal pass. Each blurred value *V* gives the grey level trunc(min(`ter.sea.mask_max` (255), *V* × `ter.sea.mask_gain` (855))), stored as an opaque grey texel of the shore-mask image at (*x*, *S* − 1 − *z*): the image is **vertically flipped**, so its row 0 (texture *v* = 0) holds mask row 127, at the edge *z* = 1.5 *Z*, in the orientation that RND-142 samples. The result is a soft "distance to the coast" mask, saturated at 255 on and near land and reaching 0 six pixels from a straight coast (one pixel is *X* ÷ 64 wu, so about 0.094 *X*).

**TER-198** (MUST) The depth map is a `SurfaceGrid<float>` of width 2 *X* and height 2 *Z*, with origin (−*X*/2, −*Z*/2), the cell size 2 *X* ÷ *S* on **both** axes (on a square terrain this gives *S* × *S* samples) and the outside value `ter.sea.depth_outside` (1.0). Its sample (*x*, *z*) is 1 − grey(*x*, *z*) ÷ 255, the grey level of mask pixel (*x*, *z*), read back from the flipped image: 1.0 is open sea and 0.0 is on or next to land.

**TER-199** (MUST) The flat-water tile mask is a `Grid2D<bool>` of (depth columns ÷ `ter.sea.flat_tile_ratio`) × (depth rows ÷ `ter.sea.flat_tile_ratio`), 32 × 32 tiles. Tile (*x*, *z*) is false, not drawn, exactly when all 16 depth samples of its block, columns 4*x* to 4*x* + 3 and rows 4*z* to 4*z* + 3, are at or above 1.0; it is stored at row 31 − *z*, the order in which RND-141 lays the tiles out.

**TER-200** (MUST) The water cell of sea-detail level *d* is *wc* = *d* × max(*X*, *Z*) ÷ `ter.sea.cell_divisor` (100), so the twice-size plane is 200, 100 or about 67 cells across at levels 1, 2 and 3 ([options and budgets](../renderer/options_and_budgets.md) RND-448).

**TER-201** (MUST) The wave mesh of level *d* takes, for *zb* from 0 to trunc(2 *Z* ÷ *wc*) − 1 (outer) and *xb* from 0 to trunc(2 *X* ÷ *wc*) − 1 (inner), the vertex pair (*fx*, sea level, *fz*) and (*fx*, sea level, *fz* + *wc*) with *fx* = −*X*/2 + *xb* × *wc* and *fz* = −*Z*/2 + *zb* × *wc*. A pair is emitted when any of the six points (*fx* − *wc*, *fz*), (*fx*, *fz*), (*fx* + *wc*, *fz*), (*fx* − *wc*, *fz* + *wc*), (*fx*, *fz* + *wc*) and (*fx* + *wc*, *fz* + *wc*) is needed, where a point is needed unless `Height` there is above `ter.sea.vertex_land_limit` (4.0 wu) or the bilinear depth there is above `ter.sea.vertex_depth_limit` (0.999). The emitted pairs, in this order, form the triangles of RND-132 between consecutive emitted pairs (RND-144).

**TER-202** (MUST) Each emitted vertex carries its bilinear depth *d*ᵥ and, with *s* = 1 − *d*ᵥ, its shore noise (*s* + `NextSigned`(*s*) × `ter.sea.wave.shore_noise` (0.25)) × `ter.sea.wave.shore_brightening` (250), one draw per vertex at build time, the first vertex of a pair before the second (TER-186).

**TER-203** (MUST) The wave colour table holds the colours of TER-041, with *colours* entries.

**TER-204** (MUST) At presentation time *t*, the time that the renderer supplies (the smooth presentation time, RND-145), the wave tables are, with the amplitude scale *A* = `ter.sea.wave.amplitude_scale` (7.0) and the sizes `tableX` = trunc(2 *X* ÷ *wc* + `ter.sea.wave.table_margin` (2)) and `tableZ` likewise:

```text
for i := 0 .. tableX - 1:  x := i * wc       # measured from the plane origin (-X/2, -Z/2)
    waveX[i] := A * (1.2 * sin32(0.01 * x + 0.65 * t) + 0.9 * sin32(0.03 * x + 1.5 * t))
for i := 0 .. tableZ - 1:  z := i * wc
    waveZ[i] := A * (0.9 * sin32(0.02 * z + 0.75 * t) + 0.65 * sin32(0.03 * z + 1.85 * t))
```

The spatial frequencies, temporal frequencies and amplitudes of the two x terms are `ter.sea.wave.x_terms` (0.01, 0.65, 1.2 and 0.03, 1.5, 0.9) and of the two z terms `ter.sea.wave.z_terms` (0.02, 0.75, 0.9 and 0.03, 1.85, 0.65).

**TER-205** (MUST) The heights and colours of the pairs are, in build order, with the brightness scale *B* = `ter.sea.wave.brightness_scale` (4.0) and the mix weights `ter.sea.wave.colour_mix` (0.7 and 0.3):

```text
prevH1 := 0 ;  prevH2 := 0                    # before the first pair of the mesh
for each pair (v1, v2) with depths d1, d2 and shore noises n1, n2:
    ix := trunc((v1.x + X/2) / wc + ter.sea.wave.index_bias)      # + 0.1
    iz := trunc((v1.z + Z/2) / wc + ter.sea.wave.index_bias)
    v1.y := (waveX[ix] + waveZ[iz]) * d1
    b1   := (prevH1 + prevH2 + v1.y) * B * (1 - d1) + n1 ;  prevH1 := v1.y
    v2.y := (waveX[ix] + waveZ[iz + 1]) * d2
    b2   := (prevH2 + prevH1 + v2.y) * B * (1 - d2) + n2 ;  prevH2 := v2.y
    colour(v1) := table[clamp(trunc(0.7 * b2 + 0.3 * b1), 0, colours - 1)]
    colour(v2) := table[clamp(trunc(0.7 * b1 + 0.3 * b2), 0, colours - 1)]
```

*b*2 uses the updated prevH1. The colour index truncates toward zero, which biases it by half an entry towards the lower indices, the darker end of the reference gradients. The cross-mix of the two brightnesses softens the sawtooth of flat-shaded facets. The renderer evaluates each pair's predecessor from the same formulae, keeps duplicated join vertices at the heights of the vertices they duplicate, and assigns facet colours (RND-145); binary32 GPU arithmetic is acceptable for this presentation result.

**TER-206** (SHOULD) The waves are two travelling sinusoids per axis, with peak amplitudes of 7 × (1.2 + 0.9) = 14.7 wu along x and 7 × (0.9 + 0.65) = 10.85 wu along z, ±25.55 wu combined, scaled by the depth so that they vanish at the shore. The brightness is a wave-height proxy times the "shoreness" 1 − depth plus the per-vertex shore whitening, so deep water is dark and coasts are bright.

### 4.15 Lifecycle

**TER-220** (MUST) The terrain follows the lifecycle of table 4.15-A.

Table 4.15-A Terrain lifecycle.

| Event | Effect |
|---|---|
| match-world initialisation | step 5 generates the field (TER-060) and the sea renderer builds; step 7 places the map-placed structures on the terrain (R-SIM-016) |
| a presentation option change (sea, sky, structure or entity detail, inverted style) | nothing is regenerated or rebuilt: the field is unchanged, and the renderer selects among the meshes it built at initialisation (RND-031, RND-448) |
| device removal | the renderer rebuilds its buffers from the unchanged field (RND-034); the colour chain is re-seeded (TER-185), so the colours are unchanged |
| an edit in the map editor | the editor regenerates the field from the edited records (§8.2) |
| match-world teardown | the field, its grids, the sea masks and the wave meshes are released with the match world (DEC-011) |

**TER-221** (MUST) The height field is never regenerated during a match, and never under a different numeric environment (DEC-246 item 8, DEC-240). In the reference, applying graphics or other options in the middle of a match regenerates the field and the sea; the heights come out identical only because generation is a pure function and the numeric environment is unchanged, while the colours change because the colour chain is not re-seeded.

**TER-222** (MUST) Generation may run on a loader or job thread. That thread runs with the numeric environment of TER-061, set and asserted there (DEC-224 item 4), and no reader sees the field before generation completes (TER-010). In the reference the whole map load runs on one loader thread while the main thread draws the loading view. How Species loads and streams world content is DEC-226.

### 4.16 Conversions the output depends on

**TER-230** (MUST) Implementation-defined and undefined conversions shape the output; table 4.16-A states the Species rule for each.

Table 4.16-A Conversions and their rules.

| Construct | Where | In the reference content | Rule |
|---|---|---|---|
| binary64 to sampler index for −1 < *v* < 0: truncation gives 0 | tile sampler (TER-121) | yes, on 22 771 merge reads | reproduce exactly; never use ⌊*v*⌋ (TER-071) |
| binary64 to index for *v* ≤ −1, undefined; the reference keeps the low 16 bits | tile sampler; the start index of a parked tile | the sampler: never; parked tiles: 2, without effect | skip a tile wholly outside the grid; reject a partly overlapping one (TER-124) |
| a negative integer index reduced to unsigned 16 bits in `At` and `Set` | every grid | yes: neighbour reads at row and column 0, flatten areas at the edges | every index outside the grid is outside (TER-072) |
| the far-edge reset of `Sample` to column or row 0 | the tile sampler and the world queries | the tile sampler: yes, equivalent to clamping (TER-121); the world: only under water (TER-153) | reproduce for tiles (or clamp, which is equivalent); for the world, the reference rule is the default of DEC-246 |
| signed 32-bit overflow in the generator | the tile generator | yes, constantly | unsigned 32-bit arithmetic (TER-068) |
| the integer conversion of ceilLog2(0) = −∞ | tiles smaller than 2 cell sizes | no | reject at build (TER-112) |
| an unset base value for a method other than 0, 1 or 2 | `Square`, `Diamond` | no | reject at build (TER-112) |
| a guide grid with too few cells | the guide-grid decoder | no | reject at build (TER-112) |
| the single-precision absolute value of the base height | the noise envelope | yes, on every call | reproduce (TER-065) |
| binary32 rounding of decimal fields | the parser | yes; it moves footprints (TER-066) | reproduce (TER-066) |

## 5 Concurrency and performance budget

The memory of the grids and the sea masks and the cost of the queries are [README](README.md) §5 (TER-241, TER-243).

## 6 Failure modes, diagnostics and telemetry

A missing ramp, sea tile or gradient falls back to the map default ([README](README.md) TER-251); the grid digests are TER-252.

## 7 Acceptance tests

The tests of this document are [README](README.md) §7.1: T5, T15, T17 to T21 and T24.

## 8 Open decisions

The decisions are [README](README.md) §8.1; the design-phase inputs on off-map heights, cosmetic noise and the sea at world scale are README §8.3 items 7, 8 and 10.
