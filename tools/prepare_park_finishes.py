"""Rebuild only the shoreline blend and firework shader in Unreal Editor."""
from pathlib import Path
import sys
import unreal as ue
sys.path.insert(0,str(Path(ue.Paths.project_dir()).resolve()/'tools'))
from prepare_unreal_assets import shoreline_material,firework_material
shoreline_material()
firework_material()
ue.log('CONTINUOUS SHORE AND VIVID FIREWORK MATERIALS READY')
