# P04 helper: simulate Nuke flight (code/worldobject/nuke.cpp:Nuke::AdvanceToTarget) on flat ground.
# Re-implements Vector3::FastRotateAround (Rodrigues, right-handed) and the cross product of code/lib/vector3.h.
import math
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def norm(v):
    m=math.sqrt(dot(v,v)); return tuple(c/m for c in v)
def rot(v,n,ang):
    d=dot(v,n); a=tuple(c*d for c in n); n1=tuple(v[i]-a[i] for i in range(3)); n2=cross(n,n1)
    s,c=math.sin(ang),math.cos(ang); return tuple(a[i]+c*n1[i]+s*n2[i] for i in range(3))
def sim(D, spd=100.0, dt=0.1):
    sub=(0.0,30.0,0.0); target=(D,-5.0,0.0); pos=(0.0,30.0,0.0)
    total=math.dist(target,pos); t=0; apex=0
    while t<120:
        rem=math.dist(target,pos); f=1-rem/total
        hf=math.dist(sub,target)/3.0
        fn=max(5*pos[1]/hf,2.0)
        front=norm(tuple(target[i]-pos[i] for i in range(3)))
        dd=norm(tuple(target[i]-sub[i] for i in range(3)))
        r=norm(cross(dd,(0,1,0)))
        front=rot(front,r,math.pi/fn); front=rot(front,r,f*(-math.pi/fn))
        v=spd/2+spd/2*f*f
        pos=tuple(pos[i]+front[i]*v*dt for i in range(3)); t+=dt; apex=max(apex,pos[1])
        if math.dist(pos,target)<5.0: return dict(D=D,flight_s=round(t,1),apex=round(apex))
    return dict(D=D,flight_s=None,apex=round(apex))
for D in (300,600,1000,1500,2000):
    print(sim(D))
