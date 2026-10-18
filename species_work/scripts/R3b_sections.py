"""R3b helper: print chosen top-level sections ("## <n> ...") of a Markdown file.

Usage: python3 -I R3b_sections.py <file> <n> [<n> ...]
Each section runs from its "## <n>" heading to the next "## " heading. Read-only.
"""

import pathlib
import re
import sys

HEAD = re.compile(r"^## (\d+)\b")


def main() -> int:
    path = pathlib.Path(sys.argv[1])
    wanted = set(sys.argv[2:])
    current = None
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if line.startswith("## "):
            match = HEAD.match(line)
            current = match.group(1) if match else None
        if current in wanted:
            print(f"{number}: {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
