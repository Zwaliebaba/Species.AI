import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
edits = [
 ("- DEC-079 — per-team population cap",
  "- DEC-045 — map editor; the cap value controls are an editor hook (R-ECON-061).\n- DEC-079 — per-team population cap"),
 ("Resolved decisions applied in §2: DEC-002, DEC-004, DEC-005, DEC-006, DEC-007, DEC-011, DEC-013, DEC-014, DEC-015, DEC-016, DEC-018, DEC-021, DEC-033, DEC-034, DEC-035, DEC-039, DEC-065, DEC-073, DEC-088, DEC-092, DEC-103, DEC-117, DEC-456, DEC-457, DEC-652 and DEC-801.",
  "Resolved decisions applied in §2: DEC-002, DEC-004, DEC-005, DEC-006, DEC-007, DEC-011, DEC-013, DEC-014, DEC-015, DEC-016, DEC-018, DEC-021, DEC-025, DEC-033, DEC-034, DEC-035, DEC-036, DEC-039, DEC-065, DEC-073, DEC-082, DEC-088, DEC-092, DEC-103, DEC-117, DEC-456, DEC-457, DEC-464, DEC-652 and DEC-801."),
 ("Every tick count in this document, including the uniform tick ranges, is part of the rule.",
  "Every tick count in this document, including the uniform tick ranges, is part of the rule, and every kept quirk is stated as an explicit rule with a named key (see DEC-006)."),
 ("Values owned by another reference document are repeated here for the population-flow view and marked \"owner\" in the notes; the owning document's value governs.",
  "Every value is a reference tuning value that Species may adopt as its baseline (DEC-004). Values owned by another reference document are repeated here for the population-flow view and marked \"owner\" in the notes; the owning document's value governs."),
]
for old, new in edits:
    n = t.count(old)
    if n != 1:
        print('MISS', n, old[:70]); continue
    t = t.replace(old, new)
p.write_text(t, encoding='utf-8')
print('done')
