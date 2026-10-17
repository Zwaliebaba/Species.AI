---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Localisation: UTF-8 string tables keyed by string id, BCP 47 language tags, ICU formatting and per-key fallback

## Context and Problem Statement

All Species player-facing text is new text keyed by string ids such as `ui.lobby.ready` (CONVENTIONS §8). The game must show text in several languages, including scripts that need shaping and right-to-left layout, insert the player's current input binding into captions, and vary text by input device (keyboard and mouse or gamepad). A missing translation must never show a raw key to players; a language that is only partly translated must still read correctly. Text entry (chat, names, search) must work with input method editors.

How is text stored, identified, formatted and selected per language?

## Decision Drivers

* One encoding for every file, data record and network message.
* Standard language identifiers that the OS, the server interface and the string tables share.
* Correct plural, gender, number and date formatting per language.
* No raw keys on screen; predictable fallback.
* Validation of every language at build time ([ADR-0059](0059-content-build-and-validators.md)).

## Considered Options

* UTF-8 string tables, BCP 47 tags, ICU MessageFormat via the in-box ICU, per-key fallback
* UTF-16 string tables with printf-style placeholders
* Bundled ICU with UTF-8 string tables

## Decision Outcome

Chosen option: "UTF-8 string tables, BCP 47 tags, ICU MessageFormat via the in-box ICU, per-key fallback".

* **Encoding:** UTF-8 in every file, data record and network message; UTF-16 only at Win32 and DirectWrite call boundaries. Source files are UTF-8 (`/utf-8`, [ADR-0051](0051-windows-toolchain-and-compiler-policy.md)).
* **Language ids:** BCP 47 tags (`en-GB`, `de-DE`, `pt-BR`, `zh-Hans-CN`) in string-table names, settings and the server interface. `en-GB` is the source language of every string.
* **Selection and fallback:** the client takes the UI language from `GetUserPreferredUILanguages`, then falls back along the tag (drop the region, then the script) and finally to `en-GB`. Fallback is per key: a key missing in the selected language shows its `en-GB` text, and the content build reports every such key.
* **Formatting:** string values use ICU MessageFormat syntax for arguments, plurals and gender; numbers, dates, plural rules and collation come from the in-box ICU C API (`icu.dll`, present on every supported OS). The documented data level is Unicode 17 and CLDR 48; the in-box ICU version follows Windows servicing and is re-checked whenever the OS floor moves ([ADR-0050](0050-client-platform-and-minimum-os.md)).
* **Input-dependent text:** a string may carry variants per input device (keyboard and mouse, gamepad), chosen by the device the player last used; captions reference actions through an action-glyph argument that the UI resolves to the current binding.
* **Release rule:** a language is offered in an edition only when the content build reports full coverage of that edition's keys, or when the release sign-off accepts the listed fallback keys explicitly.
* **Text entry:** the client supports input method editors through the window-message path chosen by the input ADR.
* **Server:** the dedicated server formats almost nothing for players; it sends string ids and arguments. If it needs ICU on a non-Windows host, the distribution's ICU is used.

### Consequences

* Good, because one encoding and one tag scheme run from authoring files to the server interface.
* Good, because plural and gender rules come from CLDR rather than hand-written rules.
* Good, because no player sees a raw key.
* Bad, because the in-box ICU exposes C APIs only and its exact version varies with Windows servicing; tests must not depend on a particular CLDR revision.
* Bad, because fonts with coverage for every offered language must be licensed for embedding (text rendering is specified by the UI framework ADRs).

### Confirmation

* The content build fails when a string value has invalid MessageFormat syntax or an argument not declared for its key.
* A test selects `pt-PT` with only `pt-BR` and `en-GB` present and checks the fallback chain per key.
* A pseudo-localised language (accented, expanded text) runs in CI screenshot tests to catch truncation and hard-coded text.

## Pros and Cons of the Options

### UTF-8, BCP 47, in-box ICU, per-key fallback

* Good, because the formatting library ships with Windows and needs no redistribution.
* Neutral, because only ICU's C API is available.

### UTF-16 tables with printf-style placeholders

* Bad, because printf-style placeholders cannot express plural or gender rules and turn stray `%` signs in text into format directives.
* Bad, because UTF-16 files double the size of most text and differ from the network encoding.

### Bundled ICU

* Good, because the ICU version and CLDR data are pinned exactly.
* Bad, because the ICU data file is large and Species would have to service it itself.

## More Information

* Facts and sources: §10.2 of the technology baseline in the [ADR index](README.md) (checked 2026-10-17): ICU and CLDR releases on GitHub, the Windows locale-names and ICU pages on learn.microsoft.com. The Unicode version was taken from the ICU 78 release notes and the BCP 47 text from Microsoft's documentation, because unicode.org and rfc-editor.org were not reachable (baseline §12.1).
* Components: UI framework (`UI`), data schemas (`DAT`).
