"""
INTERFACES de Hugo (maquette Figma "PTUT - Showroom") : regenere les Widget Blueprints a partir des mises en page C++
et dessine des APERCUS PNG dans Saved/UIPreview (sans lancer le jeu).

  WBP_ExperienceSelector  ecran d'accueil facon Disney+ (menu lateral, carrousel, vignettes, pages)
  WBP_ExperienceEntry     une vignette d'experience
  WBP_ExperienceSheet     la fiche d'une experience (galerie, auteurs, bouton GPS)
  WBP_AuthorCard          la carte d'un auteur
  WBP_InfoPanel           la fiche affichee dans chaque zone d'experience
  WBP_HandMenu            le menu de la main (Recommencer / Quitter)

Icones : Content/Showroom/UI/Icons (Scripts/CreateIcons.py). Animations (survol, apparition, respiration) :
voir UShowroomWidget (AnimateHover / AnimateIn / AnimatePulse), elles marchent aussi dans les WBP.

ATTENTION : ces 5 WBP sont REFAITS a neuf (les retouches faites a la main dedans sont perdues).
Mettre REBUILD = False pour seulement refaire les apercus.

A lancer : Output Log -> Cmd -> py "Scripts/RebuildUI.py"
ou en ligne de commande : UnrealEditor.exe Enit.uproject -ExecutePythonScript="<chemin>/Scripts/RebuildUI.py" -QuitAfterUI
"""

import os
import unreal

REBUILD = True
PREVIEWS = True
DEST = "/Game/Showroom/UI"
LEVEL = "/Game/XRFramework/Levels/L_XRTemplate"

WIDGETS = [
    # (asset, classe C++) -- dans l'ordre : les sous-interfaces avant celles qui les utilisent
    ("WBP_AuthorCard", "AuthorCardWidget"),
    ("WBP_ExperienceEntry", "ExperienceEntryWidget"),
    ("WBP_ExperienceSheet", "ExperienceSheetWidget"),
    ("WBP_ExperienceSelector", "ExperienceSelectorWidget"),
    ("WBP_InfoPanel", "InfoPanelWidget"),
    ("WBP_HandMenu", "HandMenuWidget"),
]

lib = unreal.ShowroomEditorLibrary

if REBUILD:
    for name, cpp_class in WIDGETS:
        parent = unreal.load_class(None, "/Script/Enit." + cpp_class)
        bp = lib.create_widget_blueprint_from_layout(DEST, name, parent, True)
        if not bp:
            unreal.log_error("[UI] %s : echec" % name)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(bp)
        unreal.log("[UI] %s : refait (parent %s)" % (name, cpp_class))
        for line in lib.describe_widget_blueprint(bp):
            unreal.log("[UI]    " + line)

if PREVIEWS:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL)
    out = os.path.join(unreal.Paths.project_saved_dir(), "UIPreview")
    selector = unreal.load_class(None, DEST + "/WBP_ExperienceSelector.WBP_ExperienceSelector_C")
    panel = unreal.load_class(None, DEST + "/WBP_InfoPanel.WBP_InfoPanel_C")
    shots = [
        (selector, (1920, 1000), "1_Accueil.png", 0),
        (selector, (1920, 1000), "2_Fiche.png", 1),
        (selector, (1920, 1000), "3_NotreProjet.png", 2),
        (selector, (1920, 1000), "4_Aide.png", 3),
        (panel, (1000, 600), "5_FicheZone.png", 0),
    ]
    for cls, size, file_name, page in shots:
        path = os.path.join(out, file_name)
        ok = lib.render_widget_preview(cls, unreal.Vector2D(size[0], size[1]), path, page)
        unreal.log("[UI] apercu %s : %s" % (file_name, "ok" if ok else "ECHEC"))

unreal.log("[UI] termine")

# Lance en ligne de commande avec -QuitAfterUI : l'editeur se ferme tout seul a la fin.
if "-quitafterui" in unreal.SystemLibrary.get_command_line().lower():
    unreal.SystemLibrary.quit_editor()
