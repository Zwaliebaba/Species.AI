# A0 challenge 1: vision, pillars and core loops

> Challenger: A0-C · Round 1 · Date: 2026-10-18 · Documents attacked: `Design/gdd/00_vision_pillars_and_core_loops.md` (Draft) and `Design/gdd/adoption_ledger/00_reference_overview_ledger.md` (Draft) · Read: vision brief (all), owner decisions O1–O30, prompt §1–§6, reference/00, CONVENTIONS §3 to §9, the glossary · Not read: the proposer's notes and report · Numbers: `species_design_work/scripts/A0C_SessionPacing.py` (run with `python3 -I`; every input is a value of the gdd/00 tuning table or is marked as an assumption)

## Weakest point

**Every target of the document is written in world time, and world time only passes while a server runs, but the document never works out what that means in the stage that ships first.** In stage 1 the server is integrated in the host's client (O10) and the world freezes when the client closes (O4, PIL-077), so for the solo steward and for the host of a friends' world, world time is roughly the time they play. The document nevertheless promises the solo audience and the bounded-time audience of stage 1 "a deputy they can trust between sessions" (Table B), sizes the world loop at 2 to 10 battles per week of world time (PIL-065), puts the settlement plateau at 14 days of world time (PIL-055) and makes 8-hour, 24-hour and 7-day absences the design cases of pillar 2 (PIL-022, PIL-074). Run through the session patterns of Table C, a solo steward in an integrated world meets one battle every 16.8 to 84 hours of play (8 to 168 sessions), reaches the plateau after 336 hours of play, and is never away while the world runs, so the deputy, the away report, the protection rules and the check-in, the heart of the pitch, do nothing at all between sessions. A friends' world whose host's client runs 2 hours a day needs 168 real days to reach the plateau and stretches an 8-hour respite over 4 real days. Pillar 2 is renamed "a world that runs while you are away" to state the promise exactly, and in stage 1 that promise is false for exactly the players the document names as stage 1's audience. Unless A0 states its targets per stage (or per world pace) and puts the integrated-stage consequence to the owner, every B area will tune its model to a 24-hour dedicated world that the first two audiences never play in (A0-F01).

## Findings

Summary: 1 Critical, 5 High, 12 Medium, 5 Low.

| Id | Severity | Category | Title |
|---|---|---|---|
| A0-F01 | Critical | player experience | World-time targets do not survive stage 1 |
| A0-F02 | High | griefing or exploit | The integrated host owns world time and the save |
| A0-F03 | High | economy and balance | The away-yield guard makes active play nearly worthless |
| A0-F04 | High | griefing or exploit | Attacking the away is always better, and PIL-086 forbids the counter |
| A0-F05 | High | griefing or exploit | Collusion: Essence feeding and respite gifting |
| A0-F06 | High | player experience | The world loop has nothing to win and no way to settle a conflict |
| A0-F07 | Medium | player experience | Pillar 3 binds only the AI: steward attacks need no cause |
| A0-F08 | Medium | consistency | Hostile world events are the random aggression that pillar 3 rules out |
| A0-F09 | Medium | IP and look-and-feel line | Battles between settlements of one clade are unreadable |
| A0-F10 | Medium | griefing or exploit | "Watched" is undefined, so choosing a fidelity is a strategy |
| A0-F11 | Medium | testability | The guards mix world time and active-play time |
| A0-F12 | Medium | testability | The leader guard cannot fail in the model that runs it |
| A0-F13 | Medium | scale and cost | Abandoned settlements accumulate and keep their sites |
| A0-F14 | Medium | consistency | The ledger's restatement verdicts contradict PIL-032 and CONVENTIONS §3 |
| A0-F15 | Medium | consistency | `adopt-skirmish` rows bind six-mode content that PIL-003 says is not built |
| A0-F16 | Medium | consistency | A battle floor of 100 figures is below the vision's "hundreds" |
| A0-F17 | Medium | scope | No smallest world game per stage |
| A0-F18 | Medium | IP and look-and-feel line | The pillar 5 guard checks provenance, not distinctness |
| A0-F19 | Low | consistency | R-VIS-007 is adopted with a qualifier, which makes it an adaptation |
| A0-F20 | Low | consistency | Which attacks warn the target is stated three ways |
| A0-F21 | Low | consistency | Vocabulary overloads: frozen, attack, the aide's prefix, `Fidelity::Abstract` |
| A0-F22 | Low | IP and look-and-feel line | "Race" survives in file names, and "Species" plus "clade" needs a title screen |
| A0-F23 | Low | testability | PIL-020 and AC-05/AC-06 disagree with each other and with PIL-070 |

### A0-F01 World-time targets do not survive stage 1

- **Severity:** Critical (it makes pillar 2 inert for the stage 1 audience the document names, and the stage plan that Table B implies cannot be delivered).
- **Category:** player experience; scope.
- **Attacks:** PIL-011, PIL-022, PIL-055, PIL-065, PIL-074, PIL-077, PIL-078; Table B rows "Solo players" (stage 1) and "Strategy players with bounded time" (stages 1 and 2: "a deputy they can trust between sessions"); Table C; §1.7 ("every duration is world time").
- **Failure scenario.** A solo player starts a world on the integrated server in stage 1 and plays the "active play" pattern of Table C, 60 minutes a day. The world runs only while the client runs (PIL-077; edge case "A solo steward closes the client of an integrated world"), so world time equals play time. The script's section 5 gives what the tuning table then means: one battle every 16.8 to 84 hours of play (`pil.world.battles_per_week` 2–10 over 168 hours), the plateau of PIL-055 after 336 hours of play, week 4 of PIL-028 after 672 hours. The steward is never away while the world runs except for in-session idling, so the away report is always empty, the protection rules never act, and the deputy's only work is the idle threshold of PIL-070. For a friends' world, the other stewards can only play, check in or be attacked while the host's client runs: with the host running it 2 hours a day, the plateau takes 168 real days and an 8-hour respite (an assumed B6 value) lasts 4 real days. Every B area tunes against the 8-hour and 24-hour absences of PIL-022 and the 2–10 battles a week of PIL-065, which are cases of a 24-hour dedicated world, and stage 1 ships a world game whose minute loop is rare, whose day loop takes months and whose AFK loop does not run.
- **Recommendation.** Make the integrated stage a first-class case of A0, not an edge case:
  1. State every target of §4 for two world paces, an **always-on** pace (a dedicated server, 24 hours of world time per real day) and a **session** pace (an integrated world, where world time is roughly play time), and add a world-pace setting that the world host sets at creation, with tuning keys that scale the world-loop and settlement-loop timers (growth, travel, grievance fading, respites, the loss-cap window) per hour of world time. A battle always runs at the tick rate, so only the economy and relations timers scale. This keeps O4 intact (world time is still server uptime and no catch-up runs); it changes how much happens per hour of it. First guess for the session pace: the world-loop timers run 8 to 12 times faster (provisional; A1 and B2 model it).
  2. Put an owner question at gate A, recommended option first: **(a) recommended**: the host may keep an integrated world running after the game window closes, as the headless server build (which exists from day one, vision §5) running as a background process on the host's machine, so stage 1 has real away time whenever the host's PC is on; this amends the example in O4 ("the host has closed the client") but not its rule, and it brings a local form of the dedicated server into stage 1 beside the in-process one (O10), on the same transport interface. **(b)** keep O4 as written and say plainly that pillar 2's away loop is a stage 2 feature: stage 1 worlds use the session pace and the deputy covers only in-session idling. **(c)** drop the solo integrated world and require a dedicated server (local or community) for the world game: rejected, because it moves the world game's first playable behind stage 2.
  3. Rewrite Table B and Table C with a stage column: which pattern exists in which stage, and which guard is checked at which pace. PIL-022, PIL-023, PIL-028, PIL-029 and PIL-065 each name the pace they test.

### A0-F02 The integrated host owns world time and the save

- **Severity:** High (a likely exploit in every multi-steward stage 1 world, with no mitigation in the document).
- **Category:** griefing or exploit; security.
- **Attacks:** PIL-077, PIL-082, PIL-085 ("The world game has no pause" and "only a stopped server freezes a world"), the edge case "The world host changes a settlement through admin tools or an edited save" (deferred to D9), Table 3 "world host".
- **Failure scenario.** Four friends play in Ana's integrated world. Ana is a steward there too. (1) A hostile expedition is warned 10 minutes out from Ana's settlement (PIL-025); Ana closes the client, thinks, re-queues defences offline in her head, and restarts the world when she is ready: a pause that PIL-085 forbids to everyone except the one steward who also competes. (2) Ana plays from 20:00 to 23:00 in her time zone; Ben lives 8 hours ahead and can only be online when the world is frozen, so Ben's settlement is away during all of its world time and grows at 50–80 % (PIL-023) while Ana's grows at 100 % whenever it grows at all. (3) Ana attacks Ben while Ben sleeps; Ben's deputy defends; nothing Ben can do changes the schedule. (4) Ana copies the world save before a risky battle and restores it after a loss. The document treats the world host as a neutral administrator (Table 3, PIL-081) and sends save editing to D9 (stage 2 concerns), but in stage 1 the host is a competing steward with control of the clock and the disk.
- **Recommendation.** Add a requirement that names the conflict and bounds it in stage 1: (a) the world host of an integrated world is shown as such to every steward on the join page and in the world; (b) every freeze is recorded with its cause and its world time and appears in every steward's away report; (c) closing the client while a battle runs, or while an expedition is inside its warning time of the host's settlement, asks for confirmation and records "the world host froze the world during a battle"; (d) steward-against-steward attacks in an integrated world are a world setting that is off by default and shown on the join page (A0-M1); (e) D9 and D4 own save integrity in stage 1 as well, with a recommendation to sign or hash the save so that a restored save is visible to the other stewards. B5 and C2 own the setting and the pages.

### A0-F03 The away-yield guard makes active play nearly worthless

- **Severity:** High (it is the central economic parameter of the world game, and as specified it steers B2 into an idle game or into breaking the guard).
- **Category:** economy and balance; player experience.
- **Attacks:** PIL-023 and `pil.guard.away_yield_share` (50–80 %); Table B "progress in every session"; PIL-055.
- **Failure scenario.** PIL-023 compares a settlement run by its deputy for a whole day with "the same settlement under a competent steward who is present" for a whole day. Neither player exists: in a dedicated world, a heavy player (120 minutes a day) is present for 8.3 % of world time and a check-in player (10 minutes) for 0.7 % (script, section 1). B2 meets the band exactly, and then (section 2) the heavy player out-grows the check-in player by 7.6 % at 50 %, 4.1 % at 65 % and 1.9 % at 80 %, for twelve times the play; a settlement whose steward never comes back grows at 92 % to 98 % of the heavy player's. Active play then buys almost no growth, so "progress in every session" can only come from the clock, and the stewards who invest the most time feel no return. If B2 instead makes presence matter (a present multiplier of, say, 3), the deputy's share of a real day drops below the band and the guard fails. The guard also says "above it an absence is the better strategy, and the deputy plays better than its steward", which is false: a deputy at 90 % still plays worse than its steward; an absence is only the better strategy above 100 %. And the guard is undefined until the standing orders are fixed: with the default orders (PIL-079: finish the queue, never start new construction), the deputy's yield depends on how long a queue the steward left, so a steward who queues 24 hours of construction makes the deputy almost as good as themselves.
- **Recommendation.** Restate the guard against real session patterns and against stated standing orders: "with the default orders plus a queue of `x` hours, a steward who plays the typical pattern (two check-ins and 60 minutes of active play a day) grows at least `y` % more per day than a steward who only checks in once a day, and the check-in steward grows at least `z` % of the typical one", with first guesses of y = 25 % and z = 70 % (provisional; B2 models them). Then say where presence pays: decisions that only a present steward can take (placing construction, expansion, founding, attacks), battles fought in person, and the aide. Fix the sentence about "above the band". The band of PIL-023 becomes an input of B2's model, not the guard.

### A0-F04 Attacking the away is always better, and PIL-086 forbids the counter

- **Severity:** High (the likely attacker behaviour, which empties the defender's minute loop; the document's mitigation bounds the loss, not the experience).
- **Category:** griefing or exploit; player experience.
- **Attacks:** PIL-086 ("the protection rules that apply to the settlement of a present steward are at least those that apply while the steward is away"); Table 1 pillar 4, which rules out "a deputy that plays better than an attentive steward"; the edge case "an attacker times attacks for the hours in which a defender sleeps"; the "crowd general" fantasy (Table A).
- **Failure scenario.** In a 16-steward dedicated world, the deputy is never better than an attentive steward (Table 1), and the protection rules are the same or weaker while the defender is away (PIL-086). An attacking steward therefore always does at least as well against an away settlement as against a present one, and rational attackers attack only the away. With uniform timing a heavy player would defend 0.17 to 0.83 battles a week in person out of 2 to 10 (script, section 4); with attackers who pick their moment it is close to 0. Defence becomes something a steward reads in the away report, and the minute loop is a one-sided experience for attackers. The natural counter, a defence that is stronger while the defender is away (an away fortification bonus, or a smaller loss cap while away), is exactly what PIL-086 forbids. PIL-086 exists to stop "stay away for safety", but PIL-023 already charges an absence 20–50 % of its growth, which is the price of that safety, so the trade is a choice and not an exploit.
- **Recommendation.** Replace PIL-086 with a rule that prices the trade instead of forbidding it: "an absence may carry stronger protection than presence only while the yield lost to the absence exceeds the value of the protection gained (B2 and B6 show it)". Then let B6 choose among an away-only defence bonus, a smaller away loss cap and the defence windows of A0-M3. Add a guard: in B6's protection model, an attacker's expected net gain per attack on an away settlement is at most `pil.guard.away_target_premium` (1.2, provisional) times the gain on a present one.

### A0-F05 Collusion: Essence feeding and respite gifting

- **Severity:** High (a likely exploit on any server where two stewards cooperate or one person holds two accounts; nothing in the document bounds it).
- **Category:** griefing or exploit; economy and balance.
- **Attacks:** PIL-045 ("every death leaves its Essence orb on the battlefield for the side that holds the field to collect"); PIL-048 ("an attacker may withdraw, and its withdrawal ends the battle"); Figure 5 (a resolved battle starts a respite for the defender); the edge case "a strong settlement farms a weak neighbour" (which counts only stores).
- **Failure scenario.** (1) *Feeding.* Cara and Dev are friends, or Dev is Cara's second account on a community server. Cara's settlement sits at its population limit and its peaceful Essence source (PIL-056) refills it. Cara sends 300 Workers to "attack" Dev, and Dev's Workers shoot them; Dev holds the field and collects 300 Essence orbs, which become 300 of Dev's Workers. The attacker's losses are uncapped (PIL-048), the defender's gain is not in the loss cap (the cap bounds what a defender loses, not what it gains), and the grievance that Dev's clade records against Cara costs friends nothing. One respite later they repeat it. (2) *Respite gifting.* An enemy expedition is 10 minutes from Dev. Cara declares a token attack, enters Dev's territory and withdraws at once: the battle is resolved, Dev's respite starts (Figure 5), and the enemy's expedition arrives at a settlement it cannot attack. The attack cost is the only price. (3) *Winner's snowball.* Without collusion, PIL-045 still hands the winner of every battle both sides' Essence, a transfer that runs from the weaker side to the stronger and that B2's economy model (PIL-028) never sees.
- **Recommendation.** PIL-045 decides a mechanic that belongs to B1 and B4 and decides it in the exploitable direction. Replace it with constraints: "the net transfer of a battle (stores, Workers and Essence together) to either side is bounded by the loss cap of the side that loses it; a battle that ends within `pil.battle.min_respite_minutes` (2 minutes, provisional) of its start starts no respite; Essence from deaths in a battle between two steward-run settlements that hold no grievance against each other is not collectable". Add the three scenarios to §5 with B1, B4 and B6 as owners, and ask D5 for the account rules that make a second account visible.

### A0-F06 The world loop has nothing to win and no way to settle a conflict

- **Severity:** High (it is the most likely reason a steward leaves a world after the plateau, and pillar 3's conflicts become either endless or pointless).
- **Category:** player experience; economy and balance.
- **Attacks:** PIL-046 (a battle result moves losses, stores, Essence, a grievance and a respite, and no territory); PIL-053 and PIL-060 (territory pressure and competition for sites and resources cause grievances); PIL-055 (a plateau after 14 days); PIL-069 ("a world in which one settlement leads goes on until its lifetime ends"); PIL-022 (an attacked settlement regains its strength in 2 hours of active play); Figure 4.
- **Failure scenario.** Two neighbours, Eli (steward) and an AI settlement of the clade Moss, compete for one resource field at their border. The territory pressure records a grievance; Moss's anger passes its threshold; Moss attacks; Eli wins or loses, the respite starts, and the border, the field and the pressure are exactly as they were, because no battle result moves territory. The grievance recurs, the attack recurs, and the pair fights every respite for the life of the world, while Eli's battles are worth at most the 2 hours of play that PIL-022 caps a full absence's loss at. Meanwhile the first steward to expand to the rich field keeps it for ever, because war cannot reverse an expansion, which is the "first week decides the world" that pillar 6 rules out. After day 14 Eli's settlement has plateaued, there is no standing, no objective and no end unless the host set a lifetime: the days-and-weeks loop has an input (contact) and a feedback (respites and upkeep) but no output a steward can aim at.
- **Recommendation.** Give the world loop an object of conflict and a goal, without breaking pillar 6: (a) contested resource sites and the territory outside a settlement's core can change hands in a battle, within the loss cap, so that a war can settle its cause (A0-M4, claims); (b) a grievance whose cause has been removed fades faster, and B5 gives every conflict an end state (a truce, a settled claim or a faded grievance); (c) the world keeps a standing that a steward can aim at within a lifetime (A0-M6); (d) PIL-065 adds a guard: in B5's model, at least `pil.guard.conflict_resolution_share` (60 %, provisional) of the conflicts between two neighbours end within `pil.guard.conflict_resolution_days` (7 days of world time, provisional).

### A0-F07 Pillar 3 binds only the AI: steward attacks need no cause

- **Severity:** Medium.
- **Category:** player experience; griefing or exploit.
- **Attacks:** PIL-012 ("AI attacks are motivated by grievances and never random"); PIL-063 ("a steward may send an attack against any settlement that is not under protection", an owner question); the vision's pillar 3 ("attacks feel motivated, never random").
- **Failure scenario.** A newcomer in week 4 leaves newcomer protection and is attacked at once by a steward with whom it has had no contact. From the newcomer's side the attack is random, which is what the vision's pillar 3 forbids; the pillar as written only stops the AI from doing it. The grievance it creates is recorded against the attacker but changes nothing, because only AI settlements attack on anger (edge case "A clade whose settlements are all steward-run holds a grievance").
- **Recommendation.** At gate A, recommend for PIL-063: **a steward may attack any settlement outside protection, but an attack without a cause on record (a grievance of the attacker's clade against the target, or a claim of A0-M4) is unprovoked: it pays `pil.conflict.unprovoked_cost_factor` (2, provisional) times the attack cost, it records a heavier grievance, and the target's report and the world's record name it as unprovoked.** The alternative "attacks only with a cause" is cleaner for pillar 3 but makes steward play depend on AI grievances; the alternative "any settlement, as written" leaves pillar 3 to the AI.

### A0-F08 Hostile world events are the random aggression that pillar 3 rules out

- **Severity:** Medium.
- **Category:** consistency; player experience.
- **Attacks:** PIL-041 ("a world event that B4 defines brings hostile creatures into a territory"); Table 1 pillar 3, which rules out "AI aggression from a random draw or a hidden timer"; the reference's 10 % hostile Supply crate outcomes (R-VIS-004), which B4 may turn into world events.
- **Failure scenario.** B4 adopts Supply crates as world events; a hostile draw brings a Swarm into a settlement whose steward sleeps. That is AI aggression from a random draw, so either B4 breaks pillar 3 or pillar 3 forbids the world events that PIL-041 allows. The world also has no owner for these creatures: R-VIS-030 (the Rogue faction) is `adopt-skirmish`, so the world game has no faction that owns them, and nothing says whether their attacks count against the loss cap or start a respite.
- **Recommendation.** Scope pillar 3 to clades and stewards, and give hazards their own rule: "a world event is announced at least `pil.conflict.event_warning_minutes` (5 minutes of world time, provisional) before it reaches a territory, counts against the loss cap and starts no respite (B4, B6)". Name the world's hostile non-clade side as a term of Table 3 (B4 proposes the word), or state that the Rogue faction's rules carry into the world game and mark the R-VIS-030 row accordingly.

### A0-F09 Battles between settlements of one clade are unreadable

- **Severity:** Medium (a real readability failure with a known mitigation that A0 has ruled out by wording).
- **Category:** IP and look-and-feel line; player experience.
- **Attacks:** PIL-014 ("in the world a clade's colour is the identity cue"); Table 1 pillar 5 "colour as the main identity cue"; the edge case "A steward attacks a settlement of the steward's own clade"; Table A "reads the battle from the shape and the glow of the crowd".
- **Failure scenario.** With 16 stewards and a handful of clades, two settlements of the clade Moss fight. Both crowds glow in Moss's colour; at the strategic zoom and at the distances the crowd cards are made for, nobody can tell the sides apart, and the steward bends the wrong flow. At 200 stewards, same-clade battles are the majority case unless there are dozens of clades, and the reference has 8 team colours (R-VIS-034).
- **Recommendation.** PIL-014 says what must be true instead of how: "the two sides of every battle are always distinguishable to every viewer at every zoom; a clade's colour is its identity in the strategic view, and in a battle the sides use a friend-and-foe palette". E2 and B5 then choose the colour scheme (the reference's colour families of co-op are one candidate). Add the number of clades to §4 as a provisional key that B5 owns.

### A0-F10 "Watched" is undefined, so choosing a fidelity is a strategy

- **Severity:** Medium.
- **Category:** griefing or exploit; technical feasibility.
- **Attacks:** Table 3 "abstract fidelity" (the economy "of a settlement that nobody watches") and "full fidelity"; AC-07 ("a region that no client watches"); the edge case on the fidelity hand-over; PIL-023.
- **Failure scenario.** The document introduces both fidelities but never says what "watch" means: a steward's own settlement view, another steward's view of the area, the strategic view, a battle nearby. If the two models differ by even a few percent, stewards learn which one pays: if the abstract model yields more, they log off or zoom out to the strategic view; if the full simulation yields more, they leave the client open on their settlement overnight; and a rival can force a neighbour into the worse model by looking at it. The edge case only covers the hand-over (nothing created or destroyed), not the rate. It also means that PIL-023's away yield is in practice the abstract model's yield, because an away settlement is unwatched.
- **Recommendation.** Define "watched" in Table 3 (first guess: a settlement is watched while any client's settlement view covers part of its territory; the strategic view never makes a settlement watched) and add a guard: "over a day of world time, the abstract model's yield for a settlement is within `pil.guard.fidelity_yield_gap` (±3 %, provisional) of the full simulation's, under the same orders" (B2 and D2 measure it). PIL-023 names the model it measures.

### A0-F11 The guards mix world time and active-play time

- **Severity:** Medium.
- **Category:** testability; griefing or exploit.
- **Attacks:** PIL-022 (an absence in world time, the regain "within 2 hours of active play"); PIL-029 (founding "in week 4" of world time, 50 % of the median "within 6 hours of active play"); PIL-055 (growth steps in minutes of active play, the plateau in days of world time).
- **Failure scenario.** A settlement grows during world time whether or not its steward plays (PIL-023), so a target in hours of active play does not say how much world time passes. For PIL-029 the script's section 6 shows the speed-up a late joiner needs to reach 50 % of the week-4 median: 28 times normal growth if the 6 hours are played in one sitting, 2.3 times if they are spread at 120 minutes a day, 1.2 times at 60 minutes a day (linear growth to the plateau), and 12.9, 1.08 and 0.54 times on a saturating curve. B2 can satisfy the guard with no catch-up or with a 28-fold one, and either is "correct". A large newcomer boost then invites the re-founding exploit that the document does not list: a steward who falls behind leaves the world (PIL-081 allows it) and joins again as a newcomer, with newcomer protection and the boost.
- **Recommendation.** Write every guard in one clock: world time at a stated session pattern ("a steward who founds in week 4 and plays the typical pattern reaches 50 % of the median within 3 days of world time"). Add the re-founding case to §5: newcomer protection and any catch-up apply once per person per world (D5 supplies the identity).

### A0-F12 The leader guard cannot fail in the model that runs it

- **Severity:** Medium.
- **Category:** testability; economy and balance.
- **Attacks:** PIL-028 (strongest at most 3 times the median, "in B2's economy model"); Table 3 "strength" (no formula); the edge case "A settlement keeps growing without limit".
- **Failure scenario.** In an economy model that varies only play time, the largest ratio presence can create is 1 ÷ 0.50 = 2.0, and between real patterns (240 against 10 minutes a day) it is 1.16 (script, section 3), so PIL-028 passes whatever B2 does. Runaway leaders in persistent worlds come from what B2's model leaves out: battle transfers (A0-F05), rich early sites (A1) and expansion that no battle can reverse (A0-F06). The guard therefore reports health while the world snowballs, and "strength" has no formula, so two areas can compute different ratios from one world.
- **Recommendation.** Define strength once (first guess: Workers + stores at their construction value + structures at their construction cost, with B2 owning the weights) and run PIL-028 in a joint model of B2 (economy), B4 (battle transfer), B5 (who attacks whom) and A1 (site richness), with the strongest settlement played by an always-attacking strategy and every other one by the typical pattern.

### A0-F13 Abandoned settlements accumulate and keep their sites

- **Severity:** Medium.
- **Category:** scale and cost; player experience.
- **Attacks:** PIL-078 (after 7 days a settlement "stays in the world under the deputy and the protection rules"); PIL-080 and PIL-081 (no removal except by the host's moderation); PIL-068 (free sites at least the places for further stewards); PIL-023.
- **Failure scenario.** Assume half of a world's stewards stop playing each month and every place is refilled (a genre rule of thumb, not a measurement). After 3 months a 16-steward world holds 16 active settlements and 24 deputy-run ghosts, after 6 months 48 (script, section 7), each growing at 50 to 80 % of an attentive steward, untouchable by any battle, holding its site and its territory, and costing the server a deputy and an abstract model each. A1 must keep free sites for newcomers (PIL-068) in a world that fills with ghosts, and only the host's manual moderation can clear them.
- **Recommendation.** Split the gate A owner question on PIL-080 in two, recommended options first: **removal by battle: never** (as proposed); **removal by abandonment: yes**: a settlement whose steward has not been present for `pil.session.dormancy_days` (21 days of world time, provisional) becomes dormant: it stops growing, its territory shrinks to its core and its site counts as free once the territory is empty; a returning steward restores it with its stores. B6 and C2 own the rules.

### A0-F14 The ledger's restatement verdicts contradict PIL-032 and CONVENTIONS §3

- **Severity:** Medium.
- **Category:** consistency.
- **Attacks:** the ledger rows §2.6, §2.8, §2.12, §2.13 and §2.17 (`adopt-skirmish`); prompt §5.3 ("`adopt-skirmish`: binds the skirmish only" and "The world game does not use it"); CONVENTIONS §3 ("the design phase gives a restating id the verdict of its owner"); PIL-032 ("a world battle uses the verbs of skirmish (R-VIS-151 to R-VIS-161, as C1 adopts them)"); §1.7 (world-game timers "through `ref.sim.tick_period`", which R-VIS-050 restates); AC-35.
- **Failure scenario.** An implementing agent reads that R-VIS-151 to R-VIS-161 are `adopt-skirmish`, which by the verdict's definition the world game does not use, and reads PIL-032 and AC-35, which require every one of those verbs in a world battle. The §2.17 row's own note says both. The same holds for Worker combat (§2.8), Leaders (§2.13) and the tick (§2.6). The totals of the ledger README (93 `adopt-skirmish`) are then wrong by most of these 32 requirements, and B4, C1 and A2 each have to send A0 a cross-area request to undo a verdict that CONVENTIONS §3 says they own.
- **Recommendation.** Where a PIL requirement already commits the world game to a restated rule, use `adapt` with that requirement: §2.17 `adapt` (PIL-032, PIL-085) with R-VIS-162 and R-VIS-163 as `adopt-skirmish` exceptions; §2.13 `adapt` (PIL-010, PIL-032); §2.8 and §2.12 `adapt` (PIL-040, PIL-010). For the other restatements, add a ledger convention that the effective verdict of a restating id is its owner's, and let the gate compute it, so that A0 does not have to guess wave B's verdicts in wave A.

### A0-F15 `adopt-skirmish` rows bind six-mode content that PIL-003 says is not built

- **Severity:** Medium.
- **Category:** consistency; scope.
- **Attacks:** PIL-003 ("until a mode is scheduled, its rules, maps, options and complexity label are not built"); `adopt-skirmish` "as written" on R-VIS-005, R-VIS-130 and R-VIS-131 (six modes on one skeleton), R-VIS-054 and R-VIS-081 (objectives of six modes), R-VIS-034 (Siege sides by colour), R-VIS-067 (Transit Gate waves in Launch Race, Flag Chain and Siege), R-VIS-142 (per-mode bot logic for Siege, Launch Race and Flag Chain), R-VIS-173 (Siege side choice in the lobby).
- **Failure scenario.** The v1 skirmish has one mode, but the ledger binds, as written, Siege's colour rule, the bot's Siege objective chains and the Transit Gate waves of three modes that are not built. An agent who builds what the ledger binds builds bot logic for modes that PIL-003 forbids; an agent who follows PIL-003 fails the requirements the ledger adopted. §2.2 is adapted for exactly this reason, so the treatment is inconsistent within one file.
- **Recommendation.** Mark these rows `adapt` with PIL-003 (their mode-specific clauses wait for the mode's scheduling, their Spawn Control clauses bind as written), or add one exception row per requirement. B9 confirms each when it writes the skirmish.

### A0-F16 A battle floor of 100 figures is below the vision's "hundreds"

- **Severity:** Medium.
- **Category:** consistency; scale and cost.
- **Attacks:** PIL-021 (`pil.battle.bodies_floor` 100 figures, both sides together); the vision §1 ("hundreds of small autonomous figures"); Table A ("hundreds of Workers pour into a fight").
- **Failure scenario.** At owner gate B, A2 and B4 find the 200-steward tier tight on CPU and size typical battles at the floor: 50 figures a side. That meets PIL-021 and misses the pitch by a factor of four to ten; the reference's maps cap populations at 1,400 to 2,000 (R-VIS-023), so the reference's look of a crowd fight is a scale Species would no longer show.
- **Recommendation.** Set two values: a typical-battle target of `pil.battle.bodies_typical` (400 figures, provisional), which the vision's "hundreds" means, and the at-risk floor of 200 below which the pillar is escalated. A2's and B4's models report both at 16 and at 200 stewards.

### A0-F17 No smallest world game per stage

- **Severity:** Medium.
- **Category:** scope.
- **Attacks:** Table B (stages per audience); PIL-031; the absence of any per-stage scope in §2; seed challenge 12.
- **Failure scenario.** The first playable is skirmish (O20), and the document names no first world-game milestone. Every B area therefore designs the full loop (relations, standing orders, protection, fidelity switching, expeditions), and E3 has no anchor for the smallest world that still meets each pillar. For a hobby project with one developer (AGENTS.md), the world game's minimum is then everything.
- **Recommendation.** Add a table "smallest world game per stage": for stage 1, one region, one steward plus AI settlements, the session pace (A0-F01), standing orders as the default set plus three priorities, grievances with two causes (territory pressure and attacks), full fidelity only (no abstract model while the world is one region and the server is in-process), and the away report; for stage 2, the always-on pace, several stewards, the protection rules in full and the abstract model; for stage 3, region processes. Each row names the pillars and guards it meets.

### A0-F18 The pillar 5 guard checks provenance, not distinctness

- **Severity:** Medium.
- **Category:** IP and look-and-feel line.
- **Attacks:** PIL-027 ("every asset of Species carries a record of its new authorship (E2), and the strategic view uses only the features of the renderer's visual language"); O9 ("new character silhouettes, textures, UI layout and logo, so Species is not a copy"); O13 (a legal check before commercial release); seed challenge 10.
- **Failure scenario.** An artist newly authors a Worker card whose silhouette, proportions and glow match the reference figures closely. The provenance record is complete, the guard passes, and the result is the copy that O9 forbids. The clean room stops the agents from comparing with the reference game's art, so no agent-run check can catch it.
- **Recommendation.** Add to the guard a look-alike review by people allowed to see the reference game (the owner or counsel, outside the clean room) at each art milestone of E2, against a short list of features that must differ (Worker and Leader silhouettes, structure silhouettes, the HUD layout, the logo), with its result recorded in E2's manifest. PIL-027 cites O13.

### A0-F19 R-VIS-007 is adopted with a qualifier, which makes it an adaptation

- **Severity:** Low.
- **Category:** consistency.
- **Attacks:** ledger row R-VIS-007 (`adopt`, with the note "for every reference requirement Species adopts"); prompt §5.3 (`adopt` binds "as written"; an `adapt` moves its numbers to Species keys).
- **Failure scenario.** R-VIS-007 says every reference tuning value is kept as the starting baseline. Read as written in the world game, it binds the values of the requirements that Species adapts too, which §5.3 says move to Species keys and may change. The note narrows the rule, which an `adopt` cannot do.
- **Recommendation.** Make it `adapt` with a PIL requirement that states the narrowed rule (the reference values are the starting baseline of every `adopt` and `adopt-skirmish` requirement; an `adapt` starts from them only where its area says so).

### A0-F20 Which attacks warn the target is stated three ways

- **Severity:** Low.
- **Category:** consistency.
- **Attacks:** PIL-025 (AI attacks warn at least 10 minutes ahead); Figure 5 (every attack enters `Declared` with "the target is warned"); PIL-063 and AC-16 (a steward's attack: no warning stated).
- **Failure scenario.** B5 reads Figure 5 and warns every target; B6 reads PIL-025 and warns only for AI attacks; C3 designs the HUD for one of the two. A steward attacked by a steward without warning calls it a bug.
- **Recommendation.** State one rule for every attack and its lead time, with a separate key if a steward's attack warns differently (`pil.conflict.steward_warning_minutes`, provisional), and make Figure 5 cite it.

### A0-F21 Vocabulary overloads: frozen, attack, the aide's prefix, `Fidelity::Abstract`

- **Severity:** Low.
- **Category:** consistency.
- **Attacks:** Table 3 rows "frozen", "attack", "aide", "full fidelity" and "abstract fidelity".
- **Failure scenario.** (1) "Frozen" already means the grace-period hold of a match in the reference (R-VIS-052, R-VIS-176 "During the grace period the world is frozen"; the glossary's "held (world held)"), and skirmish runs on the same server, so a player-facing "frozen" names two different states. (2) "Attack" is the world-game declaration (`Attack`, `battle.attack`), the combat verb of every Worker, Siege's "attackers and defenders" (glossary) and a member function in the reference (`Attack` in R-UNIT-361). (3) The aide is defined as a role beside the deputy, not part of it, but its string id is `deputy.aide`; the check-in is a session concept filed as `report.check_in`. (4) `Fidelity::Abstract` spells the affix that AGENTS.md R2 bans; it is an enumerator, not a type, but the repository checker of CONVENTIONS §8 (a term whose last word is a banned affix "takes a different identifier") should not need to know that.
- **Recommendation.** Keep "frozen" for the world and add to Table 4 that skirmish's grace period says "held" in player-facing text; give the declared world attack a distinct code name (`AttackDeclaration`, string id `battle.attack`) and keep "attack" in prose; move the aide to its own root (`aide`) or under an `ai` root shared with the deputy; rename the enumerator `Fidelity::Modeled`.

### A0-F22 "Race" survives in file names, and "Species" plus "clade" needs a title screen

- **Severity:** Low.
- **Category:** IP and look-and-feel line; consistency.
- **Attacks:** PIL-091 (a people is a clade, an owner question); prompt §5.1 and §5.2 (`06_races_relations_and_conflict.md`, B5 "races and relations", prefix `REL` "races, relations"); Table 3's trademark screen, which covers the vocabulary but not the product name.
- **Failure scenario.** The owner confirms "clade" and the design keeps a file and an area named for races, so the next reader finds two words for one concept. Separately, to the challenger's general knowledge a creature-evolution simulation released on PC in 2018 is titled "Species" with a subtitle, and its subject is lineages of evolving creatures, which "clade" sits right next to; the screen does not mention it.
- **Recommendation.** At gate A, recommended option first: **clade** (it fits a game named Species, avoids the real-world weight of "race" in "an angered race attacks its neighbours", and is unclaimed as a game term); the alternatives are "race" (the vision's word, familiar from strategy games, heavier) and "kin" (plain, but reads as family). If clade is confirmed, the coordinator renames the B5 file and area title, or records why not. Add the product name to the list that the pre-release registry search covers (O13), with the note above.

### A0-F23 PIL-020 and AC-05/AC-06 disagree with each other and with PIL-070

- **Severity:** Low.
- **Category:** testability.
- **Attacks:** PIL-020 (a present steward who gives no new order for 60 minutes; the exceptions promote, scrap and direct control); AC-06 (exceptions promote and direct control only); PIL-070 (a steward with no input for B6's idle threshold is away).
- **Failure scenario.** If B6 sets the idle threshold below 60 minutes, the steward of AC-05 becomes away during the test and the guard of pillar 1 measures the deputy. AC-06 fails a build that has the scrap command PIL-020 allows, or passes one without it.
- **Recommendation.** AC-05 states that the steward gives input other than orders (camera movement) throughout; AC-06 lists the same three exceptions as PIL-020 (and says whether demoting a Leader, R-VIS-114, counts as addressing one Worker).

## The A0 tests, answered

| Test | Answer | Findings |
|---|---|---|
| Does every pillar exclude something real? | Yes: each of Table 1's "rules out" columns names a design a plausible area would propose. Pillar 3 excludes it only for the AI. | A0-F07 |
| Do the pillars conflict? | Yes, in three places: pillar 2's safety (PIL-086) against a minute loop that defenders actually play; pillar 6's bounded losses against pillar 3's conflicts having any stakes; pillar 1's autonomy against pillar 3 is safe as long as only steward-level actions (expansion, expeditions, attacks) create grievances, which the document should say. | A0-F04, A0-F06; see A0-M4 |
| Is each core loop closed, with a stop? | The battle loop is closed and capped (20 minutes). The settlement loop is closed and stops at the plateau, but its battle input leaks Essence to the winner. The world loop has an input and feedback but no output a steward can aim at, and conflicts cannot settle their cause. The ten-minute player has a loop (the check-in), and under PIL-023 it earns 92 % to 98 % of the heavy player's growth, which is the problem of A0-F03 rather than a gap. | A0-F03, A0-F05, A0-F06 |
| Is the vocabulary unambiguous and clear of titles? | Mostly; four overloads, the race and clade split, and the product name. | A0-F21, A0-F22 |
| Are the ledger verdicts defensible? | The adaptations of §2.2, §2.3, R-VIS-003, R-VIS-008, R-VIS-170 and R-VIS-174 are. The blanket `adopt-skirmish` on restatements and on six-mode content is not. No `drop` leaves a hole, but the Rogue faction's world-game role is missing. | A0-F08, A0-F14, A0-F15, A0-F19 |
| Is anything left undefined, or decided too early? | Undefined: strength, "watched", the number of clades, the world's hostile side, abandonment, the integrated pace. Decided too early: the Essence-on-the-field rule (PIL-045, B1 and B4), the ban on away-only protection (PIL-086, B6). | A0-F05, A0-F04, A0-F10, A0-F12, A0-F13 |

## Make it better

### A0-M1 Co-op first in integrated worlds

In an integrated world, the stewards share one side against the AI clades by default: they can be angered at by AI clades, defend each other's settlements with expeditions and trade stores, and attacks between stewards are a world setting that is off unless the host turns it on at creation (shown on the join page). Dedicated worlds default to the full rule set.
- **Benefit:** removes the host's clock advantage over the other stewards' settlements (A0-F02) and most AFK targeting among friends (A0-F04) in the stage that cannot police them; makes the friends' audience of Table B a cooperative experience, which is the cheapest version of pillars 3 and 6 to build first (relations with AI clades only); gives stage 1 a world loop that needs no steward-against-steward balance.
- **Cost:** a world setting and a second default; B5 has to design reinforcement between stewards earlier; steward-against-steward balance is first tested in stage 2.

### A0-M2 Watch the battles your deputy fought

Every battle in the away report links a replay that the steward can watch at up to 8 times speed, with the deputy's decisions marked on the timeline and each one tied to the standing order that permitted it.
- **Benefit:** turns the defender's report into a minute-loop experience (A0-F04), is the strongest evidence for pillar 4 (stewards see why the deputy acted, which PIL-026's 80 % prediction target needs), and doubles as the debugging tool for the deputy.
- **Cost:** a replay recording per battle: either the command stream plus a seed, which needs the reproducibility that A2 decides (DEC-240, DEC-241), or a sampled state stream, at a first guess of 50 to 200 KB per battle minute (an estimate, not a measurement; D3 and D4 measure it); storage for 7 days of reports per settlement.

### A0-M3 Defence windows for attacks between stewards

Each steward sets a daily window of `dep.window.hours` (4 hours of world time, provisional) in which other stewards may attack the settlement; outside it only AI clades may attack, and only on a grievance. The window is shown on the strategic view. The host may switch the rule off for a world.
- **Benefit:** steward-against-steward battles happen when the defender is likely present, so the minute loop is two-sided; time-zone griefing disappears without making away-only protection stronger than presence (A0-F04); it is predictable protection, the lesson of Table D's survival-sandbox row.
- **Cost:** fewer attack opportunities in worlds whose stewards' windows never overlap (B6 models the overlap at 16 stewards across 3 time zones); a rule pillar 2's "no play at set times" must allow, because the window only restricts attackers and never rewards presence at a fixed hour.

### A0-M4 Claims: give conflict something to settle

Territory pressure and competition for resources create a **claim** on a named contested area (a resource site or a border cell of territory outside a settlement's core). A battle won by the claimant moves the claim's area, within the loss cap; tribute or a truce can settle it; and a settled claim removes the grievance's cause, which fades it.
- **Benefit:** closes the world loop (A0-F06): wars have an object, can end, and can reverse an early expansion, which keeps pillar 6 without capturing settlements; it gives steward attacks a cause on record (A0-F07); grievances arise only from steward-level actions, so pillar 1's autonomy never creates a war nobody chose.
- **Cost:** a new world-game term (proposed here, not invented in the documents: "claim"), territory that can change hands (A1 and B5), a strategic-view layer for claims (C3), and the risk that a strong settlement claims every border, which the attack cost and upkeep on claimed territory must bound.

### A0-M5 A check-in fast path in the client

The client offers a check-in entry that opens straight into the away report and the standing-order editor in the UI framework, without loading the 3D settlement view, and connects to the world with the same server API calls that a later companion app makes (PIL-075, D6).
- **Benefit:** a 2-minute check-in that is actually 2 minutes, not 2 minutes after a full world load; the first consumer of the server API in v1, which proves O6's "without server changes" before a companion exists; cheap, because the pages are needed anyway.
- **Cost:** a second client entry point and a page set that must stay in step with the in-world HUD; C2 and C3 own it.

### A0-M6 Standing and an epilogue for a world's lifetime

A world keeps a public standing of its settlements by renown, earned by defences held, grievances eased, claims settled and structures raised (not by stores taken), and when a world's lifetime ends, an epilogue page shows the standings and each settlement's history. A1's default lifetime then has something to end on.
- **Benefit:** a goal for the days-and-weeks loop after the plateau (A0-F06) that does not need capture or removal (pillar 6); rewards peaceful play as well as war (pillar 3's easing); a reason to return to the same community server for the next world.
- **Cost:** a renown model that B5 and B2 balance against farming (the attack cost and the respite bound it); achievements and leaderboards are reference hooks (R-VIS-041, R-VIS-043) that C2 would turn into a feature.

## Model check (no model is required for A0)

`species_design_work/scripts/A0C_SessionPacing.py` turns the world-time targets of §4 into real time at the session patterns of Table C. What it shows:

| Question | Result |
|---|---|
| Share of world time a steward is present in an always-on world | 0.7 % (10 minutes a day) to 16.7 % (240 minutes a day) |
| Growth of 120 against 10 minutes a day under PIL-023 | +7.6 % at a 50 % away yield, +4.1 % at 65 %, +1.9 % at 80 % |
| Growth of a never-present settlement against a heavy player | 92 % to 98 % |
| Largest strength ratio from presence alone (PIL-028 limit 3) | 2.0 in theory; 1.16 between real patterns |
| Battles fought in person per week, heavy player, uniform timing | 0.17 to 0.83 of 2 to 10 |
| Solo integrated world: hours of play per battle | 16.8 to 84 (8 to 168 sessions) |
| Integrated world, host running it 120 minutes a day: real days to the plateau; an 8-hour respite | 168 days; 4.0 days |
| Late joiner's growth speed-up for PIL-029 | 0.54 to 28 times, depending on how the 6 hours are played and on the growth curve |
| Abandoned settlements after 3 and 6 months (assumed churn of half the stewards a month) | 24 and 48 beside 16 active ones |

## Owner questions of gate A: the challenger's recommendations

| Question (gdd/00 §9) | Recommended | Why |
|---|---|---|
| Add pillar 6, Always a way back | Yes, with A0-M4 | It excludes real designs (capture, absorption, compounding growth) and the vision's five pillars rule on none of them; without claims it also removes the stakes of war (A0-F06), so it needs a way for conflict to move something |
| Clade or race | Clade | A0-F22 |
| May a steward attack any settlement outside protection | Yes, with the unprovoked surcharge; off by default in integrated worlds | A0-F07, A0-M1 |
| Can a settlement ever be removed from a world | Never by battle; by abandonment, yes, through dormancy | A0-F13 |
| New: keep an integrated world running after the game closes (amends O4's example and adds a local dedicated form to stage 1, O10) | Yes, as the headless build in a background process | A0-F01 |
