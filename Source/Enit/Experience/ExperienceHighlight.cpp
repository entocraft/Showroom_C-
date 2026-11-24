#include "Experience/ExperienceHighlight.h"
#include "Experience/ExperiencePing.h"
#include "UI/ExperienceLabelWidget.h"
#include "Core/ShowroomSettings.h"
#include "Core/ShowroomAudio.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float LightHeightAbove = 160.f;  // hauteur du projecteur au-dessus de l'objet (cm)
	constexpr float LabelGap = 20.f;           // espace entre le haut de l'objet et l'etiquette (cm)
	constexpr float HaloMargin = 1.35f;        // halo un peu plus grand que l'objet
}

AExperienceHighlight::AExperienceHighlight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // l'etiquette se tourne vers la tete apres son mouvement
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);

	Light = CreateDefaultSubobject<USpotLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetUsingAbsoluteLocation(true);
	Light->SetUsingAbsoluteRotation(true);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetCastShadows(true);
	Light->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));
	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	Halo->SetupAttachment(Root);
	Halo->SetMobility(EComponentMobility::Movable);
	Halo->SetUsingAbsoluteLocation(true);
	Halo->SetUsingAbsoluteRotation(true);
	Halo->SetUsingAbsoluteScale(true);
	Halo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Halo->SetCastShadow(false);
	Halo->SetCanEverAffectNavigation(false);
	Halo->SetTranslucentSortPriority(10); // devant la sphere sombre de l'experience
	Halo->SetVisibility(false);
	if (Cylinder.Succeeded()) Halo->SetStaticMesh(Cylinder.Object);
	if (GlowMat.Succeeded()) Halo->SetMaterial(0, GlowMat.Object);

	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetUsingAbsoluteLocation(true);
	Label->SetUsingAbsoluteRotation(true);
	Label->SetUsingAbsoluteScale(true);
	Label->SetWidgetSpace(EWidgetSpace::World);
	Label->SetWidgetClass(UExperienceLabelWidget::StaticClass());
	Label->SetDrawAtDesiredSize(true);
	Label->SetPivot(FVector2D(0.5f, 1.f)); // pose sur l'objet par le bas
	Label->SetTwoSided(true);
	Label->SetWorldScale3D(FVector(0.05f)); // lisible a 1-2 m
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision); // le laser passe a travers
	Label->SetTranslucentSortPriority(11);
	Label->SetVisibility(false);
}

void AExperienceHighlight::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	Label->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->LabelWidget, UExperienceLabelWidget::StaticClass()));
}

AExperienceHighlight* AExperienceHighlight::Spawn(UWorld* World, const FExperienceHighlightInfo& Highlight, float InDelay)
{
	if (!World || !Highlight.Actor)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AExperienceHighlight* Result = World->SpawnActor<AExperienceHighlight>(Highlight.Actor->GetActorLocation(), FRotator::ZeroRotator, Params);
	if (Result)
	{
		Result->Target = Highlight.Actor;
		Result->LabelText = Highlight.Label;
		Result->DetailsText = Highlight.Details;
		Result->Delay = InDelay;
	}
	return Result;
}

void AExperienceHighlight::Dismiss()
{
	bDismissed = true;
	if (!bStarted)
	{
		Destroy(); // pas encore commence : rien a eteindre
	}
}

bool AExperienceHighlight::GetTargetBounds(FVector& OutCenter, FVector& OutExtent) const
{
	const AActor* Actor = Target.Get();
	if (!Actor)
	{
		return false;
	}
	const FBox Box = Actor->GetComponentsBoundingBox(/*bNonColliding*/ true);
	if (!Box.IsValid)
	{
		return false;
	}
	Box.GetCenterAndExtents(OutCenter, OutExtent);
	return true;
}

void AExperienceHighlight::Begin()
{
	bStarted = true;
	const UShowroomSettings* Settings = UShowroomSettings::Get();

	if (UExperienceLabelWidget* Widget = Cast<UExperienceLabelWidget>(Label->GetUserWidgetObject()))
	{
		Widget->SetTexts(LabelText, DetailsText);
		Widget->PlayIntro();
	}

	Light->SetLightColor(Settings->HighlightColor);
	HaloMaterial = Halo->CreateDynamicMaterialInstance(0);
	Follow();

	// Un petit signal pour attirer l'oeil et l'oreille vers l'objet.
	FVector Center, Extent;
	if (GetTargetBounds(Center, Extent))
	{
		const FVector Base(Center.X, Center.Y, Center.Z - Extent.Z);
		const float Radius = FMath::Clamp(FVector2D(Extent.X, Extent.Y).Size(), 15.f, 200.f);
		if (Settings->bGuidePings)
		{
			AExperiencePing::Spawn(GetWorld(), Base, Radius * 0.6f, Radius * 2.2f, Settings->HighlightColor, 1.1f);
		}
		ShowroomAudio::PlayAt(this, Settings->SoundHighlight, Center, Settings->EffectsVolume);
	}
}

void AExperienceHighlight::Follow()
{
	FVector Center, Extent;
	if (!GetTargetBounds(Center, Extent))
	{
		return;
	}
	const float Radius = FMath::Max(FVector2D(Extent.X, Extent.Y).Size(), 10.f) * HaloMargin;
	const float Bottom = Center.Z - Extent.Z;
	const float Top = Center.Z + Extent.Z;

	// Halo au sol, sous l'objet
	const float Diameter = Radius * 2.f / 100.f; // cylindre du moteur : 100 cm
	Halo->SetWorldLocation(FVector(Center.X, Center.Y, Bottom + 1.f));
	Halo->SetWorldScale3D(FVector(Diameter, Diameter, 0.005f));

	// Projecteur au-dessus, qui eclaire l'objet et le sol autour
	const FVector LightPos(Center.X, Center.Y, Top + LightHeightAbove);
	const float Distance = LightPos.Z - Bottom;
	const float Cone = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Radius, Distance)) + 4.f, 8.f, 70.f);
	Light->SetWorldLocationAndRotation(LightPos, FRotator(-90.f, 0.f, 0.f));
	Light->SetOuterConeAngle(Cone);
	Light->SetInnerConeAngle(Cone * 0.6f);
	Light->SetAttenuationRadius(Distance * 1.6f);

	// Etiquette au-dessus, tournee vers le joueur (seulement autour de la verticale : reste bien droite)
	const FVector LabelPos(Center.X, Center.Y, Top + LabelGap);
	FRotator Facing = FRotator::ZeroRotator;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
		const FVector Eyes = Camera ? Camera->GetComponentLocation() : Player->GetPawnViewLocation();
		Facing = FRotator(0.f, (Eyes - LabelPos).Rotation().Yaw, 0.f);
	}
	Label->SetWorldLocationAndRotation(LabelPos, Facing);
}

void AExperienceHighlight::Apply(float Brightness)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	const bool bVisible = Brightness > KINDA_SMALL_NUMBER;

	Light->SetVisibility(bVisible);
	Light->SetIntensity(Settings->HighlightLightIntensity * Brightness);

	Halo->SetVisibility(bVisible);
	if (HaloMaterial)
	{
		const float Pulse = 0.8f + 0.2f * FMath::Cos(Time * UE_TWO_PI * 0.5f);
		HaloMaterial->SetVectorParameterValue(TEXT("Color"), Settings->HighlightColor * 2.5f);
		HaloMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.35f * Brightness * Pulse);
	}

	const bool bShowLabel = bVisible && Settings->bHighlightLabels && !LabelText.IsEmpty();
	Label->SetVisibility(bShowLabel);
	Label->SetTintColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Brightness));
}

void AExperienceHighlight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Target.IsValid())
	{
		Destroy();
		return;
	}
	if (!bStarted)
	{
		Delay -= DeltaSeconds;
		if (Delay > 0.f)
		{
			return;
		}
		Begin();
	}

	Time += DeltaSeconds;
	const float FadeTime = UShowroomSettings::Get()->HighlightFadeTime;
	const float Wanted = bDismissed ? 0.f : 1.f;
	Alpha = FadeTime > 0.f ? FMath::FInterpConstantTo(Alpha, Wanted, DeltaSeconds, 1.f / FadeTime) : Wanted;

	Follow();
	Apply(Alpha);

	if (bDismissed && Alpha <= 0.f)
	{
		Destroy();
	}
}
