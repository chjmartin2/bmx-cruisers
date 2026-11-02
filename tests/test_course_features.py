"""Check object recognition against all five original map sequences."""
from pathlib import Path
import sys,json,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from course_features import extract

class OriginalSites(unittest.TestCase):
 def test_every_original_occurrence(self):
  for n,(p,l,c,o) in enumerate([(0,0,0,0),(1,0,0,0),(1,2,4,8),(0,2,4,8),(1,3,6,12)],1):
   d=json.loads((ROOT/f'Content/Data/course{n}.json').read_text());f=extract(d)
   self.assertEqual([len(f[k]) for k in ['pools','parking_lots','cars','oil_slicks']],[p,l,c,o])
   self.assertEqual(d['features'],f)
   v=json.loads((ROOT/f'Content/Data/course{n}-visuals.json').read_text())
   self.assertEqual(v['site_features'],f)
   for item in f['cars']+f['oil_slicks']:
    x0,y0,x1,y1=item['bounds']
    self.assertTrue(any(a<=x0<x1<=c and b<=y0<y1<=d for a,b,c,d in [lot['bounds'] for lot in f['parking_lots']]))
   for car in f['cars']:
    x0,y0,x1,y1=car['bounds'];point=((y0+y1)*3.75,((x0+x1)*.5-160)*7.5)
    self.assertFalse(any(x0<=q['x']*8<x1 and y0<=q['y']*8<y1 for q in d['fences']))
    water=v['water'];vertices=water['vertices'];indices=water['indices']
    for i in range(0,len(indices),3):
     a,b,c=[vertices[k] for k in indices[i:i+3]]
     def side(u,w):return (w[0]-u[0])*(point[1]-u[1])-(w[1]-u[1])*(point[0]-u[0])
     signs=[side(a,b),side(b,c),side(c,a)]
     self.assertFalse(all(x>=0 for x in signs) or all(x<=0 for x in signs),'Blue car was misidentified as water')
   self.assertEqual(bool(v['parking']['indices']),bool(l))
   self.assertEqual(bool(v['pool_water']['indices']),bool(p))
   self.assertEqual(bool(v['pool_deck']['indices']),bool(p))
 def test_shared_glyphs_are_not_shared_object_types(self):
  d=json.loads((ROOT/'Content/Data/course3.json').read_text());f=extract(d)
  self.assertEqual([c['color'] for c in f['cars']].count('red'),2)
  self.assertEqual([c['color'] for c in f['cars']].count('blue'),2)
  self.assertEqual(sorted((x[2]-x[0],x[3]-x[1]) for x in [o['bounds'] for o in f['oil_slicks']]),[(8,8)]*6+[(16,16)]*2)
  # Clear the oil's black color while retaining its glyph: it must not become
  # an oil hazard merely because the same glyph also occurs on a car.
  x,y,_=f['oil_slicks'][0]['cells'][0];d['colors'][y][x]=10
  self.assertNotEqual(extract(d)['oil_slicks'],f['oil_slicks'])
if __name__=='__main__':unittest.main()
