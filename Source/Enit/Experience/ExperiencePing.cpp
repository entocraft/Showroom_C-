#include "Experience/ExperiencePing.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AExperiencePing::AExperiencePing()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	RootComponent = Disc;
	Disc->SetMobility(EComponentMobility::Movable);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);
	Disc->SetCanEverAffectNavigation(false);
	Disc->SetTranslucentSortPriority(10); // devant la sphere sombre de l'experience

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));
	if (Cylinder.Succeeded()) Disc->SetStaticMesh(Cylinder.Object);
	if (GlowMat.Succeeded()) Disc->SetMaterial(0, GlowMat.Object);
}

void AExperiencePing::Spawn(UWorld* World, const FVector& Location, float InStartRadius, float InEndRadius, const FLinearColor& InColor, float InDuration, float Delay)
{
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AExperiencePing* Ping = World->SpawnActor<AExperiencePing>(Location + FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator, Params);
	if (!Ping)
	{
		return;
	}
	Ping->StartRadius = InStartRadius;
	Ping->EndRadius = InEndRadius;
	Ping->Duration = FMath::Max(InDuration, 0.05f);
	Ping->Color = InColor;
	Ping->Time = -Delay; // un temps negatif = attente avant de commencer
	Ping->Material = Ping->Disc->CreateDynamicMaterialInstance(0);
	Ping->Disc->SetVisibility(false);
}

void AExperiencePing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Time += DeltaSeconds;
	if (Time < 0.f)
	{
		return;
	}
	const float T = Time / Duration;
	if (T >= 1.f)
	{
		Destroy();
		return;
	}

	// S'agrandit vite puis ralentit, et s'efface.
	const float Grow = 1.f - FMath::Square(1.f - T);
	const float Radius = FMath::Lerp(StartRadius, EndRadius, Grow);
	const float Diameter = Radius * 2.f / 100.f; // cylindre du moteur : 100 cm
	Disc->SetVisibility(true);
	Disc->SetWorldScale3D(FVector(Diameter, Diameter, 0.005f));

	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color * 3.f);
		Material->SetScalarParameterValue(TEXT("Opacity"), 0.45f * FMath::Square(1.f - T));
	}
}
