# Design gate report

Design root: `design_next`

| Gate | Result | Failures | Summary |
|---|---|---|---|
| 1 Hard-fail patterns | PASS | 0 | 0 hard-fail hits |
| 2 Soft-flag patterns | PASS | 0 | 43 soft hits, 43 allowed by CONVENTIONS §9, 0 not allowed |
| 3 Links and anchors | PASS | 0 | 13423 relative links checked |
| 4 Mermaid | SKIP | 0 | skipped (--skip-mermaid) |
| 5 Requirement ids | FAIL | 1 | 4947 requirement ids defined, 28806 citations, 25 prefixes registered |
| 6 DEC and ADR references | PASS | 0 | 301 DECs (148 Open, 148 of them cited by a requirement), 64 ADRs |
| 7 Asset closure | PASS | 0 | 452 asset ids (1411 manifest rows), 2544 asset citations in reference and component documents |
| 8 Tuning keys | PASS | 0 | 3690 tuning keys, 0 cited by no requirement |
| 9 Truncation words | PASS | 0 | 0 truncation words |
| 10 Sizes, READMEs, templates | PASS | 0 | 221 documents, 83 checked for template headings |
| 11 Requirements index | PASS | 0 | REQUIREMENTS_INDEX.md and 25 index pages written (largest 100 KB) |

## Gate 5 Requirement ids

- requirements_index/bld.md:25 cites BLD-017, which is not defined
