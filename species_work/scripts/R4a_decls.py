"""R4a: extract declarations from every cpp/hlsl block and check AGENTS.md section 1 naming.

Usage: python3 -I R4a_decls.py <out_dir>
Writes <out_dir>/decls.tsv (name, kind, namespace, scope, block, file, line, detail) and
<out_dir>/naming.tsv (block, file, line, rule, identifier, text).
"""
import csv
import os
import re
import sys

out = sys.argv[1]
with open(os.path.join(out, "blocks.tsv"), encoding="utf-8") as f:
    rows = [r for r in csv.DictReader(f, delimiter="\t") if r["lang"] in ("cpp", "hlsl")]

PASCAL = re.compile(r"^[A-Z][A-Za-z0-9]*$")
CAMEL = re.compile(r"^[a-z][A-Za-z0-9]*$")
UPPER = re.compile(r"^[A-Z][A-Z0-9_]*$")
KEYWORDS = set("""alignas alignof and asm auto bool break case catch char char8_t char16_t char32_t class concept const consteval
constexpr constinit const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum
explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not nullptr operator or
private protected public register reinterpret_cast requires return short signed sizeof static static_assert static_cast
struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile
wchar_t while override final uint uint2 uint3 uint4 float2 float3 float4 float4x4 float3x4 int2 int3 int4 row_major
cbuffer Texture2D RWStructuredBuffer StructuredBuffer SamplerState""".split())


def strip(text):
    # remove comments and string literals but keep line structure
    res = []
    i = 0
    n = len(text)
    comments = {}
    line = 1
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            comments.setdefault(line, []).append(text[i:j])
            i = j
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            seg = text[i:j]
            res.append("\n" * seg.count("\n"))
            line += seg.count("\n")
            i = j
            continue
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == "\\":
                    j += 1
                j += 1
            res.append(c + c)
            i = j + 1
            continue
        if c == "\n":
            line += 1
        res.append(c)
        i += 1
    return "".join(res), comments


decls = []
naming = []


def add_name(kind, name, ns, scope, blk, line, detail=""):
    decls.append((name, kind, ns, scope, blk["n"], blk["file"], str(int(blk["start"]) + line), detail))


def flag(blk, line, rule, ident, text):
    naming.append((blk["n"], blk["file"], str(int(blk["start"]) + line), rule, ident, text.strip()[:160]))


def split_top(s, sep=","):
    parts, depth, cur = [], 0, []
    for ch in s:
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        parts.append("".join(cur))
    return parts


def param_names(params):
    names = []
    for p in split_top(params):
        p = p.strip()
        if not p or p == "void" or p == "...":
            continue
        p = p.split("=")[0].strip()
        p = re.sub(r"\[[^\]]*\]$", "", p).strip()
        m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", p)
        if not m:
            continue
        name = m.group(1)
        before = p[: m.start()].strip()
        if not before or before.endswith("::") or name in KEYWORDS:
            continue  # unnamed parameter (type only)
        if re.match(r"^(const\s+)?(std::)?[A-Za-z_:<>]+$", p) and " " not in p.strip():
            continue
        names.append(name)
    return names


for blk in rows:
    n = int(blk["n"])
    raw = open(os.path.join(out, "blocks", f"{n:04d}.txt"), encoding="utf-8").read()
    text, comments = strip(raw)
    # statement scan with a scope stack
    stack = [("file", "", "")]  # (kind, name, access)
    ns_stack = []
    buf = []
    buf_line = 1
    line = 1
    i = 0
    paren = 0
    angle_guard = 0
    while i < len(text):
        c = text[i]
        if c == "\n":
            line += 1
        if not "".join(buf).strip():
            buf_line = line
        if c == "(":
            paren += 1
        elif c == ")":
            paren -= 1
        if paren == 0 and c in "{};":
            stmt = " ".join("".join(buf).split())
            buf = []
            top = stack[-1]
            ns = "::".join(x[1] for x in stack if x[0] == "namespace") or "(none)"
            scope = top[0]
            # access specifiers inside class bodies
            while True:
                m = re.match(r"^(public|private|protected)\s*:\s*(.*)$", stmt)
                if not m:
                    break
                stack[-1] = (top[0], top[1], m.group(1))
                top = stack[-1]
                stmt = m.group(2)
            if stmt.startswith("#"):
                pass
            if c == "{":
                m = re.match(r"^(?:export\s+)?namespace\s+([A-Za-z_][A-Za-z0-9_:]*)$", stmt)
                me = re.match(r"^(?:enum)\s+(class|struct)?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(:\s*[A-Za-z_:0-9]+)?$", stmt)
                mc = re.match(r"^(?:template\s*<.*>\s*)?(struct|class|union|cbuffer)\s+(?:alignas\([^)]*\)\s+)?(?:\[\[[^\]]*\]\]\s*)?([A-Za-z_][A-Za-z0-9_]*)(\s+final)?\s*(:.*)?$", stmt)
                if m:
                    stack.append(("namespace", m.group(1), ""))
                    add_name("namespace", m.group(1), ns, scope, blk, buf_line)
                elif me:
                    name = me.group(2)
                    add_name("enum", name, ns, scope, blk, buf_line, me.group(3) or "")
                    # collect enumerators up to the closing brace
                    j = text.find("}", i)
                    body = text[i + 1 : j]
                    enumerators = []
                    for e in split_top(body):
                        e = e.strip()
                        if not e:
                            continue
                        en = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)", e)
                        if en:
                            enumerators.append(en.group(1))
                            if not PASCAL.match(en.group(1)):
                                flag(blk, buf_line, "enumerator-not-PascalCase", en.group(1), e)
                    decls[-1] = decls[-1][:7] + (",".join(enumerators),)
                    line += body.count("\n")
                    i = j + 1
                    continue
                elif mc:
                    kind, name = mc.group(1), mc.group(2)
                    stack.append((kind, name, "public" if kind in ("struct", "union", "cbuffer") else "private"))
                    add_name(kind, name, ns, scope, blk, buf_line, (mc.group(4) or "").strip())
                else:
                    # function body, initializer, lambda, etc.
                    stack.append(("block", stmt[:40], ""))
                    # check function definitions in class scope (inline bodies)
                    mf = re.search(r"([A-Za-z_~][A-Za-z0-9_]*)\s*\((.*)\)\s*(const)?\s*(noexcept)?\s*(override)?\s*$", stmt)
                    if mf and scope in ("struct", "class", "namespace", "file"):
                        fname = mf.group(1)
                        if fname not in KEYWORDS and not fname.startswith("~"):
                            kindf = "method" if scope in ("struct", "class") else "function"
                            if scope in ("namespace", "file"):
                                add_name(kindf, fname, ns, scope, blk, buf_line)
                            owner = top[1]
                            if fname != owner and not PASCAL.match(fname) and not fname.startswith("operator"):
                                flag(blk, buf_line, f"{kindf}-not-PascalCase", fname, stmt)
                            for p in param_names(mf.group(2)):
                                if not re.match(r"^_[a-z][A-Za-z0-9]*$", p):
                                    flag(blk, buf_line, "parameter-not-_camelCase", p, stmt)
                i += 1
                continue
            if c == "}":
                if len(stack) > 1:
                    stack.pop()
                i += 1
                continue
            # c == ';'
            if not stmt:
                i += 1
                continue
            if top[0] == "block":
                i += 1
                continue
            s = stmt
            s = re.sub(r"^template\s*<.*?>\s*", "", s)
            mu = re.match(r"^using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$", s)
            mfwd = re.match(r"^(?:enum\s+(?:class\s+)?|struct\s+|class\s+)([A-Za-z_][A-Za-z0-9_]*)(\s*:\s*[A-Za-z_:0-9]+)?$", s)
            mconcept = re.match(r"^concept\s+([A-Za-z_][A-Za-z0-9_]*)", s)
            if s.startswith("using namespace"):
                flag(blk, buf_line, "using-namespace", s, s)
            elif mu:
                add_name("using", mu.group(1), ns, scope, blk, buf_line, mu.group(2)[:80])
                if not PASCAL.match(mu.group(1)):
                    flag(blk, buf_line, "alias-not-PascalCase", mu.group(1), s)
            elif mfwd:
                add_name("forward", mfwd.group(1), ns, scope, blk, buf_line)
            elif mconcept:
                add_name("concept", mconcept.group(1), ns, scope, blk, buf_line)
            elif s.startswith("static_assert") or s.startswith("friend") or s.startswith("#"):
                pass
            else:
                # function declaration or variable
                is_static = bool(re.search(r"\bstatic\b", s))
                is_constexpr = bool(re.search(r"\b(constexpr|consteval)\b", s))
                is_const = bool(re.search(r"^(?:static\s+|inline\s+)*const\b", s))
                mf = re.search(r"([A-Za-z_~][A-Za-z0-9_:]*|operator\s*\S+)\s*\((.*)\)\s*(const)?\s*(noexcept(\(.*\))?)?\s*(override|final)?\s*(=\s*(0|default|delete))?\s*(->.*)?$", s)
                if mf and not re.search(r"[A-Za-z_][A-Za-z0-9_]*\s*=\s*[^=]*\(", s.split("(")[0] + "("):
                    fname = mf.group(1).split("::")[-1]
                    if fname in KEYWORDS or fname.startswith("operator") or fname.startswith("~") or fname == top[1]:
                        pass
                    else:
                        if scope in ("namespace", "file"):
                            add_name("function", fname, ns, scope, blk, buf_line)
                        if not PASCAL.match(fname):
                            flag(blk, buf_line, ("method" if scope in ("struct", "class") else "function") + "-not-PascalCase", fname, s)
                    for p in param_names(mf.group(2)):
                        if not re.match(r"^_[a-z][A-Za-z0-9]*$", p):
                            flag(blk, buf_line, "parameter-not-_camelCase", p, s)
                else:
                    # variable(s)
                    s2 = s.split("=")[0]
                    s2 = re.sub(r"\{.*\}$", "", s2).strip()
                    s2 = re.sub(r"\[[^\]]*\]$", "", s2).strip()
                    s2 = re.sub(r":\s*\d+$", "", s2).strip()  # bit-fields
                    s2 = re.sub(r":\s*register\(.*\)$", "", s2).strip()
                    s2 = re.sub(r":\s*[A-Z_0-9]+$", "", s2).strip()  # HLSL semantics
                    names = []
                    parts = split_top(s2)
                    for k, part in enumerate(parts):
                        part = re.sub(r"\[[^\]]*\]$", "", part.strip()).strip()
                        mm = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", part)
                        if mm and (k > 0 or len(part.split()) > 1 or "*" in part or "&" in part):
                            names.append(mm.group(1))
                    for vn in names:
                        if vn in KEYWORDS:
                            continue
                        if scope in ("struct", "class", "union", "cbuffer"):
                            acc = top[2]
                            if is_constexpr or (is_static and is_const):
                                rule_ok = UPPER.match(vn)
                                rule = "class-constant-not-UPPER_CASE"
                            elif is_static:
                                rule_ok = re.match(r"^sm_[a-z][A-Za-z0-9]*$", vn)
                                rule = "static-member-not-sm_"
                            elif acc in ("private", "protected"):
                                rule_ok = re.match(r"^m_[a-z][A-Za-z0-9]*$", vn)
                                rule = "private-member-not-m_"
                            else:
                                rule_ok = CAMEL.match(vn)
                                rule = "public-field-not-camelCase"
                            if not rule_ok:
                                flag(blk, buf_line, rule, vn, s)
                            add_name("field", vn, ns, f"{top[0]} {top[1]}", blk, buf_line, acc)
                        elif scope in ("namespace", "file"):
                            if is_constexpr or is_const:
                                add_name("constant", vn, ns, scope, blk, buf_line)
                                if not UPPER.match(vn):
                                    flag(blk, buf_line, "constant-not-UPPER_CASE", vn, s)
                            else:
                                add_name("global", vn, ns, scope, blk, buf_line)
                                if not re.match(r"^g_[a-z][A-Za-z0-9]*$", vn):
                                    flag(blk, buf_line, "global-not-g_", vn, s)
            i += 1
            continue
        buf.append(c)
        i += 1
    # macros
    for k, l in enumerate(raw.split("\n"), 1):
        mm = re.match(r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)", l)
        if mm:
            add_name("macro", mm.group(1), "(pp)", "file", blk, k)
            if not UPPER.match(mm.group(1)):
                flag(blk, k, "macro-not-UPPER_CASE", mm.group(1), l)

with open(os.path.join(out, "decls.tsv"), "w", encoding="utf-8") as g:
    g.write("name\tkind\tnamespace\tscope\tblock\tfile\tline\tdetail\n")
    for d in decls:
        g.write("\t".join(d) + "\n")
with open(os.path.join(out, "naming.tsv"), "w", encoding="utf-8") as g:
    g.write("block\tfile\tline\trule\tidentifier\ttext\n")
    for d in naming:
        g.write("\t".join(d) + "\n")
print(len(decls), "declarations;", len(naming), "naming flags")
