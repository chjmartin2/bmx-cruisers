from pathlib import Path
import unreal as ue
for path in ['/MetaHumanCharacter/Face/SKM_Face','/MetaHumanCharacter/Body/IdentityTemplate/SKM_Body','/MetaHumanAnimator/IdentityTemplate/DefaultArchetypeTexture/T_MetaHumanIdentity_Head_D']:
 a=ue.load_asset(path)
 ue.log('HERO ASSET '+path+' '+str(a))
 if isinstance(a,ue.SkeletalMesh):
  ue.log('HERO SLOTS '+str([(str(x.material_slot_name),str(x.material_interface)) for x in a.materials]))
  c=ue.PoseableMeshComponent();c.set_skinned_asset_and_update(a);ue.log('HERO BONES '+str([str(c.get_bone_name(i)) for i in range(min(25,c.get_num_bones()))]))
