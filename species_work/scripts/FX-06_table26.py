"""FX-06: rewrite table 2.6-A of reference/08 rules_and_conventions.md (RB-219, RB-221, O23).

Usage: python3 -I FX-06_table26.py <path to rules_and_conventions.md>
"""
import sys

path = sys.argv[1]
text = open(path, encoding='utf-8').read()
start = text.index('| Field | Identifier | Value | When absent | Meaning | On the reference maps |')
end = text.index('\n\n**R-MAP-061**', start)
new = """| Field | Identifier | Value | When absent | Meaning | On the reference maps |
|---|---|---|---|---|---|
| display name | `MatchOptions::displayName` | for a custom map, a text of at most `ref.map.header.name_max_chars` (511) characters; on a map of a content set, a working title for tools only | required for a custom map | the name that match setup shows for a custom map (R-MAP-020) | — |
| display-name string id | `MatchOptions::nameId` | derived: the string id `map.<map id>.name`, which must exist in the string tables | — | the map's name in match setup for a map of a content set (R-MAP-020) | — |
| modes | `MatchOptions::modes` | a list of modes (`MatchMode`) | empty: the map is listed under no mode (R-MAP-014) | the mode lists that show the map | every reference map lists exactly one mode |
| population cap | `MatchOptions::populationCap` | a number of bodies, 0 or unlimited | required: the map states unlimited explicitly when it has no cap | the per-team cap is the cap ÷ the number of open slots (R-MAP-062) | 1,400 to 2,000; unlimited on two 4-player co-op maps |
| defender population cap | `MatchOptions::defenderPopulationCap` | a number of bodies or 0 | Siege uses the population cap ÷ open slots for both sides | Siege: each defending team's cap; the attacking teams share the rest (R-MAP-062) | 200 to 350 on 6 of the 8 Siege maps |
| player count | `MatchOptions::playerCount` | `ref.map.players.min` (2) to `ref.sim.team.player_slots` (4) | required | the number of start positions and of lobby slots | 2-player 22 maps, 3-player 15, 4-player 12 |
| play time | `MatchOptions::playTime` | whole minutes | the mode's default time limit | the time limit that choosing the map sets (R-MAP-021) | 5, 10, 15 or 20 on 20 maps (§2.22) |
| difficulty | `MatchOptions::difficulty` | basic, intermediate or advanced | rank 0 | the descriptive rating that orders the lists (R-MAP-016, R-MAP-023) | every reference map sets one |
| map effects | `MatchOptions::mapEffects` | a set of map effects (`MapEffect`, table 2.7-B) | none | weather and lighting effects (§2.10) | snow 6 maps, special lighting 3, lightning 2, dust wind 1 |
| map options | `MatchOptions::mapOptions` | one value per map option (`MapOption`, table 2.7-A) | each option takes its own default (table 2.7-A) | per-map rule overrides (§2.7) | §2.22 |
| excluded outcomes | `MatchOptions::excludedOutcomes` | a set of Supply crate outcome identifiers (`CrateOutcome`) | none | outcomes that never drop on the map ([reference/04](../04_supply_crates_powers_and_weapons/README.md) R-CRATE-029, R-CRATE-030) | 7 maps (§2.23) |
| attacking positions | `MatchOptions::attackingPositions` | a set of start positions 0 to (player count − 1), held as the Siege side of each start-position record (R-MAP-042) | no attacking position | Siege: the attacking positions; every other position below the player count defends | every Siege map (§2.19) |
| co-op allowed | `MatchOptions::coopAllowed` | flag | off | match setup offers co-op on this map | 6 maps, all 4-player |
| co-op forced | `MatchOptions::coopForced` | flag | off | match setup sets co-op on this map | 4 Siege maps |
| group-1 positions, group-2 positions | `MatchOptions::groupOnePositions`, `MatchOptions::groupTwoPositions` | lists of start positions, held as the co-op group of each start-position record (R-MAP-042): each group holds at most 2 positions, in ascending position order, and a position belongs to at most one group ([data schemas](../../architecture/components/data_schemas/map_schema.md) DAT-087, DAT-088; DEC-969) | no fixed groups | fixed co-op alliances ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-020) | 6 maps; every group lists its positions in ascending order and holds at most 2 |
| official | `MatchOptions::official` | flag | off | the map belongs to the map index (R-MAP-015, DEC-563) | every reference map |
| author | `MatchOptions::author` | a text of at most `ref.map.header.author_max_chars` (63) characters | none | the author's name; never displayed | never set |
| description | `MatchOptions::description` | for a custom map, a text of at most `ref.map.header.description_max_chars` (511) characters | none | the description that match setup shows for a custom map (R-MAP-020) | — |
| description string id | `MatchOptions::descriptionId` | derived: the string id `map.<map id>.description` | — | the description in match setup for a map of a content set (R-MAP-020) | — |"""
text = text[:start] + new + text[end:]
open(path, 'w', encoding='utf-8', newline='\n').write(text)
print('table 2.6-A rewritten')
