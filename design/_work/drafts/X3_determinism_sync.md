# X3_determinism_sync — Simulation determinism model (RNG, floats, ordering, sim-vs-cosmetic, sync checks)

Scope: cross-cutting over the whole live codebase (`code/**`, `contrib/systemiv/lib/**`, `targets/vs2005/*`). Build baseline: see `build_matrix.md` (`Release|Win32` + `TARGET_PC_FULLGAME`, shipped as `multiwinia.exe`). P15 documents the wire protocol; this draft documents the determinism contract that protocol relies on.

Generated tables: `drafts/X3_syncrand_sites.csv` (every synced-RNG call site, produced by `scripts/X3_rng_sites.py`), `drafts/X3_unsynced_rng_sites.csv` (every unsynced RNG call site with context classification, same script).

(sections are appended below as they are written)
