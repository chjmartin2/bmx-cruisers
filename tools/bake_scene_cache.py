"""Pack authored meshes into bounded, compressed, spatially culled draw chunks."""
from pathlib import Path
import sys,json,struct,zlib,hashlib
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'.tools/python'))
import numpy as np
FIELDS=['grass','road','water','waterbed','banks','verge','grass_underlay','parking','pool_water','pool_deck']

def bake(course):
 data=ROOT/'Content/Data';source=data/f'course{course}-visuals.json';raw=source.read_bytes();visual=json.loads(raw)
 payload=bytearray(struct.pack('<I',len(FIELDS)));total_triangles=0;total_chunks=0
 for name in FIELDS:
  part=visual[name];v=np.asarray(part['vertices'],dtype=np.float64).reshape(-1,3);tri=np.asarray(part['indices'],dtype=np.uint32).reshape(-1,3)
  n=np.zeros_like(v);n[:,2]=1
  if name in ('grass','banks') and len(tri):
   n[:]=0;face=np.cross(v[tri[:,2]]-v[tri[:,0]],v[tri[:,1]]-v[tri[:,0]]);length=np.linalg.norm(face,axis=1);face/=np.maximum(length[:,None],1e-20)
   for side in range(3):np.add.at(n,tri[:,side],face)
   n/=np.maximum(np.linalg.norm(n,axis=1)[:,None],1e-20)
  uv=v[:,:2]/200
  colors=np.asarray(part.get('colors',[[1,1,1,1]]*len(v)),dtype=np.float32).reshape(-1,4)
  if name=='pool_deck':colors[:]=[.72,.75,.72,1]
  if name=='grass_underlay':
   v[v[:,1]<=-2025,1]=-8000;v[v[:,1]>=2025,1]=8000
   height=json.loads((data/f'course{course}.json').read_text())['height']
   v[v[:,0]<=-750,0]=-6000;end=(height*8+100)*7.5;v[v[:,0]>=end,0]=end+6000
  label=name.encode();payload.extend(struct.pack('<I',len(label))+label)
  keys=np.floor(v[tri,0].mean(axis=1)/1200).astype(np.int32) if len(tri) else np.array([],np.int32)
  unique=np.unique(keys);payload.extend(struct.pack('<I',len(unique)))
  for key in unique:
   selected=tri[keys==key];used,remap=np.unique(selected,return_inverse=True)
   # Keep positions at source double precision. Water contact boundaries must
   # remain identical even at the few grid points exactly on a contour.
   packed=np.empty(len(used),dtype=[('position','<f8',3),('attributes','<f4',9)])
   packed['position']=v[used];packed['attributes']=np.column_stack([n[used],uv[used],colors[used]])
   payload.extend(struct.pack('<II',len(used),selected.size));payload.extend(packed.tobytes());payload.extend(remap.astype('<u4').tobytes())
  total_triangles+=len(tri);total_chunks+=len(unique)
 packed=b'BMXSCN02'+struct.pack('<II',len(payload),zlib.crc32(payload))+zlib.compress(payload,6)
 (data/f'course{course}-scene.bin').write_bytes(packed)
 metadata={k:v for k,v in visual.items() if k not in FIELDS}
 metadata['source_sha256']=hashlib.sha256(raw).hexdigest()
 (data/f'course{course}-scene.json').write_text(json.dumps(metadata,separators=(',',':'))+'\n')
 print(f'Course {course}: {len(raw):,} JSON bytes -> {len(packed):,} cached bytes; {total_triangles:,} triangles in {total_chunks} culled chunks',flush=True)
if __name__=='__main__':
 for c in range(1,6):bake(c)
