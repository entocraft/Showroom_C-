"""
Importe le LOGO de l'ecran d'accueil : Scripts/Assets/Logo_UTTOP-ENIT.jpg -> /Game/Showroom/UI/Textures/T_UI_LogoENIT
(la texture que l'ecran "Choisir une experience" affiche en haut du menu lateral).

Pour changer de logo : remplacer le fichier image (meme nom) puis relancer le script.
A lancer : Output Log -> Cmd -> py "Scripts/ImportLogo.py"
"""

import os
import unreal

SOURCE = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts", "Assets", "Logo_UTTOP-ENIT.jpg")
DEST = "/Game/Showroom/UI/Textures"
NAME = "T_UI_LogoENIT"

if not os.path.exists(SOURCE):
    raise RuntimeError("Logo introuvable : " + SOURCE)

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = DEST
task.destination_name = NAME
task.replace_existing = True
task.automated = True
task.save = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

tex = unreal.load_asset(DEST + "/" + NAME)
if not tex:
    raise RuntimeError("Import du logo rate")
tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)  # net, sans artefacts
tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
tex.set_editor_property("never_stream", True)   # toujours en pleine resolution (sinon flou / invisible au debut)
tex.set_editor_property("max_texture_size", 1024)  # largement assez net pour le menu, et leger
tex.set_editor_property("srgb", True)
unreal.EditorAssetLibrary.save_loaded_asset(tex)
unreal.log("[Logo] %s importe (%dx%d)" % (DEST + "/" + NAME, tex.blueprint_get_size_x(), tex.blueprint_get_size_y()))
