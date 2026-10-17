# Replays reference float countdowns (binary64) to get exact tick-count distributions.
import sys
from collections import Counter

def count(x, dt=0.1):
    n = 0
    while True:
        x -= dt
        n += 1
        if x <= 0:
            return n

def dist(base, span, samples=200000):
    c = Counter()
    for i in range(samples):
        u = span * (i + 0.5) / samples
        c[count(base + u)] += 1
    tot = sum(c.values())
    keys = sorted(c)
    mean = sum(k * v for k, v in c.items()) / tot
    return keys[0], keys[-1], mean, {k: round(c[k] / tot, 4) for k in keys}

for name, b, s in [("first_eval", 0.0, 2.0), ("later_eval", 2.0, 1.0), ("initial_spawn", 2.0, 2.0),
                   ("pause", 10.0, 5.0), ("floating", 30.0, 20.0), ("worker_retarget", 1.0, 1.0),
                   ("limiter_recount", 9.0, 2.0)]:
    lo, hi, mean, d = dist(b, s)
    print(name, lo, hi, round(mean, 3))
    print("   ", d)
for v in [0.2, 1.0, 2.0, 3.0, 5.0, 6.0, 10.0, 15.0, 30.0, 45.0, 90.0, 9.0, 11.0, 50.0]:
    print("fixed", v, count(v))
# exact edge u=0
print("u=0 cases", count(0.0 + 1e-12), count(2.0), count(10.0))
# relay transit: progress += 0.1 until >= 1.0, overshoot carried
p = 0.0; ticks = []; n = 0
for hop in range(5):
    n = 0
    while True:
        p += 0.1; n += 1
        if p >= 1.0:
            p -= 1.0
            break
    ticks.append(n)
print("relay hops", ticks)
