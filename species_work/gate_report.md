# Gate report

| Gate | Result | Failures | Summary |
|---|---|---|---|
| 1 Hard-fail patterns | PASS | 0 | 0 hard-fail hits |
| 2 Soft-flag patterns | PASS | 0 | 38 soft hits, 38 allowed by CONVENTIONS §9, 0 not allowed |
| 3 Links and anchors | PASS | 0 | 12173 relative links checked |
| 4 Mermaid | FAIL | 1 | SKIPPED (--skip-mermaid) |
| 5 Requirement ids | FAIL | 2 | 4918 requirement ids defined, 27036 citations, 25 prefixes registered |
| 6 DEC and ADR references | FAIL | 38 | 300 DECs (148 Open, 110 of them cited by a requirement), 64 ADRs |
| 7 Asset closure | PASS | 0 | 401 asset ids (1311 manifest rows), 2273 asset citations in reference and component documents |
| 8 Tuning keys | FAIL | 10 | 3869 tuning keys, 2 cited by no requirement |
| 9 Truncation words | PASS | 0 | 0 truncation words |
| 10 Sizes, READMEs, templates | PASS | 0 | 208 documents, 76 checked for template headings |
| 11 Requirements index | PASS | 0 | index not written (no --write-index) |

## Gate 4 Mermaid

- Mermaid gate skipped

## Gate 5 Requirement ids

- reference/07_bots/README.md:132 cites R-BLDG-510, which is not defined
- reference/10_audio_design/event_model_and_sound_data.md:123 cites R-META-120, which is not defined

## Gate 6 DEC and ADR references

- Open DEC-122 is not cited by any requirement
- Open DEC-123 is not cited by any requirement
- Open DEC-263 is not cited by any requirement
- Open DEC-264 is not cited by any requirement
- Open DEC-265 is not cited by any requirement
- Open DEC-266 is not cited by any requirement
- Open DEC-850 is not cited by any requirement
- Open DEC-851 is not cited by any requirement
- Open DEC-852 is not cited by any requirement
- Open DEC-853 is not cited by any requirement
- Open DEC-854 is not cited by any requirement
- Open DEC-855 is not cited by any requirement
- Open DEC-856 is not cited by any requirement
- Open DEC-858 is not cited by any requirement
- Open DEC-859 is not cited by any requirement
- Open DEC-860 is not cited by any requirement
- Open DEC-861 is not cited by any requirement
- Open DEC-862 is not cited by any requirement
- Open DEC-864 is not cited by any requirement
- Open DEC-865 is not cited by any requirement
- Open DEC-866 is not cited by any requirement
- Open DEC-867 is not cited by any requirement
- Open DEC-868 is not cited by any requirement
- Open DEC-869 is not cited by any requirement
- Open DEC-870 is not cited by any requirement
- Open DEC-952 is not cited by any requirement
- Open DEC-954 is not cited by any requirement
- Open DEC-956 is not cited by any requirement
- Open DEC-958 is not cited by any requirement
- Open DEC-959 is not cited by any requirement
- Open DEC-962 is not cited by any requirement
- Open DEC-963 is not cited by any requirement
- Open DEC-964 is not cited by any requirement
- Open DEC-965 is not cited by any requirement
- Open DEC-966 is not cited by any requirement
- Open DEC-969 is not cited by any requirement
- Open DEC-971 is not cited by any requirement
- Open DEC-972 is not cited by any requirement

## Gate 8 Tuning keys

- tuning key ref.unit.trooper.health defined 2 times: reference/02_units/README.md:462, reference/02_units/strike_team.md:192
- tuning key ref.unit.trooper.speed defined 2 times: reference/02_units/README.md:463, reference/02_units/strike_team.md:193
- tuning key ref.unit.transport.health defined 2 times: reference/02_units/README.md:466, reference/02_units/transport.md:248
- tuning key ref.unit.collector.health defined 2 times: reference/02_units/README.md:469, reference/02_units/collector.md:180
- tuning key ref.unit.collector.speed defined 2 times: reference/02_units/README.md:470, reference/02_units/collector.md:181
- tuning key ref.unit.builder.health defined 2 times: reference/02_units/README.md:472, reference/02_units/builder.md:136
- tuning key ref.unit.builder.speed defined 2 times: reference/02_units/README.md:473, reference/02_units/builder.md:137
- tuning key ref.unit.trooper.slope_divisor defined 2 times: reference/02_units/README.md:531, reference/02_units/strike_team.md:196
- tuning key ref.bot.marker.phase_ticks (reference/07_bots/05_mode_strategies_and_tutorials.md:205) is cited by no requirement
- tuning key ref.bot.marker.scan_range_default (reference/07_bots/05_mode_strategies_and_tutorials.md:206) is cited by no requirement
