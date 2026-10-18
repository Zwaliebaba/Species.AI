"""P6-RND: check the T2 tolerance bound of the particle integration (read-only helper; no arguments)."""
T = 0.1
h = 1.0 / 60.0
types = {'RocketTrail': (2.0, 15.0, 0.6), 'ExplosionCore': (2.0, 10.0, 0.2), 'ExplosionDebris': (6.0, 20.0, 0.2),
         'MuzzleFlare': (1.0, 0.0, 0.2), 'Fire': (4.0, -4.0, 0.0), 'ControlFlash': (1.0, -2.0, 0.0),
         'Spark': (4.0, 15.0, 1.5), 'Casing': (2.0, 30.0, 0.5), 'MissileTrail': (5.0, 1.0, 0.0),
         'WorkerFire': (1.0, -10.0, 0.0), 'Leaf': (60.0, 5.0, 1.5), 'Sickness': (2.0, -0.5, 0.0),
         'Volcano': (12.0, 10.0, 0.0)}
worst = 0.0
for name, (life, g, f) in types.items():
  for v0 in (-200.0, -50.0, 0.0, 50.0, 200.0):
    for scale in (0.5, 1.0, 2.0):  # size / type size
      # reference: step T
      pr, vr = 0.0, v0
      ps, vs = 0.0, v0
      ticks = int(round(min(life, 6.0) / T))
      for k in range(ticks):
        pr += vr * T
        vr *= (1 - T * f)
        vr -= T * g * scale
        for _ in range(6):
          ps += vs * h
          vs *= (1 - T * f) ** (h / T)
          vs -= h * g * scale
        bound = (T + h) * abs(v0 - vr) + 0.01
        err = abs(pr - ps)
        worst = max(worst, err / bound)
        if err > bound:
          print('exceeds', name, v0, scale, k, err, bound)
print('largest error / bound', worst)
