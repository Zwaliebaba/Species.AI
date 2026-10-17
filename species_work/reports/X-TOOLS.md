# X-TOOLS report: retained gate tooling (prompt §9.4, owner decision O27)

Status: done. No git write commands and no publishing or upload tools were used. The coordinator's crash-safety
commits (8864211 onwards) already include the files below; `node_modules/` is not in the index (it is ignored by
`.gitignore` line 286, `node_modules/`, checked with `git check-ignore`).

## 1 Files written

| File | Bytes | What it is |
|---|---|---|
| `Tools/DesignCheck/CheckDesign.py` | 36 894 | the gates of prompt §7.3 items 1–11, generalised; CLI `CheckDesign.py <design root> [--write-index] [--report <file>] [--skip-mermaid]` |
| `Tools/DesignCheck/TechPatterns.tsv` | 7 523 | 105 plain-text patterns (54 hard: provenance 17, superseded-tech 37; 51 soft: history 12, scoped 6, unfinished 3, renamed-term 30); columns `pattern flags gate class note` |
| `Tools/DesignCheck/IpDenylist.sha256` | 35 764 | 545 SHA-256 digests (546 list lines; two normalise to the same phrase) |
| `Tools/DesignCheck/OriginalStrings.sha256` | 29 393 | 447 SHA-256 digests, one per line of `original_strings.txt` |
| `Tools/DesignCheck/BuildDenylist.py` | 4 534 | hashes a plain-text list into the two digest files (`--names`, `--strings`, merge or `--replace`); never prints an entry |
| `Tools/DesignCheck/SelfTest.py` | 9 288 | `SelfTest.py <names list> <strings list>`: sync, names, strings, clean sample, leak check |
| `Tools/DesignCheck/MermaidCheck.js` | 3 944 | gate 4 driver: recursive, Playwright resolved beside it, Chromium located by Playwright (`PLAYWRIGHT_BROWSERS_PATH` honoured) |
| `Tools/DesignCheck/package.json` | 306 | pins `mermaid` 11.4.1 and `playwright` 1.56.1 exactly |
| `Tools/DesignCheck/package-lock.json` | 55 854 | lockfile v3, 143 packages, every one resolved from registry.npmjs.org; `npm ci --prefix Tools/DesignCheck` reinstalls from it cleanly |
| `Tools/DesignCheck/README.md` | 8 626 | usage, gates, files, the hashed denylists, extending, self-test, updating the pinned Mermaid build |
| `Tools/DesignModels/TerrainGenerator.py` | 27 396 | the terrain conformance generator, with a `check` mode over every vector set of the specification |
| `Tools/DesignModels/README.md` | 2 331 | what the generator is, how to run it, and that the design phase adds its models here |

Helper scripts (working files, `python3 -I`, paths as arguments): `species_work/scripts/X-TOOLS_build_patterns.py`
(builds `TechPatterns.tsv`), `X-TOOLS_compare_gates.py` (compares two gate reports gate by gate, including the failure
sets), `X-TOOLS_compare_patterns.py` (compares the retained scans with the run's pattern scans on a corpus with hits),
`X-TOOLS_fixture.py` (builds a defect fixture), `X-TOOLS_probe.py`, `X-TOOLS_view_trial.py`, `X-TOOLS_brand_misses.py`
(normalisation and recall trials, console output only).

## 2 Gate equality on design_next

Snapshot: `design_next/`, `vision/` and `prompts/` copied to the scratchpad at 22:16:54 UTC, at HEAD a580437 with no
uncommitted change under `design_next/`; both checkers run back to back on that copy, with the same pinned Mermaid
11.4.1 build (`Tools/DesignCheck/node_modules/mermaid/dist/mermaid.min.js`). Both exit 0.
`X-TOOLS_compare_gates.py` finds all eleven gates equal: result, failure count, summary line and set of failure lines.

`species_work/scripts/check_design.py`:

```
gate  1 Hard-fail patterns           PASS     0  0 hard-fail hits
gate  2 Soft-flag patterns           PASS     0  43 soft hits, 43 allowed by CONVENTIONS §9, 0 not allowed
gate  3 Links and anchors            PASS     0  13292 relative links checked
gate  4 Mermaid                      PASS     0  blocks=266 failed=0
gate  5 Requirement ids              PASS     0  4942 requirement ids defined, 28416 citations, 25 prefixes registered
gate  6 DEC and ADR references       PASS     0  301 DECs (148 Open, 148 of them cited by a requirement), 64 ADRs
gate  7 Asset closure                PASS     0  453 asset ids (1412 manifest rows), 2622 asset citations in reference and component documents
gate  8 Tuning keys                  PASS     0  3710 tuning keys, 0 cited by no requirement
gate  9 Truncation words             PASS     0  0 truncation words
gate 10 Sizes, READMEs, templates    PASS     0  220 documents, 83 checked for template headings
gate 11 Requirements index           PASS     0  index not written (no --write-index)
```

`python3 Tools/DesignCheck/CheckDesign.py <snapshot>/design_next`:

```
gate  1 Hard-fail patterns           PASS     0  0 hard-fail hits
gate  2 Soft-flag patterns           PASS     0  43 soft hits, 43 allowed by CONVENTIONS §9, 0 not allowed
gate  3 Links and anchors            PASS     0  13292 relative links checked
gate  4 Mermaid                      PASS     0  blocks=266 failed=0
gate  5 Requirement ids              PASS     0  4942 requirement ids defined, 28416 citations, 25 prefixes registered
gate  6 DEC and ADR references       PASS     0  301 DECs (148 Open, 148 of them cited by a requirement), 64 ADRs
gate  7 Asset closure                PASS     0  453 asset ids (1412 manifest rows), 2622 asset citations in reference and component documents
gate  8 Tuning keys                  PASS     0  3710 tuning keys, 0 cited by no requirement
gate  9 Truncation words             PASS     0  0 truncation words
gate 10 Sizes, READMEs, templates    PASS     0  220 documents, 83 checked for template headings
gate 11 Requirements index           PASS     0  index not written (no --write-index)
```

An earlier snapshot (21:45:34 UTC, during wave B) was also equal on all eleven gates, including the identical 741
gate-3 failure lines that an in-progress folder split caused at the time. Run time on design_next: about 20 s for
`CheckDesign.py` against about 47 s for the run's checker.

`--write-index` was run by both tools on two copies of the same snapshot: `REQUIREMENTS_INDEX.md` and the 26 index pages
are identical except the "Generated by" line, which now names `Tools/DesignCheck/CheckDesign.py --write-index`.

## 3 Failure paths

**Defect fixture** (`X-TOOLS_fixture.py`: the snapshot plus one planted defect per gate; the planted name and string
were drawn from the plain-text lists at run time and written only to the scratch copy). Both tools fail the same
gates with the same counts and the same file and line for every planted defect: gate 1 (a listed name, a superseded
graphics API, a source path), 2 (a soft word not allowed, a listed interface string), 3 (missing file, missing anchor),
4 (an unparsable diagram), 5 (duplicate id, unregistered prefix), 6 (unknown DEC and ADR), 7 (asset not in the
manifest), 8 (uncited key), 9 (truncation word), 10 (folder without README, file over 120 KB). Two differences, both
by design:

- gate 1 prints the normalised phrase of a denylist hit (lower case, punctuation removed) where the run prints the text
  as written; file, line and count are the same;
- gate 3: the retained tool lets a link reach the `vision/` and `prompts/` folders beside the design root (the
  generalisation the brief asks for), where the run allows `vision/` and `prompts/species-design-phase.md` only. Until
  the cutover, a link from the design set to the conversion prompt passes the retained gate 3 and fails the run's; after
  the cutover such a link is a broken link and fails both. No document links into `prompts/` today.

**Gate 4 setup** (minimal synthetic design root): all gates pass, exit 0; `--skip-mermaid` reports `SKIP` with exit 0
(the run's checker counts a skip as a failure; the brief's contract is that the gate is skipped only with the flag);
`PLAYWRIGHT_BROWSERS_PATH` pointing at an empty folder gives a gate 4 failure "Chromium did not start through
Playwright: …", exit 1; a copy of the tool without `node_modules/` gives "pinned Mermaid build missing … (run npm ci …,
or pass --skip-mermaid)", exit 1; a design root that does not exist gives exit 2.

## 4 The scans on a corpus with hits

`X-TOOLS_compare_patterns.py` ran the retained scans and the run's pattern scans over the source corpus `design/`
(22 MB, every class of hit present) and over the snapshot. It counts distinct line-and-match pairs, so the
snapshot's 40 soft pairs are gate 2's 43 hits:

| Scan | Source corpus `design/` | design_next snapshot |
|---|---|---|
| hard technology and provenance rows: run / retained / lines that differ | 186 933 / 186 933 / 0 | 0 / 0 / 0 |
| soft rows: run / retained | 21 564 / 21 521 (all 43 differences are the four members moved to the denylist) | 40 / 40 |
| quoted interface strings: run `@strings` / `OriginalStrings.sha256` / differ | 1 694 / 1 694 / 0 | 0 / 0 / 0 |
| lines flagged for names: by both tools | 48 293 | 0 |
| lines flagged only by the run's IP rows | 29 597 (identifier class 27 151, brand 2 851, string-key 470, map-name 0; a line can carry several classes) | 0 |
| lines flagged only by the denylist | 917 (listed names in forms the run's regexes miss: two-word and glued spellings, plurals, prefixed identifiers) | 0 |

### 4.1 What the retained gate 1 does not catch

Of the 77 890 source-corpus lines that the run's IP rows flag, the retained gate 1 flags 48 293 (62 %). The rest is the
coverage that the brief's design leaves to the run (item 5 of section 5): mostly code identifiers of the reference without an
IP stem, which `ip_tokens.txt` does not list, and brand-class tokens that F1 kept out of the list because they are
also English words or generic phrases. Every listed entry is found (SelfTest), and the reference library itself is
clean under both tools.

## 5 Design decisions and deviations

1. **Denylist normalisation.** Brief: lower case, ASCII-folded, punctuation removed, whitespace collapsed. Applied as
   written, with two refinements found by testing on design_next, each needed for the equality of gate 1:
   - hyphens and Unicode dashes separate words, as whitespace does. Removing them glued a hyphenated genre term in one
     zone-control reference document into a listed identifier form (1 false hit);
   - an exclamation mark stays part of its word. One listed two-word interface string ends in one; without it, its
     words form an everyday phrase that the design set uses 109 times.
   F1's report proposed keeping all punctuation inside a token; that misses a listed name followed by a full stop or a
   possessive, so the brief's removal is kept and only the two cases above differ.
2. **Three text views and affixes** (README "The hashed denylists"): the canonical view; a joined view split at every
   punctuation mark except the underscore (a listed file stem before its extension, an identifier before `::`); a split
   view at every non-alphanumeric character and lower-to-upper case step; and every prefix and suffix of six or more
   characters of each longer joined word. On design_next every view and the affixes give 0 hits. On the source corpus
   they raise the lines flagged by both tools from 43 324 (canonical and split views only) to 48 293, and cut the
   run-only lines from 34 566 to 29 597; every affix match there was a listed name in a derived form (plural, prefixed or suffixed identifier, level-file variant).
3. **TechPatterns.tsv.** The rows are those of the classes the brief names; 30 patterns are rewritten into equivalent
   regular expressions that do not match their own text (for example a literal written `Foo(?:Bar)`), because the run's
   hard scan of `Tools/` must print nothing and several superseded-technology patterns are literals. Equivalence is
   shown on the source corpus: identical hit lists (section 4). The class `legacy-tech` is renamed `superseded-tech`
   (the class name itself was a soft-flag word); notes are rewritten to cite nothing that the cutover removes.
4. **Four renamed-term members moved to the denylist.** The last renamed-term row held four names that are also lines
   of `ip_tokens.txt`; they are dropped from `TechPatterns.tsv` (no IP name in plain text) and caught by
   `IpDenylist.sha256` instead. One of them was already hard-fail in the run's list (brand class); the other three were
   soft only there and are hard-fail in the retained tool. design_next has none of the four.
5. **Coverage the retained gate 1 does not carry** (by the brief's design: only `ip_tokens.txt` is hashed): the run's
   1 063 identifier patterns (code identifiers of the reference, case-sensitive and code-form only, which hashing cannot express
   and F1 left out of the list unless they carry an IP stem), the 20 generic map phrases F1 deliberately left out, and
   capitalised proper-noun patterns that are also English words. See section 4.1 for the line counts. If the owner wants
   more of it retained, the coordinator can add lines with `BuildDenylist.py --names <file>` (merge) before the cutover.
6. **`--skip-mermaid` skips** (status `SKIP`, exit 0 when everything else passes); without it a missing build or
   Chromium is a gate 4 failure. The run's checker records a skip as a failure.
7. **`CONVENTIONS.md` is always allowed for its own §9 terms**, as §9 states ("this file is always among them"); every
   §9 row lists it today, so this changes no result.
8. **`BuildDenylist.py` added** (not in the brief's list): without it the digest files could not be rebuilt or extended
   after the cutover, which the README's "how to extend" needs.
9. **SelfTest leak check** reads `Tools/DesignCheck/` and `Tools/DesignModels/` with the hashed and hard scans; it skips
   `node_modules/` and the npm-generated `package-lock.json`, whose base64 integrity digests contain a listed two-letter
   abbreviation by chance (six hits, all inside digests).
10. **Line endings:** every file is LF. `.editorconfig` names LF for Python and Markdown; for `.js`, `.json`, `.tsv` and
    `.sha256` its default is CRLF, but every non-C++ file of the repository is LF in the work tree and `* text=auto`
    stores LF, so LF is kept (npm also writes LF).
11. **Short listed names are recoverable from their digests** by hashing a dictionary; that is inherent in a hashed
    denylist of short tokens and is not a leak of the plain text.
12. **TerrainGenerator.py** keeps the arithmetic of `C-TER_generator.py` unchanged (its `full` output is identical value
    for value) and adds a `check` mode: 16 tests, T1 to T3 and T7 to T13 of the terrain README §7.1, against every value
    of tables 7.2-B to 7.2-H by exact equality, all pass. Set D (table 7.2-H, flatten areas), which the source generator
    had no mode for, is now computed and passes. The exploratory position-y candidates of the source's `traceB` mode were
    dropped. It cites `Design/architecture/components/terrain/` (README §4.1 and §7, `height_field_generation.md` §4.2 to
    §4.9 and §7.2, `edges_normals_colours_and_sea.md` §4.10 and §4.11) and the requirement ids it implements.

## 6 Checks run

| Check | Result |
|---|---|
| `CheckDesign.py` against `check_design.py` on the same snapshot | all eleven gates equal (section 2) |
| `SelfTest.py species_work/ip_tokens.txt species_work/original_strings.txt` | PASS: digests in sync (545 and 447); 546 names found in all 3 936 embeddings; 447 strings in all 2 235 embeddings; Species sample 0 hits; leak check 0 hits |
| `SelfTest.py` with one list line added and one removed | FAIL as expected (sync and names), exit 1, no entry printed |
| `scan_patterns.py forbidden_patterns.tsv Tools/` (node_modules excluded) | prints nothing |
| `scan_patterns.py soft_patterns.tsv Tools/` (node_modules excluded) | prints nothing |
| `TerrainGenerator.py check` | 16 of 16 tests pass |
| `npm ci --prefix Tools/DesignCheck`; `npx --prefix Tools/DesignCheck playwright --version` | 142 packages installed from the lock file; Version 1.56.1 (`playwright install` was not run here; Chromium 1194 is the pre-installed build Playwright 1.56.1 resolves) |
| Python syntax at 3.7 and 3.8 (`ast.parse(feature_version=…)`) | all four Python files parse; the README states Python 3.8 or later |
| Lines over 120 columns in the Python and JavaScript files | none |

## 7 Cross-document requests and notes for the coordinator

- **Cutover:** run `SelfTest.py` with `ip_tokens.txt` and `original_strings.txt` before they are deleted; afterwards the
  digest files are the only record. `build.yml`'s `design` job already runs exactly the three README commands.
- **Prompt §1 item 1** still names `tools/design-check/`; the prompt is removed at the cutover, so nothing to change.
- **TechPatterns row 2** (`tools/(?!design-check|design-models)`) keeps the run's exemptions, which are now vestigial
  (the tooling lives under `Tools/`, which the case-sensitive pattern does not match); kept for equality.
- Nothing was dropped; no DEC or ADR requests; no names needed beyond the ledger.
- The helper `X-TOOLS_build_patterns.py` holds the run's literal superseded-technology and provenance patterns (its
  rewrite table maps them to the retained forms), so the run's hard scan flags it, as it flags F1's pattern builders;
  it is a working file and is removed with `species_work/` at the cutover. The report itself scans clean on the hard
  list; its one soft hit is the run's class name `legacy-tech`, quoted in §5 item 3.
