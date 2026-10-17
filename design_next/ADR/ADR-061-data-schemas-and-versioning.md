---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Data schemas: one schema per record type, text for authoring, versioned binary for runtime

## Context and Problem Statement

Species is data-driven: unit and building statistics, supply-crate tables, sound bindings, effect definitions, maps, string tables, input bindings and edition manifests are content, not code. These records must be bound by field name, never by row or block order; every field needs a type, unit, default and valid range; references between records must be checked; and a map's menu header must be readable without parsing its terrain. Runtime loaders must be fast and must never fail on authoring mistakes such as a blank line or a missing token, because the content build ([ADR-059](ADR-059-content-build-and-validators.md)) catches those first.

How are Species data records defined, authored, compiled and versioned?

## Decision Drivers

* One definition per record type, shared by the content build, the editor and the runtime.
* Named fields with explicit types, units, defaults and ranges; no sentinel values.
* Errors reported at build time with file, line and field.
* Explicit, checked versioning of every record type.
* Numeric representation of simulation values left to the design phase's numeric model.

## Considered Options

* Schema-defined text authoring compiled to versioned binary records
* Schema-defined text read directly at run time
* Binary records edited only through tools

## Decision Outcome

Chosen option: "Schema-defined text authoring compiled to versioned binary records".

* **Schema per record type:** field names, logical types, units, defaults, ranges, enumerations (with an explicit order where the order carries meaning, such as priority or update order), and typed references to other records by stable id. Stable ids are distinct from display text, which lives in string tables ([ADR-063](ADR-063-localisation.md)).
* **Logical numeric types:** a schema declares whether a number is a simulation value, a presentation value or an integer count. The content build parses decimal text with one platform-independent, correctly rounded parser and writes simulation values in the representation that the design phase's numeric model chooses (DEC-240); until then they are stored in the representation of the current numeric default (DEC-241): each simulation decimal declares its precision, binary32-rounded (rounded once to binary32 in the content build and held widened) or full binary64. Clients and servers never parse content numbers from text, so every process reads the same bits.
* **Defaults:** one defaults table per record type, in the schema; a field that an authoring file omits takes its declared default, never 0 or a loader's own fallback. Settings defaults also come from one schema-defined table compiled into the build, so they are available before any pack is mounted. Other engine definition tables that the code and the validator must share (for example the audio effect-kind table) are schema-defined records compiled into the engine by the code generator, with no pack entry.
* **Authoring:** UTF-8 text files with named fields (syntax in [ADR-062](ADR-062-authoring-syntax-and-schema-language.md)).
* **Runtime:** the content build writes binary records into pack files ([ADR-058](ADR-058-pack-files-and-mount-policy.md)); each record type carries a schema id and version. Loaders check the header, version and hash, and never parse authoring text.
* **Versioning:** adding an optional field with a default raises the minor version; removing, renaming or re-typing a field raises the major version and requires a migration step in the content build that rewrites the authoring files in the repository. Cooked content and binaries are released together, so a loader accepts exactly the schema versions it was built with.
* **Maps:** one map asset per map, with a separately readable header (name string id, supported modes, player counts, difficulty, suggested duration, thumbnail reference), explicit start-position records, typed mode objects with validated links, and per-map rule overrides as a typed table.
* **Out of scope:** files written at run time (settings files, saved worlds, replays) and network messages, which the design phase specifies.

### Consequences

* Good, because positional binding, sentinel values and runtime parse failures are impossible by construction.
* Good, because the menu reads map headers without loading terrain.
* Good, because the simulation's numeric representation can be chosen later without changing any authoring file.
* Bad, because every schema change needs a version bump and, for major changes, a migration step.

### Confirmation

* A test loads every cooked record type and checks its schema id and version against the loader's table.
* A test reorders the records of an authoring file and expects identical cooked output.
* A test mounts a pack built with a mismatched schema version and expects a clear refusal naming the record type.

## Pros and Cons of the Options

### Text authoring compiled to versioned binary

* Good, because authors get diffable text and the runtime gets fast, validated binary.
* Neutral, because it needs a schema compiler, which the content build provides.

### Schema-defined text at run time

* Good, because there is no compile step.
* Bad, because parse errors and slow parsing move to players' machines, and the server would need the authoring parser.

### Binary records edited only through tools

* Bad, because changes cannot be reviewed as text diffs and every edit needs a tool.

## More Information

* Related: [ADR-060](ADR-060-content-identity-by-hash.md) (record hashes), [ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md) (edition manifests are schema records).
* Component: data schemas (`DAT`).
* **Amendment 2026-10-17.** Simulation values are stored in the representation of the current numeric default (DEC-241) until DEC-240 decides the numeric model, as the data schemas component specifies (DAT-010, DAT-023); omitted fields take their declared defaults (DAT-012, DEC-567); engine definition tables such as the effect-kind table are compiled into the engine and read by the validator, with no pack entry (DAT-273).
