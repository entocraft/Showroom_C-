"""
Migration du niveau vers les classes C++ + suppression de TOUS les Blueprints du projet.

A lancer UNE fois, dans l'editeur (projet compile) :
    Fenetre Output Log -> choisir "Cmd" a gauche -> taper :
        py "Scripts/MigrateToCpp.py"

Ce que fait le script :
  1. Ouvre L_XRTemplate.
  2. Remplace chaque acteur Blueprint par sa version C++ (meme position / rotation / taille) :
        BP_DetectionZone       -> DetectionZone   (taille de la boite + position de la fiche conservees)
        BP_NavigatorPoint      -> NavigatorPoint
        BP_NavigatorSystem     -> NavigatorSystem (sa cible est reprise comme etape 0)
        BP_Grabbable_SmallCube -> GrabbableActor  (mesh / materiau / taille conserves)
        VRSpectator            -> supprime (remplace par le code du pawn)
  3. Met World Settings > GameMode Override sur ShowroomGameMode.
  4. Sauvegarde le niveau.
  5. Supprime les Blueprints (et les assets d'input devenus inutiles).

Le resultat est affiche dans l'Output Log (lignes "[Migration]").
"""

import unreal

LEVEL = "/Game/XRFramework/Levels/L_XRTemplate"

CLASS_MAP = {
    "BP_DetectionZone_C": "/Script/Enit.DetectionZone",
    "BP_NavigatorPoint_C": "/Script/Enit.NavigatorPoint",
    "BP_NavigatorSystem_C": "/Script/Enit.NavigatorSystem",
    "BP_Grabbable_SmallCube_C": "/Script/Enit.GrabbableActor",
}
CLASSES_TO_REMOVE = {"VRSpectator_C"}

# Blueprints (et assets qui ne servent plus) a supprimer apres la migration.
ASSETS_TO_DELETE = [
    "/Game/Assets/Blueprints/BP_NavigatorPoint",
    "/Game/Assets/Blueprints/BP_NavigatorSystem",
    "/Game/Assets/Blueprints/BP_VRController",
    "/Game/Assets/Objects/BP_DetectionZone",
    "/Game/Assets/Objects/BP_Grabbable_SmallCube",
    "/Game/Assets/Objects/BP_InfoMenu",
    "/Game/Assets/UI/UI_InfoPanel",
    "/Game/XRFramework/Blueprints/BPI_PawnAnim",
    "/Game/XRFramework/Blueprints/BP_AssetGuideline",
    "/Game/XRFramework/Blueprints/BP_Grabbable_SmallCube",
    "/Game/XRFramework/Blueprints/BP_GrabComponent",
    "/Game/XRFramework/Blueprints/BP_Menu",
    "/Game/XRFramework/Blueprints/BP_Passthrough",
    "/Game/XRFramework/Blueprints/BP_Projectile",
    "/Game/XRFramework/Blueprints/BP_TeleportVisualizer",
    "/Game/XRFramework/Blueprints/BP_XRGameMode",
    "/Game/XRFramework/Blueprints/BP_XRPawn",
    "/Game/XRFramework/Blueprints/WBP_Menu",
    "/Game/XRFramework/Blueprints/E_GrabType",
    "/Game/XRMannequins/BP_MannequinsXR",
    "/Game/XRMannequins/Meshes/ABP_MannequinsXR",
    "/Game/LevelPrototyping/Interactable/Door/BP_DoorFrame",
    "/Game/LevelPrototyping/Interactable/JumpPad/BP_JumpPad",
    "/Game/LevelPrototyping/Interactable/Target/BP_WobbleTarget",
]
# Dossiers entiers devenus inutiles (inputs maintenant en C++, spectateur Blueprint).
DIRECTORIES_TO_DELETE = [
    "/Game/XRFramework/Input",
    "/Game/VRSpectator",
]


def log(msg):
    unreal.log("[Migration] " + msg)


def warn(msg):
    unreal.log_warning("[Migration] " + msg)


def first_component(actor, cls):
    comps = actor.get_components_by_class(cls)
    return comps[0] if comps else None


def migrate_level():
    actors_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    if not level_sub.load_level(LEVEL):
        raise RuntimeError("Impossible d'ouvrir " + LEVEL)

    old_to_new = {}
    navigator_systems = []   # (ancien, nouveau)
    to_destroy = []

    for actor in actors_sub.get_all_level_actors():
        class_name = actor.get_class().get_name()

        if class_name in CLASSES_TO_REMOVE:
            to_destroy.append(actor)
            continue
        if class_name not in CLASS_MAP:
            continue

        new_class = unreal.load_class(None, CLASS_MAP[class_name])
        if not new_class:
            raise RuntimeError("Classe C++ introuvable : %s (le projet est-il compile ?)" % CLASS_MAP[class_name])

        new_actor = actors_sub.spawn_actor_from_class(new_class, actor.get_actor_location(), actor.get_actor_rotation())
        new_actor.set_actor_scale3d(actor.get_actor_scale3d())
        new_actor.set_actor_label(actor.get_actor_label())
        folder = actor.get_folder_path()
        if folder and str(folder) != "None":
            new_actor.set_folder_path(folder)

        if class_name == "BP_DetectionZone_C":
            old_box = first_component(actor, unreal.BoxComponent)
            if old_box:
                # La boite C++ est la racine : on reprend la place et la taille de l'ancienne boite.
                new_actor.set_actor_transform(old_box.get_world_transform(), False, True)
                new_actor.box.set_box_extent(old_box.get_editor_property("box_extent"), True)
            old_menu = first_component(actor, unreal.ChildActorComponent)
            if old_menu:
                new_actor.menu.set_world_location_and_rotation(
                    old_menu.get_world_location(), old_menu.get_world_rotation(), False, True)

        elif class_name == "BP_Grabbable_SmallCube_C":
            old_mesh = first_component(actor, unreal.StaticMeshComponent)
            if old_mesh:
                new_actor.set_actor_transform(old_mesh.get_world_transform(), False, True)
                new_actor.mesh.set_static_mesh(old_mesh.get_editor_property("static_mesh"))
                for i, mat in enumerate(old_mesh.get_materials()):
                    new_actor.mesh.set_material(i, mat)

        elif class_name == "BP_NavigatorSystem_C":
            navigator_systems.append((actor, new_actor))

        old_to_new[actor] = new_actor
        to_destroy.append(actor)
        log("%s -> %s" % (actor.get_actor_label(), new_class.get_name()))

    # Le GPS : l'ancien avait une seule cible "Target".
    for old_sys, new_sys in navigator_systems:
        try:
            target = old_sys.get_editor_property("Target")
        except Exception:
            target = None
        if target and target in old_to_new:
            new_sys.set_editor_property("points", [old_to_new[target]])
            log("GPS : etape 0 = %s" % old_to_new[target].get_actor_label())
        else:
            log("GPS : pas de cible reprise -> tous les NavigatorPoint du niveau seront utilises (tries par Order)")

    for actor in to_destroy:
        actors_sub.destroy_actor(actor)

    # GameMode du niveau
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world_settings = world.get_world_settings()
    world_settings.set_editor_property("default_game_mode", unreal.load_class(None, "/Script/Enit.ShowroomGameMode"))
    log("World Settings : GameMode Override = ShowroomGameMode")

    level_sub.save_current_level()
    log("Niveau sauvegarde.")


def delete_assets():
    lib = unreal.EditorAssetLibrary
    for path in ASSETS_TO_DELETE:
        if lib.does_asset_exist(path):
            if lib.delete_asset(path):
                log("Supprime : " + path)
            else:
                warn("Echec suppression : " + path)
    for path in DIRECTORIES_TO_DELETE:
        if lib.does_directory_exist(path):
            if lib.delete_directory(path):
                log("Dossier supprime : " + path)
            else:
                warn("Echec suppression dossier : " + path)


migrate_level()
delete_assets()
log("Termine. Plus aucun Blueprint du projet n'est utilise.")
