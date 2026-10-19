# Engine components

> Status: Engine baseline · Index of the component specifications · Related: [Architecture](../README.md) · [Architecture decision records](../../ADR/README.md) · [Reference mechanics](../../reference/README.md)

Each component specification states what the component owns and must not do, its C++23 interface sketch, data model, numbered requirements, concurrency and performance budget, failure modes and diagnostics, acceptance tests, and the open decisions and design-phase inputs that concern it. Components cite the reference requirements they serve and the ADRs that decide their technology, and never contradict an ADR. What a component specifies is engine code in namespace `Engine`, except a type that has to know a game concept by name, such as the renderers of game objects, the match camera or the page catalogue of the front end: that is game-layer code in `Species`, whichever component describes it, built on the generic engine types (`AGENTS.md` R9; [conventions](../../CONVENTIONS.md#8-naming) §8).

| Component | Prefix | Scope |
|---|---|---|
| [Renderer](renderer/README.md) | `RND` | Direct3D 12 renderer: frame graph and passes, view, lighting and fog, world and entity renderers, shaders, visual language, options and budgets |
| [Terrain](terrain/README.md) | `TER` | terrain generation, height field, surface and the queries the simulation uses |
| [Asset pipeline and resources](asset_pipeline_and_resources.md) | `AST` | import, cooking, packs, content identity, residency and resource lifetime |
| [Animation and VFX](animation_and_vfx/README.md) | `VFX` | effect events, explainers, sprites, procedural animation, particles, shatter and screen effects |
| [Audio engine](audio_engine/README.md) | `AEN` | voices, buses, effects, residency, the command queue and spatialisation |
| [UI framework](ui_framework/README.md) | `UI` | retained UI core and widgets, layout, rendering, input and localisation, pages and screen recipes |
| [Input](input/README.md) | `INP` | device back-ends, the event model, bindings and the action catalogue |
| [Camera and sequences](camera_and_sequences.md) | `CAM` | the camera rig, view state, timelines and scripted sequences |
| [Data schemas](data_schemas/README.md) | `DAT` | the schema language and every authored and cooked record family |
| [Platform layer](platform_layer.md) | `PLT` | process lifecycle, window, clock, user data, settings store, crash path and system services |
| [Build tooling and distribution](build_tooling_and_distribution/README.md) | `BLD` | toolchain, the solutions and their configurations, content build, editions, CI, packaging and crash symbols |
| [Diagnostics, test and performance](diagnostics_test_and_performance/README.md) | `DBG` | logging, the black box, profiling, development overlays and commands, soak and performance tests |

> **Amendment 2026-10-19 (owner decision O60, ADR-090).** The components are built into the libraries of ADR-090: the platform layer without a window, data schemas, terrain generation, the transport and diagnostics in NeuronCore; the renderer, the window, terrain rendering, animation and VFX, input, audio, the UI framework, the camera and the client's asset loading in NeuronClient; the server host in NeuronServer. Engine code is in namespace `Neuron`, so the sketches of every component, first written with `Engine::`, were renamed to `Neuron::`, and the programs are `Species.exe` and `SpeciesServer.exe`.
