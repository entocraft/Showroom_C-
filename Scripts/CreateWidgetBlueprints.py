"""
Fabrique les WIDGET BLUEPRINTS du showroom dans /Game/Showroom/UI, a partir des mises en page par defaut (C++).
Ensuite, tout se modifie SANS CODE dans l'editeur UMG (double-clic sur un WBP).

  WBP_InfoPanel         la fiche info des zones
  WBP_HandMenu          le menu de main
  WBP_ExperienceLabel   l'etiquette au-dessus des objets mis en valeur
  WBP_GrabBar           la barre de prise sous les ecrans flottants
  WBP_ExperienceSelector + WBP_ExperienceEntry   l'ecran "Choisir une experience" et ses lignes
  WBP_EcranSimple       modele d'ecran flottant a dupliquer
  WBP_EcranDetail       ecran de detail des experiences (titre / texte / image remplis par l'Ecran flottant)

Les WBP existants ne sont PAS ecrases (tes modifications sont gardees).
Pour remettre un WBP a zero : le supprimer puis relancer, ou mettre OVERWRITE = True.

A lancer : Output Log -> Cmd -> py "Scripts/CreateWidgetBlueprints.py"
"""

import unreal

OVERWRITE = False
DEST = "/Game/Showroom/UI"

WIDGETS = [
    ("WBP_InfoPanel", "InfoPanelWidget"),
    ("WBP_HandMenu", "HandMenuWidget"),
    ("WBP_ExperienceLabel", "ExperienceLabelWidget"),
    ("WBP_GrabBar", "FloatingScreenGrabBar"),
    ("WBP_ExperienceEntry", "ExperienceEntryWidget"),
    ("WBP_ExperienceSelector", "ExperienceSelectorWidget"),
    ("WBP_EcranSimple", "SimpleScreenWidget"),
    ("WBP_EcranDetail", "SimpleScreenWidget"),
]

for name, cpp_class in WIDGETS:
    existed = unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + name)
    parent = unreal.load_class(None, "/Script/Enit." + cpp_class)
    bp = unreal.ShowroomEditorLibrary.create_widget_blueprint_from_layout(DEST, name, parent, OVERWRITE)
    if not bp:
        unreal.log_error("[WBP] %s : echec" % name)
        continue
    if existed and not OVERWRITE:
        unreal.log("[WBP] %s : existe deja (garde)" % name)
    else:
        unreal.EditorAssetLibrary.save_loaded_asset(bp)
        unreal.log("[WBP] %s : cree (parent %s)" % (name, cpp_class))
