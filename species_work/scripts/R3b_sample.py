"""R3b helper: list the requirement ids defined in each engine component file and draw a seeded sample.

Usage: python3 -I R3b_sample.py <components_dir> <seed> <per_component>
Prints, per component (file or folder), the number of requirement ids defined and the sampled ids
with the file that defines each. Read-only: it never writes anything.
"""

import pathlib
import random
import re
import sys

DEF = re.compile(r"\*\*((?:TER|RND|AST|VFX|AEN|UI|INP|CAM|DAT|PLT|BLD|DBG)-\d{3})\*\* \((MUST NOT|MUST|SHOULD NOT|SHOULD|MAY)\)")


def main() -> int:
    root = pathlib.Path(sys.argv[1])
    seed = int(sys.argv[2])
    per = int(sys.argv[3])
    rng = random.Random(seed)
    groups: dict[str, list[pathlib.Path]] = {}
    for path in sorted(root.rglob("*.md")):
        rel = path.relative_to(root)
        if rel.name == "README.md" and len(rel.parts) == 1:
            continue
        key = rel.parts[0] if len(rel.parts) > 1 else rel.stem
        groups.setdefault(key, []).append(path)
    for key, files in groups.items():
        ids: list[tuple[str, str]] = []
        for path in files:
            for match in DEF.finditer(path.read_text(encoding="utf-8")):
                ids.append((match.group(1), str(path.relative_to(root))))
        sample = rng.sample(ids, min(per, len(ids))) if ids else []
        sample.sort()
        print(f"{key}: {len(ids)} ids")
        for rid, where in sample:
            print(f"  {rid}  {where}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
