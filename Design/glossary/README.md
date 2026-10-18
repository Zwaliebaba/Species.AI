# Glossary

> Status: Reference · Glossary · Related: [Conventions](../CONVENTIONS.md), [Decision index](../decisions/README.md), [ADR index](../ADR/README.md)

The glossary defines the Species terms that the reference library uses, and the terms of the Species world game that [Vision, pillars and core loops](../gdd/00_vision_pillars_and_core_loops.md) and [World and regions](../gdd/01_world_and_regions/README.md) define (CONVENTIONS §8.15): player concepts, engine terms, data terms and abbreviations. It names only Species terms, and it holds no term for content that the library does not specify. Each definition is short; the linked owning document holds the full specification, with its requirements, tuning keys, states and edge cases.

## Files

| File | Covers |
|---|---|
| [a_to_l.md](a_to_l.md) | Terms that start with a digit, and terms from A to L |
| [m_to_z.md](m_to_z.md) | Terms from M to Z |

## How to read an entry

Every entry is one table row with four columns.

| Column | Content |
|---|---|
| Term | The Species term as the documents write it. Proper Species terms are capitalised (Worker, Spawn Node, Supply crate) and generic words stay lower case (tree, wall, turret), as [CONVENTIONS.md §8](../CONVENTIONS.md#8-naming) sets out. A qualifier in brackets separates two uses of one name, for example "Builder (outcome)" for the Supply crate outcome that places a Builder. A row that covers a pair of short forms lists both, separated by " / ". |
| Kind | One of the four kinds below. |
| Definition | What the term means, in one to three sentences, in the present tense and in Species terms only. |
| Owning document | A link to the document of the library that specifies the term. Where two links are given, the first is the primary one. |

| Kind | Used for |
|---|---|
| player concept | something a player sees, controls or decides about: a unit, creature, structure, mode, match rule, Supply crate outcome, achievement, screen or message |
| engine term | a type, subsystem or mechanism of the simulation or the engine |
| data term | a content or data format, a part of a map definition, or a naming pattern |
| abbreviation | an acronym or short form |

## Reading conventions

- **Order.** Entries are sorted alphabetically by term, word by word, ignoring case and any qualifier in brackets and reading a hyphen as a space. Terms that start with a digit come first, under "Numbers" in [a_to_l.md](a_to_l.md).
- **Units.** Distances are in world units (`wu`) and simulation durations in ticks; the tick rate is defined in [Tick timing and main loop](../reference/12_simulation_model/01_tick_timing_and_main_loop.md). A match option that the player sets in seconds or in per cent is given in that unit.
- **Numbers.** A number in a definition is the reference value, repeated for readability. The tuning table of the owning document is the authority for every value; the glossary has no requirement IDs and no tuning table of its own.
- **Cross-references.** The Species terms used inside a definition are glossary terms too; look them up in either file. `DEC-NNN` refers to an entry of the [decision register](../decisions/README.md), and `ADR-NNN` to an [architecture decision record](../ADR/README.md).
- **Identifiers.** The type names, enumerators and string-id prefixes that belong to each term are listed in [CONVENTIONS.md §8](../CONVENTIONS.md#8-naming); the glossary does not repeat them.
- **Player-facing text.** The messages that the glossary lists, such as Cancelled and Not available, are Species text kept under Species string ids.
