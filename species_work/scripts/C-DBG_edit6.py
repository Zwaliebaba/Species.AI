"""C-DBG edit pass 6: fold in the C-BLD requests. Usage: python3 -I C-DBG_edit6.py <file>"""
import sys
p = sys.argv[1]
t = open(p, encoding='utf-8').read()
reps = [
 ("CI runs it nightly on the headless server product at maximum speed, on every mode and map of each edition (DEC-220 item 3);",
  "CI runs it nightly at maximum speed as the job `ci-nightly-soak`, on the headless `server` product with the `soak` edition, which holds every mode and map of the full edition and the soak and stress test maps ([build tooling](build_tooling_and_distribution.md) tables 3.2-D and 4.9-A, BLD-057; DEC-220 item 3);"),
 ("**DBG-208** (MUST) Performance tests run on the reference test machine as nightly CI jobs (ADR-0070) and report against these budgets:",
  "**DBG-208** (MUST) Performance tests run on the reference test machine in the nightly CI job `ci-nightly-performance`, together with the build budgets of [build tooling](build_tooling_and_distribution.md) table 5-A (BLD-117, ADR-0070), and report against these budgets:"),
 ("until it is chosen, CI records the hardware of the agent that ran each performance job with the result.",
  "until it is chosen, CI records the hardware of the agent that ran each performance job with the result. The reference developer machine, on which the build budgets of build tooling table 5-A are measured, is `dbg.machine.developer_class`: the reference test machine's processor and memory with a local solid-state disk."),
 ("| `dbg.stress.spawn_nodes` | 24 | Spawn Nodes | stress map | DBG-204 | DEC-228 |",
  "| `dbg.stress.spawn_nodes` | 24 | Spawn Nodes | stress map | DBG-204 | DEC-228 |\n| `dbg.machine.developer_class` | the reference test machine with a local solid-state disk | machine class | Species; exact hardware a design-phase input; BLD table 5-A | DBG-209 | DEC-228 |"),
 ("the adapter description and the date and time of the start of the run (ADR-0067).",
  "the adapter description, the channel tag (BLD-021) and the date and time of the start of the run (ADR-0067)."),
 ("It never holds chat text, account tokens or the names of other players (ADR-0065, ADR-0067).",
  "It never holds chat text, account tokens or the names of other players (ADR-0065, ADR-0067), and it does not hold the channel tag, which reaches the log header only (BLD-021, BLD-120)."),
]
for a, b in reps:
    n = t.count(a)
    if n != 1:
        print('MISS', n, a[:80]); continue
    t = t.replace(a, b)
open(p, 'w', encoding='utf-8').write(t)
print('ok', len(t.encode('utf-8')))
