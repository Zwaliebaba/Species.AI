"""S04: discrete re-implementation of a thrown grenade on flat ground (game speed 1.0).

Usage: python3 -I S_04_grenade_sim.py
Mirrors code/location.cpp:Location::ThrowWeapon (force = 8*sqrt(d), cap 110, front.y = 1 then normalise),
code/worldobject/weapons.cpp:ThrowableWeapon::Advance (vel.y -= 9.8 per tick, pos += vel*0.1, bounce when
y < ground+1: WorldObject::BounceOffLandscape reflects about the normal and x0.85, then force *= 0.75, vel *= force)
and Grenade::Advance (3.0 s fuse, life decremented before the move). Flat ground at y = 0, normal (0,1,0).
Thrower stands on the ground (Darwinian m_pos is at ground level). Prints where the grenade is when it explodes.
"""
import math

DT = 0.1


def throw(d, fuse=3.0, cap=110.0):
    force = min(math.sqrt(d) * 8.0, cap)
    fx, fy = 1.0, 1.0                     # horizontal unit + y=1, normalised -> 45 degrees on flat ground
    n = math.hypot(fx, fy)
    vx, vy = fx / n * force, fy / n * force
    x, y = 0.0, 0.0
    f = 1.0
    life = fuse
    first_impact = None
    t = 0
    while True:
        t += 1
        life -= DT
        if life <= 0.0:
            return force, first_impact, x, t
        if f > 0.1:
            vy -= 9.8
            x += vx * DT
            y += vy * DT
            if y < 1.0:
                if first_impact is None:
                    first_impact = x
                y = 0.0
                vy = -vy                  # reflect about (0,1,0)
                vx *= 0.85
                vy *= 0.85
                f *= 0.75
                vx *= f
                vy *= f
                if y < 1.0:
                    y = 1.0


if __name__ == "__main__":
    print("target_d  force  first_impact  explode_x  explode_tick")
    for d in (25, 50, 75, 100, 125, 150, 189, 200, 250):
        force, fi, x, t = throw(d)
        print(f"{d:8.0f} {force:6.1f} {fi if fi is None else round(fi, 1)!s:>12} {x:10.1f} {t:12d}")
