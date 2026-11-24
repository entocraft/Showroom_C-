"""
REORGANISE LA SALLE : un couloir central arrondi qui part de l'entree, et 4 alcoves d'experience
de part et d'autre (chacune avec ses murs, sa zone dessinee, ses objets et ses etapes).

    Nord :  [ Espace de travail ]   [ Galerie des volumes ]
            ======== couloir arrondi (depuis l'entree) ========>
    Sud  :  [ Salle multimedia  ]   [ Coin detente        ]

Tout est construit avec les briques modulaires du projet :
  - "Couloir / Murs" (ACorridor)          : le couloir et les murs des alcoves (lignes a points),
  - "Zone d'experience" (ADetectionZone)  : une zone DESSINEE par alcove,
  - "Ecran flottant" (AFloatingScreen)    : les ecrans de detail de la salle multimedia,
  - "Zone lumineuse" (AExperienceSpot)    : les points a rejoindre.

Relancer le script reconstruit tout a neuf. Ensuite : py "Scripts/GenerateNarration.py" pour les voix.
A lancer : Output Log -> Cmd -> py "Scripts/CreateShowroomLayout.py"
"""

import unreal

LEVEL = "/Game/XRFramework/Levels/L_XRTemplate"
CORRIDOR_WIDTH = 320.0
WALL_HEIGHT = 260.0
WALL_T = 15.0
GAP = 80.0                      # espace entre le mur du couloir et l'entree d'une alcove

# Dossiers du World Outliner reconstruits par ce script (+ anciennes demos)
FOLDERS = ["Salle_Couloir", "Exp_EspaceTravail", "Exp_Galerie", "Exp_Multimedia", "Exp_Detente",
           "Experience_Demo", "Experience_Demo_Dessinee"]
KEEP = {"BP_DetectionZone", "Zone_EspaceTravail"}   # la zone d'origine du niveau est reutilisee

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub.load_level(LEVEL)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
DARK = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")
GRID = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")
ZONE_CLASS = unreal.load_class(None, "/Script/Enit.DetectionZone")
CORRIDOR_CLASS = unreal.load_class(None, "/Script/Enit.Corridor")
SCREEN_CLASS = unreal.load_class(None, "/Script/Enit.FloatingScreen")
SPOT_CLASS = unreal.load_class(None, "/Script/Enit.ExperienceSpot")
GRAB_CLASS = unreal.load_class(None, "/Script/Enit.GrabbableActor")
DETAIL_WBP = unreal.load_class(None, "/Game/Showroom/UI/WBP_EcranDetail.WBP_EcranDetail_C")

# --- Nettoyage -------------------------------------------------------------
for a in actor_sub.get_all_level_actors():
    if str(a.get_folder_path()) in FOLDERS and a.get_actor_label() not in KEEP:
        actor_sub.destroy_actor(a)
if unreal.EditorAssetLibrary.does_directory_exist("/Game/Showroom/Narration/Auto"):
    unreal.EditorAssetLibrary.delete_directory("/Game/Showroom/Narration/Auto")  # voix refaites par GenerateNarration

by_label = {a.get_actor_label(): a for a in actor_sub.get_all_level_actors()}
spawned = []


def floor_z(x, y):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 400.0), unreal.Vector(x, y, -1000.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, spawned,
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit:
        for value in hit.to_tuple():
            if isinstance(value, unreal.Vector):
                return value.z
    return 0.0


def tag(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    spawned.append(actor)
    return actor


def box(label, folder, x, y, size, z_bottom=0.0, mesh=None, material=GRID, yaw=0.0):
    z = floor_z(x, y)
    a = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor,
                                         unreal.Vector(x, y, z + z_bottom + size[2] / 2.0), unreal.Rotator(0.0, 0.0, yaw))
    a.static_mesh_component.set_static_mesh(mesh or CUBE)
    if material:
        a.static_mesh_component.set_material(0, material)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return tag(a, label, folder)


def move(label, x, y, yaw=None):
    """Deplace un objet DEJA dans le niveau (garde sa hauteur au-dessus du sol)."""
    a = by_label.get(label)
    if not a:
        unreal.log_warning("[Salle] objet introuvable : %s" % label)
        return None
    loc = a.get_actor_location()
    a.set_actor_location(unreal.Vector(x, y, loc.z), False, False)
    if yaw is not None:
        a.set_actor_rotation(unreal.Rotator(0.0, 0.0, yaw), False)
    return a


def spot(label, folder, x, y, color):
    s = actor_sub.spawn_actor_from_class(SPOT_CLASS, unreal.Vector(x, y, floor_z(x, y)))
    s.set_editor_property("color", color)
    return tag(s, label, folder)


def screen(label, folder, x, y, yaw, title, body, width=120.0, z=150.0):
    s = actor_sub.spawn_actor_from_class(SCREEN_CLASS, unreal.Vector(x, y, floor_z(x, y) + z), unreal.Rotator(0.0, 0.0, yaw))
    s.set_editor_property("content", DETAIL_WBP)
    s.set_editor_property("resolution", unreal.Vector2D(1280.0, 860.0))
    s.set_editor_property("world_width", width)
    s.set_editor_property("curvature", 15.0)
    s.set_editor_property("visible_at_start", False)
    s.set_editor_property("title", title)
    s.set_editor_property("body", body)
    return tag(s, label, folder)


def wall_line(label, points, folder="Salle_Couloir"):
    w = actor_sub.spawn_actor_from_class(CORRIDOR_CLASS, unreal.Vector(0, 0, 0))
    w.set_editor_property("layout", unreal.WallLayout.WALL)
    w.set_editor_property("wall_height", WALL_HEIGHT)
    w.set_editor_property("wall_thickness", WALL_T)
    path = w.get_editor_property("path")
    path.clear_spline_points(False)
    for i, (x, y) in enumerate(points):
        path.add_spline_point(unreal.Vector(x, y, floor_z(x, y)), unreal.SplineCoordinateSpace.WORLD, False)
        path.set_spline_point_type(i, unreal.SplinePointType.LINEAR, False)
    path.update_spline()
    w.rebuild()
    return tag(w, label, folder)


def hl(actor, label="", details=""):
    h = unreal.ExperienceHighlightInfo()
    h.set_editor_property("actor", actor)
    h.set_editor_property("label", label)
    h.set_editor_property("details", details)
    return h


A = unreal.ExperienceAdvance


def step(title, description, narration="", highlights=(), spots=(), screens=(), advance=A.BUTTON, delay=0.6):
    s = unreal.ExperienceStep()
    s.set_editor_property("title", title)
    s.set_editor_property("description", description)
    s.set_editor_property("narration_text", narration)
    s.set_editor_property("highlights", [h for h in highlights if h.get_editor_property("actor")])
    s.set_editor_property("spots", list(spots))
    s.set_editor_property("screens", list(screens))
    s.set_editor_property("advance", advance)
    s.set_editor_property("delay_between_highlights", delay)
    return s


def info(title, subtitle, description):
    i = unreal.ShowroomInfo()
    i.set_editor_property("title", title)
    i.set_editor_property("subtitle", subtitle)
    i.set_editor_property("description", description)
    return i


# ============================================================================
# 1) Le couloir central arrondi (de l'entree vers l'est)
# ============================================================================
CORRIDOR_POINTS = [(-790, 1150), (-780, 620), (-520, 150), (0, -120), (750, -220), (1500, -170), (2100, 0)]
corridor = actor_sub.spawn_actor_from_class(CORRIDOR_CLASS, unreal.Vector(0, 0, 0))
corridor.set_editor_property("layout", unreal.WallLayout.CORRIDOR)
corridor.set_editor_property("width", CORRIDOR_WIDTH)
corridor.set_editor_property("wall_height", WALL_HEIGHT)
corridor.set_editor_property("wall_thickness", WALL_T)
path = corridor.get_editor_property("path")
path.clear_spline_points(False)
for x, y in CORRIDOR_POINTS:
    path.add_spline_point(unreal.Vector(x, y, floor_z(x, y)), unreal.SplineCoordinateSpace.WORLD, False)
path.update_spline()
tag(corridor, "Couloir_Central", "Salle_Couloir")

# Le couloir, echantillonne (pour placer les alcoves juste a cote)
samples = []
length = path.get_spline_length()
d = 0.0
while d <= length:
    samples.append(path.get_location_at_distance_along_spline(d, unreal.SplineCoordinateSpace.WORLD))
    d += 25.0
HALF = CORRIDOR_WIDTH / 2.0 + WALL_T


def front_y(x0, x1, north):
    ys = [p.y for p in samples if x0 - 60 <= p.x <= x1 + 60]
    return (max(ys) + HALF + GAP) if north else (min(ys) - HALF - GAP)


openings = []


def alcove(name, x0, x1, north, back):
    """Murs d'une alcove (ouverte cote couloir) + passage dans le mur du couloir. Renvoie (x0, x1, front, back)."""
    front = front_y(x0, x1, north)
    wall_line("Murs_" + name, [(x0, front), (x0, back), (x1, back), (x1, front)])
    # passage dans le mur du couloir, en face du milieu de l'alcove
    target = unreal.Vector((x0 + x1) / 2.0, front, 0.0)
    center = path.find_location_closest_to_world_location(target, unreal.SplineCoordinateSpace.WORLD)
    right = path.find_right_vector_closest_to_world_location(target, unreal.SplineCoordinateSpace.WORLD)
    side = 1.0 if (right.x * (target.x - center.x) + right.y * (target.y - center.y)) > 0 else -1.0
    off = CORRIDOR_WIDTH / 2.0 + WALL_T / 2.0
    openings.append(unreal.Vector(center.x + right.x * off * side, center.y + right.y * off * side, center.z))
    return x0, x1, front, back


N1 = alcove("EspaceTravail", -150.0, 650.0, True, 1150.0)
N2 = alcove("Galerie", 750.0, 1650.0, True, 1150.0)
S1 = alcove("Multimedia", -150.0, 650.0, False, -1150.0)
S2 = alcove("Detente", 750.0, 1550.0, False, -1150.0)

corridor.set_editor_property("openings", openings)
corridor.set_editor_property("opening_width", 320.0)
corridor.rebuild()


def make_zone(actor, label, folder, rect, order, title, subtitle, description, start_depth=170.0):
    """Zone d'experience DESSINEE qui remplit l'alcove (rect = x0, x1, front, back)."""
    x0, x1, front, back = rect
    inward = 1.0 if back > front else -1.0
    cx = (x0 + x1) / 2.0
    zone = actor or actor_sub.spawn_actor_from_class(ZONE_CLASS, unreal.Vector(cx, (front + back) / 2.0, 100.0))
    zone.set_actor_location(unreal.Vector(cx, (front + back) / 2.0, floor_z(cx, front) + 100.0), False, False)
    zone.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    tag(zone, label, folder)
    zone.set_editor_property("shape", unreal.ZoneShape.DRAWN)
    zone.set_editor_property("drawn_height", WALL_HEIGHT)
    zone.set_editor_property("selector_order", order)
    zone.set_editor_property("info", info(title, subtitle, description))
    inset = WALL_T + 10.0
    pts = [(x0 + inset, front), (x0 + inset, back - inward * inset), (x1 - inset, back - inward * inset), (x1 - inset, front)]
    outline = zone.get_editor_property("outline")
    outline.clear_spline_points(False)
    for i, (x, y) in enumerate(pts):
        outline.add_spline_point(unreal.Vector(x, y, floor_z(x, y) + 1.0), unreal.SplineCoordinateSpace.WORLD, False)
        outline.set_spline_point_type(i, unreal.SplinePointType.LINEAR, False)
    outline.update_spline()
    loc = zone.get_actor_location()
    zone.set_editor_property("start_point", unreal.Vector(0.0, front + inward * start_depth - loc.y, -100.0))
    return zone


# ============================================================================
# 2) Espace de travail (nord-ouest) : la zone d'origine du niveau, reutilisee
# ============================================================================
F = "Exp_EspaceTravail"
x0, x1, fr, bk = N1
cx = (x0 + x1) / 2.0
zone_ws = make_zone(by_label.get("BP_DetectionZone") or by_label.get("Zone_EspaceTravail"), "Zone_EspaceTravail", F, N1, 0,
                    "Espace de travail", "Visite guidée · 9 étapes",
                    "Découvrez un espace de travail pensé pour le confort : bureau, rangement et atelier.")
bureau = box("Bureau", F, cx, bk - 200, (150, 70, 75))
pied = box("PiedEcran", F, cx, bk - 222, (10, 10, 12), z_bottom=75, material=DARK)
ecran = box("Ecran", F, cx, bk - 222, (110, 5, 65), z_bottom=87, material=DARK)
assise = box("Chaise", F, cx, bk - 310, (45, 45, 45), material=DARK)
dossier = box("ChaiseDossier", F, cx, bk - 340, (45, 8, 50), z_bottom=45, material=DARK)
spot_a = spot("ZoneLumineuse_Orange", F, x0 + 230, fr + 520, unreal.LinearColor(1.0, 0.65, 0.2, 1.0))
etagere = box("Etagere", F, x0 + 60, fr + 520, (40, 120, 180))
lampe = box("Lampe", F, x0 + 60, fr + 520, (25, 25, 30), z_bottom=180, mesh=CYLINDER, material=DARK)
spot_b = spot("ZoneLumineuse_Bleue", F, x1 - 170, fr + 300, unreal.LinearColor(0.2, 0.7, 1.0, 1.0))
table = box("TableBasse", F, x1 - 170, fr + 450, (70, 70, 45))
objets = []
for i in range(3):
    g = actor_sub.spawn_actor_from_class(GRAB_CLASS, unreal.Vector(x1 - 195 + 25 * i, fr + 450, floor_z(x1 - 170, fr + 450) + 60))
    objets.append(tag(g, "Objet_%d" % (i + 1), F))

zone_ws.set_editor_property("steps", [
    step("Bienvenue", "Découvrez un espace de travail, étape par étape.\nLaissez-vous guider par la lumière.",
         "Bienvenue ! Je vais vous faire découvrir, étape par étape, un espace de travail pensé pour le confort. "
         "Laissez-vous guider par la lumière.", advance=A.AFTER_NARRATION),
    step("Le bureau", "Un large plateau de 150 × 70 cm.\nCliquez sur Suivant pour continuer.",
         "Commençons par le bureau. Son large plateau de cent cinquante centimètres offre tout l'espace "
         "nécessaire pour travailler confortablement. Cliquez sur Suivant quand vous êtes prêt.",
         highlights=[hl(bureau, "Bureau", "150 × 70 cm")]),
    step("L'écran", "Un écran 27 pouces sur un pied réglable.",
         "Sur le bureau, un écran de vingt-sept pouces, monté sur un pied réglable en hauteur, "
         "pour garder une posture naturelle.",
         highlights=[hl(ecran, "Écran 27 pouces", "Pied réglable"), hl(pied)], advance=A.AFTER_NARRATION),
    step("La chaise", "Une chaise ergonomique avec soutien lombaire.\nCliquez sur Suivant pour continuer.",
         "Devant le bureau, une chaise ergonomique. Son dossier soutient le bas du dos, "
         "même pendant de longues journées. Cliquez sur Suivant pour continuer.",
         highlights=[hl(assise, "Chaise ergonomique", "Soutien lombaire"), hl(dossier)]),
    step("Le rangement", "Suivez le chemin lumineux jusqu'à la zone orange.",
         "Passons au rangement. Suivez le chemin lumineux au sol, jusqu'à la zone orange.",
         highlights=[hl(etagere, "Étagère", "5 niveaux de rangement")], spots=[spot_a], advance=A.REACH_SPOT),
    step("L'éclairage", "Une lampe d'appoint à lumière chaude.",
         "Parfait. Au-dessus de l'étagère, une lampe d'appoint diffuse une lumière chaude, agréable en fin de journée.",
         highlights=[hl(lampe, "Lampe d'appoint", "Lumière chaude")], spots=[spot_a], advance=A.AFTER_NARRATION),
    step("L'atelier", "Rejoignez la zone bleue.",
         "Direction l'atelier. Rejoignez maintenant la zone bleue.",
         highlights=[hl(table, "Atelier")], spots=[spot_b], advance=A.REACH_SPOT),
    step("À vous de jouer", "Attrapez les objets posés sur la table\n(bouton de prise des manettes).\nCliquez sur Suivant quand vous avez fini.",
         "À vous de jouer ! Attrapez les objets posés sur la table, avec le bouton de prise de vos manettes. "
         "Cliquez sur Suivant quand vous avez terminé.",
         highlights=[hl(objets[0], "Échantillon A"), hl(objets[1], "Échantillon B"), hl(objets[2], "Échantillon C")],
         spots=[spot_b], delay=0.35),
    step("Merci !", "La visite est terminée.\nCliquez sur Terminer l'expérience.",
         "La visite de l'espace de travail est terminée. Merci ! Cliquez sur Terminer l'expérience pour revenir au couloir."),
])

# ============================================================================
# 3) Galerie des volumes (nord-est) : les volumes d'origine du niveau, deplaces
# ============================================================================
F = "Exp_Galerie"
x0, x1, fr, bk = N2
cx = (x0 + x1) / 2.0
zone_gal = make_zone(None, "Zone_Galerie", F, N2, 1, "Galerie des volumes", "Zone dessinée · 5 étapes",
                     "Une petite galerie de formes simples : la table et ses cubes, une sphère, de grands volumes.")
old_table = by_label.get("Table")
if old_table:
    t0 = old_table.get_actor_location()
    tx, ty = x0 + 150, fr + 380
    for a in actor_sub.get_all_level_actors():   # les cubes attrapables suivent la table
        if a.get_class() == GRAB_CLASS and str(a.get_folder_path()) not in FOLDERS:
            p = a.get_actor_location()
            if abs(p.x - t0.x) < 80 and abs(p.y - t0.y) < 140:
                a.set_actor_location(unreal.Vector(p.x - t0.x + tx, p.y - t0.y + ty, p.z), False, False)
    move("Table", tx, ty)
move("SM_Ball_01", cx + 50, fr + 520)
move("Cube4", x1 - 160, fr + 430)
move("Cube3", cx, bk - 260, yaw=0.0)
spot_v = spot("ZoneLumineuse_Violette", F, cx + 50, bk - 560, unreal.LinearColor(0.75, 0.4, 1.0, 1.0))
zone_gal.set_editor_property("steps", [
    step("Bienvenue", "Une galerie de formes simples.",
         "Bienvenue dans la galerie des volumes. Cette zone a été dessinée librement au sol : "
         "son contour lumineux en montre les limites.", advance=A.AFTER_NARRATION),
    step("La table", "Des petits cubes à attraper.\nCliquez sur Suivant pour continuer.",
         "Commençons par la table. Vous pouvez attraper les petits cubes posés dessus. "
         "Cliquez sur Suivant quand vous êtes prêt.",
         highlights=[hl(by_label.get("Table"), "Table d'exposition", "Cubes à manipuler")]),
    step("La sphère", "Une sphère parfaite.", "Juste à côté, une sphère parfaite, posée directement au sol.",
         highlights=[hl(by_label.get("SM_Ball_01"), "Sphère", "70 cm de diamètre")], advance=A.AFTER_NARRATION),
    step("Les grands volumes", "Rejoignez la zone violette.",
         "Rejoignez maintenant la zone violette, au pied des grands volumes.",
         highlights=[hl(by_label.get("Cube4"), "Volume pivoté"), hl(by_label.get("Cube3"), "Grand volume", "4 × 3 mètres")],
         spots=[spot_v], advance=A.REACH_SPOT),
    step("Merci !", "La visite est terminée.\nCliquez sur Terminer l'expérience.",
         "La visite de la galerie est terminée. Merci ! Cliquez sur Terminer l'expérience pour revenir au couloir."),
])

# ============================================================================
# 4) Salle multimedia (sud-ouest) : des ECRANS de detail, non lus par le narrateur
# ============================================================================
F = "Exp_Multimedia"
x0, x1, fr, bk = S1
cx = (x0 + x1) / 2.0
mid = (fr + bk) / 2.0
zone_mm = make_zone(None, "Zone_Multimedia", F, S1, 2, "Salle multimédia", "Écrans de détail · 5 étapes",
                    "Une maquette et des écrans qui donnent tous les détails du projet.")
socle = box("Socle", F, cx, mid - 40, (60, 60, 90), material=DARK)
maquette = box("Maquette", F, cx, mid - 40, (50, 40, 22), z_bottom=90)
tour = box("MaquetteTour", F, cx + 12, mid - 45, (14, 14, 40), z_bottom=112, mesh=CYLINDER)
ecran_fond = screen("Ecran_Maquette", F, cx, bk + 60, 90.0, "La maquette du projet",
                    "Échelle 1/50 · chêne, verre feuilleté, acier brossé.\n\n"
                    "La maquette présente l'aménagement complet de l'espace : zones de travail, salles de réunion, "
                    "espaces de détente et circulations.\n\n"
                    "Chaque élément a été pensé pour faciliter la collaboration tout en préservant des zones calmes "
                    "pour la concentration.\n\n"
                    "Ce texte n'est pas lu par le narrateur : il est là pour ceux qui veulent en savoir plus.",
                    width=170.0, z=170.0)
ecran_chiffres = screen("Ecran_Chiffres", F, x0 + 70, mid, 0.0, "Les chiffres clés",
                        "• 1 200 m² de surface utile\n• 85 postes de travail\n• 6 salles de réunion\n"
                        "• 2 espaces de détente\n• 40 % de lumière naturelle en plus\n• Certification HQE visée")
ecran_calendrier = screen("Ecran_Calendrier", F, x1 - 70, mid, 180.0, "Le calendrier",
                          "Janvier — études et conception\nMars — validation de la maquette\nJuin — début des travaux\n"
                          "Octobre — aménagement intérieur\nDécembre — ouverture au public")
zone_mm.set_editor_property("steps", [
    step("Bienvenue", "Des écrans vous donnent plus de détails.\nPrenez le temps de les lire.",
         "Bienvenue dans la salle multimédia. Ici, des écrans vous donnent plus de détails : prenez le temps de les lire.",
         advance=A.AFTER_NARRATION),
    step("La maquette", "Lisez l'écran derrière la maquette,\npuis cliquez sur Suivant.",
         "Voici la maquette du projet. L'écran derrière elle donne tous les détails. "
         "Lisez-le, puis cliquez sur Suivant.",
         highlights=[hl(maquette, "Maquette", "Échelle 1/50"), hl(tour), hl(socle)], screens=[ecran_fond]),
    step("Les chiffres clés", "Les chiffres clés du projet.\nCliquez sur Suivant pour continuer.",
         "Sur cet écran, les chiffres clés du projet. Cliquez sur Suivant pour continuer.",
         screens=[ecran_chiffres]),
    step("Le calendrier", "Les grandes étapes du projet.\nCliquez sur Suivant pour continuer.",
         "Et sur celui-ci, les grandes étapes du calendrier. Cliquez sur Suivant pour continuer.",
         screens=[ecran_calendrier]),
    step("Merci !", "Vous pouvez déplacer les écrans\navec la barre située dessous.\nCliquez sur Terminer l'expérience.",
         "La visite multimédia est terminée. Astuce : vous pouvez déplacer les écrans, et cette fiche, "
         "avec la barre située en dessous. Cliquez sur Terminer l'expérience pour revenir au couloir.",
         screens=[ecran_fond, ecran_chiffres, ecran_calendrier]),
])

# ============================================================================
# 5) Coin detente (sud-est)
# ============================================================================
F = "Exp_Detente"
x0, x1, fr, bk = S2
cx = (x0 + x1) / 2.0
zone_dt = make_zone(None, "Zone_Detente", F, S2, 3, "Coin détente", "Pause · 4 étapes",
                    "Un fauteuil, une plante et une lumière douce pour faire une vraie pause.")
fauteuil = box("Fauteuil", F, cx - 120, bk + 160, (85, 80, 42))
fauteuil_dos = box("FauteuilDossier", F, cx - 120, bk + 112, (85, 15, 45), z_bottom=42)
table_d = box("TableDetente", F, cx + 20, bk + 200, (60, 60, 40), mesh=CYLINDER, material=DARK)
pot = box("PlantePot", F, x1 - 110, bk + 110, (35, 35, 40), mesh=CYLINDER, material=DARK)
feuillage = box("PlanteFeuillage", F, x1 - 110, bk + 110, (70, 70, 80), z_bottom=40, mesh=SPHERE)
spot_g = spot("ZoneLumineuse_Verte", F, cx - 60, bk + 330, unreal.LinearColor(0.3, 1.0, 0.45, 1.0))
zone_dt.set_editor_property("steps", [
    step("Bienvenue", "Un espace pour faire une pause.",
         "Bienvenue dans le coin détente, un espace pour faire une vraie pause, loin de l'écran.",
         advance=A.AFTER_NARRATION),
    step("Le fauteuil", "Rejoignez la zone verte, près du fauteuil.",
         "Rejoignez la zone verte, juste devant le fauteuil.",
         highlights=[hl(fauteuil, "Fauteuil", "Assise profonde"), hl(fauteuil_dos), hl(table_d)],
         spots=[spot_g], advance=A.REACH_SPOT),
    step("La plante", "Un peu de verdure.",
         "Un peu de verdure améliore la qualité de l'air et aide à se détendre.",
         highlights=[hl(feuillage, "Plante verte", "Améliore la qualité de l'air"), hl(pot)],
         spots=[spot_g], advance=A.AFTER_NARRATION),
    step("Merci !", "La visite est terminée.\nCliquez sur Terminer l'expérience.",
         "Bonne pause ! Cliquez sur Terminer l'expérience pour revenir au couloir."),
])

# ============================================================================
# 6) Divers : ecran de choix a l'entree, zone "pas de teleportation" hors du couloir
# ============================================================================
starts = [a for a in actor_sub.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
choice = by_label.get("Ecran_ChoixExperience")
if choice and starts:
    # meme placement que Scripts/CreateSpawnScreen.py : devant le joueur, sur le cote de l'entree du couloir
    s = starts[0].get_actor_location()
    pos = s + starts[0].get_actor_forward_vector() * 165.0 + starts[0].get_actor_right_vector() * -170.0
    pos.z = s.z + 145.0
    to_start = unreal.Vector(s.x - pos.x, s.y - pos.y, 0.0)
    yaw = unreal.MathLibrary.make_rot_from_x(to_start).yaw
    choice.set_actor_location_and_rotation(pos, unreal.Rotator(0.0, 0.0, yaw), False, False)
for label in ("NavModifier_NoTeleport", "TextRenderActor"):
    a = by_label.get(label)
    if a:
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x + 1400.0, p.y - 550.0, p.z), False, False)

# Le NavMesh se recalcule en jeu (couloir, murs) : voir aussi DefaultEngine.ini
for a in actor_sub.get_all_level_actors():
    if a.get_class().get_name() == "RecastNavMesh":
        a.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)

level_sub.save_current_level()
unreal.log("[Salle] Couloir + 4 alcoves : N1=%s N2=%s S1=%s S2=%s ; passages=%d" % (
    [round(v) for v in N1], [round(v) for v in N2], [round(v) for v in S1], [round(v) for v in S2], len(openings)))
