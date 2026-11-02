from pathlib import Path
import sys,json,zlib,struct,hashlib,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'.tools/python'))
import numpy as np
class SceneCache(unittest.TestCase):
 def test_all_course_meshes_and_collision_surfaces(self):
  for c in range(1,6):
   source=(ROOT/f'Content/Data/course{c}-visuals.json').read_bytes();authored=json.loads(source)
   meta=json.loads((ROOT/f'Content/Data/course{c}-scene.json').read_text());self.assertEqual(meta['source_sha256'],hashlib.sha256(source).hexdigest());self.assertEqual(meta['site_features'],authored['site_features'])
   file=(ROOT/f'Content/Data/course{c}-scene.bin').read_bytes();self.assertEqual(file[:8],b'BMXSCN02')
   size,crc=struct.unpack_from('<II',file,8);data=zlib.decompress(file[16:]);self.assertEqual(len(data),size);self.assertEqual(zlib.crc32(data),crc)
   offset=0
   def integer():
    nonlocal offset
    n=struct.unpack_from('<I',data,offset)[0];offset+=4;return n
   fields=integer();self.assertEqual(fields,10)
   for _ in range(fields):
    length=integer();name=data[offset:offset+length].decode();offset+=length;chunks=integer();triangles=0;rendered=[]
    for _ in range(chunks):
     nv,ni=integer(),integer();v=np.frombuffer(data,dtype=[('position','<f8',3),('attributes','<f4',9)],count=nv,offset=offset);offset+=nv*60
     i=np.frombuffer(data,dtype='<u4',count=ni,offset=offset);offset+=ni*4
     self.assertTrue(np.isfinite(v['position']).all());self.assertTrue(np.isfinite(v['attributes']).all());self.assertTrue((i<nv).all());self.assertEqual(ni%3,0);triangles+=ni//3
     if name in ('water','pool_water'):rendered.extend(v['position'][i].reshape(-1,9).tolist())
    self.assertEqual(triangles,len(authored[name]['indices'])//3)
    if name in ('water','pool_water'):
     original=np.asarray(authored[name]['vertices']).reshape(-1,3);indices=np.asarray(authored[name]['indices'],dtype=int)
     expected=original[indices].reshape(-1,9)
     # Every corner remains exactly equal to the source double, including
     # winding. Only whole triangles are reordered for spatial culling.
     original_sorted=sorted(map(tuple,expected))
     render_sorted=sorted(map(tuple,np.asarray(rendered).reshape(-1,9)))
     self.assertEqual(original_sorted,render_sorted)
   self.assertEqual(offset,len(data))
if __name__=='__main__':unittest.main()
