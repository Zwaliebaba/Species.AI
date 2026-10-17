import sys
import numpy as np
def count_double(start, step=0.1, le=True):
    x = start; n = 0
    while True:
        x -= step; n += 1
        if (x <= 0 if le else x < 0): return n
def count_float(start, step=0.1):
    x = np.float32(start); n = 0
    while True:
        x = np.float32(np.float64(x) - step); n += 1
        if x <= 0: return n
def count_float_f(start):
    x = np.float32(start); s = np.float32(0.1); n=0
    while True:
        x = np.float32(x - s); n+=1
        if x <= 0: return n
for v in [0.5,0.6,0.75,1.0,1.2,1.5,2.0,2.5,3.0,4.0,5.0,8.0,10.0,15.0,30.0,45.0,60.0]:
    print(v, 'double', count_double(v), 'float(dbl step)', count_float(v), 'float(f step)', count_float_f(v))
