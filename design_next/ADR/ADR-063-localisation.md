---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Localisation: UTF-8 string tables keyed by string id, BCP 47 language tags, ICU formatting and per-key fallback

## Context and Problem Statement

All Species player-facing text is new text keyed by string ids such as `ui.lobby.ready` (CONVENTIONS §8). The game must show text in several languages, including scripts that need shaping and right-to-left layout, insert the player's current input binding into captions, and vary text by input device (keyboard and mouse or gamepad). A missing translation must never show a raw key to players, and the engine needs a coverage report per language from which the list of offered languages is built; which languages are offered is product policy (DEC-099), not engine technology. Text entry (chat, names, search) must work with input method editors.

How is text stored, identified, formatted and selected per language?

## Decision Drivers

* One encoding for every file, data record and network message.
* Standard language identifiers that the OS, the server interface and the string tables share.
* Correct plural, gender, number and date formatting per language.
* No raw keys on screen; predictable fallback.
* Validation of every language at build time ([ADR-059](ADR-059-content-build-and-validators.md)).

## Considered Options

* UTF-8 string tables, BCP 47 tags, ICU MessageFormat via the in-box ICU, per-key fallback
* UTF-16 string tables with printf-style placeholders
* Bundled ICU with UTF-8 string tables

## Decision Outcome

Chosen option: "UTF-8 string tables, BCP 47 tags, ICU MessageFormat via the in-box ICU, per-key fallback".

* **Encoding:** UTF-8 in every file, data record and network message; UTF-16 only at Win32 and DirectWrite call boundaries. Source files are UTF-8 (`/utf-8`, [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)).
* **Language ids:** BCP 47 tags (`en-GB`, `de-DE`, `pt-BR`, `zh-Hans-CN`) in string-table names, settings and the server interface. `en-GB` is the source language of every string.
* **Selection and fallback:** at start-up the client tries, in this order, until a language loads: the stored language setting; the OS preferred UI languages from `GetUserPreferredUILanguages`, each falling back along its tag (drop the region, then the script); `en-GB`; the first offered language. The language that loads is stored as the setting. Fallback is per key: a key missing in the selected language shows its `en-GB` text, and the content build reports every such key.
* **Formatting:** string values use ICU MessageFormat syntax for arguments, plurals and gender; numbers, dates, plural rules and collation come from the in-box ICU C API (`icu.dll`, present on every supported OS). The documented data level is Unicode 17 and CLDR 48; the in-box ICU version follows Windows servicing and is re-checked whenever the OS floor moves ([ADR-050](ADR-050-client-platform-and-minimum-os.md)).
* **Input-dependent text:** a string may carry variants per input device (keyboard and mouse, gamepad), chosen by the device the player last used; captions reference actions through an action-glyph argument that the UI resolves to the current binding.
* **Coverage report and offered languages:** the content build reports, per edition, each language's coverage of that edition's keys and lists every key that would fall back; the edition's list of offered languages is built from that report. Which languages an edition offers is product policy and belongs to DEC-099 (a language is offered only when it is complete); this record specifies only the mechanism.
* **Text entry:** the client supports input method editors through the window-message path chosen by the input ADR ([ADR-028](ADR-028-input-device-back-ends.md)); composition, caret, selection and clipboard follow [ADR-032](ADR-032-ui-focus-graph-page-stack-and-input.md).
* **Server:** the dedicated server formats almost nothing for players; it sends string ids and arguments. If it needs ICU on a non-Windows host, the distribution's ICU is used.

### Consequences

* Good, because one encoding and one tag scheme run from authoring files to the server interface.
* Good, because plural and gender rules come from CLDR rather than hand-written rules.
* Good, because no player sees a raw key.
* Bad, because the in-box ICU exposes C APIs only and its exact version varies with Windows servicing; tests must not depend on a particular CLDR revision.
* Bad, because fonts with coverage for every offered language must be licensed for embedding (text rendering is specified by [ADR-034](ADR-034-text-rendering-and-glyph-cache.md)).

### Confirmation

* The content build fails when a string value has invalid MessageFormat syntax or an argument not declared for its key.
* A test selects `pt-PT` with only `pt-BR` and `en-GB` present and checks the fallback chain per key.
* A test runs the start-up selection with each step of the order failing in turn and checks the language chosen and stored.
* The content build's coverage report lists every missing key per language and edition, and the offered-language list of each edition matches the report under the policy of DEC-099.
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

* Facts and sources: technology baseline item 10.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): ICU and CLDR releases on GitHub, the Windows locale-names and ICU pages on learn.microsoft.com. The Unicode version was taken from the ICU 78 release notes and the BCP 47 text from Microsoft's documentation, because unicode.org and rfc-editor.org were not reachable (see [Items not verified](README.md#items-not-verified)).
* Components: UI framework (`UI`), data schemas (`DAT`), asset pipeline (`AST`), build tooling (`BLD`).
* **Amendment 2026-10-17.** This record covers only the technical mechanism: per-key fallback, the coverage report and building the offered-language list from it. Which languages ship is product policy in DEC-099; the sign-off exception for incomplete languages is removed, so the record agrees with DEC-099, R-META-463, AST-088 and BLD-081. The start-up order adds the stored setting first and the first offered language last, as R-META-465 specifies.
