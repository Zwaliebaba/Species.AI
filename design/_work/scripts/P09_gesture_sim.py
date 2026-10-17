"""P09 helper: faithful re-implementation (including source defects) of the
Gesture recogniser (code/gesture.cpp) used to produce test vectors.
Usage: python3 -I P09_gesture_sim.py <gestures.txt> <gesture_demos_dir> [screenH]
Pure-python (no numpy) so it can run with -I."""
import sys, math, os

NF = 13

def load_training(path):
    toks = open(path, encoding='latin-1').read().split()
    pos = 0
    n = int(toks[pos]); pos += 1
    classes = []
    for _ in range(n):
        nt = int(toks[pos]); pos += 1
        tr = []
        for _ in range(nt):
            tr.append([float(t) for t in toks[pos:pos+NF+1]]); pos += NF+1
        classes.append(tr)
    return classes

def invert(a):
    # Gaussian elimination with scaled partial pivoting (code/lib/invert_matrix.cpp:inv)
    n = len(a)
    A = [row[:] for row in a]
    b = [[1.0 if i == j else 0.0 for j in range(n)] for i in range(n)]
    index = list(range(n))
    scale = [max(abs(x) for x in A[i]) for i in range(n)]
    for k in range(n-1):
        ratiomax = 0.0; jp = k
        for i in range(k, n):
            r = abs(A[index[i]][k]) / scale[index[i]] if scale[index[i]] else 0.0
            if r > ratiomax: jp = i; ratiomax = r
        indexJ = index[k]
        if jp != k:
            indexJ = index[jp]; index[jp] = index[k]; index[k] = indexJ
        for i in range(k+1, n):
            coeff = A[index[i]][k] / A[indexJ][k]
            for j in range(k+1, n):
                A[index[i]][j] -= coeff * A[indexJ][j]
            A[index[i]][k] = coeff
            for j in range(n):
                b[index[i]][j] -= A[index[i]][k] * b[indexJ][j]
    inv = [[0.0]*n for _ in range(n)]
    for k in range(n):
        inv[n-1][k] = b[index[n-1]][k] / A[index[n-1]][n-1]
        for i in range(n-2, -1, -1):
            s = b[index[i]][k]
            for j in range(i+1, n):
                s -= A[index[i]][j] * inv[j][k]
            inv[i][k] = s / A[index[i]][i]
    return inv

def train(classes):
    C = len(classes)
    means = [[sum(t[i] for t in tr)/len(tr) for i in range(NF+1)] for tr in classes]
    cov = []
    for c, tr in enumerate(classes):
        m = means[c]
        cov.append([[sum((t[i]-m[i])*(t[j]-m[j]) for t in tr) for j in range(NF+1)] for i in range(NF+1)])
    trainerSum = -C + sum(len(tr) for tr in classes)
    common = [[sum(cov[c][i][j] / float(len(classes[c])) - 1.0 for c in range(C)) / trainerSum
               for j in range(NF+1)] for i in range(NF+1)]   # reproduces the precedence defect
    inv = invert(common)
    weights = []
    for c in range(C):
        w = [0.0]*(NF+1)
        for j in range(1, NF+1):
            w[j] = sum(inv[i][j]*means[c][i] for i in range(1, NF+1))
        w[0] = -0.5 * sum(w[i]*means[c][i] for i in range(1, NF+1))
        weights.append(w)
    return means, inv, weights

def features(samples):
    n = len(samples)
    f = [0.0]*(NF+1)
    if n < 3: return None
    x = [s[0] for s in samples]; y = [s[1] for s in samples]; t = [s[2] for s in samples]
    sdx = x[2]-x[0]; sdy = y[2]-y[0]; d = math.sqrt(sdx*sdx+sdy*sdy)
    f[1] = sdx/d; f[2] = sdy/d
    L, R, T, B = 999999.0, 0.0, 999999.0, 0.0
    for i in range(n):
        if x[i] < L: L = x[i]
        if x[i] > R: R = y[i]          # defect reproduced: uses y
        if y[i] < T: T = y[i]
        if y[i] > B: B = y[i]
    f[3] = math.sqrt((R-L)**2 + (B-T)**2)
    f[4] = math.atan((B-T)/(R-L)) if (R-L) != 0 else math.copysign(math.pi/2, (B-T))
    fdx = x[-1]-x[0]; fdy = y[-1]-y[0]
    f[5] = math.sqrt(fdx*fdx+fdy*fdy)
    f[6] = fdx/f[5] if f[5] else float('nan'); f[7] = fdy/f[5] if f[5] else float('nan')
    f[8] = sum(math.hypot(x[j+1]-x[j], y[j+1]-y[j]) for j in range(n-2))   # last segment omitted (defect)
    tot = sa = ss = 0.0
    for k in range(1, n-1):
        adx = x[k+1]-x[k]; ady = y[k+1]-y[k]; bdx = x[k]-x[k-1]; bdy = y[k]-y[k-1]
        num = adx*bdy - bdx*ady; den = adx*bdx + ady*bdy
        ang = math.atan(num/den) if den != 0 else math.copysign(math.pi/2, num)
        tot += ang; sa += abs(ang); ss += ang*ang
    f[9], f[10], f[11] = tot, sa, ss
    mx = dur = 0.0
    for l in range(n-1):
        dx = x[l+1]-x[l]; dy = y[l+1]-y[l]; dt = t[l+1]-t[l]
        if dt == 0: continue
        sp = (dx*dx+dy*dy)/(dt*dt)
        if sp > mx: mx = sp
        dur += dt
    f[12], f[13] = mx, dur
    return f

def classify(f, means, inv, weights):
    C = len(weights)
    r = [weights[c][0] + sum(weights[c][i]*f[i] for i in range(1, NF+1)) for c in range(C)]
    conf = []
    for i in range(C):
        tot = 0.0
        for j in range(C):
            e = (r[j]-r[i])/500.0
            tot += math.exp(min(e, 700))
        conf.append(1.0/tot)
    maha = []
    for i in range(C):
        tot = 0.0
        for j in range(1, NF):        # features 1..12 only (defect reproduced)
            for k in range(1, NF):
                tot += inv[j][k]*(f[j]-means[i][j])*(f[k]-means[i][k])
        maha.append(tot)
    best = max(range(C), key=lambda c: (r[c], -c))
    # first maximum wins (GetSymbolID uses strict >)
    best = 0
    for c in range(1, C):
        if r[c] > r[best]: best = c
    return best, conf[best], maha[best], r

def demo_samples(path, screenH):
    pts = []
    for line in open(path, encoding='latin-1'):
        p = line.split()
        if len(p) >= 2: pts.append((float(p[0])*screenH, float(p[1])*screenH))
    out = []
    for i, (px, py) in enumerate(pts):
        xi, yi = int(px), int(py)
        if out:
            if math.hypot(xi-out[-1][0], yi-out[-1][1]) < 6.0: continue
        out.append((xi, yi, i*40.0))
    return out

def main():
    gpath, ddir = sys.argv[1], sys.argv[2]
    screenH = float(sys.argv[3]) if len(sys.argv) > 3 else 768.0
    names = ["Squad","Engineer","Grenade","Rocket","AirStrike","Controller","Officer","Armour"]
    classes = load_training(gpath)
    means, inv, weights = train(classes)
    # self-test on training data
    ok = 0; tot = 0
    for c, tr in enumerate(classes):
        for t in tr:
            b, cf, mh, _ = classify(t, means, inv, weights)
            tot += 1; ok += (b == c and cf >= 0.5 and mh <= 3000.0)
    print(f"training-set acceptance: {ok}/{tot}")
    for fn in sorted(os.listdir(ddir)):
        s = demo_samples(os.path.join(ddir, fn), screenH)
        f = features(s)
        b, cf, mh, r = classify(f, means, inv, weights)
        print(f"{fn:16s} samples={len(s):3d} -> {names[b]:10s} conf={cf:.3f} maha={mh:10.1f} success={cf>=0.5 and mh<=3000.0}")

if __name__ == '__main__':
    main()
