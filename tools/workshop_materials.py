"""Material graphs and static imports for the rider/equipment art pass."""

def add(m,a,b):
 n=expression(m,ue.MaterialExpressionAdd);link(a,n,'A');link(b,n,'B');return n

def mix(m,a,b,alpha):
 n=expression(m,ue.MaterialExpressionLinearInterpolate);link(a,n,'A');link(b,n,'B');link(alpha,n,'Alpha');return n

def component(m,source,channel):
 n=expression(m,ue.MaterialExpressionComponentMask,r=channel==0,g=channel==1,b=channel==2,a=channel==3);link(source,n);return n

def subtract(m,a,b):
 n=expression(m,ue.MaterialExpressionSubtract);link(a,n,'A');link(b,n,'B');return n

def clamp01(m,n):
 c=expression(m,ue.MaterialExpressionSaturate);link(n,c);return c

def rider_finish(m,sample,region,girl):
 uv=expression(m,ue.MaterialExpressionTextureCoordinate);u=component(m,uv,0);v=component(m,uv,1)
 rgb=[]
 for k in 'RGB':
  n=expression(m,ue.MaterialExpressionComponentMask,r=k=='R',g=k=='G',b=k=='B',a=False);link(sample,n);rgb.append(n)
 score=add(m,mul(m,subtract(m,rgb[0],rgb[1]),scalar(m,6)),mul(m,subtract(m,rgb[1],rgb[2]),scalar(m,5)))
 skin=clamp01(m,score)
 if region=='body':
  outside=expression(m,ue.MaterialExpressionOneMinus);link(uv_range(m,u,.30,.70),outside)
  if girl:
   span=add(m,uv_range(m,v,0,.24),uv_range(m,v,.26,.50))
   semantic=mul(m,outside,span)
  else:semantic=mul(m,outside,add(m,uv_range(m,v,.25,.444),uv_range(m,v,.60,1.01)))
  skin=mul(m,skin,clamp01(m,semantic))
 tint=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='SkinTint',default_value=ue.LinearColor(.43,.235,.135,1))
 gray=expression(m,ue.MaterialExpressionDotProduct);link(sample,gray,'A','RGB');link(color(m,(.2126,.7152,.0722)),gray,'B')
 flesh=mul(m,mul(m,gray,scalar(m,2.6)),tint)
 if region=='body':
  extreme=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='Extreme',default_value=0)
  # Replace the stock polo/pink blouse colors with coordinated sports fabrics.
  fold=add(m,mul(m,gray,scalar(m,.22)),scalar(m,.55))
  jersey=mix(m,color(m,(.08,.17,.23)),color(m,(.65,.67,.61)),extreme)
  diagonal=add(m,u,mul(m,v,scalar(m,.55)))
  slash=uv_range(m,diagonal,.84,.90)
  slash2=uv_range(m,diagonal,.92,.945)
  jersey=mix(m,jersey,color(m,(.58,.026,.012)),mul(m,slash,extreme))
  jersey=mix(m,jersey,color(m,(.74,.77,.71)),mul(m,slash2,extreme))
  outside=expression(m,ue.MaterialExpressionOneMinus);link(uv_range(m,u,.30,.70),outside)
  trousers=mul(m,outside,uv_range(m,v,.55,1.01) if girl else uv_range(m,v,0,.25))
  jersey=mix(m,jersey,color(m,(.025,.04,.052)),trousers)
  cloth=mul(m,jersey,fold)
  output(mix(m,cloth,flesh,skin),m,ue.MaterialProperty.MP_BASE_COLOR)
  # Only the original shoes disappear when a real skate boot covers the foot.
  m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
  pre=expression(m,ue.MaterialExpressionPreSkinnedPosition)
  varying=expression(m,ue.MaterialExpressionVertexInterpolator);link(pre,varying)
  height=component(m,varying,2)
  feet=clamp01(m,mul(m,subtract(m,scalar(m,17.0),height),scalar(m,2)))
  hide=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='HideShoes',default_value=0)
  opacity=expression(m,ue.MaterialExpressionOneMinus);link(mul(m,feet,hide),opacity);output(opacity,m,ue.MaterialProperty.MP_OPACITY_MASK)
 else:output(mix(m,sample,flesh,mul(m,skin,scalar(m,.92))),m,ue.MaterialProperty.MP_BASE_COLOR)
 m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_SUBSURFACE)
 output(mul(m,mul(m,tint,skin),scalar(m,.48)),m,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
 output(scalar(m,.42),m,ue.MaterialProperty.MP_OPACITY)
 output(mix(m,scalar(m,.79),scalar(m,.44),skin),m,ue.MaterialProperty.MP_ROUGHNESS)
 output(scalar(m,.34),m,ue.MaterialProperty.MP_SPECULAR)

def workshop_assets(refresh=()):
 for name,rgb,rough,metal in [('Alloy',(.42,.46,.5),.27,.93),('Shell',(.028,.035,.043),.37,.04),('Leather',(.30,.20,.12),.55,0),('Liner',(.035,.042,.05),.91,0),('Lace',(.58,.61,.55),.84,0),('Stitch',(.68,.60,.42),.8,0),('Urethane',(.64,.59,.41),.33,0),('Grip',(.018,.021,.023),.94,0),('Rubber',(.018,.022,.028),.72,0)]:
  m=material('Gear_'+name,rgb,rough,metal)
  if name=='Liner':m.set_editor_property('two_sided',True)
  uv=expression(m,ue.MaterialExpressionTextureCoordinate)
  # Fine grain is measured in object centimetres, so it stays small on a boot.
  p=expression(m,ue.MaterialExpressionWorldPosition)
  grain=expression(m,ue.MaterialExpressionNoise,quality=1,levels=1,scale=5,output_min=0,output_max=1);link(p,grain)
  variation=add(m,scalar(m,.86),mul(m,grain,scalar(m,.14)))
  output(mul(m,color(m,rgb),variation),m,ue.MaterialProperty.MP_BASE_COLOR)
  output(add(m,scalar(m,max(0,rough-.06)),mul(m,grain,scalar(m,.12))),m,ue.MaterialProperty.MP_ROUGHNESS)
  EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)
 folder=ROOT/'assets/source/workshop'
 for entry in json.loads((folder/'inventory.json').read_text()):
  name=entry['name'];options=ue.FbxImportUI();options.set_editor_property('import_as_skeletal',False);options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False);options.set_editor_property('automated_import_should_detect_type',False);options.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_STATIC_MESH)
  options.static_mesh_import_data.set_editor_property('combine_meshes',True)
  options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
  options.static_mesh_import_data.set_editor_property('normal_import_method',ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
  mesh=imported(folder/(name+'.obj'),name,'/Game/Equipment',options,force=name in refresh)
  ue.log('WORKSHOP MODEL '+name+' '+str(mesh.get_bounds()))
  ue.EditorAssetLibrary.save_loaded_asset(mesh)

def trail_material():
 m=material('Asphalt',roughness=.88)
 p=expression(m,ue.MaterialExpressionWorldPosition);xy=expression(m,ue.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(p,xy)
 a=mul(m,xy,scalar(m,.0031))
 x=component(m,xy,0);y=component(m,xy,1)
 rx=add(m,mul(m,x,scalar(m,.7986)),mul(m,y,scalar(m,-.6018)))
 ry=add(m,mul(m,x,scalar(m,.6018)),mul(m,y,scalar(m,.7986)))
 rot=expression(m,ue.MaterialExpressionAppendVector);link(rx,rot,'A');link(ry,rot,'B')
 b=add(m,mul(m,rot,scalar(m,.00183)),expression(m,ue.MaterialExpressionConstant2Vector,r=.173,g=.419))
 macro=expression(m,ue.MaterialExpressionNoise,quality=1,levels=3,scale=.0016,output_min=0,output_max=1);link(p,macro)
 macro2=expression(m,ue.MaterialExpressionNoise,quality=1,levels=2,scale=.009,output_min=0,output_max=1);link(p,macro2)
 for texname,prop,normal in [('Diffuse',ue.MaterialProperty.MP_BASE_COLOR,False),('nor_dx',ue.MaterialProperty.MP_NORMAL,True),('Rough',ue.MaterialProperty.MP_ROUGHNESS,False)]:
  tex=ue.load_asset('/Game/Textures/asphalt_01_'+texname)
  nodes=[]
  for coords in [a,b]:
   n=expression(m,ue.MaterialExpressionTextureSample,texture=tex,sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if texname=='Diffuse' else ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);link(coords,n);nodes.append(n)
  blended=mix(m,nodes[0],nodes[1],macro)
  if prop==ue.MaterialProperty.MP_BASE_COLOR:
   hue=mix(m,color(m,(.58,.60,.58)),color(m,(.83,.77,.62)),macro2)
   output(mul(m,mul(m,blended,hue),add(m,scalar(m,.74),mul(m,macro,scalar(m,.4)))),m,prop)
  elif normal:output(blended,m,prop)
  else:output(add(m,scalar(m,.73),mul(m,blended,scalar(m,.24))),m,prop)
 output(scalar(m,.16),m,ue.MaterialProperty.MP_SPECULAR)
 EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)
 wear=material('TrailWear',roughness=.91)
 vertex=expression(wear,ue.MaterialExpressionVertexColor);output(vertex,wear,ue.MaterialProperty.MP_BASE_COLOR)
 wear.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT);wear.set_editor_property('translucency_lighting_mode',ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
 output(vertex,wear,ue.MaterialProperty.MP_OPACITY,'A');EDIT.recompile_material(wear);ue.EditorAssetLibrary.save_loaded_asset(wear)
