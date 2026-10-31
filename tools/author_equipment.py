"""Author centimetre-scale equipment meshes with UVs, normals and material slots.
No basic-shape stand-ins are used by the new skate and scooter assemblies.
"""
from pathlib import Path
import math,json
import numpy as np
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/source/workshop';OUT.mkdir(parents=True,exist_ok=True)
class Mesh:
 def __init__(self,name):self.name=name;self.v=[];self.n=[];self.uv=[];self.f=[];self.mat='Leather'
 def vertex(self,p,n,uv=(0,0)):
  self.v.append(p);self.n.append(n);self.uv.append(uv);return len(self.v)
 def tri(self,a,b,c):self.f.append((self.mat,a,b,c))
 def quad(self,a,b,c,d):self.tri(a,b,c);self.tri(a,c,d)
 def save(self):
  out=['# BMX Cruisers authored equipment, cm, Z up','mtllib workshop.mtl']
  for p in self.v:out.append('v '+' '.join(f'{x:.6f}' for x in p))
  for p in self.uv:out.append('vt '+' '.join(f'{x:.6f}' for x in p))
  for p in self.n:out.append('vn '+' '.join(f'{x:.6f}' for x in p))
  prev=None
  for mat,*ids in self.f:
   if mat!=prev:out.append('usemtl '+mat);prev=mat
   out.append('f '+' '.join(f'{i}/{i}/{i}' for i in ids))
  (OUT/(self.name+'.obj')).write_text('\n'.join(out)+'\n')
 def tube(self,points,radius,mat,sides=12):
  self.mat=mat;p=np.asarray(points,float);base=len(self.v)+1
  for i,pos in enumerate(p):
   tangent=p[min(i+1,len(p)-1)]-p[max(i-1,0)];tangent/=max(np.linalg.norm(tangent),1e-8)
   up=np.array([0,0,1.]) if abs(tangent[2])<.95 else np.array([0,1.,0])
   u=np.cross(tangent,up);u/=np.linalg.norm(u);v=np.cross(tangent,u)
   for j in range(sides+1):
    t=j*2*math.pi/sides;n=np.cos(t)*u+np.sin(t)*v;self.vertex(pos+n*radius,n,(j/sides,i/max(len(p)-1,1)))
    if i<len(p)-1 and j<sides:
     a=base+i*(sides+1)+j;self.quad(a,a+sides+1,a+sides+2,a+1)
  for i in [0,len(p)-1]:
   a=self.vertex(p[i],(-1 if i==0 else 1)*tangent)
   for j in range(sides):self.tri(a,base+i*(sides+1)+j,base+i*(sides+1)+j+1)
 def loft(self,rings,mat,sides=48):
  # Each X cross-section has lateral half width, centre height and half height.
  self.mat=mat;base=len(self.v)+1
  for i,(x,w,z,h) in enumerate(rings):
   for j in range(sides+1):
    t=j*2*math.pi/sides;ny,nz=math.cos(t),math.sin(t)
    self.vertex((x,w*ny,z+h*nz),(0,ny,nz),(j/sides,i/max(1,len(rings)-1)))
    if i<len(rings)-1 and j<sides:
     a=base+i*(sides+1)+j;self.quad(a,a+1,a+sides+2,a+sides+1)
  for end in [0,len(rings)-1]:
   x,w,z,h=rings[end];a=self.vertex((x,0,z),(-1 if end==0 else 1,0,0))
   for j in range(sides):
    b=base+end*(sides+1)+j;self.tri(a,b+1,b) if end==0 else self.tri(a,b,b+1)
 def box(self,center,size,bevel,mat):
  self.mat=mat;c=np.array(center);half=np.array(size)/2;b=min(bevel,min(half)*.95);steps=5
  for axis in range(3):
   for sign in [-1,1]:
    ids=[];other=[x for x in range(3) if x!=axis]
    for i in range(steps+1):
     for j in range(steps+1):
      p=np.zeros(3);p[axis]=sign*half[axis];p[other[0]]=half[other[0]]*(2*i/steps-1);p[other[1]]=half[other[1]]*(2*j/steps-1)
      core=np.clip(p,-half+b,half-b);n=p-core;n/=max(np.linalg.norm(n),1e-9);ids.append(self.vertex(c+core+n*b,n,(i/steps,j/steps)))
    for i in range(steps):
     for j in range(steps):
      a=i*(steps+1)+j;q=[ids[a],ids[a+1],ids[a+steps+2],ids[a+steps+1]]
      n=np.cross(np.array(self.v[q[1]-1])-self.v[q[0]-1],np.array(self.v[q[2]-1])-self.v[q[0]-1])
      if n[axis]*sign<0:q.reverse()
      self.quad(*q)
 def lathe(self,center,profile,mat,sides=64):
  self.mat=mat;cx,cy,cz=center;base=len(self.v)+1
  for i,(y,r) in enumerate(profile):
   prev=profile[max(0,i-1)];nxt=profile[min(len(profile)-1,i+1)];dy=nxt[0]-prev[0];dr=nxt[1]-prev[1]
   norm=math.hypot(dy,dr) or 1
   for j in range(sides+1):
    t=j*2*math.pi/sides;self.vertex((cx+r*math.cos(t),cy+y,cz+r*math.sin(t)),(dy/norm*math.cos(t),-dr/norm,dy/norm*math.sin(t)),(j/sides,i/max(1,len(profile)-1)))
    if i<len(profile)-1 and j<sides:
     a=base+i*(sides+1)+j;self.quad(a,a+sides+1,a+sides+2,a+1)
 def ring(self,center,r,thick,mat):
  profile=[(thick*math.sin(t),r+thick*math.cos(t)) for t in np.linspace(0,2*math.pi,13)];self.lathe(center,profile,mat)
 def bolt(self,p,r=.38):self.lathe(p,[(-.2,r),(.2,r),(.3,r*.7)],'Alloy',sides=12)

inventory=[]
def publish(m):m.save();inventory.append(dict(name=m.name,vertices=len(m.v),triangles=len(m.f),materials=sorted(set(x[0] for x in m.f))))
# Anatomical skate uppers: moulded shell, separate rubber outsole, stitched liner,
# lace eyelets, toe vents, heel reinforcement and working-looking cuff buckles.
for quad in [False,True]:
 m=Mesh('QuadBootV2' if quad else 'InlineBootV2')
 sole=12 if not quad else 11
 m.loft([(-14,1,sole+1,1),(-12,4.8,sole+1,1.3),(-5,5.7,sole+1,1.3),(8,6.3,sole+1,1.3),(18,5,sole+1,1.3),(21,.5,sole+1,.5)],'Rubber')
 m.loft([(-13,1,sole+5,1),(-11,4.5,sole+6,6),(-6,5.2,sole+7,7),(3,5.8,sole+6,6),(12,5.6,sole+4.5,4.5),(18,4.2,sole+3,3),(20,.5,sole+2,.5)],'Leather' if quad else 'Shell')
 # Raised high-top collar, with an open visible padded interior.
 for z,w in [(sole+12,5.2),(sole+17,5.5),(sole+20,5.3)]:
  points=[(-6+4.8*math.cos(t),w*math.sin(t),z) for t in np.linspace(0,2*math.pi,49)];m.tube(points,.85,'Leather' if quad else 'Paint')
 m.loft([(-11,.4,sole+8,1),(-10,4.5,sole+12,8),(-7,4.8,sole+12.5,8.5),(-3,4.2,sole+9,5)],'Liner')
 m.box((2,0,sole+10),(17,4.9,1.5),.6,'Liner')
 for n,x in enumerate(np.linspace(-3,13,8)):
  z=sole+12-max(0,x)*.38
  for sign in [-1,1]:
   m.bolt((x,sign*3.4,z),.3)
   m.tube([(x,sign*3.2,z+.3),(x+1.6,0,z+1),(x+1.7,-sign*3.2,z-.1)],.17,'Lace',sides=8)
 # Double perimeter stitch lines are geometry, not flat painted bands.
 for sign in [-1,1]:
  for x in np.arange(-9,17,.9):m.tube([(x,sign*(5.2+.5*math.sin(x/8)),sole+2.2),(x+.42,sign*(5.2+.5*math.sin(x/8)),sole+2.2)],.055,'Stitch',sides=5)
 if not quad:
  m.box((-5,0,sole+15),(4,13,1.5),.55,'Paint');m.box((-5,6.3,sole+15),(4.5,2.2,2.5),.45,'Alloy')
  for side in [-1,1]:
   for x in [1,4,7]:m.box((x,side*5.6,sole+6),(1.2,.35,4),.12,'Rubber')
   m.bolt((-7,side*5.8,sole+13),.85)
  for side in [-1,1]:m.box((2,side*1.5,8.7),(31,1.3,5.2),.5,'Alloy')
 else:
  m.box((-6,0,sole+17),(4,12,1.2),.4,'Leather');m.box((-6,6,sole+17),(4,1.5,2),.3,'Alloy')
  m.box((1,0,8.6),(29,5.3,2),.5,'Alloy')
  for x in [-9,11]:
   m.tube([(x-2,0,9),(x,0,5.3),(x,-7.5,4)],.9,'Alloy');m.tube([(x,0,5.3),(x,7.5,4)],.9,'Alloy')
   m.bolt((x,0,9),.65)
  m.tube([(17,0,10),(22,0,4.8)],1.1,'Alloy');m.lathe((23,0,3.8),[(-2.3,2.3),(2.3,2.3)],'Rubber')
 publish(m)
# Polyurethane skate wheels, correctly solid webbed hubs rather than bicycle spokes.
for name,radius,width in [('InlineWheelV2',4.2,2.3),('QuadWheelV2',3.1,3.4),('ScooterWheelV2',5.5,2.4)]:
 m=Mesh(name);r=radius;w=width/2
 m.lathe((0,0,0),[(-w*.97,r*.73),(-w,r*.88),(-w*.7,r*.98),(0,r),(w*.7,r*.98),(w,r*.88),(w*.97,r*.73)],'Urethane')
 m.lathe((0,0,0),[(-w*.9,r*.61),(-w*.82,r*.78),(w*.82,r*.78),(w*.9,r*.61)],'Paint')
 m.lathe((0,0,0),[(-w*.9,.38),(-w*.8,r*.25),(w*.8,r*.25),(w*.9,.38)],'Alloy')
 for n in range(5):
  a=n*2*math.pi/5
  m.tube([(math.cos(a)*r*.21,0,math.sin(a)*r*.21),(math.cos(a+.1)*r*.45,0,math.sin(a+.1)*r*.45),(math.cos(a+.25)*r*.72,0,math.sin(a+.25)*r*.72)],r*.105,'Alloy' if 'Scooter' in name else 'Paint')
 for side in [-1,1]:m.ring((0,side*w*.92,0),r*.27,.12,'Alloy');m.bolt((0,side*(w+.1),0),.45)
 publish(m)
# Stunt scooter: 110 mm wheels, extruded boxed/concave deck, separate grip tape,
# shaped head tube, integrated headset, two-blade fork and sprung rear brake.
m=Mesh('ScooterFrameV2')
m.box((-9,0,8.5),(53,12.2,3.6),1.2,'Paint');m.box((-10,0,10.42),(49,10.6,.32),.15,'Grip')
for side in [-1,1]:m.tube([(-34,side*5,9.9),(15,side*5,9.9)],.32,'Alloy')
m.tube([(15,0,9),(25,0,12),(34,0,22),(38,0,27)],2.3,'Paint',sides=32)
m.tube([(38,0,24),(36,0,37)],3.2,'Paint',sides=32)
for z in [24,37]:m.ring((37,0,z),3.1,.4,'Alloy')
m.tube([(36,0,32),(30,0,102)],1.65,'Paint',sides=32)
m.box((30,0,103),(5,58,4.4),1.8,'Paint')
for sign in [-1,1]:
 m.tube([(30,sign*17,103),(30,sign*29,103)],2.15,'Rubber',sides=28)
 for n in range(12):m.ring((30,sign*(17+n),103),2,.08,'Rubber')
 m.tube([(38,sign*2.4,24),(43,sign*2.4,7)],1.35,'Alloy',sides=20)
 m.bolt((43,sign*2.8,7),.65)
 # rear wheel bracket
 m.box((-34,sign*2.1,7.5),(9,1.2,7),.5,'Alloy');m.bolt((-35,sign*2.7,7),.65)
# Rear brake curves over the rear polyurethane wheel.
for side in [-1,1]:m.tube([(-27,side*1.7,12),(-30,side*1.7,15),(-35,side*1.7,15.5),(-40,side*1.7,13)],.8,'Alloy')
m.box((-36,0,15.4),(8,4.5,.7),.3,'Alloy')
# Four-bolt double clamp, weld beads and machining lines.
for z in [35,39]:
 m.box((35,0,z),(6.5,7.2,2.8),.7,'Alloy')
 for side in [-1,1]:m.bolt((35,side*4,z),.5)
for n in range(18):
 t=n*2*math.pi/18;m.tube([(33+2.7*math.cos(t),2.7*math.sin(t),24),(33+2.7*math.cos(t+.18),2.7*math.sin(t+.18),24.1)],.14,'Alloy',sides=6)
publish(m)
# Vented bicycle helmets. The extreme version has a connected chin guard and
# curved peak, leaving the face opening clear instead of a floating mouth piece.
for extreme in [False,True]:
 m=Mesh('HelmetExtremeV3' if extreme else 'HelmetCasualV3')
 for inner in [True,False]:
  m.mat='Liner' if inner else 'Paint';base=len(m.v)+1;scale=.965 if inner else 1
  for r in range(25):
   ph=r*math.pi*.54/24
   for a in range(97):
    t=a*2*math.pi/96;normal=np.array([math.sin(ph)*math.cos(t),math.sin(ph)*math.sin(t),math.cos(ph)])
    m.vertex(normal*np.array([12.7,11,11.5])*scale,normal,(a/96,r/24))
    if r<24 and a<96:
     vent=not inner and 6<=r<=16 and any(abs((a%48)-k)<2 for k in [8,16,24,32,40])
     if not vent:
      q=base+r*97+a
      if inner:m.quad(q,q+1,q+98,q+97)
      else:m.quad(q,q+97,q+98,q+1)
 # Finely molded lower rim and retention strap.
 points=[(12.65*math.cos(t),10.95*math.sin(t),-1.5) for t in np.linspace(0,2*math.pi,97)];m.tube(points,.38,'Rubber')
 for side in [-1,1]:
  m.tube([(-4,side*9,-1),(1,side*7,-13),(7,side*3,-19)],.34,'Liner',sides=8)
  m.bolt((2,side*10,-1),.55)
 if extreme:
  # U-shaped guard joined to padded side cheeks; the opening remains above it.
  for side in [-1,1]:m.tube([(-1,side*9,-2),(0,side*10,-9),(7,side*11,-16),(15,side*8,-19)],1.6,'Paint',sides=20)
  m.tube([(15,-8,-19),(18,-4,-20),(19,0,-20),(18,4,-20),(15,8,-19)],1.9,'Paint',sides=24)
  # Curved peak with rolled edges and two mounting bolts.
  m.loft([(6,.2,6,.3),(10,10.5,6.5,.5),(16,12.5,5.5,.5),(22,11.5,3.5,.35),(23,.4,3.2,.2)],'Paint',sides=32)
  for side in [-1,1]:m.bolt((9,side*10,4),.7)
 publish(m)
# Full-face downhill helmet: a continuous composite shell, recessed EPS liner,
# ear/cheek coverage, a broad chin bar, intake grille and a separate curved peak.
# +X is forward; origin sits nine centimetres above the head bone.
m=Mesh('HelmetExtremeV4')
def shell_point(t,u,inset=0):
 # The opening rises to the brow at the front, and drops below the ears at the back.
 front=max(0,math.cos(t))
 end=2.48-1.02*front**3
 p=u*end
 return np.array([(12.4-inset)*math.sin(p)*math.cos(t),
                  (11.25-inset)*math.sin(p)*math.sin(t),
                  (11.8-inset)*math.cos(p)-max(0,p-1.55)*6])
for inner in [False,True]:
 rows,cols=40,128;base=len(m.v)+1;m.mat='Liner' if inner else 'Paint'
 for r in range(rows+1):
  for a in range(cols+1):
   t=a*2*math.pi/cols;u=max(.0001,r/rows);p=shell_point(t,u,.65 if inner else 0)
   du=shell_point(t,u+.0001)-shell_point(t,u-.0001);dt=shell_point(t+.0001,u)-shell_point(t-.0001,u)
   n=np.cross(du,dt);n/=max(np.linalg.norm(n),1e-9)
   m.vertex(p,-n if inner else n,(a/cols,r/rows))
   if r<rows and a<cols:
    # Narrow actual shell apertures reveal the recessed dark impact foam.
    vent=not inner and 10<=r<=18 and any(abs(a-k)<=1 for k in [12,25,52,76,103,116])
    if not vent:
     q=base+r*(cols+1)+a
     m.quad(q,q+1,q+cols+2,q+cols+1) if inner else m.quad(q,q+cols+1,q+cols+2,q+1)
rim=[shell_point(t,1) for t in np.linspace(0,2*math.pi,161)]
m.tube(rim,.32,'Rubber',sides=12)
# Swept rounded-rectangle chin shell. The broad side arms meet the ear shell,
# unlike the earlier unsupported tubular cage.
path=[]
for t in np.linspace(-math.pi/2,math.pi/2,81):
 path.append(np.array([-1+20*math.cos(t),11*math.sin(t),-8-8.5*math.cos(t)]))
for inner in [False,True]:
 base=len(m.v)+1;sides=24;m.mat='Liner' if inner else 'Paint'
 for i,p in enumerate(path):
  tangent=path[min(i+1,len(path)-1)]-path[max(i-1,0)];tangent[2]=0;tangent/=np.linalg.norm(tangent)
  out=np.array([tangent[1],-tangent[0],0]);height=3.6+1.0*abs(2*i/(len(path)-1)-1)
  for j in range(sides+1):
   a=j*2*math.pi/sides;c=math.cos(a);s=math.sin(a)
   # Superellipse cross-section preserves broad molded faces and softened edges.
   xx=math.copysign(abs(c)**.55,c);zz=math.copysign(abs(s)**.55,s)
   center=p-out*.7 if inner else p
   n=out*c+np.array([0,0,s]);m.vertex(center+out*xx*(.5 if inner else .95)+np.array([0,0,zz*height]),n,(j/sides,i/(len(path)-1)))
   if i<len(path)-1 and j<sides:
    q=base+i*(sides+1)+j;m.quad(q,q+sides+1,q+sides+2,q+1)
# Padded cheek cushions nest inside the shell, clear of the face opening.
for side in [-1,1]:
 m.loft([(-5,.4,-8,.5),(-3,1.8,-9,4),(2,2,-10.5,4.5),(6,1.5,-12,3.5),(8,.2,-13,.4)],'Liner',sides=32)
 # Move only the last loft's vertices to the corresponding cheek.
 for k in range(len(m.v)-(5*33+2),len(m.v)):m.v[k]=np.asarray(m.v[k])+np.array([0,side*8.4,0])
 # Retention webbing with a rectangular buckle, rather than rounded cords.
 m.box((-2,side*6.2,-15),(2,.32,8),.12,'Liner')
 m.bolt((3,side*11.1,1.7),.65)
 m.box((0,side*9.9,-6.5),(4,.25,1),.12,'Accent')
m.box((2,-5,-18.5),(3.4,1.2,2.2),.35,'Shell')
m.box((2,-5.65,-18.5),(1.7,.22,1.1),.12,'Accent')
# Mouth intake has a black recessed bed and narrow metal grille ribs.
m.box((19.9,0,-16.5),(1,8.6,3.9),.45,'Rubber')
for y in np.linspace(-3.5,3.5,8):m.box((20.43,y,-16.5),(.28,.23,2.9),.10,'Alloy')
for z in [-17.8,-15.2]:m.box((20.42,0,z),(.25,7.8,.18),.07,'Shell')
# Curved two-sided visor: narrow mounting roots, broad flared peak, rolled edge.
for bottom in [False,True]:
 m.mat='Shell' if bottom else 'Paint';base=len(m.v)+1
 for r in range(17):
  u=r/16;x=5.5+15*u;w=10.3+2*math.sin(math.pi*u)-2.2*u
  for a in range(41):
   v=a/20-1;z=4.5-2.6*u+.7*math.sin(math.pi*u)-1.3*v*v-(.42 if bottom else 0)
   du=np.array([15-1.8*v*v,(2*math.pi*math.cos(math.pi*u)-2.2)*v,-2.6+.7*math.pi*math.cos(math.pi*u)])
   dv=np.array([-3.6*v*u,w,-2.6*v]);n=np.cross(du,dv);n/=np.linalg.norm(n)
   m.vertex((x-1.8*v*v*u,w*v,z),-n if bottom else n,(u,a/40))
   if r<16 and a<40:
    q=base+r*41+a
    m.quad(q,q+1,q+42,q+41) if bottom else m.quad(q,q+41,q+42,q+1)
m.tube([(20.5-1.8*v*v,8.1*v,1.9-1.3*v*v-.18) for v in np.linspace(-1,1,49)],.23,'Rubber')
# Fine contrasting molded rear ridge and side graphics.
for side in [-1,1]:
 m.tube([shell_point(side*t,.67)+np.array([0,side*.12,.1]) for t in np.linspace(1.55,2.7,40)],.19,'Accent',sides=8)
publish(m)
m=Mesh('GuardV2');m.loft([(-1,.4,2,.5),(0,4,3,1.5),(5,5.2,3.5,2.4),(10,4,3,1.5),(12,.4,2,.5)],'Shell',sides=32)
for x in [1,9]:
 m.tube([(x,-4,1),(x,-5,-1),(x,0,-4),(x,5,-1),(x,4,1)],.6,'Liner')
for x in [3,5,7]:m.box((x,0,5.7),(1,3.2,.25),.1,'Rubber')
publish(m)
(OUT/'workshop.mtl').write_text('\n'.join(f'newmtl {name}\nKd .3 .3 .3\n' for name in ['Leather','Rubber','Shell','Paint','Liner','Lace','Alloy','Stitch','Urethane','Grip','Accent']))
(OUT/'inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
print(json.dumps(inventory,indent=2))
