# G02b: tick counts of fixed reference durations under binary64 accumulation (DEC-241 item 3).
import sys
def countdown(x, dt=0.1):
    n = 0
    while True:
        x -= dt; n += 1
        if x <= 0: return n
def countup_gt(x, dt=0.1):
    t = 0.0; n = 0
    while True:
        t += dt; n += 1
        if t > x: return n
def countup_ge(x, dt=0.1):
    t = 0.0; n = 0
    while True:
        t += dt; n += 1
        if t >= x: return n
for v in [0.5, 1.0, 1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 10.0, 15.0, 60.0, 100.0, 120.0, 600.0]:
    print(v, 'countdown', countdown(v), 'up>', countup_gt(v), 'up>=', countup_ge(v))
