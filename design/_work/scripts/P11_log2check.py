"""P11 helper: emulate Multiwinia Log2/GetPowerOfTwo with float32-truncated logs (iv_log) and
report where ceil(Log2(x)) differs from the exact ceil(log2(x)) for x in 1..70000.
Usage: python3 -I P11_log2check.py"""
import math, struct
def f32(x): return struct.unpack('<f', struct.pack('<f', x))[0]
one_over = 1.0 / f32(math.log(2.0))
def Log2(x): return f32(math.log(x)) * one_over
bad = []
for x in range(1, 70000):
    a = math.ceil(Log2(x))
    exact = math.ceil(math.log2(x)) if (x & (x-1)) else int(math.log2(x))
    if a != exact: bad.append((x, Log2(x), a, exact))
print('mismatches:', len(bad))
for b in bad[:40]: print(b)
for k in range(0, 17):
    x = 2**k
    print(k, x, repr(Log2(x)), math.ceil(Log2(x)))
