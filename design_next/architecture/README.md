# Architecture

> Status: Engine baseline · Related: [Engine components](components/README.md) · [Architecture decision records](../adr/README.md) · [Decision register](../decisions/README.md) · [CONVENTIONS](../CONVENTIONS.md)

This folder holds the Species engine baseline. It is organised as arc42: the building-block view at level 3 is the set of component specifications in [components/](components/README.md), each written to the component template of [CONVENTIONS](../CONVENTIONS.md) (responsibilities, interfaces, data model, behaviour, concurrency and performance, failure modes, acceptance tests, open decisions). Technology choices are recorded once, as MADR 4 records in [adr/](../adr/README.md), and the components cite them.

The world-level arc42 sections (context and scope, solution strategy, the level-1 and level-2 building blocks, the runtime and deployment views for the integrated and dedicated servers, cross-cutting concepts and quality scenarios) are written by the Species design phase, which builds on these components and on the deferred decisions in [decisions/deferred/](../decisions/deferred/README.md).
