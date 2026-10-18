"""A6 vectors of reference/12 01 under tick scheduling (R-SIM-541, R-SIM-548, R-SIM-549).

Usage: python3 -I P6-RE_a6_species.py [initial_carry]
Model: the request is made in command frame 0; from frame 1 on, each command frame
blends once (snap within 0.01); the frame then runs floor(I_k) - floor(I_{k-1}) world
ticks, I_k the running sum of the speed after each frame's blend, from the carry given;
each world tick that ends with no request pending and a hold set decreases the hold
by 1, and at 0 requests speed 1.0 at the ended rate, blended from the next frame.
"""
import math
import sys


def run(start, target, rate, hold, carry):
    s, req, pending, h = start, target, True, hold
    integ = carry
    ticks_total = 0
    reached = expired = back = None
    returning = False
    for k in range(1, 100000):
        if pending:
            s = s + (req - s) * rate
            if abs(s - req) < 0.01:
                s = req
                pending = False
                if returning:
                    back = k
                elif reached is None:
                    reached = k
        held = s < 0.01
        w = 0
        if not held:
            before = math.floor(integ)
            integ += s
            w = math.floor(integ) - before
        for _ in range(w):
            ticks_total += 1
            if not pending and h is not None:
                h -= 1
                if h <= 0:
                    h = None
                    expired = k
                    req, pending, returning = 1.0, True, True
        if back is not None:
            return reached, expired, back, ticks_total, integ - carry
        if not pending and h is None and not returning:
            return reached, expired, back, ticks_total, integ - carry
    raise RuntimeError('no end')


ROWS = [
    ('grace end', 0.0, 1.0, 1.0, None),
    ('pause', 1.0, 0.0, 0.2, None),
    ('resume', 0.0, 1.0, 0.2, None),
    ('match won', 1.0, 0.0, 0.2, None),
    ('Time Dilation 0.2', 1.0, 0.2, 0.05, 151),
    ('Time Dilation 0.3', 1.0, 0.3, 0.05, 151),
    ('Time Dilation 0.4', 1.0, 0.4, 0.05, 151),
    ('Time Rush 1.3', 1.0, 1.3, 0.05, 151),
    ('Time Rush 1.65', 1.0, 1.65, 0.05, 151),
    ('Time Rush 2.0', 1.0, 2.0, 0.05, 151),
    ('meteor', 1.0, 0.4, 0.1, 40),
]


def main(argv):
    carries = [float(a) for a in argv[1:]] or [0.0]
    for c in carries:
        print(f'carry {c}')
        for name, a, b, r, h in ROWS:
            reached, expired, back, ticks, integ = run(a, b, r, h, c)
            print(f'  {name:18} reached {reached} expired {expired} back {back} world ticks {ticks} integral {integ:.4f}')


if __name__ == '__main__':
    sys.exit(main(sys.argv))
