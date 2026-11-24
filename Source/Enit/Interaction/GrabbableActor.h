#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrabbableActor.generated.h"

class UStaticMeshComponent;
class UShowroomGrabComponent;

/**
 * Remplace BP_Grabbable_SmallCube.
 * Un objet physique repliqué qu'on peut attraper. Glisse-le dans le niveau,
 * puis change le Mesh / le materiau / la taille dans le panneau Details.
 */
UCLASS()
class ENIT_API AGrabbableActor : public AActor
{
	GENERATED_BODY()

public:
	AGrabbableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UShowroomGrabComponent> Grab;
};
