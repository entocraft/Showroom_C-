"""
Cree le materiau /Game/Showroom/M_ExperienceDarkness utilise par les DetectionZone
pour assombrir l'exterieur de la zone pendant une experience.

  - Translucide, non eclaire (unlit), double face (visible depuis l'interieur de la sphere)
  - Parametre "Color"   (couleur, noir par defaut)
  - Parametre "Opacity" (0..1, pilote par le code pour le fondu)

A lancer une fois : Output Log -> Cmd -> py "Scripts/CreateDarknessMaterial.py"
(ou en ligne de commande avec -run=pythonscript). Sans effet si le materiau existe deja.
"""

import unreal

PATH = "/Game/Showroom"
NAME = "M_ExperienceDarkness"

if unreal.EditorAssetLibrary.does_asset_exist(PATH + "/" + NAME):
    unreal.log("[Darkness] Le materiau existe deja.")
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)

    lib = unreal.MaterialEditingLibrary
    color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    opacity = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.0)
    lib.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(PATH + "/" + NAME)
    unreal.log("[Darkness] Materiau cree : %s/%s" % (PATH, NAME))
