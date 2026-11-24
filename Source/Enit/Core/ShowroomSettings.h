#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ShowroomSettings.generated.h"

class USoundBase;
class UUserWidget;

/** Mode de rotation du joueur au joystick. */
UENUM(BlueprintType)
enum class EShowroomTurnMode : uint8
{
	/** Rotation par crans (ex: 45 degres d'un coup) - le plus confortable en VR. */
	Snap   UMETA(DisplayName = "Par crans (Snap)"),
	/** Rotation continue. */
	Smooth UMETA(DisplayName = "Continue (Smooth)")
};

/**
 * TOUS LES REGLAGES DU SHOWROOM AU MEME ENDROIT.
 *
 * Deux facons de les modifier :
 *   1) Dans l'editeur : Edit > Project Settings > Game > Showroom
 *   2) A la main dans Config/DefaultGame.ini, section [/Script/Enit.ShowroomSettings]
 *
 * Aucune recompilation n'est necessaire pour changer une valeur.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Showroom"))
class ENIT_API UShowroomSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Acces rapide depuis le C++ : UShowroomSettings::Get()->WalkSpeed */
	static const UShowroomSettings* Get() { return GetDefault<UShowroomSettings>(); }

	// ------------------------------------------------------------------
	// Deplacement
	// ------------------------------------------------------------------

	/** Vitesse de marche au joystick (cm/s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;

	/** true = on avance dans la direction du regard. false = dans la direction de la manette gauche. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement")
	bool bWalkFollowsHead = true;

	/** Rayon du joueur pour les collisions avec les murs (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "1"))
	float PlayerCollisionRadius = 20.f;

	/** Hauteur max d'une marche que l'on peut monter en marchant (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "0"))
	float MaxStepHeight = 45.f;

	/** Rotation par crans ou continue. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement")
	EShowroomTurnMode TurnMode = EShowroomTurnMode::Smooth;

	/** Angle d'un cran de rotation (degres). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "1", ClampMax = "180"))
	float SnapTurnDegrees = 45.f;

	/** Vitesse de rotation continue (degres/s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "0"))
	float SmoothTurnSpeed = 90.f;

	/** Rotation continue : douceur du demarrage / de l'arret (petit = plus progressif, grand = plus direct). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "0.5"))
	float SmoothTurnAcceleration = 6.f;

	/** Zone morte du joystick de rotation (0..1). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Deplacement", meta = (ClampMin = "0", ClampMax = "1"))
	float TurnDeadZone = 0.5f;

	// ------------------------------------------------------------------
	// Teleportation
	// ------------------------------------------------------------------

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Teleportation")
	bool bTeleportEnabled = true;

	/** Vitesse de lancer de l'arc de teleportation (plus grand = plus loin). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Teleportation", meta = (ClampMin = "0"))
	float TeleportLaunchSpeed = 650.f;

	/** Rayon de l'arc (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Teleportation", meta = (ClampMin = "0"))
	float TeleportProjectileRadius = 3.6f;

	/** Distance de recherche du NavMesh autour du point vise. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Teleportation")
	FVector TeleportNavMeshExtent = FVector(500.f, 500.f, 500.f);

	// ------------------------------------------------------------------
	// Attraper des objets
	// ------------------------------------------------------------------

	/** Distance max entre la main et un objet pour pouvoir l'attraper (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Attraper", meta = (ClampMin = "0"))
	float GrabRadius = 6.f;

	// ------------------------------------------------------------------
	// Mains
	// ------------------------------------------------------------------

	/** Position/rotation du modele de main gauche par rapport a la manette (a ajuster si la main est mal placee). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mains")
	FTransform HandOffsetLeft = FTransform(FRotator(-25.f, 180.f, 90.f), FVector(-2.98f, -3.5f, 4.56f));

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mains")
	FTransform HandOffsetRight = FTransform(FRotator(25.f, 0.f, 90.f), FVector(-2.98f, 3.5f, 4.56f));

	/** L'AnimBP des mains n'a que des animations de main droite : on les inverse pour la gauche. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mains")
	bool bMirrorLeftHandAnimation = true;

	// ------------------------------------------------------------------
	// Fiche info (InfoMenu)
	// ------------------------------------------------------------------

	/** Distance entre les yeux et la fiche (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Fiche info", meta = (ClampMin = "20"))
	float InfoPanelDistance = 70.f;

	/** La fiche est placee de combien sous les yeux (cm) : ~25 = hauteur de la poitrine / des mains. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Fiche info")
	float InfoPanelHeightBelowEyes = 25.f;

	/** Vitesse a laquelle la fiche rattrape le joueur quand il tourne (petit = plus de retard, grand = plus colle). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Fiche info", meta = (ClampMin = "0.1"))
	float InfoPanelFollowSpeed = 4.f;

	/** Une fiche deplacee par le joueur revient devant lui s'il s'en eloigne de plus de (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Fiche info", meta = (ClampMin = "50"))
	float InfoPanelReleaseDistance = 400.f;

	/** Taille de la fiche : 0.05 = 50 cm de large. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Fiche info", meta = (ClampMin = "0.01"))
	float InfoPanelScale = 0.05f;

	// ------------------------------------------------------------------
	// Experience (bouton "Lancer l'experience" des fiches)
	// ------------------------------------------------------------------

	/** A quel point l'exterieur de la zone est assombri (0 = pas du tout, 1 = noir complet). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0", ClampMax = "1"))
	float ExperienceDarkness = 0.9f;

	/** Duree du fondu au noir / du retour a la lumiere (s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0"))
	float ExperienceFadeTime = 1.f;

	/** Couleur de l'assombrissement. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Experience")
	FLinearColor ExperienceDarknessColor = FLinearColor::Black;

	/** Marge entre la zone et le debut de l'assombrissement (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0"))
	float ExperienceDarknessMargin = 50.f;


	/** Duree de l'allumage / extinction des zones lumineuses de guidage (s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0"))
	float ExperienceSpotFadeTime = 0.6f;

	// ------------------------------------------------------------------
	// Zones d'experience
	// ------------------------------------------------------------------

	/** Couleur de la ligne au sol qui montre le contour des zones. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Zones")
	FLinearColor ZoneOutlineColor = FLinearColor(0.2f, 0.7f, 1.f, 1.f);

	// ------------------------------------------------------------------
	// Interfaces (Widget Blueprints modifiables dans l'editeur UMG)
	// ------------------------------------------------------------------

	/** La fiche info des zones (vide = mise en page C++ par defaut). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interfaces", meta = (AllowedClasses = "/Script/Enit.InfoPanelWidget"))
	TSoftClassPtr<UUserWidget> InfoPanelWidget;

	/** Le menu de main (bouton Menu). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interfaces", meta = (AllowedClasses = "/Script/Enit.HandMenuWidget"))
	TSoftClassPtr<UUserWidget> HandMenuWidget;

	/** L'etiquette au-dessus des objets mis en valeur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interfaces", meta = (AllowedClasses = "/Script/Enit.ExperienceLabelWidget"))
	TSoftClassPtr<UUserWidget> LabelWidget;

	/** La barre de prise sous les ecrans flottants. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interfaces", meta = (AllowedClasses = "/Script/Enit.FloatingScreenGrabBar"))
	TSoftClassPtr<UUserWidget> GrabBarWidget;

	// ------------------------------------------------------------------
	// Mise en valeur des objets (pendant une experience)
	// ------------------------------------------------------------------

	/** Couleur du projecteur et du halo qui mettent un objet en valeur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mise en valeur")
	FLinearColor HighlightColor = FLinearColor(1.f, 0.85f, 0.6f, 1.f);

	/** Puissance du projecteur au-dessus de l'objet (candelas). 0 = pas de projecteur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mise en valeur", meta = (ClampMin = "0"))
	float HighlightLightIntensity = 60.f;

	/** Afficher l'etiquette (nom + texte) au-dessus des objets mis en valeur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mise en valeur")
	bool bHighlightLabels = true;

	/** Duree de l'allumage / extinction d'une mise en valeur (s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mise en valeur", meta = (ClampMin = "0"))
	float HighlightFadeTime = 0.6f;

	// ------------------------------------------------------------------
	// Guidage visuel (pendant une experience)
	// ------------------------------------------------------------------

	/** Chemin lumineux anime au sol, du joueur vers la zone lumineuse a rejoindre. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage")
	bool bGuidePath = true;

	/** Distance entre deux points du chemin (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage", meta = (ClampMin = "10", EditCondition = "bGuidePath"))
	float GuidePathSpacing = 45.f;

	/** Vitesse de defilement des points vers la zone (cm/s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage", meta = (ClampMin = "0", EditCondition = "bGuidePath"))
	float GuidePathSpeed = 90.f;

	/** Fleche devant le joueur qui indique la zone (ou l'objet mis en valeur) quand il n'est pas dans son champ de vision. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage")
	bool bGuideArrow = true;

	/** Angle (degres) a partir duquel la zone est consideree hors du regard et la fleche apparait. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage", meta = (ClampMin = "5", ClampMax = "180", EditCondition = "bGuideArrow"))
	float GuideArrowAngle = 35.f;

	/** Onde lumineuse au sol quand une zone s'allume ou qu'un objet est mis en valeur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Guidage")
	bool bGuidePings = true;

	// ------------------------------------------------------------------
	// Sons (remplacables : choisir un autre son ici)
	// ------------------------------------------------------------------

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundExperienceStart;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundExperienceEnd;

	/** Passage a une nouvelle etape. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundStepChange;

	/** Un objet est mis en valeur (joue a la position de l'objet). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundHighlight;

	/** Une zone lumineuse s'allume (joue a la position de la zone). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundSpotActivate;

	/** Le joueur atteint la zone lumineuse demandee. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundSpotReached;

	/** Clic sur un bouton de la fiche. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundUIClick;

	/** Le laser survole un bouton / une vignette (tres discret). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundUIHover;

	/** Fond sonore en boucle pendant l'experience (vide = pas d'ambiance). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> SoundAmbience;

	/** Volume des effets sonores. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons", meta = (ClampMin = "0", ClampMax = "2"))
	float EffectsVolume = 0.8f;

	/** Volume du fond sonore. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons", meta = (ClampMin = "0", ClampMax = "2"))
	float AmbienceVolume = 0.35f;

	/** Volume du fond sonore pendant que le narrateur parle (pour bien l'entendre). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Sons", meta = (ClampMin = "0", ClampMax = "2"))
	float AmbienceVolumeDuringNarration = 0.15f;

	// ------------------------------------------------------------------
	// Narrateur
	// ------------------------------------------------------------------

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Narrateur")
	bool bNarrationEnabled = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Narrateur", meta = (ClampMin = "0", ClampMax = "2"))
	float NarrationVolume = 1.f;

	/** Attente entre le debut de l'etape et la voix (s) : laisse le temps aux objets d'apparaitre. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Narrateur", meta = (ClampMin = "0"))
	float NarrationDelay = 0.6f;

	/** Mode "Apres la narration" : attente entre la fin de la voix et l'etape suivante (s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Narrateur", meta = (ClampMin = "0"))
	float NarrationAdvanceDelay = 1.f;

	// ------------------------------------------------------------------
	// GPS (NavigatorSystem)
	// ------------------------------------------------------------------

	/** Intervalle de recalcul du chemin GPS (s). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS", meta = (ClampMin = "0.02"))
	float NavigatorRefreshInterval = 0.25f;

	/** Distance a laquelle un point GPS est considere comme atteint (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS", meta = (ClampMin = "0"))
	float NavigatorReachDistance = 150.f;

	/** On ne recalcule le chemin que si le joueur a bouge d'au moins cette distance (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS", meta = (ClampMin = "0"))
	float NavigatorMinMoveToRecalc = 10.f;

	/** Couleur de la ligne GPS. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS")
	FLinearColor NavigatorLineColor = FLinearColor(0.f, 0.6f, 1.f, 1.f);

	/** Epaisseur de la ligne GPS. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS", meta = (ClampMin = "0"))
	float NavigatorLineThickness = 3.f;

	/** Hauteur de la ligne GPS au-dessus du sol (cm). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS")
	float NavigatorLineHeight = 10.f;

	/** Quand le dernier point est atteint : true = on recommence au premier, false = le GPS s'arrete. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "GPS")
	bool bNavigatorLoop = false;

	// ------------------------------------------------------------------
	// Reseau
	// ------------------------------------------------------------------

	/** Nombre d'envois par seconde de la position tete/mains au serveur. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Reseau", meta = (ClampMin = "1", ClampMax = "120"))
	float TrackingSendRate = 30.f;

	/** Vitesse de lissage des mouvements des autres joueurs (plus grand = plus reactif). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Reseau", meta = (ClampMin = "1"))
	float RemoteSmoothing = 15.f;

	// UDeveloperSettings
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
