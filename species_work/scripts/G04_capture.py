def ticks(n, period=0.1):
    t = 0.0; k = 0
    while True:
        d = min(0.05, period * n / 100.0)
        t = min(2.5, t + d); k += 1
        if t >= 2.5: return k
res = {n: ticks(n) for n in range(1, 61)}
for n in range(1, 61):
    exact = -(-2500 // min(50, n))  # ceil(2500/min(50,n))
    print(n, res[n], exact, res[n]-exact)
