"""Optional Blender bridge. Prepared here; Blender execution NOT performed here.

blender --background --python Tools/ArtPipeline/export_blender.py -- \
  --input 05_ART/PrototypeKit_v01/Characters/SK_Joe_v01.glb \
  --out 05_ART/PrototypeKit_v01/Blender
"""
import argparse,json,sys
from pathlib import Path
import bpy

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--out',type=Path,required=True);args=p.parse_args(args)
    source=args.input.resolve();target=args.out.resolve();target.mkdir(parents=True,exist_ok=True)
    if not source.is_file():raise FileNotFoundError(source)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    armatures=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
    if not meshes:raise RuntimeError('GLB importou sem malhas')
    is_character=source.name.startswith('SK_')
    if is_character and not armatures:raise RuntimeError('Personagem importou sem armature')
    for obj in meshes:
        for face in obj.data.polygons:face.use_smooth=True
    bpy.ops.file.pack_all()
    blend=target/(source.stem+'.blend');fbx=target/(source.stem+'.fbx')
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.context.scene.objects:
        if obj.type in {'MESH','ARMATURE','EMPTY'}:obj.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH','ARMATURE','EMPTY'},add_leaf_bones=False,use_armature_deform_only=False,bake_anim=True,bake_anim_use_all_actions=True,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0.0,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',path_mode='COPY',embed_textures=True)
    report={'blender_version':bpy.app.version_string,'source':str(source),'blend':str(blend),'fbx':str(fbx),'mesh_objects':len(meshes),'armatures':[{'name':a.name,'bones':len(a.data.bones)} for a in armatures],'actions':[a.name for a in bpy.data.actions],'status':'exported_needs_visual_and_unreal_import_review'}
    (target/(source.stem+'_export_report.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
