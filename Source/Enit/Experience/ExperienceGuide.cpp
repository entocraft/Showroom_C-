#include "Experience/ExperienceGuide.h"
#include "Experience/ExperienceSpot.h"
#include "Core/ShowroomSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName OpacityParam(TEXT("Opacity"));
	const FName ColorParam(TEXT("Color"));

	constexpr float DotRadius = 6.f;         // cm
	constexpr float DotHeight = 3.f;         // au-dessus du sol
	constexpr float PathRecalcInterval = 0.3f;
	constexpr float FadeSpeed = 3.f;         // fondus (1/s)

	UStaticMeshComponent* SetupGlowMesh(UStaticMeshComponent* Mesh, UStaticMesh* Shape, UMaterialInterface* Mat)
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetTranslucentSortPriority(10); // devant la sphere sombre de l'experience
		if (Shape) Mesh->SetStaticMesh(Shape);
		if (Mat) Mesh->SetMaterial(0, Mat);
		return Mesh;
	}
}

AExperienceGuide::AExperienceGuide()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // apres le mouvement de la tete
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));

	Dots = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Dots"));
	Dots->SetupAttachment(Root);
	SetupGlowMesh(Dots, Sphere.Object, GlowMat.Object);
	Dots->SetUsingAbsoluteLocation(true);
	Dots->SetUsingAbsoluteRotation(true);
	Dots->SetUsingAbsoluteScale(true);

	Arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	SetupGlowMesh(Arrow, Cone.Object, GlowMat.Object);
	Arrow->SetUsingAbsoluteLocation(true);
	Arrow->SetUsingAbsoluteRotation(true);
	Arrow->SetUsingAbsoluteScale(true);
	Arrow->SetWorldScale3D(FVector(0.06f, 0.06f, 0.12f)); // cone de 6 x 12 cm
	Arrow->SetVisibility(false);
}

void AExperienceGuide::SetLookTarget(AActor* Actor, const FLinearColor& InColor)
{
	Target = nullptr;
	LookTarget = Actor;
	if (Actor)
	{
		Color = InColor;
	}
	SetActorTickEnabled(true);
}

void AExperienceGuide::SetTarget(AExperienceSpot* Spot)
{
	Target = Spot;
	LookTarget = nullptr;
	if (Spot)
	{
		Color = Spot->Color;
		RecalcTimer = 0.f; // nouveau chemin tout de suite
	}
	SetActorTickEnabled(true);
}

void AExperienceGuide::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!DotsMaterial)
	{
		DotsMaterial = Dots->CreateDynamicMaterialInstance(0);
		ArrowMaterial = Arrow->CreateDynamicMaterialInstance(0);
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
	const FVector Head = Camera ? Camera->GetComponentLocation() : Player->GetPawnViewLocation();
	const FRotator View = Camera ? Camera->GetComponentRotation() : Player->GetActorRotation();
	const FVector Feet(Head.X, Head.Y, Player->GetActorLocation().Z);

	UpdatePath(DeltaSeconds, Feet);
	UpdateArrow(DeltaSeconds, Head, View);

	// Plus de cible et tout est efface : on s'endort.
	if (!Target.IsValid() && !LookTarget.IsValid() && PathAlpha <= 0.f && ArrowAlpha <= 0.f)
	{
		SetActorTickEnabled(false);
	}
}

// ----------------------------------------------------------------------------
// Chemin au sol
// ----------------------------------------------------------------------------

void AExperienceGuide::RecalcPath(const FVector& Feet)
{
	const FVector Goal = Target->GetActorLocation();
	Path.Reset();

	if (const UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, Feet, Goal))
	{
		if (NavPath->IsValid() && NavPath->PathPoints.Num() >= 2)
		{
			Path = NavPath->PathPoints;
		}
	}
	if (Path.Num() < 2)
	{
		Path = { Feet, Goal }; // pas de NavMesh : ligne droite
	}

	PathDistances.SetNum(Path.Num());
	PathLength = 0.f;
	PathDistances[0] = 0.f;
	for (int32 i = 1; i < Path.Num(); ++i)
	{
		PathLength += FVector::Dist(Path[i - 1], Path[i]);
		PathDistances[i] = PathLength;
	}
}

FVector AExperienceGuide::PointAlongPath(float Distance) const
{
	for (int32 i = 1; i < Path.Num(); ++i)
	{
		if (Distance <= PathDistances[i])
		{
			const float SegLength = PathDistances[i] - PathDistances[i - 1];
			const float Alpha = SegLength > KINDA_SMALL_NUMBER ? (Distance - PathDistances[i - 1]) / SegLength : 0.f;
			return FMath::Lerp(Path[i - 1], Path[i], Alpha);
		}
	}
	return Path.Num() > 0 ? Path.Last() : FVector::ZeroVector;
}

void AExperienceGuide::UpdatePath(float DeltaSeconds, const FVector& Feet)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	AExperienceSpot* Spot = Target.Get();

	// Visible tant qu'il y a une zone a rejoindre et que le joueur n'y est pas.
	const bool bWanted = Settings->bGuidePath && Spot && !Spot->IsPlayerInside();
	PathAlpha = FMath::FInterpConstantTo(PathAlpha, bWanted ? 1.f : 0.f, DeltaSeconds, FadeSpeed);

	Dots->ClearInstances();
	if (PathAlpha <= 0.f)
	{
		return;
	}

	if (Spot)
	{
		RecalcTimer -= DeltaSeconds;
		if (RecalcTimer <= 0.f || Path.Num() < 2)
		{
			RecalcTimer = PathRecalcInterval;
			RecalcPath(Feet);
		}
	}
	if (Path.Num() < 2)
	{
		return;
	}

	// Les points partent d'un peu devant le joueur et s'arretent au bord de la zone.
	const float Spacing = Settings->GuidePathSpacing;
	const float StartGap = 40.f;
	const float EndGap = Spot ? Spot->Radius * 0.8f : 0.f;
	const float Usable = PathLength - EndGap;
	Flow = FMath::Fmod(Flow + Settings->GuidePathSpeed * DeltaSeconds, Spacing);

	TArray<FTransform> Instances;
	for (float D = StartGap + Flow; D < Usable; D += Spacing)
	{
		// Les points grossissent en partant du joueur et retrecissent en arrivant : effet de "flux".
		const float FadeIn = FMath::Clamp((D - StartGap) / 60.f, 0.f, 1.f);
		const float FadeOut = FMath::Clamp((Usable - D) / 60.f, 0.f, 1.f);
		const float Size = DotRadius * 2.f / 100.f * FMath::Min(FadeIn, FadeOut);
		if (Size <= 0.f)
		{
			continue;
		}
		const FVector Location = PointAlongPath(D) + FVector(0.f, 0.f, DotHeight);
		Instances.Add(FTransform(FQuat::Identity, Location, FVector(Size, Size, Size * 0.5f)));
	}
	Dots->AddInstances(Instances, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);

	if (DotsMaterial)
	{
		DotsMaterial->SetVectorParameterValue(ColorParam, Color * 4.f);
		DotsMaterial->SetScalarParameterValue(OpacityParam, 0.9f * PathAlpha);
	}
}

// ----------------------------------------------------------------------------
// Fleche "par ici"
// ----------------------------------------------------------------------------

void AExperienceGuide::UpdateArrow(float DeltaSeconds, const FVector& Head, const FRotator& View)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	AExperienceSpot* Spot = Target.Get();

	FVector ToTarget = FVector::ZeroVector;
	bool bWanted = false;
	FVector Goal;
	bool bHasGoal = false;
	if (Spot && !Spot->IsPlayerInside())
	{
		Goal = Spot->GetActorLocation();
		bHasGoal = true;
	}
	else if (const AActor* Look = LookTarget.Get())
	{
		Goal = Look->GetComponentsBoundingBox(true).GetCenter();
		bHasGoal = true;
	}
	if (Settings->bGuideArrow && bHasGoal)
	{
		ToTarget = Goal - Head;
		ToTarget.Z = 0.f;
		const FVector Look = FRotator(0.f, View.Yaw, 0.f).Vector();
		if (ToTarget.Normalize())
		{
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Look, ToTarget), -1.f, 1.f)));
			bWanted = Angle > Settings->GuideArrowAngle;
		}
	}
	ArrowAlpha = FMath::FInterpConstantTo(ArrowAlpha, bWanted ? 1.f : 0.f, DeltaSeconds, FadeSpeed);

	Arrow->SetVisibility(ArrowAlpha > 0.f);
	if (ArrowAlpha <= 0.f)
	{
		bArrowPlaced = false;
		return;
	}

	// Devant le joueur, un peu sous les yeux, et suit son regard en douceur.
	const FVector Look = FRotator(0.f, View.Yaw, 0.f).Vector();
	const FVector Wanted = Head + Look * 60.f - FVector(0.f, 0.f, 20.f);
	ArrowLocation = bArrowPlaced ? FMath::Lerp(ArrowLocation, Wanted, 1.f - FMath::Exp(-6.f * DeltaSeconds)) : Wanted;
	bArrowPlaced = true;

	// Pointe vers la zone, avec un petit mouvement de va-et-vient.
	if (!ToTarget.IsNearlyZero())
	{
		ArrowTime += DeltaSeconds;
		const float Bob = 3.f * FMath::Sin(ArrowTime * UE_TWO_PI * 1.2f);
		Arrow->SetWorldLocationAndRotation(ArrowLocation + ToTarget * Bob, FRotationMatrix::MakeFromZ(ToTarget).Rotator());
	}

	if (ArrowMaterial)
	{
		ArrowMaterial->SetVectorParameterValue(ColorParam, Color * 3.f);
		ArrowMaterial->SetScalarParameterValue(OpacityParam, 0.9f * ArrowAlpha);
	}
}
