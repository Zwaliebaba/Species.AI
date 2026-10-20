---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M0 task T9), with owner decision O67
---

# Tuning in data at M0: the tuning record, the schema generator's first subset and the server's interim read

## Context and Problem Statement

MVP-099 requires every tuning value to live in data with its stable key, loaded at server start and never written as a literal in code. ADR-062 and the data schemas component fix how records work:
- JSON authoring files;
- JSON Schema documents in a restricted subset;
- a hand-written strict parser in a project that holds no generated code;
- a schema code generator in a tool project, whose committed output CI regenerates and compares.

The content build that cooks records arrives at M1, and DAT-010 and DAT-050 say that servers never parse content numbers.

M0 task T9 builds the generator for the subset that one tuning record needs, the record of the MVP's `mvp.*` keys loaded at server start, and `Build\CheckGeneratedSchemas.py`. Its done-when is that a tuning value changed in data changes the server's behaviour with no rebuild of code.

The owner decided three things (O67):
- the server parses the authoring file at start, as an interim until M1;
- the value that proves the done-when is the tick period;
- the record therefore holds `ref.sim.tick_period` beside every key of the MVP scope's table 3.

The specifications leave open:
- the record type's id and folder, and where schemas live;
- the shape of the record, the subset and the generated code;
- where the server finds the content root before AST-105's loose-content mount exists;
- what a server does with a record it cannot read.

## Decision Drivers

* A tuning key reads the same in the design, the authoring file and the code (CONVENTIONS §4).
* The generated code is formatted and linted like hand-written code, and CI regenerates it byte for byte (O28).
* The generator builds from a tree whose generated headers are stale or missing (ADR-062).
* A malformed record never starts a server, and every finding names its file, line, column and field (DAT-021, DAT-024).

## Considered Options

* 1 — one record type `tuning.mvp` whose nested objects follow the tuning keys; a generator that writes a struct per object and a reader per struct; the server reads the authoring file
* 2 — a flat record whose field names are the whole keys (`mvp_pace_session_factor`)
* 3 — a key–value table read by key name at run time

## Decision Outcome

Chosen option: "1".
- Option 2 loses the key's structure in every identifier.
- Option 3 gives no compile-time check that a value exists, which MVP-099's "never written as a literal" needs to be enforceable.

* **The record type** is `tuning.mvp`, registered in DAT table 1-A.
  - Its authoring file is `GameData/Records/tuning/mvp.json` and its schema `GameData/Schemas/tuning.mvp.schema.json`.
  - `GameData/Schemas/` holds the JSON Schema documents, one per schema id. The generator reads them and authors' editors use them; the content build packs none of them.
  - A field's path is its tuning key: `mvp.pace.session_factor` is the field `session_factor` of the object `pace` of the object `mvp`.
  - The record holds every key of the MVP scope's table 3, 71 keys in 31 groups, at their table values, and `ref.sim.tick_period` at 0.1 s (ADR-079).
  - From M1 it also holds, under `ref.*`, every reference key the simulation reads, at its reference table's value; each M1 task adds the keys it reads (ADR-101). A vector key is an object of three decimals, `x`, `y` and `z`, because the subset has no vector type.
* **The schema subset of M0** is enforced by the generator, which refuses everything else (DAT-015):
  - `$schema` pinned to draft 2020-12;
  - `$id`, the schema id; `title` and `description`;
  - `type`, `properties`, `required` and `additionalProperties: false`;
  - `minimum`, `exclusiveMinimum` and `maximum`;
  - `x-species-type` of `object`, `integer`, `count`, `ticks` or `sim_decimal`, with `x-species-unit` and `x-species-precision` (`binary64` or `binary32-rounded`);
  - `x-species-presence` of `required`, `x-species-sim` and `x-species-project`.
  - The record's `schema` and `version` fields are text enumerations of one value, which give the generator the id and the version.
  - Every field is required: the defaulted and absence-meaningful fields of DAT-011 come with the first record that needs them.
  - A count or ticks field has minimum 0, as table 3.1-A says.
  - Of table 3's keys, the ticks keys, and the counts whose unit is a countable thing (Workers, orbs, ports, construction sites, snapshots, figures, frames per second, renown, AI settlements), are integers. Every other key is a simulation decimal at binary64, the numeric model of ADR-081.
* **The generator** is `Tools/SpeciesSchemaGen/SpeciesSchemaGen.exe`, a tool project of the one solution that references NeuronCore only.
  - It reads `GameData/Schemas/*.schema.json` in name order and writes each header to `<x-species-project>/<Type>.h`. The type name is the PascalCase of the schema id, here `GameShared/TuningMvp.h` with the type `TuningMvp`.
  - The header holds the id, the version and the simulation flag as constants, a struct per object with each field's key and unit in a comment above it, and a `ReadFields` overload per struct.
  - It also holds `ReadTuningMvp`, which returns the record or every finding about it.
  - The generator converts case only. It refuses a key that is not snake_case, that spells an R11 stem, or whose identifier is a C++ keyword (DAT-054).
  - Every line it writes is within the 140 columns of `.clang-format` and in the form clang-format leaves, so the generator refuses an over-long line rather than let the format check rewrite it.
  - A developer runs `SpeciesSchemaGen.exe --root <repository>` after a schema changes and commits the header.
* **The parser and the reader** are NeuronCore's (the record and schema runtime of ADR-090), which holds no generated code.
  - `AuthoringParser` implements DAT-019 to DAT-023: UTF-8 with an optional byte-order mark, the I-JSON restrictions, numbers through `std::from_chars`, a duplicate key reported at both places, a depth limit of 64, and one diagnostic of table 6-A with its location for malformed text.
  - `RecordReader` gives the generated readers their checks:
    - the header;
    - each required field;
    - an unknown field when its object closes;
    - `notes` of at most 2,000 characters;
    - the JSON form;
    - an integer's exact digits;
    - a decimal's range and precision.
  - It collects every finding of a record, not only the first.
* **The interim read (O67).** When the integrated server starts, it loads `Records/tuning/mvp.json` under its content root with the parser and the generated reader. It takes the tick loop's period from `ref.sim.tick_period`, to the nearest nanosecond, and gives the record to the world.
  - This departs from DAT-010 and DAT-050 until M1's content build cooks the record. The parser is the content build's own correctly rounded one, so the values are those the cooker will write.
  - A record that cannot be read leaves the server stopped and returns its findings.
  - The client logs each finding and ends with `ui.error.content_missing` when the file is absent, or `ui.error.content_damaged` otherwise. These are the compiled-in messages of table 6.4-A, whose base-language texts the composition root holds until the string table exists.
* **The interim content root.** Until AST-105's Debug loose-content mount exists, the content root is the `GameData` folder in the executable's folder or the nearest folder above it (`FindInterimContentRoot`). So a build under `x64\Debug` or `ARM64\Debug` finds the repository's. The tests find it from their own source file's path.
* **The drift check** (`Build\CheckGeneratedSchemas.py`, the step CI already guarded):
  - it builds the generator through the solution (`/t:Tools\SpeciesSchemaGen`, Debug|x64) and runs it into a temporary tree;
  - it fails on a header that differs, one that is generated but not committed, and a committed generated header that is no longer written;
  - a committed header is generated when its first line carries the generator's mark;
  - CR LF and LF compare equal, because line endings are git's;
  - its fixture proves that a changed header is reported.

### Consequences

* Good, because every tuning key is one field with one path, checked by the compiler where it is read and by the reader where it is authored.
* Good, because changing the record changes the server with no rebuild, as `ref.sim.tick_period` shows, and a broken record stops the server with every finding.
* Bad, because the server parses JSON at M0, which DAT-010 and DAT-050 forbid. The cost, unmeasured at M0, is the parse at start and the parser's code in the server; it ends at M1.
* Bad, because a record of 71 required fields must be edited whole: a missing key is an error, never a default.
* It forecloses: a tuning value read by its key's text at run time; a generated header edited by hand.

### Confirmation

* `NeuronCoreTests` checks:
  - locations by line and code-point column;
  - every refused form of malformed text, encoding and nesting, each at its place;
  - a duplicate key at both places;
  - that no truncation or single-byte change hangs or crashes the parser;
  - the reader's findings, header and precisions.
* `GameSharedTests` checks:
  - the committed record reads whole with table 3's values;
  - a changed tick period is the period read;
  - a broken record gives its findings with file and line;
  - an absent file is a missing record.
* `GameLogicTests` checks the integrated server's rising ticks at a period the record states, a server that does not start without its record, and ticks 200 ms apart when the record says 0.2 s.
* `Build\CheckGeneratedSchemas.py` finds no difference on CI.
* ADR-062's "confirmed when" is reached when the M3 build loads its records from code that the generator wrote, cooked by the content build.

## Pros and Cons of the Options

### 1 — nested objects that follow the keys

* Good, because the key, the authoring path and the code path are one.
* Bad, because table 3 has 31 groups, so the record has 31 small objects.

### 2 — a flat record

* Good, because there is one object.
* Bad, because every identifier repeats its group.

### 3 — a key–value table

* Good, because adding a key needs no generator run.
* Bad, because a missing or misspelt key is found only when it is read.

## More Information

* Related: [ADR-059](ADR-059-content-build-and-validators.md), [ADR-061](ADR-061-data-schemas-and-versioning.md), [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md).
* The code is in NeuronCore (`AuthoringParser`, `RecordReader`, `ContentDiagnostic`, `FindInterimContentRoot`), `Tools/SpeciesSchemaGen`, GameShared (`TuningMvp.h`, generated, and `Tuning`) and GameLogic (`IntegratedServer`).
* **Amendment 2026-10-20 (M1 task T2, [ADR-101](ADR-101-region-teams-figures-and-update-slices.md)).** The record holds the `ref.*` keys the simulation reads beside table 3's, each M1 task adding its own, and a vector key is an object of three decimals.
* **Amendment 2026-10-20 (M1 task T6, [ADR-105](ADR-105-combat-weapons-and-essence-orbs.md)).** The generator breaks a `ReadFields` signature after its second parameter, aligned after the open bracket, and a field assignment after its `=`, one level in, when the line would pass column 140, which is the layout clang-format gives. A key with a segment that generates a C++ keyword is renamed the first time code reads it (DAT-054): `ref.weapon.throw.*` became `ref.weapon.throwing.*` (CONVENTIONS table 4.1-A).
