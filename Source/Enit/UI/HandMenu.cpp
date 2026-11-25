#include "UI/HandMenu.h"
#include "UI/HandMenuWidget.h"
#include "Core/ShowroomSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AHandMenu::AHandMenu()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // apres le mouvement des mains et de la tete : pas de tremblement

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetWidgetClass(UHandMenuWidget::StaticClass());
	Widget->SetDrawSize(FVector2D(560.f, 760.f)); // menu + page Sessions (liste des sessions)
	Widget->SetPivot(FVector2D(0.5f, 1.f));        // pose par le BAS : le panneau monte au-dessus de la main
	Widget->SetTwoSided(true);

	// ~22 cm de large.
	Widget->SetRelativeScale3D(FVector(0.04f));

	// Le laser des manettes trace sur le canal "3DWidget".
	Widget->SetCollisionProfileName(TEXT("UI"));
	Widget->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AHandMenu::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	Widget->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->HandMenuWidget, UHandMenuWidget::StaticClass()));
}

void AHandMenu::FollowHand(USceneComponent* InHand)
{
	Hand = InHand;
	FVector Location;
	FQuat Rotation;
	if (ComputeTarget(Location, Rotation))
	{
		SetActorLocationAndRotation(Location, Rotation); // apparait directement au bon endroit
	}
}

bool AHandMenu::ComputeTarget(FVector& OutLocation, FQuat& OutRotation) const
{
	const USceneComponent* HandComp = Hand.Get();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!HandComp || !Player)
	{
		return false;
	}
	const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
	const FVector Eyes = Camera ? Camera->GetComponentLocation() : Player->GetPawnViewLocation();
	const FVector HandPos = HandComp->GetComponentLocation();

	// Au-dessus de la main, un peu en avant (cote oppose aux yeux) pour ne pas la cacher.
	const FVector Away = (HandPos - Eyes).GetSafeNormal2D();
	OutLocation = HandPos + FVector(0.f, 0.f, HeightAboveHand) + Away * ForwardOffset;

	// Face aux yeux (la face du widget est son axe +X), sans roulis : toujours droit.
	FRotator Facing = (Eyes - OutLocation).Rotation();
	Facing.Roll = 0.f;
	OutRotation = Facing.Quaternion();
	return true;
}

void AHandMenu::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector TargetLocation;
	FQuat TargetRotation;
	if (!ComputeTarget(TargetLocation, TargetRotation))
	{
		return;
	}
	// Suit la main avec un leger retard (doux, sans trembler) ; saut direct apres une teleportation.
	const bool bFar = FVector::DistSquared(GetActorLocation(), TargetLocation) > FMath::Square(80.f);
	const float Alpha = bFar ? 1.f : 1.f - FMath::Exp(-FollowSpeed * DeltaSeconds);
	SetActorLocationAndRotation(
		FMath::Lerp(GetActorLocation(), TargetLocation, Alpha),
		FQuat::Slerp(GetActorQuat(), TargetRotation, bFar ? 1.f : FMath::Min(1.f, Alpha * 0.8f)));
}
