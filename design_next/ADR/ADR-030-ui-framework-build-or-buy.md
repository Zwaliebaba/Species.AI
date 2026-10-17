---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# UI framework: an own retained framework

## Context and Problem Statement

Every Species screen and in-match panel is built from the same pieces: full-screen pages built from declarative widget lists, a page graph with Back as the universal way out, buttons that cascade in and pages that swipe, option lists whose shown value is the value confirmed by the authority rather than the value requested, captions that change with the input device, a single navigation cursor shared by mouse hover and gamepad, and the power-slot and objective panels of the match HUD. These page patterns are kept (the UI component, `architecture/components/ui_framework/`). The look uses three blend modes for UI (alpha, additive and a darkening "ink" mode), and text is drawn from a glyph cache (ADR-034). The UI must support gamepad navigation, every supported language with IME text entry, DPI and text scaling, a high-contrast palette, and an accessible name on every widget so screen narration can be added (ADR-064). Its state must survive window resizes and language changes.

The problem is whether Species builds its own UI framework or adopts a third-party one, and what shape the framework has.

## Decision Drivers

- The UI renders through Species' renderer: its blend modes, its glyph cache and its frame graph.
- Gamepad navigation is first-class, through an explicit focus graph (ADR-032).
- Layouts are data; widgets have typed identities; state lives in view-models, not in widgets.
- Localisation, IME, DPI and text scaling, contrast palettes and accessible names are built in (ADR-033, ADR-063, ADR-064).
- The kept page patterns map directly onto the framework.
- Licence cost and long-term maintenance.

## Considered Options

1. An **own retained UI framework** (`UiCore`), data-driven, rendering through the batched UI pipeline (ADR-031).
2. **Third-party UI middleware** with a Direct3D 12 back-end and gamepad navigation.
3. A **web-technology UI** (an embedded HTML and CSS engine).
4. A **code-driven UI library that rebuilds the interface every frame**, used for the game UI as well as for tools.

## Decision Outcome

Proposed option: **1, an own retained UI framework**. **Default while Proposed:** the team builds `UiCore`, because the UI's needs are specific (three blend modes, glyph cache, focus graph, kept page patterns) and modest in size, and an own framework meets them with no integration seams. Middleware remains acceptable if a candidate meets every decision driver; choosing one is a revision of this ADR.

- **Retained tree.** The UI is a retained tree of widgets with **typed handles**; names exist only for debugging and as localisation keys, never for lookup.
- **Data-driven layouts.** Every page is described in data (in the schema of ADR-061): its widgets, their layout on the screen-fraction grid (ADR-033), their string ids, their focus neighbours (ADR-032) and their bindings to the page's view-model.
- **View-models.** Each page has a view-model that owns its state. Widgets bind to it; rebuilding a layout (resize, DPI change, language change) never loses state.
- **Page stack.** Pages are pushed, popped and replaced on an explicit page stack with a deferred switch (ADR-032).
- **Rendering.** Widgets emit quads, lines and glyph runs into the batched UI pipeline (ADR-031); the framework draws nothing directly.
- **Text.** Text comes from string tables by string id (ADR-063) and is drawn through the glyph cache (ADR-034). Text entry goes through the UI's text service with IME support (ADR-028).
- **Accessibility.** Every widget has an accessible name and role; palettes, including the high-contrast palette, are data; the text-scale setting feeds layout and the glyph cache (ADR-064).
- **Guarantees.** Escape and Back never apply a change; input is handled in the update phase, never in drawing; message text is never interpreted as a format string; no widget storage has a fixed capacity that user data can overflow.
- **Developer tools** may use a separate immediate-mode library for debug panels, drawn through the same batched pipeline; it is never used for player-facing UI.

### Consequences

- Good, because the framework fits the look, the renderer and the kept page patterns exactly.
- Good, because there is no licence cost and no third-party release cycle.
- Good, because accessibility and localisation hooks exist from the first widget.
- Bad, because the framework (layout, focus, text service, widgets) is engine work that a middleware would provide.
- Bad, because there is no ready-made visual authoring tool; layouts are authored as data, with a hot-reload preview in development builds.

### Confirmation

- UI tests drive every page through the page stack with keyboard, mouse and gamepad, and check focus order, Back behaviour and that no state is lost on resize, DPI change and language change.
- A data validator rejects layouts with unknown widget kinds, missing string ids, unbound view-model fields or focus neighbours that do not exist.
- An accessibility check fails when a widget has no accessible name.

## Pros and Cons of the Options

### 1. Own retained UI framework

- Good, because it meets the drivers with no integration seams.
- Bad, because it is engine work to build and maintain.

### 2. Third-party UI middleware

- Good, because widgets, layout and tooling come ready-made.
- Bad, because the three UI blend modes, the glyph cache and the focus graph must be integrated into or around its renderer, and the licence and release cycle are outside Species' control.

### 3. Web-technology UI

- Good, because layout and styling tools are mature and widely known.
- Bad, because an embedded browser engine is large, costs memory and start-up time, and composes poorly with additive and darkening blending over the 3-D scene.

### 4. Every-frame code-driven UI library for the game UI

- Good, because it is quick to write and needs no layout data.
- Bad, because layouts in code are not localisable or data-driven, focus navigation for gamepads is weak, and widget state lives in calling code.

## More Information

- Related: ADR-028 (text input), ADR-031 (batched UI rendering), ADR-032 (focus graph and page stack), ADR-033 (layout, scaling and DPI), ADR-034 (text rendering), ADR-061 (data schemas), ADR-063 (localisation), ADR-064 (accessibility), ADR-068 (profiling overlay).
- The UI framework component (`architecture/components/ui_framework/`) specifies the widget set, the page patterns and the screen recipes.
