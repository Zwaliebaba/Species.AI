"""FX-09 helper: split architecture/components/audio_engine.md into the folder audio_engine/.

Usage: python3 -I FX-09_split_audio.py <components_dir>

Writes <components_dir>/audio_engine/README.md and two sub-topic files, keeping every heading,
requirement id and key, and rewrites relative links for the new folder depth. Removes the old
file only after the three new files are written. Section numbers stay those of the component.
"""
import os
import re
import sys

SUB_A = "instances_voices_and_envelopes.md"
SUB_B = "spatialisation_mixing_and_streaming.md"
A_SECTIONS = ["4.5", "4.6", "4.7", "4.8", "4.9", "4.10", "4.11", "4.12", "4.13"]
B_SECTIONS = ["4.14", "4.15", "4.16", "4.17", "4.18", "4.19"]

LINK = re.compile(r"\]\(([^)#\s]*)(#[^)\s]*)?\)")


def relink(text: str) -> str:
    def fix(m: re.Match) -> str:
        target, anchor = m.group(1), m.group(2) or ""
        if not target or target.startswith(("http:", "https:")):
            return m.group(0)
        if target.startswith("../"):
            new = "../" + target
        elif target == "diagnostics_test_and_performance.md":
            new = "../diagnostics_test_and_performance/README.md"
        elif target == "terrain.md":
            new = "../terrain/README.md"
        else:
            new = "../" + target
        return f"]({new}{anchor})"

    return LINK.sub(fix, text)


def split_sections(lines):
    """Return (preamble, list of (number, heading line, body lines)) for ### 4.x sections and others."""
    blocks = []
    current = {"head": None, "lines": []}
    for line in lines:
        if line.startswith("## ") or line.startswith("### "):
            blocks.append(current)
            current = {"head": line, "lines": [line]}
        else:
            current["lines"].append(line)
    blocks.append(current)
    return blocks


def number_of(head):
    if head is None:
        return None
    m = re.match(r"#{2,3} (\d+(?:\.\d+)?) ", head)
    return m.group(1) if m else None


def main() -> int:
    comp = sys.argv[1]
    src = os.path.join(comp, "audio_engine.md")
    with open(src, encoding="utf-8") as f:
        lines = f.read().split("\n")
    blocks = split_sections(lines)
    readme, part_a, part_b = [], [], []
    for b in blocks:
        n = number_of(b["head"])
        if n in A_SECTIONS:
            part_a.extend(b["lines"])
        elif n in B_SECTIONS:
            part_b.extend(b["lines"])
        else:
            readme.extend(b["lines"])
    out_dir = os.path.join(comp, "audio_engine")
    os.makedirs(out_dir, exist_ok=True)

    readme_text = relink("\n".join(readme))
    index = (
        "## Folder index\n\n"
        "| File | Holds | Requirement IDs |\n"
        "|---|---|---|\n"
        "| [README.md](README.md) (this file) | responsibilities and boundaries, the interface sketch, the commands and the "
        "threading contract, the inputs, the voice back-end contract, the data model, the lifecycle, the trigger gate, "
        "the update schedule, the data flow, the tuning table, budgets, failure modes, the acceptance tests and the open "
        "decisions | AEN-001 to AEN-124, AEN-264 to AEN-265, AEN-300 to AEN-399 |\n"
        f"| [{SUB_A}]({SUB_A}) | §4.5 to §4.13: from a command to an instance, ranking, virtual voices and stealing, the "
        "reference allocation and emitter selection, instance states, envelope arithmetic, play position and loops, "
        "start and advance, modulatable values, pitch and the game-speed bend | AEN-130 to AEN-199 |\n"
        f"| [{SUB_B}]({SUB_B}) | §4.14 to §4.19: spatialisation, the volume law and gain stages, buses, routing and "
        "effects, sample residency, streaming, device loss and restart | AEN-200 to AEN-263 |\n\n"
        "Section numbers are those of the whole component: a section cited as §4.16 is in the file that the index names "
        "for it.\n\n"
    )
    readme_text = readme_text.replace("## 1 Responsibilities and boundaries", index + "## 1 Responsibilities and boundaries", 1)

    def sub_file(title, scope, body, adrs, related, tests):
        head = (
            f"# {title}\n\n"
            f"> Status: Engine baseline · Prefix: AEN · ADRs: {adrs} · Related: [Audio engine](README.md) · {related}\n\n"
            f"This document specifies {scope} The notation and the units are those of the [folder README](README.md), "
            "and section numbers are those of the whole component.\n\n"
            "## 1 Responsibilities and boundaries\n\n"
            "The responsibilities and boundaries of the audio engine are [README](README.md) §1.\n\n"
            "## 2 Interfaces\n\n"
            "The interface sketch, the commands, the inputs and the voice back-end contract are [README](README.md) §2.\n\n"
            "## 3 Data model\n\n"
            "The types, the layer data, the instance state, the effect kinds and the stream format are [README](README.md) §3.\n\n"
            "## 4 Behaviour and algorithms\n\n"
        )
        tail = (
            "\n## 5 Concurrency and performance budget\n\n"
            "The threads, the budgets and the memory figures are [README](README.md) §5.\n\n"
            "## 6 Failure modes, diagnostics and telemetry\n\n"
            "The failure table, the silent device and the diagnostics are [README](README.md) §6.\n\n"
            "## 7 Acceptance tests\n\n"
            f"The tests of this document are in [README](README.md) §7: {tests}.\n\n"
            "## 8 Open decisions\n\n"
            "The decisions and architecture records are [README](README.md) §8.1.\n"
        )
        return head + relink("\n".join(body)).rstrip("\n") + "\n" + tail

    adr = lambda n, slug: f"[ADR-{n}](../../../ADR/ADR-{n}-{slug}.md)"
    a_text = sub_file(
        "Instances, voices and envelopes",
        "how a command becomes an instance and how instances live: the trigger and stop paths, the ranking onto the "
        "real-voice budget with virtual voices and stealing, the reference allocation, the instance states, the "
        "envelope, the play position and loops, the start and advance of an instance, the modulatable values and the "
        "game-speed pitch bend.",
        part_a,
        " · ".join([adr("022", "audio-engine-on-xaudio2"), adr("023", "virtual-voices-and-voice-stealing"),
                    adr("026", "audio-command-queue-and-callbacks")]),
        f"[Spatialisation, mixing and streaming]({SUB_B}) · "
        "[event model and sound data](../../../reference/10_audio_design/event_model_and_sound_data.md) · "
        "[positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md)",
        "T4 to T18",
    )
    b_text = sub_file(
        "Spatialisation, mixing and streaming",
        "how a real voice is placed and mixed and where its samples come from: the distance gain and panning, the "
        "volume law and gain stages, the bus graph with its routing and effects, the residency of resident banks, "
        "streaming, and device loss and restart.",
        part_b,
        " · ".join([adr("024", "audio-buses-and-per-bus-effects"), adr("025", "audio-formats-codecs-and-residency"),
                    adr("027", "spatialisation-and-distance-attenuation"), adr("058", "pack-files-and-mount-policy")]),
        f"[Instances, voices and envelopes]({SUB_A}) · "
        "[positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md) · "
        "[asset pipeline and resources](../asset_pipeline_and_resources.md)",
        "T19 to T26 and T28",
    )
    for name, text in (("README.md", readme_text), (SUB_A, a_text), (SUB_B, b_text)):
        with open(os.path.join(out_dir, name), "w", encoding="utf-8", newline="\n") as f:
            f.write(text if text.endswith("\n") else text + "\n")
    os.remove(src)
    for name in ("README.md", SUB_A, SUB_B):
        print(name, os.path.getsize(os.path.join(out_dir, name)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
