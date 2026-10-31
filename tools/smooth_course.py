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

def smooth(rectangles,min_area):
    source=union_all([box(x,y,x+w,y+h) for x,y,w,h in rectangles])
    result=[]
    for p in polygons(source):
        if p.area<min_area:continue
        p=p.simplify(2.75,preserve_topology=True)
        holes=[spline(h) for h in p.interiors if Polygon(h).area>8]
        result.extend(polygons(make_valid(Polygon(spline(p.exterior),holes))))
    return union_all(result),source

def mesh(geometry,z):
    vertices=[];indices=[]
    for polygon in polygons(geometry):
        if polygon.area<.05:continue
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

def bank(geometry):
    vertices=[];indices=[]
    for polygon in polygons(geometry):
        p=orient(polygon,sign=1)
        for ring in [p.exterior,*p.interiors]:
            points=np.asarray(ring.coords[:-1]);base=len(vertices)
            for i,point in enumerate(points):
                tangent=points[(i+1)%len(points)]-points[(i-1)%len(points)]
                normal=np.array([tangent[1],-tangent[0]])/max(.001,np.linalg.norm(tangent))
                for pos,z in [(point-normal*1.5,-38),(point,-11),(point+normal*.85,-2)]:
                    vertices.append([round(float(pos[1])*7.5,3),round((float(pos[0])-160)*7.5,3),z])
            for i in range(len(points)):
                a=base+i*3;b=base+((i+1)%len(points))*3
                for band in range(2):
                    indices.extend([a+band,b+band+1,b+band,a+band,a+band+1,b+band+1])
    return dict(vertices=vertices,indices=indices)

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

def export():
    data=json.loads((ROOT/'Content/Data/course1.json').read_text())
    road,original_road=smooth(data['road_rects'],12)
    water,original_water=smooth(data['water_rects'],16)
    field=box(-110,-100,430,data['height']*8+100)
    result=dict(format=1,method='Periodic Catmull-Rom after 2.75-pixel contour simplification with circular smoothing',
                road=mesh(road,0),water=mesh(water,-10),
                grass=mesh(field.difference(water.buffer(.8)), -2),
                waterbed=mesh(water.buffer(.85),-38),banks=bank(water),
                verge=trail_edge(road))
    # Only the meadow undulates. Rideable asphalt and every obstacle stay in the
    # original plane, so terrain dressing cannot change a gap or the riding line.
    import shapely
    gx=np.arange(-110,434,4,dtype=float);gy=np.arange(-100,data['height']*8+104,4,dtype=float)
    xx,yy=np.meshgrid(gx,gy);points=shapely.points(xx.ravel(),yy.ravel())
    distance=shapely.distance(points,union_all([road,water])).reshape(xx.shape)
    fade=np.clip((distance-2)/12,0,1)
    world_x=yy*7.5;world_y=(xx-160)*7.5
    heights=-2+fade*(16+11*np.sin(world_x/330)*np.sin(world_y/240)+9*np.sin(world_x/510+world_y/340)+3*np.sin(world_x/93+world_y/143))
    submerged=shapely.contains(water,points).reshape(xx.shape)
    water_distance=shapely.distance(points,water).reshape(xx.shape)
    heights=np.maximum(heights,-1.8)
    vertices=np.stack([world_x,world_y,heights],axis=-1).reshape(-1,3)
    indices=[];width=len(gx)
    for row in range(len(gy)-1):
        for col in range(width-1):
            a=row*width+col
            if np.min(water_distance[row:row+2,col:col+2])<3:continue
            indices.extend([a,a+1,a+width+1,a,a+width+1,a+width])
    # Warm/cool growth patches at metre scale, rather than one perfectly even lawn.
    shade=.82+.12*np.sin(world_x/430+world_y/370)+.07*np.cos(world_x/180-world_y/290)
    colors=np.stack([shade*.92,shade,shade*.90,np.ones_like(shade)],axis=-1).reshape(-1,4)
    result['grass_underlay']=mesh(field.difference(water.buffer(.8)),-2)
    result['grass']=dict(vertices=np.round(vertices,3).tolist(),indices=indices,colors=np.round(colors,4).tolist())
    heights.astype('<f4').tofile(ROOT/'Content/Data/meadow-heights.bin')
    (ROOT/'Content/Data/course1-visuals.json').write_text(json.dumps(result,separators=(',',':'))+'\n')
    print('Smoothed meshes:',{name:len(result[name]['indices'])//3 for name in ['road','water','grass','waterbed','banks','verge']})
    print('Main road area change:',round(100*(road.area-original_road.area)/original_road.area,3),'percent')
    assert road.is_valid and water.is_valid
    assert abs(road.area-original_road.area)/original_road.area<.03
if __name__=='__main__':export()
