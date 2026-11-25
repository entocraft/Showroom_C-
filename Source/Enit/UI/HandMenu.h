#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandMenu.generated.h"

class UWidgetComponent;

/**
 * Le panneau 3D du menu de main (contient un UHandMenuWidget), cree par le pawn (bouton Menu / Y / B).
 *
 * Il n'est PAS colle a la manette : il flotte AU-DESSUS de la main, la suit en douceur, se tourne vers
 * les yeux et reste bien droit (il ne tourne pas avec le poignet). On le vise avec le laser de l'autre main.
 */
UCLASS()
class ENIT_API AHandMenu : public AActor
{
	GENERATED_BODY()

public:
	AHandMenu();

	/** La main a suivre (le menu se place tout de suite au-dessus). */
	void FollowHand(USceneComponent* Hand);

	/** Hauteur du bas du panneau au-dessus de la main (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	float HeightAboveHand = 10.f;

	/** Decalage vers l'avant (en s'eloignant des yeux), pour ne pas masquer la main (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	float ForwardOffset = 6.f;

	/** Vitesse a laquelle le menu rattrape la main (petit = plus de retard, plus doux). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu", meta = (ClampMin = "1"))
	float FollowSpeed = 12.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> Widget;

protected:
	/** Choisit le Widget Blueprint du menu (Project Settings > Showroom > Interfaces). */
	virtual void PreInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool ComputeTarget(FVector& OutLocation, FQuat& OutRotation) const;

	TWeakObjectPtr<USceneComponent> Hand;
};
