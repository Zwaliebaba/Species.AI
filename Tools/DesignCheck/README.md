# DesignCheck

The design gate checker. It runs every structural gate over the design set (`Design/`) and must pass before a design
change is pushed (`AGENTS.md` §6). The `design` job of `.github/workflows/build.yml` runs it on every pull request. It
is a development tool: nothing here ships or links into the executable (`AGENTS.md` R14).

## Usage

```
npm ci --prefix Tools/DesignCheck
npx --prefix Tools/DesignCheck playwright install --with-deps chromium
python3 Tools/DesignCheck/CheckDesign.py Design
```

The first two commands install the pinned Mermaid build and headless Chromium once (on Windows, or where the system
libraries are already present, `playwright install chromium` without `--with-deps` is enough). The third runs the
gates and prints one line per gate:

```
gate  1 Hard-fail patterns           PASS     0  0 hard-fail hits
gate  2 Soft-flag patterns           PASS     0  39 soft hits, 39 allowed by CONVENTIONS §9, 0 not allowed
...
```

| Option | Effect |
|---|---|
| `<design root>` | the folder to check, for example `Design` |
| `--report <file>` | also writes the gate table and every failure, with file and line, as Markdown |
| `--write-index` | writes `REQUIREMENTS_INDEX.md` and `requirements_index/` at the design root (gate 11) |
| `--skip-mermaid` | skips gate 4; without it, gate 4 fails when the Mermaid build or Chromium is missing |
| `--design-phase` | also runs the design-phase gates 12 to 19 ([below](#the-design-phase-gates)); gates 1 to 11 behave exactly as without it |
| `--final` | the end of the design phase: every finding the design-phase gates report becomes a failure; implies `--design-phase` |

Exit status: 0 when every gate passes (or is skipped with `--skip-mermaid`), 1 when any gate fails, 2 on a usage error.
Python 3.8 or later with the standard library only; Node.js for gate 4. Chromium is found through Playwright, which
honours `PLAYWRIGHT_BROWSERS_PATH`.

## The gates

| Gate | Checks |
|---|---|
| 1 Hard-fail patterns | the `hard` rows of `TechPatterns.tsv` and the hashed IP denylist `IpDenylist.sha256`: zero hits in any file under the design root |
| 2 Soft-flag patterns | the `soft` rows of `TechPatterns.tsv`, and quoted strings of three or more words against `OriginalStrings.sha256`: every hit is allowed for its path by `CONVENTIONS.md` §9 |
| 3 Links and anchors | every relative link resolves and its anchor exists (GitHub heading slugs and `<a id>`); a link leaves the design root only for the `vision/` and `prompts/` folders beside it |
| 4 Mermaid | every `mermaid` block parses with `mermaid.parse()` of the pinned build, in headless Chromium (`MermaidCheck.js`) |
| 5 Requirement ids | every id has a prefix registered in `CONVENTIONS.md` §3.1, is defined exactly once (`**PREFIX-NNN**`), and every cited id exists |
| 6 DEC and ADR references | every cited `DEC-NNN` or `DEC-NNNN` (the design phase numbers from DEC-1000) exists in `decisions/` and every `ADR-NNN` in `ADR/`; every Open DEC is cited inside at least one requirement |
| 7 Asset closure | every asset id cited in `reference/` or `architecture/components/` exists in `assets/asset_manifest.csv`, every manifest row names a consumer, and asset stems share no prefix with tuning keys or string ids |
| 8 Tuning keys | every key of a tuning table (`key \| value \| ...`) is defined once and is cited by a requirement |
| 9 Truncation words | none of the words that stand in for a list, outside code |
| 10 Sizes, READMEs, templates | no file over 120 KB (`assets/*.csv` and `assets/GameAssets.md` exempt), a `README.md` in every folder, and the template headings in every reference and component document |
| 11 Requirements index | with `--write-index`, the generated index of every requirement id |

The prefix registry (`CONVENTIONS.md` §3.1) and the allowed terms (§9) are read from the design set itself, so a new
prefix or allowed term needs no change here.

## The design-phase gates

`--design-phase` adds the checks of `prompts/species-design-phase.md` §7 (with §5.3, §5.4 and §6), numbered after gate
11, each with one summary line in the same table. Some findings are **reported** rather than failed while the design
phase works: they are listed under "reported" in the `--report` file, counted in the summary line, and become failures
with `--final`. A gate with nothing to check yet (no `gdd/` folder, no rationale files) passes and says so, so the
option runs from Phase 0; the ledger gate runs as soon as `gdd/adoption_ledger/` exists.

| Gate | Checks | Reported (a failure with `--final`) |
|---|---|---|
| 12 Adoption ledger | every table `Scope \| Verdict \| Species requirements \| Note` of `gdd/adoption_ledger/*.md` (README excluded). A scope is a reference file relative to `reference/` (`02_units/worker.md`), that file and a heading number (`02_units/worker.md §2.5`: the numbered heading and every heading under it), or one `R-` id. Every `R-` id defined in `reference/` belongs to the nearest numbered heading that encloses its definition, and resolves to exactly one effective verdict, the most specific row winning: an `R-` id row, then the deepest heading, then the file. Fails on: an id with no verdict; two rows of the same specificity covering one id; a scope that matches no `R-` id (no such file, no such heading, an id not defined); an unknown verdict (`adopt`, `adopt-skirmish`, `adapt`, `drop`, `pending`); an `adapt` row that names no requirement id or one that is not defined; a `drop` row with an empty note; a ledger file without a ledger table. Prints the totals per verdict | every `pending` row; every `R-` id that the text of an `adopt` or `adopt-skirmish` requirement cites and that is dropped by a row whose Species requirements column names no defined replacement; with `--final`, a missing `gdd/adoption_ledger/` |
| 13 Design-phase decisions | counts the Open decisions of `decisions/` and their `Decide by:` values | every Open decision whose `Decide by:` starts with "design phase" |
| 14 GDD and component templates | every GDD file (every `.md` under `gdd/` except `README.md` files and the ledger; a GDD split into a folder carries the template in each file, as gate 10 asks of reference files) has the nine numbered headings of the GDD template (`## 1 Purpose and player experience` to `## 9 Open decisions`) and a `> Status:` line naming `Rationale:`; every file of a new component (any component under `architecture/components/` other than the 12 baseline components, `README.md` files excluded) has `## 9 Scale path`. Gate 10 already checks the component template headings | — |
| 15 Rationale files | every GDD file and new component names, after `Rationale:` in its status line (a path or a link), a file under `rationale/` that exists; in every rationale file, every row of a findings table (a table with a `Severity` and a `Resolution` column) whose severity is Critical or High has a resolution that is not empty or a dash | — |
| 16 Owner decisions | every `O<number>` cited in the design set (a word-bounded `O` and one to three digits, outside code spans and code blocks, `O(…)` forms aside; both ends and every number of a range such as `O1–O30`) is an entry of `decisions/owner_decisions.md`: a table row whose first cell is `O<n>`, or a `### O<n>` heading | — |
| 17 Tuning status | every row of a tuning table (`key \| value \| ...`) in a GDD file or a new component has a `status` cell of `final` or `provisional`, and every provisional row a non-empty `playtest note` cell; a table without a status column fails | — |
| 18 Models | the rationale file of every "model required" area (A1, A2, B2, B4, B5, B6; a rationale file is named for its area, `rationale/<area>.md` or `rationale/<area>_<slug>.md`, any case) has a `## Model results` section (numbered or not) that names `Tools/DesignModels/<Model>.py`, and every script it names exists | a model-required area without a rationale file |
| 19 Repository scan | the `hard` rows of `TechPatterns.tsv` and the hashed IP denylist over the whole repository (the parent of the design root), outside `.git/`, every `node_modules/` and `Tools/DesignCheck/package-lock.json`; the hits inside the design root are gate 1's, not scanned twice. A hit on a line that `ScanAllow.tsv` allows for its path is skipped; every other hit fails, and so does an allow-list row that allows no hit (the line is gone or no longer matches) or is malformed | — |

**The section rule is shared.** `reference_requirements()` in `CheckDesign.py` returns every `R-` id with its file and
section; a script that generates or extends the ledger imports it, so the ledger and gate 12 agree on the sections.

**`ScanAllow.tsv`** is TAB-separated with a header row and the columns `path` (relative to the repository root, with
`/` separators), `sha256` and `reason`. The digest is the SHA-256 of the exact line text without its line terminator
(LF or CRLF), in UTF-8: a digest rather than the text, so that the allow-list never matches a pattern itself. It holds
the three stock Visual Studio template lines of `.gitignore` and `.gitattributes` that the file-extension and
source-path patterns match; they name nothing of the reference product. To allow a line, print its digest, for example
`python3 -I -c "import hashlib,sys; print(hashlib.sha256(sys.argv[1].encode()).hexdigest())" '<line>'`, and add a
row with the reason; when the line is deleted or changed, gate 19 fails on the row until it is removed.

## Files

| File | What it is |
|---|---|
| `CheckDesign.py` | the gates |
| `TechPatterns.tsv` | provenance, superseded-technology and soft-flag patterns, in plain text; a gap between words is written `[- ]` so that a hyphenated spelling matches too |
| `IpDenylist.sha256` | SHA-256 digests of names and short phrases (gate 1) |
| `OriginalStrings.sha256` | SHA-256 digests of interface strings (gate 2) |
| `BuildDenylist.py` | hashes a plain-text list into the two digest files |
| `SelfTest.py` | proves that the hashed scans find every entry of the plain-text lists, and nothing in a sample of Species text |
| `SelfTestDesignPhase.py` | proves that the design-phase gates 12 to 19 pass and fail where they must, on synthetic design roots |
| `ScanAllow.tsv` | the lines the repository scan (gate 19) allows, as path and SHA-256 digest, with the reason |
| `MermaidCheck.js` | the Mermaid parser driver of gate 4 |
| `package.json`, `package-lock.json` | the pinned `mermaid` and `playwright` packages; `node_modules/` is installed by `npm ci` and is never checked in |

## The hashed denylists

The denylists cover names and text of the reference product and its IP. The repository holds only their SHA-256
digests, never the names or the text.

- **Names (gate 1).** An entry is a phrase of one to five words in its canonical form: ASCII-folded (NFKD, accents and
  other non-ASCII characters dropped), lower case, hyphens and dashes read as spaces, every other punctuation mark
  removed from inside a word (an exclamation mark is kept), whitespace collapsed. The scan reads each paragraph of a
  file in three views and hashes every n-gram of one to five words of each: the canonical view; a joined view that
  also splits at every punctuation mark except the underscore (a listed file stem before its extension, a listed
  identifier before `::`); and a split view that splits at every non-alphanumeric character and at every lower-to-upper
  case step (a listed name inside a CamelCase identifier or before a possessive). It also hashes every prefix and suffix
  of six or more characters of each longer word of the joined view, so a listed word of six or more characters is
  found inside a compound or before a plural ending. N-grams run across line breaks inside a paragraph.
- **Strings (gate 2).** An entry is an interface string of three or more words, in its canonical form: ASCII-folded,
  lower case, printf and `*T` placeholders removed, punctuation removed except apostrophes inside words, whitespace
  collapsed. The scan hashes every quoted string (straight or curly quotes) of three or more words the same way.

## Extending the checks

- **A technology, provenance or soft-flag pattern:** add a row to `TechPatterns.tsv` (TAB-separated: `pattern`, in
  Python `re` syntax; `flags`, `i` for ignore-case or empty; `gate`, `hard` or `soft`; `class`, printed with each hit;
  `note`). Write the pattern so that it does not match its own text, because a hard scan of this folder must stay
  clean: `Foo(?:Bar)` rather than `FooBar`. A soft hit that is right in context is allowed through `CONVENTIONS.md` §9.
- **An allowed term:** add a row to the table of `CONVENTIONS.md` §9, with the backticked path globs (relative to the
  design root) in its `Paths` column. `CONVENTIONS.md` itself is always allowed.
- **A requirement prefix:** add a row to `CONVENTIONS.md` §3.1.
- **A name or an interface string:** put the entries in a plain-text file outside the repository, one per line, and run
  `python3 Tools/DesignCheck/BuildDenylist.py --names <file>` (or `--strings <file>`). The new digests are merged into
  the existing ones (`--replace` rebuilds a file from the list alone). Run `SelfTest.py` with the full lists, then run
  `CheckDesign.py` over `Design/`, before the plain-text file is deleted.
- **A template, a size exemption or another folder outside the design root:** change the constants at the top of
  `CheckDesign.py` (`REF_HEADINGS`, `CMP_HEADINGS`, `SIZE_EXEMPT`, `OUTSIDE_FOLDERS`; for the design-phase gates
  `GDD_HEADINGS`, `SCALE_PATH_HEADING`, `BASELINE_COMPONENTS`, `MODEL_AREAS`, `VERDICTS` and `SCAN_EXCLUDE`).

## Self-test

```
python3 Tools/DesignCheck/SelfTest.py <names list> <strings list>
```

It checks that the digest files match the two plain-text lists exactly, that gate 1 finds every name in a sentence, in
upper and title case, in a code span, a table cell and a quoted string, wrapped across a line break, inside snake_case
and CamelCase identifiers and with a plural ending and a prefix, that gate 2 finds every string in straight and curly
quotes, in upper and title case and with closing punctuation, that a sample paragraph of Species terms gives no hit,
and that the files of this folder and of `Tools/DesignModels/` give no gate 1 hit. It prints counts and list line
numbers, never a list entry.

```
python3 -I Tools/DesignCheck/SelfTestDesignPhase.py
```

It needs no input and runs anywhere. Each case writes a small synthetic repository (a design root with reference
documents, ledger files, GDD files, components, rationale files and decisions, `Tools/DesignModels/`, and files for the
repository scan) into a temporary directory, runs every gate with `--design-phase` (and `--final` where the case says
so), and compares the failures and reported findings of one design-phase gate with the expected ones, message by
message. It covers, for each of gates 12 to 19, a passing case and every failure the gate knows, including Phase 0 with
nothing written yet, scope specificity, pending and dropped-citation reports, and the allow-list (per path, CRLF lines,
stale and malformed rows). The repository scan runs with a synthetic pattern file and denylist, so no fixture names
anything real. It prints one line per check and exits 0 when all pass.

## Updating the pinned Mermaid build

1. Set the new exact version of `mermaid` (or `playwright`) in `package.json`, with no range operator.
2. Run `npm install --prefix Tools/DesignCheck`; it rewrites `package-lock.json`.
3. After a `playwright` update, run `npx --prefix Tools/DesignCheck playwright install chromium`, because each
   Playwright release expects its own Chromium build.
4. Run `python3 Tools/DesignCheck/CheckDesign.py Design` and fix any diagram the new build rejects.
5. Check in `package.json` and `package-lock.json` together; the `design` job installs exactly what the lock file pins
   (`npm ci`).
