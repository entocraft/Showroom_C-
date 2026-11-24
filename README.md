# ENIT Showroom — version 100 % C++

Même projet que `ENIT-Showroom` (même niveau, mêmes modèles 3D, VR + multijoueur), mais **toute la logique est en C++**
dans `Source/Enit/` (contrôles, expériences, GPS, animation des mains), et tous les réglages sont dans un seul fichier texte.

Seule exception voulue : les **interfaces** sont des **Widget Blueprints** (`Content/Showroom/UI`), modifiables
sans code dans l'éditeur UMG (aucune logique dedans, juste la mise en page).

## La salle

```
           entrée (écran « Choisir une expérience »)
             │
             │   ┌──────────────────┐   ┌──────────────────┐
             │   │ Espace de travail│   │ Galerie des      │
             ╰─╮ │   (9 étapes)     │   │ volumes (5)      │
               ╰─┴────┤ passage ├────────┤ passage ├─────────╮
                 ═══════════ couloir central arrondi ════════╯
               ┌──────┤ passage ├─┐   ┌────┤ passage ├────┐
               │ Salle multimédia │   │  Coin détente    │
               │ (écrans, 5)      │   │  (4 étapes)      │
               └──────────────────┘   └──────────────────┘
```

Construite par `Scripts/CreateShowroomLayout.py` avec les briques modulaires (couloir / murs, zones dessinées,
écrans, zones lumineuses). Relancer le script reconstruit toute la salle à neuf, puis
`py "Scripts/GenerateNarration.py"` refait les voix. Copies du niveau d'avant : `Saved/Backup_AvantDemo`.

## Où modifier quoi ?

| Je veux changer…                                   | Où                                                                 |
|----------------------------------------------------|--------------------------------------------------------------------|
| Vitesse de marche, rotation, téléportation, GPS…   | `Config/DefaultGame.ini` section `[/Script/Enit.ShowroomSettings]` — ou *Project Settings > Game > Showroom* (pas besoin de recompiler) |
| Les boutons des manettes                           | `Source/Enit/Input/ShowroomInput.cpp` (une ligne par bouton) |
| L'apparence d'une interface (fiche, menu, étiquettes, écrans…) | Double-clic sur son **Widget Blueprint** dans `Content/Showroom/UI` (éditeur UMG, sans code) |
| Un nouvel écran flottant                           | Glisser un **Ecran flottant** dans le niveau → *Content* = un Widget Blueprint (dupliquer `WBP_EcranSimple`) |
| Un couloir (droit ou arrondi) ou des murs          | Glisser **Couloir / Murs** → déplacer les points du composant *Path* ; passages : *Openings* |
| Déplacer la fiche d'une expérience                 | En jeu : viser la **barre sous la fiche** + gâchette maintenue |
| Des écrans de détail dans une expérience           | Poser un **Ecran flottant** (*Content* = `WBP_EcranDetail`, remplir *Title / Body / Image*) puis l'ajouter dans *Screens* d'une étape |
| Une nouvelle zone d'expérience, de forme libre     | Glisser une **Zone d'experience** → *Shape* = Dessinée → déplacer les points du composant *Outline* |
| Les expériences proposées sur l'écran du spawn     | Zone → *Selecteur* : *Show In Selector*, *Selector Order* ; titre / photo = sa fiche *Info* |
| Le menu de main (boutons Recommencer / Quitter)    | `WBP_HandMenu` (apparence), `Source/Enit/UI/HandMenuWidget.cpp` (actions) |
| Le texte / la photo d'une fiche info               | Sélectionner la **DetectionZone** → panneau Details → *Zone > Info* |
| La position de la fiche                            | Dans la DetectionZone, sélectionner le composant **Menu** et le déplacer |
| Un seul grand panneau pour plusieurs zones         | Poser un **InfoMenu** dans le niveau et le choisir dans *Linked Menu* de chaque zone |
| L'ordre des étapes du GPS                          | Acteurs **NavigatorPoint** → champ *Order* (0, 1, 2…), ou la liste *Points* du NavigatorSystem |
| L'animation des doigts                             | `Source/Enit/Player/ShowroomHandAnimInstance.cpp` |
| Rotation continue / par crans, douceur             | Réglages → *Deplacement* : `TurnMode`, `SmoothTurnSpeed`, `SmoothTurnAcceleration` |
| Position / suivi de la fiche devant le joueur      | Réglages → *Fiche info* : distance, hauteur, vitesse de suivi, taille |
| L'expérience (assombrissement, fondu)              | Réglages → *Experience* ; texte du bouton / désactiver : DetectionZone → *Experience* |
| Les étapes d'une expérience (objets mis en valeur, textes) | DetectionZone → *Experience > Steps* (voir plus bas) |
| Une zone lumineuse de guidage                      | Poser un **ExperienceSpot** et l'ajouter dans *Spots* d'une étape |
| La voix du narrateur                               | Texte : étape → *Narration Text*, puis `py "Scripts/GenerateNarration.py"` ; moteur de voix : `ENGINE` dans ce script ; vraie voix : glisser le son dans *Narration* |
| La mise en valeur des objets                       | Réglages → *Mise en valeur* |
| Les sons (début, apparition, clic, ambiance…)      | Réglages → *Sons* (choisir un autre son) |
| Chemin lumineux, flèche, ondes                     | Réglages → *Guidage* |

## Organisation du code

```
Source/Enit/
├─ Enit.cpp                    ← module (+ avertissement si le niveau n'utilise pas ShowroomGameMode)
├─ Core/
│  ├─ ShowroomSettings         ← TOUS les réglages (lus depuis DefaultGame.ini)
│  ├─ ShowroomAudio            ← raccourcis pour jouer les sons
│  └─ ShowroomGameMode         ← règles du jeu (fait apparaître le joueur)
├─ Input/
│  └─ ShowroomInput            ← toutes les actions et tous les boutons (Quest, Index, Pico, Vive)
├─ Player/
│  ├─ ShowroomPawn             ← le joueur VR : marche, rotation, téléport, grab, lasers, mains, réseau
│  └─ ShowroomHandAnimInstance ← animation des doigts
├─ Interaction/
│  ├─ ShowroomGrabComponent    ← rend un objet attrapable
│  ├─ GrabbableActor           ← cube physique attrapable
│  ├─ DetectionZone            ← zone d'expérience (boîte ou forme dessinée), fiche, étapes
│  ├─ ZoneGeometry             ← calculs de polygones (zones dessinées)
│  └─ InfoMenu                 ← panneau 3D de fiche info (déplaçable)
├─ Level/
│  └─ Corridor                 ← couloir / murs modulaires dessinés avec une ligne
├─ Experience/
│  ├─ ExperienceTypes          ← une étape d'expérience (FExperienceStep)
│  ├─ ExperienceSpot           ← zone lumineuse au sol qui guide le joueur
│  ├─ ExperienceHighlight      ← mise en valeur d'un objet (projecteur, halo, étiquette)
│  ├─ ExperienceGuide          ← chemin lumineux au sol + flèche vers la zone / l'objet
│  └─ ExperiencePing           ← onde lumineuse au sol (zone qui s'allume, objet mis en valeur)
├─ Navigation/
│  ├─ NavigatorPoint           ← étape du GPS
│  └─ NavigatorSystem          ← le GPS (chemin NavMesh dessiné au sol)
└─ UI/
   ├─ ShowroomWidget           ← base des interfaces (mise en page par défaut → Widget Blueprint)
   ├─ FloatingScreen           ← écran flottant façon Meta Quest (+ FloatingScreenGrabBar)
   ├─ ScreenDrag               ← déplacement au laser (écrans flottants, fiche)
   ├─ ExperienceSelectorWidget ← écran « Choisir une expérience » (+ ExperienceEntryWidget)
   ├─ SimpleScreenWidget       ← modèle d'écran (titre, texte, Fermer)
   ├─ InfoPanelWidget          ← contenu de la fiche (photo, titre, sous-titre, description)
   ├─ ExperienceLabelWidget    ← étiquette flottante au-dessus d'un objet mis en valeur
   ├─ HandMenu                 ← panneau 3D du menu de main
   └─ HandMenuWidget           ← boutons du menu de main

Source/EnitEditor/             ← module ÉDITEUR seulement : fabrique les Widget Blueprints (ShowroomEditorLibrary)
```

| Ancien Blueprint / asset               | Remplacé par (C++)                               |
|----------------------------------------|--------------------------------------------------|
| BP_XRPawn                              | `AShowroomPawn`                                  |
| BP_XRGameMode                          | `AShowroomGameMode`                              |
| BP_VRController                        | (plus nécessaire : le pawn ajoute ses contrôles) |
| IMC_* / IA_* / BP_InputModifier        | `UShowroomInput`                                 |
| ABP_MannequinsXR                       | `UShowroomHandAnimInstance`                      |
| BP_Menu / WBP_Menu                     | `AHandMenu` / `UHandMenuWidget`                  |
| BP_TeleportVisualizer                  | composant `TeleportRing` du pawn                 |
| VRSpectator                            | code du pawn (écran du PC = vue du casque)       |
| BP_GrabComponent                       | `UShowroomGrabComponent`                         |
| BP_Grabbable_SmallCube                 | `AGrabbableActor`                                |
| BP_DetectionZone / BP_InfoMenu         | `ADetectionZone` / `AInfoMenu`                   |
| UI_InfoPanel                           | `UInfoPanelWidget`                               |
| BP_NavigatorPoint / BP_NavigatorSystem | `ANavigatorPoint` / `ANavigatorSystem`           |

## Migration du niveau (déjà faite une fois)

`Scripts/MigrateToCpp.py` remplace les acteurs Blueprint du niveau par les classes C++ (positions conservées),
règle le GameMode du niveau sur `ShowroomGameMode` et supprime tous les Blueprints.
Une copie du dossier `Content` d'avant migration est dans `Saved/Backup_Content_AvantMigration`.

Pour le relancer (par ex. sur une copie fraîche du projet) : **Output Log** → choisir **Cmd** → `py "Scripts/MigrateToCpp.py"`.
Les plugins *Python Editor Script Plugin* et *Editor Scripting Utilities* ne servent qu'à ce script.

## L'expérience (bouton de la fiche)

La fiche d'une DetectionZone affiche un bouton **« Lancer l'expérience »** :
le joueur est placé au centre de la zone et tout ce qui est hors de l'expérience s'assombrit en fondu
(une sphère sombre semi-transparente l'entoure, matériau `Content/Showroom/M_ExperienceDarkness`).
Le bouton devient **« Terminer l'expérience »**. S'éloigner de l'expérience la termine aussi.

Les objets (meubles, écrans, objets attrapables…) sont **déjà dans le niveau** : l'expérience ne les touche pas,
elle n'ajoute que de la **lumière**, de l'**interface** et du **son**. À la fin, tout s'éteint.

### Les étapes

Sélectionner la **DetectionZone** → *Details > Experience > Steps* → **+** pour ajouter une étape.
Les étapes se jouent dans l'ordre ; la fiche affiche **« < Précédent | Étape 2 / 4 | Suivant > »**.

| Champ d'une étape      | Rôle |
|------------------------|------|
| *Title / Description*  | Texte affiché sur la fiche pendant l'étape (vide = texte normal de la fiche) |
| *Highlights*           | Objets **mis en valeur**, l'un après l'autre dans l'ordre de la liste : projecteur au-dessus, halo au sol, étiquette flottante (*Label* = nom, *Details* = petit texte ; vide = pas d'étiquette) |
| *Delay Between Highlights* | Temps entre deux mises en valeur (s) |
| *Spots*                | Zones lumineuses allumées pendant l'étape (celles des étapes passées restent faiblement allumées) |
| *Screens*              | **Écrans flottants de détail** affichés pendant l'étape (cachés sinon) : plus de texte, d'images… que le narrateur **ne lit pas** |
| *Teleport To Spot*     | Placer directement le joueur sur la première zone lumineuse |
| *Advance*              | Passage à l'étape suivante : **Bouton Suivant**, **Minuteur** (*Duration*), **Atteindre la zone lumineuse** ou **Après la narration** |
| *Narration Text / Narration* | Ce que dit le narrateur, et le son de sa voix (voir plus bas) |
| *Sound*                | Son joué au début de l'étape |

Option de la zone : *End After Last Step* (terminer après la dernière étape).

Pour désigner un objet : cliquer sur la pipette à côté du champ *Actor* puis sur l'objet dans le niveau.
Pour un objet en plusieurs morceaux (chaise + dossier…), ajouter chaque morceau ; ne mettre l'étiquette que sur un.

Apparence de la mise en valeur : Réglages → *Mise en valeur* (couleur, puissance du projecteur, étiquettes, fondu).

**La fiche se déplace** : viser la barre arrondie sous la fiche + gâchette maintenue. Une fois posée, elle reste à sa
place (tournée vers le joueur) ; elle revient devant lui s'il s'éloigne de plus de 4 m
(Réglages → *Fiche info* → `InfoPanelReleaseDistance`) ou quand elle se rouvre.

**Écrans de détail** : poser un **Ecran flottant** dans la zone, *Content* = `WBP_EcranDetail`, remplir
*Contenu > Title / Body / Image* directement sur l'écran (pas besoin d'ouvrir le Widget Blueprint), décocher
*Visible At Start*, puis l'ajouter dans *Screens* de l'étape voulue. Le narrateur ne lit que *Narration Text* :
les écrans servent aux détails. Exemple : la **Salle multimédia** (maquette + 3 écrans).

Les 4 expériences de la salle sont dans les dossiers `Exp_…` du World Outliner (voir « La salle »).

### Les zones lumineuses (ExperienceSpot)

Glisser **ExperienceSpot** dans le niveau, au sol (le cylindre orange montre sa taille dans l'éditeur).
Réglages : rayon, couleur, brillance, colonne de lumière, lumière, luminosité une fois l'étape passée, pulsation.
Elles sont invisibles en jeu tant qu'une étape ne les allume pas.
La zone sombre s'agrandit toute seule pour englober les zones lumineuses et les objets mis en valeur.

Durée de l'allumage : Réglages → *Experience* → `ExperienceSpotFadeTime`.

### Le narrateur

Chaque étape a un champ *Narration Text* (ce que dit le narrateur ; vide = titre + description)
et un champ *Narration* (le son de la voix). La voix démarre peu après le début de l'étape
et le fond sonore baisse pendant qu'elle parle. Mode d'avance **« Après la narration »** : l'étape suivante
démarre toute seule quand la voix est finie.

- **Voix de synthèse** (pour commencer) : après avoir écrit ou modifié des textes,
  **Output Log** → **Cmd** → `py "Scripts/GenerateNarration.py"`. Le script crée les sons dans
  `Content/Showroom/Narration/Auto` et remplit *Narration*. Seuls les textes modifiés sont refaits.
- **Moteur de voix** (variable `ENGINE` en haut de `GenerateNarration.py`) :

  | Moteur | Qualité | Ce qu'il faut |
  |---|---|---|
  | `piper` (par défaut) | voix neuronale naturelle, **locale et gratuite** | `Scripts/InstallPiper.bat` (déjà fait sur ce PC : `Saved/TTS`) |
  | `windows` | voix Windows, plus robotique | rien |
  | `elevenlabs` | la plus humaine (service en ligne) | clé dans la variable d'environnement `ELEVENLABS_API_KEY` + id de voix dans le script |
  | `azure` | voix neuronales Microsoft (en ligne) | `AZURE_SPEECH_KEY` et `AZURE_SPEECH_REGION` |

  Réglages Piper dans le script : voix (`PIPER_VOICE`), vitesse (`PIPER_SPEED`), pause entre phrases (`PIPER_PAUSE`).
  Changer de moteur ou de réglage refait toutes les voix au prochain lancement.
- **Vraie voix** : importer l'enregistrement et le glisser dans *Narration* de l'étape.
  Le script ne touche plus jamais à une étape qui a un son personnalisé.

Réglages → *Narrateur* : activer / couper, volume, délai avant la voix, délai avant l'étape suivante.

### Le guidage visuel

- vers une **zone lumineuse à rejoindre** : un chemin de points lumineux défile au sol, du joueur vers la zone
  (il contourne les obstacles grâce au NavMesh), et la zone envoie des ondes lumineuses quand elle s'allume
  puis toutes les 3 s (*Ping Interval* de la zone) ;
- vers un **objet mis en valeur** : une onde au sol quand il s'allume ;
- dans les deux cas, une **flèche** apparaît devant le joueur si la cible n'est pas dans son champ de vision.

Tout se règle (ou se coupe) dans Réglages → *Guidage*. Le GPS se met en pause pendant une expérience.

### Les sons

Sons de base (synthétiques, provisoires) dans `Content/Showroom/Sounds`, créés par `Scripts/CreateBaseSounds.py` :
début / fin d'expérience, changement d'étape, objet mis en valeur (entendu à l'endroit de l'objet),
zone lumineuse qui s'allume / atteinte, clic sur la fiche, ambiance en boucle.

**Pour en changer** : importer ton son, puis Réglages → *Sons* → choisir le son à la place
(ou vider le champ pour aucun son). Volumes : effets, ambiance, ambiance pendant la voix.
Chaque étape peut aussi jouer son propre son (*Sound*).

Pour réagir en code : `OnExperienceStarted`, `OnExperienceStopped`, `OnStepChanged` de `ADetectionZone`.
L'expérience est locale : en multijoueur, chacun voit sa propre expérience.

Le matériau est créé par `Scripts/CreateDarknessMaterial.py` (déjà fait ; à relancer seulement s'il a été supprimé).
Il sert aussi aux zones lumineuses, halos et ondes.

## Zones d'expérience dessinées

Une **Zone d'experience** (classe `DetectionZone`) peut être une boîte ou une **forme libre** :

1. Glisser **Zone d'experience** dans le niveau (panneau *Place Actors*, chercher « Zone »).
2. *Details > Zone > Shape* = **Dessinée (points au sol)**.
3. Sélectionner le composant **Outline** : déplacer ses points au sol ; **Alt + glisser** un point en crée un nouveau,
   clic droit sur un point → *Delete Spline Point* pour l'enlever. Clic droit → *Spline Point Type > Curve* pour arrondir.
4. *Drawn Height* : hauteur de la zone. *Start Point* (petit losange dans le niveau) : où le joueur commence l'expérience
   et où le GPS l'amène.

La zone se voit dans l'éditeur et en jeu (ligne lumineuse au sol, plus brillante pendant l'expérience ;
couleur : Réglages → *Zones*, à couper avec *Show Outline*). Pendant l'expérience, **des murs sombres** suivent
le contour (tout ce qui est dehors s'assombrit) ; sortir de la forme termine l'expérience.
Garder les objets mis en valeur et les zones lumineuses **à l'intérieur** du contour.
Une zone « Boîte » garde l'ancien comportement (sphère sombre qui englobe tout ce qu'utilise l'expérience).

Les 4 alcoves de la salle sont des zones dessinées (`Zone_EspaceTravail`, `Zone_Galerie`, `Zone_Multimedia`, `Zone_Detente`).

## Couloirs et murs modulaires

Glisser **Couloir / Murs** (classe `Corridor`) dans le niveau, sélectionner son composant **Path** et déplacer les points
(Alt + glisser = nouveau point ; clic droit sur un point → *Spline Point Type* : *Curve* = arrondi, *Linear* = angle droit).
- *Layout* = **Couloir** : un sol + deux murs, largeur *Width*, lignes lumineuses au bord du sol (*Edge Lights*) ;
  **Mur** : un seul mur posé sur la ligne (pour fermer une alcove, séparer deux espaces…).
- *Openings* : des passages dans les murs (petits losanges à déplacer dans le niveau), largeur *Opening Width*.
- *Wall Height*, *Wall Thickness*, matériaux (*Wall Material*, *Floor Material*), bouton *Rebuild*.

Les murs bloquent le joueur et le GPS : le NavMesh se recalcule en jeu (`RuntimeGeneration=Dynamic`).

## Écrans flottants et interfaces modifiables sans code

**Écran flottant** (classe `FloatingScreen`) : glisser **Ecran flottant** dans le niveau, puis :
- *Content* : le Widget Blueprint à afficher (n'importe lequel) ;
- *Resolution* (pixels), *World Width* (largeur en cm), *Curvature* (courbure, 0 = plat) ;
- *Movable* : une barre arrondie sous l'écran → viser + **gâchette maintenue** pour le déplacer ;
- *Face Player At Start*, *Visible At Start*.

**Les interfaces sont des Widget Blueprints** (`Content/Showroom/UI`) : double-clic → éditeur UMG → changer couleurs,
textes, polices, images, ajouter des éléments… sans code. Les widgets dont le nom est utilisé par le code
(ex : `TitleText`, `EntryList`, `CloseButton`) peuvent être déplacés et restylés, mais **gardez leur nom**.

| Widget Blueprint | Rôle | Widgets à garder |
|---|---|---|
| `WBP_EcranSimple` | **modèle à dupliquer** pour tes écrans | `CloseButton` (ferme l'écran) |
| `WBP_EcranDetail` | écrans de détail des expériences | `TitleText`, `BodyText`, `MainImage` (remplis par *Title / Body / Image* de l'écran), `CloseButton` |
| `WBP_ExperienceSelector` | écran « Choisir une expérience » | `TitleText`, `EntryList`, `StatusText`, `StopButton` |
| `WBP_ExperienceEntry` | une ligne du sélecteur | `EntryButton`, `TitleText`, `SubtitleText`, `Thumbnail` |
| `WBP_InfoPanel` | fiche info des zones | `TitleText`, `SubtitleText`, `DescriptionText`, `PhotoImage`, `ActionButton`, `StepNav`, `PreviousButton`, `NextButton`, `StepText` |
| `WBP_ExperienceLabel` | étiquette des objets mis en valeur | `LabelText`, `DetailsText` |
| `WBP_HandMenu` | menu de main | `RestartButton`, `QuitButton` |
| `WBP_GrabBar` | barre sous les écrans flottants et la fiche | `GrabButton` |

Quel Widget Blueprint est utilisé : Réglages → *Interfaces* (fiche, menu, étiquette, barre) ; pour le sélecteur,
le champ *Entry Class* de `WBP_ExperienceSelector` (Class Defaults). Un WBP vidé (ou supprimé) revient à la mise en page par défaut.
Les WBP ont été créés par `Scripts/CreateWidgetBlueprints.py` (ne remplace jamais un WBP existant ;
supprimer un WBP puis relancer le script le remet à zéro).

## Icônes et animations des interfaces

- **Icônes** : `Content/Showroom/UI/Icons` (T_Icon_grid, pin, clock, play, help, gear, hand…), dessinées par
  `Scripts/CreateIcons.py` (blanches, colorées par le code). Pour en ajouter une : une fonction de forme dans le
  script, ou importer ton propre PNG blanc sur fond transparent sous le nom `T_Icon_<nom>`.
  En C++ : `ShowroomStyle::MakeIcon(Tree, Nom, TEXT("pin"), Taille, Couleur)` / `MakeIconLabel(...)`.
- **Animations** (sans timeline, dans `UShowroomWidget`, utilisables aussi depuis un Widget Blueprint) :
  `AnimateHover` (l'élément se soulève au survol du laser, avec un petit son), `AnimateIn` (apparition en fondu +
  glissement, en cascade pour les vignettes), `AnimatePulse` (respiration : GPS, « Lancer l'expérience »),
  `PlayIntro` (rejouée quand un écran, une fiche ou une étiquette apparaît).
- **Accueil** : bienvenue, carrousel avec dégradé et « Découvrir l'expérience », vignettes colorées et numérotées
  (couleur = `ShowroomStyle::ExperienceColor`, utilisée tant qu'une expérience n'a pas de photo), badge GPS,
  page d'aide illustrée, barre d'état du GPS. **Fiche de zone** : barre de progression des étapes, boutons à icônes.
- Son de survol : Réglages → *Sons* → `SoundUIHover`.

## Écran « Choisir une expérience » et GPS

`Ecran_ChoixExperience`, devant le point d'apparition, liste toutes les zones d'expérience (titre, sous-titre et photo
de leur fiche *Info*). **Viser une expérience + gâchette** : le **trait GPS** au sol guide le joueur jusqu'à elle
(jusqu'au *Start Point* de la zone) ; il s'arrête quand le joueur entre dans la zone, et la fiche s'ouvre.
« Arrêter le guidage » l'annule. Placé par `Scripts/CreateSpawnScreen.py`.

Écran large (1920 × 1000 px, 2,10 m) : les 4 expériences et la vignette « Prochainement » tiennent sur une ligne
(*Tiles Per Page* = 5). Les mises en page de l'accueil, de la fiche d'expérience et de la fiche de zone viennent du C++
(`ExperienceSelectorWidget`, `ExperienceSheetWidget`, `InfoPanelWidget`, charte dans `ShowroomStyle`) :
après les avoir modifiées, `Scripts/RebuildUI.py` refait les Widget Blueprints et des aperçus PNG dans `Saved/UIPreview`
(lancer l'éditeur complet, voir l'en-tête du script ; les retouches faites à la main dans ces WBP sont écrasées).

Déplacer un panneau (barre sous l'écran ou la fiche) ne déclenche jamais la téléportation : un appui de gâchette
qui commence sur une interface ne téléporte pas, même si le laser quitte l'interface avant le relâchement.

Le GPS ne suit plus automatiquement les NavigatorPoint au lancement (pour ne pas mélanger deux traits) :
pour le retrouver, cocher *Start On Begin Play* sur le NavigatorSystem du niveau. En code : `ANavigatorSystem::GuideTo(Acteur)`.

## Ajouter un objet attrapable

- Le plus simple : glisser **GrabbableActor** dans le niveau et changer son *Mesh*.
- Sur un acteur existant : *Add Component* → **Showroom Grab Component**, en enfant du mesh (qui doit *Simulate Physics*).

## À savoir

- Il faut **Visual Studio 2022** (« Développement de jeux en C++ ») et **Unreal Engine 5.8**.
- Les mains : si elles sont mal orientées dans la manette, ajuster `HandOffsetLeft/Right` dans les réglages (catégorie *Mains*).
- Le GPS utilise le NavMesh : il faut un *NavMeshBoundsVolume* dans le niveau (touche `P` dans l'éditeur pour le voir).
- Seul Blueprint encore visible : le ciel `BP_Sky_Sphere` du sous-niveau d'éclairage, qui fait partie du **moteur** (pas du projet).
