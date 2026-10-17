# X1a_text_formats — Text data formats (formal grammars, part A)
Scope: `data/levels/**`, `data/purity/*`, `data/game*.txt`, `data/locations.txt`, `data/multiwinia*.txt`, `data/*betatesters.txt`, `data/default_preferences.txt`, `data/input/**`, `data/language/*`, `data/scripts/*`, root `testbedconfig.txt`, `MultiwiniaSupport.ini`, plus every parser that reads them and every runtime-written text file found on the way. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Conventions used in every grammar below:

* EBNF: `a b` sequence, `a | b` alternative, `[ a ]` optional, `{ a }` zero or more, `'lit'` literal (case-insensitive unless marked **(cs)** = case-sensitive), `EOL` end of a physical line.
* All grammars are stated **after** the shared line-cleaning and tokenising pass of §1 (`fmt-textreader`). "Token" means a maximal run of non-separator characters on a cleaned line.
* `INT` = whatever C `atoi` accepts (optional sign, decimal digits, leading junk stops the scan, no error reporting; an unparsable token yields 0). `FLOAT` = whatever C `atof` accepts (decimal, optional exponent). `F32` = `FLOAT` then rounded to IEEE-754 single precision and widened back to `double` (`iv_atof` ≡ `(double)(float)atof(s)`, `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof`, `iv_trunc_low_nibble`) [C]. The F32 rounding is a determinism measure; a re-implementation must replicate it for any value that feeds the simulation.
* "MISSING ⇒ CRASH" means the parser calls `GetNextToken()` without checking `TokenAvailable()`; a short line gives a `NULL` token which is passed to `atoi`/`atof`/`strcpy`/`stricmp` → access violation or CRT invalid-parameter termination [C: code path; I: exact failure mode is MSVC CRT behaviour; no `_set_invalid_parameter_handler` exists in `code/` or `contrib/systemiv/lib`].

<a id="fmt-textreader"></a>
## [→ 29_data_formats_reference] 1. Shared line reader and tokeniser (`TextReader` family)

### 1.1 Purpose, classes and where they are chosen

Every text format in this document is read through one of five reader classes, all compiled into the baseline [B:FULL] [C] `targets/vs2005/Darwinia.vcproj` (entries `contrib\systemIV\lib\filesys\text_stream_readers.cpp`, `contrib\systemIV\lib\unicode\unicode_text_stream_reader.cpp`):

| Class | Source | Input | Encoding | Used when |
|---|---|---|---|---|
| `TextReader` (abstract) | `contrib/systemiv/lib/filesys/text_stream_readers.{h,cpp}` | – | – | base: cleaning + tokenising |
| `TextFileReader` | same | loose disk file, `fopen(...,"r")` (CRT text mode → CRLF read as LF) | single-byte (no transcoding; bytes ≥0x80 pass through) | disk file without a UTF-16 BOM |
| `TextDataReader` | same | in-memory block from a `.dat` archive | single-byte, raw bytes (CR kept, removed by cleaning) | archive entry without a UTF-16 BOM |
| `UnicodeTextFileReader` | `contrib/systemiv/lib/unicode/unicode_text_stream_reader.{h,cpp}` | loose disk file, `fopen(...,"rb")` | UTF-16 with BOM `FF FE` (LE) or `FE FF` (BE); 2 bytes per code unit, no surrogate handling | disk file whose first two bytes are a BOM |
| `UnicodeTextDataReader` | same | archive block | as above | archive entry beginning with a BOM |

Selection is done by `code/lib/resource.cpp:Resource::GetTextReader(name)` [C]:

1. (`USE_DARWINIA_MOD_SYSTEM` only — [B:DEAD]) `<profile>/mods/<mod>/<name>`, then (`TARGET_OS_VISTA`) `mods/<mod>/<name>`.
2. Loose file `data/<name>` (relative to the working directory): try `UnicodeTextFileReader`; if `IsUnicode()` is false (no BOM) delete it and use `TextFileReader`.
3. Archive entry `data/<name>` (exact, case-sensitive key; archive keys are lower-cased by `Resource::ParseArchive`, so callers must pass lower-case names to hit an archive) — `UnicodeTextDataReader`, else `TextDataReader`.
4. Otherwise `NULL` and the debug log line `Failed to find text resource: <path>`.

A few callers bypass `Resource` and construct `TextFileReader`/`UnicodeTextFileReader` on an absolute or cwd-relative path directly (preferences, profile saves, `testbedconfig.txt`, `origin`, input bindings): they are listed per format. `contrib/systemiv/lib/filesys/file_system.cpp:FileSystem::GetTextReader` is compiled but `g_fileSystem` is never created by Multiwinia [C] (grep: no `new FileSystem` under `code/`) — ignore it.

`TextFileReader` and `UnicodeTextFileReader` call `FindCaseInsensitive(path)` before opening (`contrib/systemiv/lib/filesys/filesys_utils.cpp`) so loose files are matched case-insensitively on case-sensitive file systems [C]; on Win32 this is a no-op in effect [I].

### 1.2 Physical lines

* A line ends at LF (`0x0A` / `L'\n'`). The final line may lack an LF. Lines may be arbitrarily long: the buffer starts at 128 characters and doubles; a line longer than 65 536 characters triggers `AppReleaseAssert("Text file contains line with more than 65536 characters")` [C] `TextReader::DoubleMaxLineLen`, `UnicodeTextReader::DoubleMaxLineLen`.
* End of file: only `TextFileReader` processes an unterminated last line (its `ReadLine` returns `true` for a final line without LF and `false` on the following call). `TextDataReader` (archive), `UnicodeTextFileReader` and `UnicodeTextDataReader` load the unterminated last line but return `false` for it, so the `while (in->ReadLine())` loop body **never sees it** [C] `TextDataReader::ReadLine` (`eof = true` when no `\n` is found), `UnicodeTextFileReader::ReadUnicodeLine` (returns `NULL` as soon as `feof`), `UnicodeTextDataReader::ReadLine`. Rule for authors: **always end a text file with a newline** (every shipped file in scope does — survey §1.6).
* A U+0000 code unit in a UTF-16 disk file is skipped without being stored (the slot keeps stale data) [C] `UnicodeTextFileReader::ReadUnicodeLine` (`temp != NULL` test) — never emit NUL.

### 1.3 Line cleaning (`TextReader::CleanLine`, `UnicodeTextReader::CleanLine`) — applied to every line, in this order [C]

1. **De-obfuscation.** On the first line only, if the line starts with `redshirt2` (case-insensitive, 9 chars) the file is marked encrypted and those 9 characters are removed. For every subsequent character `> 32` in an encrypted file: `i = (i+1) mod 16; c' = c − K[i]; if c' < 33 then c' += 95` with `K = {31,7,9,1,11,2,5,5,3,17,40,12,35,22,27,2}` and `i` starting at 0 and persisting across lines. Characters `≤ 32` (space, tab, CR, LF) are untouched. The writer side is `TextFileWriter(..., encrypt=true)` / `UnicodeTextFileWriter` (`c' = c + K[i]; if c' ≥ 128 then c' −= 95`) [C] `code/unicode/unicode_text_file_writer.cpp:UnicodeTextFileWriter::printf`. No shipped file under `data/` is encrypted (survey: no file starts with `redshirt2`) [C] `X1a_text_survey.py`; encryption is used only for runtime-written profile saves (§13).
2. **Comment removal.** Scan left to right; the first `#` and everything after it is deleted. There is **no escape and no quoting**: a `#` inside what looks like a string still starts a comment. While scanning, runs of `<` / `>` are counted and three adjacent angle brackets trip `AppDebugAssert` (merge-conflict detector; debug builds only — [B:DEBUG]).
3. **Line-end removal.** Truncate at the first `\n`, then truncate at the first `\r`. Consequence: a stray CR in the middle of a line silently discards the rest of that line.

Edge case (UTF-16 disk files): `UnicodeTextFileReader::ReadLine` converts the wide line to the narrow mirror buffer (`UnicodeString::CopyUnicodeToCharArray`, every code unit > 255 becomes `?`) **before** `CleanLine` runs on the wide buffer, so the narrow mirror still contains the comment text; `UnicodeTextDataReader::ReadLine` cleans first and converts afterwards [C]. Tokenising of the first token is done on the narrow mirror (`TextReader::GetNextToken` is not virtual) while `TokenAvailable`/`GetNextUnicodeToken`/`GetRestOfUnicodeLine` are overridden and use the wide buffer. Because both buffers have identical indices (1 code unit ↔ 1 byte), this only matters for a line whose *first* token is followed by `#`: the narrow first token is still correct (a comment can only start after a separator or at the token start, see below) [I]. A line that is entirely a comment is correctly skipped because `TokenAvailable` looks at the cleaned wide buffer [C].

### 1.4 Tokens

* Separator set (default): space, TAB, LF, CR, `:` and `,` — `" \t\n\r:,"` for the narrow readers; the Unicode readers additionally treat form-feed `\f` as a separator (`L" \f\t\n\r:,"`) [C] `DEFAULT_SEPERATOR_CHARS` in both `.cpp` files. A caller may replace the set with `SetSeperatorChars` (max 15 characters) — only the input-binding parser does (§7).
* `GetNextToken()`: skip separators; if end of line → `NULL`; else return the maximal run of non-separators and overwrite the following separator with NUL. `:` and `,` are therefore exactly equivalent to whitespace — `BuildingOnline :mine,90` is the three tokens `BuildingOnline`, `mine`, `90`.
* `GetRestOfLine()`: skip separators, return the remainder of the line **verbatim** (internal separators, trailing spaces and tabs preserved — there is no right-trim) or `NULL` at end of line. Because of the leading skip, a "rest of line" value can never begin with space, tab, `:` or `,`.
* `TokenAvailable()`: true iff a non-separator character remains on the line.
* There is **no quoting, no escaping and no line continuation** at this level. Individual formats add their own conventions (the language table's `\n`, §9).
* Case: the reader never changes case. Keyword comparisons are done by each parser (mostly `stricmp`, i.e. ASCII case-insensitive; exceptions flagged **(cs)**).

### 1.5 Formal summary

```
file          := [ 'redshirt2' ] { line }                 ; header only on the very first line (case-insensitive)
line          := { char - LF } ( LF | EOF )
cleaned_line  := prefix of line before the first '#', then before the first LF, then before the first CR
separator     := ' ' | TAB | LF | CR | ':' | ',' [ | FF  (UTF-16 readers only) ]
token         := 1*( char - separator )
token_line    := { separator } [ token { 1*separator token } ] { separator }
rest_of_line  := everything after the leading separators, verbatim
```

### 1.6 Encoding survey of the files in scope [C] `X1a_text_survey.py`

See the coverage table at the end of this document and the per-format sections; summary: every file is single-byte ASCII/Latin-1 with CRLF line ends **except** 8 of the 9 language files (UTF-16 with BOM: big-endian French, German, Italian, Korean, Portuguese, Spanish; little-endian Japanese, Russian). `data/betatesters.txt` and `data/multiwinia_betatesters.txt` contain Latin-1 bytes (accented names) which pass through the narrow reader unchanged and are then widened byte-for-byte by `UnicodeString(char const*)` (each byte becomes the code unit of the same value, i.e. ISO-8859-1) [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::UnicodeString(char const*)`. The reverse conversion (`CopyUnicodeToCharArray`) maps every code unit > 255 to `?` [C].

### 1.7 Re-implementation notes

* Implement one tokeniser with: BOM sniffing (UTF-16LE/BE, else bytes-as-Latin-1), optional `redshirt2` de-obfuscation (only needed to read legacy saves), `#` comments, first-CR truncation, the separator set above, `next_token`, `rest_of_line`, `token_available`, a per-reader separator override, and EOF semantics that *do* process an unterminated last line (fixing the archive asymmetry).
* Keep the `F32` rounding helper for numeric fields that feed simulation.

