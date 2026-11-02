"""Prepare only the original pool / car-park materials in Unreal Editor."""
from pathlib import Path
import sys
import unreal as ue
sys.path.insert(0,str(Path(ue.Paths.project_dir()).resolve()/'tools'))
from prepare_unreal_assets import material,expression,scalar,color,link,output,mul,EDIT,add,mix,component,clamp01,subtract

def save(m):EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)
def noise(m,scale,levels=2):
 n=expression(m,ue.MaterialExpressionNoise,quality=1,levels=levels,scale=scale,output_min=0,output_max=1)
 link(expression(m,ue.MaterialExpressionWorldPosition),n);return n
def vertex(m):return expression(m,ue.MaterialExpressionVertexColor)

m=material('Parking',roughness=.87)
base=mix(m,color(m,(.009,.011,.013)),color(m,(.024,.026,.028)),noise(m,.003))
grain=add(m,scalar(m,.88),mul(m,noise(m,.9,1),scalar(m,.18)))
output(mul(m,base,grain),m,ue.MaterialProperty.MP_BASE_COLOR)
output(add(m,scalar(m,.80),mul(m,noise(m,.9,1),scalar(m,.16))),m,ue.MaterialProperty.MP_ROUGHNESS)
save(m)
m=material('ParkingPaint',roughness=.78);output(vertex(m),m,ue.MaterialProperty.MP_BASE_COLOR);save(m)
m=material('CarPaint',roughness=.22,metallic=.38);output(vertex(m),m,ue.MaterialProperty.MP_BASE_COLOR)
# Clear-coat's unconnected custom inputs use Unreal's coat defaults.
m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_CLEAR_COAT);save(m)
m=material('CarGlass',(.018,.038,.052),.10,.4);save(m)
m=material('PoolTiles',roughness=.34)
uv=mul(m,expression(m,ue.MaterialExpressionTextureCoordinate),scalar(m,16))
fraction=expression(m,ue.MaterialExpressionFrac);link(uv,fraction)
edges=[]
for axis in (0,1):
 a=expression(m,ue.MaterialExpressionAbs);link(subtract(m,mul(m,component(m,fraction,axis),scalar(m,2)),scalar(m,1)),a)
 edges.append(clamp01(m,mul(m,subtract(m,a,scalar(m,.93)),scalar(m,20))))
edge=expression(m,ue.MaterialExpressionMax);link(edges[0],edge,'A');link(edges[1],edge,'B')
base=mix(m,vertex(m),color(m,(.48,.53,.52)),edge)
output(mul(m,base,add(m,scalar(m,.92),mul(m,noise(m,.4,1),scalar(m,.08)))),m,ue.MaterialProperty.MP_BASE_COLOR);save(m)
m=material('PoolWater',(.025,.23,.32),.16,.16)
m.set_editor_property('two_sided',True)
p=expression(m,ue.MaterialExpressionWorldPosition);t=expression(m,ue.MaterialExpressionTime)
waves=[]
for axis,frequency,speed in [(0,.06,1.4),(1,.085,1.05)]:
 phase=add(m,mul(m,component(m,p,axis),scalar(m,frequency)),mul(m,t,scalar(m,speed)))
 wave=expression(m,ue.MaterialExpressionSine,period=6.2831853);link(phase,wave);waves.append(mul(m,wave,scalar(m,.09)))
n=expression(m,ue.MaterialExpressionAppendVector);link(waves[0],n,'A');link(waves[1],n,'B')
n3=expression(m,ue.MaterialExpressionAppendVector);link(n,n3,'A');link(scalar(m,1),n3,'B');output(n3,m,ue.MaterialProperty.MP_NORMAL)
# Opaque stylized depth avoids translucent sorting seams at the tile walls.
output(mix(m,color(m,(.018,.18,.25)),color(m,(.07,.39,.45)),noise(m,.025)),m,ue.MaterialProperty.MP_BASE_COLOR);save(m)
m=material('OilSlick',(.006,.008,.011),.11,.55)
rainbow=mix(m,color(m,(.009,.015,.027)),color(m,(.035,.012,.028)),noise(m,.065))
output(mix(m,color(m,(.003,.004,.006)),rainbow,scalar(m,.70)),m,ue.MaterialProperty.MP_BASE_COLOR);save(m)
ue.log('ORIGINAL POOL AND PARKING MATERIALS READY')
