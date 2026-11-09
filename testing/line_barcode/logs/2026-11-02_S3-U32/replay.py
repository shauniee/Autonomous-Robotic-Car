import re,sys
A=[0x109,0x049,0x148,0x019,0x118,0x058,0x00d,0x10c,0x04c,0x01c,0x103,0x043,0x142,0x013,0x112,0x052,0x007,0x106,0x046,0x016,0x181,0x0c1,0x1c0,0x091,0x190,0x0d0]
def rev(p):
    r=0
    for i in range(9): r=(r<<1)|(p&1);p>>=1
    return r
def grp(w,ratio=1.5):
    s=sorted(w)
    if s[6]<s[5]*ratio: return None
    p=0
    for x in w: p=(p<<1)|(1 if x>=s[6] else 0)
    return p
def dec(w):
    g=[grp(w[i*10:i*10+9]) for i in range(3)]
    if None in g: return None
    if g[0]==0x94 and g[2]==0x94: pay=g[1];o='F'
    elif g[0]==rev(0x94) and g[2]==rev(0x94): pay=rev(g[1]);o='R'
    else: return None
    return (chr(65+A.index(pay)) if pay in A else '?',o)
L=open(sys.argv[1],errors='replace').readlines()
rs=[]
for l in L:
    if l.startswith('BARS'):
        rs+=[(t[0],int(t[1:])) for t in l.split()[2:]]
print('raw',len(rs),'same-polarity pairs',sum(a[0]==b[0] for a,b in zip(rs,rs[1:])))
def merge(rs,mn):
    out=[]
    for a,b in rs:
        if out and out[-1][0]==a: out[-1][1]+=b
        else: out.append([a,b])
    # tiny run joins both neighbours
    ch=True
    while ch:
        ch=False
        for i in range(1,len(out)-1):
            if out[i][1]<mn and out[i-1][0]==out[i+1][0]:
                out[i-1][1]+=out[i][1]+out[i+1][1];del out[i:i+2];ch=True;break
    return out
for mn in (1,4,8,12,20,30):
    m=merge(rs,mn)
    hits=[]
    w=[]
    for a,b in m:
        if b>400: w=[];continue
        w.append(b)
        if len(w)>29: w.pop(0)
        if len(w)==29:
            d=dec(w)
            if d: hits.append(d);w=[]
    print('min',mn,'runs',len(m),'hits',hits)
m=merge(rs,12)
print(' '.join(f"{a}{b}" for a,b in m)[:2500])
