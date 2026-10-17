"""FX-03 helper: apply exact text replacements to one file.

Usage: python3 -I FX-03_apply.py <target.md> <replacements.json>

The JSON file holds a list of [old, new] pairs. Each old string must occur
exactly once in the target (checked before any write); the pairs are applied
in order and the file is written once, only if every pair matched.
"""
import json
import sys


def main() -> int:
    target, spec = sys.argv[1], sys.argv[2]
    with open(target, encoding="utf-8") as f:
        text = f.read()
    with open(spec, encoding="utf-8") as f:
        pairs = json.load(f)
    for i, (old, new) in enumerate(pairs):
        n = text.count(old)
        if n != 1:
            print(f"pair {i}: found {n} times: {old[:80]!r}")
            return 1
        text = text.replace(old, new)
    with open(target, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"{target}: {len(pairs)} replacements, {len(text.encode('utf-8'))} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
