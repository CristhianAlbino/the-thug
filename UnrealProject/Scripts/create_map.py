"""Creates the empty M01 map. The hospital itself is generated at runtime by ATTHospital.

Run headless:
  UnrealEditor-Cmd.exe TheThug.uproject -run=pythonscript -script="<abs path>/create_map.py" -unattended -nosplash
"""
import unreal

MAP_PATH = "/Game/Maps/Hospital_M01"

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    unreal.log("Map already exists: " + MAP_PATH)
else:
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    unreal.log("Saved map {} -> {}".format(MAP_PATH, ok))
