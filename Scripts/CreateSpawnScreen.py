"""
Pose l'ecran flottant "Choisir une experience" devant le point d'apparition du joueur (PlayerStart).
Un clic sur une experience -> le trait GPS au sol guide le joueur jusqu'a la zone.

Le GPS ne demarre plus tout seul vers les NavigatorPoint au lancement (il guide vers l'experience choisie) :
pour retrouver l'ancien comportement, cocher "Start On Begin Play" sur le NavigatorSystem du niveau.

Relancer le script replace l'ecran (l'ancien est supprime).
A lancer : Output Log -> Cmd -> py "Scripts/CreateSpawnScreen.py"
"""

import unreal

LEVEL = "/Game/XRFramework/Levels/L_XRTemplate"
LABEL = "Ecran_ChoixExperience"
DISTANCE = 220.0   # distance devant le point d'apparition (cm)
HEIGHT = 145.0     # hauteur du centre de l'ecran (cm)
SIDE = -170.0      # decalage sur le cote (cm, negatif = a gauche du joueur)

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub.load_level(LEVEL)
actors = actor_sub.get_all_level_actors()

for a in actors:
    if a.get_actor_label() == LABEL:
        actor_sub.destroy_actor(a)

starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
if not starts:
    raise RuntimeError("Aucun PlayerStart dans le niveau")
start = starts[0]
# Devant le joueur mais sur le cote (pour ne pas boucher l'entree du couloir), tourne vers lui.
forward = start.get_actor_forward_vector()
right = start.get_actor_right_vector()
origin = start.get_actor_location()
loc = origin + forward * (DISTANCE - 45.0) + right * SIDE
loc.z = origin.z + HEIGHT
yaw = unreal.MathLibrary.make_rot_from_x(unreal.Vector(origin.x - loc.x, origin.y - loc.y, 0.0)).yaw

screen = actor_sub.spawn_actor_from_class(unreal.load_class(None, "/Script/Enit.FloatingScreen"), loc, unreal.Rotator(0.0, 0.0, yaw))
screen.set_actor_label(LABEL)
screen.set_editor_property("content", unreal.load_class(None, "/Game/Showroom/UI/WBP_ExperienceSelector.WBP_ExperienceSelector_C"))
screen.set_editor_property("resolution", unreal.Vector2D(1920.0, 1000.0))   # large : toutes les experiences visibles
screen.set_editor_property("world_width", 210.0)
screen.set_editor_property("curvature", 32.0)

# Le GPS guide vers l'experience choisie (plus de parcours automatique au lancement).
nav_class = unreal.load_class(None, "/Script/Enit.NavigatorSystem")
for a in actors:
    if a.get_class() == nav_class:
        a.set_editor_property("start_on_begin_play", False)

level_sub.save_current_level()
unreal.log("[Ecran] %s pose en %s (devant %s)" % (LABEL, loc, start.get_actor_label()))
