"""Check the C++ sketches of a design document against clang-format.

Usage: python3 -I P6-X3_fmt.py <style file> <scratch dir> <document> [<marker> ...]

Every ```cpp block of the document that contains one of the markers (all blocks
when no marker is given) is written to a scratch file, formatted with
clang-format --style=file:<style file>, and the difference is printed.
"""

import difflib
import pathlib
import subprocess
import sys


def blocks(text):
    lines = text.splitlines()
    inside = False
    start = 0
    for index, line in enumerate(lines):
        stripped = line.strip()
        if not inside and stripped.startswith("```cpp"):
            inside = True
            start = index + 1
        elif inside and stripped == "```":
            inside = False
            yield start + 1, lines[start:index]


def main():
    style = pathlib.Path(sys.argv[1])
    scratch = pathlib.Path(sys.argv[2])
    document = pathlib.Path(sys.argv[3])
    markers = sys.argv[4:]
    scratch.mkdir(parents=True, exist_ok=True)
    text = document.read_text(encoding="utf-8")
    total = 0
    for number, (first_line, body) in enumerate(blocks(text)):
        source = "\n".join(body) + "\n"
        if markers and not any(marker in source for marker in markers):
            continue
        path = scratch / f"sketch_{number}.cpp"
        path.write_text(source, encoding="utf-8")
        result = subprocess.run(
            ["clang-format", f"--style=file:{style}", str(path)],
            capture_output=True,
            text=True,
            check=True,
        )
        diff = list(
            difflib.unified_diff(
                source.splitlines(), result.stdout.splitlines(), f"{document}:{first_line}", "formatted", lineterm=""
            )
        )
        total += 1
        if diff:
            print("\n".join(diff))
        else:
            print(f"{document}:{first_line}: clean")
    if total == 0:
        print("no block matched")


if __name__ == "__main__":
    main()
