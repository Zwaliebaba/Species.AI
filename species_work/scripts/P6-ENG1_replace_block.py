"""Replace one exact text block in a Markdown file.

Usage: python3 -I P6-ENG1_replace_block.py <file.md> <old block file> <new block file>

The old block must occur exactly once in the Markdown file; otherwise nothing is written.
Only the Markdown file named on the command line is written.
"""

import pathlib
import sys


def main():
    md = pathlib.Path(sys.argv[1])
    old = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8")
    new = pathlib.Path(sys.argv[3]).read_text(encoding="utf-8")
    text = md.read_text(encoding="utf-8")
    n = text.count(old)
    if n != 1:
        print(f"old block found {n} times; nothing written")
        sys.exit(1)
    md.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
    print("replaced")


if __name__ == "__main__":
    main()
