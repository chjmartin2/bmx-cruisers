"""Recreate the excluded Epic template derivatives from a licensed UE installation."""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
folder = root / 'assets/source/hero'
folder.mkdir(parents=True, exist_ok=True)
scope = {'__name__': 'engine_asset_restore'}
exec((root / 'tools/prepare_unreal_assets.py').read_text(encoding='utf-8'), scope)
source = ue.load_asset('/MetaHumanCharacter/Face/SKM_Face')
assert source, 'Enable the installed MetaHumanCharacter plugin.'
head = ue.CruiserAssetTools.bake_studio_head(source, '/Game/Characters/HeroHeadV3')
assert head
slots = head.get_editor_property('static_materials')
for slot in slots:
    name = str(slot.material_slot_name).lower()
    material = 'HeroSkinMale' if name.startswith('head') else 'HeroEye' if 'eyeleft' in name or 'eyeright' in name else 'White' if 'teeth' in name else 'HeroHidden'
    slot.set_editor_property('material_interface', ue.load_asset('/Game/Materials/M_' + material))
head.set_editor_property('static_materials', slots)
ue.EditorAssetLibrary.save_loaded_asset(head)
normal = ue.load_asset('/MetaHuman/IdentityTemplate/DefaultArchetypeTexture/T_MetaHumanIdentity_Head_N')
assert normal, 'Enable the installed MetaHuman plugin.'
task = ue.AssetExportTask()
task.object = normal
task.filename = str(folder / 'normal.png')
task.automated = True
task.prompt = False
task.replace_identical = True
task.exporter = ue.TextureExporterPNG()
assert ue.Exporter.run_asset_export_task(task)
texture = scope['imported'](folder / 'normal.png', 'Hero_normal', '/Game/Characters/HeroTextures', force=True)
texture.set_editor_property('srgb', False)
texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_NORMALMAP)
ue.EditorAssetLibrary.save_loaded_asset(texture)
ue.log('BETA ENGINE ASSETS RESTORED')
