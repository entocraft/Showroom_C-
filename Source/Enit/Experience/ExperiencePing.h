#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExperiencePing.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Onde lumineuse au sol : un disque qui s'agrandit en s'effacant, puis disparait.
 * Utilisee pour attirer l'oeil quand une zone s'allume ou qu'un objet apparait.
 * Creee par le code (pas besoin de la poser dans le niveau).
 */
UCLASS(NotPlaceable)
class ENIT_API AExperiencePing : public AActor
{
	GENERATED_BODY()

public:
	AExperiencePing();

	/** Lance une onde au sol a Location (rayons en cm). */
	static void Spawn(UWorld* World, const FVector& Location, float StartRadius, float EndRadius, const FLinearColor& Color, float Duration = 1.2f, float Delay = 0.f);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	float StartRadius = 50.f;
	float EndRadius = 150.f;
	float Duration = 1.2f;
	float Time = 0.f;
	FLinearColor Color = FLinearColor::White;
};
