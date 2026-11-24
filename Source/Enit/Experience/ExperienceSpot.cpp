#include "Experience/ExperienceSpot.h"
#include "Core/ShowroomSettings.h"
#include "Core/ShowroomAudio.h"
#include "Experience/ExperiencePing.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName OpacityParam(TEXT("Opacity"));
	const FName ColorParam(TEXT("Color"));

	constexpr float FloorOpacity = 0.75f;
	constexpr float ColumnOpacity = 0.12f;
	constexpr float TriggerHalfHeight = 100.f;

	UStaticMeshComponent* MakeShape(AActor* Owner, const TCHAR* Name, USceneComponent* Parent)
	{
		UStaticMeshComponent* Mesh = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetHiddenInGame(true);
		Mesh->SetTranslucentSortPriority(10); // devant la sphere sombre de l'experience
		return Mesh;
	}
}

AExperienceSpot::AExperienceSpot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // ne tourne que pendant les fondus / le pulse

	// Le pivot de l'acteur est AU SOL, au centre de la zone.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);

	Trigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetMobility(EComponentMobility::Movable);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetCanEverAffectNavigation(false);
	Trigger->ShapeColor = FColor(255, 160, 40);
	Trigger->SetLineThickness(2.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));

	Floor = MakeShape(this, TEXT("Floor"), Root);
	Column = MakeShape(this, TEXT("Column"), Root);
	for (UStaticMeshComponent* Mesh : { Floor.Get(), Column.Get() })
	{
		if (Cylinder.Succeeded()) Mesh->SetStaticMesh(Cylinder.Object);
		// Meme materiau que l'assombrissement : translucide, non eclaire, couleur + opacite.
		if (GlowMat.Succeeded()) Mesh->SetMaterial(0, GlowMat.Object);
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetCastShadows(false);
	Light->SetVisibility(false);

	ApplyShape();
}

float AExperienceSpot::GetBoundingRadius() const
{
	const float Height = bShowColumn ? ColumnHeight : TriggerHalfHeight * 2.f;
	return FVector2D(Radius, Height).Size();
}

void AExperienceSpot::ApplyShape()
{
	// Detection : une capsule posee au sol, haute de 2 m.
	Trigger->SetCapsuleSize(Radius, FMath::Max(TriggerHalfHeight, Radius));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, Trigger->GetUnscaledCapsuleHalfHeight()));

	const float Diameter = Radius * 2.f / 100.f; // le cylindre du moteur fait 100 x 100 cm
	Floor->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	Floor->SetRelativeScale3D(FVector(Diameter, Diameter, 0.01f));

	Column->SetRelativeLocation(FVector(0.f, 0.f, ColumnHeight * 0.5f));
	Column->SetRelativeScale3D(FVector(Diameter * 0.9f, Diameter * 0.9f, ColumnHeight / 100.f));

	Light->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Light->SetAttenuationRadius(Radius * 3.f);
	Light->SetLightColor(Color);
}

void AExperienceSpot::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShape();
}

void AExperienceSpot::BeginPlay()
{
	Super::BeginPlay();

	ApplyShape();

	if (UMaterialInterface* Mat = Floor->GetMaterial(0))
	{
		FloorMaterial = Floor->CreateDynamicMaterialInstance(0, Mat);
		ColumnMaterial = Column->CreateDynamicMaterialInstance(0, Mat);
	}
	ApplyBrightness(0.f);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AExperienceSpot::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AExperienceSpot::HandleEndOverlap);
}

void AExperienceSpot::SetState(EExperienceSpotState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	PulseTime = 0.f;
	PingTimer = 0.f;
	SetActorTickEnabled(true);

	// Elle s'allume : une onde au sol + un son, pour attirer l'oeil (et l'oreille) vers elle.
	if (NewState == EExperienceSpotState::Active)
	{
		SendPing();
		ShowroomAudio::PlayAt(this, UShowroomSettings::Get()->SoundSpotActivate, GetActorLocation(), UShowroomSettings::Get()->EffectsVolume);
	}
}

void AExperienceSpot::ApplyBrightness(float Brightness)
{
	const bool bVisible = Brightness > KINDA_SMALL_NUMBER;

	Floor->SetHiddenInGame(!bVisible);
	Column->SetHiddenInGame(!(bVisible && bShowColumn));
	Light->SetVisibility(bVisible && LightIntensity > 0.f);

	const FLinearColor Glow = Color * GlowStrength;
	if (FloorMaterial)
	{
		FloorMaterial->SetVectorParameterValue(ColorParam, Glow);
		FloorMaterial->SetScalarParameterValue(OpacityParam, FloorOpacity * Brightness);
	}
	if (ColumnMaterial)
	{
		ColumnMaterial->SetVectorParameterValue(ColorParam, Glow);
		ColumnMaterial->SetScalarParameterValue(OpacityParam, ColumnOpacity * Brightness);
	}
	Light->SetIntensity(LightIntensity * Brightness);
}

void AExperienceSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = State == EExperienceSpotState::Active ? 1.f
		: State == EExperienceSpotState::Done ? DoneBrightness
		: 0.f;

	const float FadeTime = UShowroomSettings::Get()->ExperienceSpotFadeTime;
	Alpha = FadeTime > 0.f ? FMath::FInterpConstantTo(Alpha, Target, DeltaSeconds, 1.f / FadeTime) : Target;

	// Pulse doux (entre 70 % et 100 %) pour la zone de l'etape en cours.
	const bool bPulsing = State == EExperienceSpotState::Active && bPulse;
	float Pulse = 1.f;
	if (bPulsing)
	{
		// Une onde de rappel de temps en temps, tant que le joueur n'y est pas.
		PingTimer += DeltaSeconds;
		if (PingInterval > 0.f && PingTimer >= PingInterval)
		{
			PingTimer = 0.f;
			if (!IsPlayerInside())
			{
				SendPing();
			}
		}
		PulseTime += DeltaSeconds;
		Pulse = 0.85f + 0.15f * FMath::Cos(PulseTime * UE_TWO_PI * 0.7f);
	}

	ApplyBrightness(Alpha * Pulse);

	if (!bPulsing && FMath::IsNearlyEqual(Alpha, Target))
	{
		SetActorTickEnabled(false);
	}
}

void AExperienceSpot::SendPing() const
{
	if (UShowroomSettings::Get()->bGuidePings)
	{
		AExperiencePing::Spawn(GetWorld(), GetActorLocation(), Radius * 0.6f, Radius * 3.f, Color, 1.4f);
		AExperiencePing::Spawn(GetWorld(), GetActorLocation(), Radius * 0.6f, Radius * 2.f, Color, 1.2f, 0.35f);
	}
}

void AExperienceSpot::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled() || OverlapCount++ > 0)
	{
		return;
	}
	OnPlayerReached.Broadcast(this);
}

void AExperienceSpot::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsLocallyControlled() && OverlapCount > 0)
	{
		--OverlapCount;
	}
}
