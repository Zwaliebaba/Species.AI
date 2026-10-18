---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Authoring syntax and schema language: JSON validated by JSON Schema, with a Species code generator

## Context and Problem Statement

[ADR-061](ADR-061-data-schemas-and-versioning.md) fixes that every record type has one schema and that authoring files are UTF-8 text compiled to versioned binary. It leaves open which text syntax authors write and which language the schemas are written in. The syntax must suit both hand-edited tables (statistics, sound bindings, edition manifests) and large editor-written files (maps). The schema language must express types, units, defaults, ranges, enumerations and typed references, drive validation with file and line, give authors editor support, and generate the C++ types and binary layouts the engine loads.

Which authoring syntax and schema language does Species use?

## Decision Drivers

* One schema source that drives validation, editor support and C++ code generation.
* A syntax that the map editor can write losslessly and that diffs well.
* A strict parser that reports the location of every value and rejects duplicate keys, the same on every build platform.
* Room for Species-specific annotations (units, logical numeric types, reference kinds).

## Considered Options

* JSON authoring files validated by JSON Schema, with a Species code generator
* TOML authoring files with an own schema language
* YAML authoring files with JSON Schema
* FlatBuffers schemas with JSON authoring and FlatBuffers binaries
* An own text format and schema language

## Decision Outcome

Chosen option: "JSON authoring files validated by JSON Schema, with a Species code generator". This is the default the team proceeds with.

* **Schemas** are JSON Schema documents, one per record type, using a restricted subset (object, array, string, integer, number, boolean, enumeration, reference) plus Species annotation keywords prefixed `x-species-` for units, logical numeric types and precision, presence, reference kinds and enumeration order. The `$schema` keyword pins the draft **2020-12** in every schema.
* **Code generation:** a Species generator in the content build tool (a tool project of the one solution under `Tools/`, owner decision O29; outside `AGENTS.md` R14) turns each schema into C++23 record types, the binary layout and the loader table. Its output is committed source: a developer runs it when a schema changes, it writes `AGENTS.md`-style `.h` files into the owning project folder, and CI regenerates them and fails on any difference (owner decision O28). Generated C++ follows `AGENTS.md` naming: PascalCase types and enumerators, files named for their primary type, plain `camelCase` fields of the generated record aggregates (R8), `UPPER_CASE` constants, the `Engine` namespace for engine records and `Species` for game records (R9), and American spelling in identifiers (R11). The generator only converts case: a snake_case key becomes a camelCase field (`color_role` becomes `colorRole`) and a snake_case enumerator string a PascalCase enumerator (`point_magnify_clamp` becomes `PointMagnifyClamp`). Every key and enumerator string that generates an identifier is therefore spelt in American English (CONVENTIONS §8), and the generator rejects one that R11 would spell otherwise (`colour`, `centre`, `grey`) or whose identifier is a C++ keyword. Schema ids and the other data names keep their spelling, and a generated declaration is the only declaration of its type (DAT-054).
* **Authoring files** are JSON. Every record may carry a `notes` string for author comments, because JSON has no comments. The JSON parser records line and column for every value so that errors name the exact location.
* **Parser and validator:** the authoring parser is Species code, a strict parser of RFC 8259 JSON with the I-JSON restrictions of RFC 7493 (UTF-8 only, no duplicate keys), and the validator is generated from the schemas; no third-party JSON or JSON Schema library sits in the content build's validation path, so duplicate-key detection and per-value locations never depend on a library's mode. Numbers are converted with the C++ standard library's correctly rounded `std::from_chars` and written back in shortest round-trip form with `std::to_chars`. The published JSON Schema documents serve authors' editors only.
* **Editors:** schemas are published to authors' text editors for inline validation and completion.

### Consequences

* Good, because one standard schema format serves the validator, editors and the code generator.
* Good, because the map editor writes the same syntax authors read.
* Bad, because JSON has no comments and is verbose for large tables; the `notes` field and editor support mitigate this.
* Bad, because the code generator, the parser and the generated validator are Species-owned and must support exactly the restricted schema subset and the JSON grammar.

### Confirmation

* The generator rejects schemas that use keywords outside the subset or unknown `x-species-` annotations.
* Each validator error class has a fixture checking the reported file, line and column.

## Pros and Cons of the Options

### JSON with JSON Schema and a Species generator

* Good, because parsers, validators and editor support are widely available.
* Neutral, because the generator is small when the schema subset is restricted.

### TOML with an own schema language

* Good, because TOML supports comments and reads well for flat tables.
* Bad, because TOML has no standard schema language, and deeply nested map data is awkward in TOML.

### YAML with JSON Schema

* Good, because YAML supports comments and is compact.
* Bad, because implicit typing (unquoted values read as booleans or numbers) and a large specification make silent data errors likely.

### FlatBuffers

* Good, because one tool gives schema, code generation, JSON parsing and a zero-copy binary format.
* Bad, because the binary layout and evolution rules belong to the library, units and logical numeric types need custom attributes, and the runtime format would no longer carry Species' own record headers.

### Own text format and schema language

* Good, because it fits Species exactly.
* Bad, because parsers, validators, editor support and documentation would all be first-party work.

## More Information

* The standards baseline does not cover JSON Schema or JSON parser libraries. The data schemas component (`DAT`) pins the draft (`dat.schema.draft`, 2020-12) and specifies the Species-owned parser and generated validator; no JSON library is pinned through [ADR-055](ADR-055-dependencies-three-nuget-packages.md).
* **Amendment 2026-10-17.** The record states the pinned draft 2020-12 (DAT-015), the Species-owned strict parser and generated validator (DAT-019 to DAT-022), and the standard-library number conversions (DAT-023, DAT-041), as the data schemas component specifies.
* **Amendment 2026-10-17 (AGENTS.md, O21).** The schema code generator is part of the content build tool under `Tools/`, a development tool outside `AGENTS.md` R14, and the C++ it generates follows `AGENTS.md` naming (R8, R9, R11).
* **Amendment 2026-10-17 (AGENTS.md §2, O26).** The content build tool, and with it the schema code generator, is a project of the separate tools solution under `Tools/` ([ADR-054](ADR-054-build-system-msbuild-solution.md), [ADR-059](ADR-059-content-build-and-validators.md)), not of the game's solution.
* Revisit trigger: if authors report that comment-less JSON slows tuning work, TOML may be added as a second syntax for flat tables, validated against the same JSON Schema data model.
* Amendment 2026-10-17 (owner decision O28): the generator's output is **committed source**. A developer runs the generator when a schema changes; it writes ordinary `.h` files in `AGENTS.md` style directly into the owning project folder, each starting with a comment that names the schema it was generated from and says it is never edited by hand. They are committed, formatted and linted like hand-written code, so the game's solution needs nothing from `Tools/`. CI regenerates them and fails on any difference (a guarded `Build/CheckGeneratedSchemas.py` step in `build.yml`).
