"""Creates the few materials the code-driven prototype needs.

  M_TTSmoke     lit translucent, soft edges (smoke grenade clouds). Params: Color, Opacity, Glow
  M_TTEmissive  unlit opaque glow (lamp tubes, Hunter eyes, keypads). Params: Color, Intensity

Run headless:
  UnrealEditor-Cmd.exe TheThug.uproject -run=pythonscript -script="<abs path>/create_materials.py" -unattended -nosplash
"""
import unreal

FOLDER = "/Game/Materials"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def fresh(name):
    path = FOLDER + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def vector_param(m, name, value, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def scalar_param(m, name, value, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def make_smoke():
    m = fresh("M_TTSmoke")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)

    color = vector_param(m, "Color", unreal.LinearColor(0.5, 0.52, 0.5, 1.0), -700, -100)
    glow = scalar_param(m, "Glow", 0.015, -700, 50)
    opacity = scalar_param(m, "Opacity", 0.55, -700, 200)

    emissive = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 0)
    mel.connect_material_expressions(color, "", emissive, "A")
    mel.connect_material_expressions(glow, "", emissive, "B")

    # Soft silhouette: opaque in the middle, fading at grazing angles.
    fresnel = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -700, 350)
    fresnel.set_editor_property("exponent", 1.5)
    inv = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -500, 350)
    mel.connect_material_expressions(fresnel, "", inv, "")
    soft = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 260)
    mel.connect_material_expressions(opacity, "", soft, "A")
    mel.connect_material_expressions(inv, "", soft, "B")
    depth = mel.create_material_expression(m, unreal.MaterialExpressionDepthFade, -100, 260)
    depth.set_editor_property("fade_distance_default", 120.0)
    mel.connect_material_expressions(soft, "", depth, "InOpacity")

    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(depth, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    unreal.log("Created " + m.get_path_name())


def make_emissive():
    m = fresh("M_TTEmissive")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = vector_param(m, "Color", unreal.LinearColor(1.0, 0.95, 0.85, 1.0), -500, 0)
    intensity = scalar_param(m, "Intensity", 4.0, -500, 150)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 50)
    mel.connect_material_expressions(color, "", mul, "A")
    mel.connect_material_expressions(intensity, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    unreal.log("Created " + m.get_path_name())


make_smoke()
make_emissive()
