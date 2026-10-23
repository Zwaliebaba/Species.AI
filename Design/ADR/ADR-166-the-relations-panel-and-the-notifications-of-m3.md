---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T11)
---

# The relations panel and the notifications of M3

## Context and Problem Statement

M3 task T11 shows on M2's HUD ([ADR-152](ADR-152-the-settlement-hud-at-m2.md)) what T10's valley status and notices carry ([ADR-165](ADR-165-the-valleys-status-notices-and-battle-results-at-m3.md)), for MVP-102's M3 part:
- the relations panel: each Cladan's anger toward the steward's settlement, every grievance with its cause, weight and fade (MVP-050, MVP-052), and every claim (MVP-053);
- the border areas' holders and claims marked on the map ([ADR-143](ADR-143-territory-lines-and-border-areas-drawn-at-m2.md));
- the notifications of contact, claims, the warning with its countdown, battles starting and ending, and the respite (MVP-047);
- the M3 icons of the MVP assets.

Its done-when is the owner's run: they write an attack's cause down before opening the panel, and check it.

These decisions are open:
- where the panel sits and what it lists;
- how a Cladan is named, since the MVP gives a Cladan a colour and no name (MVP-013);
- how the warning's countdown is shown, since a notification card lasts `mvp.hud.notice_seconds`;
- how a border half shows its holder and a claim on it.

## Decision Drivers

* The HUD's layout and palette are provisional until the owner judges them (O95, DEC-1005), so the panel keeps to the HUD's measures and adds as few keys as it can.
* A Cladan's colour is its site's index (ADR-130), and every Cladan has one of the four.
* The warning's countdown runs for `mvp.conflict.warning_minutes` (5 minutes), far longer than a card.
* The client holds the valley status at most a second old, and the replica's tick every frame.

## Considered Options

For the panel:
1. **A panel at the bottom right**, with the HUD's row height and width, its rows from the valley status: the warnings, the steward's expedition and respite, each other Cladan with its grievances, then the claims; at most `mvp.hud.relations_rows_max` rows.
2. **A window of the UI core** that the steward opens and closes ([ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)).

For a Cladan's name:
3. **Its colour's name**: Green, Red, Yellow or Blue, the colours of sites 0 to 3.
4. **A generated name** per Cladan, kept in the state.

For the warning:
5. **A row of the panel** that counts down from the valley status and the replica's tick, beside the card that arrives with the notice.
6. **A card that stays** until the warning ends.

For the border halves:
7. **The half's edge spots in its holder's colour**, or its claimant's while a claim on it stands; the halfway line stays neutral.
8. **A world marker** over each claimed half (T12's markers).

## Decision Outcome

Chosen options: 1, 3, 5 and 7.
- Option 2 hides the anger the run turns on behind a press, and MVP-102 lists the panel among what the HUD shows.
- Option 4 adds state and strings for a name the MVP's design never gave a Cladan.
- Option 6 breaks the cards' one rule of a fixed life and order.
- Option 8 is T12's for an attack's or a claim's marker; the spots mark every half at once.

* **The rows** (`RelationRowsOf`), each a row of the HUD:
  - the heading;
  - each attack declared on the steward's settlement while its warning holds, counting down to the warning's end in minutes and seconds, in the negative colour;
  - the steward's own expedition and where it stands (held, marching, fighting);
  - the steward's respite, in whole minutes left;
  - each other Cladan, in site order, with a swatch of its colour, its anger toward the steward's settlement (the sum of its grievances against it, in whole units) and, at the row's end, its respite;
  - under each, its grievances against the steward's settlement: the cause's icon, the cause, the weight in whole units, and whether it fades fast, its cause removed;
  - the claims: each claimant and area, and whether it is open, waiting or contested.
  A longer list keeps its first rows and ends with one saying how many it leaves out. The grievances the steward's own Cladan holds are not listed: the steward eases only those held against it (MVP-054).
* **Names.** A Cladan is named by its colour (`ui.hud.cladan`). A half of a border area is named by the settlement whose half it is and the neighbour it faces (`ui.hud.area`): "Green's half facing Red"; the central site is "the central site".
* **The requests.** The HUD asks for the valley status with the settlement's, every `mvp.hud.status_period`, and keeps the latest for its settlement's team.
* **The cards.** Every kind of notice has a card with its icon and a sentence of `ui.hud.notice`, in the positive colour when the news is good for the steward: a claim or attack of its own, a won claim or battle, an attack on it ended, and the respite; else in the negative colour. A battle's end reads as won or lost from the notice's detail and whether the steward's settlement attacked.
* **The border halves** (`TerritoryLook::SetValley`, `SpotColor`). Each spot on a half's edge knows its area (VALLEY_PAIRS index × 2 + side), and takes its claimant's colour while a claim on it stands, else its holder's, else the neutral mark's. The halfway line's spots stay neutral.
* **The icons.** `UiImage` adds the nine relations and run icons and the result card of the MVP assets (`icon.hud.contact` to `icon.hud.run_clock`, `icon.hud.result_card`). Until the owner draws them, the content holds none, and the HUD draws without them, as it does M2's icons.
* **The key.** `mvp.hud.relations_rows_max` (14), provisional with the HUD's layout (DEC-1005).

### Consequences

* Good, because the steward sees the anger that turns into attacks, and its causes, without a press.
* Good, because the warning counts down to the second while the card is long gone.
* Good, because a held or claimed half shows on the map in the colours of the territory lines.
* Bad, because a Cladan's name is only its colour, which a steward with colour vision deficiency reads from the text alone; the swatch adds nothing for them.
* Bad, because the panel's rows grow with the grievances, and a long list loses its last rows to the bound.
* It forecloses: Cladans' names in the state; the relations panel as a window.

### Confirmation

* `GameClientTests`:
  - the relation rows of a valley status: the warning's countdown, the expedition, the respite, each Cladan's anger as the sum of its grievances with them under it, a removed cause marked, the steward's own grievances left out, and the claims; and the bound's last row;
  - the panel is a zone at the bottom right, a row high per row, absent before the first status, and draws its icons; the cards of M3's kinds have their icons;
  - the committed en-US strings give the sentences: a Cladan, an area, a claim notice, a battle won, the warning, a grievance, an anger, a claim;
  - each spot on a half's edge names its area, and takes its holder's colour, its claimant's while a claim stands, and the neutral mark's with no status.
* The owner's run on a Debug build, in a world with the AI settlements (the M3 plan's T11): the panel lists each neighbour's anger and grievances; when an AI settlement declares an attack, a card comes and the panel counts the warning down; the owner writes the attack's cause before opening the panel, and it matches the grievance listed; a claimed half's dots turn to the claimant's colour.

## Pros and Cons of the Options

### 1 — a panel at the bottom right

* Good, because it is always there, on the HUD's own measures.
* Bad, because it covers a corner of the world.

### 2 — a window

* Good, because it covers nothing until opened.
* Bad, because the anger and the warning would hide behind a press.

### 3 — the colour's name

* Good, because it matches the swatch, the territory line and the figures.
* Bad, because it is no name.

### 4 — a generated name

* Good, because it reads as a people.
* Bad, because it needs state, strings and a design the MVP does not have.

### 5 — a row that counts down

* Good, because it lasts as long as the warning.
* Bad, because it is up to a second late at its start.

### 6 — a card that stays

* Good, because it is where the news arrived.
* Bad, because the cards would no longer age and fall in order.

### 7 — the spots in colour

* Good, because every half shows at once.
* Bad, because a claim and a holding share a colour: the claimant's covers the holder's while the claim stands.

### 8 — a marker per claimed half

* Good, because it is an icon.
* Bad, because it is T12's and marks claims alone.

## More Information

* Amends:
  - [ADR-143](ADR-143-territory-lines-and-border-areas-drawn-at-m2.md): the border halves' spots take their holders' and claimants' colours;
  - [ADR-152](ADR-152-the-settlement-hud-at-m2.md): the valley's status, the relations panel and M3's cards.
* Owner decisions O95, O98, O101; DEC-1002, DEC-1005; MVP-013, MVP-042, MVP-047, MVP-050, MVP-052 to MVP-054, MVP-084, MVP-102.
* The code:
  - `GameClient/SettlementHud` (`RelationRowsOf`, the panel, the cards), `TerritoryLook` (`SetValley`, `SpotColor`), `UiLook` (the M3 images), `WorldPresentation`;
  - `GameShared/TuningPresentation` (`mvp.hud.relations_rows_max`); `GameData/Strings/en_us.json` (`ui.hud.cladan` to `ui.hud.notice`);
  - `GameClientTests/SettlementHudTests`, `TerritoryLookTests`, `TestSupport` (`CommittedStrings`).
* **Amendment 2026-10-23 (M3 task T12, [ADR-167](ADR-167-the-conflict-verbs-and-markers-at-m3.md)).** Each row ends in the buttons of its actions, and the claimable areas follow the claims as rows of their own; a grievance's row carries its subject and weight, and a claim's its id, for their commands. The attacks' and claims' world markers are T12's (option 8's).
