"""Create smooth visual contours while leaving all original gameplay bytes intact."""
from pathlib import Path
import sys,json,math
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.tools/python'))
import numpy as np
from shapely import union_all,make_valid
from shapely.geometry import box,Polygon,MultiPolygon
from shapely.geometry.polygon import orient
import mapbox_earcut
import shapely

def polygons(geometry):
    if isinstance(geometry,Polygon):return [geometry]
    if hasattr(geometry,'geoms'):return [p for g in geometry.geoms for p in polygons(g)]
    return []

def spline(ring):
    # Remove the raster stair steps, then interpolate a periodic Catmull-Rom curve.
    points=np.asarray(ring.coords[:-1],dtype=float)
    output=[]
    for i,p1 in enumerate(points):
        p0,p2,p3=points[(i-1)%len(points)],points[(i+1)%len(points)],points[(i+2)%len(points)]
        count=max(2,math.ceil(np.linalg.norm(p2-p1)/.5))
        for t in np.arange(count)/count:
            output.append(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t))
    # Smooth the residual sub-tile quantisation along the densely sampled curve.
    # Sampling is ~0.5 pixel, so this spans only a few original pixels.
    curve=np.asarray(output)
    for _ in range(12):curve=(np.roll(curve,1,axis=0)+2*curve+np.roll(curve,-1,axis=0))/4
    return curve

def smooth(rectangles,min_area,tolerance=2.75):
    source=union_all([box(x,y,x+w,y+h) for x,y,w,h in rectangles])
    result=[]
    for p in polygons(source):
        if p.area<min_area:continue
        p=p.simplify(tolerance,preserve_topology=True)
        holes=[spline(h) for h in p.interiors if Polygon(h).area>8]
        result.extend(polygons(make_valid(Polygon(spline(p.exterior),holes))))
    return union_all(result),source

def mesh(geometry,z):
    vertices=[];indices=[]
    for polygon in polygons(geometry):
        if polygon.area<1e-9:continue
        polygon=orient(polygon,sign=1)
        rings=[polygon.exterior,*polygon.interiors]
        coords=np.concatenate([np.asarray(r.coords[:-1]) for r in rings])
        ends=np.cumsum([len(r.coords)-1 for r in rings],dtype=np.uint32)
        triangles=mapbox_earcut.triangulate_float64(coords,ends)
        base=len(vertices)
        vertices.extend([[round(float(y)*7.5,3),round((float(x)-160)*7.5,3),z] for x,y in coords])
        # XY -> forward/right swaps orientation; these indices face upward in UE.
        indices.extend((triangles+base).tolist())
    return dict(vertices=vertices,indices=indices)

def smooth_water(rectangles):
    source=union_all([box(x,y,x+w,y+h) for x,y,w,h in rectangles])
    water=source.simplify(.7,preserve_topology=True)
    # Filter at uniform physical distances, rather than once per raster corner.
    # The old unevenly sampled filter left the 8-pixel source steps visible.
    for sigma in (3.,2.4,1.9,1.5,1.):
        result=[]
        for polygon in polygons(water):
            rings=[]
            for ring in [polygon.exterior,*polygon.interiors]:
                pts=np.asarray(ring.coords);lengths=np.r_[0,np.cumsum(np.linalg.norm(np.diff(pts,axis=0),axis=1))]
                count=max(16,math.ceil(lengths[-1]/.5));dist=np.arange(count)*lengths[-1]/count
                curve=np.stack([np.interp(dist,lengths,pts[:,i]) for i in (0,1)],axis=1)
                radius=math.ceil(sigma*6);weights=np.exp(-.5*(np.arange(-radius,radius+1)*.5/sigma)**2);weights/=weights.sum()
                curve=sum(w*np.roll(curve,i-radius,axis=0) for i,w in enumerate(weights))
                rings.append(curve)
            result.extend(polygons(make_valid(Polygon(rings[0],rings[1:]))))
        result=union_all(result)
        if abs(result.area-source.area)/source.area<.03 and result.hausdorff_distance(water)<4:break
    assert abs(result.area-source.area)/source.area<.03
    return result,source

def meadow_color(x,y):
    shade=.82+.12*np.sin(x/430+y/370)+.07*np.cos(x/180-y/290)
    return [float(shade*.92),float(shade),float(shade*.90)]

def boundary_distance(geometry,points):
    segments=[]
    for p in polygons(geometry):
        for ring in [p.exterior,*p.interiors]:
            q=np.asarray(ring.coords);segments.extend(np.stack([q[:-1],q[1:]],axis=1))
    segments=shapely.linestrings(np.asarray(segments))
    nearest=shapely.STRtree(segments).nearest(points)
    return shapely.distance(points,segments[nearest])

def bank(geometry):
    result=dict(vertices=[],indices=[],colors=[])
    # Shared curved offset contours replace the old 6-cm flat strip. Each
    # adjoining band shares its boundary exactly; the outer edge meets grass.
    offsets=(-3.,-2.,-1.,0.,1.,2.,3.,4.,4.75,5.)
    lookup={}
    for inner,outer in zip(offsets,offsets[1:]):
        region=geometry.buffer(outer,quad_segs=8).difference(geometry.buffer(inner,quad_segs=8))
        part=mesh(region,0);remap=[]
        for x,y,_ in part['vertices']:
            key=(x,y)
            if key not in lookup:
                lookup[key]=len(result['vertices']);result['vertices'].append([x,y,0])
            remap.append(lookup[key])
        result['indices'].extend(remap[i] for i in part['indices'])
    v=np.asarray(result['vertices']);points=shapely.points(v[:,1]/7.5+160,v[:,0]/7.5)
    d=boundary_distance(geometry,points)*np.where(shapely.contains(geometry,points),-1,1)
    t=np.clip(d/5,0,1);land=t*t*(3-2*t);u=np.clip(-d/3,0,1);depth=u*u*(3-2*u)
    v[:,2]=np.where(d>=0,-2-8*(1-land),-10-28*depth)
    result['vertices']=np.round(v,3).tolist()
    result['colors']=[[*meadow_color(x,y),round(float(alpha),5)] for (x,y,z),alpha in zip(v,1-land)]
    return result

def clipped_meadow(vertices,width,land):
    """Keep the height grid for fences; clip its drawn triangles to the bank."""
    result=vertices.tolist();indices=[];lookup={(round(v[0],3),round(v[1],3)):i for i,v in enumerate(vertices)}
    grid=np.arange(len(vertices)).reshape(-1,width)
    a=grid[:-1,:-1].ravel();b=grid[:-1,1:].ravel();c=grid[1:,1:].ravel();d=grid[1:,:-1].ravel()
    triangles=np.concatenate([np.stack([a,b,c],axis=1),np.stack([a,c,d],axis=1)])
    # Shapely operates in the source XY plane; rendering uses forward/right.
    xy=np.stack([vertices[:,1]/7.5+160,vertices[:,0]/7.5],axis=1)
    shapes=shapely.polygons(xy[triangles]);inside=shapely.contains(land,shapes)
    indices.extend(triangles[inside].ravel().tolist())
    edge=np.flatnonzero(~inside & shapely.intersects(land,shapes))
    for k in edge:
        part=mesh(shapes[k].intersection(land),0);local=[]
        for x,y,_ in part['vertices']:
            key=(round(x,3),round(y,3))
            if key not in lookup:
                # New clip vertices lie on the bank boundary: join the -2 cm
                # grass lip exactly instead of inheriting a raised grid edge.
                lookup[key]=len(result);result.append([x,y,-2])
            local.append(lookup[key])
        indices.extend(local[i] for i in part['indices'])
    return dict(vertices=result,indices=indices,colors=[[*meadow_color(x,y),1] for x,y,z in result])

def shoreline_samples(geometry):
    result=[]
    for p in polygons(geometry):
        for ring in [orient(p,sign=1).exterior,*orient(p,sign=1).interiors]:
            count=max(3,math.ceil(ring.length/9))
            for i in range(count):
                q=ring.interpolate(i*ring.length/count)
                result.append([round(q.y*7.5,3),round((q.x-160)*7.5,3),-10])
    return result

def trail_edge(geometry):
    vertices=[];indices=[];colors=[]
    for polygon in polygons(geometry):
        p=orient(polygon,sign=1)
        for ring in [p.exterior,*p.interiors]:
            points=np.asarray(ring.coords[:-1]);base=len(vertices)
            for i,point in enumerate(points):
                tangent=points[(i+1)%len(points)]-points[(i-1)%len(points)]
                normal=np.array([tangent[1],-tangent[0]])/max(.001,np.linalg.norm(tangent))
                for pos,z,alpha in [(point,.08,1),(point+normal*1.5,-1.8,0)]:
                    vertices.append([round(float(pos[1])*7.5,3),round((float(pos[0])-160)*7.5,3),z]);colors.append([1,1,1,alpha])
            for i in range(len(points)):
                a=base+i*2;b=base+((i+1)%len(points))*2
                indices.extend([a,b+1,b,a,a+1,b+1])
    return dict(vertices=vertices,indices=indices,colors=colors)

def export(course=1):
    data=json.loads((ROOT/f'Content/Data/course{course}.json').read_text(encoding='utf-8'))
    road,original_road=smooth(data['road_rects'],12,2.75 if course==1 else .5)
    # Dense later routes have many small corners. Limit their aggregate rounding
    # gain with sub-pixel offsets, keeping the same area budget as Course One.
    if course>1:
        for _ in range(20):
            if abs(road.area-original_road.area)/original_road.area<.03:break
            road=road.buffer(-.05 if road.area>original_road.area else .05)
    water,original_water=smooth_water(data['water_rects'])
    outer_bank=water.buffer(5,quad_segs=8)
    field=box(-110,-100,430,data['height']*8+100)
    result=dict(format=2,method='Catmull-Rom road; arc-length Gaussian shoreline with clipped meadow and blended sloping bank',
                road=mesh(road,0),water=mesh(water,-10),
                waterbed=mesh(water.buffer(.85),-38),banks=bank(water),shoreline_samples=shoreline_samples(water),
                verge=trail_edge(road))
    # Only the meadow undulates. Rideable asphalt and every obstacle stay in the
    # original plane, so terrain dressing cannot change a gap or the riding line.
    import shapely
    gx=np.arange(-110,434,4,dtype=float);gy=np.arange(-100,data['height']*8+104,4,dtype=float)
    xx,yy=np.meshgrid(gx,gy);points=shapely.points(xx.ravel(),yy.ravel())
    flat=union_all([road,outer_bank]);shapely.prepare(flat)
    distance=np.where(shapely.contains(flat,points),0,boundary_distance(flat,points)).reshape(xx.shape)
    fade=np.clip((distance-2)/12,0,1)
    world_x=yy*7.5;world_y=(xx-160)*7.5
    heights=-2+fade*(16+11*np.sin(world_x/330)*np.sin(world_y/240)+9*np.sin(world_x/510+world_y/340)+3*np.sin(world_x/93+world_y/143))
    heights=np.maximum(heights,-2)
    vertices=np.stack([world_x,world_y,heights],axis=-1).reshape(-1,3)
    result['grass_underlay']=mesh(field.difference(outer_bank),-3)
    result['grass_underlay']['colors']=[[*meadow_color(x,y),1] for x,y,z in result['grass_underlay']['vertices']]
    result['meadow_grid_vertices']=len(vertices)
    result['grass']=clipped_meadow(np.round(vertices,3),len(gx),field.difference(outer_bank))
    px,py=np.meshgrid(np.arange(320,dtype=float),np.arange(data['height']*8,dtype=float))
    shapely.contains(water.buffer(5.5),shapely.points(px,py)).astype('uint8').tofile(ROOT/f'Content/Data/course{course}-shore-mask.bin')
    heights.astype('<f4').tofile(ROOT/('Content/Data/meadow-heights.bin' if course==1 else f'Content/Data/course{course}-meadow.bin'))
    (ROOT/f'Content/Data/course{course}-visuals.json').write_text(json.dumps(result,separators=(',',':'))+'\n',encoding='utf-8')
    print('Smoothed meshes:',{name:len(result[name]['indices'])//3 for name in ['road','water','grass','waterbed','banks','verge']})
    print('Main road area change:',round(100*(road.area-original_road.area)/original_road.area,3),'percent')
    print('Water area change:',round(100*(water.area-original_water.area)/original_water.area,3),'percent')
    assert road.is_valid and water.is_valid
    assert abs(road.area-original_road.area)/original_road.area<.03
if __name__=='__main__':export()
