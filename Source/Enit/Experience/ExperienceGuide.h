#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExperienceGuide.generated.h"

class AExperienceSpot;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Guide visuel (cree par la DetectionZone pendant l'experience) :
 *   - vers une zone lumineuse a rejoindre : un chemin de points lumineux au sol qui defilent
 *     du joueur vers la zone (suit le NavMesh) + une fleche si la zone est hors du champ de vision,
 *   - vers un objet mis en valeur : seulement la fleche, quand l'objet est hors du champ de vision.
 * Reglages : Project Settings > Game > Showroom > Guidage.
 */
UCLASS(NotPlaceable)
class ENIT_API AExperienceGuide : public AActor
{
	GENERATED_BODY()

public:
	AExperienceGuide();

	/** La zone vers laquelle guider (nullptr = plus de guidage, tout s'efface). */
	void SetTarget(AExperienceSpot* Spot);

	/** Un objet a regarder (fleche seulement). */
	void SetLookTarget(AActor* Actor, const FLinearColor& InColor);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdatePath(float DeltaSeconds, const FVector& Feet);
	void UpdateArrow(float DeltaSeconds, const FVector& Head, const FRotator& View);
	void RecalcPath(const FVector& Feet);

	/** Point du chemin a une distance donnee depuis le debut. */
	FVector PointAlongPath(float Distance) const;

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<UInstancedStaticMeshComponent> Dots;

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Arrow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DotsMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArrowMaterial;

	TWeakObjectPtr<AExperienceSpot> Target;
	TWeakObjectPtr<AActor> LookTarget;
	FLinearColor Color = FLinearColor::White;

	TArray<FVector> Path;
	TArray<float> PathDistances; // distance cumulee a chaque point
	float PathLength = 0.f;
	float RecalcTimer = 0.f;
	float Flow = 0.f;            // defilement des points

	float PathAlpha = 0.f;       // fondu du chemin
	float ArrowAlpha = 0.f;      // fondu de la fleche
	float ArrowTime = 0.f;
	FVector ArrowLocation = FVector::ZeroVector;
	bool bArrowPlaced = false;
};
