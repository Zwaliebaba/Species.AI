# Phase 2 completion log (coordinator notes)

## P09_control_taskmanager — done
- 29 task types, 4 states, 6 slots MP (2 when eliminated); MW interface is the only live variant (Gestures/Icons unreachable); Rubine-style gesture recogniser (13 features, 8 symbols, Python port validated on 8 demos); 9 cursor types. [U]=5.
- Critical: MW task bar not clickable (keys/bumpers/world-click only); task id sent as 1 byte (256th+ task misaddressed); UI/render writes sim state (crosshair `=` vs `==` overwrites building type; local-only m_mostRecentTaskId reset; dormant local terminate paths).

## P16_sound — done
- 27.1–27.15 engine, 10.1–10.11 design, 09.1 sound options; ~33 engine + ~40 editor classes; 68 sources × 296 event blocks; 130 sample groups; 14 DSP types; 8 options. [U]=5.
- Critical: music likely never audible in MW [I] (triggers compiled out under USE_LOADERS or in Darwinia scripts — confirm with P19/P20); object-attached sounds hard-cut on object removal; new voice replaces quietest without priority compare; DSound vs software back-end volume curves differ (5 dB vs 4.34 dB per unit).
