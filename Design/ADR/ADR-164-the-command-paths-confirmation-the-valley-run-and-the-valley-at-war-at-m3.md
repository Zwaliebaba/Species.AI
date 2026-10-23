---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T9, owner decisions O80, O98)
---

# The command path's confirmation, the valley run and the valley at war at M3

## Context and Problem Statement

M3's T9 closes phase A with three things:
- **The command path's confirmation** of [ADR-087](ADR-087-command-path-validation-and-timing.md) on M3's build (table 1 of the M3 plan, MVP-094). The server refuses every forged command of a test client with its reason and its counter: another settlement's order, a missing field, a pool slot as an id. A crowd order for a group emptied between issue and execution does nothing. The steward and an AI settlement whose commands reach one tick win a contested action in shares within 45 % to 55 % over 1,000 ticks.
- **The valley run** (the M3 plan §4). Every settlement of a valley is driven by a policy, the steward's by its scripted steward with a stated conflict policy, and stepped faster than real time on a corpus of seeds. It reports Playtest 1's measurable lines (the MVP scope's table 5): the figures in the steward's battles, battle lengths, conflicts settled, the strongest settlement against the median, the steward's battles in a run, and the growth steps of the first 5 hours. The task is done when the valley run meets the lines on the corpus, or the values it needs go to the owner.
- **The performance scene** of four settlements with two battles, headless and drawn: the region at the MVP's bound (MVP-015, MVP-022, [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md) item 10).

The tuning of table 3's relation, battle and run values waits for the owner's M2 runs (O98).

Four things stand in the way:
- **At the MVP no two senders command one object.** Each command names a team its sender commands, so no command of the steward's can undo an AI settlement's in the same tick.
- **Playtest 1 judges conflicts "within 7 world hours"**, PIL-123's 7 days at the session pace, where an hour of play is a world day (MVP-070).
- **The valley run is long.** A run is 8 hours of play, 288,000 ticks of four settlements, and the corpus has eight seeds.
- **The tool worlds have runs.** Since [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md), a world ends `mvp.run.length_hours` after it begins and is then read-only. The growth scene, the region of four scripted settlements and the frame bench's grown settlement would measure a world that no longer changes.

## Decision Drivers

* ADR-087's M3 confirmation, on the game's own senders and the world server's path.
* Playtest 1's measured lines predicted before the owner plays two runs over two to three weeks (the M3 plan §4).
* The region's bound measured as the owner's performance run measures every other figure.
* No new state version for what only measures.

## Considered Options

1. **The game's tests and tools**: the confirmation as GameLogic tests through the world server and the region's queue; the valley run as a scene of the performance tool on threads of its own, judged by the performance script; the valley at war a scene of the tool and of the frame bench, made by one function of GameLogic.
2. **A test client over the transport**, as ADR-087 states the M4 part.
3. **The valley run as a CI test.**

## Decision Outcome

Chosen option: 1.
- **Option 2** is M4's: the MVP's client reaches the integrated server in process, and MsQuic arrives at M4 (MVP-070).
- **Option 3** would put two hours of four cores into every CI build. CI runs the quick valley run and the valley at war's quick scenes instead, through the performance script's self-test.

* **The command path's confirmation** (`GameLogicTests/CommandPathTests`):
  - **Forged commands.** A test client of the world server, in the valley with three AI settlements, sends:
    - another settlement's declaration, claim, tribute and placement, each refused as not permitted;
    - a declaration and a claim each with its last field cut off, each refused as malformed;
    - a declaration naming its Leader's pool slot in place of its stable id, which executes and does nothing, with `AttackRefusal::NoLeader` as its reason.

    Each outcome reaches the client with its sequence number, and the region's queue counts four not permitted and two malformed.
  - **An emptied group.** The steward selects Workers of its settlement and orders them. The Workers die before the order's tick, so the order, checked again at its tick, executes with no effect and moves no Worker.
  - **The contested tick.** No two senders command one object at the MVP, so what the keyed order decides between them is which of their commands executes first. That is the action contested. The steward, connected as `ConnectSteward` connects it, and the AI settlement at site 3 each send a command every tick for 1,000 ticks of the valley. The steward's executes first in 48.8 % of them.
* **The valley run** (`Tools/SpeciesPerformance/ValleyScene`):
  - **The run.** `RunValley` starts seed *n*'s valley with the steward's settlement at `mvp.sandbox.steward_site` and three AI settlements. The steward's settlement is run by its scripted steward with the decisions of a Territorial settlement, which is the run's stated conflict policy. The run lasts `mvp.run.length_hours`, then as long as its battles in progress need, up to `pil.battle.max_minutes`.
  - **What it keeps.** Each battle's ticks, sides, end and figures. The figures are the most of both sides' living figures on its battlefield at once (MVP-043), counted every second. It also keeps each claim and its end from the server's statistics (MVP-064), the steward's growth steps, and each settlement's strength, Workers and renown at the end.
  - **The corpus.** `RunValleys` runs seeds 1 to 8 on as many threads as the machine has, since each run is a world of its own. Its step times are therefore those of runs sharing the machine, and are reported, not judged.
  - **The tool.** `SpeciesPerformance.exe --scenes valley` runs the corpus only when named, since it takes about an hour of a 4-core machine's time. `Build\RunPerformance.py --valley` runs it and judges each line over the corpus (`valley_rows`). It records the result in its own section of `playtests/M3.md` and fails when a line is missed. A valley run's figures are the simulation's, not the machine's speed, so it neither reads nor pins the machine. A build by another compiler may contract floating-point sums differently and so play the corpus differently.
  - **The lines.** Each is judged as follows:
    - the largest gap between the steward's growth steps in hour 0 to 5, on every seed (20 minutes);
    - the figures in the steward's battles over the corpus: the median at 300 to 450, none below 200;
    - every battle's length: 90 % within 4 to 12 minutes, none above 20;
    - conflicts reaching an end state within 7 hours of play, at least 60 %. A conflict is a claim, which has an end state, and it is judged when it ended or was made 7 hours before the run's end. These 7 hours are PIL-123's 7 world days at the session pace;
    - the strongest settlement's strength at the end against the median, at most 3 times on every seed;
    - the steward's battles in a run, 4 to 10 on every seed.
* **The AI's defense of a claimed area** ([ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md) amended). The first valley run found that every claim battle on an AI settlement, and on the steward's scripted stand-in, was won by the claimant in 30 seconds.
  - **Why.** A claim battle's objective is the claimed half's resource site, away from the holder's home. The holder's battle core stepped only once the battle had started, with no objective. So no defender reached the site before the attacker had held it alone for `mvp.battle.objective_hold_s`.
  - **The change.** An AI settlement now defends from the warning of a claim battle declared on it: its battle core steps with the claimed site as its objective (`ClaimDefenseOf`). Its Leaders take their Workers there, and it starts no muster and demotes no Leader meanwhile. A raid's defense is unchanged.
  - **What it changes.** The settlement AI's state does not change, so no state version moves.
* **The valley at war** (`GameLogic/ValleyAtWar`), the region at the MVP's bound:
  - **The world.** Seed 1's valley with the steward's settlement under its scripted steward, which has the Territorial decisions. The two raids of `ValleyRaidsOf` start at once, each of two Leaders with 200 Workers between them: the steward's first valley neighbour raids the steward's settlement, and the other pair raids within itself.
  - **The population.** Every settlement is filled at home to `mvp.settlement.population_max` (300), its expedition among it, so the region holds 1,200 figures.
  - **Stepping.** It steps until both battles are in progress, and its run never ends.
  - **Headless.** The performance tool measures it as `valley-battles`, 1,000 ticks, and the script judges its 95th-percentile step against the region step budget (item 3) and an AI settlement's decisions, its economy and its battle cores (`sim.stewards.step`) against an AI's 60 µs (item 2).
  - **Drawn.** The frame bench makes it before its window opens, saves it, and continues it from its snapshot as `valley-drawn`, the steward the scene's client. The camera is aimed over the attack on the steward's settlement, and the script judges the scene against the frame budget of the sandbox's battle (ADR-126, ADR-156).
* **The tool worlds' runs** (ADR-163 amended). The growth scene, the region of four scripted settlements, the valley run's corpus aside, and the frame bench's grown settlement set their run's end at the largest tick, since they measure settlements, not a run's end.

### Consequences

* Good, because ADR-087's M3 part runs in CI on every build, through the world server and on the game's own senders.
* Good, because Playtest 1's measurable lines are predicted, and their misses traced to their causes, before the owner plays.
* Good, because the region's bound at the MVP, headless and drawn, is judged by the owner's performance run as every other figure is.
* Bad, because the contested action is the order of execution, which the MVP's commands cannot make into a contest of one object.
* Bad, because the valley run's steward is a policy, not a player. A steward who defends, tributes or claims differently fights other battles, and Playtest 1 judges that.
* Bad, because the corpus takes about an hour of a 4-core machine, so it runs on demand, not in CI.
* It forecloses: a claim battle decided before its holder could reach the claimed site; a tool world that stops changing at its run's end.

### Confirmation

GameLogicTests:
- **`CommandPathTests`.** The forged commands refused with their reasons and counted; the emptied group's order doing nothing; the steward first in 45 % to 55 % of 1,000 contested ticks.
- **`SettlementAiTests`.** An AI settlement whose half is claimed and declared on defends from the warning. Its Leaders bring at least five of its figures to the claimed site before the claimant is released. Without the change, the test fails.
- **`BattleTests`.** The valley at war holds both raids' battles in progress, every settlement at its population, the steward's run by its scripted steward with the Territorial decisions, and a run that never ends.

`Build\RunPerformance.py --self-test` runs the quick valley run and the quick scenes `valley-battles` and `valley-drawn`, and proves the valley run's judgement and record on fixtures.

**The valley run of seeds 1 to 8** on a Linux build of the game's libraries (`playtests/M3.md`, "The valley run") meets two of Playtest 1's six measurable lines: every conflict settled within 7 hours of play, and the strongest settlement at most 1.40 times the median. It misses the steward's battles in a run (11 to 22), battle length (13 % within 4 to 12 minutes), the figures in the steward's battles (249 typical, 79 least) and the growth steps' largest gap (76.6 minutes). [DEC-1008](../decisions/species_design.md#dec-1008-the-valley-runs-misses-of-playtest-1s-lines) puts these misses, their causes and the levers to the owner (T9's "done when"); the tuning waits for the owner's M2 runs (O98). The first run, before the defense of a claimed area, missed the same four lines further: 17 to 42 battles, 123 figures typical, and every claim battle won in 30 seconds.

**The owner's performance run** on `Release|ARM64` judges `valley-battles` and `valley-drawn` (T9's "done when").

## Pros and Cons of the Options

### 1 — the game's tests and tools

* Good, because it reuses the world server, the tool, the bench and the script.
* Bad, because the transport is not under test until M4.

### 2 — a test client over the transport

* Good, because it is the path a remote client takes.
* Bad, because the MVP's transport before M4 is in process.

### 3 — the valley run in CI

* Good, because every change would be judged against Playtest 1's lines.
* Bad, because it costs about an hour of four cores per build.

## More Information

* Amends:
  - [ADR-087](ADR-087-command-path-validation-and-timing.md): its M3 confirmation;
  - [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md): the region at the MVP's bound measured;
  - [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md): the scene `valley-battles`, the valley run and `--valley`;
  - [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md): the scene `valley-drawn`;
  - [ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md): the defense of a claimed area from its warning;
  - [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md): the tool worlds' runs.
* Owner decisions O80, O98; MVP-015, MVP-022, MVP-043, MVP-049, MVP-053, MVP-056, MVP-064, MVP-070, MVP-094; PIL-021, PIL-028, PIL-047, PIL-055, PIL-065, PIL-123; DEC-1006, DEC-1008.
* The code:
  - `GameLogic/ValleyAtWar`, `SettlementAi` (`ClaimDefenseOf`), `Battles` (`ObjectiveOf`);
  - `Tools/SpeciesPerformance/ValleyScene`, `SpeciesPerformance`, `SceneRun`, `GrowthScene`;
  - `Tools/SpeciesFrameBench/SpeciesFrameBench`;
  - `Build/RunPerformance.py`;
  - `GameLogicTests/CommandPathTests`, `SettlementAiTests`, `BattleTests`, `RunTests`.
