"""S22: generate the RNG / local-state census tables for 22_determinism_and_networking.md.

Inputs (passed as arguments, run with python3 -I):
  argv[1] = _work/drafts/X3_syncrand_sites.csv
  argv[2] = _work/drafts/X3_unsynced_rng_sites.csv
  argv[3] = _work/drafts/X3_local_state_refs.csv
Output: Markdown tables on stdout.
Only baseline rows are counted: kind == 'call', build == 'FULL', compiled == 'Y'.
"""
import csv
import collections
import sys

SUBSYS = [
    # (subsystem label, list of basenames)
    ("Core loop / rules", ["location.cpp", "multiwinia.cpp", "unit.cpp", "iframe.cpp", "level_file.cpp",
                           "taskmanager.cpp", "global_world.cpp"]),
    ("Units and entities", ["darwinian.cpp", "officer.cpp", "spirit.cpp", "entity.cpp", "engineer.cpp",
                            "armour.cpp", "insertion_squad.cpp", "harvester.cpp", "lasertrooper.cpp", "tank.cpp",
                            "shaman.cpp", "lander.cpp"]),
    ("Creatures", ["anthill.cpp", "armyant.cpp", "centipede.cpp", "souldestroyer.cpp", "spaceship.cpp",
                   "spider.cpp", "triffid.cpp", "tripod.cpp", "virii.cpp", "tentacle.cpp", "spam.cpp"]),
    ("Weapons and effects", ["weapons.cpp", "rocket.cpp", "mine.cpp", "lightning.cpp", "meteor.cpp", "snow.cpp"]),
    ("Crates and AI", ["crate.cpp", "ai.cpp", "aiobjective.cpp"]),
    ("Buildings", ["building.cpp", "bridge.cpp", "cave.cpp", "chess.cpp", "eruptionmarker.cpp", "factory.cpp",
                   "generator.cpp", "gunturret.cpp", "incubator.cpp", "laserfence.cpp", "multiwiniazone.cpp",
                   "radardish.cpp", "randomiser.cpp", "spawnpoint.cpp", "spiritreceiver.cpp", "spiritstore.cpp",
                   "sporegenerator.cpp", "statue.cpp", "teleport.cpp", "tree.cpp", "controltower.cpp",
                   "portal.cpp", "researchitem.cpp", "smokemarker.cpp"]),
]


def subsystem(path):
    base = path.split("/")[-1]
    for label, files in SUBSYS:
        if base in files:
            return label
    return "Other (presentation / platform)"


def base_rows(path):
    with open(path, newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    return [r for r in rows if r["kind"] == "call" and r["build"] == "FULL" and r["compiled"] == "Y"]


def ctx_bucket(c):
    if c.startswith("advance"):
        return "advance"
    if c.startswith("init"):
        return "init"
    if c.startswith("render"):
        return "render"
    return "other"


def main():
    sync = base_rows(sys.argv[1])
    unsync = base_rows(sys.argv[2])
    with open(sys.argv[3], newline="", encoding="utf-8") as fh:
        local = list(csv.DictReader(fh))

    gens = ["syncfrand", "syncsfrand", "syncrand", "FRAND", "SFRAND"]
    out = []
    out.append("**Table A — synced call sites per file (baseline [B:FULL], %d calls).** Columns: calls by generator; "
               "context of the enclosing function (advance = `*Advance*`/tick, init = constructor/`Initialise`/`Read`/"
               "`Create*`/`Spawn*`, other = other simulation functions); `cosm.` = draws flagged by the "
               "`cosmetic_hint` heuristic; liveness of the file's classes in Multiwinia." % len(sync))
    out.append("")
    out.append("| File | Subsystem | Liveness | Calls | " + " | ".join(gens) + " | advance | init | other | cosm. |")
    out.append("|---|---|---|---|" + "---|" * len(gens) + "---|---|---|---|")
    byfile = collections.defaultdict(list)
    for r in sync:
        byfile[r["file"]].append(r)
    for f in sorted(byfile):
        rows = byfile[f]
        g = collections.Counter(r["generator"] for r in rows)
        c = collections.Counter(ctx_bucket(r["context"]) for r in rows)
        live = collections.Counter(r["liveness"] for r in rows).most_common(1)[0][0]
        cos = sum(1 for r in rows if r["cosmetic_hint"])
        out.append("| `%s` | %s | %s | %d | %s | %d | %d | %d | %d |" % (
            f, subsystem(f), live, len(rows), " | ".join(str(g.get(x, 0)) for x in gens),
            c.get("advance", 0), c.get("init", 0), c.get("other", 0) + c.get("render", 0), cos))
    out.append("")

    out.append("**Table B — synced call sites rolled up by subsystem.**")
    out.append("")
    out.append("| Subsystem | Files | Calls | LIVE-file calls | MIXED-file calls | LEGACY-file calls |")
    out.append("|---|---|---|---|---|---|")
    agg = collections.OrderedDict()
    for label, _ in SUBSYS + [("Other (presentation / platform)", [])]:
        agg[label] = [set(), 0, 0, 0, 0]
    for r in sync:
        a = agg[subsystem(r["file"])]
        a[0].add(r["file"])
        a[1] += 1
        lv = r["liveness"]
        if lv == "LIVE":
            a[2] += 1
        elif lv == "MIXED":
            a[3] += 1
        else:
            a[4] += 1
    for label, a in agg.items():
        if a[1]:
            out.append("| %s | %d | %d | %d | %d | %d |" % (label, len(a[0]), a[1], a[2], a[3], a[4]))
    tot = [sum(len(a[0]) for a in agg.values()), len(sync)]
    out.append("| **Total** | %d | %d | %d | %d | %d |" % (
        tot[0], tot[1], sum(a[2] for a in agg.values()), sum(a[3] for a in agg.values()),
        sum(a[4] for a in agg.values())))
    out.append("")

    ugens = ["sfrand", "frand", "AppRandom", "rand", "AppSeedRandom"]
    out.append("**Table C — unsynced generator call sites per file (baseline compiled files, %d calls).** "
               "Context buckets as Table A plus `render`." % len(unsync))
    out.append("")
    out.append("| File | Calls | " + " | ".join(ugens) + " | advance | init | render | other |")
    out.append("|---|---|" + "---|" * len(ugens) + "---|---|---|---|")
    byfile = collections.defaultdict(list)
    for r in unsync:
        byfile[r["file"]].append(r)
    for f in sorted(byfile):
        rows = byfile[f]
        g = collections.Counter(r["generator"] for r in rows)
        c = collections.Counter(ctx_bucket(r["context"]) for r in rows)
        out.append("| `%s` | %d | %s | %d | %d | %d | %d |" % (
            f, len(rows), " | ".join(str(g.get(x, 0)) for x in ugens),
            c.get("advance", 0), c.get("init", 0), c.get("render", 0), c.get("other", 0)))
    out.append("")

    syms = collections.Counter(r["symbol"] for r in local)
    ctxs = sorted(set(r["context"] for r in local))
    out.append("**Table D — local-state references in simulation-adjacent code (%d rows of "
               "`X3_local_state_refs.csv`), symbol class × enclosing-function context.**" % len(local))
    out.append("")
    out.append("| Symbol class | Total | " + " | ".join(ctxs) + " |")
    out.append("|---|---|" + "---|" * len(ctxs))
    m = collections.Counter((r["symbol"], r["context"]) for r in local)
    for s, n in syms.most_common():
        out.append("| `%s` | %d | %s |" % (s, n, " | ".join(str(m.get((s, c), 0)) for c in ctxs)))
    out.append("")
    sys.stdout.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
