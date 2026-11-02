# Kinematic model, not a measurement. Geometry: spacing 33, line 23, lookahead 120, track 120.
import math,sys
SP,LW,LOOK,TR=33.,23.,120.,120.
DT=.002
def path(kind,R):
    # returns function: distance of a world point to the line centre (signed unneeded)
    if kind=='straight':
        return lambda x,y:abs(y)
    if kind=='curve':  # straight to x=0, then left arc radius R for 90 deg, then straight +y
        def d(x,y):
            if x<=0: return abs(y)
            if y>=R and True and x>=R-1e-9 and False: pass
            a=math.atan2(x,R-y) # angle from arc centre (0,R)
            if 0<=a<=math.pi/2: return abs(math.hypot(x,y-R)-R)
            return min(math.hypot(x-R,y-R)+0 if y<R else abs(x-R), math.hypot(x,y))
        return d
    if kind=='s':
        def d(x,y):
            ds=[]
            a=math.atan2(x,R-y)
            ds.append(abs(y) if x<=0 else 1e9)
            if 0<=a<=math.pi/2: ds.append(abs(math.hypot(x,y-R)-R))
            # then right arc centre (2R,R)
            a2=math.atan2(2*R-x,y-R)
            if 0<=a2<=math.pi/2: ds.append(abs(math.hypot(x-2*R,y-R)-R))
            if x>=2*R: ds.append(abs(y-2*R))
            return min(ds) if ds else 1e9
        return d
def run(p,kind,R,y0=3.,h0=0.,L=1500.,lag=.06):
    d=path(kind,R); x,y=-400.,y0; th=h0; vl=vr=0.;q=0.;req=0
    side=prev=0;bend=False;t=0.;s=0.;dark=touch=0.;nexts=nextm=0.;maxe=0;flips=0;last=0;lost=None;bends=0;peak=0
    while s<L:
        fx,fy=x+LOOK*math.cos(th),y+LOOK*math.sin(th)
        nx,ny=-math.sin(th),math.cos(th)
        lb=d(fx+nx*SP/2,fy+ny*SP/2)<=LW/2; rb=d(fx-nx*SP/2,fy-ny*SP/2)<=LW/2
        e=d(fx,fy); maxe=max(maxe,e)
        if t>=nexts:
            nexts+=.01
            if lb and rb: req=0   # both black: treat as no steer here
            else:
                sg=1 if lb else -1 if rb else 0   # + = turn left
                if sg:
                    if sg!=prev:
                        w=p['lean_mm'] if bend else p['touch_mm']
                        nb=sg==side and s-dark<=w
                        if nb and not bend: bends+=1
                        bend=nb; touch=s
                    elif s-touch>p['touch_mm']:
                        if not bend: bends+=1
                        bend=True
                    side=sg;dark=s;req=sg*(p['bend'] if bend else p['edge'])
                else:
                    lean=bend and s-dark<=p['lean_mm']
                    req=side*(p['lean'] if lean else p['drift'])
                    if side and s-dark>=200: lost=s;break
                prev=sg
        if t>=nextm:
            nextm+=.02
            fast=req==0 or (q!=0 and (req<0)!=(q<0)) or (p.get('fast_drift') and abs(req)<abs(q))
            step=(1800 if fast else 900)*.02
            q+=max(-step,min(step,req-q))
            if q*last<0: flips+=1
            if q: last=q
            peak=max(peak,abs(q))
        tv=100.*(1-q/127),100.*(1+q/127)  # (left,right) wheel: + q turns left in the model
        vl+=(tv[0]-vl)*DT/lag; vr+=(tv[1]-vr)*DT/lag
        v=(vl+vr)/2; x+=v*math.cos(th)*DT; y+=v*math.sin(th)*DT; th+=(vr-vl)/TR*DT
        s+=abs(v)*DT; t+=DT
    return dict(lost=lost is not None and round(lost),maxe=round(maxe,1),flips=flips,bends=bends,deg=round(math.degrees(th)))
base=dict(edge=115,bend=127,lean=57,drift=8,touch_mm=50,lean_mm=100)
cands={'current 115/127/57/8':base,
 'edge 70':dict(base,edge=70),'edge 60 bend 90 lean 30':dict(base,edge=60,bend=90,lean=30),
 'edge 50 bend 90 lean 25':dict(base,edge=50,bend=90,lean=25),'edge 40 bend 90 lean 20':dict(base,edge=40,bend=90,lean=20),
 'current + fast unwind':dict(base,fast_drift=True),
 'edge 60 bend 90 lean 30 + fast':dict(base,edge=60,bend=90,lean=30,fast_drift=True),
 'edge 50 bend 90 lean 25 + fast':dict(base,edge=50,bend=90,lean=25,fast_drift=True),
 'edge 40 bend 90 lean 20 + fast':dict(base,edge=40,bend=90,lean=20,fast_drift=True)}
for lag in (.03,.06,.1):
  print('--- wheel lag',lag)
  for n,p in cands.items():
    r=[]
    for k,R,y0 in (('straight',0,3),('curve',300,3),('curve',200,3),('curve',150,3),('s',200,3)):
        o=run(p,k,R,y0,lag=lag); r.append(f"{k[:2]}{R}:{'LOST' if o['lost'] else 'ok'} e{o['maxe']} f{o['flips']} b{o['bends']}")
    print(f"{n:34s}"," | ".join(r))
