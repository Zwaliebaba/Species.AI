import sys
def down(x):
    n=0
    while x>0:
        x-=0.1; n+=1
    return n
def up(th):
    t=0.0;n=0
    while not t>th:
        t+=0.1;n+=1
    return n
for v in [0.1,0.2,0.3,0.5,0.6,1.0,2.0,2.5,3.0,4.0,5.0,6.0,10.0,20.0,60.0,90.0,120.0,180.0]:
    print('down',v,down(v))
for v in [0.1,0.3,1.0]:
    print('up >',v,up(v))
