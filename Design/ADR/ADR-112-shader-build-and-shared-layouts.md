---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# The shader build and the layouts C++ and HLSL share

## Context and Problem Statement

Task T13 of the [M1 plan](../implementation/m1_plan.md) draws the first frame that is more than a clear. It needs:
- the shader build that ADR-003 and `AGENTS.md` §2 fix in outline: `FXCompile` with the Windows SDK's `dxc` at Shader Model 6.6, into byte arrays in `<Lib>/CompiledShader/`, compiled into the executable;
- the shared C++/HLSL header that ADR-006 requires for every constant and structured-buffer layout.

These decisions are open:
- the compiler's flags, and how Debug and Release keep them identical (BLD-031);
- where the shared header lives, how its HLSL vector types satisfy the naming rules of `AGENTS.md` §1, and how two packings are kept from differing;
- the per-frame and per-draw layouts of ADR-006 as T13 first fills them.

## Decision Drivers

* Debug and Release differ only in optimisation (`AGENTS.md` §3), and CI builds Debug alone.
* One declaration per layout (ADR-006): a field added in C++ and not in HLSL is a defect no compiler sees.
* The names in C++ follow `AGENTS.md` §1, which clang-tidy enforces on every header a translation unit includes.
* Shaders are compiled into the executable, and nothing is loaded from disk at run time (`AGENTS.md` §2).

## Considered Options

* 1 — one header in the NeuronClient folder, read by both compilers, with PascalCase type aliases (`Float4`, `Float4x4`, `Float3x4`, `Uint4`) that HLSL declares with `typedef` and C++ with `using` over DirectXMath
* 2 — an `.hlsli` that C++ also includes
* 3 — separate C++ and HLSL declarations, checked against each other by reflection at start-up

## Decision Outcome

Chosen option: "1".
- Option 2 makes a C++ translation unit include a file kind that `AGENTS.md` R7 does not allow for C++.
- Option 3 finds a mismatch only when the program runs, and only for the layouts reflection can see.

* **The build.** One `FXCompile` item definition with no condition, so every configuration reads the same settings:
  - Shader Model 6.6, entry point `main`, HLSL 2021 (`-HV 2021`);
  - row-major matrices (`-Zpr`, RND-260);
  - warnings as errors;
  - optimisation on and debug information off in every configuration;
  - the header `CompiledShader\<File>.h` with the byte array `g_<File>`, and no object file.

  A build target makes the `CompiledShader` folder before the shaders compile into it; the folder stays build output. Each `.hlsl` names its stage in its item (`ShaderType`). An `.hlsli` is a `None` item, never compiled on its own (BLD-121). PIX shader debugging, which needs debug information, comes with the first shader that needs it and is a later amendment.
* **The shared header** is `NeuronClient/ShaderLayouts.h`. A shader includes it by its path relative to `Shader/`, so the project puts no folder of its own on an include path (`AGENTS.md` §3).
  - HLSL sees `typedef`s and `static const uint` constants; C++ sees `using` aliases of DirectXMath types and `inline constexpr` constants, through one macro each.
  - Every field is a whole 16-byte register (a `Float4`, a matrix or a `Uint4`), so HLSL's packing and C++'s cannot differ. `static_assert`s state each layout's size.
* **The layouts at T13** (ADR-006):
  - **`FrameConstants`** (b0, 336 bytes) holds:
    - the view, projection and view-projection matrices;
    - the camera's front, whose dot product with a position is its view depth;
    - the fog's colour and range;
    - the key and fill lights' directions towards them and their colours, with no ambient term (RND-226);
    - the smooth presentation time and the viewport.
  - **`PassConstants`** (b1) holds eight registers that a pass fills as it needs.
  - **`DrawConstants`** (b2, 20 root constants) holds the draw's transform as a 3x4 matrix (the transpose of the row-vector transform, applied as `mul(world, float4(p, 1))`, RND-262), its tint, and four heap indices.
  - **`SkyLayerInstance`** holds one sky quad: its rectangle, colour, texture coordinates, height and fog, and how it samples. The sky's single draw reads it per instance through the input assembler (ADR-115).
  - Positions are relative to the camera (WLD-246), so the view matrix holds the camera's rotation alone.
* **The shaders at T13** are `Final`, `Sky`, `Terrain`, `SeaFlat` and `SeaWaves`. Each is a vertex and a pixel stage that share one `<Shader>.hlsli`, which declares the constant buffers it reads and its interpolants. Values that one facet holds on all its corners are `nointerpolation`. The full-screen triangle of step 1 is the final pass's vertex stage.
* **The sampler indices** of ADR-018 (linear clamp 0, linear wrap 1, point clamp 2, point-magnify clamp 3) are constants of the header, and the sampler heap creates them at those indices.

### Consequences

* Good, because a layout is declared once, and a mismatch is a compile error in C++ (`static_assert`) or in HLSL (an unknown field).
* Good, because Debug and Release compile identical shaders, so a shader fault seen in a Debug run is the Release fault.
* Bad, because a `Float3`-sized value costs a whole register; the per-frame block is 352 bytes where a packed one would be smaller.
* Bad, because shaders are optimised in Debug too, and PIX's source-level shader debugging waits for an amendment.
* It forecloses: compiling shaders at run time; a shader file loaded from disk; layouts declared twice.

### Confirmation

* CI's `Debug|x64` build compiles every shader with these settings, and fails on any warning.
* The first pipeline state that binds a shader against the root signature of ADR-006 is created on the owner's Adreno with the debug layer silent (M1 plan T13's run).

## Pros and Cons of the Options

### 1 — one header, aliases in both languages

* Good, because both compilers read one file.
* Bad, because the header carries two small macro blocks for the two languages.

### 2 — an `.hlsli` that C++ includes

* Good, because no macro is needed in HLSL.
* Bad, because a C++ translation unit includes a file kind outside R7, which the checkers would not see.

### 3 — separate declarations checked at start-up

* Good, because each language uses its own idiom.
* Bad, because a mismatch is found at run time, on the machine that runs it.

## More Information

* Related: [ADR-003](ADR-003-shader-language-compiler-and-material-shaders.md), [ADR-006](ADR-006-root-signature-and-bindless-resource-access.md), [ADR-018](ADR-018-texture-import-manifest.md), [ADR-090](ADR-090-solution-and-project-layout.md).
* The rules: RND-260, RND-262, BLD-121, `AGENTS.md` §2 and §3.
* The code is in NeuronClient (`ShaderLayouts.h`, `RootSignature`, `DescriptorHeaps`, `PipelineCatalog`, `Shader/`).
