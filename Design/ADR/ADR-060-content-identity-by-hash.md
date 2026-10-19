---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Content identity: SHA-256 over cooked content, produced by the content build

## Context and Problem Statement

Client and server must agree on every input the simulation reads: each map (including every terrain input the terrain runtime model consumes), unit statistics, rules, collision shapes and the asset lists the simulation draws from. A map must be identified by what it contains, not by its file name, so that a modified map is detected. Code must never contain a compiled-in table of map identities that silently breaks whenever a map changes. How identities are exchanged and compared before a session starts belongs to the networking design; this ADR defines how they are produced.

How is the identity of content, and of a map in particular, defined and produced?

## Decision Drivers

* Identity changes exactly when the cooked content changes, and not when only formatting or line endings of the authoring file change.
* Collision resistance strong enough that a modified file cannot be made to match on purpose.
* Produced by the content build ([ADR-059](ADR-059-content-build-and-validators.md)), never computed by hand or compiled into code.
* Cheap enough to verify on every load of simulation-relevant entries ([ADR-058](ADR-058-pack-files-and-mount-policy.md)).

## Considered Options

* SHA-256 over cooked entries, combined in a content manifest
* A 64-bit or 128-bit non-cryptographic hash over cooked entries
* A checksum over authoring files

## Decision Outcome

Chosen option: "SHA-256 over cooked entries, combined in a content manifest".

* **Entry hash:** SHA-256 of the uncompressed cooked bytes of each entry. The content build's output is canonical (sorted, no timestamps), so the same content always gives the same hash.
* **Map identity:** the entry hash of the cooked map asset, which contains the map header, every placed object and every terrain input. Code and data refer to maps by their stable map id (`map_<mode>_<players>p_<nn>`); the hash is the content version of that id, never a substitute name.
* **Simulation content identity:** SHA-256 over the sorted list of (key, entry hash) pairs of every entry flagged simulation-relevant in the current mount set, including the simulation-relevant runtime feature flags of [ADR-056](ADR-056-products-configurations-and-feature-flags.md).
* **Content manifest:** written by the content build next to the packs; lists every entry's key, hash and simulation-relevant flag, and the simulation content identity per edition.
* **Display:** logs and the build details screen show the first 16 hexadecimal digits; every comparison uses the full 32 bytes.
* **Out of scope:** when and how client and server exchange and compare identities, and what a mismatch does to a session (networking design); how stored worlds record the content they were created with (persistence design).

### Consequences

* Good, because any change to simulation-relevant content changes the identity, and formatting-only edits of authoring files do not.
* Good, because no compiled-in identity table exists to fall out of date.
* Bad, because SHA-256 costs more CPU than a non-cryptographic hash; the cost applies to simulation-relevant entries at load and is measured by the asset pipeline's performance tests.

### Confirmation

* A test reformats an authoring file (whitespace and line endings only), rebuilds, and expects an unchanged entry hash.
* A test changes one statistic and expects a changed entry hash and a changed simulation content identity.
* `Build\CheckProjectFiles.py` rejects source files that embed map content hashes ([build tooling](../architecture/components/build_tooling_and_distribution/README.md) table 4.1-B row 14, BLD-076).

## Pros and Cons of the Options

### SHA-256 over cooked entries

* Good, because it is a standard, collision-resistant hash available in Windows and in common libraries on every server platform.
* Neutral, because it is slower than non-cryptographic hashes, which is acceptable for Species' content volume.

### A non-cryptographic hash

* Good, because it is faster.
* Bad, because a deliberately modified file can be made to collide, which defeats the purpose of comparing identities.

### A checksum over authoring files

* Bad, because line-ending or formatting changes alter the identity while the content is unchanged.
* Bad, because the runtime never sees authoring files, so it could not verify the identity.

## More Information

* Related: [ADR-057](ADR-057-version-identity.md) (the build identity includes the content manifest hash).
* Components: asset pipeline and resources (`AST`), data schemas (`DAT`).
* The terrain runtime model, the networking model and persistence are design-phase decisions; this ADR stays valid for any of their outcomes because it hashes whatever the cooked map contains.
* **Amendment 2026-10-18.** The check against embedded map hashes is a rule of the project-file checker `Build\CheckProjectFiles.py` (build tooling table 4.1-B row 14), not a separate lint step.
