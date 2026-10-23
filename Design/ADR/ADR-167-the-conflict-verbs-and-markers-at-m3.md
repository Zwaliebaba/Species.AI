---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T12)
---

# The conflict verbs and markers at M3

## Context and Problem Statement

M3 task T12 gives the steward the conflict verbs of MVP-101 and marks the valley's conflicts on the settlement view:
- forming an expedition of up to three Leaders, declaring an attack with its cost and whether it is unprovoked shown before it is confirmed, withdrawing it, making and withdrawing a claim, and sending tribute (MVP-041, MVP-042, MVP-053, MVP-054);
- the edge compass for attacks and claims, and their world markers;
- the expedition's Leaders marked over the fog and at the screen's edge while it is outside a battle (O98);
- the battlefield joining the steward's areas, the camera's bound and the fog's mask for a battle's duration (WLD-244).

Its done-when is the owner's run: they raid an AI settlement, follow the expedition to it, claim an area and ease a grievance.

These decisions are open:
- where the steward presses a conflict verb;
- how a declaration shows its cost and whether it is unprovoked before it is confirmed;
- which Leaders lead the expedition, since the battle verbs select one Leader at a time (ADR-124);
- how much tribute is sent, since the recipient's temperament sets the rate (MVP-057) and the client is never told it;
- how the steward commands its side of a battle (MVP-049) when that side is its expedition, a team of its own (ADR-161);
- how the attacks' and claims' markers are drawn, since their icons are HUD art the owner has not drawn yet and the UI's quads do not turn;
- how the battlefields reach the steward's areas.

## Decision Drivers

* Every refusal is the server's (ADR-136, ADR-153): the client may foresee one, but it sends the command and shows the reason that comes back.
* The valley status (ADR-165) is the client's only source of the claims, grievances, expeditions and battles, and it is up to a second old.
* The HUD's layout and palette are provisional until the owner judges them (O95, DEC-1005).
* GameClient includes no GameLogic header (MVP-090).
* `icon.hud.attack` and `icon.hud.claim` are not in the content yet (ADR-166), and a scene texture that is missing stops every scene texture from loading.

## Considered Options

For the verbs:
1. **Buttons at the end of the relations panel's rows** ([ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md)), each row offering the actions on what it names.
2. **Verbs in the world**: a press on a settlement or a border half, with a menu.

For the declaration:
3. **Two presses on the same button**: the first shows the attack, its cost and whether it is unprovoked; the second declares it.
4. **A confirmation window** of the UI core ([ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)).

For the expedition's Leaders:
5. **The selected Leader first, then the settlement's other Leaders in id order**, up to three.
6. **A multi-selection of Leaders**, a new battle verb.

For the tribute:
7. **The stores offered**, of which the server takes what eases the grievance whole at its holder's rate (ADR-160).
8. **The cost at the standard rate**, `mvp.relation.tribute_ru_per_weight` a unit of weight.

For the steward's side in a battle away:
9. **The battle verbs follow the Leader pressed**: a press on a Leader of the expedition commands its team, and a press on one of the settlement's Leaders the settlement's again.
10. **Both teams at once**, a selection across two teams.

For the markers:
11. **The disc and compass in pass 16 and the icon in the HUD**: the world markers draw the attack's or claim's team disc with the compass ring on screen or the compass at the edge, and the HUD draws the icon over the disc.
12. **All in the HUD**, the icon on a backing with no compass.

## Decision Outcome

Chosen options: 1, 3, 5, 7, 9 and 11.
- Option 2 needs a menu in the world for five verbs whose targets the panel already lists by name.
- Option 4 adds a window for one question, where the panel's own button can ask it.
- Option 6 changes M1's battle verbs (ADR-124, ADR-127), which select one Leader by design.
- Option 8 eases only half a grievance held by a Territorial Cladan, whose rate is half (MVP-057), and the steward never learns why.
- Option 10 needs a selection across teams, which the server keeps per team (R-SIM-405).
- Option 12 has no compass, which the asset table gives the attacks' and claims' markers (`marker.edge_compass`), and turns nothing.

* **The rows' actions** (`RelationActionsOf`), at most two a row, the first nearest its end:
  - a Cladan's raid on its settlement, dimmed while it is under a respite, which would refuse it;
  - a grievance's tribute, for each grievance held against the steward's settlement;
  - its expedition's withdrawal;
  - its own claim's withdrawal and, for an open or contested claim, its battle;
  - a claimable area's claim. The claimable rows follow the claims: each half of the settlement's valley pairs that its neighbour holds, and the central site while the settlement does not hold it, that the settlement does not already claim and whose cooldown has passed (O102), as `CheckClaim` decides on the server; `ui.hud.claimable` names it.

  Each button is a zone of the HUD (`HudZone::RelationAction`, index = the row's place × 2 + the slot, for the first 128 rows), found before the panel under it.
* **Declaring** (`AttackOf`, `SettlementVerbs::PressRelation`). The first press on a raid or a claim battle makes the attack pending:
  - its target: the raided settlement, or for a claim battle the area's holder, or for a contested claim the rival claimant (ADR-161);
  - whether it is unprovoked: its Cladan holds no grievance against the target and claims nothing the target holds (PIL-063);
  - its cost: `mvp.conflict.attack_cost_ru`, × `pil.conflict.unprovoked_cost_factor` when unprovoked, as the server charges it.

  The HUD shows the attack, the Leaders and the cost on its declare line (`ui.hud.declare`), with its button in the accent strip. A second press on the same button sends `DeclareAttackCommand`; any other press over the HUD, or the secondary press anywhere, drops it, and the secondary press then reaches no battle verb.
* **The expedition's Leaders** (`ExpeditionLeadersOf`): the settlement's living Leaders, the selected one first, then the others in id order, at most `EXPEDITION_LEADERS_MAX`. The Workers that follow them go with them (ADR-161).
* **The tribute** offers the stores whole. The server takes only what eases the grievance whole at its holder's rate and refuses an empty offer.
* **The refusals.** A conflict command's refusal is kept with its kind and reason for `mvp.hud.notice_seconds`, as a placement's is (ADR-153), and shown under the panel (`ui.hud.conflict_refused`).
* **The steward's side** (`BattleTeamOf`; MVP-049). The battle verbs command the settlement's team until a primary press on a Leader of its expedition, `ExpeditionTeamOf(site)`, and then the expedition's until a press on one of the settlement's Leaders or the expedition's end. A change drops the verbs' state. The hover finds the expedition's Leaders too (`LeaderUnderRay`), and the crowd's selection is drawn for the team the verbs command.
* **The expedition's Leaders' markers** (O98). The world markers also make a marker for each of the expedition's Leaders. While one is away, outside the steward's areas and so under the fog with no figure drawn, its marker stands at it on screen with no compass, and stays at the screen's edge off screen however long it has been seen (R-CTRL-384's rule 5 waived). Inside the areas it is M1's Leader marker.
* **The attacks' and claims' markers** (`ConflictTargetsOf`):
  - each battle the steward's settlement is a side of, at its objective, in the other side's colour;
  - each attack declared on it and not yet fought, at its expedition's figures that the replica holds, which the seen layer decides (ADR-158);
  - its own expedition not yet fighting, at its target: the claimed area's centre, or the target's Wellspring, which is public;
  - each claim it made, or made on an area it holds, at the area's centre (`ClaimAreaCenter`), in the claimant's colour.

  Pass 16 draws each as the team disc's shadow, the compass's shadow, the empty compass ring on screen or the compass turned towards it at the edge, and the disc (RND-357), under the Leaders' markers, always. The HUD draws the icon, `icon.hud.attack` or `icon.hud.claim`, over the disc where the markers say (`WorldMarkers::IconPlaces`), under its panels. Until the owner draws the icons, the disc and the compass show alone.
* **The battlefields** (`StewardAreas::Field`; WLD-244). The settlement view reads the battlefield of each battle that the valley status lists with the steward's settlement as a side. It passes them to the areas, which are rebuilt when the fields change: a cell whose centre lies within a field's radius of its centre is in the areas. The camera's bound and the fog's mask follow the areas' revision (ADR-142, ADR-144).
* **Shared values.** `ExpeditionTeamOf` and `EXPEDITION_TEAM_FIRST` move from GameLogic's `Battles.h` to `GameShared/RelationMessages.h`, unchanged. `ClaimAreaCenter` joins `GameShared/ResourceSites`, placing an area's centre where `PlaceResourceSites` puts its resource site. `TeamColor` gives an expedition's team its settlement's colour.

### Consequences

* Good, because every verb sits on the row that names its target, and an attack is declared only after its cost and provocation were shown.
* Good, because the steward commands its expedition in a battle with M1's verbs and follows it over the fog by its Leaders' markers.
* Good, because the server alone refuses, and the steward reads its reason.
* Bad, because the steward commands one team at a time: while it commands its expedition, the settlement's Leaders keep their last orders.
* Bad, because a tribute to a Territorial Cladan costs twice what the panel's weight suggests, which the steward learns only from the stores.
* Bad, because a battlefield joins the areas up to a second after its battle starts, at the valley status's pace.
* It forecloses: conflict verbs in the world; a confirmation window for an attack; a selection of Leaders across teams.

### Confirmation

* `GameClientTests`:
  - `SettlementVerbsTests`:
    - a claimable area's, a claim's, a grievance's and the expedition's buttons send their commands, the tribute offering the stores;
    - an attack is pending after one press with its target, cost and provocation, is declared by the second with its Leaders, and is dropped by another press over the HUD or the secondary press;
    - a claim battle is fought against the area's holder with its claim;
    - a conflict refusal is kept with its kind and reason for its time;
    - the expedition's Leaders are the selected one and then the others, three at most;
  - `SettlementHudTests`: each row's actions are zones at its end, found before the panel; the relations rows carry their grievance's subject and weight and their claim's id, and list the claimable central site;
  - `StewardAreasTests`: a battlefield joins the areas while given, and leaves them when its battle ends;
  - `WorldMarkersTests`: an away expedition Leader's marker over the fog and at the edge; an attack's marker on screen and a claim's at the edge, with their icons' places;
  - `SettlementCameraTests`: the hover finds the expedition's Leader; a press on an expedition Leader commands its team until the expedition ends;
  - `DevelopmentOverlayTests`: an expedition's team takes its settlement's colour.
* `GameSharedTests`: each area's centre is its resource site.
* The owner's run on a Debug build, in a world with the AI settlements (the M3 plan's T12): they raid an AI settlement, its cost and provocation shown before the second press; follow the expedition by its Leaders' markers to the target; command it in the battle, the battlefield clear of fog; claim an area and see its marker; and ease a grievance by tribute, its weight falling on the panel.

## Pros and Cons of the Options

### 1 — buttons on the panel's rows

* Good, because each row names the target of its verbs.
* Bad, because a verb needs the panel open, which it always is (ADR-166).

### 2 — verbs in the world

* Good, because the target is where the steward looks.
* Bad, because a hidden target under the fog has no place to press.

### 3 — two presses

* Good, because the cost is shown where the steward presses.
* Bad, because a press on another button forgets it.

### 4 — a confirmation window

* Good, because it cannot be missed.
* Bad, because it is a window for one question.

### 5 — the selected Leader first

* Good, because M1's single selection decides who leads.
* Bad, because the other two are not chosen.

### 6 — a multi-selection

* Good, because the steward chooses all three.
* Bad, because it changes M1's verbs.

### 7 — the stores offered

* Good, because the grievance is eased whole whatever the rate.
* Bad, because the cost shows only after.

### 8 — the standard rate's cost

* Good, because the cost is known before.
* Bad, because it eases a Territorial Cladan's grievance by half.

### 9 — the verbs follow the Leader pressed

* Good, because M1's verbs serve either team unchanged.
* Bad, because the other team waits on its last orders.

### 10 — both teams at once

* Good, because nothing switches.
* Bad, because the server keeps a selection per team.

### 11 — disc in pass 16, icon in the HUD

* Good, because the compass turns and the icon is HUD art.
* Bad, because one marker is drawn in two passes.

### 12 — all in the HUD

* Good, because one pass draws it.
* Bad, because it has no compass.

## More Information

* Amends:
  - [ADR-124](ADR-124-battle-verbs-at-m1.md): the verbs command the expedition's team from a press on one of its Leaders;
  - [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md): the world markers' other types, attacks, claims and the expedition's Leaders;
  - [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md) and [ADR-144](ADR-144-black-fog-outside-the-stewards-areas-at-m2.md): the battlefields join the steward's areas;
  - [ADR-153](ADR-153-the-settlement-verbs-at-m2.md): the settlement verbs' conflict commands;
  - [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md): `ExpeditionTeamOf` in GameShared;
  - [ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md): the rows' buttons and the claimable rows.
* Owner decisions O95, O98, O102, O101; DEC-1005; MVP-041, MVP-042, MVP-049, MVP-053, MVP-054, MVP-057, MVP-101; PIL-063; R-CTRL-384, R-CTRL-386; RND-357; WLD-244.
* The code:
  - `GameClient/SettlementVerbs` (`ExpeditionLeadersOf`, `AttackOf`, `PressRelation`), `SettlementHud` (`RelationActionsOf`, the buttons, the declare and refusal lines, the markers' icons), `SettlementView` (`BattleTeamOf`, `LeaderUnderRay`, the conflict view, the battlefields, the targets), `WorldMarkers` (`ConflictTargetsOf`, `LayTarget`, the away markers), `StewardAreas` (`Field`), `DevelopmentOverlay` (`TeamColor`), `WorldPresentation`;
  - `GameShared/RelationMessages` (`ExpeditionTeamOf`), `ResourceSites` (`ClaimAreaCenter`); `GameLogic/Battles.h`;
  - `GameData/Strings/en_us.json` (`ui.hud.claimable`, `ui.hud.declare`, `ui.hud.conflict_refused`);
  - `GameClientTests/SettlementVerbsTests`, `SettlementHudTests`, `StewardAreasTests`, `WorldMarkersTests`, `SettlementCameraTests`, `DevelopmentOverlayTests`; `GameSharedTests/RegionTerrainTests`.
