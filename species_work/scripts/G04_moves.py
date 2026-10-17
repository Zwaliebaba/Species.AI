def count_double(start, step=0.1):
    x = start; n = 0
    while True:
        x -= step; n += 1
        if x <= 0: return n
for v in [0.2,0.4,0.6,0.8,1.0,1.2,0.6*2,0.625,1.25,0.75,2.5,0.25,1.0,1.5,2.0,0.3]:
    print(v, 'countdown ticks', count_double(v), 'moves-before-removal', count_double(v)-1)
