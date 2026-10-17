#!/usr/bin/env python3
"""S_20_gametimer_sim.py — re-implementation of code/gametimer.cpp:GameTimer::Tick (IEEE double,
same statement order) used to tabulate the real duration of every speed request in 20 §10.7.

Usage: python3 -I S_20_gametimer_sim.py
Assumptions: one Tick per 0.1 s of real time (one processed server UPDATE), GamePaused() reduced to
IsPaused() (speed < 0.01) — out-of-sync and lost-focus pauses are ignored.
"""


class GameTimer:
    def __init__(self, speed):
        self.speed = speed
        self.game_time = 0.0
        self.req = -1.0
        self.rate = -1.0
        self.length = -1.0
        self.locked = False

    def request(self, s, r, l):
        if self.locked:
            return
        self.req, self.rate, self.length = s, r, l

    def paused(self):
        return self.speed < 0.01

    def tick(self):
        if not self.paused():
            self.game_time += 0.1 * self.speed
        if self.req >= 0.0:
            diff = self.req - self.speed
            self.speed += diff * self.rate
            if abs(self.speed - self.req) < 0.01:
                self.speed = self.req
                self.req = -1.0
        if self.req < 0.0 or self.speed == self.req:
            if self.length >= 0.0:
                self.length -= 0.1
                if self.length <= 0.0:
                    self.length = -1.0
                    self.req = 1.0


def run(name, start_speed, s, r, l, max_ticks=2000):
    t = GameTimer(start_speed)
    t.request(s, r, l)
    reached = None
    back = None
    for n in range(1, max_ticks + 1):
        t.tick()
        if reached is None and t.req < 0.0 and t.speed == s:
            reached = n
        if l >= 0 and reached is not None and t.req < 0.0 and t.speed == 1.0 and n > reached:
            back = n
            break
        if l < 0 and reached is not None:
            break
    total = back if back is not None else reached
    print('| %s | %.2f -> %.2f | %.2f | %s | %d ticks (%.1f s) | %s | %.3f game s over %.1f real s |' % (
        name, start_speed, s, r, ('%.1f s' % l) if l >= 0 else 'none', reached, reached / 10.0,
        ('%d ticks (%.1f s)' % (back, back / 10.0)) if back else '—', t.game_time, total / 10.0))


def main():
    print('| Request | Speed change | Rate | Hold | Reaches target after | Back at 1.0 after | Game time elapsed |')
    print('|---|---|---|---|---|---|---|')
    run('grace period end', 0.0, 1.0, 1.0, -1.0)
    run('user pause (NET_DARWINIA_PAUSE)', 1.0, 0.0, 0.2, -1.0)
    run('user unpause', 0.0, 1.0, 0.2, -1.0)
    run('game won (then locked)', 1.0, 0.0, 0.2, -1.0)
    for v in (0.2, 0.3, 0.4):
        run('slow-down crate, target %.1f' % v, 1.0, v, 0.05, 15.0)
    for v in (1.3, 1.65, 2.0):
        run('speed-up crate, target %.2f' % v, 1.0, v, 0.05, 15.0)
    run('meteor within 200 of target', 1.0, 0.4, 0.1, 4.0)


if __name__ == '__main__':
    main()
