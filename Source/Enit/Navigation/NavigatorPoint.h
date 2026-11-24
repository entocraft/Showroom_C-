#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigatorPoint.generated.h"

class UStaticMeshComponent;

/**
 * Remplace BP_NavigatorPoint.
 * Une etape du parcours GPS. Place-en plusieurs dans le niveau et regle "Order"
 * (0 = premiere etape, 1 = deuxieme, ...).
 */
UCLASS()
class ENIT_API ANavigatorPoint : public AActor
{
	GENERATED_BODY()

public:
	ANavigatorPoint();

	/** Ordre de passage (utilise quand le NavigatorSystem collecte les points automatiquement). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	int32 Order = 0;

	/** Nom de l'etape (pratique pour l'afficher dans une UI). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	FText Label;

	/** Afficher un repere visible en jeu sur l'etape active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	bool bShowMarkerInGame = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Marker;

	/** Appele par le NavigatorSystem quand ce point devient (ou n'est plus) la destination. */
	UFUNCTION(BlueprintCallable, Category = "GPS")
	void SetActiveTarget(bool bActive);
};
