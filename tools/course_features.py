"""Recognize original objects by character code AND color, never course number."""
from pathlib import Path
from collections import deque
import json
ROOT=Path(__file__).resolve().parents[1]

def groups(cells):
 cells=set(cells);result=[]
 while cells:
  first=min(cells,key=lambda p:(p[1],p[0]));cells.remove(first);q=deque([first]);part=[]
  while q:
   x,y=q.popleft();part.append((x,y))
   for p in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
    if p in cells:cells.remove(p);q.append(p)
  result.append(part)
 return result

def bounds(cells):
 xs,ys=zip(*cells);return [min(xs)*8,min(ys)*8,(max(xs)+1)*8,(max(ys)+1)*8]

def extract(data):
 codes=data['codes'];colors=data['colors'];pools=[];lots=[];cars=[];oil=[];markings=[]
 # 0x12 is the rectangular pool's tiled surround. Natural ponds never use it.
 for cells in groups((x,y) for y,row in enumerate(codes) for x,c in enumerate(row) if c==0x12):
  b=bounds(cells)
  if len(cells)<12:raise ValueError('Incomplete original swimming-pool surround')
  pools.append({'bounds':b})
 rows=sorted({y for y,row in enumerate(codes) if 0x25 in row});bands=[]
 for y in rows:
  if not bands or y-bands[-1][-1]>6:bands.append([])
  bands[-1].append(y)
 for band in bands:
  ys=range(min(band),max(band)+1)
  xs=[x for y in ys for x,c in enumerate(codes[y]) if c==0x25]
  b=[(min(xs)-4)*8,min(ys)*8,(max(xs)+4)*8,(max(ys)+1)*8]
  lots.append({'bounds':b})
  for y in ys:
   for x,c in enumerate(codes[y]):
    if c==0x25:markings.append([x*8,y*8,x*8+2,(y+1)*8])
  # Cars share glyphs with oil and water. Their color and parking context
  # disambiguate them; testing a glyph alone loses or mislabels the objects.
  for color in (10,14):
   cells=[(x,y) for y in ys for x,c in enumerate(codes[y]) if c in (42,43,44,49,50,51,87) and colors[y][x]&15==color]
   for part in groups(cells):cars.append({'bounds':bounds(part),'color':'red' if color==10 else 'blue'})
  cells=[(x,y) for y in ys for x,c in enumerate(codes[y]) if c in (42,43,45,46,47) and colors[y][x]&15==8]
  for part in groups(cells):oil.append({'bounds':bounds(part),'cells':[[x,y,codes[y][x]] for x,y in part]})
 return {'format':1,'pools':pools,'parking_lots':lots,'cars':cars,'oil_slicks':oil,'parking_markings':markings}

def main():
 for n in range(1,6):
  path=ROOT/f'Content/Data/course{n}.json';data=json.loads(path.read_text());restore_types(data)
  path.write_text(json.dumps(data,separators=(',',':'))+'\n',encoding='utf8')
  print(n,{k:len(v) for k,v in data['features'].items() if isinstance(v,list)})
def restore_types(data):
 data['features']=extract(data)
 # The red car's full-color glass/body glyph is also used for bridge rails.
 # Remove only those misidentified car glyphs from the derived fence list.
 data['fences']=[p for p in data['fences'] if not any(x0<=p['x']*8<x1 and y0<=p['y']*8<y1 for x0,y0,x1,y1 in [c['bounds'] for c in data['features']['cars']])]
 return data
if __name__=='__main__':main()
