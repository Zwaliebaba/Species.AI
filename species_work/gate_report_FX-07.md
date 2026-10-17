# Gate report

| Gate | Result | Failures | Summary |
|---|---|---|---|
| 1 Hard-fail patterns | PASS | 0 | 0 hard-fail hits |
| 2 Soft-flag patterns | FAIL | 1 | 36 soft hits, 35 allowed by CONVENTIONS §9, 1 not allowed |
| 3 Links and anchors | PASS | 0 | 12634 relative links checked |
| 4 Mermaid | FAIL | 1 | SKIPPED (--skip-mermaid) |
| 5 Requirement ids | FAIL | 1 | 4928 requirement ids defined, 27651 citations, 25 prefixes registered |
| 6 DEC and ADR references | FAIL | 12 | 301 DECs (148 Open, 136 of them cited by a requirement), 64 ADRs |
| 7 Asset closure | PASS | 0 | 453 asset ids (1412 manifest rows), 2462 asset citations in reference and component documents |
| 8 Tuning keys | FAIL | 2 | 3738 tuning keys, 2 cited by no requirement |
| 9 Truncation words | PASS | 0 | 0 truncation words |
| 10 Sizes, READMEs, templates | PASS | 0 | 208 documents, 76 checked for template headings |
| 11 Requirements index | PASS | 0 | index not written (no --write-index) |

## Gate 2 Soft-flag patterns

- reference/03_buildings_and_world_objects/README.md:388 [history] port of

## Gate 4 Mermaid

- Mermaid gate skipped

## Gate 5 Requirement ids

- reference/07_bots/README.md:132 cites R-BLDG-510, which is not defined

## Gate 6 DEC and ADR references

- Open DEC-850 is not cited by any requirement
- Open DEC-861 is not cited by any requirement
- Open DEC-864 is not cited by any requirement
- Open DEC-870 is not cited by any requirement
- Open DEC-952 is not cited by any requirement
- Open DEC-954 is not cited by any requirement
- Open DEC-956 is not cited by any requirement
- Open DEC-958 is not cited by any requirement
- Open DEC-962 is not cited by any requirement
- Open DEC-965 is not cited by any requirement
- Open DEC-966 is not cited by any requirement
- Open DEC-971 is not cited by any requirement

## Gate 8 Tuning keys

- tuning key ref.bot.marker.phase_ticks (reference/07_bots/05_mode_strategies_and_tutorials.md:205) is cited by no requirement
- tuning key ref.bot.marker.scan_range_default (reference/07_bots/05_mode_strategies_and_tutorials.md:206) is cited by no requirement
