"""Check shared shoreline boundaries without depending on a render screenshot."""
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.tools/python'))
import numpy as np
import shapely
for course in range(1,6):
    data=json.loads((ROOT/f'Content/Data/course{course}-visuals.json').read_text(encoding='utf-8'))
    grass=data['grass'];bank=data['banks'];water=data['water']
    assert data['meadow_grid_vertices']%136==0
    assert data['meadow_grid_vertices']<=len(grass['vertices'])
    grass_lip={(round(x,3),round(y,3)) for x,y,z in grass['vertices'] if abs(z+2)<.002}
    water_edge={(round(x,3),round(y,3)) for x,y,z in water['vertices']}
    outer=[(round(v[0],3),round(v[1],3)) for v,c in zip(bank['vertices'],bank['colors']) if c[3]<.00001]
    wet=[(round(v[0],3),round(v[1],3)) for v in bank['vertices'] if abs(v[2]+10)<.00001]
    assert outer and wet
    missing=[p for p in outer if p not in grass_lip]
    if missing:
        # Polygon clipping may remove collinear vertices, leaving them exactly
        # on a longer drawn edge. Test the surface edges, not just vertex IDs.
        v=np.asarray(grass['vertices']);t=np.asarray(grass['indices']).reshape(-1,3)
        e=np.concatenate([t[:,[0,1]],t[:,[1,2]],t[:,[2,0]]])
        e=e[np.all(np.abs(v[e,2]+2)<.002,axis=1)]
        lines=shapely.linestrings(v[e,:2]);points=shapely.points(missing)
        distance=shapely.distance(points,lines[shapely.STRtree(lines).nearest(points)])
        assert np.max(distance)<.003,f'Course {course}: grass/bank crack {np.max(distance)} cm'
    assert all(p in water_edge for p in wet),f'Course {course}: bank/water crack'
    assert all(-38.001<=v[2]<=-1.999 for v in bank['vertices'])
    print(f'Course {course}: {len(outer)} grass joins and {len(wet)} water joins continuous')
