"""Generate inexpensive park props from the installed engine's basic shapes.
Run with Unreal Editor Python. Rider/equipment meshes are not changed.
"""
import unreal as ue

ue.load_module('StaticMeshEditor')
subsystem=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
if subsystem is None:subsystem=ue.new_object(ue.StaticMeshEditorSubsystem)
for source,name,fraction in [('Sphere','D_PropSphere',.08),('Cylinder','D_PropCylinder',.16)]:
    destination='/Game/Models/'+name
    mesh=ue.load_asset(destination) if ue.EditorAssetLibrary.does_asset_exist(destination) else ue.EditorAssetLibrary.duplicate_asset('/Engine/BasicShapes/'+source,destination)
    if not mesh:raise RuntimeError('Cannot create '+destination)
    options=ue.StaticMeshReductionOptions()
    options.auto_compute_lod_screen_size=False
    settings=ue.StaticMeshReductionSettings()
    settings.percent_triangles=fraction
    settings.screen_size=1
    options.reduction_settings=[settings]
    if subsystem.set_lods(mesh,options)<1:raise RuntimeError('Prop reduction failed: '+destination)
    ue.EditorAssetLibrary.save_loaded_asset(mesh)
    ue.log('PERFORMANCE PROP '+name+': '+str(mesh.get_num_triangles(0))+' triangles')
