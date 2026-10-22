---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T3, owner decisions O81, O98)
---

# The seen layer and its readers at M3

## Context and Problem Statement

M2 builds each settlement's seen layer on the server, once per step (MVP-016, MVP-097, [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)), and draws MVP-084's fog from the territory. Its readers are M3's task T3 (O81):
- **The AI.** The AI of an AI settlement reads only what its settlement sees (MVP-058, PIL-084).
- **The replication.** The server sends a client only what its settlement may see (MVP-097), so a client no longer receives the whole region ([ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-139](ADR-139-replicating-the-settlement-at-m2.md)).

Five things stand in the way:
- **The node census.** The bot nodes are shared simulation state ([ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md)). Each node counts every actor within `ref.bot.node.range` of it, and its owner is the team with the most there (R-BOT-042, R-BOT-047). An AI that read a node's counts would learn of figures outside its seen layer.
- **Enemies.** The census counts as enemies every team but the counted one. Since [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md) settlements are at peace, so another settlement's Workers are no enemy.
- **The order of the step.** The AIs decide before the settlements step and their seen layers are made, so they read the previous step's layers. A region restored from its snapshot holds none, since the seen layer is derived state ([ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)). Its first step would decide on nothing, where the world it was saved from decided on its layers, and the two would part.
- **The public layer.** WLD-242 makes the sites, the settlements and their territories public: every steward sees them wherever they are. The resource sites and the Wellsprings are replicated as structures.
- **The events.** The presentation events of [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md) carry positions. A shot fired outside the seen layer would show where another team's figures are.

The M3 plan's table 1 confirms two baseline records on this task:
- **[ADR-082](ADR-082-counter-based-simulation-random-draws.md).** A test client given everything the steward's client receives cannot compute a later draw.
- **[ADR-084](ADR-084-client-boundary-through-the-transport.md) and [ADR-086](ADR-086-replication-and-interest-management.md).** The replica holds no figure, structure or store outside its settlement's seen layer.

## Decision Drivers

* No reader of the seen layer learns of a figure, structure or store that its settlement does not see (MVP-016, PIL-084).
* The sandbox and a watched world, which have no steward's settlement, keep what they show today.
* A restored world decides as the world it was saved from (MVP-076).
* The AI's cost stays within ADR-080 item 2's 60 µs (O74).

## Considered Options

1. **A sight per reader**, read at every use: an AI reads a node only where its seen layer holds the node's whole census, and a client is sent another team's objects and events only where its settlements see them.
2. **A census per team**: each node counts, for each team, only the actors its seen layer holds.
3. **The AI reads the region whole**, and only the replication is narrowed.

## Decision Outcome

Chosen option: 1.
- **Option 2** multiplies the census by the teams and makes the node state depend on the seen layers. The node state is simulation state, which the hash and the snapshot hold; the seen layers are derived.
- **Option 3** breaks MVP-058 and PIL-084: an AI settlement would see the steward's whole settlement.

* **The sight** (`GameLogic/SeenLayer`, `Sight`): the union of a reader's settlements' seen layers at the step's end.
  - **The AI of a team** (`Sight::OfTeam`) sees through the seen layer of the settlement whose team it is.
  - **A steward's client** (`Sight::OfSteward`) sees through the seen layers of the settlements its steward commands.
  - **A reader with no settlement sees every place**: the sandbox's teams, and a watched world's steward.
  - **A settlement whose seen layer is not yet made** sees nothing.
  - `SeesDisc` holds a disc only when every cell a point of it can lie in is held: the cells whose centers lie within the radius and half a cell's diagonal more.
* **The node census** (`BotNavigation`): a node's enemies of a team are the friends of the teams the region makes that team's enemies (`RegionSimulation::EnemiesOf`). A node held by an enemy is one whose owner is such a team (`HeldByEnemy`). In the sandbox every other team is an enemy, so its counts are unchanged.
* **The AI** (`AiController`) reads each node through its sight (`SeenBotNode`).
  - **A node it sees whole** reads as it is: the sight sees its census disc of `ref.bot.node.range`.
  - **Any other node** reads as one no team owns and nothing is counted at.
  - **This applies to every read:** the formation Leaders, the dispatch, a Leader's policy and `NeedsDefending`.
  - **A Leader's policy** counts only the enemies its sight sees.
  - **A Worker's census shortcut** (R-BOT-045) is a rule of the simulation, not a decision of the AI, and still reads the node as it is.
* **The order of the step.** The AIs decide after the settlements step and their seen layers are made, beside the scripted stewards. They read the layers of their own step, which a restored region makes again at its first step. The sandbox has no settlement, so its AIs read what they read before.
* **The replication** (`StateSender`):
  - **What is narrowed.** `ViewForSteward` sends another team's figures, effects and Essence orbs, and the structures no settlement of the steward owns, only where the steward's sight sees them.
  - **The public layer stays.** The Wellsprings and the resource sites are sent wherever they are. Where the steward does not see them, they show no manned port. The settlements, their colors and radii and their territories' chunks are public too.
  - **What was already its commander's alone stays so:** a settlement's stores, population and seen layer, and a team's order marks and selection ([ADR-139](ADR-139-replicating-the-settlement-at-m2.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md)).
  - **Events.** `EventsForSteward` sends a team's events to its commander wherever they are. Another team's droppable events go only where the steward's sight holds the event's position, and never its selections and orders.
  - **Objects at the edge.** An object that leaves the seen layer is removed from the client's replica, and one that enters is sent whole, as [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md) handles any object that leaves or joins.
* **No state version raise and no wire change.** The census's counts change only where settlements stand together, and neither the hash's nor the snapshot's format changes.

### Consequences

* Good, because an AI settlement and the steward's client act on the same layers, the seen layer and the public layer, and only those (MVP-016, MVP-097).
* Good, because the sandbox and a watched world keep their whole region.
* Good, because the AIs read the layers their own step made, so a restored world decides as the one it was saved from.
* Bad, because a node at the edge of the seen layer reads empty until its whole census disc is seen. An AI is slower to see a node beyond its Workers than the counts alone would allow.
* Bad, because a shot fired from outside the seen layer at the steward's Workers is not shown; only the hurt and the death of its Workers are.
* Bad, because a figure that walks along the edge of the seen layer is sent whole each time it re-enters.
* Bad, because WLD-242's last-seen marks are not built: what the settlement no longer sees disappears at once.
* It forecloses: an AI that reads the region whole; a client that receives another settlement's figures, structures or stores outside its seen layer.

### Confirmation

Confirmed when the M3 plan's table 1 rows for ADR-082, ADR-084 and ADR-086 pass. The tests:
- **GameLogicTests `StateSenderTests`:**
  - In a valley of four settlements, the steward's client holds the server's view after every tick. That view holds no other team's figure, effect or orb and no other settlement's structure outside its settlement's seen layer, nor another settlement's stores. The public sites show no manned port outside it, and the other settlements' Workers at home are not sent.
  - Another team's events go only where the steward sees, and never its selections.
  - No message the steward's client is sent holds either half of the simulation key, at any bit offset and in either bit order, over every message kind of a settlement world. The search finds a half that is sent.
- **GameLogicTests `ValleyTests`:**
  - An AI settlement reads a node only where its seen layer holds every figure the node counts, and reads an empty node elsewhere. A wild band's node is unseen by every AI settlement.
  - A node counts the enemies of the region's rule: another settlement's Workers are no enemy, and a wild band's are.
  - An AI commanding an AI settlement gives the same hash on every tick after its world is restored. The snapshot is taken on the tick before the AI's main cycle, which promotes a formation Leader. The same test fails with the AIs deciding before the seen layers are made.

The whole GameLogic suite passes unchanged.

## Pros and Cons of the Options

### 1 — a sight per reader

* Good, because the node state, the hash and the snapshot are unchanged.
* Bad, because every node read pays for a disc test of the seen layer, about 40 cells.

### 2 — a census per team

* Good, because a node's counts would be exact for each team.
* Bad, because the census grows with the teams, and simulation state would depend on derived state.

### 3 — the AI reads the region whole

* Good, because it costs nothing.
* Bad, because it breaks MVP-058 and PIL-084.

## More Information

* Amends:
  - [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md): the census's enemies, the AI's sight and the order of the step;
  - [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md): the replication narrowed and the events filtered;
  - [ADR-139](ADR-139-replicating-the-settlement-at-m2.md): the structures narrowed and the public sites.
* Related: [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-086](ADR-086-replication-and-interest-management.md), [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md), [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md).
* Owner decisions O81, O98; MVP-016, MVP-058, MVP-076, MVP-097; PIL-084; WLD-242; R-BOT-042, R-BOT-045, R-BOT-047, R-BOT-051, R-BOT-091.
* The code:
  - `GameLogic/SeenLayer` (`Sight`);
  - `BotNavigation` (`SeenBotNode`, `HeldByEnemy`, `NeedsDefending`, the census);
  - `AiController`, `RegionSimulation` (the step's order);
  - `StateSender` (`ViewForSteward`, `EventsForSteward`), `WorldServer`;
  - `GameLogicTests/ReplicationTests`, `GameLogicTests/ValleyTests`.
