#!/usr/bin/env python3
"""S90 glossary helper.

Usage (always run with python3 -I):
  python3 -I S_90_terms.py harvest <design_dir> <class_csv> <out_dir>
      Harvests candidate glossary terms from the final documents:
        headings.tsv      every heading of every final document with its GitHub anchor
        bold.tsv          every **bold** term, total count, first doc and heading
        classes.tsv       each of the 159 classes: mention count, best heading anchor
        tags.tsv          evidence/build tag counts per document
        abbrev.tsv        upper-case abbreviations (2-7 chars) with counts
        backticks.tsv     most frequent backticked identifiers
        ipitems.tsv       every row of every document's "IP items" table
  python3 -I S_90_terms.py render <design_dir> <terms_tsv> <out_md_fragment>
      Renders the curated glossary table (alphabetical, letter headings) from
      a TSV with columns Term, Kind, Definition, Link, IP. Link entries are
      "NN:<section-number>" or "NN:<heading text>" (several separated by ';').
      Every link is resolved against the real headings; unresolved links abort.
"""
import csv
import os
import re
import sys
from collections import Counter, defaultdict

DOC_RE = re.compile(r"^(\d\d)_[a-z0-9_]+\.md$")


def slugify(text):
    """GitHub-style anchor: lowercase, drop punctuation, spaces to hyphens."""
    t = text.strip().lower()
    out = []
    for ch in t:
        if ch.isalnum() or ch in " -_":
            out.append(ch)
    return "".join(out).replace(" ", "-")


def strip_md(text):
    text = re.sub(r"`([^`]*)`", r"\1", text)
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
    text = text.replace("**", "").replace("*", "")
    return text


def list_docs(design_dir):
    docs = {}
    for name in sorted(os.listdir(design_dir)):
        m = DOC_RE.match(name)
        if m and m.group(1) not in ("90", "91", "99"):
            docs[m.group(1)] = name
    return docs


def doc_headings(path):
    """Return list of (level, raw_text, plain_text, anchor, line_no)."""
    heads = []
    seen = Counter()
    in_fence = False
    with open(path, encoding="utf-8") as fh:
        for no, line in enumerate(fh, 1):
            if line.startswith("```"):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            m = re.match(r"^(#{1,6})\s+(.*?)\s*$", line)
            if not m:
                continue
            raw = m.group(2)
            plain = strip_md(raw)
            base = slugify(plain)
            anchor = base if seen[base] == 0 else f"{base}-{seen[base]}"
            seen[base] += 1
            heads.append((len(m.group(1)), raw, plain, anchor, no))
    return heads


def harvest(design_dir, class_csv, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    docs = list_docs(design_dir)
    classes = []
    with open(class_csv, encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            classes.append(row)
    class_names = {r["class"] for r in classes}

    bold = Counter()
    bold_first = {}
    tags = defaultdict(Counter)
    abbrev = Counter()
    abbrev_docs = defaultdict(set)
    bt = Counter()
    bt_docs = defaultdict(set)
    class_count = Counter()
    class_docs = defaultdict(set)
    ip_rows = []
    tag_re = re.compile(r"\[(C|I|U|B:FULL|B:D3D|B:DEBUG|B:EDITOR|B:DEMO|B:LEGACY|B:DEAD)\]")
    ab_re = re.compile(r"\b([A-Z][A-Z0-9]{1,6})\b")

    with open(os.path.join(out_dir, "headings.tsv"), "w", encoding="utf-8") as hout:
        hout.write("doc\tlevel\tline\tanchor\theading\n")
        for nn, name in docs.items():
            path = os.path.join(design_dir, name)
            heads = doc_headings(path)
            for lvl, raw, plain, anchor, no in heads:
                hout.write(f"{nn}\t{lvl}\t{no}\t{anchor}\t{plain}\n")
            # body scan
            cur_head = ""
            in_fence = False
            in_ip = False
            with open(path, encoding="utf-8") as fh:
                for line in fh:
                    if line.startswith("```"):
                        in_fence = not in_fence
                        continue
                    if in_fence:
                        continue
                    hm = re.match(r"^(#{1,6})\s+(.*?)\s*$", line)
                    if hm:
                        cur_head = strip_md(hm.group(2))
                        in_ip = cur_head.strip().lower().startswith("ip items")
                        continue
                    for t in tag_re.findall(line):
                        tags[t][nn] += 1
                    for b in re.findall(r"\*\*([^*\n]{2,80})\*\*", line):
                        key = b.strip()
                        bold[key] += 1
                        bold_first.setdefault(key, (nn, cur_head))
                    stripped = re.sub(r"`[^`]*`", " ", line)
                    for a in ab_re.findall(stripped):
                        abbrev[a] += 1
                        abbrev_docs[a].add(nn)
                    for b in re.findall(r"`([^`\n]{2,60})`", line):
                        bt[b] += 1
                        bt_docs[b].add(nn)
                        for ident in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", b):
                            if ident in class_names:
                                class_count[ident] += 1
                                class_docs[ident].add(nn)
                    if in_ip and line.startswith("|") and not re.match(r"^\|\s*-", line):
                        cells = [c.strip() for c in line.strip().strip("|").split("|")]
                        if cells and cells[0].lower() not in ("item", "term"):
                            ip_rows.append([nn] + cells)

    with open(os.path.join(out_dir, "bold.tsv"), "w", encoding="utf-8") as out:
        out.write("count\tterm\tfirst_doc\tfirst_heading\n")
        for term, cnt in bold.most_common():
            d, h = bold_first[term]
            out.write(f"{cnt}\t{term}\t{d}\t{h}\n")

    with open(os.path.join(out_dir, "tags.tsv"), "w", encoding="utf-8") as out:
        out.write("tag\ttotal\tper_doc\n")
        for t, c in sorted(tags.items()):
            out.write(f"[{t}]\t{sum(c.values())}\t" + ",".join(f"{k}:{v}" for k, v in sorted(c.items())) + "\n")

    with open(os.path.join(out_dir, "abbrev.tsv"), "w", encoding="utf-8") as out:
        out.write("count\tabbrev\tdocs\n")
        for a, c in abbrev.most_common():
            if c >= 2:
                out.write(f"{c}\t{a}\t{','.join(sorted(abbrev_docs[a]))}\n")

    with open(os.path.join(out_dir, "backticks.tsv"), "w", encoding="utf-8") as out:
        out.write("count\tident\tndocs\n")
        for b, c in bt.most_common(3000):
            out.write(f"{c}\t{b}\t{len(bt_docs[b])}\n")

    # best heading per class: in its assigned doc, heading whose plain text
    # starts with the class name (or contains it as a word); else any doc.
    all_heads = {nn: doc_headings(os.path.join(design_dir, name)) for nn, name in docs.items()}
    with open(os.path.join(out_dir, "classes.tsv"), "w", encoding="utf-8") as out:
        out.write("class\tbase\theader\tdoc\tmentions\tndocs\tbest_link\tbest_heading\n")
        for r in classes:
            cname = r["class"]
            best = None
            order = [r["doc"]] + [d for d in docs if d != r["doc"]]
            for d in order:
                if d not in all_heads:
                    continue
                for lvl, raw, plain, anchor, no in all_heads[d]:
                    words = re.findall(r"[A-Za-z0-9_]+", plain)
                    if cname in words:
                        best = (d, anchor, plain)
                        break
                if best:
                    break
            bl = f"{best[0]}#{best[1]}" if best else ""
            bh = best[2] if best else ""
            out.write(f"{cname}\t{r['base']}\t{r['header']}\t{r['doc']}\t{class_count[cname]}\t{len(class_docs[cname])}\t{bl}\t{bh}\n")

    with open(os.path.join(out_dir, "ipitems.tsv"), "w", encoding="utf-8") as out:
        for row in ip_rows:
            out.write("\t".join(row) + "\n")
    print(f"docs={len(docs)} bold={len(bold)} abbrev={len(abbrev)} classes={len(classes)} ip_rows={len(ip_rows)}")


def resolve_link(spec, docs, heads_cache, design_dir):
    spec = spec.strip()
    m = re.match(r"^(\d\d):(.*)$", spec)
    if not m:
        raise ValueError(f"bad link spec {spec!r}")
    nn, sec = m.group(1), m.group(2).strip()
    if nn not in docs:
        raise ValueError(f"unknown doc {nn} in {spec!r}")
    if nn not in heads_cache:
        heads_cache[nn] = doc_headings(os.path.join(design_dir, docs[nn]))
    heads = heads_cache[nn]
    fname = docs[nn]
    if sec == "":
        return f"[{nn}]({fname})"
    if re.match(r"^\d+(\.\d+)*$", sec):
        for lvl, raw, plain, anchor, no in heads:
            if re.match(r"^" + re.escape(sec) + r"(\s|$)", plain.strip()):
                return f"[{nn} §{sec}]({fname}#{anchor})"
        raise ValueError(f"section {sec} not found in {fname}")
    # heading text (case-insensitive prefix match on plain text)
    low = sec.lower()
    for lvl, raw, plain, anchor, no in heads:
        if plain.strip().lower() == low:
            return f"[{nn} {sec}]({fname}#{anchor})"
    for lvl, raw, plain, anchor, no in heads:
        if plain.strip().lower().startswith(low):
            num = re.match(r"^(\d+(\.\d+)*)\s", plain.strip())
            label = f"{nn} §{num.group(1)}" if num else f"{nn} {plain.strip()}"
            return f"[{label}]({fname}#{anchor})"
    raise ValueError(f"heading {sec!r} not found in {fname}")


def esc(cell):
    """Escape table pipes (GFM honours \\| inside code spans in tables)."""
    return cell.replace("|", "\\|")


def sort_key(term):
    t = re.sub(r"^[^A-Za-z0-9]+", "", strip_md(term)).lower()
    return t


def render(design_dir, terms_tsv, out_md):
    docs = list_docs(design_dir)
    heads_cache = {}
    rows = []
    errors = []
    seen = set()
    with open(terms_tsv, encoding="utf-8") as fh:
        for no, line in enumerate(fh, 1):
            line = line.rstrip("\n")
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) != 5:
                errors.append(f"line {no}: expected 5 columns, got {len(parts)}: {line[:80]}")
                continue
            term, kind, definition, links, ip = parts
            key = sort_key(term)
            if key in seen:
                errors.append(f"line {no}: duplicate term {term}")
            seen.add(key)
            rendered = []
            for spec in [s for s in links.split(";") if s.strip()]:
                try:
                    rendered.append(resolve_link(spec, docs, heads_cache, design_dir))
                except ValueError as exc:
                    errors.append(f"line {no}: {exc}")
            rows.append((key, term, kind, definition, ", ".join(rendered), ip))
    if errors:
        for e in errors:
            print("ERROR", e)
        sys.exit(1)
    rows.sort(key=lambda r: r[0])
    groups = defaultdict(list)
    for r in rows:
        first = r[0][:1].upper()
        letter = first if first.isalpha() else "0–9"
        groups[letter].append(r)
    letters = sorted(groups, key=lambda l: ("" if l == "0–9" else l))
    with open(out_md, "w", encoding="utf-8") as out:
        out.write("Jump to: " + " · ".join(f"[{l}](#{slugify(l)})" for l in letters) + "\n\n")
        for l in letters:
            out.write(f"### {l}\n\n")
            out.write("| Term | Kind | Definition | Primary document | IP? |\n|---|---|---|---|---|\n")
            for key, term, kind, definition, link, ip in groups[l]:
                cells = [esc(c) for c in (term, kind, definition)] + [link, esc(ip)]
                out.write("| " + " | ".join(cells) + " |\n")
            out.write("\n")
    kinds = Counter(r[2] for r in rows)
    print(f"rendered {len(rows)} terms; " + ", ".join(f"{k}={v}" for k, v in sorted(kinds.items())))


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "harvest" and len(sys.argv) == 5:
        harvest(sys.argv[2], sys.argv[3], sys.argv[4])
    elif len(sys.argv) >= 2 and sys.argv[1] == "render" and len(sys.argv) == 5:
        render(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print(__doc__)
        sys.exit(2)
