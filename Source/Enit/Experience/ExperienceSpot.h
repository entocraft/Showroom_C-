#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExperienceSpot.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EExperienceSpotState : uint8
{
	Off,     // eteinte (invisible)
	Active,  // etape en cours : allumee et pulse doucement
	Done,    // etape passee : reste faiblement allumee
};

/**
 * Zone lumineuse au sol qui guide le joueur pendant une experience.
 * A poser dans le niveau, puis a ajouter dans la liste "Spots" d'une etape de la DetectionZone.
 *
 * Invisible en jeu tant qu'elle n'est pas allumee. Dans l'editeur, le cylindre orange montre sa taille.
 * Quand le joueur entre dedans, elle previent l'experience (mode "Atteindre la zone lumineuse").
 */
UCLASS()
class ENIT_API AExperienceSpot : public AActor
{
	GENERATED_BODY()

public:
	AExperienceSpot();

	/** Rayon de la zone au sol (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "10"))
	float Radius = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse")
	FLinearColor Color = FLinearColor(1.f, 0.65f, 0.2f);

	/** Brillance du disque au sol (au-dela de 1, il "brille" avec le bloom). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "0"))
	float GlowStrength = 4.f;

	/** Colonne de lumiere verticale, visible de loin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse")
	bool bShowColumn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "10", EditCondition = "bShowColumn"))
	float ColumnHeight = 250.f;

	/** Lumiere qui eclaire les objets poses dans la zone (candelas). 0 = pas de lumiere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "0"))
	float LightIntensity = 20.f;

	/** Luminosite d'une zone d'une etape deja passee (0 = eteinte, 1 = comme l'etape en cours). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "0", ClampMax = "1"))
	float DoneBrightness = 0.2f;

	/** L'etape en cours pulse doucement pour attirer l'oeil. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse")
	bool bPulse = true;

	/** Une onde lumineuse part de la zone toutes les X secondes tant que le joueur ne l'a pas rejointe (0 = jamais). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone lumineuse", meta = (ClampMin = "0"))
	float PingInterval = 3.f;

	UFUNCTION(BlueprintCallable, Category = "Zone lumineuse")
	void SetState(EExperienceSpotState NewState);

	UFUNCTION(BlueprintPure, Category = "Zone lumineuse")
	EExperienceSpotState GetState() const { return State; }

	/** Le joueur local est-il dans la zone ? */
	UFUNCTION(BlueprintPure, Category = "Zone lumineuse")
	bool IsPlayerInside() const { return OverlapCount > 0; }

	/** Rayon d'une sphere qui contient toute la zone (sol + colonne). */
	float GetBoundingRadius() const;

	/** Appele quand le joueur local entre dans la zone. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpotReached, AExperienceSpot*);
	FOnSpotReached OnPlayerReached;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UCapsuleComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Column;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UPointLightComponent> Light;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void ApplyShape();
	void ApplyBrightness(float Brightness);
	void SendPing() const;

	EExperienceSpotState State = EExperienceSpotState::Off;
	float Alpha = 0.f;       // luminosite actuelle (fondu)
	float PulseTime = 0.f;
	float PingTimer = 0.f;
	int32 OverlapCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ColumnMaterial;
};
