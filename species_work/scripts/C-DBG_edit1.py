"""C-DBG edit pass 1: replace literal phrases in the target file. Usage: python3 -I C-DBG_edit1.py <file>"""
import sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("`dbg.profile_view.render`", "`devtools.profile_view.render`"),
 ("scatter 0, with a wander range of `dbg.commands.crawler_range` (1000 wu)", "scatter 0, with a roam range of `dbg.commands.crawler_range` (1000 wu)"),
 ("The first accepted debug command sets the session's cheats-used marker, which the session browser, the lobby and the match results show (`ui.match.cheats_used`) and which the match statistics carry, so that no achievement or leaderboard result is recorded for that session ([reference/09](../../reference/09_front_end_and_match_flow/README.md) hooks).",
  "The first accepted debug command sets the session's cheats-used marker, which the lobby and the match results show (`ui.match.cheats_used`) and which the match statistics carry ([reference/09](../../reference/09_front_end_and_match_flow/README.md) hooks)."),
 ("A full-width button is at x = `dbg.commands.margin` (10) and the window width − 20 = 180 px wide; a pair of buttons is each (window width − 25) ÷ 2 = 87 px wide, the second at x = 10 + 87 + `dbg.commands.pair_gap` (5) = 102; the Supply crate row has its button at x = 10, the window width ÷ 2 − 20 = 80 px wide, and the outcome drop-down at x = the window width ÷ 2 = 100, the window width ÷ 2 − 10 = 90 px wide.",
  "With m = `dbg.commands.margin` (10 px) and g = `dbg.commands.pair_gap` (5 px): a full-width button is at x = m and the window width − 2m = 180 px wide; a pair of buttons is each (window width − 2m − g) ÷ 2 = 87 px wide, the second at x = m + 87 + g = 102; the Supply crate row has its button at x = m, the window width ÷ 2 − 2m = 80 px wide, and the outcome drop-down at x = the window width ÷ 2 = 100, the window width ÷ 2 − m = 90 px wide."),
 ("an inset panel (UI-061) at (`dbg.netstats.inset_x`, `dbg.netstats.inset_y`) = (10, 110), as wide as the window less 2 × 10 px and `dbg.netstats.inset_height` (135 px) high, holds the participant table. The window is placed at (`dbg.netstats.margin`, H − its height) = (10, H − h). Text uses the body role, and text lines are `dbg.netstats.line_pitch` (15 px) apart from the window top + 15 px.",
  "an inset panel (UI-061) at (`dbg.netstats.inset_x`, `dbg.netstats.inset_y`) = (10, 110), as wide as the window less 2 × `dbg.netstats.margin` (10 px) and `dbg.netstats.inset_height` (135 px) high, holds the participant table. The window is placed at (`dbg.netstats.margin`, H − its height) = (10, H − h). Text uses the body role, and text lines are `dbg.netstats.line_pitch` (15 px) apart, the first one line pitch below the window top."),
 ("text size `dbg.netstats.host_text_size` (12) at the window's x + 10:", "text size `dbg.netstats.host_text_size` (12) at the window's x + `dbg.netstats.margin`:"),
 ("at the offsets `dbg.netstats.columns` (0, 60, 200, 260 and 410 px) from x + 20, and the divergence column at 500 px.", "at the offsets `dbg.netstats.columns` (0, 60, 200, 260 and 410 px) from x + `dbg.netstats.column_origin` (20 px), and the divergence column at offset `dbg.netstats.divergence_column` (500 px)."),
 ("when at least one frame has been processed, a caption (text size 13) at the window bottom − 50 px states that the ideal queue holds 0 or 1 frames with no gaps; below it a label with the last processed frame number at the bottom − 25 px, and a strip starting at the bottom − 30 px of `dbg.netstats.strip_boxes` (10) boxes for the frames last + 1 to last + 10, each (window width − 120) ÷ 10 = 48 px wide",
  "when at least one frame has been processed, a caption (text size `dbg.netstats.queue_caption_size`, 13) at the window bottom − `dbg.netstats.queue_caption_rise` (50 px) states that the ideal queue holds 0 or 1 frames with no gaps; below it a label with the last processed frame number at the bottom − `dbg.netstats.queue_label_rise` (25 px), and a strip starting at the bottom − `dbg.netstats.strip_rise` (30 px) of `dbg.netstats.strip_boxes` (10) boxes for the frames last + 1 to last + 10, each (window width − `dbg.netstats.strip_side_space` (120 px)) ÷ 10 = 48 px wide"),
 ("and its estimated latency in seconds with one decimal, centred, at text size 15.", "and its estimated latency in seconds with one decimal, centred, at text size `dbg.netstats.latency_text_size` (15)."),
 ("clamped to [105, 255].", "clamped to [`dbg.profile.view.brightness_min`, `dbg.profile.view.brightness_min` + `dbg.profile.view.brightness_range`] = [105, 255]."),
 ("one red vertical line per frame of the frame-time history, newest at the right, of height 0.5 × (frame time ÷ 100 ms) × 100 px, so a frame of `dbg.profile.view.graph_full_ms` (200 ms) fills the graph.",
  "one red vertical line per frame of the frame-time history, newest at the right, of height `dbg.profile.view.graph_height` × frame time ÷ `dbg.profile.view.graph_full_ms` (200 ms), that is 0.5 px per ms, so a 200 ms frame fills the graph."),
 ("the 50th, 95th and 99th percentiles of the frame-time history;", "the percentiles `dbg.telemetry.percentiles` (50th, 95th and 99th) of the frame-time history;"),
 ("the simulation update-time percentiles (50th, 95th and 99th); and the peak population (ADR-0070).", "the simulation update-time percentiles of `dbg.telemetry.percentiles`; and the peak population (ADR-0070)."),
 ("frame-time percentiles (50th, 95th and 99th) rather than a frame-rate integer;", "frame-time percentiles (`dbg.telemetry.percentiles`: 50th, 95th and 99th) rather than a frame-rate integer;"),
 ("for every structure and every one of its ports: `dbg.commands.port_workers` (3) Workers of team slot 0 at the port position, scattered within 30 wu;", "for every structure and every one of its ports: `dbg.commands.port_workers` (3) Workers of team slot 0 at the port position, scattered within `dbg.commands.spawn_spread` (30 wu);"),
 ("Every node is timed whether or not the view shows it.", "Every node is timed whether or not the view shows it; the reference times a node only while its parent is expanded in the view, so a collapsed subtree costs a look-up per call and shows no time."),
 ("The roll-over runs in every frame loop, the front-end loop included.", "The roll-over runs in every frame loop, the front-end loop included; the reference rolls over only in its match loops."),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:80]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok')
