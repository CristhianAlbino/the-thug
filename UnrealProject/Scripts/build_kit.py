"""Builds the art kit The Thug uses from the Fab pack "Horror Hospital" (Higher Fusion Studio).

The pack is NOT in the repository (Fab Standard License: no redistribution; 18 GB). Install it with the
Epic Launcher (Fab > Horror Hospital > Add to project > TheThug), then run this headless:

  UnrealEditor-Cmd.exe TheThug.uproject -run=pythonscript -script="<abs path>/build_kit.py" -unattended -nosplash -nullrhi

It
  1. picks the meshes and materials per category (patterns below),
  2. caps their textures at 2048 px (VRAM and package size; the source is 4K/8K),
  3. fills /Game/Kit/DA_TTKitRefs (hard references, /Game/Kit is always cooked) so only what the game uses is packaged,
  4. writes Source/TheThug/TTKitAssets.inl, the table TTKit.cpp loads at runtime.
Without the pack the game falls back to its procedural art.
"""
import os
import re
import unreal

ROOT = "/Game/Horror_Hospital"
eal = unreal.EditorAssetLibrary
reg = unreal.AssetRegistryHelpers.get_asset_registry()
SOURCE = os.path.join(unreal.Paths.project_dir(), "Source", "TheThug", "TTKitAssets.inl")

MESHES = {
    "bed": r"^SM_Bed_01_v0(1|11|2|3|4|5|6|7)$",
    "bedside": r"^SM_Bedside_Table_0\d_v\d+$",
    "desk": r"^(SM_Desk_0[1-3]_v\d+|SM_Metal_Desk_01_v\d+)$",
    "rack": r"^SM_Rack_0[1-9]_v\d+$",
    "cabinet": r"^SM_Cabinet_01_v[1-4]$",
    "fridge": r"^SM_Fridge_(Tall|Mid)_01_v\d+$",
    "ivstand": r"^SM_IV[Ss]tand_01_v0\d$",
    "curtain": r"^SM_Curtain_01_v\d+$",
    "barrier": r"^SM_Medical_Barrier_01_v\d+$",
    "lamp": r"^SM_Lamp_0[12]_v01$",
    "window": r"^SM_Window_0[12]_v0[12]$",
    "doorleaf": r"^SM_Door_01_p04$",
    "lift": r"^SM_Lift_01_p01$",
    "generator": r"^SM_Generator_01_v01$",
    "genwire": r"^SM_Generator_Wire_0\d_v0\d$",
    "pipe": r"^SM_Pipe_0[1-5]_v01$",
    "vent": r"^SM_Vent_0\d_v01$",
    "trash": r"^SM_(TrashCan|Medical_Waste_Bin)_01_v0\d$",
    "wheelchair": r"^SM_Wheel_Chair_01_v01$",
    "stretcher": r"^SM_Stretcher_01_v0\d$",
    "clutter": r"^SM_(Paper_0[12]|Rag_01|Cardboard_0[1-4])_v0\d$",
    "board": r"^SM_(Board_0[1-6]|Image_Board_0[1-3])_v01$",
    "radiator": r"^SM_Radiator_01_v01$",
    "clock": r"^SM_Clock_01_v01$",
    "surgery": r"^SM_(Anesthesia_Machine_01_v01|Treatment_Table_01_v0\d|Scanner_Monitor_01_v1|Bed_Devices_01_v01)$",
    "reception": r"^SM_Reception_01_v01$",
    "bench": r"^SM_Bench_0[12]_v01$",
    "camera": r"^SM_Security_Cam_01_v01$",
    "wires": r"^SM_Wire_0[1-5]_v0\d$",
}
MATERIALS = {
    "wall_paint": "MI_Wall_Front_Painted",
    "wall_back": "MI_Wall_Back",
    "floor_1": "MI_Indoor_Ground_01_A",
    "floor_2": "MI_Indoor_Ground_02_A",
    "floor_toilet": "MI_Toilet_Ground_03_A",
    "floor_or": "MI_Operating_Room_Ground_A",
    "floor_out": "MI_Outdoor_Ground_A",
    "ceiling": "MI_Ceiling_A",
    "door": "MI_Door_01_A",
}
MAX_TEXTURE = 2048


def all_assets():
    out = {}
    for path in eal.list_assets(ROOT, recursive=True, include_folder=False):
        name = path.rsplit("/", 1)[-1].split(".")[0]
        out.setdefault(name, path.split(".")[0])
    return out


def deps_recursive(package, seen):
    if package in seen:
        return
    seen.add(package)
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True)
    for d in reg.get_dependencies(package, opts) or []:
        d = str(d)
        if d.startswith(ROOT):
            deps_recursive(d, seen)


def cap_textures(roots, budget=40):
    """Caps source textures at MAX_TEXTURE, a few per run (8 GB RAM cannot rebuild hundreds of 8K maps at once).
    Run again until it reports 0 remaining."""
    packages = set()
    for p in roots:
        deps_recursive(p, packages)
    todo = []
    for pkg in sorted(packages):
        for ad in reg.get_assets_by_package_name(pkg) or []:
            if str(ad.asset_class_path.asset_name) == "Texture2D":
                todo.append(str(ad.package_name))
    done = 0
    remaining = 0
    for pkg in todo:
        tex = unreal.load_asset(pkg)
        if not tex or tex.get_editor_property("max_texture_size") == MAX_TEXTURE:
            continue
        if done >= budget:
            remaining += 1
            continue
        tex.set_editor_property("max_texture_size", MAX_TEXTURE)
        eal.save_loaded_asset(tex, False)
        done += 1
        tex = None
        unreal.SystemLibrary.collect_garbage()
    unreal.log_warning("[KIT] textures in use: %d, capped now: %d, remaining: %d" % (len(todo), done, remaining))


def main():
    assets = all_assets()
    if not assets:
        unreal.log_error("[KIT] /Game/Horror_Hospital is missing. Add the Fab pack to the project first.")
        return
    mesh_paths = {}
    for cat, pattern in MESHES.items():
        rx = re.compile(pattern)
        found = sorted(p for n, p in assets.items() if rx.match(n))
        mesh_paths[cat] = found
        unreal.log_warning("[KIT] %-10s %d" % (cat, len(found)))
    mat_paths = {}
    for key, name in MATERIALS.items():
        if name in assets:
            mat_paths[key] = assets[name]
        else:
            unreal.log_warning("[KIT] material missing: " + name)

    # Everything the game references, and what it pulls in.
    roots = [p for lst in mesh_paths.values() for p in lst] + list(mat_paths.values())
    if "--textures" in unreal.SystemLibrary.get_command_line():
        cap_textures(roots)
        return

    # The hospital draws floors, walls and ceilings as instanced cubes: the pack's master materials
    # must allow that, or the engine falls back to the default checker material.
    fixed = 0
    for key, p in mat_paths.items():
        mat = eal.load_asset(p)
        base = mat.get_base_material() if mat else None
        if base and not base.get_editor_property("used_with_instanced_static_meshes"):
            base.set_editor_property("used_with_instanced_static_meshes", True)
            unreal.MaterialEditingLibrary.recompile_material(base)
            eal.save_loaded_asset(base, False)
            fixed += 1
    unreal.log_warning("[KIT] master materials enabled for instanced meshes: %d" % fixed)

    # Cook only this: /Game/Kit/DA_TTKitRefs (UTTKitRefs) hard-references every used piece and
    # /Game/Kit is in DirectoriesToAlwaysCook, so the cooker packages them and nothing else of the pack.
    refs_path = "/Game/Kit/DA_TTKitRefs"
    refs = eal.load_asset(refs_path) if eal.does_asset_exist(refs_path) else None
    if not refs:
        factory = unreal.DataAssetFactory()
        refs_class = unreal.load_class(None, "/Script/TheThug.TTKitRefs")
        factory.set_editor_property("data_asset_class", refs_class)
        refs = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_TTKitRefs", "/Game/Kit", refs_class, factory)
    refs.set_editor_property("assets", [eal.load_asset(p) for p in roots])
    eal.save_loaded_asset(refs, False)
    if eal.does_asset_exist("/Game/Kit/PAL_TTKit"):
        eal.delete_asset("/Game/Kit/PAL_TTKit")

    lines = ["// Generated by UnrealProject/Scripts/build_kit.py from the Fab pack Horror Hospital. Do not edit.", "",
             "static const FKitEntry KitMeshes[] = {"]
    for cat, lst in mesh_paths.items():
        for p in lst:
            lines.append('\t{ TEXT("%s"), TEXT("%s.%s") },' % (cat, p, p.rsplit("/", 1)[-1]))
    lines += ["};", "", "static const FKitEntry KitMaterials[] = {"]
    for key, p in mat_paths.items():
        lines.append('\t{ TEXT("%s"), TEXT("%s.%s") },' % (key, p, p.rsplit("/", 1)[-1]))
    lines += ["};", ""]
    with open(SOURCE, "w", encoding="utf-8-sig", newline="\n") as f:
        f.write("\n".join(lines))
    unreal.log_warning("[KIT] wrote %s (%d meshes, %d materials)" % (SOURCE, len([1 for l in mesh_paths.values() for _ in l]), len(mat_paths)))


main()
