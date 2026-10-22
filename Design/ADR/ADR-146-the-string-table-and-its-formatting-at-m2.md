---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T14)
---

# The string table and its formatting at M2

## Context and Problem Statement

M2 task T14's second step brings the en-GB string table, its look-up and the formatting that the HUD's strings need (ADR-063, MVP-100). The M2 plan's run check for it is that a string missing from the table shows its key in a Debug build. The first step drew GameFont's text ([ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md)). The third brings the page stack, focus, layout and DPI.

The data schemas specify the table (DAT-206 to DAT-218): an object of entries keyed by string id, each with variants per input mode and caption markers expanded when the table is built. ADR-063 specifies the rest:
- ICU MessageFormat syntax with named arguments;
- the number forms and plural rules of the in-box ICU;
- per-key fallback to the base language;
- no raw key shown to a player.

These decisions are open:
- **Formatting.** The in-box ICU's C API (`umsg_*`) takes a message's arguments as a positional variable argument list, so it cannot take the named arguments that DAT-208 and UI-227 require.
- **The schema.** The schema generator reads fixed fields only (DAT-026), and a string table is a map keyed by id.
- **Missing strings.** A Debug build should show a missing string, which UI-219 forbids.
- **Bindings.** Binding arguments name keys from bindings that only T15's settings store can change.
- **Who writes the strings.** The owner writes the en-GB strings (the M2 plan's §10), and the code needs ids before the strings exist.

## Decision Drivers

* No third-party code (AGENTS.md R14): ICU is the operating system's (ADR-050), its header and library the Windows SDK's.
* The content build refuses a bad string with its file and id, so no player meets one (ADR-059, DEC-015).
* The headless server formats nothing for players (ADR-063), so ICU stays on the client.
* Formatting is tested on CLDR data, not on hand-written rules.

## Considered Options

For formatting:
1. **Species' own parser of a MessageFormat subset**, shared by the content build and the client, with numbers and plural categories from the in-box ICU's C API (`unum_*`, `uplrules_*`).
2. **ICU's `umsg_*`**, with each named argument numbered when the table is cooked.
3. **Hand-written en-GB rules** for plurals and grouping, with no ICU.

For a missing string in a Debug build:
4. **Its id**, which the M2 plan's run check asks for.
5. **The empty text, logged once**, as UI-219 states.

## Decision Outcome

Chosen options: 1 and 4.
- Option 2 rewrites every message and its argument list at cook time, and loses the names that a failing check reports.
- Option 3 is CLDR by hand, which ADR-063 rules out.
- Option 5 leaves every HUD string blank until the owner writes it, so a missing string cannot be told from an empty one.

* **The message subset** (`NeuronCore/MessagePattern`):
  - Text in ICU's default apostrophe quoting.
  - Named arguments, of ASCII letters, digits and underscores; a numbered argument is refused (UI-227).
  - `{name}`, `{name, number}` and `{name, number, integer}`.
  - `plural` with an optional `offset:`, exact `=n` cases, the CLDR categories and `#`; `selectordinal`; and `select`. Each needs an `other` case.
  - Cases nest at most 16 deep. Any other form (date, time, choice, a number style) is refused, naming where.
  - The content build parses every text with it, and the client once, when the table loads.
* **Formatting** (`NeuronClient/MessageFormat`, `FormatPattern`):
  - An argument's value is a whole number, a number or a text, and a text is inserted literally (DEC-102).
  - A plural takes its exact case, then its category's, then `other`.
  - An argument not given is written as `{name}`, as ICU writes it.
* **The language's forms** (`NeuronClient/MessageLocale`):
  - The in-box ICU's decimal format for whole numbers and for numbers of at most three fractional digits, and its cardinal and ordinal plural rules, opened once from the BCP 47 tag.
  - `icu.dll` is linked from the Windows SDK's `icu.lib` by NeuronClient alone, so the headless server, which links no NeuronClient code (ADR-090), never loads it.
  - If ICU cannot open the language, which is logged, numbers are plain digits and every plural is `other`.
* **The table** (`text.string_table`, `GameData/Strings/en_gb.json`):
  - The cooker is its validator (`Tools/SpeciesContentBuild/StringTableCooker`), as the font packer is for what the glyph table's schema cannot state. There is no schema document, because the generator reads fixed fields only.
  - The cooker checks the four fields (`schema`, `version`, `language`, `strings`), the language against the file name, each id's form and length, and each entry's fields (DAT-206 to DAT-209, UI-225).
  - It expands the markers in each input mode (DAT-211 to DAT-213): `[[string:]]`, `[[open]]` and the mode passages. Every text is checked as a message (ADR-063). Each character of a text and of its upper-cased form must be drawn by GameFont, the face of every en-GB font role (UI-199 as O63 amends it, UI-233).
  - Every fault is reported with the file and the id, and the cooked entry is read back (AST-091).
* **The cooked format** (`NeuronCore/StringTableAsset`, version 1): the language tag, then each string sorted by id, with its id, its embed-only flag and its text in each input mode.
* **The look-up** (`NeuronClient/StringTable`):
  - `Lookup(id, mode)` and `Exists(id)` (UI-218 to UI-221), and `Format(id, arguments, locale, mode)` (UI-227). An embed-only string is never returned.
  - A miss is logged once per id. A Debug build returns the id itself, and a Release build the empty text, so no player sees a key (ADR-063).
  - The input mode is INP-150's `InputMode`, always keyboard and mouse at the MVP (MVP-100).
* **What waits:**
  - **Binding arguments.** `[[binding:]]` is refused until T15's settings store rebinds and names keys (INP-178).
  - **`#` in an embedded string.** A `[[string:]]` whose text holds `#` is refused, since no quoting serves both a plural's case and plain text.
  - **More languages.** The comparison with the base language (DAT-210) and the language choice (ADR-063's start-up order) arrive with a second language and the settings. The client loads `ref.meta.lang.base`, en-GB, the MVP's one language (MVP scope §5.1).
  - **Declared arguments.** ADR-063's check of an argument against its key's declaration waits for a declaration to exist.
  - **The currency signs.** AST-088's check of `ast.font.currency_block` waits for the DirectWrite upgrade (UI-199 amendment).
* **The table's first string.** The owner writes the strings (the M2 plan's §10). This step commits a table holding only `ui.title.game_name`, the product's name (UI-228), so that the cooker, the coverage check and the look-up run on the real file.
* **The test page** of `--ui-test` shows three lines:
  - `ui.title.game_name` looked up;
  - an id no table holds, which a Debug build shows as the id;
  - a plural formatted through ICU at 1, 2 and 1,500 ("1 Worker, 2 Workers, 1,500 Workers").

  CI's settlement-world render check therefore runs ICU on Windows.

### Consequences

* Good, because a string's syntax, characters and markers are refused at build time with its id, and the client formats with CLDR's rules through the operating system's ICU.
* Good, because a HUD string the owner has not written shows where it belongs in a Debug build.
* Bad, because Species carries a MessageFormat parser of its own, a subset that a new string may outgrow. A form it refuses is a build error, never a wrong text.
* Bad, because the client depends on `icu.dll`, which ADR-050's floor guarantees but whose CLDR version follows Windows servicing. The tests use en-GB's stable forms only.
* It forecloses: `umsg_*`; hand-written plural rules.

### Confirmation

* `NeuronCoreTests`:
  - `MessagePatternTests`: the subset's forms and refusals, the quoting, the nesting bound and the argument names.
  - `StringTableAssetTests`: a table reads back as written, and a damaged one is refused.
* `NeuronClientTests`:
  - `MessageFormatTests`, on the in-box ICU: exact cases, categories, offsets, `#` in en-GB's grouping, number styles, select, ordinals, and literal text arguments.
  - `StringTableTests`: look-ups by mode, a miss as the id in a Debug build, embed-only strings, and formatting.
* `GameClientTests`, `UiLookTests`: the test page still draws every UI pipeline inside its client area.
* `Build\CheckContentBuild.py` refuses eight new fixtures:
  - a character GameFont does not draw, and one whose upper-cased form it does not draw;
  - a text that is no message, and a numbered argument;
  - an unknown marker, and a cycle of embedded strings;
  - a malformed id, and a language other than the file name's.
* The owner's run of `--ui-test` on `Debug|ARM64`, in T14's checklist.

## Pros and Cons of the Options

### 1 — Species' own subset with ICU's numbers and plurals

* Good, because named arguments and every check work the same in the content build and the client.
* Bad, because the subset is code of Species' own to keep.

### 2 — `umsg_*`

* Good, because ICU parses and formats the whole syntax.
* Bad, because its arguments are positional, so the names are lost at cook time.

### 3 — hand-written rules

* Good, because nothing outside the tree is needed.
* Bad, because CLDR's rules would be copied by hand, against ADR-063.

### 4 — the id in a Debug build

* Good, because a missing string is seen where it belongs.
* Bad, because a Debug build shows what UI-219 hides, which UI-219 now states.

### 5 — the empty text

* Good, because it is UI-219 as written.
* Bad, because missing strings and empty ones look the same.

## More Information

* Related: [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), [ADR-050](ADR-050-client-platform-and-minimum-os.md), [ADR-059](ADR-059-content-build-and-validators.md), [ADR-063](ADR-063-localisation.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md).
* The rules: DAT-206 to DAT-218; UI-199, UI-218 to UI-233; INP-150; AST-088, AST-091; MVP-100.
* Owner decisions O62, O63.
* The code:
  - `NeuronCore/MessagePattern`, `StringTableAsset`;
  - `NeuronClient/MessageLocale`, `MessageFormat`, `StringTable`, `InputRouter.h` (`InputMode`);
  - `Tools/SpeciesContentBuild/StringTableCooker`, `ContentBuild`;
  - `GameClient/UiLook`, `WorldPresentation`;
  - `GameData/Strings/en_gb.json`.
* ADR-063 is amended with this record.
