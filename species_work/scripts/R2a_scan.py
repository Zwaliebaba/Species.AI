"""R2a helper: scan the R2a review scope for cleanliness leads the gates do not cover.

Usage: python3 -I R2a_scan.py <design_root> <out_file>

Writes one line per hit: <class>\t<file>:<line>\t<pattern>\t<snippet>.
The leads are candidates only; every one is checked by reading the document.
"""
import re
import sys
from pathlib import Path

SCOPE = ["reference", "glossary", "CONVENTIONS.md", "README.md", "REQUIREMENTS_INDEX.md"]

PATTERNS = [
    # paraphrased history
    ("history", r"\bas before\b", "i"),
    ("history", r"\bpreviously\b", "i"),
    ("history", r"\bformerly\b", "i"),
    ("history", r"\bearlier (version|build|release|game|title|edition)s?\b", "i"),
    ("history", r"\b(old|older|new|newer|later|previous|first) (version|build|release|edition|patch|update)s?\b", "i"),
    ("history", r"\bonce (was|were|had)\b", "i"),
    ("history", r"\bno longer\b", "i"),
    ("history", r"\bhistoric(al|ally)?\b", "i"),
    ("history", r"\bsequel\b|\bprequel\b|\bpredecessor\b|\bsuccessor game\b", "i"),
    ("history", r"\bcampaign\b", "i"),
    ("history", r"\bin the reference (implementation|code|engine|build|binary|executable|game's code)\b", "i"),
    ("history", r"\breference implementation\b", "i"),
    ("history", r"\bthe reference (engine|renderer|code|source|executable|binary)\b", "i"),
    ("history", r"\bbackward[- ]compat\w*\b|\bbackwards[- ]compat\w*\b", "i"),
    ("history", r"\bdeprecated\b|\bobsolete\b|\bretired\b|\boutdated\b|\bvintage\b|\bclassic\b", "i"),
    ("history", r"\bpatch(ed|es)?\b", "i"),
    ("history", r"\bremaster\w*\b|\bremake\b|\bre-?release\w*\b|\bupdate \d\b|\bv\d+\.\d+\b", "i"),
    ("history", r"\bdead code\b|\bunreachable\b|\bcut content\b|\bnever (shipped|used|reached)\b|\bunused\b", "i"),
    ("history", r"\bbug\b|\bbugs\b|\bbuggy\b|\bquirk\w*\b|\bworkaround\b|\bhack\b", "i"),
    ("history", r"\bdisassembl\w*\b|\bdecompil\w*\b|\breverse[- ]engineer\w*\b|\bforensic\b", "i"),
    ("history", r"\bcode (comment|path|base)s?\b|\bcodebase\b|\bin code\b|\bthe code\b|\bhard-?coded\b|\bhard-?wired\b", "i"),
    ("history", r"\bmisspel\w*\b|\btypo\b|\bsic\b", "i"),
    ("history", r"\bstudio\b|\bpublisher\b|\bdeveloper'?s\b", "i"),
    ("history", r"\binherit(ed|s)? from\b", "i"),
    # conversion-run internals
    ("internals", r"\bORIG\b|\bNEXT\b|\bWORK\b", ""),
    ("internals", r"\bconversion\b|\bledger\b|\brouting\b|\brouted\b|\bwork package\b|\bpackage\b|\bagent\b|\bsubagent\b|\bcoordinator\b|\bcutover\b|\bwave\b", "i"),
    ("internals", r"\bphase \d\b|\bPhase \d\b|\bG[0-9]\b|\bF[0-9][a-z]?\b|\bP[0-9][a-z]?\b", ""),
    ("internals", r"\b(RA|RB|FX|X|RES|C|G|F|P|R)-[A-Z0-9]{1,6}(-[a-z0-9]+)?\b", ""),
    ("internals", r"\bbrief\b|\breport\b|\bgate\b|\bgates\b|\breviewer\b", "i"),
    ("internals", r"\b90 §|\b40 §|\b41 §|\b21 §|\b§ ?\d+\.\d+ of\b", ""),
    ("internals", r"\bclean[- ]?(break|room|form|library)\b|\bprovenance\b|\bdenylist\b|\brename map\b", "i"),
    # legacy technology in disguise
    ("legacy", r"\bDirectX\b|\bDirect3D ?(8|9|10|11)\b|\bD3D ?(8|9|10|11)\b|\bDirectInput\b|\bDirectDraw\b|\bDirectPlay\b|\bDirectMusic\b|\bXInput\b|\bXACT\b|\bWinMM\b|\bMCI\b|\bGDI\b|\bOpenAL\b|\bFMOD\b|\bMiles\b|\bBink\b", ""),
    ("legacy", r"\b32-?bit\b|\b16-?bit\b|\bx86\b|\bWin32\b|\bMMX\b|\bSSE\d?\b|\bFPU\b|\bpalettised\b|\bpalettized\b|\b256[- ]colou?r\b|\bindexed colou?r\b", "i"),
    ("legacy", r"\bvertex arrays?\b|\bvertex buffer objects?\b|\bVBOs?\b|\bmatrix stack\b|\bglBegin\b|\btexture environment\b|\bfixed pipeline\b|\bpixel format\b|\bswap interval\b|\bvsync\b|\baccumulation buffer\b|\bstencil\b|\bselection buffer\b|\bfeedback mode\b", "i"),
    ("legacy", r"\bregistry\b|\b\.ini\b|\bINI file\b|\bMy Documents\b|\bApplication Data\b|\bProgram Files\b|\bGame Explorer\b|\bGames for Windows\b|\bWindows (XP|Vista|7|98|95|2000|ME)\b|\bXP\b|\bVista\b", ""),
    ("legacy", r"\bSteam\b|\bXbox\b|\bXBLA\b|\bLive Arcade\b|\bGameSpy\b|\bPunkBuster\b|\bFTP\b|\bCD[- ]?key\b|\bserial key\b|\bactivation key\b|\bDRM\b|\bpiracy\b|\bpirat\w+\b", "i"),
    ("legacy", r"\b\.ogg\b|\bOgg\b|\bVorbis\b|\b\.bmp\b|\bBMP\b|\b\.tga\b|\bTGA\b|\bWAV\b|\bMP3\b|\bMIDI\b|\btracker music\b|\bchiptune\b|\bSID\b", ""),
    ("legacy", r"\bDSound\b|\bdsound\b|\bI3DL2\b|\bWaves ?Reverb\b|\bgargle\b|\bEAX\b|\bA3D\b|\bHRTF\b", "i"),
    ("legacy", r"\bfloppy\b|\bCD-ROM\b|\bdial-?up\b|\bmodem\b|\bLAN party\b|\bIPX\b|\bNetBIOS\b|\bCRT\b|\b4:3\b|\b640 ?x ?480\b|\b800 ?x ?600\b|\b1024 ?x ?768\b", "i"),
    # fiction, film and literature cues
    ("fiction", r"\bsmell\b|\bin the morning\b|\bflattest\b|\bfeed me\b|\btake me to your\b|\bwe come in peace\b|\bresistance is futile\b|\ball your base\b|\bphone home\b|\bexterminate\b|\bhasta la vista\b|\bI'll be back\b|\bgame over,? man\b|\bopen the pod bay\b|\bmay the force\b|\bbeam me up\b|\bklaatu\b|\binvaders?\b|\bwar of the worlds\b|\bmartian\w*\b|\bday of the\b|\bzombie\w*\b|\bghost in\b|\bmotherboard\b|\bcyberspace\b|\bdigital (world|life|lifeform|people)\b|\bsentient\b|\bprogram(me)? (code|life)\b|\bvirus\w*\b|\bsoul\w*\b|\bspirit\w*\b|\bdoctor\b|\bDr\.? [A-Z]\w+\b|\bmentor\b|\bnarrator\b|\badvisor\b", "i"),
    ("fiction", r"\bChristmas\b|\bSanta\b|\bXmas\b|\bHalloween\b|\bEaster\b", "i"),
    ("fiction", r"\bnuke\w*\b|\bweapons? of mass\b", "i"),
    ("fiction", r"\bquote\b|\bquotation\b|\bfilm\b|\bmovie\b|\bnovel\b|\bpoem\b|\bsong\b|\blyric\w*\b|\bhomage\b|\bpastiche\b|\bparody\b|\ballusion\b|\bpun\b|\bin-joke\b|\beaster egg\b", "i"),
    # world-game design creep
    ("world", r"\bsettlement\w*\b|\bpersistent\b|\bworld game\b|\bthe world\b|\bserver-run\b|\bMMO\w*\b|\bspecies\b(?! design| reference| engine| string| term| name| vocabulary| identifier| game\b)", "i"),
    ("world", r"\bshould Species\b|\bSpecies (should|could|may|might|will|would)\b|\bthe design phase (should|could|may|might|will)\b|\brecommend\w*\b|\bwe suggest\b|\bconsider (using|adopting|dropping)\b|\badopt(ion|s|ed)?\b|\bfor Species\b|\bin Species\b", "i"),
    ("world", r"\bsubspecies\b|\bevolution\b|\bevolv\w+\b|\bgenome\b|\bgenetic\w*\b|\bmutation\w*\b|\bbreed\w*\b|\btrait\w*\b|\bterritor\w+\b|\bdiplomac\w*\b|\btrade route\w*\b|\bseason(s|al)?\b|\bpersist\w*\b|\boffline progress\b|\bshard\w*\b", "i"),
]

# spelling-pair checks (term spelled two ways)
PAIRS = [
    (r"\bEssence [Oo]rb", None),
    (r"\bSpawn [Nn]ode", None),
    (r"\bSupply [Cc]rate", None),
    (r"\bStrike [Tt]eam", None),
    (r"\bPower [Ss]lot", None),
    (r"\bCatch-[Uu]p [Bb]onus", None),
    (r"\bComeback [Bb]onus", None),
    (r"\b[Rr]ogue [Ff]action", None),
    (r"\b[Vv]isitor [Ff]action", None),
    (r"\b[Ss]ource [Nn]ode", None),
    (r"\b[Bb]eam [Dd]ish", None),
    (r"\b[Bb]arrier [Ee]mitter", None),
    (r"\b[Ss]olar [Aa]rray", None),
    (r"\b[Cc]ommand [Cc]onsole", None),
    (r"\b[Cc]ore [Dd]evice", None),
    (r"\b[Ll]aunch [Vv]essel", None),
    (r"\b[Tt]ransit [Gg]ate", None),
    (r"\b[Bb]reach [Cc]harge", None),
    (r"\b[Rr]elic [Bb]ase", None),
    (r"\b[Rr]elic [Ss]pawn [Zz]one", None),
    (r"\b[Cc]ontrol [Zz]one", None),
    (r"\b[Ff]lag [Pp]ost", None),
    (r"\b[Hh]ome [Pp]ost", None),
    (r"\b[Pp]opulation [Ll]imiter", None),
    (r"\b[Ss]pitter [Pp]lant", None),
    (r"\b[Ss]eed [Pp]od", None),
    (r"\b[Bb]rood [Dd]rifter", None),
    (r"\b[Rr]eaper [Ss]erpent", None),
    (r"\b[Cc]rawler [Ee]gg", None),
    (r"\b[Ww]aypoint [Bb]eacon", None),
    (r"\b[Oo]rder [Mm]arker", None),
    (r"\b[Ff]uel [Dd]epot", None),
    (r"\b[Ff]uel [Pp]ump", None),
    (r"\b[Ff]uel [Ll]ine", None),
    (r"\b[Ee]nergy [Gg]rid", None),
    (r"\b[Ee]nergy [Ss]urge", None),
    (r"\b[Gg]rid [Pp]ylon", None),
    (r"\b[Gg]rid [Ss]plitter", None),
    (r"\b[Gg]rid [Tt]erminal", None),
    (r"\b[Ss]pawn [Nn]etwork", None),
    (r"\b[Ee]xclusion [Zz]one", None),
    (r"\b[Jj]ump [Pp]ad", None),
    (r"\b[Gg]un [Tt]urret", None),
    (r"\b[Rr]ocket [Tt]urret", None),
    (r"\b[Ff]lame [Tt]urret", None),
    (r"\b[Mm]ortar [Tt]urret", None),
    (r"\b[Cc]onvert [Tt]urret", None),
    (r"\b[Ff]irebomb [Rr]un", None),
    (r"\b[Mm]issile [Bb]arrage", None),
    (r"\b[Mm]eteor [Ss]hower", None),
    (r"\b[Ss]peed [Bb]oost", None),
    (r"\b[Ss]pawn [Ss]urge", None),
    (r"\b[Rr]ocket [Tt]roops", None),
    (r"\b[Ss]upply [Ss]torm", None),
    (r"\b[Tt]ime [Dd]ilation", None),
    (r"\b[Tt]ime [Rr]ush", None),
    (r"\b[Ee]ssence [Gg]rove", None),
    (r"\b[Bb]light [Gg]rove", None),
    (r"\b[Aa]llegiance [Ss]crambler", None),
    (r"\b[Ss]warm [Ss]treak", None),
    (r"\b[Ss]udden [Dd]eath", None),
    (r"\b[Gg]race [Pp]eriod", None),
    (r"\b[Kk]ill [Mm]ilestone", None),
    (r"\b[Ff]lag [Cc]hain", None),
    (r"\b[Ll]aunch [Rr]ace", None),
    (r"\b[Rr]elic [Rr]un", None),
    (r"\b[Ss]pawn [Cc]ontrol", None),
    (r"\b[Zz]one [Cc]ontrol", None),
    (r"\b[Ss]kill [Rr]ating", None),
    (r"\b[Ii]ron [Dd]efen[cs]e", None),
    (r"\b[Gg]iant [Ss]layer", None),
    (r"\b[Ff]lag [Ss]weep", None),
    (r"\b[Ff]riendly [Ff]ire", None),
    (r"\b[Tt]otal [Cc]ontrol", None),
    (r"\b[Cc]lean [Ll]aunch", None),
    (r"\b[Tt]ransport [Cc]ap", None),
    (r"\b[Ss]trike-team", None),
    (r"\b[Ee]ssence-orb", None),
    (r"\b[Ss]upply-crate", None),
    (r"\b[Ss]pawn-[Nn]ode", None),
    (r"\b[Pp]ower-slot", None),
]


def scope_files(root: Path):
    for item in SCOPE:
        p = root / item
        if p.is_file():
            yield p
        elif p.is_dir():
            yield from sorted(p.rglob("*.md"))


def main():
    root = Path(sys.argv[1])
    out = Path(sys.argv[2])
    compiled = [(c, re.compile(p, re.I if "i" in f else 0), p) for c, p, f in PATTERNS]
    pairs = [re.compile(p) for p, _ in PAIRS]
    lines_out = []
    pair_counts = {}
    for f in scope_files(root):
        rel = f.relative_to(root)
        for n, line in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
            for c, rx, p in compiled:
                for m in rx.finditer(line):
                    s = max(0, m.start() - 90)
                    e = min(len(line), m.end() + 90)
                    lines_out.append(f"{c}\t{rel}:{n}\t{m.group(0)}\t{line[s:e]}")
            for rx in pairs:
                for m in rx.finditer(line):
                    key = (rx.pattern, m.group(0))
                    pair_counts.setdefault(key, []).append(f"{rel}:{n}")
    with out.open("w", encoding="utf-8") as fh:
        fh.write("\n".join(lines_out))
        fh.write("\n\n# SPELLING VARIANTS\n")
        by_pat = {}
        for (pat, form), locs in pair_counts.items():
            by_pat.setdefault(pat, {})[form] = locs
        for pat, forms in sorted(by_pat.items()):
            if len(forms) > 1:
                fh.write(f"{pat}\n")
                for form, locs in sorted(forms.items(), key=lambda kv: -len(kv[1])):
                    fh.write(f"  {form!r}: {len(locs)}  e.g. {', '.join(locs[:6])}\n")


if __name__ == "__main__":
    main()
