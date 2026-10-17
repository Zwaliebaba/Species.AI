"""S_07: compute how many 0.1 s ticks each AI countdown actually takes.

Models the pattern used throughout code/worldobject/ai.cpp, aiobjective.cpp,
armour.cpp, officer.cpp: each tick `t -= 0.1` (SERVER_ADVANCE_PERIOD at speed 1.0),
then either `if (t > 0) return;` (fires when t <= 0) or `if (t <= 0)` (same).
IEEE-754 double arithmetic (x87 is forced to 53-bit mantissa by
StandardiseCPUPrecision, so results match SSE2 doubles for +/-).
Usage: python3 -I S_07_timer_ticks.py
"""
import sys

def ticks(start, step=0.1):
    t = start
    n = 0
    while True:
        t -= step
        n += 1
        if t <= 0.0:
            return n
        if n > 100000:
            return None

def main():
    for v in (1.0, 2.0, 3.0, 5.0, 10.0, 20.0):
        print(f"{v:5.1f} s timer -> fires after {ticks(v)} ticks")

if __name__ == "__main__":
    sys.exit(main())
