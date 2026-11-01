"""Run inside Unreal Editor once, after the native project has been compiled."""
from pathlib import Path
import json
import unreal as ue

ROOT=Path(ue.Paths.project_dir()).resolve()
exec((ROOT/'tools/workshop_materials.py').read_text(),globals())
ASSET=ue.AssetToolsHelpers.get_asset_tools()
EDIT=ue.MaterialEditingLibrary


def imported(source,name,path='/Game/Textures',options=None,force=False):
    asset_path=f'{path}/{name}'
    if ue.EditorAssetLibrary.does_asset_exist(asset_path) and not force:return ue.load_asset(asset_path)
    task=ue.AssetImportTask()
    task.set_editor_property('filename',str(source))
    task.set_editor_property('destination_path',path)
    task.set_editor_property('destination_name',name)
    task.set_editor_property('automated',True)
    task.set_editor_property('save',True)
    task.set_editor_property('replace_existing',force)
    if options:task.set_editor_property('options',options)
    ASSET.import_asset_tasks([task])
    paths=task.get_editor_property('imported_object_paths')
    if not paths:raise RuntimeError(f'Import failed: {source}')
    result=ue.load_asset(paths[0])
    if result.get_path_name().split('.')[0]!=asset_path:
        if not ue.EditorAssetLibrary.rename_asset(result.get_path_name(),asset_path):
            raise RuntimeError(f'Cannot name imported asset {asset_path}')
        result=ue.load_asset(asset_path)
    return result


def expression(material,kind,**properties):
    node=EDIT.create_material_expression(material,kind)
    for key,value in properties.items():node.set_editor_property(key,value)
    return node


def link(source,dest,pin='',output=''):
    if not EDIT.connect_material_expressions(source,output,dest,pin):
        raise RuntimeError(f'Material connection failed: {source} -> {dest}.{pin}')


def output(node,material,prop,channel=''):
    if not EDIT.connect_material_property(node,channel,prop):raise RuntimeError('Material output connection failed')


def scalar(material,value):return expression(material,ue.MaterialExpressionConstant,r=value)

def mul(material,a,b):
    node=expression(material,ue.MaterialExpressionMultiply);link(a,node,'A');link(b,node,'B');return node

def uv_range(material,axis,low,high):
    lower=expression(material,ue.MaterialExpressionSubtract,const_b=low);link(axis,lower,'A')
    upper=expression(material,ue.MaterialExpressionSubtract,const_a=high);link(axis,upper,'B')
    values=[]
    for source in [lower,upper]:
        scale=expression(material,ue.MaterialExpressionMultiply,const_b=1000);link(source,scale,'A')
        clamp=expression(material,ue.MaterialExpressionSaturate);link(scale,clamp);values.append(clamp)
    return mul(material,*values)

def rider_clothes(material,sample):
    uv=expression(material,ue.MaterialExpressionTextureCoordinate)
    u=expression(material,ue.MaterialExpressionComponentMask,r=True,g=False,b=False,a=False);link(uv,u)
    v=expression(material,ue.MaterialExpressionComponentMask,r=False,g=True,b=False,a=False);link(uv,v)
    torso=mul(material,uv_range(material,u,.324,.676),uv_range(material,v,.023,.86))
    outside=expression(material,ue.MaterialExpressionOneMinus);link(uv_range(material,u,.308,.692),outside)
    sleeves=mul(material,outside,uv_range(material,v,.444,.60))
    shirt=expression(material,ue.MaterialExpressionMax);link(torso,shirt,'A');link(sleeves,shirt,'B')
    shorts=mul(material,outside,uv_range(material,v,0,.25))
    gray=expression(material,ue.MaterialExpressionDotProduct);link(sample,gray,'A','RGB');link(color(material,(.2126,.7152,.0722)),gray,'B')
    blue=mul(material,gray,color(material,(.065,.3,.82)))
    purple=mul(material,gray,color(material,(.42,.085,.52)))
    top=expression(material,ue.MaterialExpressionLinearInterpolate);link(sample,top,'A','RGB');link(blue,top,'B');link(shirt,top,'Alpha')
    outfit=expression(material,ue.MaterialExpressionLinearInterpolate);link(top,outfit,'A');link(purple,outfit,'B');link(shorts,outfit,'Alpha')
    extreme=expression(material,ue.MaterialExpressionScalarParameter,parameter_name='Extreme',default_value=0)
    blend=expression(material,ue.MaterialExpressionLinearInterpolate);link(sample,blend,'A','RGB');link(outfit,blend,'B');link(extreme,blend,'Alpha')
    output(blend,material,ue.MaterialProperty.MP_BASE_COLOR)


def color(material,value):
    return expression(material,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(*value,1))

def weather(material,source,tint,vertex=False):
    position=expression(material,ue.MaterialExpressionWorldPosition)
    scale=expression(material,ue.MaterialExpressionMultiply,const_b=.0014);link(position,scale,'A')
    noise=expression(material,ue.MaterialExpressionNoise,quality=1,levels=2,scale=1,output_min=0,output_max=1);link(scale,noise)
    amount=expression(material,ue.MaterialExpressionMultiply,const_b=.42);link(noise,amount,'A')
    shade=expression(material,ue.MaterialExpressionAdd,const_b=.67);link(amount,shade,'A')
    result=mul(material,mul(material,source,color(material,tint)),shade)
    if vertex:
        mask=expression(material,ue.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False);link(expression(material,ue.MaterialExpressionVertexColor),mask)
        result=mul(material,result,mask)
    return result

def breeze(material,amount):
    position=expression(material,ue.MaterialExpressionWorldPosition)
    phase=expression(material,ue.MaterialExpressionDotProduct);link(position,phase,'A');link(color(material,(.007,.004,.002)),phase,'B')
    time=expression(material,ue.MaterialExpressionTime)
    move=expression(material,ue.MaterialExpressionMultiply,const_b=.8);link(time,move,'A')
    add=expression(material,ue.MaterialExpressionAdd);link(phase,add,'A');link(move,add,'B')
    wave=expression(material,ue.MaterialExpressionSine,period=6.2831853);link(add,wave)
    uv=expression(material,ue.MaterialExpressionTextureCoordinate)
    tip=expression(material,ue.MaterialExpressionComponentMask,r=False,g=True,b=False,a=False);link(uv,tip)
    anchor=expression(material,ue.MaterialExpressionOneMinus);link(tip,anchor)
    offset=mul(material,mul(material,wave,anchor),color(material,(amount*3.4,amount*1.5,amount*.3)))
    output(offset,material,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)


def material(name,rgb=(.2,.2,.2),roughness=.7,metallic=0):
    path=f'/Game/Materials/M_{name}'
    result=ue.load_asset(path) if ue.EditorAssetLibrary.does_asset_exist(path) else ASSET.create_asset(f'M_{name}','/Game/Materials',ue.Material,ue.MaterialFactoryNew())
    # Snapshot the expression array: UE 5.8's DeleteAll mutates its live iterator
    # and can leave old custom outputs behind when regenerating a material.
    for node in list(EDIT.get_material_expressions(result)):EDIT.delete_material_expression(result,node)
    result.set_editor_property('used_with_instanced_static_meshes',True)
    output(color(result,rgb),result,ue.MaterialProperty.MP_BASE_COLOR)
    output(scalar(result,roughness),result,ue.MaterialProperty.MP_ROUGHNESS)
    output(scalar(result,metallic),result,ue.MaterialProperty.MP_METALLIC)
    return result


def scanned(name,asset,masked=False,world_uv=True):
    result=material(name)
    folder=ROOT/'assets/source'/asset
    uv=None
    if not masked and world_uv:
        pos=expression(result,ue.MaterialExpressionWorldPosition)
        mask=expression(result,ue.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False)
        link(pos,mask)
        uv=expression(result,ue.MaterialExpressionMultiply,const_b=.012 if name in ['Rock','Bank'] else .0035 if name in ['Asphalt','Verge'] else 1/140 if name=='Grass' else .005)
        link(mask,uv,'A')
    for file,prop,compression,srgb in [
        ('Diffuse.jpg',ue.MaterialProperty.MP_BASE_COLOR,False,True),
        ('nor_dx.jpg',ue.MaterialProperty.MP_NORMAL,True,False),
        ('Rough.jpg',ue.MaterialProperty.MP_ROUGHNESS,False,False),
        ('Alpha.png',ue.MaterialProperty.MP_OPACITY_MASK,False,False)]:
        source_file={'Diffuse.jpg':'Grass004_2K-JPG_Color.jpg','nor_dx.jpg':'Grass004_2K-JPG_NormalDX.jpg','Rough.jpg':'Grass004_2K-JPG_Roughness.jpg'}.get(file,file) if asset=='ambientcg-grass004' else file
        if not (folder/source_file).exists():continue
        texture=imported(folder/source_file,f'{asset}_{Path(file).stem}')
        texture.set_editor_property('srgb',srgb)
        if compression:texture.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        ue.EditorAssetLibrary.save_loaded_asset(texture)
        sample=expression(result,ue.MaterialExpressionTextureSample,texture=texture)
        sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if compression else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if srgb else ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        if uv:link(uv,sample)
        if name=='Grass' and prop==ue.MaterialProperty.MP_BASE_COLOR:
            output(weather(result,sample,(.42,.69,.30),True),result,prop)
        elif name=='Grass' and prop==ue.MaterialProperty.MP_ROUGHNESS:
            # The scan's low roughness looks like a wet lawn under the sun.
            dry=expression(result,ue.MaterialExpressionMultiply,const_b=.14);link(sample,dry,'A','R')
            rough=expression(result,ue.MaterialExpressionAdd,const_b=.82);link(dry,rough,'A');output(rough,result,prop)
        elif name=='Grass' and prop==ue.MaterialProperty.MP_NORMAL:
            normal=expression(result,ue.MaterialExpressionLinearInterpolate,const_alpha=.32)
            link(color(result,(0,0,1)),normal,'A');link(sample,normal,'B','RGB');output(normal,result,prop)
        elif name in ['Asphalt','Verge'] and prop==ue.MaterialProperty.MP_BASE_COLOR:
            gray=expression(result,ue.MaterialExpressionDotProduct);link(sample,gray,'A','RGB');link(color(result,(.2126,.7152,.0722)),gray,'B')
            output(weather(result,gray,(.72,.74,.71)),result,prop)
        elif name=='Wood' and prop==ue.MaterialProperty.MP_BASE_COLOR:
            output(weather(result,sample,(.69,.72,.63),True),result,prop)
        elif name=='Rock' and prop==ue.MaterialProperty.MP_BASE_COLOR:
            output(weather(result,sample,(.58,.63,.54)),result,prop)
        elif masked and prop==ue.MaterialProperty.MP_BASE_COLOR:
            random=expression(result,ue.MaterialExpressionPerInstanceRandom)
            amount=expression(result,ue.MaterialExpressionMultiply,const_b=.28);link(random,amount,'A')
            brightness=expression(result,ue.MaterialExpressionAdd,const_b=.74);link(amount,brightness,'A')
            tint=expression(result,ue.MaterialExpressionMultiply);link(sample,tint,'A','RGB');link(brightness,tint,'B')
            leaf_tint=color(result,(.72,.95,.54) if name=='LawnGrass' else (.75,.88,.64))
            output(mul(result,tint,leaf_tint),result,prop)
        else:output(sample,result,prop,'RGB' if prop in [ue.MaterialProperty.MP_NORMAL,ue.MaterialProperty.MP_BASE_COLOR] else 'R')
    if name=='Grass' or masked:output(scalar(result,.12),result,ue.MaterialProperty.MP_SPECULAR)
    if masked:
        result.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
        result.set_editor_property('two_sided',True)
        result.set_editor_property('opacity_mask_clip_value',.35)
        result.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        output(color(result,(.04,.09,.018)),result,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
        breeze(result,.7 if name=='LawnGrass' else 1.5)
    EDIT.recompile_material(result);ue.EditorAssetLibrary.save_loaded_asset(result)
    return result


def shoreline_material():
    m=material('Shoreline')
    vertex=expression(m,ue.MaterialExpressionVertexColor)
    alpha=expression(m,ue.MaterialExpressionMultiply,const_b=1);link(vertex,alpha,'A','A')
    position=expression(m,ue.MaterialExpressionWorldPosition)
    xy=expression(m,ue.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False);link(position,xy)
    def sample(asset,file,scale,normal=False):
        tex=ue.load_asset(f'/Game/Textures/{asset}_{file}')
        if not tex:raise RuntimeError(f'Missing shoreline texture {asset}_{file}')
        s=expression(m,ue.MaterialExpressionTextureSample,texture=tex)
        s.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if file=='Rough' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR)
        link(mul(m,xy,scalar(m,scale)),s)
        return s
    grass=weather(m,sample('ambientcg-grass004','Diffuse',1/140),(.42,.69,.30),True)
    soil=mul(m,sample('brown_mud_02','Diffuse',.012),color(m,(.52,.48,.40)))
    output(mix(m,grass,soil,alpha),m,ue.MaterialProperty.MP_BASE_COLOR)
    dry=add(m,scalar(m,.82),mul(m,sample('ambientcg-grass004','Rough',1/140),scalar(m,.14)))
    wet=add(m,scalar(m,.30),mul(m,sample('brown_mud_02','Rough',.012),scalar(m,.28)))
    output(mix(m,dry,wet,alpha),m,ue.MaterialProperty.MP_ROUGHNESS)
    gn=mix(m,color(m,(0,0,1)),sample('ambientcg-grass004','nor_dx',1/140,True),scalar(m,.32))
    sn=mix(m,color(m,(0,0,1)),sample('brown_mud_02','nor_dx',.012,True),scalar(m,.55))
    normal=expression(m,ue.MaterialExpressionNormalize);link(mix(m,gn,sn,alpha),normal)
    output(normal,m,ue.MaterialProperty.MP_NORMAL);output(mix(m,scalar(m,.12),scalar(m,.25),alpha),m,ue.MaterialProperty.MP_SPECULAR)
    EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)

def firework_material():
    m=material('FireworkStreak',(0,0,0),1)
    m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('blend_mode',ue.BlendMode.BLEND_ADDITIVE)
    output(scalar(m,1),m,ue.MaterialProperty.MP_OPACITY)
    channels=[expression(m,ue.MaterialExpressionPerInstanceCustomData,data_index=i) for i in range(4)]
    rg=expression(m,ue.MaterialExpressionAppendVector);link(channels[0],rg,'A');link(channels[1],rg,'B')
    rgb=expression(m,ue.MaterialExpressionAppendVector);link(rg,rgb,'A');link(channels[2],rgb,'B')
    varying=expression(m,ue.MaterialExpressionVertexInterpolator);link(mul(m,rgb,channels[3]),varying)
    output(varying,m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
    EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)

def model(asset,name):
    options=ue.FbxImportUI()
    options.set_editor_property('import_mesh',True)
    options.set_editor_property('import_as_skeletal',False)
    options.set_editor_property('import_materials',False)
    options.set_editor_property('import_textures',False)
    options.set_editor_property('automated_import_should_detect_type',False)
    options.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_STATIC_MESH)
    options.static_mesh_import_data.set_editor_property('combine_meshes',False)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
    # Poly Haven FBX files contain several plant variants laid out side by side.
    # Import each plant independently rather than compressing the whole collection.
    source_path=f'/Game/ModelSources/{asset}'
    paths=ue.EditorAssetLibrary.list_assets(source_path,recursive=False) if ue.EditorAssetLibrary.does_directory_exist(source_path) else []
    if not paths:
        task=ue.AssetImportTask()
        task.set_editor_property('filename',str(ROOT/'assets/source'/asset/f'{asset}.fbx'))
        task.set_editor_property('destination_path',source_path)
        task.set_editor_property('automated',True);task.set_editor_property('save',True)
        task.set_editor_property('options',options)
        ASSET.import_asset_tasks([task]);paths=task.get_editor_property('imported_object_paths')
    meshes=[ue.load_asset(path) for path in paths if isinstance(ue.load_asset(path),ue.StaticMesh)]
    if not meshes:raise RuntimeError(f'No plant meshes imported for {asset}')
    selected=next((mesh for mesh in meshes if '_c_' in mesh.get_name()),meshes[0])
    destination=f'/Game/Models/{name}'
    if ue.EditorAssetLibrary.does_asset_exist(destination):ue.EditorAssetLibrary.delete_asset(destination)
    result=ASSET.duplicate_asset(name,'/Game/Models',selected)
    if not result:raise RuntimeError(f'Cannot prepare plant {name}')
    if name=='LawnGrass':
        if not ue.CruiserAssetTools.configure_grass_detail(result):raise RuntimeError('Grass LOD generation failed')
    ue.EditorAssetLibrary.save_loaded_asset(result)
    if name in ['Shrub','DenseShrub']:
        for i,mesh in enumerate(sorted(meshes,key=lambda item:item.get_name())):
            variant=f'/Game/Models/{name}_{i}'
            if not ue.EditorAssetLibrary.does_asset_exist(variant):
                saved=ASSET.duplicate_asset(f'{name}_{i}','/Game/Models',mesh)
                ue.EditorAssetLibrary.save_loaded_asset(saved)
    return result


def rider(girl=False):
    person="Female_Adult_01" if girl else "Male_Adult_01"
    prefix="Girl" if girl else "Rider"
    name="YoungGirlV2" if girl else "YoungRiderV2"
    texprefix="f001" if girl else "m002"
    folder=ROOT/"assets/source/rocketbox"/person
    mats={}
    for region in ['body','head','opacity']:
        result=material(f'{prefix}_{region}',roughness=.75)
        result.set_editor_property('used_with_skeletal_mesh',True)
        for kind,prop in [('color',ue.MaterialProperty.MP_BASE_COLOR),('normal',ue.MaterialProperty.MP_NORMAL)]:
            source=folder/f'{texprefix}_{region}_{kind}.tga'
            if not source.exists():continue
            tex=imported(source,f'{prefix}_{region}_{kind}','/Game/Characters/Textures')
            normal=kind=='normal';tex.set_editor_property('srgb',not normal)
            if normal:tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
            ue.EditorAssetLibrary.save_loaded_asset(tex)
            node=expression(result,ue.MaterialExpressionTextureSample,texture=tex,
                            sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else ue.MaterialSamplerType.SAMPLERTYPE_COLOR)
            if region in ['body','head'] and kind=='color':rider_finish(result,node,region,girl)
            else:output(node,result,prop,'RGB')
            if region=='opacity' and kind=='color':
                result.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
                result.set_editor_property('two_sided',True)
                output(node,result,ue.MaterialProperty.MP_OPACITY_MASK,'A')
        EDIT.recompile_material(result);ue.EditorAssetLibrary.save_loaded_asset(result);mats[region]=result
    path=f'/Game/Characters/{name}'
    if not ue.EditorAssetLibrary.does_asset_exist(path):
        options=ue.FbxImportUI()
        options.set_editor_property('import_as_skeletal',True)
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_animations',False)
        options.set_editor_property('import_materials',False)
        options.set_editor_property('import_textures',False)
        options.set_editor_property('automated_import_should_detect_type',False)
        options.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property('create_physics_asset',False)
        mesh=imported(ROOT/'assets/source/workshop'/f'{name}.fbx',name,'/Game/Characters',options)
    else:mesh=ue.load_asset(path)
    if not mesh.get_editor_property('skeleton'):
        if not ue.CruiserAssetTools.ensure_rider_skeleton(mesh):raise RuntimeError('The rider skeleton could not be created')
        skeleton=mesh.get_editor_property('skeleton')
        ue.EditorAssetLibrary.save_loaded_asset(skeleton)
    slots=mesh.get_editor_property('materials')
    for slot in slots:
        name=str(slot.get_editor_property('imported_material_slot_name')).lower()
        region='head' if 'head' in name else 'opacity' if 'opacity' in name else 'body'
        slot.set_editor_property('material_interface',mats[region])
        ue.log(f'Rider material {name} -> {region}')
    mesh.set_editor_property('materials',slots);ue.EditorAssetLibrary.save_loaded_asset(mesh)
    # Export a compact rig inventory to make the runtime riding pose reviewable.
    temp=ue.PoseableMeshComponent()
    temp.set_skinned_asset_and_update(mesh)
    bones=[]
    for index in range(temp.get_num_bones()):
        name=temp.get_bone_name(index)
        bones.append(dict(name=str(name)))
    (ROOT/'reference/extracted/rider-rig.json').write_text(json.dumps(bones,indent=2)+'\n')
    ue.log('RIGGED YOUNG RIDER READY')


def water():
    result=material('Water',(.008,.027,.032),.34,0)
    result.set_editor_property('blend_mode',ue.BlendMode.BLEND_OPAQUE)
    result.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    result.set_editor_property('two_sided',True)
    output(scalar(result,.12),result,ue.MaterialProperty.MP_OPACITY)
    output(scalar(result,.25),result,ue.MaterialProperty.MP_SPECULAR)
    volume=expression(result,ue.MaterialExpressionSingleLayerWaterMaterialOutput)
    inputs=EDIT.get_material_expression_input_names(volume)
    for node,pin in zip([color(result,(.003,.008,.01)),color(result,(.04,.015,.009)),scalar(result,.3),scalar(result,.85)],inputs):link(node,volume,pin)
    position=expression(result,ue.MaterialExpressionWorldPosition)
    time=expression(result,ue.MaterialExpressionTime)
    # Several oblique wave trains avoid the contour bands from warped sine noise.
    axes=[(.031,.017),(-.012,.049),(.073,-.028),(.009,.022),(-.042,-.057),(.061,.083)]
    waves=[]
    for channel in [0,1]:
        total=scalar(result,0)
        for index,(dx,dy) in enumerate(axes):
            phase=expression(result,ue.MaterialExpressionDotProduct);link(position,phase,'A');link(color(result,(dx,dy,0)),phase,'B')
            motion=expression(result,ue.MaterialExpressionMultiply,const_b=(.7+index*.19)*(-1 if index%2 else 1));link(time,motion,'A')
            moving=expression(result,ue.MaterialExpressionAdd);link(phase,moving,'A');link(motion,moving,'B')
            sine=expression(result,ue.MaterialExpressionSine,period=6.2831853);link(moving,sine)
            amplitude=expression(result,ue.MaterialExpressionMultiply,const_b=(dx if channel==0 else dy)*1.8);link(sine,amplitude,'A')
            added=expression(result,ue.MaterialExpressionAdd);link(total,added,'A');link(amplitude,added,'B');total=added
        waves.append(total)
    xy=expression(result,ue.MaterialExpressionAppendVector);link(waves[0],xy,'A');link(waves[1],xy,'B')
    normal=expression(result,ue.MaterialExpressionAppendVector);link(xy,normal,'A');link(scalar(result,1),normal,'B')
    output(normal,result,ue.MaterialProperty.MP_NORMAL)
    assert sum(isinstance(node,ue.MaterialExpressionSingleLayerWaterMaterialOutput) for node in EDIT.get_material_expressions(result))==1
    EDIT.recompile_material(result);ue.EditorAssetLibrary.save_loaded_asset(result)


def prepare():
    ue.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
    sky=material('Sky')
    sky.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
    sky.set_editor_property('two_sided',True);sky.set_editor_property('is_sky',True)
    p=expression(sky,ue.MaterialExpressionWorldPosition)
    z=expression(sky,ue.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False);link(p,z)
    height=expression(sky,ue.MaterialExpressionMultiply,const_b=.000006);link(z,height,'A')
    clamp=expression(sky,ue.MaterialExpressionSaturate);link(height,clamp)
    gradient=expression(sky,ue.MaterialExpressionLinearInterpolate);link(color(sky,(.56,.69,.78)),gradient,'A');link(color(sky,(.15,.34,.66)),gradient,'B');link(clamp,gradient,'Alpha')
    output(mul(sky,gradient,scalar(sky,4800)),sky,ue.MaterialProperty.MP_EMISSIVE_COLOR)
    EDIT.recompile_material(sky);ue.EditorAssetLibrary.save_loaded_asset(sky)
    cloud=material('CloudShade')
    cloud.set_editor_property('material_domain',ue.MaterialDomain.MD_LIGHT_FUNCTION)
    pos=expression(cloud,ue.MaterialExpressionWorldPosition)
    clock=expression(cloud,ue.MaterialExpressionTime)
    drift=mul(cloud,clock,color(cloud,(19,7,0)))
    moving=expression(cloud,ue.MaterialExpressionAdd);link(pos,moving,'A');link(drift,moving,'B')
    scaled=mul(cloud,moving,scalar(cloud,.00022))
    noise=expression(cloud,ue.MaterialExpressionNoise,quality=1,levels=3,scale=1,output_min=0,output_max=1);link(scaled,noise)
    amount=mul(cloud,noise,scalar(cloud,.22));shade=expression(cloud,ue.MaterialExpressionAdd,const_b=.78);link(amount,shade,'A');output(shade,cloud,ue.MaterialProperty.MP_EMISSIVE_COLOR)
    EDIT.recompile_material(cloud);ue.EditorAssetLibrary.save_loaded_asset(cloud)
    scanned('Grass','ambientcg-grass004');scanned('Asphalt','asphalt_01')
    bank=scanned('Bank','brown_mud_02');bank.set_editor_property('two_sided',True);ue.EditorAssetLibrary.save_loaded_asset(bank)
    shoreline_material()
    verge=scanned('Verge','asphalt_01')
    verge.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
    verge.set_editor_property('translucency_lighting_mode',ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    output(expression(verge,ue.MaterialExpressionVertexColor),verge,ue.MaterialProperty.MP_OPACITY,'A')
    EDIT.recompile_material(verge);ue.EditorAssetLibrary.save_loaded_asset(verge)
    scanned('Foliage','shrub_01',True);scanned('DenseFoliage','shrub_04',True);scanned('GrassBlade','grass_bermuda_01',True)
    scanned('LawnGrass','grass_medium_01',True)
    leaf=material('Leaf',roughness=.78)
    leaf.set_editor_property('two_sided',True);leaf.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    vertex=expression(leaf,ue.MaterialExpressionVertexColor)
    mask=expression(leaf,ue.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False);link(vertex,mask)
    output(mask,leaf,ue.MaterialProperty.MP_BASE_COLOR)
    output(color(leaf,(.025,.07,.012)),leaf,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
    output(scalar(leaf,.17),leaf,ue.MaterialProperty.MP_SPECULAR);breeze(leaf,1.3)
    EDIT.recompile_material(leaf);ue.EditorAssetLibrary.save_loaded_asset(leaf)
    gravel=material('Gravel',roughness=.93)
    output(expression(gravel,ue.MaterialExpressionVertexColor),gravel,ue.MaterialProperty.MP_BASE_COLOR)
    output(scalar(gravel,.12),gravel,ue.MaterialProperty.MP_SPECULAR)
    EDIT.recompile_material(gravel);ue.EditorAssetLibrary.save_loaded_asset(gravel)
    for name,rgb,rough,metal in [
        ('Metal',(.56,.61,.64),.23,.9),
        ('Rubber',(.014,.017,.018),.85,0),('Blue',(.025,.17,.48),.48,0),
        ('Purple',(.24,.045,.31),.7,0),('Skin',(.48,.285,.175),.7,0),
        ('Dark',(.024,.031,.039),.75,0),('White',(.86,.86,.81),.65,0),
        ('FlowerWhite',(.88,.86,.71),.8,0),('FlowerYellow',(.95,.57,.035),.8,0),
        ('FlowerPurple',(.36,.12,.48),.8,0),('FlowerRed',(.62,.045,.035),.8,0)]:
        result=material(name,rgb,rough,metal);EDIT.recompile_material(result);ue.EditorAssetLibrary.save_loaded_asset(result)
    scanned('Wood','wood_planks_dirt',world_uv=False)
    scanned('Rock','rock_boulder_dry')
    paint=material('Paint',roughness=.24,metallic=.55)
    output(expression(paint,ue.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=ue.LinearColor(.02,.14,.6,1)),paint,ue.MaterialProperty.MP_BASE_COLOR)
    EDIT.recompile_material(paint);ue.EditorAssetLibrary.save_loaded_asset(paint)
    for title,rgb,glow in [('Ripple',(.15,.24,.23),.08),('Firework',(1,.4,.06),12000),('Dust',(.17,.12,.065),0)]:
        m=material(title,rgb,roughness=.8)
        if title in ['Firework','Ripple']:
            m.set_editor_property('two_sided',True)
            vertex=expression(m,ue.MaterialExpressionVertexColor)
            if title=='Firework':
                m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
                rnd=expression(m,ue.MaterialExpressionPerInstanceRandom)
                bright=expression(m,ue.MaterialExpressionLinearInterpolate);link(color(m,(.08,.6,1)),bright,'A');link(color(m,(1,.05,.18)),bright,'B');link(rnd,bright,'Alpha')
                output(mul(m,bright,scalar(m,glow)),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            else:
                m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
                m.set_editor_property('translucency_lighting_mode',ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
                alpha=expression(m,ue.MaterialExpressionMultiply,const_b=.38);link(vertex,alpha,'A','A')
                output(alpha,m,ue.MaterialProperty.MP_OPACITY)
        EDIT.recompile_material(m);ue.EditorAssetLibrary.save_loaded_asset(m)
    firework_material()
    for file,name in [('bmx-cruisers-splash.png','Splash'),('bmx-cruisers-logo.png','Logo')]:
        tex=imported(ROOT/'assets/source/branding'/file,name,'/Game/UI')
        tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property('lod_group',ue.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property('never_stream',True);ue.EditorAssetLibrary.save_loaded_asset(tex)
    workshop_assets();trail_material();water();model('shrub_01','Shrub');model('shrub_04','DenseShrub');model('grass_bermuda_01','ShortGrass');model('grass_medium_01','LawnGrass');rider();rider(True)
    ue.EditorAssetLibrary.make_directory('/Game/Maps')
    ue.EditorLoadingAndSavingUtils.new_blank_map(False)
    ue.EditorLoadingAndSavingUtils.save_map(ue.EditorLevelLibrary.get_editor_world(),'/Game/Maps/CourseOne')
    ue.EditorAssetLibrary.save_directory('/Game',only_if_is_dirty=False,recursive=True)
    (ROOT/'Content/Data/assets-ready.json').write_text(json.dumps(dict(ready=True,engine=ue.SystemLibrary.get_engine_version()))+'\n')
    ue.log('BMX CRUISER ASSETS READY')


if __name__=='__main__':prepare()
