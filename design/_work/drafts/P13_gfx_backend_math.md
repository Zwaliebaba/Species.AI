# P13_gfx_backend_math — GL/D3D9 backend, window managers, video capture, math library

Scope: `code/lib/opengl_directx{.cpp,.h,_inline.h,_internals.h,_stubs.cpp,_dlist*,_dlist_dev*,_matrix_stack*}`, `code/lib/ogl_extensions{.cpp,.h,_directx.cpp}`, `code/lib/opengl_trace.*`, `code/lib/safegl.*`, `code/lib/window_manager{,_directx,_win32}.*`, `code/lib/avi_generator.*`, `code/lib/avi_player.*`, `code/lib/math_utils.*`, `code/lib/matrix33.*`, `code/lib/matrix34.*`, `code/lib/vector2.*`, `code/lib/vector3.*`, `code/lib/plane.*`, `code/lib/invert_matrix.*`, `code/lib/rgb_colour.*`. Supporting evidence read in full: `data/shaders/FixedFunc{0,1,2}.vs`, `data/shaders/FixedFunc{0,1,2}Maps.ps`, `data/shaders/globals.h`, `code/globals.h`, `code/lib/FixedPipeline.*`, `code/lib/shader.*`, `contrib/systemiv/lib/math/math_stubs.*`, `docs/OpenGLState.txt`, `docs/HardwareComplianceIssues.txt`; partially: `code/renderer.cpp` (Initialise, SetScreenGamma, Check/SetOpenGLState), `code/interface/prefs_screen_window.cpp` (RestartWindowManagerAndRenderer), `code/landscape_renderer.cpp` (VBO path), `code/main.cpp` (StandardiseCPUPrecision), `code/lib/universal_include.h` (GL/D3D header switch, iv_* macros), `targets/vs2005/Darwinia.vcproj` (compiler FP flags). Build baseline: see build_matrix.md (`Release|Win32` OpenGL + `TARGET_PC_FULLGAME`; secondary `ReleaseDirect3D` = `USE_DIRECT3D`).

Conventions: all paths relative to the source root. "GL" = OpenGL 1.x fixed-function API as consumed by game code. "Emulator" = the `OpenGLD3D` layer that re-implements that API on Direct3D 9.

---

## [→ 24_rendering] Backend architecture — overview and build selection

### How the backend is chosen [C]

The game's renderer code is written once against the **OpenGL 1.x immediate-mode API** (plus two ARB extensions). The backend is chosen at compile time purely by which header set `code/lib/universal_include.h` pulls in:

| Build | Define | Headers included by `universal_include.h` | What `glXxx()` calls resolve to |
|---|---|---|---|
| `Release|Win32` / `Debug|Win32` **[B:FULL]** | (none) | `<GL/gl.h>`, `<GL/glu.h>` | Real `opengl32.dll` / `glu32.dll` entry points (WGL context created by `WindowManagerWin32`) |
| `ReleaseDirect3D|Win32` / `DebugDirect3D|Win32` **[B:D3D]** | `USE_DIRECT3D` | `<d3d9.h>`, `<d3dx9.h>`, `lib/opengl_directx.h`, `lib/safegl.h` | Free functions of the same names implemented in `code/lib/opengl_directx*.cpp` on top of `IDirect3DDevice9` + HLSL shaders |
| Mac OS X **[B:DEAD]** | `TARGET_OS_MACOSX` | `<OpenGL/gl.h>`, `<OpenGL/glu.h>`, `lib/safegl.h` | Apple OpenGL |

Evidence: `code/lib/universal_include.h` (block after the `TARGET_OS_MACOSX` section: `#ifdef USE_DIRECT3D … #include "lib/opengl_directx.h" … #else #include <GL/gl.h>`). [C]

`opengl_directx.h` re-declares every GL entry point the game uses **without `APIENTRY`/`__stdcall` DLL linkage** and with its own `#define`s for every GL enum used (the header comment explains the DLL-linkage reason) [C] `code/lib/opengl_directx.h`. Two signatures are *extended* relative to real GL: `glTexImage2D(…, bool _compressed = false)` and `gluBuild2DMipmaps(…, bool _compressed = false)` (the extra flag selects DXT1) [C].

### Compilation guards per file [C]

All files below are listed for **all four** VS configurations in `targets/vs2005/Darwinia.vcproj` (manifest `vcproj_build=all`); their bodies are switched by preprocessor guards:

| File | Guard | Content in baseline [B:FULL] |
|---|---|---|
| `code/lib/opengl_directx.cpp`, `_dlist.cpp`, `_dlist_dev.cpp`, `_matrix_stack.cpp`, `_stubs.cpp`, `opengl_trace.cpp`, `ogl_extensions_directx.cpp`, `window_manager_directx.cpp` | `#ifdef USE_DIRECT3D` | empty translation unit |
| `code/lib/FixedPipeline.cpp`, `code/lib/shader.cpp` (P12) | `#ifdef USE_DIRECT3D` | empty |
| `code/lib/ogl_extensions.cpp`, `code/lib/window_manager_win32.cpp` | `#if !defined USE_DIRECT3D` | live |
| `code/lib/safegl.cpp` | unguarded; inner `#ifndef USE_DIRECT3D` | compiled, but `safeGlNewList/EndList` are **never reached** in baseline because the `#define glNewList safeGlNewList` lives in `safegl.h`, which is only included for D3D/Mac |
| `code/lib/window_manager.cpp` | none | live (abstract base) |
| `code/lib/avi_generator.cpp` | `#ifdef AVI_GENERATOR` (defined only by `TARGET_DEBUG`) | empty in FULL ([B:DEBUG]) |
| `code/lib/avi_player.cpp` | `#ifdef EGAMESBUILD` (never defined) | empty ([B:DEAD]) |
| math files (`math_utils`, `matrix33`, `matrix34`, `vector2`, `vector3`, `plane`, `invert_matrix`, `rgb_colour`) | none | live in all builds |

Not in the build at all (manifest `vcproj_build` empty): `code/lib/ogl_extensions_sdl.cpp`, `code/lib/window_manager_sdl.{cpp,h}` → Appendix.

### Layer diagram

```mermaid
flowchart TD
  G["Game render code<br/>(renderer.cpp, location.cpp, shape.cpp, landscape_renderer.cpp, …)<br/>calls glBegin/glVertex/glEnable/glCallList/gglBindBufferARB …"]
  G -->|"[B:FULL]"| GL["opengl32.dll + glu32.dll<br/>(driver fixed-function pipeline)"]
  G -->|"[B:D3D]"| EMU["OpenGLD3D emulator<br/>code/lib/opengl_directx*.cpp"]
  EMU --> ST["State cache<br/>s_currentAttribs, s_textureStates[2], hints, cull"]
  EMU --> MS["MatrixStackActual ×2<br/>(ID3DXMatrixStack)"]
  EMU --> DL["DisplayListDevice (recorder)<br/>→ DisplayList (VB + commands)"]
  EMU --> FP["FixedPipeline<br/>selects FixedFunc0/1/2 HLSL"]
  FP --> SH["Shader (D3DXCompileShader + constant table)"]
  EMU --> D3D["IDirect3DDevice9<br/>(DrawPrimitiveUP / DrawPrimitive, render & sampler states)"]
  SH --> D3D
  G -.->|"some files bypass the emulator under USE_DIRECT3D<br/>(landscape_renderer, water, water_reflection, deform, radardish, renderer, texture)"| D3D
```

Files that talk to `OpenGLD3D::g_pd3dDevice` directly under `USE_DIRECT3D` (outside this partition): `code/landscape_renderer.cpp`, `code/water.cpp`, `code/water_reflection.cpp`, `code/deform.cpp`, `code/renderer.cpp`, `code/worldobject/radardish.cpp`, `code/lib/texture.cpp`, `code/lib/shader.cpp`, `code/lib/FixedPipeline.cpp` [C] (grep `OpenGLD3D::|g_pd3dDevice`). 46 files under `code/` contain `USE_DIRECT3D` [C].

### Is the D3D configuration buildable at this commit? [I]

Two pieces of evidence suggest `ReleaseDirect3D` does **not** compile unmodified at commit `f909630`:

1. `WindowManager::CreateWin` is pure-virtual with a `const wchar_t *_title` parameter [C] `code/lib/window_manager.h:WindowManager::CreateWin`, while `WindowManagerDirectX::CreateWin` takes `const char *_title` [C] `code/lib/window_manager_directx.h`. The derived function therefore *hides* rather than overrides; `WindowManagerDirectX` stays abstract and `new WindowManagerDirectX()` in `code/lib/window_manager_directx.cpp:WinMain` is ill-formed (MSVC C2259). `WindowManagerWin32` was updated to `wchar_t` and uses `RegisterClassW/CreateWindowW` [C]; the D3D variant was not.
2. `glLineWidth` sets `D3DRS_LINEWIDTH` [C] `code/lib/opengl_directx.cpp:glLineWidth`; to my knowledge this render state is not in the PC DirectX 9 SDK `D3DRENDERSTATETYPE` (it exists on Xbox-family D3D), and it is not defined anywhere in the tree [C grep]. Comment `// autogenmipmap ignored by xbox` in `d3dCreateTexture` corroborates Xbox-360 heritage of this layer.

Resolution: attempt a `ReleaseDirect3D` build with the DirectX SDK (March 2008) headers named in the vcproj include path. Logged in Unknowns. The D3D path is nevertheless documented in full below because the build matrix designates it the secondary configuration and it encodes Introversion's own "GL-on-D3D" design.

---

## [→ 24_rendering] Native OpenGL path [B:FULL]

### API surface used [C]

The baseline renders entirely with the OpenGL 1.1–1.3 fixed-function pipeline: immediate mode (`glBegin/glEnd`), display lists (`glNewList/glCallList`), client vertex arrays (`glVertexPointer` + `glDrawArrays`), fixed-function lighting (2 lights, `GL_COLOR_MATERIAL`), linear fog, alpha test, blending, `glCopyTexImage2D` render-to-texture, `glReadPixels`, `gluBuild2DMipmaps`, `gluPerspective`, `gluLookAt`, `gluOrtho2D`, `gluProject/gluUnProject`. No GLSL/ARB programs are used in the baseline [C] (P12 confirms all shader code is `USE_DIRECT3D`-only). The two extension families accessed through function pointers are `GL_ARB_multitexture` and `GL_ARB_vertex_buffer_object` (next section).

The default-state contract asserted by the renderer (`Renderer::SetOpenGLState`, `Renderer::CheckOpenGLState`, owned by P12) is cross-checked against `docs/OpenGLState.txt` under *Docs-vs-code discrepancies*.

### Extension handling — `code/lib/ogl_extensions.cpp` [B:FULL] [C]

`InitialiseOGLExtensions()` is called once per renderer creation from `Renderer::Initialise` immediately after window creation [C] `code/renderer.cpp:Renderer::Initialise`. It:

1. Loads every function pointer below with `wglGetProcAddress` (no check of the extension string first; a missing entry point simply leaves the pointer `NULL`).
2. Reads `glGetString(GL_VENDOR/GL_RENDERER/GL_VERSION/GL_EXTENSIONS)` into static buffers `s_glVendor[256]`, `s_glRenderer[256]`, `s_glVersion[256]`, `s_glExtensions[4096]` (`strncpy` + forced NUL ⇒ **extension string silently truncated at 4095 chars**) and logs them with `AppDebugOut`.

| Global pointer | Loaded name | Extension | Consumers (outside scope) | Behaviour if missing |
|---|---|---|---|---|
| `gglMultiTexCoord2fARB` | `glMultiTexCoord2fARB` | GL_ARB_multitexture | `render_utils.cpp`, `water.cpp`, `laserfence.cpp`, `feedingtube.cpp`, `radardish.cpp`, `taskmanager_interface_gestures.cpp`, `deform.cpp` | **No guard anywhere** — call through `NULL` ⇒ crash [C grep: no `if (gglActiveTextureARB)` in `code/`] |
| `gglActiveTextureARB` | `glActiveTextureARB` | GL_ARB_multitexture | same | same |
| `gglBindBufferARB` | `glBindBufferARB` | GL_ARB_vertex_buffer_object | `landscape_renderer.cpp` | `LandscapeRenderer` falls back to display lists if pointer is NULL or extension unsupported |
| `gglDeleteBuffersARB` | `glDeleteBuffersARB` | VBO | landscape | (as above) |
| `gglGenBuffersARB` | `glGenBuffersARB` | VBO | landscape | (as above) |
| `gglIsBufferARB` | `glIsBufferARB` | VBO | none found | — |
| `gglBufferDataARB` | `glBufferDataARB` | VBO | landscape | (as above) |
| `gglBufferSubDataARB` | `glBufferSubDataARB` | VBO | none found | — |
| `gglGetBufferSubDataARB` | `glGetBufferSubDataARB` | VBO | none found | — |
| `gglMapBufferARB` | `glMapBufferARB` | VBO | none found | — |
| `gglUnmapBufferARB` | `glUnmapBufferARB` | VBO | none found | — |
| `gglGetBufferParameterivARB` | `glGetBufferParameterivARB` | VBO | none found | — |
| `gglGetBufferPointervARB` | `glGetBufferPointervARB` | VBO | none found | — |
| `gglChoosePixelFormatARB` | `wglChoosePixelFormatARB` | WGL_ARB_pixel_format | **none** — loaded but never called [C grep] (so no FSAA pixel format is ever requested) | — |

Constants declared in `ogl_extensions.h` but used only as enum values: `GL_COMBINE_EXT 0x8570`, `GL_COMBINE_RGB_EXT 0x8571`, `GL_COMBINE_ALPHA_EXT 0x8572`, `GL_ADD_SIGNED_EXT 0x8574`, `GL_INTERPOLATE_EXT 0x8575`, `GL_CONSTANT_EXT 0x8576`, `GL_SOURCE0_ALPHA_ARB 0x8588`, `GL_TEXTURE0_ARB 0x84C0`, `GL_TEXTURE1_ARB 0x84C1`; `WGL_*_PBUFFER_*` (0x202D–0x2036, pbuffer API commented out); full `GL_*_BUFFER_*_ARB` enum set (0x8892–0x88EA, 0x8764/0x8765, 0x88B8–0x88BD); `WGL_SAMPLE_BUFFERS_ARB 0x2041`, `WGL_SAMPLES_ARB 0x2042`, `WGL_DOUBLE_BUFFER_ARB 0x2011`, `WGL_ACCELERATION_ARB 0x2003`, `WGL_FULL_ACCELERATION_ARB 0x2027`, `GL_MULTISAMPLE_ARB 0x809D`, `WGL_DRAW_TO_WINDOW_EXT 0x2001`, `WGL_SUPPORT_OPENGL_EXT 0x2010`, `WGL_COLOR_BITS_EXT 0x2014`, `WGL_ALPHA_BITS_EXT 0x201B`, `WGL_DEPTH_BITS_EXT 0x2022`, `WGL_STENCIL_BITS_EXT 0x2023` [C] `code/lib/ogl_extensions.h`.

**Query functions** [C]:

- `IsOGLExtensionSupported(name)`: returns 0 if `IsOGLExtensionBlacklisted(name)`; else performs a whole-word search of `s_glExtensions` (match must be preceded by start-of-string or space and followed by space or NUL); names containing a space or empty names return 0.
- `IsOGLExtensionBlacklisted(name)`: only ever blacklists `"GL_ARB_vertex_buffer_object"`, when `GL_VENDOR` contains `"Intel"` and `GL_RENDERER` contains any of the Intel list, or `GL_VENDOR` contains `"SiS"` and renderer contains any SiS list entry (substring match):

| Vendor | Renderer substrings (exact strings from `blacklistIntel[]` / `blacklistSiS[]`) | Source annotation in code |
|---|---|---|
| Intel | `Intel 945GM`, `Intel 965/963`, `Intel Bear Lake B` | "Reported by users" (Bear Lake B = G33/G31, GMA 3100) |
| Intel | `Intel 82810`, `Intel 82815`, `Intel 82830M`, `Intel 82845G`, `Intel 82845PE`, `Intel 82852`, `Intel 82855`, `Intel 82865G`, `Intel 82910GL`, `Intel 82915G`, `Intel 82945G`, `Intel 910GML`, `Intel 915GM`, `Intel 945`, `Intel 963`, `Intel 965` | "Untested" |
| SiS | `Mirage Graphics3` | "Reported by users" (SiS Mirage 3) |

(A second, commented-out list of full Windows driver names and a commented "Ok Intel card?" list — G965, 946GZ, Q35 — exist [C].)

- `IsOGLExtensionPerfectlySupported(name)`: `IsOGLExtensionSupported(name)`, but additionally returns 0 for `GL_ARB_vertex_buffer_object` whenever the vendor string contains `"Intel"` **or** `"SiS"` (any renderer) [C].

**Every extension query in the code and its fallback** [C]:

| Query site | Extension | Fallback when absent |
|---|---|---|
| `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer` (pref `RenderLandscapeMode` = −1, the default) | `IsOGLExtensionPerfectlySupported("GL_ARB_vertex_buffer_object")` | `RenderModeDisplayList` instead of `RenderModeVertexBufferObject` (so all Intel/SiS GPUs get display lists by default) |
| same, after mode chosen | `!gglBindBufferARB || !IsOGLExtensionSupported("GL_ARB_vertex_buffer_object")` | forced to `RenderModeDisplayList` even if the pref asked for VBO |
| `code/lib/math_utils.cpp:RequirePowerOfTwoTextures` | `strstr(glGetString(GL_EXTENSIONS), "GL_ARB_texture_non_power_of_two")` (raw substring, cached in a function-static) | returns true ⇒ `code/lib/bitmap.cpp` rescales textures to `nearestPowerOfTwo`; `code/renderer.cpp` uses POT-sized `glCopyTexImage2D` and scales tex-coords by `screenW/nearestPowerOfTwo(screenW)` |
| `code/lib/math_utils.cpp:RequireSquareTextures` | `strstr(…, "GL_ARB_texture_rectangle")` | returns true ⇒ `bitmap.cpp` makes textures square |
| `GL_ARB_multitexture` | never queried | none (crash if absent; see table above) |
| pref `ManuallyScaleTextures` (default 0) | — | forces POT rescale regardless of extensions [C] `code/lib/bitmap.cpp` (P12) |

Under `USE_DIRECT3D` both `Require*Textures` return `false` immediately [C].

### WGL context [B:FULL]

Created by `WindowManagerWin32::EnableOpenGL` — documented in the 31 section below (pixel format, secondary context, no vsync control, no MSAA).

### `safegl` [C]

`code/lib/safegl.h` redefines `glNewList`→`safeGlNewList` and `glEndList`→`safeGlEndList`. `safeGlNewList` (non-D3D only) logs `"glNewList called from non-mainthread (bad for OpenGL)\n"` if `NetGetCurrentThreadId() != g_app->m_mainThreadId`, then **locks the global `NetMutex g_openGLMutex`** and calls the real `glNewList`; `safeGlEndList` calls `glEndList` then unlocks. The same mutex is taken by `LoadingScreen::RenderFrame` [C] `code/loading_screen.cpp` (P20). Purpose [I]: serialise display-list compilation (done on the loading thread) against the loading-screen renderer. Because `safegl.h` is included only for `USE_DIRECT3D` and Mac, **in the baseline the wrapper is inactive**: display lists compile without the lock, and the loading screen's lock protects nothing [C for include topology; I for consequence].

### Hardware-compatibility fallbacks summary [B:FULL]

| Problem | Mechanism | Ref |
|---|---|---|
| Intel/SiS VBO driver bugs | blacklist + "perfectly supported" ⇒ display lists | `ogl_extensions.cpp` |
| No NPOT textures | rescale to POT; POT copy-tex | `math_utils.cpp:RequirePowerOfTwoTextures` |
| No rectangle textures | square textures | `math_utils.cpp:RequireSquareTextures` |
| Requested mode fails | 4-stage `CreateWin` retry chain | `renderer.cpp:Renderer::Initialise` (see 31) |
| Long extension strings | truncated at 4095 chars (risk: late-listed extensions reported missing) | `InitialiseOGLExtensions` |
| Multitexture missing | **none** | — |

`docs/HardwareComplianceIssues.txt` contains only: *"Johnny's Linux Machine: – Multitexturing ARB not supported – Mouse pointer background not transparent"* [C]. Neither issue has a code-side fallback on Win32 (see discrepancies).

---

## [→ 24_rendering] Direct3D 9 path: OpenGL-API emulation layer [B:D3D]

### Module map [C]

| File | Namespace / classes | Role |
|---|---|---|
| `code/lib/opengl_directx.h` | — | GL type/enum `#define`s and prototypes for every emulated entry point; includes `_inline.h` |
| `code/lib/opengl_directx_inline.h` | — | `__forceinline` wrappers: `glColor*`, `glNormal*`, `glVertex*`, `glTexCoord*`, `glFogf/glFogi`, template `openGLMatrixToDirect3D` |
| `code/lib/opengl_directx_internals.h` | `OpenGLD3D::CustomVertex`, externs `g_pd3dDeviceActual`, `g_pd3dDevice`, `g_currentVertexBuffer`, `g_supportsHwVertexProcessing`, `glActiveTextureD3D`, `glMultiTexCoord2fD3D` | shared internals |
| `code/lib/opengl_directx.cpp` | `OpenGLD3D::TextureState`, `CurrentAttributes`, `Attributes`, `MipMapTask`; device globals | core emulator + device lifecycle |
| `code/lib/opengl_directx_stubs.cpp` | — | no-op stubs |
| `code/lib/opengl_directx_matrix_stack.*` | `MatrixStack` (abstract), `MatrixStackActual`, `MatrixStackDisplayList` | GL matrix stacks |
| `code/lib/opengl_directx_dlist_dev.*` | `DisplayListDevice : IDirect3DDevice9` | records draws while a list is open |
| `code/lib/opengl_directx_dlist.*` | `Command` (abstract) + `CommandDrawPrimitive`, `CommandLoadTransform`, `CommandMultiplyTransform`, `CommandPushMatrix`, `CommandPopMatrix`; `DisplayList` | compiled display list + playback |
| `code/lib/ogl_extensions_directx.cpp` | — | ARB multitexture + VBO emulation, extension query stubs |
| `code/lib/FixedPipeline.*` (P12 file, behaviour documented here) | `FixedPipeline` | fixed-function emulation: shader selection + uniform upload |
| `code/lib/shader.*` (P12) | `Shader` | HLSL compile/bind/constant-table helper |
| `code/lib/opengl_trace.*` | — | debug call tracing |

```mermaid
classDiagram
  class MatrixStack{<<abstract>> +m_transformType +m_modified +Load() +Multiply() +Push() +Pop() +SetTransform() +GetTransform() +FastSetTransform()}
  class MatrixStackActual{-ID3DXMatrixStack* m_matrixStack}
  class MatrixStackDisplayList{-DisplayListDevice* m_device -MatrixStack* m_actual}
  MatrixStack <|-- MatrixStackActual
  MatrixStack <|-- MatrixStackDisplayList
  class IDirect3DDevice9
  class DisplayListDevice{-unsigned m_id -vector~Command*~ m_commands -vector~CustomVertex~ m_vertices +DrawPrimitiveUP() +Record*() +Compile()}
  IDirect3DDevice9 <|-- DisplayListDevice
  class Command{<<abstract>> +Execute()}
  Command <|-- CommandDrawPrimitive
  Command <|-- CommandLoadTransform
  Command <|-- CommandMultiplyTransform
  Command <|-- CommandPushMatrix
  Command <|-- CommandPopMatrix
  class DisplayList{-IDirect3DVertexBuffer9* m_pVertexBuffer -Command** m_commands +Draw()}
  DisplayListDevice ..> DisplayList : Compile()
  DisplayList o-- Command
  class FixedPipeline{+m_fogEnabled +m_fogStart +m_fogEnd +m_lighting +m_lightEnabled[2] +m_material*[] +State m_state +State m_oldState -Shader* m_fixedShader[3] -Shader* m_bound -m_enabled +Bind() +Unbind() +Disable() +Enable()}
  FixedPipeline o-- Shader
```

### Device globals [C] `code/lib/opengl_directx.cpp` (namespace `OpenGLD3D`)

| Global | Type | Meaning |
|---|---|---|
| `g_pD3D` | `LPDIRECT3D9` | `Direct3DCreate9(D3D_SDK_VERSION)`; created once, kept across window recreation |
| `g_d3dpp` | `D3DPRESENT_PARAMETERS` | last present parameters (reused for `Reset`) |
| `g_pd3dDevice` | `LPDIRECT3DDEVICE9` | **current command target** — the real device, or the `DisplayListDevice` recorder while `glNewList` is open |
| `g_pd3dDeviceActual` | `LPDIRECT3DDEVICE9` | the real device |
| `g_supportsHwVertexProcessing` | `bool` (default true) | `caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT` |
| `g_currentVertexBuffer` | `IDirect3DVertexBuffer9 **` | slot of the VBO bound by `glBindBufferD3D` (in `ogl_extensions_directx.cpp`) |

### Vertex format [C]

`OpenGLD3D::CustomVertex` (44 bytes) and matching `D3DVERTEXELEMENT9 s_customVertexDesc[]` (vertex declaration created in `InitialiseData`, set after `BeginScene` and after every reset):

| Offset | Field(s) | Decl type | Usage |
|---|---|---|---|
| 0 | `float x, y, z` | FLOAT3 | POSITION0 |
| 12 | `float nx, ny, nz` | FLOAT3 | NORMAL0 |
| 24 | `unsigned char b8, g8, r8, a8` | D3DCOLOR (BGRA in memory = ARGB dword) | COLOR0 |
| 28 | `float u, v` | FLOAT2 | TEXCOORD0 |
| 36 | `float u2, v2` | FLOAT2 | TEXCOORD1 |

### State cache — what GL state is tracked and where it goes [C]

| GL state | Emulator storage | D3D9 target |
|---|---|---|
| Current colour (`glColor*`) | `s_currentAttribs.r8/g8/b8/a8` (float→byte: `x<0→0`, `x>1→255`, else `(unsigned char)(x*255)` truncation) | baked into each vertex's COLOR0 |
| Current normal (`glNormal*`) | `s_currentAttribs.nx/ny/nz` **negated** (comment: LH/RH) | NORMAL0 |
| Current texcoord unit 0 / 1 | `u,v` / `u2,v2` (`glTexCoord*`, `glMultiTexCoord2fD3D`) | TEXCOORD0 / TEXCOORD1 |
| Alpha reference | `s_currentAttribs.alphaRef` (clamped float) | `D3DRS_ALPHAREF = DWORD(clamp(ref,0,1)*255)` |
| Alpha func | — | `D3DRS_ALPHAFUNC` (`GL_LEQUAL→LESSEQUAL`, `GL_GREATER→GREATER`; others assert→LESSEQUAL) |
| Alpha test enable | — | `D3DRS_ALPHATESTENABLE` |
| Blend enable / func | — | `D3DRS_ALPHABLENDENABLE`, `D3DRS_SRCBLEND`, `D3DRS_DESTBLEND` |
| Depth test / func / mask | — | `D3DRS_ZENABLE` (`D3DZB_TRUE/FALSE`), `D3DRS_ZFUNC`, `D3DRS_ZWRITEENABLE` |
| Cull enable | `s_currentAttribs.backCullingEnabled` | `D3DRS_CULLMODE = s_cullMode` or `D3DCULL_NONE` |
| Front face | `s_ccwFrontFace` (default true) → `s_cullMode` (`GL_CCW→D3DCULL_CW`, `GL_CW→D3DCULL_CCW`) | `D3DRS_CULLMODE` when culling enabled |
| Polygon mode | — | `D3DRS_FILLMODE` (`GL_FILL→SOLID`, `GL_LINE→WIREFRAME`; face argument ignored) |
| Polygon offset enable / params | `polygonOffsetEnabled`, `depthBias=units`, `slopeDepthBias=factor` | `D3DRS_DEPTHBIAS`, `D3DRS_SLOPESCALEDEPTHBIAS` (see defect D11) |
| Scissor enable / rect | — | `D3DRS_SCISSORTESTENABLE`, `SetScissorRect` |
| Clip planes 0–2 enable / equation | — | `D3DRS_CLIPPLANEENABLE` bit `1<<(cap−GL_CLIP_PLANE0)`, `SetClipPlane` |
| Point size | — | `D3DRS_POINTSIZE` (float bits) |
| Line width | — | `D3DRS_LINEWIDTH` (float bits) — see build note |
| Viewport | — | `SetViewport` (MinZ/MaxZ preserved) |
| Clear colour | `s_clearColor` (default `D3DCOLOR_RGBA(0,0,0,255)`) | `Clear(…, colour, z=1.0f, stencil=0)` |
| Texture enable per unit | `s_currentAttribs.texturingEnabled[2]` | selects FixedFunc shader variant |
| Bound texture per unit | `s_textureStates[unit].target` (GL id) | `SetTexture(unit, s_textureIds[id])` |
| Active texture unit | `s_activeTexture` (0..1, `MAX_ACTIVE_TEXTURES = 2`) | stage/sampler index |
| Tex env mode / combine / colour | `TextureState.envMode` (default `GL_MODULATE`), `rgbCombineMode`, `envColour` | **stored only — no D3D effect** |
| Tex wrap / filters | — | sampler states of the **active stage** (`D3DSAMP_ADDRESSU/V`, `MAGFILTER`, `MINFILTER`, `MIPFILTER`) |
| Colour material enable / mode | `colorMaterialEnabled`, `colorMaterialMode` (default `GL_DIFFUSE`) | stored only |
| Lighting enable | `g_fixedPipeline->m_lighting` | shader `numLights` |
| Light 0/1 enable | `g_fixedPipeline->m_lightEnabled[0..1]` | shader `numLights` |
| Light params | `g_fixedPipeline->m_state.m_light[i]` (`D3DLIGHT9`) | shader `lights[i]` |
| Material diffuse/specular/shininess | `g_fixedPipeline->m_materialDiffuse[4]`, `m_materialSpecular[4]`, `m_materialShininess` | **stored only — never uploaded** |
| Fog enable/start/end | `g_fixedPipeline->m_fogEnabled/m_fogStart/m_fogEnd` | shader `fogRange` |
| Fog colour/mode/density | — | **ignored** |
| Hints | `s_fogHint`, `s_polygonSmoothHint` | stored only (readable via `glGetIntegerv`) |
| Matrix mode | `s_matrixMode`, `s_pTargetMatrixStack`, `s_targetMatrixTransformType` | — |
| Attribute stack | `class Attributes{CurrentAttributes, IDirect3DStateBlock9*}`, `std::stack<Attributes> s_attributeStack` | **declared, unused** — there is no `glPushAttrib/glPopAttrib` in the emulator, and none in game code [C grep] |

`CurrentAttributes` derives from `CustomVertex` and adds `alphaRef`, `texturingEnabled[2]`, `backCullingEnabled`, `polygonOffsetEnabled`, `depthBias`, `slopeDepthBias`, `colorMaterialEnabled`, `colorMaterialMode`; constructor zero-fills then sets `colorMaterialMode = GL_DIFFUSE`, `a8 = 255` [C]. `TextureState` = `{target=0, lastTarget=-1, envColour=0, envMode=GL_MODULATE, lastEnvMode=0, rgbCombineMode=0}` [C]. There is **no redundant-state filtering**: every `glEnable/glBlendFunc/…` issues a `SetRenderState` (the commented-out "lastEnvMode/lastTarget" optimisation in `setupTexturing` is disabled) [C].

### `glEnable` / `glDisable` / `glIsEnabled` capability table [C]

| Cap | `glEnable` | `glDisable` | `glIsEnabled` |
|---|---|---|---|
| `GL_ALPHA_TEST` | ALPHATESTENABLE=TRUE | FALSE | reads RS |
| `GL_BLEND` | ALPHABLENDENABLE=TRUE | FALSE | reads RS |
| `GL_CLIP_PLANE0..2` | set bit in CLIPPLANEENABLE (Get→Set) | clear bit | not handled (assert, returns 0) |
| `GL_COLOR_MATERIAL` | flag only | flag only | flag |
| `GL_CULL_FACE` | CULLMODE=`s_cullMode` | CULLMODE=NONE | `CULLMODE != NONE` |
| `GL_DEPTH_TEST` | ZENABLE=TRUE | FALSE | reads RS |
| `GL_FOG` | `m_fogEnabled=true` | false | flag |
| `GL_LIGHTING` | `m_lighting=true` | false | flag |
| `GL_LIGHT0`, `GL_LIGHT1` | `m_lightEnabled[i]=true` | false | flag |
| `GL_LIGHT2`..`GL_LIGHT7` | default branch (trace "not implemented") | same | assert, 0 |
| `GL_LINE_SMOOTH` | no-op | no-op | assert, 0 |
| `GL_NORMALIZE` | no-op | no-op | assert, 0 |
| `GL_POINT_SMOOTH` | default branch (trace) | no-op | FALSE |
| `GL_POLYGON_OFFSET_FILL` | flag + SLOPESCALEDEPTHBIAS/DEPTHBIAS from cache | flag + both 0 | assert, 0 |
| `GL_SCISSOR_TEST` | SCISSORTESTENABLE=TRUE | FALSE | reads RS |
| `GL_TEXTURE_2D` | `texturingEnabled[active]=true`; if bound texture pointer non-NULL → `SetTexture` | flag false | flag |

### Enum translation tables [C]

| GL | D3D9 |
|---|---|
| Blend `GL_ZERO`, `GL_ONE`, `GL_SRC_COLOR`, `GL_ONE_MINUS_SRC_COLOR`, `GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA` | `D3DBLEND_ZERO`, `ONE`, `SRCCOLOR`, `INVSRCCOLOR`, `SRCALPHA`, `INVSRCALPHA`; any other factor → assert + `D3DBLEND_ONE` |
| Compare `GL_LEQUAL`, `GL_GREATER` | `D3DCMP_LESSEQUAL`, `D3DCMP_GREATER`; others (NEVER/LESS/EQUAL/NOTEQUAL/GEQUAL/ALWAYS commented out) → assert + LESSEQUAL |
| Filter `GL_LINEAR`, `GL_NEAREST`, `GL_LINEAR_MIPMAP_LINEAR` | `D3DTEXF_LINEAR`, `D3DTEXF_POINT`, `D3DTEXF_LINEAR`; any other (incl. `GL_NEAREST_MIPMAP_LINEAR`, which *is* `#define`d) → **`AppReleaseAssert`** (fatal in release) |
| MIN filter → MIPFILTER | `GL_LINEAR_MIPMAP_LINEAR → D3DTEXF_LINEAR`, else `D3DTEXF_NONE` |
| Wrap `GL_CLAMP`, `GL_REPEAT` | `D3DTADDRESS_CLAMP`, `D3DTADDRESS_WRAP`; other → `AppReleaseAssert` |
| Fill `GL_FILL`, `GL_LINE` | `D3DFILL_SOLID`, `D3DFILL_WIREFRAME` |
| Clear `GL_COLOR_BUFFER_BIT`, `GL_DEPTH_BUFFER_BIT` | `D3DCLEAR_TARGET`, `D3DCLEAR_ZBUFFER` (other bits → debug assert) |

### Complete emulated entry-point table [C]

Legend: **Impl** = implemented; **Stub** = compiled, does nothing (traces in `_DEBUG`); **Partial** = subset of GL semantics.

| GL / GLU / extension function | Status | D3D9 implementation summary (ref `code/lib/opengl_directx.cpp` unless noted) |
|---|---|---|
| `glBegin(mode)` | Impl | reset vertex cursor to `s_vertices[0]`, count 0, store `s_primitiveMode` |
| `glEnd()` | Impl | `FastSetTransform()` (no-op); if `g_fixedPipeline` and not recording → `FixedPipeline::Bind()`; convert primitive (table below) and `DrawPrimitiveUP` with stride 44 |
| `glVertex2i/2f/2fv/2dv/3d/3dv/3f/3fv` | Impl (inline) | `glVertex3f_impl`: grow buffer if `count+2 ≥ allocated` (×2, initial 1024); copy whole `s_currentAttribs` as the vertex, set xyz; quad expansion (below). 2D variants set z=0 |
| `glColor3ub/3ubv/3f/4ub/4ubv/4f/4fv` | Impl (inline + `glColor4ub`) | set current colour bytes (3-component variants alpha=255 / 1.0) |
| `glNormal3f/3fv/3dv` | Impl (inline) | current normal = **−n** |
| `glTexCoord2i/2f/2fv/2dv` | Impl (inline) | current `u,v` |
| `glEnable/glDisable/glIsEnabled` | Partial | capability table above |
| `glGetError()` | Stub-like | always returns 0 |
| `glGetString(name)` | Stub | returns NULL (`opengl_directx_stubs.cpp`) |
| `glHint(target, mode)` | Partial | stores `GL_FOG_HINT` / `GL_POLYGON_SMOOTH_HINT`; no effect |
| `glFrontFace(mode)` | Impl | updates `s_ccwFrontFace`, `s_cullMode`; re-applies CULLMODE if culling on |
| `glShadeModel(mode)` | Stub | ignored (flat look emulated only for strips, see conversion) |
| `glBlendFunc(s,d)` | Impl | SRCBLEND/DESTBLEND |
| `glAlphaFunc(func, ref)` | Impl | ALPHAFUNC; ALPHAREF = `DWORD(clamp(ref)*255)` (truncation) |
| `glDepthFunc(func)` | Impl | ZFUNC |
| `glDepthMask(flag)` | Impl | ZWRITEENABLE |
| `glClipPlane(plane, eq)` | Impl | normalise plane; transform by `(MV⁻¹)ᵀ` then `(P⁻¹)ᵀ` → clip-space plane; `SetClipPlane(plane−GL_CLIP_PLANE0)` |
| `glClear(mask)` | Impl | `Clear(0,NULL,flags,s_clearColor,1.0f,0)` |
| `glClearColor(r,g,b,a)` | Impl | `s_clearColor = D3DCOLOR_COLORVALUE(r,g,b,a)` |
| `glFinish()` | Stub | returns immediately (back-buffer lock code after `return` is dead) |
| `glLineWidth(w)` | Impl | `SetRenderState(D3DRS_LINEWIDTH, *(DWORD*)&w)` |
| `glPointSize(s)` | Impl | `D3DRS_POINTSIZE` float bits |
| `glViewport(x,y,w,h)` | Impl | Get/SetViewport, **no y-flip** |
| `glScissor(x,y,w,h)` | Impl | `RECT{left=x, right=x+w, bottom=ScreenH−y, top=bottom−h}` |
| `glPolygonMode(face, mode)` | Partial | FILLMODE; face ignored |
| `glPolygonOffset(factor, units)` | Partial | caches; applies DEPTHBIAS=units, SLOPESCALEDEPTHBIAS=factor if enabled (value conversion, see D11) |
| `glGetIntegerv(pname)` | Partial | `GL_ALPHA_TEST_FUNC`, `GL_BLEND_DST`, `GL_BLEND_SRC`, `GL_DEPTH_FUNC`, `GL_DEPTH_WRITEMASK` (read back from device RS); `GL_COLOR_MATERIAL_FACE` (always `GL_FRONT_AND_BACK`); `GL_COLOR_MATERIAL_PARAMETER`; `GL_FOG_HINT`; `GL_FRONT_FACE`; `GL_MATRIX_MODE`; `GL_POLYGON_MODE` (both entries = fill mode); `GL_POLYGON_SMOOTH_HINT`; `GL_VIEWPORT`. Others: trace only, output untouched |
| `glGetFloatv(pname)` | Partial | `GL_ALPHA_TEST_REF` (cached float), `GL_CURRENT_COLOR` (bytes × 1/255), `GL_FOG_COLOR` (no-op, output untouched), `GL_FOG_START`, `GL_FOG_END`; others assert |
| `glGetDoublev(pname)` | Partial | `GL_MODELVIEW_MATRIX`, `GL_PROJECTION_MATRIX` (top of stack, copied element-wise) |
| `glMatrixMode(mode)` | Impl | switch target stack (asserts MODELVIEW/PROJECTION) |
| `glLoadIdentity()` | Impl | `Load(identity)` |
| `glLoadMatrixd(m)` | Impl | `Load(openGLMatrixToDirect3D(m))` |
| `glMultMatrixd(m)`, `glMultMatrixf(m)` | Impl | `Multiply(converted)` |
| `glPushMatrix()`, `glPopMatrix()` | Impl | stack Push/Pop |
| `glTranslatef`, `glScalef` | Impl | `D3DXMatrixTranslation/Scaling` then Multiply |
| `glRotatef(angle,x,y,z)` | Impl | `D3DXMatrixRotationAxis(axis, D3DXToRadian(angle))` then Multiply |
| `glFrustum(...)` | Stub | no-op (`opengl_directx_stubs.cpp`) |
| `gluOrtho2D(l,r,b,t)` | Impl | `Load(D3DXMatrixOrthoOffCenterRH(l,r,b,t,−1,+1))` |
| `gluPerspective(fovy,aspect,zn,zf)` | Impl | `D3DXMatrixPerspectiveFovRH(rad(fovy),aspect,zn,zf)`, then **`_43 *= 2`**, Load |
| `gluLookAt(eye,center,up)` | Partial | `Load(D3DXMatrixLookAtRH(...))` — **replaces** the current matrix (GL multiplies) |
| `gluProject`, `gluUnProject` | Impl | D3DX vector/matrix maths with GL window conventions (x,y,z mapped [−1,1]↔[0,1], viewport offset; returns false on w=0 / singular) |
| `glLightfv(light, pname)` | Partial | `GL_POSITION` w=0 → directional: `Direction = xyz` transformed by current modelview (w=0) then normalised; w≠0 → `Type=POINT`, `Position=xyz` but **the code transforms `Direction` instead of `Position`** (D17); `GL_DIFFUSE`, `GL_SPECULAR`, `GL_AMBIENT` copied |
| `glGetLightfv` | Impl | returns stored values (POSITION as dir,w=0 or pos,w=1) |
| `glLightModelfv` | Stub | ignored (light-model ambient is not emulated) |
| `glMaterialfv(face, pname)` | Partial | `GL_SPECULAR`, `GL_DIFFUSE`/`GL_AMBIENT_AND_DIFFUSE`, `GL_SHININESS` stored; `GL_AMBIENT` ignored; face ignored; values never reach shaders |
| `glGetMaterialfv` | Stub | no-op |
| `glColorMaterial(face, mode)` | Partial | stores mode only |
| `glFogf(pname, v)` | Partial | `GL_FOG_START`, `GL_FOG_END` → FixedPipeline; `GL_FOG_DENSITY`, `GL_FOG_MODE` accepted & ignored; others assert |
| `glFogi(pname, v)` | Partial | `DENSITY/END/START`: converts as GL normalised integer (`v/INT_MAX` if v>0 else `v/INT_MIN`) then `glFogf`; `GL_FOG_MODE` silently ignored |
| `glFogfv(pname, v)` | Stub | ignored (**fog colour lost**) |
| `glGenTextures(n)` | Impl | ids = next indices of `std::vector s_textureIds`, initialised NULL (**id 0 is a real texture**) |
| `glDeleteTextures` | Stub | no-op (D3D textures leak until device reset/shutdown) |
| `glBindTexture(GL_TEXTURE_2D, id)` | Impl | `s_activeTextureState->target=id`; if unit enabled → `SetTexture(unit, s_textureIds[id])` |
| `glTexImage2D(…, _compressed)` | Partial | asserts target 2D, level 0, internalformat 4, format RGBA; `d3dCreateTexture(mipmaps=false)` |
| `gluBuild2DMipmaps(…, _compressed)` | Impl | `d3dCreateTexture(mipmaps=true)` |
| `glTexParameteri/f` | Partial | wrap/filter → sampler states of **active stage** |
| `glGetTexParameteriv` | Partial | MAG/MIN from sampler state; WRAP_S/T read from `D3DRS_WRAP0` bits (wrong state, D16) |
| `glGetTexLevelParameteriv` | Partial | `GL_TEXTURE_WIDTH/HEIGHT` from `GetTexture(stage)->GetLevelDesc(level)` (leaks the AddRef'd texture ref) |
| `glTexEnvf` | Partial | `GL_TEXTURE_ENV_MODE`, `GL_COMBINE_RGB_EXT` stored (+rebind); else assert |
| `glTexEnvi` | Partial | `GL_TEXTURE_ENV_MODE` stored; else assert |
| `glTexEnviv` | Partial | `GL_TEXTURE_ENV_COLOR` stored (/255); else assert |
| `glGetTexEnviv` | Partial | `GL_TEXTURE_ENV_COLOR` (×255), `GL_TEXTURE_ENV_MODE` |
| `glCopyTexImage2D(…, x,y,w,h)` | Partial | lazily `CreateTexture(w,h,1,RENDERTARGET, backbuffer fmt, DEFAULT)`; `StretchRect(backbuffer {x,y,x+w,y+h} → level 0, D3DTEXF_NONE)`; internalFormat ignored |
| `glReadPixels(x,y,w,h,GL_RGB,…)` | Partial | only GL_RGB; reads from (0,0) regardless of x,y; temp A8R8G8B8 render-target `Texture`, `StretchRect(POINT)` → `GetRenderTargetData` → SYSTEMMEM surface → BGRA→RGB with vertical flip, pitch assumed `w*4` |
| `glGenLists`, `glDeleteLists`, `glNewList`, `glEndList`, `glCallList` | Impl | display-list emulation (below) |
| `glEnableClientState`, `glDisableClientState`, `glVertexPointer`, `glColorPointer`, `glNormalPointer`, `glTexCoordPointer`, `glDrawArrays` | Stub | no-ops — callers needing arrays have `#ifdef USE_DIRECT3D` code that draws with the device directly (landscape, water) |
| `gluErrorString` | Stub-like | returns `"[Error Reporting Not Yet Implemented]"` |
| `gglActiveTextureARB` → `glActiveTextureD3D(target)` | Impl | `s_activeTexture = target − GL_TEXTURE0_ARB` (asserts 0..1) |
| `gglMultiTexCoord2fARB` → `glMultiTexCoord2fD3D(target,s,t)` | Impl | unit 0 → `u,v`; unit 1 → `u2,v2` |
| `gglGenBuffersARB` → `glGenBuffersD3D` | Impl | ids `++s_lastVertexBufferId` (start at 1) (`ogl_extensions_directx.cpp`) |
| `gglDeleteBuffersARB` → `glDeleteBuffersD3D` | Impl | Release + erase from `std::map s_vertexBuffers` |
| `gglBindBufferARB` → `glBindBufferD3D` | Partial | asserts `GL_ARRAY_BUFFER_ARB` (index buffers unsupported); `g_currentVertexBuffer = &s_vertexBuffers[id]` or NULL for 0 |
| `gglBufferDataARB` → `glBufferDataD3D` | Partial | releases old buffer, `CreateVertexBuffer(size, WRITEONLY[+SOFTWAREPROCESSING], FVF 0, MANAGED)`, Lock/memcpy/Unlock; usage hint ignored |
| `gglIsBufferARB`, `gglBufferSubDataARB`, `gglGetBufferSubDataARB`, `gglMapBufferARB`, `gglUnmapBufferARB`, `gglGetBufferParameterivARB`, `gglGetBufferPointervARB`, `gglChoosePixelFormatARB` | NULL | set to 0 by `InitialiseOGLExtensions` (D3D variant) |
| `IsOGLExtensionSupported(name)` (D3D) | Impl | returns 1 only for `"GL_ARB_multitexture"` and `"GL_ARB_vertex_buffer_object"`; `IsOGLExtensionPerfectlySupported` = same |

Non-GL helpers exported by the emulator: `Direct3DInit`, `Direct3DShutdown`, `Direct3DSwapBuffers`, `WaitAndResetDevice`, `CreateD3dPoolDefaultResources`, `ReleaseD3DPoolDefaultResources`, `CreateD3DResources`, `ReleaseD3DResources`, `SafeDeleteTexture`, `OpenGLRegisterD3DTexture` (wraps an external `IDirect3DTexture9` as a GL id; **no callers** [C grep]), `GenerateMipmaps`, `GeneratingMipmaps` (polled by `code/loading_screen.cpp` and declared in `code/lib/text_renderer.cpp`), `getWorldViewMatrix`, `getProjMatrix`, `Shader::SetSamplerGLTextureId(name, glId)`, `Shader::SetSamplerGLFixedStage(name, unit)` (binds the texture bound to GL unit `unit`) [C].

### Immediate mode and primitive conversion [C] `glVertex3f_impl`, `glEnd`

- Vertices are accumulated in a growable system-memory array `s_vertices` (`new CustomVertex[1024]`, doubled with `memcpy` when `count+2 ≥ allocated`), never shrunk.
- Each `glEnd` issues **one `DrawPrimitiveUP`** (user-pointer draw; no dynamic vertex buffer, no batching), preceded by a full `FixedPipeline::Bind` (constant uploads).

| GL mode | Conversion | D3D draw |
|---|---|---|
| `GL_POINTS` | none | `POINTLIST`, n |
| `GL_LINES` | none | `LINELIST`, n/2 |
| `GL_LINE_LOOP` | `v[n] = v[0]` appended | `LINESTRIP`, n |
| `GL_LINE_STRIP` | none | `LINESTRIP`, **n** (should be n−1 ⇒ one extra segment to stale `v[n]`, D9) |
| `GL_TRIANGLE_STRIP`, `GL_QUAD_STRIP` | expanded to a triangle list: for i = 0..n−3 emit `(v[i+2]; v[i] with v[i+2]'s colour+normal; v[i+1] with v[i+2]'s colour+normal)` — i.e. each triangle takes the **last vertex's colour/normal** (GL flat-shading provoking vertex). Positions and both UV sets come from their own vertex. n ≤ 2 → nothing drawn. The original smooth `TRIANGLESTRIP` call is commented out | `TRIANGLELIST`, n−2 |
| `GL_TRIANGLE_FAN` | none | `TRIANGLEFAN`, n−2 |
| `GL_TRIANGLES` | none | `TRIANGLELIST`, n/3 (skipped if 0 — "could be 0 in PaintPixels") |
| `GL_QUADS` | at submission: after the 3rd vertex of each quad, copies of quad vertices 0 and 2 are appended, so quad (a,b,c,d) → (a,b,c)(a,c,d). Uses a **function-static `vertexCount` (0..3) never reset by `glBegin`** (D10) | `TRIANGLELIST`, n/3 |
| other (`GL_POLYGON` not even `#define`d) | trace | none |

[I] Winding: GL renders odd strip triangles as (v[i+1], v[i], v[i+2]); the list conversion emits a rotation of (v[i], v[i+1], v[i+2]) for every i, so **odd triangles have reversed winding** and are removed when back-face culling is on (D8). (`USE_DIRECT3D_TRILIST` in landscape/water — P11 — avoids strips for exactly the flat-shading reason.)

### Matrix stacks [C] `code/lib/opengl_directx_matrix_stack.cpp`

- Two `MatrixStackActual` objects (modelview tagged `D3DTS_WORLD`, projection tagged `D3DTS_PROJECTION`) wrap `ID3DXMatrixStack` (`D3DXCreateMatrixStack(0, …)`); recreated by `InitialiseData` at every `Direct3DInit`.
- `Load`→`LoadMatrix`; `Multiply`→`MultMatrixLocal` (top = M × top, which in D3D's row-vector convention equals GL's post-multiplication top × M); `Push`/`Pop`; `GetTransform`→`GetTop()`.
- `SetTransform()` is **empty**: the device's fixed-function transforms are never set; matrices reach the GPU only as shader constants in `FixedPipeline::Bind` (`matWorldView` = modelview top, `matProj` = projection top). `m_modified` is set but never cleared, so `FastSetTransform` always calls the empty `SetTransform` [C].
- GL ↔ D3D matrix copies are element-for-element (`openGLMatrixToDirect3D`, `direct3DMatrixToOpenGL`): GL column-major storage with column vectors is bit-identical to D3D row-major storage with row vectors, so **no transpose** is needed [C comment + code].
- No stack-depth limit or `GL_STACK_OVERFLOW/UNDERFLOW` emulation (ID3DXMatrixStack grows dynamically) [I].
- Projection quirk [I, derived algebraically from `gluPerspective`]: D3DX RH perspective gives `_33 = zf/(zn−zf)`, `_43 = zn·zf/(zn−zf)` and maps eye depth [zn, zf] → [0,1]. After `_43 *= 2`, clip z = 0 occurs at eye depth **2·zn** and z = w at **2·zf**, i.e. the effective near/far planes are doubled relative to the GL build. The shader fog distance `oPos.z` then equals `zf·(e−2zn)/(zf−zn)` ≈ eye depth − 2zn for zf ≫ zn. Whether this was intentional (comment says "Preserve OpenGL compatibility") is [U].

### Display-list emulation [C] `glGenLists`…`glCallList`, `DisplayListDevice`, `DisplayList`

```mermaid
sequenceDiagram
  participant Game
  participant Emu as opengl_directx.cpp
  participant Rec as DisplayListDevice
  participant DL as DisplayList
  Game->>Emu: glNewList(id, GL_COMPILE)  (via safeGlNewList: lock g_openGLMutex)
  Emu->>Rec: new DisplayListDevice(id); MV/Proj stacks → MatrixStackDisplayList; g_pd3dDevice = Rec
  Game->>Emu: glBegin/glVertex…/glEnd
  Emu->>Rec: DrawPrimitiveUP(type, count, verts)  [FixedPipeline::Bind skipped]
  Rec->>Rec: append verts, push CommandDrawPrimitive(type, start, count)
  Game->>Emu: glPushMatrix/glTranslatef/glMultMatrix/glPopMatrix/glLoadMatrix
  Emu->>Rec: RecordPushMatrix / RecordMultMatrix / RecordPopMatrix / RecordLoadTransform (bound to the *actual* stacks)
  Game->>Emu: glEnable(GL_BLEND) etc.
  Emu->>Rec: SetRenderState → DLIST_DEV_NOT_IMPLEMENTED (debug assert, release no-op)
  Game->>Emu: glEndList()  (safeGlEndList: unlock)
  Emu->>DL: Compile(): VB (MANAGED, WRITEONLY) + NULL-terminated Command* array
  Emu->>Emu: g_pd3dDevice = actual; restore stacks; s_displayLists[id] = DL
  Game->>Emu: glCallList(id)
  Emu->>DL: Draw(): SetStreamSource(0, VB, 0, 44); for each command Execute()
  DL->>Emu: CommandDrawPrimitive: FixedPipeline::Bind(); DrawPrimitive(type, start, count)
```

What a list can contain and how it behaves:

| Content recorded inside `glNewList…glEndList` | Emulator behaviour |
|---|---|
| Geometry (`glBegin/glEnd`, any primitive) | recorded (already converted to D3D primitive types; vertex counts per type: POINTLIST n, LINELIST 2n, LINESTRIP n+1, TRIANGLELIST 3n, TRIANGLESTRIP n+2, TRIANGLEFAN n+2) |
| Per-vertex colour/normal/texcoords | baked into the VB at record time (current colour at *record* time — a list without its own `glColor` cannot inherit the colour current at playback, unlike GL) |
| `glPushMatrix`, `glPopMatrix`, `glLoadMatrixd`, `glLoadIdentity`, `glMultMatrix*`, `glTranslatef`, `glScalef`, `glRotatef`, `gluOrtho2D`, `gluPerspective`, `gluLookAt` | recorded as commands against the stack that was current **when the matrix call was made** |
| Render/sampler/texture state (`glEnable`/`glDisable` of device states, `glBlendFunc`, `glDepthMask`, `glAlphaFunc`, `glBindTexture`, `glTexParameter*`, `glClear`, `glViewport`, `glPointSize`, `glScissor`, `glClipPlane`) | **dropped** (every non-draw `IDirect3DDevice9` method of the recorder is `DLIST_DEV_NOT_IMPLEMENTED`) |
| Emulator-side state (`glEnable(GL_LIGHTING/GL_FOG/GL_LIGHTx/GL_TEXTURE_2D/GL_COLOR_MATERIAL)`, `glFogf`, `glLightfv`, `glMaterialfv`, `glTexEnv*`) | applied **immediately at record time** (side effect), not recorded |
| `glGetDoublev`, `gluProject` (matrix queries) | `MatrixStackDisplayList::GetTransform` asserts and returns a static identity |
| Nested `glCallList` | its `SetStreamSource`/`DrawPrimitive` go to the recorder → dropped (nesting unsupported) |
| `GL_COMPILE_AND_EXECUTE` | mode argument ignored; lists are compile-only |

Other details: `glGenLists(range)` returns `s_lastDisplayId` (starts at **0**) and inserts `range` NULL entries; `glCallList` on an unknown id inserts a NULL entry via `std::map::operator[]` and draws nothing; `glDeleteLists` deletes and erases; MANAGED VBs survive device reset. Playback binds the fixed pipeline per recorded draw, so state current **at playback** (blend, texture, lights) applies — which is how game code uses lists (state set outside the list) [I].

### Vertex buffers (ARB VBO emulation) [C] `code/lib/ogl_extensions_directx.cpp`

GL VBO ids map to `IDirect3DVertexBuffer9*` in `std::map<unsigned, IDirect3DVertexBuffer9*> s_vertexBuffers`. There is no emulated `glDrawArrays`; the only VBO client, `LandscapeRenderer` (P11), under `USE_DIRECT3D` binds via `gglBindBufferARB`, then calls `g_pd3dDevice->SetVertexDeclaration(GetVertexDecl())`, `SetStreamSource(0, *g_currentVertexBuffer, 0, sizeof(LandVertex))`, `FixedPipeline::Bind()` and `DrawPrimitive(D3DPT_TRIANGLESTRIP or TRIANGLELIST, …)` directly [C] `code/landscape_renderer.cpp:LandscapeRenderer::RenderMainSlow`/`RenderOverlaySlow`. Landscape render mode is forced to `RenderModeVertexBufferObject` under D3D [C].

### Textures [C] `d3dCreateTexture`, `MipMapTask`, `GenerateMipmaps`

1. `glGenTextures` reserves a slot; texture object is created lazily on upload.
2. `d3dCreateTexture(target, components, w, h, format, type, data, mipmapping, compressed)`:
   - releases any previous texture in the slot;
   - debug-asserts power-of-two `w`, `h` (`w == 1 << ceil(log2 w)`), `format == GL_RGBA`, `type == GL_UNSIGNED_BYTE`;
   - `CreateTexture(w, h, Levels = mipmapping ? 0 : 1, Usage = D3DUSAGE_AUTOGENMIPMAP, Format = compressed ? D3DFMT_DXT1 : D3DFMT_A8R8G8B8, Pool = D3DPOOL_MANAGED)`;
   - fills level 0 with `D3DXLoadSurfaceFromMemory(src fmt D3DFMT_A8B8G8R8 (= R,G,B,A bytes), pitch w*4, D3DX_FILTER_NONE)` (on-the-fly DXT1 compression when `compressed`);
   - if `mipmapping`: wraps the texture in a `MipMapTask` (holds every level surface), **sets the GL slot to NULL** (texture invisible meanwhile), enqueues `MipMapTask::GenerateMipmaps` on `s_workQueue` (a `WorkQueue` thread whose first job pins it to CPUs 0,2,4,6: `SetThreadAffinityMask(0x01|0x04|0x10|0x40)`) and appends to `s_texturesToMipmap` under `s_texturesToMipmapMutex`;
   - finally calls `setupTexturing()` — the guard `if (s_currentAttribs.texturingEnabled)` tests an *array* (always true), so the active stage is always re-bound (D19).
3. Worker: `GenerateMipmaps` box-filters level 0 into every level ≥ 1 with `D3DXLoadSurfaceFromSurface(…, D3DX_FILTER_TRIANGLE)`, then sets `m_complete = true` (plain `bool`, written outside the list mutex — benign race [I]).
4. Main thread: `GenerateMipmaps()` runs at the start of every `Direct3DSwapBuffers`; under the mutex it deletes completed tasks; `~MipMapTask` releases the surfaces and **restores the slot pointer**. `GeneratingMipmaps()` keeps `LoadingScreen` up while tasks are pending (P20).
5. [I/U] With `D3DUSAGE_AUTOGENMIPMAP`, D3D9 exposes only level 0 (`GetLevelCount()` = 1), so the worker loop likely runs zero iterations and the runtime/driver generates mips itself; the asynchronous machinery then only delays visibility by ≥ 1 frame. Resolve by checking `GetLevelCount()` at runtime.

Formats in use: `D3DFMT_A8R8G8B8` (default), `D3DFMT_DXT1` (callers passing `_compressed=true`), back-buffer format (copy-tex render targets) [C].

### Fixed-function emulation: `FixedPipeline` + `data/shaders/FixedFunc*` [C]

**Lifecycle.** `g_fixedPipeline = new FixedPipeline()` in `CreateD3dPoolDefaultResources` (at `Direct3DInit` and after every device reset); deleted in `ReleaseD3DPoolDefaultResources`. The constructor zero-fills the object (`memset(this,0,…)` ⇒ fog/light/enable state reset to 0/false on every reset), sets `m_enabled=true`, compiles three shader pairs, sets `D3DRS_LIGHTING=FALSE`, and writes identity into VS constants c0–c3 and c4–c7 [C] `code/lib/FixedPipeline.cpp:FixedPipeline::FixedPipeline`. `Direct3DSwapBuffers` re-writes the same identities after every `Present` ("JAKHACK") [C].

**Shader compilation** (P12 owns `Shader`): `D3DXCompileShader` of the file text with entry `vs_main`/`ps_main` and profile from `D3DXGetVertexShaderProfile/D3DXGetPixelShaderProfile(device)`; `#include` resolved under `shaders/` through the resource system; failure → `AppReleaseAssert("Shader create error…")` [C] `code/lib/shader.cpp`.

| Index | Files | Chosen when (`FixedPipeline::Bind`) |
|---|---|---|
| 0 | `shaders/FixedFunc0.vs` + `shaders/FixedFunc0Maps.ps` | texture unit 0 disabled (unit 1 state irrelevant) |
| 1 | `shaders/FixedFunc1.vs` + `shaders/FixedFunc1Maps.ps` | unit 0 enabled, unit 1 disabled |
| 2 | `shaders/FixedFunc2.vs` + `shaders/FixedFunc2Maps.ps` | units 0 and 1 enabled |

**`FixedPipeline::Bind()` algorithm** (called from every `glEnd`, every display-list draw, and directly by landscape/water D3D code):

```
if !m_enabled: return                       # Disable()/Enable() bracket custom shaders (deform, water reflection)
ActiveTexture(1); tex1 = IsEnabled(TEXTURE_2D)
ActiveTexture(0); tex0 = IsEnabled(TEXTURE_2D)   # side effect: active unit is left at 0
shader = fixed[tex0 ? (tex1 ? 2 : 1) : 0]
resetAll = (shader != m_bound); if so: Unbind old; bind shader (SetVertexShader+SetPixelShader)
upload matWorldView = MV.top; matWorldViewIT = inverse(MV.top); matProj = P.top      # every call
if resetAll or specular changed: bSpecular = m_state.m_specular
numLights = (m_lighting && m_lightEnabled[0]) ? 1 + (m_lightEnabled[1] ? 1 : 0) : 0
if resetAll or changed: numLights
for i < numLights: upload lights[i].vDir / vDiffuse / vSpecular when changed (memcmp vs m_oldState)
fogRange = m_fogEnabled ? (m_fogEnd, m_fogEnd − m_fogStart) : (100000, 1000); upload when changed
upload samplers environmentDiffuse/environmentSpecular when changed (never set; not declared by FixedFunc shaders → no-op)
m_oldState = m_state
```

Constants are set **by name** through the D3DX constant tables of both VS and PS (string lookup per call). `m_state.m_specular` is toggled only by `code/location.cpp` (true at L1999, false at L2041 — P11/P12 context) [C].

**Constant/uniform layout** [C for declarations; register numbers other than c0/c4 are compiler-assigned [U]]:

| Uniform | Declared in | Register | Set by |
|---|---|---|---|
| `shared float4x4 matCameraView` | FixedFunc0/1/2.vs | **c0–c3** (explicit) | identity (ctor, after each Present) |
| `shared float4x4 matCameraViewIT` | FixedFunc0/1/2.vs | **c4–c7** (explicit) | identity |
| `float4x4 matProj` | all VS | compiler | `Bind` |
| `float4x4 matWorldView` | all VS | compiler | `Bind` |
| `float4x4 matWorldViewIT` | all VS | compiler | `Bind` (plain inverse; HLSL `mul((float3x3)M, n)` applies it transposed) |
| `float2 fogRange` (x = end, y = end − start) | all VS | compiler | `Bind` |
| `bool bSpecular` | FixedFunc0.vs | compiler | `Bind` |
| `int numLights` | FixedFunc0.vs | compiler | `Bind` |
| `CLight lights[2] {float3 vDir; float4 vDiffuse; float4 vSpecular;}` | FixedFunc0.vs | compiler | `Bind` |
| `float4 fogColour` | FixedFunc2.vs | compiler | **never set, never used** |
| `sampler map0` | FixedFunc1Maps.ps, FixedFunc2Maps.ps | s0 [I] | texture bound by GL emulation on stage 0 |
| `sampler map1` | FixedFunc2Maps.ps | s1 [I] | stage 1 |

`data/shaders/globals.h` contains only `#define DEFORM_PRECISION 10` (deform-shader precision vs max-deformation trade-off) and is included by `deformDiamond.ps`, `deformSphere.ps`, `deformFullscreen.ps` — **not** by any FixedFunc shader [C].

**What each shader computes** (vertex input from `s_customVertexDesc`; `COLOR0` arrives as float4 RGBA in [0,1]):

| Shader | Vertex stage | Pixel stage |
|---|---|---|
| FixedFunc0 | `P = pos·(matWorldView·matCameraView)`; `oPos = P·matProj`. If `numLights ≤ 0`: `c = iColor`. Else `N = normalize((float3x3)(matCameraViewIT·matWorldViewIT) · normalize(iNormal) + (0, 0.0001, 0))`, `V = normalize(P)`, for each light i: `L = −vDir_i`, `d = N·L`; if `d > 0`: diffuse `d·vDiffuse_i`, specular (only if `bSpecular`) `pow(max(0, normalize(L+V)·N), 16)·vSpecular_i`; `c = Σ (diffuse_i·iColor + spec_i)`; then **`c.a = 1`** ("fixes world map"). Fog: `f = saturate((fogRange.x − oPos.z)/fogRange.y)`; output colour `c·f` (all four channels) in TEXCOORD0 | `return colour` |
| FixedFunc1 | same transform; `oTex0 = iTex0`; `oColor = iColor·f` (TEXCOORD1) — **no lighting** | `colour · tex2D(map0, tc0)` |
| FixedFunc2 | same; `oTex0`, `oTex1`; `oColor = iColor·f` (TEXCOORD2) — no lighting | `colour · tex2D(map0, tc0) · tex2D(map1, tc1)` |

Sign conventions [C/I]: normals are negated on input (`glNormal3f_impl`) and `L = −vDir` where `vDir` is the eye-space GL light "position" vector (pointing *towards* the light); the two negations cancel, so `N·L` equals the standard GL Lambert term. Likewise both `H` and `N` are negated in the specular term, so the Blinn term matches GL's sign. Colour is carried in a TEXCOORD interpolator, so `D3DRS_SHADEMODE` has no effect — everything is Gouraud-interpolated [I].

### Behavioural differences / defects of the emulator vs the GL baseline

| # | Area | D3D behaviour | GL baseline | Ref | Tag |
|---|---|---|---|---|---|
| D1 | Lighting + texture | textured draws are never lit (FixedFunc1/2 have no lighting) | lit | `FixedPipeline::Bind`, `FixedFunc1.vs` | [C] |
| D2 | Lighting model | no ambient (light or model), no material colours/shininess, specular exponent fixed 16, specular not × material, LIGHT1 ignored unless LIGHT0, LIGHT2–7 unsupported, directional only | full GL | `FixedFunc0.vs`, `glMaterialfv`, `glLightModelfv` | [C] |
| D3 | Lit alpha | forced to 1 | material/vertex alpha | `FixedFunc0.vs` | [C] |
| D4 | Fog | multiplies RGBA by fog factor → fades to **black and transparent**; fog colour ignored; distance = clip-space z | blends RGB to fog colour (background colour, `Location::SetupFog`), alpha kept; eye distance | `FixedFunc*.vs`, `glFogfv` | [C] |
| D5 | Texture env | always MODULATE; REPLACE/DECAL/COMBINE/env colour ignored | honoured | `TextureState`, shaders | [C] |
| D6 | Texture parameters | stored per **sampler stage**, not per texture object (rebinding a texture does not restore its filter/wrap) | per texture object | `glTexParameteri` | [C] |
| D7 | Shade model | ignored; strips emulate flat shading by provoking-vertex copy; triangles/quads/fans interpolate | `GL_FLAT` default (`Renderer::SetOpenGLState`) | `glShadeModel`, `glEnd` | [C]/[I] |
| D8 | Strip winding | odd triangles reversed → culled when culling enabled | correct | `glEnd` | [I] |
| D9 | `GL_LINE_STRIP` | draws n segments (one extra to stale vertex) | n−1 | `glEnd` | [C] |
| D10 | `GL_QUADS` | static counter not reset per `glBegin`: a quad batch with a vertex count not divisible by 4 corrupts the following batch | per-primitive | `glVertex3f_impl` | [C] |
| D11 | Polygon offset | float passed to `SetRenderState(DWORD)` by **value conversion** (not bit cast) and with GL units semantics → wrong bias | correct | `glPolygonOffset`, `glEnable` | [C] |
| D12 | `glFrustum` | no-op | works | stubs | [C] |
| D13 | `gluLookAt` | replaces current matrix | multiplies | `gluLookAt` | [C] |
| D14 | Texture/list ids | id 0 is a real texture / list | 0 reserved | `glGenTextures`, `glGenLists` | [C] |
| D15 | `glDeleteTextures` | no-op (leak) | frees | stubs | [C] |
| D16 | `glGetTexParameteriv(WRAP_*)` | reads `D3DRS_WRAP0` (wrong state) | correct | `glGetTexParameteriv` | [C] |
| D17 | Point-light position | transforms `Direction`, leaves `Position` in object space | eye-space | `glLightfv` | [C] |
| D18 | Active texture unit | reset to 0 by every `FixedPipeline::Bind` (every `glEnd`) | persistent | `FixedPipeline::Bind` | [C] |
| D19 | Texture upload | always rebinds active stage (array-as-bool) | — | `d3dCreateTexture` | [C] |
| D20 | Mip-mapped uploads | texture is NULL until its async task is reaped at a later `SwapBuffers` | immediate | `d3dCreateTexture` | [C] |
| D21 | `glViewport` | no y-flip (sub-viewports would be mirrored vertically) | bottom-left origin | `glViewport` | [C]/[I] |
| D22 | `glReadPixels` | ignores x,y; assumes pitch = w·4; RGB only | general | `glReadPixels` | [C] |
| D23 | Depth range | effective near/far doubled by `_43 *= 2` | [zn, zf] | `gluPerspective` | [I] |
| D24 | Errors | `glGetError` always 0; `gluErrorString` placeholder | real | — | [C] |
| D25 | Client arrays / `glDrawArrays` | no-ops | work | stubs | [C] |
| D26 | Blend/compare/filter coverage | only enumerated values; `GL_NEAREST_MIPMAP_LINEAR` is a **release-fatal** assert | full | translation tables | [C] |
| D27 | Line width / point smooth / line smooth / normalize / hints | `D3DRS_LINEWIDTH` (non-PC state), others ignored | driver | — | [C]/[I] |
| D28 | Device reset | all emulator-side GL state (`g_fixedPipeline` recreated, render states reset by `Reset`) is lost until game code re-issues it; `s_currentAttribs` cache is *not* re-applied | n/a | `WaitAndResetDevice` | [I] |

### `opengl_trace` [B:D3D] + `_DEBUG` [C] `code/lib/opengl_trace.*`

- `GL_TRACE(...)` expands to `if (g_tracingOpenGL) glTrace(...)` only when `_DEBUG`; `GL_TRACE_IMP(...)` (used on every successful call) is defined **empty in both debug and release** (the live variant is commented out). Net effect: only the "not implemented"/fallback paths trace, and only in `DebugDirect3D`.
- `glTrace(fmt, …)`: opens `gltrace.txt` (mode `"w"`) on first use, writes `"%8d "` line number + message + `\n`, `fflush` each line; after `MAX_LINES = 50000` lines writes `"Log closed after 50000 lines"`, closes, and clears `g_tracingOpenGL`.
- `glTraceEnable(bool)` toggles `g_tracingOpenGL` (default false). Only callers: `code/loaders/gameoflife_loader.cpp` (dead: `USE_LOADERS` off).
- Formatting helpers: `glEnumToString` (switch over every GL enum in `opengl_directx.h` plus the 8 ARB/EXT enums; else `"(unknown enum)"`), `glBooleanToString` (`"TRUE"/"FALSE"`), `glArrayToString<double|float|int>` (`"[a, b, …]"`) and a `GLubyte` hex overload, `glBitfieldToString` (`GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TEXTURE_BIT|(unknown_bit)`), backed by `rotatingStaticString` (ring of 32 heap buffers). `s_numberOfCalls` is a `bool` that is incremented (no effect).

---

## [→ 31_platform_build_and_tools] Window management

### Class hierarchy [C]

```mermaid
classDiagram
  class WindowManager{<<abstract>>
    +LList~Resolution*~ m_resolutions
    +bool m_mousePointerVisible
    +bool m_invertY
    #int m_mouseOffsetX, m_mouseOffsetY
    #bool m_windowed, m_mouseCaptured
    #int m_screenW, m_screenH
    #int m_desktopScreenW, m_desktopScreenH, m_desktopColourDepth, m_desktopRefresh
    +GetResolutionId(w,h) int
    +GetResolution(id) Resolution*
    +SuggestDefaultRes(w,h,refresh,depth)
    +GetClosestResolution(w,h,refresh)
    +CreateWin(w,h,windowed,colourDepth,refresh,zDepth,waitVRT,antiAlias,title)*
    +DestroyWin()* +Window()* +Flip()*
    +NastySetMousePos()* +NastyMoveMouse()* +NastyPollForMessages()*
    +CaptureMouse()* +UncaptureMouse()* +HideMousePointer()* +UnhideMousePointer()*
    +EnsureMouseCaptured() +EnsureMouseUncaptured()
    +OpenWebsite(url)* +Windowed() +Captured() +WindowMoved()
  }
  class Resolution{+int m_width +int m_height +LList~int~ m_refreshRates}
  class WindowManagerWin32{#m_waitVRT #m_borderWidth #m_titleHeight #HWND m_hWnd #HDC m_hDC #HGLRC m_hRC #HGLRC m_hRC2 +SaveDesktop() +RestoreDesktop() +EnableOpenGL() +DisableOpenGL() +ListAllDisplayModes()}
  class WindowManagerDirectX{#m_waitVRT #m_borderWidth #m_titleHeight #HWND m_hWnd #HDC m_hDC #HGLRC m_hRC}
  WindowManager <|-- WindowManagerWin32 : [B:FULL]
  WindowManager <|-- WindowManagerDirectX : [B:D3D]
  WindowManager o-- Resolution
```

Single global `WindowManager *g_windowManager` (set in `WinMain`, asserted NULL on construction) [C] `code/lib/window_manager.cpp`. `PlatformWindow` is `typedef void*`; `Window()` returns the `HWND` cast to `PlatformWindow*` [C].

Base-class behaviour [C] `code/lib/window_manager.cpp`:

| Member | Behaviour |
|---|---|
| ctor | `m_mousePointerVisible=true`, `m_mouseOffsetX=INT_MAX` (sentinel = "recompute offset"), `m_mouseCaptured=false`; calls `ListAllDisplayModes()` via `AbstractListAllDisplayModes` — during base construction this dispatches to the **empty base version**, so derived constructors call their own `ListAllDisplayModes()` again |
| `SuggestDefaultRes` | returns the desktop width/height/refresh/bit depth captured by `SaveDesktop` |
| `GetResolutionId(w,h)` | index in `m_resolutions` with exact w,h, else −1 |
| `GetResolution(id)` | pointer or NULL |
| `GetClosestResolution(&w,&h,&refresh)` | exact (w,h) match → only snap refresh; else pick the resolution minimising `|Δw|+|Δh|` (first wins ties) and snap refresh. Refresh snapping: exact match, else entry minimising `|Δrefresh|`; if the resolution has no rates the requested value is kept |
| `EnsureMouseCaptured/Uncaptured` | capture/uncapture if state differs (called from `Camera::Advance`) |
| `WindowMoved` | resets `m_mouseOffsetX=INT_MAX` (called by `W32EventHandler` on window move — P10) |
| `m_invertY` | never initialised or read [C grep] |

### Display-mode enumeration [C] `ListAllDisplayModes` (identical in Win32 and DirectX variants)

Loop `EnumDisplaySettings(NULL, i, &devMode)` for i = 0,1,… until it fails; keep modes with `dmBitsPerPel ≥ 15 && dmPelsWidth ≥ 640 && dmPelsHeight ≥ 480`; de-duplicate by (width, height) regardless of bit depth; per resolution collect unique `dmDisplayFrequency` values. Order is driver enumeration order (not sorted). `devMode.dmSize` is not initialised before the call [C] (works in practice; strictly required by Win32). The list feeds the options screen (`code/interface/prefs_screen_window.cpp`, P17b) and `GetClosestResolution`. Colour depths are **not** stored per resolution; depth is a separate preference.

### WindowManagerWin32 — OpenGL window [B:FULL] [C] `code/lib/window_manager_win32.cpp`

**`WinMain`**: stores `HINSTANCE`, `g_windowManager = new WindowManagerWin32()`, calls `AppMain(cmdLine)`, returns `WM_QUIT` [C]. Constructor: `ListAllDisplayModes()`, `SaveDesktop()` (records `ENUM_CURRENT_SETTINGS` width, height, bpp, frequency).

**`CreateWin(w, h, windowed, colourDepth, refreshRate, zDepth, waitVRT, antiAlias, title)`**:

1. Store `m_screenW/H`, `m_windowed`. Register `WNDCLASSW{style=CS_OWNDC, WndProc, icon=IDI_APPLICATION, cursor=IDC_ARROW, background=BLACK_BRUSH, class name = title}`.
2. **Windowed**: `RestoreDesktop()` (if the current display mode differs from the saved desktop mode, `ChangeDisplaySettings(desktop, CDS_FULLSCREEN)`); style `WS_VISIBLE|WS_POPUPWINDOW|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX`; `AdjustWindowRect` → `m_borderWidth = (Δwidth)/2`, `m_titleHeight = Δheight − 2·border`; **fail (return false) if the client size exceeds the desktop**; window centred on the desktop rect.
3. **Fullscreen**: style `WS_VISIBLE|WS_POPUP`; `ChangeDisplaySettings({bpp=colourDepth, w, h, freq=refreshRate}, CDS_FULLSCREEN)`; return false unless `DISP_CHANGE_SUCCESSFUL`; position (0,0); `m_borderWidth=1`, `m_titleHeight=0`.
4. `CreateWindowW(class, title, style, x, y, w(+borders), h(+borders+title))`; false on failure; reset mouse offset.
5. `EnableOpenGL(colourDepth, zDepth)` (return value ignored).

`waitVRT` and `antiAlias` are **ignored** by the Win32 GL window manager (`m_waitVRT` never assigned) [C].

**`EnableOpenGL`**: `GetDC`; `PIXELFORMATDESCRIPTOR{PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, cColorBits=colourDepth, cDepthBits=zDepth, PFD_MAIN_PLANE}` → `ChoosePixelFormat` + `SetPixelFormat` (results unchecked; no alpha, stencil or multisample bits requested); `wglCreateContext` ×2 — primary `m_hRC` and secondary `m_hRC2` "for loading thread" joined with `wglShareLists` (failure only logged with `FormatMessage`); `wglMakeCurrent(m_hDC, m_hRC)`; `glClear(GL_COLOR_BUFFER_BIT)`. `m_hRC2` is **never made current anywhere** and never deleted (`UseSecondaryContext/ReleaseSecondaryContext/DestroySecondaryContext` are declared but not defined) [C].

**`DisableOpenGL`**: `wglMakeCurrent(NULL,NULL)`, delete `m_hRC`, `ReleaseDC`. **`DestroyWin`**: `DisableOpenGL` + `DestroyWindow`. **`Flip`**: `SwapBuffers(m_hDC)`. **`NastyPollForMessages`**: `PeekMessage/TranslateMessage/DispatchMessageW` loop (called by `W32InputDriver` — P10). `WndProc` forwards to `W32EventHandler::WndProc`, falling back to `DefWindowProcW` when the handler returns −1 or is absent.

**Mouse**: `NastySetMousePos(x,y)` lazily computes the client-origin offset (`GetWindowRect` + border + title when windowed, 0 when fullscreen), `SetCursorPos(x+off, y+off)` and tells `W32InputDriver::SetMousePosNoVelocity(x,y)`; `NastyMoveMouse(dx,dy)` = relative `SetCursorPos` + `SetMousePosNoVelocity(dx,dy)` (passes the *delta* as a position [C]); `CaptureMouse`=`SetCapture`; `UncaptureMouse`=`ReleaseCapture`; `Hide/UnhideMousePointer`=`ShowCursor(false/true)` (OS cursor — the game draws its own, P12 `targetcursor`). `OpenWebsite(url)` = `ShellExecute(NULL,"open",url,…,SW_SHOWNORMAL)` (callers: demo-end window, main menu, script "buy now", `main.cpp`).

**Vsync (GL)**: no `wglSwapIntervalEXT` call exists anywhere [C grep] ⇒ swap interval is whatever the driver default/control panel says; the `WaitVerticalRetrace` preference has no effect in the baseline [C].

**Multisampling (GL)**: none — `wglChoosePixelFormatARB` is loaded but unused; the `antiAlias` argument (always passed `true` by the renderer) is ignored [C].

**Gamma (GL)**: no gamma-ramp code exists for the GL path [C grep `SetDeviceGammaRamp`/`SetGammaRamp`].

### WindowManagerDirectX — Direct3D window [B:D3D] [C] `code/lib/window_manager_directx.cpp`

Same structure as Win32 with these differences: ANSI `WNDCLASS/RegisterClass/CreateWindow/DefWindowProc/DispatchMessage`; icon = resource `MAKEINTRESOURCE(101)`, `hCursor = NULL`; `m_waitVRT = waitVRT` is stored; **`RestoreDesktop` body is commented out** and the fullscreen branch does **not** call `ChangeDisplaySettings` (D3D performs the mode switch); `EnableOpenGL` = `Direct3DInit(m_hWnd, m_windowed, m_screenW, m_screenH, colourDepth, zDepth, m_waitVRT)` + `glClear(GL_COLOR_BUFFER_BIT)`; on failure the window is destroyed and `CreateWin` returns false; `DisableOpenGL` = `Direct3DShutdown()`; `Flip` = `Direct3DSwapBuffers()`; `EnsureMouseCaptured/Uncaptured` overridden as no-ops; `Hide/UnhideMousePointer` only toggle the flag (no `ShowCursor`); destructor also `m_resolutions.EmptyAndDelete()`. `WinMain` additionally handles `TARGET_OS_VISTA` (`-mediacenter` → `g_mediaCenter=true` + `Sleep(7500)`; otherwise a save-file path argument → `g_saveFile`, `SetCurrentDirectory(exe dir)`) — `TARGET_OS_VISTA` is never defined ([B:DEAD]). Title parameter type mismatch: see build-viability note in 24.

### Direct3D device creation [B:D3D] [C] `code/lib/opengl_directx.cpp:Direct3DInit`

| Step | Value |
|---|---|
| D3D object | `Direct3DCreate9(D3D_SDK_VERSION)` once; failure → `AppReleaseAssert("Direct3D 9 not available.")` |
| Vertex processing | `GetDeviceCaps(DEFAULT, HAL)`: `D3DDEVCAPS_HWTRANSFORMANDLIGHT` ⇒ `D3DCREATE_HARDWARE_VERTEXPROCESSING`, else `SOFTWARE` (and VBs get `D3DUSAGE_SOFTWAREPROCESSING`) |
| Adapter / type | `D3DADAPTER_DEFAULT`, `D3DDEVTYPE_HAL` (NVPerfHUD variant commented) |
| Behaviour flags | only the vertex-processing flag — **no `D3DCREATE_FPU_PRESERVE`, no `D3DCREATE_MULTITHREADED`** (see 22 risks) |
| Windowed | `Windowed=TRUE`, back-buffer size 0 (= client area), `BackBufferFormat=D3DFMT_UNKNOWN` (desktop format) |
| Fullscreen | `Windowed=FALSE`, `BackBufferWidth/Height=w/h`; colour 24/32 → `X8R8G8B8`; 15/16 → `X1R5G5B5`; other → `AppReleaseAssert("Unexpected colourDepth.")`. `FullScreen_RefreshRateInHz` not set (0 = adapter default) ⇒ **refresh-rate preference ignored** |
| Format fallback (fullscreen) | `CheckDeviceType` fails → 15/16-bit: try `R5G6B5` ("Fixes 16bit modes on Radeon X300"), then `A1R5G5B5` (display `X1R5G5B5`); 24/32-bit: `A8R8G8B8` (display `X8R8G8B8`). If the final check still fails: revert to `X8R8G8B8` (≥24) or `X1R5G5B5` and hope |
| Depth buffer | if zDepth ≠ 0: `EnableAutoDepthStencil`; 16 → `D3DFMT_D16`; 24 → `D3DFMT_D24X8`; 32 → `D3DFMT_D32`; other → `AppReleaseAssert("Unexpected zDepth.")`. No stencil |
| Swap | `SwapEffect=DISCARD`, `BackBufferCount` default (1); triple-buffering block commented out |
| Vsync | `PresentationInterval = waitVRT ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE` |
| Multisampling | `MultiSampleType` left 0 = `D3DMULTISAMPLE_NONE` |
| `CreateDevice` failure | returns false ("non fatal end, res change can help") |
| After creation | `g_pd3dDeviceActual = g_pd3dDevice`; `InitialiseData()` (matrix stacks, vertex decl, vertex array 1024, texture table cleared, cull/hints/attribs reset, mip work-queue created once); `BeginScene()`; `SetVertexDeclaration`; `CreateD3DResources()` → `DeformEffect::Create`, `WaterReflectionEffect::Create`, `new FixedPipeline` |

`Direct3DShutdown`: `ReleaseD3DResources()` (default-pool resources + water/landscape managed resources + `TaskManagerInterfaceGestures::ReleaseD3dResources`), release device and D3D object.

### Frame present and lost-device handling [B:D3D] [C]

```mermaid
sequenceDiagram
  participant R as Renderer (per frame)
  participant W as WindowManagerDirectX::Flip
  participant E as Direct3DSwapBuffers
  participant D as IDirect3DDevice9
  R->>W: Flip()
  W->>E: Direct3DSwapBuffers()
  E->>E: GenerateMipmaps() — reap finished mip tasks
  E->>D: EndScene()
  E->>D: Present(NULL,NULL,NULL,NULL)
  alt returns D3DERR_DEVICELOST
    E->>E: WaitAndResetDevice()
    E->>E: ReleaseD3DPoolDefaultResources(): delete FixedPipeline, DeformEffect, WaterReflectionEffect; water & landscape default-pool; Release ALL GL-emulated textures
    loop every 100 ms
      E->>D: TestCooperativeLevel()
      alt D3DERR_DEVICENOTRESET
        E->>D: Reset(g_d3dpp) — on failure keep looping
      end
      E->>E: g_inputManager->PollForEvents() (keeps window responsive)
    end
    E->>E: g_app->m_resource->FlushOpenGlState(); CreateD3dPoolDefaultResources(); RegenerateOpenGlState(); SetVertexDeclaration()
  end
  E->>D: SetVertexShaderConstantF(c0, identity,4); (c4, identity,4)
  E->>D: BeginScene()
```

Notes [C unless marked]: the loop exits only after a successful `Reset` (a `TestCooperativeLevel` returning `D3D_OK` without reset would spin forever [I]); managed-pool textures are released too and rebuilt through `Resource::RegenerateOpenGlState` (P12), which allocates **new** texture ids because `glDeleteTextures` is a no-op ⇒ `s_textureIds` grows on each reset [I]; a 5-second FPS counter (`FRAMES_PER_SECOND_COUNTER`) is computed but its output line is commented out.

### Window creation fallback chain and resolution switching [C] (code in P12/P17b files, behaviour owned here)

`Renderer::Initialise` (`code/renderer.cpp`) reads preferences, then:

| Attempt | Parameters | On failure |
|---|---|---|
| 0 (first run) | if `ScreenWidth`=`ScreenHeight`=0 → `SuggestDefaultRes` (desktop w,h,refresh,depth) and save to prefs | — |
| 1 | `GetClosestResolution(w,h,refresh)`; `CreateWin(w,h,windowed,depth,refresh,zDepth, waitVRT=true, antiAlias=true, L"Multiwinia")` | show `MessageDialog("Error", "Failed to set requested screen resolution of …")` at (100,100) and continue |
| 2 | desktop default res/refresh/depth, `zDepth=24`, `waitVRT` variable set 0 (but literal `true` still passed) | — |
| 3 | 800×600, windowed, 16-bit colour, 16-bit z, 60 Hz, `waitVRT` (=0) | — |
| 4 | same with 24-bit z | `AppReleaseAssert("Failed to set screen mode")` |

Afterwards all screen prefs are written back and saved; `InitialiseOGLExtensions()` and `BuildOpenGlState()` follow. Note that attempts 1–2 pass the literal `true` for `waitVRT`, so the `WaitVerticalRetrace` preference only matters on attempts 3–4 (and only in the D3D build) [C].

`RestartWindowManagerAndRenderer()` (`code/interface/prefs_screen_window.cpp`, used when applying screen options and by `SetWindowed`): records old size; if currently fullscreen and staying fullscreen (`hack`), first creates and destroys a temporary **640×480 windowed** renderer/window ("necessary for resolution change in fullscreen"); `DestroyWin()`, `delete m_renderer`, new `Renderer` + `Initialise()`, then `Resource::FlushOpenGlState()` + `RegenerateOpenGlState()` (all textures and display lists rebuilt), clear to black, reposition UI windows. On `TARGET_OS_LINUX` a "Restart Required" dialog is shown instead [B:DEAD].

### Preferences consumed by window/renderer creation [C] `code/renderer.cpp:Renderer::Initialise`, `code/lib/preference_names.h`

| Key | Default in code | Used by |
|---|---|---|
| `ScreenWidth` / `ScreenHeight` | 0 / 0 (→ desktop) | CreateWin |
| `ScreenWindowed` | 0 | CreateWin |
| `ScreenColourDepth` | 32 | GL: `ChangeDisplaySettings` bpp + PFD colour bits; D3D: back-buffer format |
| `ScreenRefresh` | 75 | GL fullscreen `dmDisplayFrequency`; ignored by D3D |
| `ScreenZDepth` | 24 | PFD depth bits / D3D depth format |
| `WaitVerticalRetrace` | 1 | D3D `PresentationInterval` (see caveat); no effect in GL |
| `ScreenOverscan` | forced 0 | written only |
| `RenderLandscapeMode` | −1 (auto) | VBO vs display list (GL) |
| `ManuallyScaleTextures` | 0 | POT rescale (`bitmap.cpp`) |

### Gamma [B:D3D] [C]

`Renderer::SetScreenGamma(gamma)` exists only under `USE_DIRECT3D`: exponent `g = 2.0 − gamma`; ramp `red[i]=green[i]=blue[i] = iv_pow(i/255, g) · 65535` for i = 0..255; `g_pd3dDevice->SetGammaRamp(0, 0, &ramp)`. **No caller exists** [C grep] ⇒ gamma control is dead in both builds.

### Video capture — `AVIGenerator` [B:DEBUG] [C] `code/lib/avi_generator.*`

Compiled only with `AVI_GENERATOR`, which only `TARGET_DEBUG` defines (and Linux/Mac `#undef`) [C] `code/lib/universal_include.h`. Based on Jonathan de Halleux's CodeProject "AVIGenerator" (header comment) using Video for Windows (`vfw.h`).

| Member | Behaviour |
|---|---|
| ctor | `m_motionBlurFactor = pref AVIMotionBlurFactor (1)`; `m_frameRate = pref DemoFrameRate (25) / m_motionBlurFactor`; `BITMAPINFOHEADER{size, w=ScreenW, h=ScreenH, planes 1, 24 bpp, BI_RGB, biSizeImage 0, 1000 ppm ×2}`; output file `"Untitled.avi"` |
| `InitEngine()` | requires VfW ≥ 1.10 (`VideoForWindowsVersion`), `AVIFileInit`, `AVIFileOpen(OF_WRITE|OF_CREATE)`, video stream header (`dwScale=1`, `dwRate=frameRate`), `AVIFileCreateStream`, `Setup()` = **interactive `AVISaveOptions` codec dialog**, `AVIMakeCompressedStream`, `AVIStreamSetFormat(m_bih)`; allocates `m_pixels` and `m_accumBuffer` of `w·h·(biBitCount / sizeof(unsigned char))` = `w·h·24` bytes (8× larger than needed); error strings in `m_error` |
| `AddFrame()` | `glReadPixels(0,0,w,h,GL_RGB)`; motion-blur accumulation: on frame `k % factor == 0` the accumulator is overwritten with `pixel·(1/factor)` (BGR swizzle), otherwise added; on `k % factor == factor−1` writes the accumulator via `AddFrame(bits)` |
| `AddFrame(bits)` | `CheckSanity()` (filename ends in "avi", w and h multiples of 4 — debug asserts), `AVIStreamWrite(compressed, outFrame++, 1, bits, biSizeImage (=0), AVIIF_KEYFRAME)` |
| `ReleaseEngine()` | release streams/file, free buffers (`delete` on arrays), `AVIFileExit` |
| dtor | if pref `RecordDemo == 2` → `ReleaseEngine()`; asserts all released |
| `GetRecordingFrameRate()` | `m_frameRate` |

Integration [C]: created by the debug **Grabber** window (`code/interface/grabber_window.cpp`, opened from `DebugKeyBindings::GrabberButton` in `code/interface/debugmenu.cpp`): *Start* sets fake-time mode and calls `InitEngine`; *Stop* restores real time and releases. `UpdateAdvanceTime` (`code/main.cpp`) uses a fixed step `1/DemoFrameRate` when pref `RecordDemo` is 1 or 2. **No code calls `AVIGenerator::AddFrame()`** [C grep] — in this tree a recording produces an AVI with no frames. `RecordDemo == 1` additionally saves `serverHistory.dat` on game shutdown (`App::ShutdownCurrentGame`, `main.cpp`) — P15/P20.

### Video playback — `AviPlayer` [B:DEAD] [C] `code/lib/avi_player.*`

Guarded by `EGAMESBUILD` (never defined; consumer `code/loaders/egames_loader.cpp`). `OpenAvi` opens video stream 0 + audio stream 0 via VfW, begins streaming at speed 2000, allocates a fixed **512×256×3** RGB frame buffer; `DecodeVideoFrame(int)` copies the DIB from `AVIStreamGetFrame` into the POT buffer with BGR→RGB swizzle (row stride 512·3; returns 1 at end, 0 otherwise, counts displayed/missed frames); `DecodeVideoFrame(float t)` → `AVIStreamTimeToSample(t·1000 ms)`; `GetAudioData(n, short*)` reads n PCM samples sequentially; `CloseAvi` releases everything. → Appendix only.

