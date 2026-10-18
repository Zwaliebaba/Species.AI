---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# View, projection and depth conventions

## Context and Problem Statement

Every Species subsystem that touches positions on screen has to agree on a few conventions: the handedness and up axis of world and view space, how the projection maps depth, how matrices are stored and multiplied on the CPU and on the GPU, how the field of view adapts to wide and narrow windows, and the 2-D space the in-match HUD is laid out in. Mixed conventions produce mirrored meshes, inverted culling, wrong picking of units under the cursor, and depth fighting between terrain, decals and shadows. The world is large at the strategic zoom, so depth precision over long view distances matters. Mesh sources arrive in glTF, which is right-handed with +Y up (ADR-019).

The problem is which conventions the renderer, the camera, picking and the mesh cooker share.

## Decision Drivers

- One handedness, one up axis and one depth convention everywhere; conversions happen once, at a boundary.
- Depth precision over long view distances, to avoid fighting between terrain, decals, shadows and overlays.
- Matrices in the same layout on the CPU and in shaders, with no transposes scattered through the code.
- Wide and ultra-wide windows show more of the world to the sides, never less vertically.
- The HUD layout stays the same at every resolution and aspect ratio.

## Considered Options

1. **Right-handed, +Y up, reversed-Z** depth in a 32-bit float buffer, Hor+ field of view.
2. **Left-handed**, +Y up, conventional depth (near 0, far 1).
3. Right-handed, +Y up, **conventional depth** in a 24-bit depth buffer.

## Decision Outcome

Proposed option: **1, right-handed, +Y up, reversed-Z, Hor+**. **Default while Proposed:** the renderer, camera, picking and mesh cooker use these conventions, because they match the mesh source format and give the best depth precision for long views. **Confirmed when:** the unit, golden-image and layout tests of the Confirmation section pass and the first cooked mesh is drawn and picked under these conventions; it is checked before the first mesh is cooked, because a change after that touches the renderer, the camera, picking and the mesh cooker together.

- **Axes.** World and view space are right-handed with +Y up; the camera looks along −Z in view space. Distances are in world units (`wu`). The mesh cooker converts source axes into world axes once, at cook time (ADR-019). If the simulation model's axes differ, the conversion happens once at the boundary between simulation state and presentation, never inside a sub-renderer.
- **Projection and depth.** Perspective projection with **reversed Z**: the near plane maps to depth 1 and the far plane to depth 0. The depth buffer is `D32_FLOAT`, cleared to 0, and the default depth comparison is greater-or-equal (the catalogue default of ADR-005). The far plane stays finite, so culling and fog distances keep a defined end.
- **Depth bias.** Decals, blob shadows and overlays use the pipeline's `DepthBias` and `SlopeScaledDepthBias` with signs chosen for reversed Z (ADR-005), never a change of the projection's near plane. A depth-biased variant has a bias factor *k* above 1 (RND-219), which the renderer converts to a constant bias of round((*k* − 1) × 2²⁴) units with no slope term (RND-220). On the `D32_FLOAT` target a bias unit is 2^(*e* − 23) for a primitive whose largest depth has the binary exponent *e*, so the offset lies between (*k* − 1) and 2 × (*k* − 1) times that depth, so the draw wins depth ties against unbiased coplanar surfaces (https://learn.microsoft.com/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias, checked 2026-10-18).
- **Matrices.** The CPU uses DirectXMath with row vectors. Constant and structured buffers store matrices row-major, every shader compiles with row-major packing (`-Zpr`, RND-260) and multiplies `mul(v, M)`, so CPU and GPU agree without transposes. The one exception is the world transform (RND-015): it is passed as a 3x4 matrix (`Float3x4`) that holds the transpose of the maths library's row-vector `Mat3x4` (PLT-136), written in one place, where presentation code fills GPU data, and applied as `mul(M, float4(p, 1))` (RND-262).
- **Field of view.** The vertical field of view is fixed by the camera; the horizontal field of view grows with the aspect ratio (Hor+). The camera component owns the field-of-view values; the renderer owns the near and far planes and clamps the field of view to its valid range before each projection set-up.
- **HUD space.** The in-match HUD is laid out in an orthographic space 600 units high whose width follows the window's aspect ratio. The front-end UI uses the screen-fraction layout grid of the UI framework (ADR-033).
- **Picking.** Screen-to-world rays are built from the inverse view-projection matrix with reversed-Z depth values; one helper serves the cursor, unit selection and the camera.

### Consequences

- Good, because glTF sources need no handedness flip, only an axis and unit check.
- Good, because reversed Z with a float buffer gives nearly uniform depth precision, which suits long views at the strategic zoom.
- Good, because one matrix layout removes a whole class of transpose errors.
- Good, because the HUD looks the same at any resolution.
- Bad, because DirectXMath's `RH` functions and reversed Z must be used consistently; a single conventional-depth helper anywhere inverts tests.
- Bad, because a 32-bit depth buffer with no stencil costs more memory than a 24-bit buffer (no pass uses stencil).

### Confirmation

- Unit tests check the projection helper (near maps to 1, far to 0), the picking helper against known screen points, and the cooker's axis conversion against a test mesh with marked axes.
- A golden-image test renders a scene with decals and shadows at long and short distances and checks for no depth fighting.
- A layout test renders the HUD at 4:3, 16:9 and 21:9 and checks element positions in HUD units.

## Pros and Cons of the Options

### 1. Right-handed, +Y up, reversed Z, Hor+

- Good, because it matches glTF and gives the best depth precision.
- Bad, because reversed Z requires care in every depth-related helper.

### 2. Left-handed, conventional depth

- Good, because many Direct3D samples use it.
- Bad, because every glTF source needs a handedness flip, and conventional float depth wastes precision far from the camera.

### 3. Right-handed, conventional 24-bit depth

- Good, because 24-bit depth is compact and familiar.
- Bad, because its precision falls off sharply with distance, which causes fighting between terrain, decals and shadows in long views.

## More Information

- Related: ADR-005 (depth comparison and depth bias), ADR-009 (shadows), ADR-019 (mesh source axes), ADR-033 (UI layout and scaling).
- The camera component (`architecture/components/camera_and_sequences.md`) owns the field-of-view values; the renderer component (`architecture/components/renderer/`) owns the near and far planes and the field-of-view clamp.
- **Amendment 2026-10-17.** Ownership of the view values follows the components: the camera owns the field of view (CAM-005, CAM-012), the renderer the near and far planes (RND-211) and the clamp (RND-212). The depth-bias conversion for a raised near plane is the renderer's (RND-220).
- **Amendment 2026-10-18.** The depth-bias variants are stated as bias factors (RND-219, RND-220), not as a raised near plane.
- **Amendment 2026-10-18 (world transforms).** World transforms are passed as `Float3x4`, the transpose of the row-vector `Mat3x4` of the maths library (PLT-136), and applied as `mul(M, float4(p, 1))`: three rows of four fill three registers of root constants in the same layout in C++ and in HLSL, and a row vector of four cannot multiply them. It is the one transposed matrix of the renderer (RND-262); the Matrices bullet says so.
- **Amendment 2026-10-18 (matrix packing).** Row-major packing is a compiler option of every shader (`-Zpr`, RND-260), so no HLSL declaration carries a `row_major` keyword; the Matrices bullet says so.
