#include "Interaction/ShowroomGrabComponent.h"
#include "Components/PrimitiveComponent.h"
#include "MotionControllerComponent.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

UShowroomGrabComponent::UShowroomGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Petite vibration par defaut quand on attrape (asset du template).
	static ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> Haptic(TEXT("/Game/XRFramework/Haptics/GrabHapticEffect.GrabHapticEffect"));
	if (Haptic.Succeeded()) GrabHapticEffect = Haptic.Object;
}

void UShowroomGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* Prim = GetGrabbedPrimitive())
	{
		bSimulateOnDrop = Prim->IsAnySimulatingPhysics();
		if (bSimulateOnDrop)
		{
			Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
		}
	}
}

UPrimitiveComponent* UShowroomGrabComponent::GetGrabbedPrimitive() const
{
	return Cast<UPrimitiveComponent>(GetAttachParent());
}

void UShowroomGrabComponent::SetPrimitivePhysics(bool bSimulate) const
{
	if (UPrimitiveComponent* Prim = GetGrabbedPrimitive())
	{
		Prim->SetSimulatePhysics(bSimulate);
	}
}

bool UShowroomGrabComponent::TryGrab(UMotionControllerComponent* Hand)
{
	if (!Hand || GrabType == EShowroomGrabType::None)
	{
		return false;
	}

	// Si une autre main tenait deja l'objet, elle le lache.
	if (bIsHeld)
	{
		TryRelease();
	}

	UPrimitiveComponent* Prim = GetGrabbedPrimitive();

	switch (GrabType)
	{
	case EShowroomGrabType::Free:
		if (!Prim) return false;
		SetPrimitivePhysics(false);
		Prim->AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform);
		break;

	case EShowroomGrabType::Snap:
	{
		if (!Prim) return false;
		SetPrimitivePhysics(false);
		// On place l'objet pour que CE composant tombe exactement sur la main.
		// (on garde l'echelle actuelle de l'objet)
		FTransform Target = Hand->GetComponentTransform();
		Target.SetScale3D(GetComponentScale());
		const FTransform NewWorld = GetRelativeTransform().Inverse() * Target;
		Prim->SetWorldTransform(NewWorld, false, nullptr, ETeleportType::TeleportPhysics);
		Prim->AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform);
		break;
	}

	case EShowroomGrabType::Custom:
	default:
		break;
	}

	bIsHeld = true;
	HeldBy = Hand;

	// Vibration dans la manette du joueur local.
	if (GrabHapticEffect)
	{
		if (const APawn* Pawn = Cast<APawn>(Hand->GetOwner()))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()); PC && PC->IsLocalController())
			{
				const EControllerHand Side = Hand->MotionSource.ToString().StartsWith(TEXT("Left")) ? EControllerHand::Left : EControllerHand::Right;
				PC->PlayHapticEffect(GrabHapticEffect, Side);
			}
		}
	}

	OnGrabbed.Broadcast(Hand);
	return true;
}

bool UShowroomGrabComponent::TryRelease()
{
	if (!bIsHeld)
	{
		return false;
	}

	if (GrabType == EShowroomGrabType::Free || GrabType == EShowroomGrabType::Snap)
	{
		if (UPrimitiveComponent* Prim = GetGrabbedPrimitive())
		{
			Prim->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			if (bSimulateOnDrop)
			{
				Prim->SetSimulatePhysics(true);
			}
		}
	}

	bIsHeld = false;
	HeldBy = nullptr;
	OnDropped.Broadcast();
	return true;
}
