"""Step the tick clock of reference/12 01 R-SIM-541 and print the A6 vectors.

Usage: python3 -I P6-RE_tickclock.py
Two variants: hold decremented by 1 per tick (integer) with hold H, and the
float form (hold seconds decreased by 0.1 per tick) in float32 and float64.
"""
import struct
import sys


def f32(x):
    return struct.unpack('f', struct.pack('f', x))[0]


def run(start, target, rate, hold, mode, f=lambda x: x, integral='before'):
    s = f(start)
    req = f(target)
    pending = True
    blend = f(rate)
    h = hold
    if mode == 'float' and hold is not None:
        h = f(hold)
    tick_period = f(0.1)
    game = 0.0
    reached = None
    back = None
    tick = 0
    returning = False
    while tick < 10000:
        tick += 1
        s_before = s
        # step 2 (reference arithmetic): match time with speed before blend
        if integral == 'before':
            game += 0.1 * s_before
        # step 3
        if pending:
            s = f(s + f(f(req - s) * blend))
            if abs(s - req) < 0.01:
                s = req
                pending = False
                if not returning and reached is None:
                    reached = tick
                if returning:
                    back = tick
        if integral == 'after':
            game += 0.1 * s
        if returning and back is not None:
            break
        # step 4
        if not pending and h is not None:
            if mode == 'int':
                h -= 1
                done = h <= 0
            else:
                h = f(h - tick_period)
                done = h <= 0
            if done:
                h = None
                req = f(1.0)
                pending = True
                returning = True
        if not pending and h is None and not returning:
            break
    return reached, back, tick, game


def main():
    rows = [
        ('grace end', 0.0, 1.0, 1.0, None),
        ('pause', 1.0, 0.0, 0.2, None),
        ('resume', 0.0, 1.0, 0.2, None),
        ('match won', 1.0, 0.0, 0.2, None),
        ('TD 0.2', 1.0, 0.2, 0.05, 'crate'),
        ('TD 0.3', 1.0, 0.3, 0.05, 'crate'),
        ('TD 0.4', 1.0, 0.4, 0.05, 'crate'),
        ('TR 1.3', 1.0, 1.3, 0.05, 'crate'),
        ('TR 1.65', 1.0, 1.65, 0.05, 'crate'),
        ('TR 2.0', 1.0, 2.0, 0.05, 'crate'),
        ('meteor', 1.0, 0.4, 0.1, 'meteor'),
    ]
    for name, start, target, rate, kind in rows:
        out = [name]
        for label, mode, hold, f, integ in [
            ('int151', 'int', {'crate': 151, 'meteor': 40}.get(kind), lambda x: x, 'before'),
            ('int150', 'int', {'crate': 150, 'meteor': 40}.get(kind), lambda x: x, 'before'),
            ('flt32', 'float', {'crate': 15.0, 'meteor': 4.0}.get(kind), f32, 'before'),
            ('flt64', 'float', {'crate': 15.0, 'meteor': 4.0}.get(kind), lambda x: x, 'before'),
            ('int151-after', 'int', {'crate': 151, 'meteor': 40}.get(kind), lambda x: x, 'after'),
        ]:
            r, b, t, g = run(start, target, rate, hold, mode, f, integ)
            out.append(f'{label}: reached {r} back {b} game {g:.4f}')
        print(' | '.join(out))


if __name__ == '__main__':
    sys.exit(main())
