#!/usr/bin/env python3
"""Write species_work/dec_map_X-DEC-5b.csv for the X-DEC-5b register entries.

Usage: python3 -I X-DEC-5b_decmap.py <out.csv>
"""
import csv
import sys

ROWS = [
    {
        "dec_id": "DEC-750",
        "title": "Explainer-animation loop and visibility rules",
        "status": "Open",
        "affects_prefix": "VFX;R-META;DAT",
        "affects_files": ";".join([
            "architecture/components/animation_and_vfx.md",
            "reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md",
            "architecture/components/data_schemas.md",
        ]),
        "orig_refs": "26_animation_and_vfx.md:389-401",
    },
    {
        "dec_id": "DEC-751",
        "title": "Game-speed pitch state starts every match at normal speed",
        "status": "Resolved",
        "affects_prefix": "AEN;R-AUD",
        "affects_files": ";".join([
            "architecture/components/audio_engine.md",
            "reference/10_audio_design.md",
        ]),
        "orig_refs": "27_audio_engine.md:722-734",
    },
    {
        "dec_id": "DEC-752",
        "title": "Audio queries and reused voices carry no stale state",
        "status": "Resolved",
        "affects_prefix": "AEN",
        "affects_files": "architecture/components/audio_engine.md",
        "orig_refs": ";".join([
            "27_audio_engine.md:472-507",
            "27_audio_engine.md:923-930",
        ]),
    },
]

FIELDS = ["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"]


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    with open(sys.argv[1], "w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(ROWS)
    return 0


if __name__ == "__main__":
    sys.exit(main())
