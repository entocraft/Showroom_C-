#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandMenu.generated.h"

class UWidgetComponent;

/**
 * Remplace BP_Menu : le panneau 3D du menu de main (contient un UHandMenuWidget).
 * Cree par le pawn quand on appuie sur le bouton Menu / B, attache a la manette.
 */
UCLASS()
class ENIT_API AHandMenu : public AActor
{
	GENERATED_BODY()

public:
	AHandMenu();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> Widget;

protected:
	/** Choisit le Widget Blueprint du menu (Project Settings > Showroom > Interfaces). */
	virtual void PreInitializeComponents() override;
};
