#!/usr/bin/env python3
"""P17b: scan every g_prefsManager access in the live source tree.

Usage: python3 -I P17b_prefs_scan.py <SRC_ROOT> <OUT_CSV>

For each access records: key (resolved through #defines), op, default
argument text, file, enclosing function (heuristic), line, and the stack of
enclosing preprocessor conditions. Only code/ and contrib/eclipse are scanned
(contrib/systemiv prefs code is not compiled into Multiwinia - see build_matrix).
"""
import csv
import os
import re
import sys

SRC = sys.argv[1]
OUT = sys.argv[2]

SCAN_DIRS = ["code", "contrib/eclipse"]
EXTS = (".cpp", ".h", ".c")

define_re = re.compile(r'^\s*#\s*define\s+(\w+)\s+(.+?)\s*(//.*)?$')
call_re = re.compile(
    r'g_prefsManager\s*->\s*(GetInt|GetFloat|GetString|GetUnicodeString|SetInt|SetFloat|SetString|DoesKeyExist|AddLine)\s*\(')
func_re = re.compile(r'^(?:[A-Za-z_][\w:<>\*&\s,]*?\s+)?([A-Za-z_~][\w:~]*)\s*\([^;]*$')
pp_if = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
pp_elif = re.compile(r'^\s*#\s*elif\b(.*)$')
pp_else = re.compile(r'^\s*#\s*else\b')
pp_endif = re.compile(r'^\s*#\s*endif\b')

files = []
for d in SCAN_DIRS:
    root = os.path.join(SRC, d)
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(EXTS):
                files.append(os.path.join(dp, f))
files.sort()

# Pass 1: collect string #defines (prefer code/ over others)
defines = {}
for path in files:
    with open(path, encoding="latin-1") as fh:
        for line in fh:
            m = define_re.match(line)
            if m:
                name, val = m.group(1), m.group(2).strip()
                if name not in defines:
                    defines[name] = val


def resolve(tok, depth=0):
    tok = tok.strip()
    if tok.startswith('"'):
        m = re.match(r'"([^"]*)"', tok)
        return m.group(1) if m else tok
    if depth < 5 and tok in defines:
        return resolve(defines[tok], depth + 1)
    return "<" + tok + ">"


def split_args(s):
    """Split the text after '(' into top-level args up to the matching ')'."""
    depth = 0
    args, cur = [], ""
    i = 0
    in_str = False
    while i < len(s):
        c = s[i]
        if in_str:
            cur += c
            if c == '\\' and i + 1 < len(s):
                cur += s[i + 1]
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
            cur += c
        elif c == '(':
            depth += 1
            cur += c
        elif c == ')':
            if depth == 0:
                args.append(cur.strip())
                return args
            depth -= 1
            cur += c
        elif c == ',' and depth == 0:
            args.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    args.append(cur.strip())
    return args


rows = []
for path in files:
    rel = os.path.relpath(path, SRC).replace(os.sep, "/")
    with open(path, encoding="latin-1") as fh:
        lines = fh.readlines()
    func = ""
    ppstack = []
    for idx, line in enumerate(lines):
        s = line.rstrip("\n")
        m = pp_if.match(s)
        if m:
            kind, cond = m.group(1), m.group(2).strip()
            cond = re.sub(r'//.*', '', cond).strip()
            if kind == "ifdef":
                cond = "defined(" + cond + ")"
            elif kind == "ifndef":
                cond = "!defined(" + cond + ")"
            ppstack.append(cond)
            continue
        m = pp_elif.match(s)
        if m and ppstack:
            ppstack[-1] = "elif " + re.sub(r'//.*', '', m.group(1)).strip()
            continue
        if pp_else.match(s) and ppstack:
            ppstack[-1] = "else-of(" + ppstack[-1] + ")"
            continue
        if pp_endif.match(s) and ppstack:
            ppstack.pop()
            continue
        if s and not s[0].isspace() and not s.startswith(('#', '//', '{', '}', '/*', '*')):
            fm = func_re.match(s)
            if fm and not s.rstrip().endswith(';') and not s.startswith(('if', 'while', 'for', 'switch', 'return', 'else')):
                func = fm.group(1)
        stripped = s.strip()
        commented = stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*")
        for cm in call_re.finditer(s):
            # join following lines for multi-line calls
            rest = s[cm.end():] + "".join(lines[idx + 1: idx + 4])
            args = split_args(rest)
            op = cm.group(1)
            key_raw = args[0] if args else ""
            key = resolve(key_raw) if op != "AddLine" else key_raw
            default = args[1] if len(args) > 1 else ""
            pre = s[:cm.start()]
            line_comment = "//" in pre
            rows.append({
                "key": key,
                "op": op,
                "arg2": default.replace("\n", " ")[:80],
                "file": rel,
                "function": func,
                "line": idx + 1,
                "commented": "Y" if (commented or line_comment) else "",
                "pp": " && ".join(ppstack),
            })

with open(OUT, "w", newline="") as fh:
    w = csv.DictWriter(fh, fieldnames=["key", "op", "arg2", "file", "function", "line", "commented", "pp"])
    w.writeheader()
    for r in rows:
        w.writerow(r)
print(len(rows), "accesses;", len({r['key'] for r in rows}), "distinct keys")
