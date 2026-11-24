#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/InfoPanelWidget.h"
#include "UI/ScreenDrag.h"
#include "InfoMenu.generated.h"

class UWidgetComponent;
class UInfoPanelWidget;

/**
 * Remplace BP_InfoMenu.
 * Un panneau 3D (fiche info) cache au depart, affiche par une ADetectionZone.
 * On clique dessus avec le laser des manettes (gachette).
 *
 * Quand il est visible et que "Follow Player" est coche (par defaut) :
 *   - il se place DEVANT le joueur, un peu sous les yeux (hauteur des mains),
 *   - il est incline vers le regard,
 *   - il suit la rotation du joueur avec un leger retard amorti (il "suit" sans etre "colle").
 * Distance, hauteur, taille et vitesse de suivi : Project Settings > Game > Showroom > Fiche info.
 *
 * DEPLACABLE : une barre arrondie sous la fiche ; viser + gachette maintenue = deplacer la fiche.
 * Une fois posee, elle reste a cet endroit (tournee vers le joueur) jusqu'a ce que le joueur s'eloigne
 * (InfoPanelReleaseDistance) ou que la fiche se rouvre : elle revient alors devant lui.
 */
UCLASS()
class ENIT_API AInfoMenu : public AActor, public IShowroomDraggable
{
	GENERATED_BODY()

public:
	AInfoMenu();

	/** Le texte / la photo de la fiche. Modifiable directement dans le panneau Details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	FShowroomInfo Info;

	/** true = le panneau vient devant le joueur et le suit. false = il reste a sa place et se tourne vers le joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	bool bFollowPlayer = true;

	/** Le panneau est visible au lancement du jeu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	bool bVisibleAtStart = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> Widget;

	/** Barre de prise sous la fiche (pour la deplacer). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> GrabBar;

	virtual void BeginDrag() override;
	virtual void EndDrag() override;

	/** La fiche a ete deplacee par le joueur et reste a sa place. */
	UFUNCTION(BlueprintPure, Category = "Fiche")
	bool IsAnchored() const { return bAnchored; }

	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void ShowMenu(bool bShow);

	UFUNCTION(BlueprintPure, Category = "Fiche")
	bool IsMenuVisible() const { return bMenuVisible; }

	/** Change le contenu de la fiche (et met a jour l'affichage). */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetInfo(const FShowroomInfo& NewInfo);

	UFUNCTION(BlueprintPure, Category = "Fiche")
	UInfoPanelWidget* GetPanel() const;

	/** Bouton d'action en bas de la fiche (utilise par la DetectionZone pour "Lancer l'experience"). */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetActionButton(bool bVisible, const FText& Label, bool bIsStop = false);

	/** Navigation des etapes d'une experience (utilise par la DetectionZone). */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetStepNav(bool bVisible, const FText& StepLabel, bool bCanPrevious, bool bCanNext, int32 StepIndex = -1, int32 StepCount = 0);

	/** La DetectionZone qui affiche actuellement ce panneau (utile quand plusieurs zones partagent un panneau). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Fiche")
	TObjectPtr<AActor> ActiveZone;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

private:
	/** Clic sur le bouton de la fiche : transmis a la zone qui l'affiche. */
	UFUNCTION()
	void HandleActionClicked();

	UFUNCTION()
	void HandlePreviousClicked();

	UFUNCTION()
	void HandleNextClicked();

	/** Ou le panneau DEVRAIT etre (devant le joueur) et son orientation (tourne vers les yeux). */
	bool ComputeTargetTransform(FVector& OutLocation, FQuat& OutRotation) const;

	/** Orientation qui tourne la face du panneau vers les yeux. */
	static FQuat FaceTowards(const FVector& From, const FVector& Eyes);

	bool bMenuVisible = false;
	bool bAnchored = false;
	FScreenDrag Drag;
};
