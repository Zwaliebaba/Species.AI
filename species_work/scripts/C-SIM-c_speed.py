import sys
def run(speed0, target, rate, hold, maxticks=2000):
    s=speed0; gt=0.0; req=target; rr=rate; hl=hold
    reached=None; back=None
    for n in range(1,maxticks+1):
        paused = s < 0.01
        if not paused: gt += 0.1*s
        if req >= 0:
            s += (req - s)*rr
            if abs(s-req) < 0.01:
                s=req; req=-1
                if reached is None: reached=n
        if (req < 0) and hl >= 0:
            hl -= 0.1
            if hl <= 0:
                hl=-1; req=1.0
        if reached is not None and req < 0 and hl < 0 and (target!=1.0 and abs(s-1.0)<1e-12):
            back=n; break
        if reached is not None and hold < 0:
            break
    return reached, back, gt
cases=[('grace',0.0,1.0,1.0,-1),('pause',1.0,0.0,0.2,-1),('unpause',0.0,1.0,0.2,-1),('won',1.0,0.0,0.2,-1),
('slow0.2',1.0,0.2,0.05,15.0),('slow0.3',1.0,0.3,0.05,15.0),('slow0.4',1.0,0.4,0.05,15.0),
('fast1.3',1.0,1.3,0.05,15.0),('fast1.65',1.0,1.65,0.05,15.0),('fast2.0',1.0,2.0,0.05,15.0),('meteor',1.0,0.4,0.1,4.0)]
for c in cases:
    print(c[0], run(*c[1:]))
