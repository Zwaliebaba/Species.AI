"""R2b cleanliness scan: prints candidate hits for manual review.

Usage: python3 -I R2b_scan.py <category> <path> [<path> ...]
Categories: history, internals, legacy, world, quotes, proper, all.
Each hit prints as file:line: [category] match :: line text (trimmed).
The scan is a reading aid only; every hit is judged by hand.
"""

import os
import re
import sys

PATTERNS = {
  "history": [
    r"\bas before\b", r"\boriginal(ly)?\b", r"\bprevious(ly)?\b", r"\bformer(ly)?\b",
    r"\bearlier (version|build|release|engine|code|design|format|scheme|layout)\b",
    r"\bold (version|engine|code|renderer|system|format|build|path|behaviou?r|scheme|layout|name)\b",
    r"\blegacy\b", r"\bport(ed|ing|s)?\b", r"\bused to\b", r"\bno longer\b", r"\bany ?more\b",
    r"\bhistoric(al(ly)?)?\b", r"\bat the time\b", r"\bback then\b", r"\bin the past\b",
    r"\bbackwards?[- ]compat\w*\b", r"\bcarr(y|ied|ies) (over|forward)\b", r"\bkept\b",
    r"\binherit(ed|s)?\b", r"\bretain(ed|s)?\b", r"\bfaithful(ly)?\b", r"\bmirror(s|ed|ing)?\b",
    r"\breference (implementation|code|engine|build|source|renderer|binary|executable|program)\b",
    r"\breference's (code|engine|source|implementation|renderer|build)\b",
    r"\bshipped\b", r"\bretired\b", r"\bdeprecated\b", r"\bobsolete\b", r"\bsuperseded?\b",
    r"\binstead of\b", r"\bunlike\b", r"\breintroduc\w*\b", r"\brestor(e|ed|es)\b",
    r"\brewr(ite|itten|ote)\b", r"\breworked\b", r"\bre-?implement\w*\b", r"\brecreat\w*\b",
    r"\brenam(e|ed|es|ing)\b", r"\bupgrad\w*\b", r"\bmodernis\w*\b", r"\bmoderniz\w*\b",
    r"\bretarget\w*\b", r"\bquirk\w*\b", r"\bbug\b", r"\bbugs\b", r"\bglitch\w*\b",
    r"\bworkaround\w*\b", r"\bhistory\b", r"\bnow\b", r"\btoday\b", r"\bstill\b", r"\bagain\b",
    r"\bfound\b", r"\bdiscovered\b", r"\bturned out\b", r"\bin practice\b", r"\bwhat it was\b",
    r"\bthe reference did\b", r"\bthe reference does\b", r"\bthe reference uses\b",
    r"\bthe reference game\b", r"\bthe reference\b", r"\bfaithful\b", r"\bverbatim\b",
    r"\bclassic\b", r"\bvintage\b", r"\bera\b", r"\bdated\b", r"\boutdated\b",
  ],
  "internals": [
    r"\bpackages?\b", r"\bagents?\b", r"\bphase ?\d\b", r"\bPhase\b", r"\brouting\b", r"\brouted\b",
    r"\bORIG\b", r"\bNEXT\b", r"\bWORK\b", r"species_work", r"design_next", r"\bconversion\b",
    r"\bcoordinator\b", r"\bledger\b", r"\brename[- ]map\b", r"\brecover(y|ed)\b", r"\bcorpus\b",
    r"\bgate G\d\b", r"\bG[1-4]\b", r"\bwave\b", r"\bbriefs?\b", r"\bsubagents?\b",
    r"\bwork package\b", r"\bR[AB]-\d", r"\bFX-\d", r"\bX-[A-Z]{2,}", r"\bRES-\d", r"\bF[1-3][a-c]?\b",
    r"\bC-[A-Z]{2,}", r"\bG\d\d[a-z]?\d?\b", r"\bR[1-4][ab]\b", r"\bpass [12]\b", r"\breports?\b",
    r"\breviewers?\b", r"\breview\b", r"\bfix round\b", r"\bcutover\b", r"\bstaging\b",
    r"\bdesign/", r"\b_work\b", r"\bsource corpus\b", r"\bdec_map\b", r"\bheadings\.csv\b",
    r"\bKEEP\b", r"\bMERGE\b", r"\bDROP\b", r"\bDEFER\b", r"\bcarry[- ]forward\b",
    r"\bverdicts?\b", r"\bdrop topics?\b", r"\bowner questions?\b", r"\bbatch \d\b",
    r"\bP\d{2}[a-z]?\b", r"\bT-row\b", r"\bD\d{1,2}\b", r"\b(4[01]|9[01])_\w+\.md\b", r"\bS\d+\.\d\b",
  ],
  "legacy": [
    r"DirectX ?1[01]", r"\bD3D1[01]\b", r"Direct3D ?1[01]", r"\bDX1[01]\b", r"\bXInput\b",
    r"DirectInput", r"\bdinput", r"\bXACT\b", r"X3DAudio", r"DirectMusic", r"DirectPlay",
    r"DirectDraw", r"DirectShow", r"Media Foundation", r"\bWinMM\b", r"mmsystem",
    r"timeGetTime", r"GetTickCount", r"PlaySound", r"\bMCI\b", r"\bGDI\b", r"fixed[- ]function",
    r"texture stage", r"\bFVF\b", r"vertex buffer object", r"\bVBO\b", r"immediate[- ]mode",
    r"matrix stack", r"alpha[- ]test", r"alpha reference", r"render ?states?\b", r"SetRenderState",
    r"texture environment", r"texenv", r"multi-?texture", r"point sprites?", r"palett(e|ised|ized)d? texture",
    r"\b(8|16|24|32|64)-bit\b", r"\bx86\b", r"\bWin32\b", r"\bWin64\b", r"WOW64", r"LARGEADDRESSAWARE",
    r"address space", r"\bSSE\d?", r"/arch", r"\bMMX\b", r"CMake", r"vcpkg", r"\bConan\b", r"Premake",
    r"\bNinja\b", r"clang-cl", r"\bGCC\b", r"MinGW", r"[Mm]akefile", r"Jenkins", r"AppVeyor", r"Travis",
    r"\bzlib\b", r"libpng", r"FreeType", r"\bstb_", r"TinyXML", r"\bLua\b", r"\bOgg\b", r"Vorbis",
    r"\bOpus\b", r"\bFLAC\b", r"\bMP3\b", r"FMOD", r"Wwise", r"OpenAL", r"PortAudio", r"\bBink\b",
    r"Steam", r"GameSpy", r"Epic Online", r"Xbox Live", r"\bDRM\b", r"CD[- ]key", r"serial key",
    r"activation key", r"registry", r"HKEY", r"\.ini\b", r"INI file", r"\bMFC\b", r"\bATL\b", r"TCHAR",
    r"_UNICODE", r"\bANSI\b", r"code page", r"WinMain", r"Visual Studio 20[01]\d", r"VC\+\+",
    r"\bv14[0-4]\b", r"\.sln\b", r"D3DCompile", r"d3dcompiler", r"Shader Model [1-5]\b",
    r"\b[pv]s_[1-4]_", r"\bCg\b", r"GLSL", r"\bWARP\b", r"[Ff]eature level (9|1[01])",
    r"D3D_FEATURE_LEVEL_(9|1[01])", r"exclusive full ?screen", r"full ?screen exclusive",
    r"display mode change", r"gamma ramp", r"WinSock", r"ws2_32", r"\bIPX\b", r"NAT punch",
    r"master server", r"matchmak\w*", r"CD-?ROM", r"floppy", r"joystick", r"force feedback",
    r"DualShock", r"Xbox 360", r"\bXNA\b", r"\bTGA\b", r"\bBMP\b", r"ADPCM", r"xWMA", r"\bXMA\d?\b",
    r"\bDXT[1-5]\b", r"S3TC", r"power-of-two", r"\bNPOT\b", r"render to texture", r"pbuffer", r"\bFBO\b",
    r"accumulation buffer", r"selection buffer", r"feedback buffer", r"timeBeginPeriod", r"\brdtsc\b",
    r"SetThreadAffinityMask", r"CRITICAL_SECTION", r"\bvolatile\b", r"device lost", r"lost device",
    r"\bReset\(\)", r"D3DPOOL", r"managed pool", r"\bDDS\b", r"\bvertex fog\b", r"\btable fog\b",
    r"\bfog table\b", r"\bfog (start|end)\b", r"\bglBegin\b", r"\bGL\b", r"\bgl[A-Z]", r"OpenGL",
    r"\bARB\b", r"wgl", r"\bglu", r"display list", r"\bCOM\b", r"\bDMO\b", r"gargle", r"Gargle",
    r"I3DL2", r"WavesReverb", r"\bEAX\b", r"A3D", r"\bHRTF\b", r"Sensaura", r"\bMIDI\b", r"\bCDDA\b",
    r"redbook", r"\.NET\b", r"C#", r"\bJava\b", r"Python 2", r"\bC\+\+(98|03|11|14|17|20)\b",
    r"\bstd::auto_ptr\b", r"\bboost\b", r"\bQt\b", r"wxWidgets", r"ImGui", r"Dear ImGui", r"\bSDL\b",
    r"\bGLFW\b", r"\bGLEW\b", r"\bGLUT\b", r"\bDirectXMath\b", r"\bXNAMath\b", r"\bD3DXMath\b",
    r"\bdxerr\b", r"DXUT", r"\bdxguid\b", r"\bd3dx", r"Effects ?11", r"\.fx\b", r"\.fxo\b", r"\.cso\b",
    r"\bHLSL ?[1-5]\b", r"Windows (XP|Vista|7|8|8\.1)\b", r"Windows 10", r"\bXP\b", r"Vista",
    r"\bNT\b", r"\bDOS\b", r"\bUAC\b", r"Program Files \(x86\)", r"My Documents", r"Application Data",
    r"\bAppData\b", r"\bCSIDL", r"SHGetFolderPath", r"\bMAX_PATH\b", r"8\.3", r"long file names?",
    r"\bWinHTTP\b", r"\bWinINet\b", r"\blibcurl\b", r"\bcurl\b", r"OpenSSL", r"Schannel",
    r"\bXAudio2 ?2\.[0-8]\b", r"XAudio2_7", r"XAudio2_8", r"\bDXSDK", r"Windows Media", r"\bWMA\b",
    r"\bQuickTime\b", r"Flash", r"\bActiveX\b", r"\bInstallShield\b", r"\bWiX\b", r"\bClickOnce\b",
    r"\bMSIX\b", r"\bMSI\b", r"\bAppX\b", r"\bUWP\b", r"\bWinRT\b", r"\bC\+\+/CX\b", r"\bC\+\+/WinRT\b",
  ],
  "world": [
    r"\bsettlements?\b", r"\bdeputy\b", r"\bAFK\b", r"\braces?\b", r"\bpersistent\b",
    r"\bworld[- ]game\b", r"\bregions?\b", r"\bserver[- ]run\b", r"\bangr(y|er|ered)\b",
    r"\bgrievances?\b", r"\bstanding orders?\b", r"\bstrategic zoom\b", r"\babstract (model|economy)\b",
    r"\bcommunity[- ]host\w*\b", r"\bdedicated server\w*\b", r"\bofficial hosting\b",
    r"\bshield period\b", r"\bprotection rules?\b", r"\bworld state\b", r"\bworld save\b",
    r"\bcheck-in\b", r"\bcompanion\b", r"\bpersistent world\b", r"\bbig world\b", r"\blarge world\b",
    r"\bworld time\b", r"\binhabitants?\b", r"\bSpecies (world|game|design|player|settlement)\b",
    r"\bauthoritative\b", r"\binterest manag\w*\b", r"\breplication\b", r"\b200\+?\b", r"\b16 players\b",
  ],
  "quotes": [
    r"\"[^\"\n]{12,}\"", r"“[^”\n]{8,}”", r"‘[^’\n]{8,}’",
    r"(?<![\w])'[A-Z][^'\n]{10,}'",
  ],
}

PROPER = re.compile(r"(?<![\w`.#/-])([A-Z][a-z]+(?:[ -][A-Z][a-z]+)+)(?![\w`])")


def iter_files(paths):
  for p in paths:
    if os.path.isdir(p):
      for root, _dirs, files in os.walk(p):
        for f in sorted(files):
          if f.endswith((".md", ".csv", ".tsv", ".txt", ".json", ".yml")):
            yield os.path.join(root, f)
    else:
      yield p


def main():
  if len(sys.argv) < 3:
    print(__doc__)
    return 2
  cat = sys.argv[1]
  cats = list(PATTERNS) if cat == "all" else [cat]
  for path in iter_files(sys.argv[2:]):
    try:
      with open(path, encoding="utf-8") as fh:
        lines = fh.read().split("\n")
    except UnicodeDecodeError:
      continue
    in_code = False
    for n, line in enumerate(lines, 1):
      if line.lstrip().startswith("```"):
        in_code = not in_code
      if cat == "proper":
        for m in PROPER.finditer(line):
          print(f"{m.group(1)}\t{path}:{n}")
        continue
      for c in cats:
        if c not in PATTERNS:
          continue
        for pat in PATTERNS[c]:
          flags = 0
          for m in re.finditer(pat, line, flags):
            s = max(0, m.start() - 90)
            e = min(len(line), m.end() + 90)
            print(f"{path}:{n}: [{c}] {m.group(0)!r} :: {line[s:e]}")
  return 0


if __name__ == "__main__":
  sys.exit(main())
