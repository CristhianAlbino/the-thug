"""Creates M_TTSurface: the procedural master material for walls, floors and props (M07 art).

No textures: everything is computed from world position, so stretched cube instances never smear.
  Color       base albedo (rgb)
  GrimeColor  dirt / rust / oil the base drifts towards (rgb)
  Shape       x = tile size in cm (0 = no tiles), y = grout width in cm, z = grime amount 0-1, w = wetness 0-1
  Finish      x = roughness, y = speckle 0-1 (terrazzo, concrete pores), z = seed, w = metallic

Grime: multi-octave value noise + a dirty band near the floor on walls.
Wetness: puddles on floors (dark, mirror-like), damp sheen on walls.

Run headless:
  UnrealEditor-Cmd.exe TheThug.uproject -run=pythonscript -script="<abs path>/create_surface_material.py" -unattended -nosplash
"""
import unreal

FOLDER = "/Game/Materials"
NAME = "M_TTSurface"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

HLSL = r"""
float3 an = abs(N);
float isFloor = an.z > 0.5 ? 1.0 : 0.0;
float2 uv = an.z > 0.5 ? WP.xy : (an.x > an.y ? WP.yz : WP.xz);

// 4-octave value noise (large stains)
float n = 0.0; float amp = 0.5; float2 p = uv * 0.0035 + Finish.z * 17.0;
for (int i = 0; i < 4; i++)
{
    float2 ip = floor(p); float2 fp = frac(p); fp = fp * fp * (3.0 - 2.0 * fp);
    float a = frac(sin(dot(ip, float2(127.1, 311.7))) * 43758.5453);
    float b = frac(sin(dot(ip + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float c = frac(sin(dot(ip + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float d = frac(sin(dot(ip + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    n += amp * lerp(lerp(a, b, fp.x), lerp(c, d, fp.x), fp.y);
    p *= 2.07; amp *= 0.5;
}
// puddle noise (one smooth octave)
float2 q = uv * 0.0022 + 7.31 + Finish.z;
float2 iq = floor(q); float2 fq = frac(q); fq = fq * fq * (3.0 - 2.0 * fq);
float qa = frac(sin(dot(iq, float2(269.5, 183.3))) * 43758.5453);
float qb = frac(sin(dot(iq + float2(1, 0), float2(269.5, 183.3))) * 43758.5453);
float qc = frac(sin(dot(iq + float2(0, 1), float2(269.5, 183.3))) * 43758.5453);
float qd = frac(sin(dot(iq + float2(1, 1), float2(269.5, 183.3))) * 43758.5453);
float pn = lerp(lerp(qa, qb, fq.x), lerp(qc, qd, fq.x), fq.y);

// fine speckle (2 cm grains)
float s = frac(sin(dot(floor(uv * 0.5), float2(12.9898, 78.233))) * 43758.5453);

// tiles and grout
float grout = 0.0; float tileTint = 1.0;
if (Shape.x > 1.0)
{
    float2 t = frac(uv / Shape.x);
    float dl = min(min(t.x, 1.0 - t.x), min(t.y, 1.0 - t.y)) * Shape.x;
    grout = 1.0 - saturate(dl / max(Shape.y, 0.05));
    float h = frac(sin(dot(floor(uv / Shape.x), float2(41.3, 289.1))) * 43758.5453);
    tileTint = 0.93 + 0.14 * h;
}

// dirt band where walls meet the floor (floors are 400 cm apart)
float hz = frac((WP.z + 4000.0) / 400.0) * 400.0;
float band = (1.0 - isFloor) * saturate(1.0 - hz / 70.0);

float grime = saturate(saturate(n * 1.7 - 0.45) * Shape.z + band * Shape.z * 0.8 + grout * 0.35);
float3 col = lerp(Col * tileTint, Grime3, grime);
col *= 1.0 - grout * 0.45;
col *= 1.0 + (s - 0.5) * Finish.y * 0.35;

float puddle = isFloor * saturate((pn - (1.0 - Shape.w)) * 6.0);
col *= 1.0 - puddle * 0.4;
float rough = Finish.x + (n - 0.5) * 0.15;
rough = lerp(rough, 0.92, grout * 0.6);
rough = lerp(rough, 0.05, max(puddle, Shape.w * 0.3 * (1.0 - isFloor)));
return float4(col, saturate(rough));
"""


def vector_param(m, name, value, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def mask(m, src, r, g, b, a, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, x, y)
    for ch, v in (("r", r), ("g", g), ("b", b), ("a", a)):
        e.set_editor_property(ch, v)
    mel.connect_material_expressions(src, "", e, "")
    return e


def make_surface():
    path = FOLDER + "/" + NAME
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    m = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())

    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, -200)
    nrm = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, -80)
    col = vector_param(m, "Color", unreal.LinearColor(0.35, 0.37, 0.34, 1.0), -900, 40)
    grime = vector_param(m, "GrimeColor", unreal.LinearColor(0.08, 0.07, 0.05, 1.0), -900, 200)
    shape = vector_param(m, "Shape", unreal.LinearColor(0.0, 0.3, 0.4, 0.0), -900, 360)
    finish = vector_param(m, "Finish", unreal.LinearColor(0.75, 0.2, 0.0, 0.0), -900, 520)

    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 100)
    custom.set_editor_property("code", HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    custom.set_editor_property("description", "TTSurface")
    inputs = []
    for name in ("WP", "N", "Col", "Grime3", "Shape", "Finish"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)
    mel.connect_material_expressions(wp, "", custom, "WP")
    mel.connect_material_expressions(nrm, "", custom, "N")
    mel.connect_material_expressions(col, "", custom, "Col")
    mel.connect_material_expressions(grime, "", custom, "Grime3")
    # Vector parameters output RGB on the main pin: rebuild float4 = RGB + A.
    def rgba(param, x, y):
        e = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, x, y)
        mel.connect_material_expressions(param, "", e, "A")
        mel.connect_material_expressions(param, "A", e, "B")
        return e
    mel.connect_material_expressions(rgba(shape, -700, 360), "", custom, "Shape")
    mel.connect_material_expressions(rgba(finish, -700, 520), "", custom, "Finish")

    rgb = mask(m, custom, True, True, True, False, -200, 40)
    rough = mask(m, custom, False, False, False, True, -200, 200)
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(finish, "A", unreal.MaterialProperty.MP_METALLIC)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    eal.save_asset(m.get_path_name())
    unreal.log("Created " + m.get_path_name())


make_surface()
