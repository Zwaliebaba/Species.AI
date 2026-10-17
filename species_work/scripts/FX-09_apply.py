"""FX-09 helper: apply exact, unique-match replacements from a spec file.

Usage: python3 -I FX-09_apply.py <spec.py> [--dry-run]

The spec file is Python source that defines EDITS, a list of (path, old, new) tuples; paths are
relative to the current directory. Every old string must occur exactly once in its file at the
moment it is applied (edits apply in order). On any miss nothing is written.
"""
import sys


def main() -> int:
    spec_path = sys.argv[1]
    dry = "--dry-run" in sys.argv[2:]
    namespace = {}
    with open(spec_path, encoding="utf-8") as f:
        exec(compile(f.read(), spec_path, "exec"), namespace)
    edits = namespace["EDITS"]
    texts = {}
    failures = 0
    for i, (path, old, new) in enumerate(edits, 1):
        if path not in texts:
            with open(path, encoding="utf-8") as f:
                texts[path] = f.read()
        count = texts[path].count(old)
        if count != 1:
            print(f"edit {i} ({path}): old string occurs {count} times: {old[:90]!r}")
            failures += 1
            continue
        texts[path] = texts[path].replace(old, new, 1)
    if failures:
        print(f"{failures} edits failed; nothing written")
        return 1
    if not dry:
        for path, text in texts.items():
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
    print(f"{len(edits)} edits applied to {len(texts)} files" + (" (dry run)" if dry else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
