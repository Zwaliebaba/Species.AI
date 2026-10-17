# Exact binary64 replay of a few accumulating structure timers (G03a helper).
dt = 0.1
# 1. energy surge hop: percent += dt*2 from 0 until >= 1.0
p = 0.0; n = 0
while True:
    p += dt * 2; n += 1
    if p >= 1.0: break
print("surge hop ticks", n, p)
# 2. Ready countdown 10, -= dt until <= 0
c = 10.0; n = 0
while True:
    c -= dt; n += 1
    if c <= 0: break
print("ready ticks", n, c)
# 3. Countdown 10, -= 0.5*dt, clamp >= 0, == 0 -> flight
c = 10.0; n = 0
while True:
    c -= 0.5 * dt
    if c < 0: c = 0.0
    n += 1
    if c == 0.0: break
print("countdown ticks", n)
# 4. destruct timer 10 s -= dt until <= 0
c = 10.0; n = 0
while True:
    c -= dt; n += 1
    if c <= 0: break
print("destruct ticks", n)
# 5. fuel pump: one surge decays by dt per tick from 1.0 to <= 0
s = 1.0; n = 0
while s > 0:
    s -= 1.0 * dt; n += 1
    if s < 0: s = 0
print("one surge lasts ticks", n)
# 6. flag throttle 20.0 -= 0.1 per recount until <= 0
c = 20.0; n = 0
while c > 0:
    c -= dt; n += 1
print("flag throttle recounts", n, c)
# 7. activation 30 s, beam dish timeSync 0.1
c = 0.1 - dt
print("timesync after one tick", c)
# 8. statue/relic timers 1 s pattern (now > t+1): from tau=0
# 9. Launch vessel exploding: damage 100 -> 0 with per tick -0.033 and -0.1 ; empty extra -0.2
import math
d = 100.0; n=0
while d > 0:
    d -= 0.33*dt; d -= 0.1; n+=1
print("exploding ticks (no empty extra)", n)
d = 100.0; n=0
while d > 0:
    d -= 0.33*dt; d -= 0.1; d -= 0.2; n+=1
print("exploding ticks (empty from start)", n)
# 10. Breach / launch vessel refuel: max steady rate
print("refuel steady", 3*1.0*0.4)
# 11. fuel EMA: f = 0.2*dt
print("fuel ema f", 0.2*dt, 3*0.2*dt)
# 12. Launch Vessel refuel from fuel 0, rate 0, target 1.2 per second
fuel = 0.0; rate = 0.0; n = 0
while fuel < 95:
    rate = 0.99 * rate + 0.01 * (3 * 1.0 * 0.4)
    fuel = min(fuel + rate * 0.1, 100.0); n += 1
print("refuel ticks to 95", n)
# 13. wrecked recovery: damage 100.5, 100 passengers, fuel 100
d = 100.5; p = 100; f = 100.0; floor = 50.0; n = 0
while True:
    n += 1
    d -= 0.33 * 0.1
    d -= 0.1
    f = max(f - 0.5, floor)
    if p > 0: p -= 1
    if p == 0 and f <= floor: d -= 0.2
    if d <= 0: break
print("wrecked ticks", n)
