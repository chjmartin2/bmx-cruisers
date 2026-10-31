from pathlib import Path
import unreal as ue,json
ROOT=Path(ue.Paths.project_dir()).resolve();folder=ROOT/'assets/source/hero';folder.mkdir(exist_ok=True)
scope={'__name__':'hero_preparation'};exec((ROOT/'tools/prepare_unreal_assets.py').read_text(),scope)
mesh=ue.load_asset('/MetaHumanCharacter/Face/SKM_Face');assert mesh
result=ue.CruiserAssetTools.bake_studio_head(mesh,'/Game/Characters/HeroHeadV1');assert result
textures={}
for key,name in [('color','Head_D'),('normal','Head_N'),('eye','Eye_D')]:
 source='/MetaHuman/IdentityTemplate/DefaultArchetypeTexture/T_MetaHumanIdentity_'+name
 tex=ue.load_asset(source);ue.log('HERO TEXTURE '+source+' '+str(tex));assert tex
 out=folder/(key+'.png');task=ue.AssetExportTask();task.object=tex;task.filename=str(out);task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=ue.TextureExporterPNG();assert ue.Exporter.run_asset_export_task(task)
 textures[key]=scope['imported'](out,'Hero_'+key,'/Game/Characters/HeroTextures')
 tex=textures[key];tex.set_editor_property('srgb',key!='normal')
 if key=='normal':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
 ue.EditorAssetLibrary.save_loaded_asset(tex)
expression=scope['expression'];link=scope['link'];output=scope['output'];mul=scope['mul'];color=scope['color'];scalar=scope['scalar'];material=scope['material'];mix=scope['mix']
skins=[]
for gender,file in [('Male','male-skin-albedo.png'),('Female','female-skin-albedo.png')]:
 texture=scope['imported'](folder/file,'HeroSkin'+gender,'/Game/Characters/HeroTextures');texture.set_editor_property('srgb',True);texture.set_editor_property('never_stream',True);ue.EditorAssetLibrary.save_loaded_asset(texture)
 head=material('HeroSkin'+gender,roughness=.43);head.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_SUBSURFACE)
 sample=expression(head,ue.MaterialExpressionTextureSample,texture=texture);gray=expression(head,ue.MaterialExpressionDotProduct);link(sample,gray,'A','RGB');link(color(head,(.2126,.7152,.0722)),gray,'B')
 tint=expression(head,ue.MaterialExpressionVectorParameter,parameter_name='SkinTint',default_value=ue.LinearColor(.43,.235,.135,1))
 base=mul(head,mul(head,gray,scalar(head,2.6)),tint);output(base,head,ue.MaterialProperty.MP_BASE_COLOR);output(mul(head,tint,scalar(head,.4)),head,ue.MaterialProperty.MP_SUBSURFACE_COLOR);output(scalar(head,.38),head,ue.MaterialProperty.MP_OPACITY);output(scalar(head,.35),head,ue.MaterialProperty.MP_SPECULAR)
 n=expression(head,ue.MaterialExpressionTextureSample,texture=textures['normal'],sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL);coords=expression(head,ue.MaterialExpressionTextureCoordinate,u_tiling=12,v_tiling=12);link(coords,n);output(n,head,ue.MaterialProperty.MP_NORMAL,'RGB');ue.MaterialEditingLibrary.recompile_material(head);ue.EditorAssetLibrary.save_loaded_asset(head);skins.append(head)
head=skins[0]
eye=material('HeroEye',roughness=.13)
uv=expression(eye,ue.MaterialExpressionTextureCoordinate);center=expression(eye,ue.MaterialExpressionConstant2Vector,r=.5,g=.5);delta=scope['subtract'](eye,uv,center)
radius=expression(eye,ue.MaterialExpressionDistance);link(uv,radius,'A');link(center,radius,'B')
def disc(edge,soft):
 amount=scope['clamp01'](eye,mul(eye,scope['subtract'](eye,radius,scalar(eye,edge)),scalar(eye,soft)));n=expression(eye,ue.MaterialExpressionOneMinus);link(amount,n);return n
x=scope['component'](eye,delta,0);y=scope['component'](eye,delta,1);angle=expression(eye,ue.MaterialExpressionArctangent2);link(x,angle,'X');link(y,angle,'Y');rays=expression(eye,ue.MaterialExpressionSine,period=6.2831853);link(mul(eye,angle,scalar(eye,93)),rays)
shade=scope['add'](eye,scalar(eye,.5),mul(eye,rays,scalar(eye,.5)))
iris=mix(eye,color(eye,(.025,.012,.006)),color(eye,(.16,.075,.025)),shade)
base=mix(eye,color(eye,(.72,.75,.7)),iris,disc(.133,130));base=mix(eye,base,color(eye,(.001,.001,.001)),disc(.06,260));output(base,eye,ue.MaterialProperty.MP_BASE_COLOR);output(scalar(eye,.65),eye,ue.MaterialProperty.MP_SPECULAR);ue.MaterialEditingLibrary.recompile_material(eye);ue.EditorAssetLibrary.save_loaded_asset(eye)
neck=material('NeckSkin',roughness=.46);neck.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_SUBSURFACE);tint=expression(neck,ue.MaterialExpressionVectorParameter,parameter_name='SkinTint',default_value=ue.LinearColor(.43,.235,.135,1));output(mul(neck,tint,scalar(neck,.64)),neck,ue.MaterialProperty.MP_BASE_COLOR);output(mul(neck,tint,scalar(neck,.35)),neck,ue.MaterialProperty.MP_SUBSURFACE_COLOR);output(scalar(neck,.4),neck,ue.MaterialProperty.MP_OPACITY);ue.MaterialEditingLibrary.recompile_material(neck);ue.EditorAssetLibrary.save_loaded_asset(neck)
hidden=material('HeroHidden');hidden.set_editor_property('used_with_skeletal_mesh',True);hidden.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);output(scalar(hidden,0),hidden,ue.MaterialProperty.MP_OPACITY_MASK);ue.MaterialEditingLibrary.recompile_material(hidden);ue.EditorAssetLibrary.save_loaded_asset(hidden)
slots=result.get_editor_property('static_materials')
for slot in slots:
 name=str(slot.material_slot_name).lower();mat=head if name.startswith('head') else eye if 'eyeleft' in name or 'eyeright' in name else ue.load_asset('/Game/Materials/M_White') if 'teeth' in name else hidden
 slot.set_editor_property('material_interface',mat)
result.set_editor_property('static_materials',slots);ue.EditorAssetLibrary.save_loaded_asset(result)
(folder/'provenance.json').write_text(json.dumps(dict(source_mesh='/MetaHumanCharacter/Face/SKM_Face',source='Installed Unreal Engine 5.8 MetaHuman character/identity template resources',runtime='Baked neutral head geometry plus exported textures; no creator services or facial-animation plugin dependency',materials=[str(s.material_slot_name) for s in slots]),indent=2)+'\n')
ue.log('HERO HEAD READY')
