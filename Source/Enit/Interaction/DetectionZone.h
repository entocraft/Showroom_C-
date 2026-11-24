#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/InfoPanelWidget.h"
#include "Experience/ExperienceTypes.h"
#include "DetectionZone.generated.h"

class UBoxComponent;
class UChildActorComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AInfoMenu;
class AExperienceSpot;
class AExperienceGuide;
class AExperienceHighlight;
class AFloatingScreen;
class UAudioComponent;
class USplineComponent;
class UProceduralMeshComponent;

/** Forme d'une zone d'experience. */
UENUM(BlueprintType)
enum class EZoneShape : uint8
{
	/** Une boite (composant Box : la taille se regle avec Box Extent). */
	Box     UMETA(DisplayName = "Boite"),
	/** Une forme libre dessinee au sol avec les points du composant "Outline" (spline). */
	Drawn   UMETA(DisplayName = "Dessinee (points au sol)"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDetectionZoneSignature, APawn*, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExperienceStepSignature, int32, StepIndex);

/**
 * ZONE D'EXPERIENCE (remplace BP_DetectionZone).
 *
 * FORME : une boite, ou une forme LIBRE dessinee au sol (Shape = Dessinee) :
 * selectionner le composant "Outline" puis deplacer ses points ; Alt + glisser un point en cree un nouveau,
 * clic droit sur un point > Delete pour en retirer. La zone est visible dans le niveau (ligne au sol).
 *
 * Comme l'ancien Blueprint, chaque zone contient SA PROPRE fiche (composant "Menu").
 * Quand le joueur LOCAL entre dans la boite, la fiche s'affiche ; quand il sort, elle se cache.
 *
 * EXPERIENCE : la fiche a un bouton "Lancer l'experience". Il place le joueur au centre de la zone
 * et assombrit tout ce qui est hors de l'experience (une sphere sombre semi-transparente l'entoure).
 * L'experience est une suite d'ETAPES (liste "Steps"). Les objets sont DEJA dans le niveau :
 * a chaque etape, des objets sont mis en valeur (projecteur, halo, etiquette), des zones lumineuses
 * (AExperienceSpot) guident le joueur, le narrateur parle et la fiche affiche le texte de l'etape
 * avec les boutons Precedent / Suivant.
 * Un clic sur "Terminer l'experience" ou s'eloigner de l'experience eteint tout.
 *
 * Le texte / la photo se reglent directement sur la zone (Details > Zone > Info).
 */
UCLASS(meta = (DisplayName = "Zone d'experience"))
class ENIT_API ADetectionZone : public AActor
{
	GENERATED_BODY()

public:
	ADetectionZone();

	/** Contenu de la fiche affichee par cette zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FShowroomInfo Info;

	/**
	 * Optionnel : utiliser un autre panneau InfoMenu du niveau au lieu de celui de la zone
	 * (ex : un grand ecran partage par plusieurs zones). Le texte "Info" de la zone y est affiche.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	TObjectPtr<AInfoMenu> LinkedMenu;

	/** Boite, ou forme libre dessinee avec les points du composant Outline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	EZoneShape Shape = EZoneShape::Box;

	/** Hauteur de la zone dessinee (cm), au-dessus de ses points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "50", EditCondition = "Shape == EZoneShape::Drawn", EditConditionHides))
	float DrawnHeight = 300.f;

	/**
	 * Ou le joueur est place au lancement de l'experience (et ou le GPS l'amene).
	 * Deplacable dans le niveau avec le petit losange. (0,0,0) = centre de la zone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (MakeEditWidget = true))
	FVector StartPoint = FVector::ZeroVector;

	/** Ligne lumineuse au sol qui montre le contour de la zone en jeu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bShowOutline = true;

	/** La zone apparait dans les ecrans "Choisir une experience" (titre / sous-titre / photo = sa fiche Info). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	bool bShowInSelector = true;

	/** Ordre dans le selecteur (plus petit = plus haut). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	int32 SelectorOrder = 0;

	/** Le joueur local est-il dans la zone ? */
	UFUNCTION(BlueprintPure, Category = "Zone")
	bool IsPlayerInside() const { return bPlayerInside; }

	/** Un point est-il dans la zone (boite ou forme dessinee) ? */
	UFUNCTION(BlueprintPure, Category = "Zone")
	bool IsPointInside(const FVector& Point) const;

	/** Point au sol ou commence l'experience (StartPoint). */
	UFUNCTION(BlueprintPure, Category = "Zone")
	FVector GetStartPointOnFloor() const;

	/** Cacher le panneau quand le joueur sort de la zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bHideOnExit = true;

	// --- Experience ---

	/** Afficher le bouton "Lancer l'experience" sur la fiche. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	bool bHasExperience = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	FText StartButtonText = FText::FromString(TEXT("Lancer l'exp\u00E9rience"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	FText StopButtonText = FText::FromString(TEXT("Terminer l'exp\u00E9rience"));

	/**
	 * Les etapes de l'experience, jouees dans l'ordre.
	 * Liste vide = experience simple (juste le placement au centre et l'assombrissement).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience", meta = (TitleProperty = "Title"))
	TArray<FExperienceStep> Steps;

	/** Terminer automatiquement l'experience quand on passe la derniere etape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	bool bEndAfterLastStep = false;

	/** Etape en cours (-1 = pas d'experience). */
	UFUNCTION(BlueprintPure, Category = "Experience")
	int32 GetCurrentStep() const { return CurrentStep; }

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void NextStep();

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void PreviousStep();

	/** Aller directement a une etape (0 = premiere). */
	UFUNCTION(BlueprintCallable, Category = "Experience")
	void GoToStep(int32 StepIndex);

	/** Appele a chaque changement d'etape. */
	UPROPERTY(BlueprintAssignable, Category = "Experience")
	FExperienceStepSignature OnStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Experience")
	FDetectionZoneSignature OnExperienceStarted;

	UPROPERTY(BlueprintAssignable, Category = "Experience")
	FDetectionZoneSignature OnExperienceStopped;

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void StartExperience();

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void StopExperience();

	UFUNCTION(BlueprintCallable, Category = "Experience")
	void ToggleExperience();

	UFUNCTION(BlueprintPure, Category = "Experience")
	bool IsExperienceRunning() const { return bExperienceRunning; }

	/** Une experience est-elle en cours quelque part (n'importe quelle zone) ? */
	static bool IsAnyExperienceRunning();

	// --- Evenements ---

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FDetectionZoneSignature OnPlayerEnter;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FDetectionZoneSignature OnPlayerExit;

	// --- Composants ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UBoxComponent> Box;

	/** La fiche propre a cette zone (un InfoMenu). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UChildActorComponent> Menu;

	/** Contour d'une zone dessinee : deplacer ses points pour dessiner la zone (Shape = Dessinee). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<USplineComponent> Outline;

	/** Murs sombres le long du contour d'une zone dessinee (pendant l'experience). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UProceduralMeshComponent> Curtain;

	/** Ligne lumineuse au sol qui montre la zone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UProceduralMeshComponent> OutlineMesh;

	/** Sphere sombre qui entoure une zone "boite" pendant l'experience (vue de l'interieur, elle assombrit l'exterieur). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> DarknessShell;

	/** Le panneau effectivement utilise (LinkedMenu, sinon la fiche de la zone). */
	UFUNCTION(BlueprintPure, Category = "Zone")
	AInfoMenu* GetMenu() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Le joueur entre / sort de la zone. */
	void HandlePlayerEnter(APawn* Player);
	void HandlePlayerExit(APawn* Player);

private:
	/** Verifie regulierement si le joueur local est dans la zone. */
	void CheckPresence();

	/** Recalcule le polygone de la zone, les murs sombres et la ligne au sol. */
	void RebuildShape();

	/** Affiche / cache l'assombrissement (sphere ou murs selon la forme). */
	void SetDarknessVisible(bool bVisible);

	TArray<FVector2D> Polygon;   // contour au sol (monde)
	float FloorZ = 0.f;
	float ZoneHeight = 200.f;
	bool bPlayerInside = false;
	float OutsideTime = 0.f;     // temps passe hors d'une zone dessinee pendant l'experience
	float OutlineAlpha = 0.f;
	FTimerHandle PresenceTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CurtainMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMaterial;

	/** Pousse "Info" dans le panneau (aussi dans l'editeur, pour voir le resultat). */
	void PushInfoToMenu() const;

	/** Met a jour le bouton de la fiche ("Lancer" / "Terminer"). */
	void UpdateMenuButton() const;

	/** Place et dimensionne la sphere sombre autour de la boite. */
	void FitDarknessShell();


	// --- Etapes ---

	/** Repere les zones lumineuses des etapes. */
	void InitExperienceSpots();

	/** Applique l'etat d'une etape (-1 = rien) : zones lumineuses et objets mis en valeur. */
	void ApplyStep(int32 StepIndex);

	/** Eteint toutes les mises en valeur en cours. */
	void ClearHighlights();

	/** Met a jour la fiche (texte de l'etape, boutons). */
	void UpdateMenuForStep() const;

	void HandleSpotReached(AExperienceSpot* Spot);

	// --- Narrateur et sons ---

	void PlayNarration(int32 StepIndex);
	void StopNarration();

	UFUNCTION()
	void HandleNarrationFinished();

	/** Le guide (chemin au sol + fleche) vers la zone lumineuse ou l'objet de l'etape. */
	void UpdateGuide();

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> NarrationAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbienceAudio;

	UPROPERTY(Transient)
	TObjectPtr<AExperienceGuide> Guide;

	FTimerHandle NarrationTimer;
	FTimerHandle AdvanceTimer;

	/** Cache la fiche si le joueur n'est plus dans la zone. */
	void HideMenuIfOutside();

	TArray<TWeakObjectPtr<AExperienceSpot>> AllSpots;
	TArray<TWeakObjectPtr<AFloatingScreen>> AllScreens;

	/** Cache les ecrans des etapes (au lancement, une fois que tous les acteurs ont fait leur BeginPlay). */
	void HideStepScreens();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AExperienceHighlight>> ActiveHighlights;

	int32 CurrentStep = -1;
	float StepTime = 0.f;
	float ExperienceRadius = 0.f;  // le joueur qui s'eloigne au-dela termine l'experience


	bool bExperienceRunning = false;
	float DarknessAlpha = 0.f;   // opacite actuelle de la sphere (fondu)

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DarknessMaterial;
};
