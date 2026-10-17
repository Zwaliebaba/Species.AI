# P04 helper: simulate AirstrikeUnit flight (code/worldobject/airstrike.cpp) to check that
# the bomber flight leaves the world after the attack. Pure arithmetic re-implementation.
import math, sys
def norm(v):
    m=math.sqrt(sum(c*c for c in v)); return [c/m for c in v] if m>0 else v
def sim(X,Z,ax,az):
    dt=0.1; inset=100; h=500
    corners=[(inset,h,inset),(inset,h,Z-inset),(X-inset,h,Z-inset),(X-inset,h,inset)]
    att=(ax,70.0,az)
    d=[math.dist(c,att) for c in corners]
    exit_i=d.index(min(d)); enter_i=min((i for i in range(4) if i!=exit_i), key=lambda i:d[i])
    wp=list(corners[enter_i]); front=norm([X/2-wp[0],0-wp[1],Z/2-wp[2]]); speed=400.0
    state='A'; t=0; drop=None
    while t<200:
        tgt=att if state=='A' else corners[exit_i]
        tf=norm([tgt[i]-wp[i] for i in range(3)])
        front=norm([front[i]*(1-dt)+tf[i]*dt for i in range(3)])
        dist=math.dist(att,wp); des=40.0
        if (state=='A' and dist>600) or (state=='L' and dist>100): des*=10
        speed=speed*(1-dt)+des*dt
        wp=[wp[i]+front[i]*speed*dt for i in range(3)]
        if drop is None and math.dist(wp,att)<90: drop=(round(t,1),round(speed,1))
        if state=='A' and math.dist(tgt,wp)<10: state='L'
        t+=dt
        if wp[0]<0 or wp[2]<0 or wp[0]>=X or wp[2]>=Z: return dict(enter=enter_i,exit=exit_i,drop=drop,left_world_at=round(t,1))
    return dict(enter=enter_i,exit=exit_i,drop=drop,left_world_at=None)
for X,Z,ax,az in [(2000,2000,1000,1000),(2000,2000,400,1500),(3000,2000,2500,300),(1500,1500,700,800)]:
    print((X,Z,ax,az), sim(X,Z,ax,az))
