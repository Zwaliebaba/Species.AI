# Playtests

The evidence that the MVP's playtests produce (`Design/implementation/mvp_scope.md` §6, MVP-002). The paused design areas resume from it (owner decision O51), starting with B1, B2, B5 and B6. It lives here, at the repository root and outside `Design/` (§6.3).

## What is here

| File | What it holds | Written |
|---|---|---|
| `Playtest1.md` | the session log of Playtest 1: an hour of play and a run (§6.1) | after every session of M3's playtest |
| `Playtest2.md` | the session log of Playtest 2: coming back (§6.2) | after every session of M4's playtest |
| `M0.md` to `M4.md` | one record per milestone: the build that met its exit criteria and the evidence for each one (§3) | when the milestone ends |
| `M1.md`, its performance section | the reference test machine, pinned by the first run, and each run of the performance job with every figure against its budget (DBG-208, DBG-209) | by `Build/RunPerformance.py`, on every run on the reference test machine |

## The session log

After every session of a playtest, the steward adds one entry to that playtest's file. An entry is a heading and three lines:

```markdown
## 2026-11-02, world day 3 14:20, build 0.1.0+3f2a9c1

- **Fun:** what was fun.
- **Dull:** what was dull or frustrating.
- **Confusing:** what was confusing.
```

- The heading carries the real date, the world time when the session ended, and the build's identity. The identity is the one generated build identity of ADR-057; until it exists, it is the commit the build was made from.
- Each line is one sentence or a few, in the steward's words.

The playtest's measured lines (tables 5 and 6 of §6) come from the server's statistics (MVP-064) and the epilogues. They are added to the playtest's file under their own heading when the playtest ends, beside the session log rather than in it.

## A milestone record

A milestone record names the build (its identity, as above) and lists each exit criterion of the milestone in §3 of the MVP scope, with what showed it was met: a test, a CI run, or the owner's run and what was seen.
