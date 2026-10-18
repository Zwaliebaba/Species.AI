"""V2 helper: final sweep of the clean library for three kinds of hit.

Usage: python3 -I species_work/scripts/V2_sweep.py <design root> <kind>
  kind = internals | tools | numbers
Read-only. Skips the generated index (requirements_index/ and REQUIREMENTS_INDEX.md).
Prints file:line and the matching text; the reviewer judges each hit.
"""

import os
import re
import sys

INTERNALS = [
    ("packet id", r"\bP6-[A-Z0-9]+"),
    ("reviewer id", r"\bR[1-4][ab]\b|\bR[1-4][ab]-\d+"),
    ("RA/RB id", r"\bR[AB]-\d+"),
    ("fixer id", r"\bFX-\d+"),
    ("agent id", r"\bX-(?:DEC|ADR|AST|GLO|ROUTE|ARB|TOOLS)\b"),
    ("verifier id", r"\bV[12]\b"),
    ("ledger", r"\bledger\b"),
    ("routing", r"\brouting\b"),
    ("ORIG", r"\bORIG\b"),
    ("coordinator", r"\bcoordinator\b"),
    ("phase N", r"\b[Pp]hase\s+\d\b"),
    ("wave N", r"\b[Ww]ave\s+[12]\b(?!\s*(?:at|runs|gives|is|of|and|,))"),
    ("gate N", r"\b[Gg]ate\s+(?:G?\d{1,2})\b"),
    ("G-gate", r"\bG\d{1,2}\b"),
    ("conversion run", r"\bconversion run\b|\bthe run's\b|\bfix packet|\breviewer\b|\bfixer\b"),
]

TOOLS = [
    ("tools solution", r"\btools? solution\b"),
    ("separate solution", r"\bseparate solution\b|\bsecond solution\b|\bown solution\b|\bsolution of (?:its|their) own\b"),
    ("glTF validator", r"[Gg][Ll][Tt][Ff][ _-]?[Vv]alidator|Khronos"),
    ("download", r"\bdownload"),
    ("MSBuild task", r"DownloadFile|<Exec\b|\bExec task|Invoke-WebRequest|\bcurl\b|\bwget\b|\bnpm\b|Node\.js|\bnode\b"),
]

NUMBERS = [
    ("population cap", r"(?i)population cap.{0,80}(?:÷|divided|per team|player count|players|open slots|used slots)|(?:÷|divided).{0,40}(?:player count|number of players|used slots)"),
    ("Transport cap", r"(?i)transport cap|transport_cap"),
    ("speed hold", r"(?i)crate_hold|\bhold(?:s)? (?:for|of|lasts)\b.{0,40}tick|\b15[01]\b.{0,30}(?:world )?ticks?.{0,40}(?:hold|Dilation|Rush)|(?:Dilation|Rush).{0,80}\b1[45]\d?\b"),
    ("Strike team life", r"(?i)strike.team.{0,120}\b(?:600|601|60 s|60\.1)\b|life_ticks.{0,40}strike|strike_team\.life"),
    ("DSP kind count", r"(?i)\b(?:nine|9|fourteen|14|thirteen|13|ten|10)\b.{0,30}(?:DSP|effect kinds|filter kinds|kinds and)|(?:DSP|effect kinds|filter kinds).{0,30}\b(?:nine|fourteen|14|9)\b|\bof the (?:35|36|63) param"),
]


def files(root):
    for base, dirs, names in os.walk(root):
        dirs[:] = [d for d in dirs if d != "requirements_index"]
        for name in sorted(names):
            if name == "REQUIREMENTS_INDEX.md":
                continue
            if name.endswith((".md", ".csv", ".tsv", ".json", ".txt")):
                yield os.path.join(base, name)


def main():
    root, kind = sys.argv[1], sys.argv[2]
    table = {"internals": INTERNALS, "tools": TOOLS, "numbers": NUMBERS}[kind]
    pats = [(label, re.compile(rx)) for label, rx in table]
    count = 0
    for path in files(root):
        rel = os.path.relpath(path, root)
        with open(path, encoding="utf-8") as f:
            for no, line in enumerate(f, 1):
                for label, pat in pats:
                    m = pat.search(line)
                    if m:
                        count += 1
                        start = max(0, m.start() - 90)
                        print("%-17s %s:%d: …%s…" % (label, rel, no, line[start:m.end() + 90].strip()))
    print("hits: %d" % count)


if __name__ == "__main__":
    main()
