"""X-GLO-2 helper: print status and specified behaviour of selected DEC entries.

Usage: python3 -I X-GLO-2_decs.py <decisions_dir> DEC-002 DEC-003 ...
"""
import os
import re
import sys


def main():
    d = sys.argv[1]
    want = set(sys.argv[2:])
    files = []
    for root, _, names in os.walk(d):
        for n in names:
            if n.endswith(".md") and n != "README.md":
                files.append(os.path.join(root, n))
    for f in sorted(files):
        text = open(f, encoding="utf-8").read()
        for m in re.finditer(r"^### (DEC-\d{3}) (.*)$", text, re.M):
            dec = m.group(1)
            if dec not in want:
                continue
            start = m.end()
            nxt = re.search(r"^### DEC-\d{3} ", text[start:], re.M)
            body = text[start: start + nxt.start()] if nxt else text[start:]
            status = re.search(r"- Status: (.*)", body)
            spec = re.search(r"- Specified behaviour: (.*)", body)
            print(f"{dec} [{status.group(1).strip() if status else '?'}] {m.group(2)}")
            if spec:
                print("   " + spec.group(1).strip()[:900])


if __name__ == "__main__":
    main()
