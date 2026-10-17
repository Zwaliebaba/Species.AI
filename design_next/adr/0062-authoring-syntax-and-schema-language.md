---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Authoring syntax and schema language: JSON validated by JSON Schema, with a Species code generator

## Context and Problem Statement

[ADR-0061](0061-data-schemas-and-versioning.md) fixes that every record type has one schema and that authoring files are UTF-8 text compiled to versioned binary. It leaves open which text syntax authors write and which language the schemas are written in. The syntax must suit both hand-edited tables (statistics, sound bindings, edition manifests) and large editor-written files (maps). The schema language must express types, units, defaults, ranges, enumerations and typed references, drive validation with file and line, give authors editor support, and generate the C++ types and binary layouts the engine loads.

Which authoring syntax and schema language does Species use?

## Decision Drivers

* One schema source that drives validation, editor support and C++ code generation.
* A syntax that the map editor can write losslessly and that diffs well.
* Mature parsers and validators for C++ on every build platform.
* Room for Species-specific annotations (units, logical numeric types, reference kinds).

## Considered Options

* JSON authoring files validated by JSON Schema, with a Species code generator
* TOML authoring files with an own schema language
* YAML authoring files with JSON Schema
* FlatBuffers schemas with JSON authoring and FlatBuffers binaries
* An own text format and schema language

## Decision Outcome

Chosen option: "JSON authoring files validated by JSON Schema, with a Species code generator". This is the default the team proceeds with.

* **Schemas** are JSON Schema documents, one per record type, using a restricted subset (object, array, string, integer, number, boolean, enumeration, reference) plus Species annotation keywords prefixed `x-species-` for units, logical numeric types, reference kinds and enumeration order. The schema draft is pinned by the `$schema` keyword when the validator library is chosen.
* **Code generation:** a Species generator in the content build turns each schema into C++23 record types, the binary layout and the loader table; generated files are build outputs, never edited.
* **Authoring files** are JSON. Every record may carry a `notes` string for author comments, because JSON has no comments. The JSON parser records line and column for every value so that errors name the exact location.
* **Editors:** schemas are published to authors' text editors for inline validation and completion.

### Consequences

* Good, because one standard schema format serves the validator, editors and the code generator.
* Good, because the map editor writes the same syntax authors read.
* Bad, because JSON has no comments and is verbose for large tables; the `notes` field and editor support mitigate this.
* Bad, because the code generator is Species-owned and must support exactly the restricted schema subset.

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

* The standards baseline does not cover JSON Schema or JSON parser libraries; the draft version and the libraries are chosen and pinned in the data schemas component (`DAT`) through [ADR-0055](0055-dependencies-vcpkg-manifest-mode.md).
* Revisit trigger: if authors report that comment-less JSON slows tuning work, TOML may be added as a second syntax for flat tables, validated against the same JSON Schema data model.
