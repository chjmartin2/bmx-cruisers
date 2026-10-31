"""One Loop subdivision of MIT Rocketbox meshes, preserving UV/material/skin data.
The source FBX files are untouched; authored derivatives have distinct names.
"""
from pathlib import Path
import sys,struct,zlib,json,copy,collections
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
from inspect_fbx import read
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/source/workshop'
def child(n,name):return next((c for c in n['children'] if c['name']==name),None)
def array(n,values,kind):
 fmt={'d':'d','i':'i','b':'?'}[kind];raw=struct.pack('<'+str(len(values))+fmt,*values);packed=zlib.compress(raw)
 n['raw_props']=kind.encode()+struct.pack('<III',len(values),1,len(packed))+packed;n['props']=[tuple(values)]
def serialize(nodes,version,footer_id):
 wide=version>=7500;hs=25 if wide else 13;headerfmt='<QQQB' if wide else '<IIIB';position=27
 def node(n):
  nonlocal position
  start=position;name=n['name'].encode();raw=n['raw_props'];position+=hs+len(name)+len(raw)
  kids=b''.join(node(c) for c in n['children']);tail=b'\0'*hs if n['children'] else b'';position+=len(tail)
  return struct.pack(headerfmt,position,len(n['props']),len(raw),len(name))+name+raw+kids+tail
 body=b'Kaydara FBX Binary  \x00\x1a\x00'+struct.pack('<I',version)+b''.join(node(n) for n in nodes)+b'\0'*hs
 footer=footer_id+b'\0'*4
 footer+=b'\0'*((-len(body)-len(footer))%16)
 footer+=struct.pack('<I',version)+b'\0'*120+bytes.fromhex('f85a8c6adef5d97eece90ce3758f290b')
 return body+footer
results=[]
for person,name in [('Male_Adult_01','YoungRiderV2'),('Female_Adult_01','YoungGirlV2')]:
 source=ROOT/'assets/source/rocketbox'/person/(person+'.fbx');data=source.read_bytes();version=struct.unpack_from('<I',data,23)[0];nodes=read(source)
 obj=next(n for n in nodes if n['name']=='Objects');geometries=[n for n in obj['children'] if n['name']=='Geometry']
 for geo in geometries:
  verts=np.array(child(geo,'Vertices')['props'][0]).reshape(-1,3);flat=child(geo,'PolygonVertexIndex')['props'][0]
  polys=[];poly=[]
  for i in flat:
   poly.append(-i-1 if i<0 else i)
   if i<0:polys.append(poly);poly=[]
  assert all(len(p)==3 for p in polys),'Source must be triangulated'
  tris=np.array(polys);edgefaces=collections.defaultdict(list);neighbors=[set() for _ in verts]
  for face,(a,b,c) in enumerate(tris):
   for u,v,opposite in [(a,b,c),(b,c,a),(c,a,b)]:edgefaces[tuple(sorted((int(u),int(v))))].append(int(opposite));neighbors[u].add(int(v));neighbors[v].add(int(u))
  edgeidx={e:len(verts)+i for i,e in enumerate(edgefaces)};new=[]
  for i,v in enumerate(verts):
   border=[j for j in neighbors[i] if len(edgefaces[tuple(sorted((i,j)))])==1]
   if len(border)==2:p=v*.75+(verts[border[0]]+verts[border[1]])*.125
   elif len(border)>0 or len(neighbors[i])<3:p=v
   else:
    n=len(neighbors[i]);beta=3/16 if n==3 else 3/(8*n);p=(1-n*beta)*v+beta*sum((verts[j] for j in neighbors[i]),np.zeros(3))
   new.append(p)
  for (a,b),op in edgefaces.items():new.append((verts[a]+verts[b])*.375+(verts[op[0]]+verts[op[1]])*.125 if len(op)==2 else (verts[a]+verts[b])*.5)
  new=np.array(new);faces=[]
  for a,b,c in tris:
   ab=edgeidx[tuple(sorted((a,b)))];bc=edgeidx[tuple(sorted((b,c)))];ca=edgeidx[tuple(sorted((c,a)))];faces.extend([(a,ab,ca),(b,bc,ab),(c,ca,bc),(ab,bc,ca)])
  faces=np.array(faces);out=faces.copy();out[:,2]=-out[:,2]-1;array(child(geo,'Vertices'),new.ravel(),'d');array(child(geo,'PolygonVertexIndex'),out.ravel(),'i')
  geo['children']=[n for n in geo['children'] if n['name']!='Edges']
  normals=np.zeros_like(new)
  for face in faces:
   a,b,c=new[face];n=np.cross(b-a,c-a)
   for i in face:normals[i]+=n
  normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-12)
  normal_layer=child(geo,'LayerElementNormal');array(child(normal_layer,'Normals'),normals[faces.ravel()].ravel(),'d')
  uv_layer=child(geo,'LayerElementUV');uv=np.array(child(uv_layer,'UV')['props'][0]).reshape(-1,2);uvindex=np.array(child(uv_layer,'UVIndex')['props'][0]).reshape(-1,3);newuv=[]
  for ids in uvindex:
   a,b,c=uv[ids];ab=(a+b)/2;bc=(b+c)/2;ca=(c+a)/2;newuv.extend([a,ab,ca,b,bc,ab,c,ca,bc,ab,bc,ca])
  array(child(uv_layer,'UV'),np.array(newuv).ravel(),'d');array(child(uv_layer,'UVIndex'),list(range(len(newuv))),'i')
  for layer_name,field in [('LayerElementMaterial','Materials'),('LayerElementVisibility','Visibility')]:
   layer=child(geo,layer_name)
   if layer:
    n=child(layer,field)
    if n and len(n['props'][0])==len(tris):array(n,np.repeat(n['props'][0],4),'b' if field=='Visibility' else 'i')
  for deformer in obj['children']:
   if deformer['name']!='Deformer':continue
   ix=child(deformer,'Indexes');weight=child(deformer,'Weights')
   if ix is None or weight is None:continue
   if max(ix['props'][0],default=-1)>=len(verts):continue
   weights=dict(zip(ix['props'][0],weight['props'][0]));dense=dict(weights)
   for (a,b),index in edgeidx.items():
    value=(weights.get(a,0)+weights.get(b,0))*.5
    if value>1e-7:dense[index]=value
   array(ix,list(dense),'i');array(weight,list(dense.values()),'d')
  results.append(dict(name=name,source_vertices=len(verts),vertices=len(new),triangles=len(faces),skin_clusters='preserved and interpolated',license='MIT / Microsoft Rocketbox'))
 i=27
 while True:
  end=struct.unpack_from('<Q' if version>=7500 else '<I',data,i)[0]
  if not end:break
  i=end
 footer_id=data[i+(25 if version>=7500 else 13):i+(25 if version>=7500 else 13)+16]
 (OUT/(name+'.fbx')).write_bytes(serialize(nodes,version,footer_id))
(OUT/'rider-derivatives.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results,indent=2))
