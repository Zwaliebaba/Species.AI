---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Pack files: own indexed, chunked and hashed format; explicit mount order; loose files only in development builds

## Context and Problem Statement

Species ships its cooked content (textures, meshes, audio, data records, maps, string tables, shaders) in pack files. The format must give fast random access without decompressing whole archives, verify the integrity of every entry, especially simulation-relevant ones, support patches by an explicit mount order, and be readable by the client and by a headless server on any operating system. Players' clients must not be able to replace simulation-relevant data by dropping files next to the game, because that data is part of the content identity that client and server compare ([ADR-0060](0060-content-identity-by-hash.md)). Developers, however, need fast iteration on single files.

What container format does Species use for cooked content, and how are packs and loose files mounted?

## Decision Drivers

* Random access to any entry, with per-entry or per-chunk decompression.
* Integrity: a hash per entry and a hash over the pack's table of contents.
* An explicit, logged mount order; no implicit override.
* No loose-file override in profile or shipping builds.
* A format readable on any server operating system, and ready for a DirectStorage back-end later.
* Open, unencumbered compression codecs.

## Considered Options

* An own indexed pack format with independently compressed chunks
* A ZIP container with stored or Deflate entries
* Loose cooked files in the install directory
* DirectStorage-only packages

## Decision Outcome

Chosen option: "An own indexed pack format with independently compressed chunks".

* **Layout:** a fixed header (Species magic value, format version, flags), the chunk area, and a table of contents (TOC) with its own SHA-256. Entry records hold the canonical key, content type, schema id and version for data records ([ADR-0061](0061-data-schemas-and-versioning.md)), uncompressed size, chunk range, a simulation-relevant flag and the SHA-256 of the uncompressed entry. The extension is `.pack`.
* **Chunks:** at most 256 KiB uncompressed, aligned to 64 KiB in the file, each with its own codec id. Codecs: stored (for already-compressed payloads such as streamed audio) and Zstandard (the default). The codec id field reserves values so that LZ4 or a GPU-decompressible codec can be added without a format change.
* **Keys:** UTF-8, lower case, `/`-separated, relative to the content root. The content build rejects keys that differ only in case and keys that are not canonical; lookups are exact on the canonical key. Zero-byte entries are valid entries.
* **Reads:** through one asynchronous I/O interface, backed by memory-mapped files or streamed reads. DirectStorage is **not** adopted in v1; the chunk size and alignment keep a DirectStorage back-end (1.3.0 stable, or 1.4 once it leaves preview) addable behind the same interface.
* **Integrity:** the TOC hash is checked at mount in every flavour. Every simulation-relevant entry is checked against its hash on every load; other entries are checked in `Debug` and `Development` builds, in QA runs and by the repair function.
* **Mount order:** an ordered mount manifest lists the base packs, then patch packs. A later pack overrides an earlier entry only where the manifest marks it as a patch for that pack; every override is logged. A missing required pack is a start-up error with a clear message; optional packs are declared as optional.
* **Loose files:** a loose-content directory can be mounted last, by an explicit start-up option, in `Debug` and `Development` builds only. Profile and shipping builds do not contain the loose-file mount code ([ADR-0056](0056-editions-build-flavours-and-feature-flags.md)). A session that runs with loose files reports a modified content identity.

### Consequences

* Good, because any entry can be read without decompressing a whole archive, and memory holds only what is loaded.
* Good, because integrity failures are detected at mount or load, with the key of the failing entry.
* Good, because players cannot override simulation-relevant data in shipping builds, which the content-identity comparison relies on.
* Good, because the format is platform-neutral, so the dedicated server reads the same packs.
* Bad, because Species owns a reader, a writer and their tests instead of using an off-the-shelf archive.
* Bad, because modding through loose files is not possible in shipping builds; modding is a later design topic (vision §7).

### Confirmation

* Unit and fuzz tests of the reader: truncated files, wrong TOC hash, out-of-range chunk references, unknown codec ids.
* An integration test mounts base and patch packs, checks the effective entry for every key and checks the override log.
* A CI check confirms that profile and shipping binaries contain no loose-file mount code.
* A round-trip test: cook, pack, mount, read every entry and compare its SHA-256 with the content manifest.

## Pros and Cons of the Options

### Own indexed pack format with independently compressed chunks

* Good, because chunking, alignment, hashing and mount rules are designed for Species' needs and for a later DirectStorage back-end.
* Bad, because it is first-party code to maintain.

### ZIP container

* Good, because tools to inspect it are everywhere.
* Bad, because per-entry hashes, chunk alignment and modern codecs are extensions that common tools do not honour, so the format would be ZIP in name only.

### Loose cooked files

* Good, because it is simple to build and patch.
* Bad, because any file can be replaced or deleted by the player, and opening thousands of small files is slow.

### DirectStorage-only packages

* Good, because it gives the fastest I/O path on NVMe drives.
* Bad, because the dedicated server may run where DirectStorage does not exist, and Species' small, flat-shaded content is limited by parsing and upload rather than I/O.

## More Information

* Facts and sources: §5.1 of the technology baseline in the [ADR index](README.md) (checked 2026-10-17): the DirectStorage NuGet package and the DirectStorage 1.4 announcement.
* Related: [ADR-0059](0059-content-build-and-validators.md) (writes the packs), [ADR-0071](0071-install-layout-runtime-files-and-user-data.md) (where packs are installed).
* Component: asset pipeline and resources (`AST`).
