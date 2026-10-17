"""G03b helper: binary64 replay of per-tick accumulations used in defences_and_props.md.
Usage: python3 -I G03b_ticks.py"""
dt = 0.1
def first(pred, step, x0):
    x = x0; n = 0
    while True:
        x = step(x); n += 1
        if pred(x): return n, x
# Barrier Emitter raise: status += 0.3*dt until >= 0.5 (hand-off) and >= 1 (raised)
print('raise handoff', first(lambda s: s >= 0.5, lambda s: s + 0.3*dt, 0.0))
print('raise full', first(lambda s: s >= 1.0, lambda s: s + 0.3*dt, 0.0))
# lowering: check before decrement
s = 1.0; n = 0; hand = None
while True:
    n += 1
    if hand is None and s <= 0.5: hand = n
    s -= 0.3*dt
    if s <= 0: break
print('lower handoff tick', hand, 'lowered tick', n)
# tree burn from 0.01 by 0.02*dt until > 1.0
print('burn', first(lambda p: p > 1.0, lambda p: p + 0.02*dt, 0.01))
# regrow from -1 by 0.05*dt: healthy when > -0.3, full when >= 0
print('regrow healthy', first(lambda p: p > -0.3, lambda p: p + 0.05*dt, -1.0))
print('regrow full', first(lambda p: p >= 0.0, lambda p: p + 0.05*dt, -1.0))
print('leaves visible (|p| <= 0.5)', first(lambda p: abs(p) <= 0.5, lambda p: p + 0.05*dt, -1.0))
# spitter plant shrink from 1.0 by 0.006 until <= 0.3
print('spitter shrink', first(lambda s: s <= 0.3, lambda s: s - 0.006, 1.0))
# corruption count-up from -10 by dt until > 0
print('blight delay', first(lambda t: t > 0, lambda t: t + dt, -10.0))
# countdowns: 5.0, 0.3, 1.0 decremented by dt until <= 0
for T in (0.3, 1.0, 5.0, 15.0):
    print('countdown', T, first(lambda t: t <= 0, lambda t: t - dt, T))
# neighbour ignition: decay 0.1 (clamped at 0) then +1 per tick until > 50
f = 0.0; n = 0
while f <= 50:
    f = max(0.0, f - 0.1); f += 1.0; n += 1
print('neighbour ignition ticks (decay first)', n)
f = 0.0; n = 0
while True:
    f += 1.0; n += 1
    if f > 50: break
    f = max(0.0, f - 0.1)
print('neighbour ignition ticks (damage first)', n)
# turret gun cadence: 2 barrels per tick, 4 ports
print('gun rounds per second per manned port', 10 * 2 / 4)
# jump pad launch: speeds range
import math
for F, a in ((76.17, 0.51), (227.38, 1.08)):
    vy = F*math.sin(a); vh = F*math.cos(a); g = 15.0
    print('jump pad F', F, 'angle', a, 'vy', round(vy,2), 'vh', round(vh,2))
