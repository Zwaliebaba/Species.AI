"""Delete the rows of named action ids from the numbered binding tables of one Markdown file and renumber them.

Usage: python3 -I P6-ENG1_drop_rows.py <file.md> <action id> [<action id> ...]

A row is deleted when it is a table row whose first cell is a number and whose second cell is exactly the
backticked action id. Within each table that lost a row, the first column is renumbered 1, 2, 3 in row order.
Only the Markdown file named on the command line is written.
"""

import pathlib
import re
import sys

ROW = re.compile(r"^\| (\d+) \| `([^`]+)` \|")


def main():
    path = pathlib.Path(sys.argv[1])
    drop = set(sys.argv[2:])
    lines = path.read_text(encoding="utf-8").split("\n")
    out = []
    table = []  # indices into out of the numbered rows of the current table
    touched = False
    deleted = []

    def flush():
        nonlocal table, touched
        if touched:
            for k, idx in enumerate(table, 1):
                out[idx] = re.sub(r"^\| \d+ \|", f"| {k} |", out[idx], count=1)
        table = []
        touched = False

    for line in lines:
        m = ROW.match(line)
        if m:
            if m.group(2) in drop:
                deleted.append(m.group(2))
                touched = True
                continue
            table.append(len(out))
            out.append(line)
            continue
        if not line.startswith("|"):
            flush()
        out.append(line)
    flush()
    missing = drop - set(deleted)
    if missing:
        print("not found: " + ", ".join(sorted(missing)) + "; nothing written")
        sys.exit(1)
    path.write_text("\n".join(out), encoding="utf-8", newline="\n")
    print("deleted: " + ", ".join(deleted))


if __name__ == "__main__":
    main()
