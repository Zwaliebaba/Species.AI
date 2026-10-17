# Species — technology standards baseline

> Brief: F3 (Phase 1). Run date and date checked for every item: **2026-10-17** (UTC), unless an item says otherwise.
> Scope: Windows 11 client, C++23, Direct3D 12; a headless dedicated server that may also be built for Linux.
> Out of scope (design phase): networking, online services, hosting.
> Rule: every version and support status below carries its source URL. Where a source could not be reached, the item says so; nothing is stated from memory.

## Contents

1. Operating system: Windows 11 baseline, minimum build, Windows 10 status
2. Compilers and language: MSVC and Visual Studio, C++23, `import std`, C++26; Clang and GCC for a Linux server
3. Build and dependencies: CMake presets, vcpkg manifest mode
4. Direct3D 12: Agility SDK, feature level, Shader Model, Enhanced Barriers, DXC and HLSL 2021, flip-model swap chain, VRR and HDR, PIX
5. Content I/O: DirectStorage; DirectXTex and texconv; BC7, BC5, BC4
6. Input: GameInput
7. Audio: XAudio2 2.9 and X3DAudio
8. Asset formats: glTF 2.0 and KHR extensions; DDS with the DX10 header; audio source and runtime formats
9. Text: DirectWrite glyph rasterisation for a glyph-cache UI
10. Accessibility and localisation: XAG; Unicode, CLDR, BCP 47
11. Shipping: Windows Error Reporting and crash reporting; MSIX and store packaging; code signing
12. Summary table

