#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ShowroomGrabComponent.generated.h"

class UMotionControllerComponent;
class UHapticFeedbackEffect_Base;
class UPrimitiveComponent;

/** Comment l'objet se colle a la main. */
UENUM(BlueprintType)
enum class EShowroomGrabType : uint8
{
	None,
	/** L'objet garde la position ou on l'a attrape. */
	Free,
	/** L'objet se place pour que ce composant soit exactement dans la main. */
	Snap,
	/** Rien n'est attache : a toi de gerer via OnGrabbed / OnDropped. */
	Custom
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowroomGrabSignature, UMotionControllerComponent*, Hand);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowroomDropSignature);

/**
 * Remplace BP_GrabComponent.
 *
 * Utilisation : ajouter ce composant comme ENFANT du mesh (physique) qu'on veut pouvoir attraper.
 * Le pawn cherche le composant le plus proche de la main dans un rayon (reglage GrabRadius).
 */
UCLASS(ClassGroup = (Showroom), meta = (BlueprintSpawnableComponent))
class ENIT_API UShowroomGrabComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UShowroomGrabComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attraper")
	EShowroomGrabType GrabType = EShowroomGrabType::Free;

	/** Vibration jouee dans la manette quand on attrape l'objet (optionnel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attraper")
	TObjectPtr<UHapticFeedbackEffect_Base> GrabHapticEffect;

	UPROPERTY(BlueprintAssignable, Category = "Attraper")
	FShowroomGrabSignature OnGrabbed;

	UPROPERTY(BlueprintAssignable, Category = "Attraper")
	FShowroomDropSignature OnDropped;

	/** Essaie d'attacher l'objet a la main. Renvoie true si reussi. */
	UFUNCTION(BlueprintCallable, Category = "Attraper")
	bool TryGrab(UMotionControllerComponent* Hand);

	/** Lache l'objet. Renvoie true si reussi. */
	UFUNCTION(BlueprintCallable, Category = "Attraper")
	bool TryRelease();

	UFUNCTION(BlueprintPure, Category = "Attraper")
	bool IsHeld() const { return bIsHeld; }

	UFUNCTION(BlueprintPure, Category = "Attraper")
	UMotionControllerComponent* GetHeldByHand() const { return HeldBy; }

	/** Le mesh physique auquel ce composant est attache. */
	UFUNCTION(BlueprintPure, Category = "Attraper")
	UPrimitiveComponent* GetGrabbedPrimitive() const;

protected:
	virtual void BeginPlay() override;

private:
	void SetPrimitivePhysics(bool bSimulate) const;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> HeldBy;

	bool bIsHeld = false;

	/** Si l'objet simulait la physique au debut, il retombera quand on le lache. */
	bool bSimulateOnDrop = false;
};
