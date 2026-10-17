"""S_30: simulate Camera::AdvanceMoveToTargetMode distance integration.

Usage: python3 -I S_30_movetotarget_sim.py
Model (code/camera.cpp:Camera::AdvanceMoveToTargetMode): f = max(0.2, t/T);
v = 2*vmax*f for f < 0.5, 2*vmax*(1-f) for 0.5 <= f < 1, vmax = 2*dist/T;
the first advance happens one frame after the request; motion stops at f >= 1
without snapping. Prints travelled distance as a multiple of the intended one.
"""
for T in (0.5, 2.0, 5.0, 10.0, 30.0):
    for fps in (20.0, 30.0, 60.0, 144.0):
        dt = min(1.0 / fps, 0.25)
        dist, t, x = 1.0, 0.0, 0.0
        vmax = 2 * dist / T
        while True:
            t += dt
            f = max(0.2, t / T)
            if f < 0.5:
                x += vmax * 2 * f * dt
            elif f < 1.0:
                x += vmax * 2 * (1 - f) * dt
            else:
                break
        print(f'T={T:5.1f}s fps={fps:5.0f} travelled={x:.4f}*dist')
