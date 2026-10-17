# Derived throughput figures for reference/05 (integer-tick model, game speed 1).
pause_mean = sum(range(101, 151)) / 50  # 125.5
for h in range(0, 100, 10):
    n = 20 + 2 * (h // 10)
    burst = 2 * (n - 1)
    cyc = burst + pause_mean
    rate = n / (cyc / 10)
    lo = n / ((burst + 150) / 10); hi = n / ((burst + 101) / 10)
    print(h, n, burst / 10, round(cyc / 10, 2), round(rate, 3), round(rate * 60, 1), round(lo, 2), round(hi, 2))
r1 = 20 / (163.5 / 10); r9 = 38 / (199.5 / 10)
for k in range(1, 16):
    print(k, round(k * r1, 2), round(k * r1 * 60), round(k * r9, 2), round(k * r9 * 60))
for c in (40, 100, 75, 60):
    print('gate', c, round(c / 66, 2), round(c / 66 * 60, 1))
print('gate100 vs node', round((100 / 66) / r1, 2), 'gate40 vs node', round((40 / 66) / r1, 2))
print('fill A', (1000 - 45) / (3 * r1), 'fill B', (500 - 45) / (2 * r1))
waves = [0] + [649 + 660 * i for i in range(12)]
print('wave ticks', waves[:13])
print('activation foregone', 300 / 10 * r1)
