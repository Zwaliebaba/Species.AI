---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T12; owner decisions O109, O110)
---

# The away run

## Context and Problem Statement

Playtest 2 is eight real days of the owner's computer and attention (the MVP scope §6.2). The [M4 plan](../implementation/m4_plan.md) T12 predicts its measurable lines headless first, so that the owner's eight days test what a script cannot. The values are retuned before the owner starts, not during. Table 6 of the scope has two measurable lines:
- **PIL-022.** After a night or a day away with attacks, the settlement is back to its strength from before the absence within 2 world days.
- **PIL-023.** The steward's growth per world day: present days grow more than check-in days.

PIL-102 names the typical pattern, an hour of active play and two check-ins a real day, and the check-in pattern, one check-in a day. How does a headless run stand for a steward who is present, and for one who is away?

## Decision Drivers

* The run uses the simulation, the deputy and the presence the server uses (ADR-185, ADR-186), not a model of them.
* A present steward is played as the valley run plays one (ADR-164), so that the runs compare.
* The two lines need runs that differ in the steward's pattern alone.
* The run is a measurement: it names its schedule and its stand-ins.

## Considered Options

1. **Two always-on runs per seed, one at each pattern.** Presence follows the pattern's schedule. While present, the steward's settlement is run by its scripted steward with a Territorial settlement's decisions; while away, by the deputy alone.
2. **One run per seed alternating the patterns by day.** Present days are compared with check-in days within the run.
3. **B2's economy model.** The lines are computed from the abstract model of the design phase.

## Decision Outcome

Chosen option: 1.
- Option 2 carries a present day's queue and battles into the check-in day after it, so the days are not independent.
- Option 3 does not exist at M4's fidelity, and would not run the deputy.

What is built:

* **The schedule.** Each world day from the run's start:
  - the typical pattern visits for 60 minutes at its start, then checks in for 6 minutes at 12 h and at 17.5 h;
  - the check-in pattern checks in for 6 minutes at its start.

  6 minutes is the middle of `pil.session.check_in_minutes`' 2 to 10 (PIL-074). The typical pattern's night absence is 12 h less its hour, and the check-in pattern's absence is a day less 6 minutes, so both meet PIL-022's absences of 8 and 24 hours.
* **Presence.** Between ticks, as the world server applies it (ADR-185), `SetStewardPresence` opens or closes the absence:
  - present: the steward's settlement has a scripted steward with a Territorial settlement's decisions (ADR-162, ADR-164);
  - away: it has none, and the deputy commands its battles (ADR-186).
* **The world.** The valley of each seed at the always-on pace (ADR-184), with three AI settlements, run for its run (MVP-060, 8 world days at the always-on pace).
* **What it keeps** (`Tools/SpeciesPerformance/AwayScene`):
  - each absence: its ticks; the battles the settlement defended that began in it; the deputy's actions; the strength (MVP-038) when the steward left and came back; and the first hourly sample after the return at which the strength was back;
  - each world day's strength of the steward's settlement and the AI settlements' mean, and the steward's Workers.
* **The lines** (`Build/RunPerformance.py --away`, judged over the corpus):
  - **PIL-022.** At the typical pattern, every absence of 8 hours or more with an attack is back within 2 world days of the return. One not back with less than 2 days of the run left is open and not judged. A corpus with no such absence misses the line, since nothing showed it.
  - **PIL-023.** The steward's strength gained per world day is greater at the typical pattern than at the check-in pattern on every seed. PIL-023's guard is reported beside it: the typical pattern at least 25 % more, and the check-in pattern at least 70 % of it.
  - Each run's strengths, battles, absences and deputy actions are reported.
* **The record.** `playtests/M4.md`'s section "The away run". A missed line fails the run with exit code 1. The machine is neither read nor pinned.
* **The corpus.** Seeds 1 to 8, both patterns, as many runs at once as the machine has threads. `SpeciesPerformance.exe --scenes away`. The quick run takes seed 1's first 8,000 ticks, and CI runs it.

### Consequences

* Good, because both lines are measured on the server's own rules, and the deputy is the one the owner's steward will have.
* Good, because the corpus says before Playtest 2 whether its lines can be met, and which values the owner must decide if not.
* Bad, because a present steward is a script. It places and decides as the scripted steward does, never as a person, so presence pays only through what the script does better while present.
* Bad, because an 8-day always-on run is 6.9 million ticks. On the development machine a run takes about 80 minutes, so the corpus takes hours.
* It forecloses nothing: a long absence of `pil.session.long_absence_days` (7 days) is another schedule.

### Confirmation

* `RunPerformance.py --self-test` runs the quick away run and proves the lines and the record on fixtures, in CI.
* The owner's `Build\RunPerformance.py --away` on the reference test machine records the corpus in `playtests/M4.md`. A missed line goes to the owner with the values it needs before Playtest 2 starts.

## Pros and Cons of the Options

### 1 — two runs per seed

* Good, because the patterns differ in nothing else.
* Bad, because it doubles the runs.

### 2 — alternating days

* Good, because one run serves both lines.
* Bad, because the days carry into each other.

### 3 — the model

* Good, because it is fast.
* Bad, because it is not built and does not run the deputy.

## More Information

* Owner decisions O109, O110; PIL-022, PIL-023, PIL-074, PIL-102, MVP-003, MVP-060; the MVP scope §6.2 (table 6); the [M4 plan](../implementation/m4_plan.md), task T12.
* Related: [ADR-164](ADR-164-the-command-paths-confirmation-the-valley-run-and-the-valley-at-war-at-m3.md) (the valley run), [ADR-184](ADR-184-the-always-on-pace.md), [ADR-185](ADR-185-presence-and-absences.md), [ADR-186](ADR-186-the-deputy.md).
* The code: `Tools/SpeciesPerformance/AwayScene`, `Tools/SpeciesPerformance/SpeciesPerformance.cpp`, `Build/RunPerformance.py`.
