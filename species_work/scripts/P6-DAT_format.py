"""Check the C++ sketches of the given Markdown files against clang-format.

Usage: python3 -I P6-DAT_format.py <clang-format style file> <scratch dir> <file.md> [<file.md> ...]

Every ```cpp block is written to the scratch directory, formatted there with
clang-format (the style file given), and compared with the original. The script
only reads the Markdown files; it prints the blocks that differ, with a unified diff.
"""

import difflib
import pathlib
import shutil
import subprocess
import sys


def blocks(text):
    lines = text.split("\n")
    out = []
    i = 0
    while i < len(lines):
        if lines[i].strip() == "```cpp":
            start = i + 1
            j = start
            while j < len(lines) and lines[j].strip() != "```":
                j += 1
            out.append((start + 1, "\n".join(lines[start:j]) + "\n"))
            i = j + 1
        else:
            i += 1
    return out


def main():
    style = pathlib.Path(sys.argv[1]).resolve()
    scratch = pathlib.Path(sys.argv[2]).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    shutil.copy(style, scratch / ".clang-format")
    bad = 0
    for name in sys.argv[3:]:
        path = pathlib.Path(name)
        for line, code in blocks(path.read_text(encoding="utf-8")):
            src = scratch / "block.cpp"
            src.write_text(code, encoding="utf-8")
            res = subprocess.run(["clang-format", "--style=file", str(src)], capture_output=True, text=True, cwd=scratch)
            if res.returncode != 0:
                print(f"{name}:{line}: clang-format failed: {res.stderr}")
                bad += 1
                continue
            if res.stdout != code:
                bad += 1
                print(f"== {name}:{line}")
                sys.stdout.writelines(
                    difflib.unified_diff(code.splitlines(True), res.stdout.splitlines(True), "original", "formatted")
                )
    print(f"{bad} block(s) differ")


if __name__ == "__main__":
    main()
