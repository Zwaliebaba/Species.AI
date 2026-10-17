"""FX-05 helper: apply exact text replacements to one file.

Usage: python3 -I FX-05_apply.py <target file> <edits file>

The edits file holds blocks:
@@@ OLD
<old text>
@@@ NEW
<new text>
@@@ END
Each old text must occur exactly once in the target at the time it is applied;
otherwise nothing is written and the script reports the failing block.
"""
import sys


def parse(path):
    blocks = []
    mode = None
    old, new = [], []
    with open(path, encoding="utf-8") as f:
        for line in f:
            tag = line.rstrip("\n")
            if tag == "@@@ OLD":
                mode, old, new = "old", [], []
            elif tag == "@@@ NEW":
                mode = "new"
            elif tag == "@@@ END":
                blocks.append(("".join(old).rstrip("\n"), "".join(new).rstrip("\n")))
                mode = None
            elif mode == "old":
                old.append(line)
            elif mode == "new":
                new.append(line)
    return blocks


def main():
    target, edits = sys.argv[1], sys.argv[2]
    with open(target, encoding="utf-8") as f:
        text = f.read()
    for i, (old, new) in enumerate(parse(edits), 1):
        n = text.count(old)
        if n != 1:
            print(f"block {i}: old text occurs {n} times: {old[:120]!r}")
            sys.exit(1)
        text = text.replace(old, new)
    with open(target, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    print(f"applied {len(parse(edits))} blocks to {target} ({len(text.encode('utf-8'))} bytes)")


if __name__ == "__main__":
    main()
