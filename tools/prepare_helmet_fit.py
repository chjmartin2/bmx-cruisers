"""Targeted editor import; preserves existing body/skin art and course assets."""
from pathlib import Path
import unreal as ue
ROOT=Path(ue.Paths.project_dir()).resolve()
scope={'__name__':'helmet_preparation'}
exec((ROOT/'tools/prepare_unreal_assets.py').read_text(encoding='utf-8'),scope)
scope['workshop_assets'](refresh=('HelmetExtremeV4',))
hidden=ue.load_asset('/Game/Materials/M_HeroHidden');assert hidden
hidden.set_editor_property('used_with_skeletal_mesh',True)
ue.MaterialEditingLibrary.recompile_material(hidden);ue.EditorAssetLibrary.save_loaded_asset(hidden)
material=scope['material'];output=scope['output'];scalar=scope['scalar'];expression=scope['expression'];link=scope['link']
paint=material('HelmetComposite',roughness=.34,metallic=.04)
output(expression(paint,ue.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=ue.LinearColor(.02,.14,.6,1)),paint,ue.MaterialProperty.MP_BASE_COLOR)
output(scalar(paint,.42),paint,ue.MaterialProperty.MP_SPECULAR)
ue.MaterialEditingLibrary.recompile_material(paint);ue.EditorAssetLibrary.save_loaded_asset(paint)
accent=material('Gear_Accent',(.74,.78,.80),.43,.08)
ue.MaterialEditingLibrary.recompile_material(accent);ue.EditorAssetLibrary.save_loaded_asset(accent)
source=ue.load_asset('/MetaHumanCharacter/Face/SKM_Face');assert source
head=ue.CruiserAssetTools.bake_studio_head(source,'/Game/Characters/HeroHeadV3');assert head
old=ue.load_asset('/Game/Characters/HeroHeadV1');assert old
head.set_editor_property('static_materials',old.get_editor_property('static_materials'))
ue.EditorAssetLibrary.save_loaded_asset(head)
ue.log('HELMET AND NECK FIT ASSETS READY')
