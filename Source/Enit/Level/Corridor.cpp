#include "Level/Corridor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float FloorLift = 1.f;      // le sol du couloir est 1 cm au-dessus du sol existant
	constexpr float EdgeLightWidth = 5.f; // largeur des lignes lumineuses (cm)
	constexpr float EdgeLightInset = 12.f;
}

ACorridor::ACorridor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable); // pas besoin de recalculer l'eclairage quand on modifie le couloir

	// Ligne de depart : un couloir de 8 m legerement courbe.
	Path = CreateDefaultSubobject<USplineComponent>(TEXT("Path"));
	Path->SetupAttachment(Root);
	Path->ClearSplinePoints(false);
	Path->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
	Path->AddSplinePoint(FVector(400.f, 80.f, 0.f), ESplineCoordinateSpace::Local, false);
	Path->AddSplinePoint(FVector(800.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
	Path->UpdateSpline();
	Path->EditorUnselectedSplineSegmentColor = FLinearColor(1.f, 0.7f, 0.2f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMat(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray_02.MI_PrototypeGrid_Gray_02"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMat(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));
	if (WallMat.Succeeded()) WallMaterial = WallMat.Object;
	if (FloorMat.Succeeded()) FloorMaterial = FloorMat.Object;

	Walls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	Walls->SetupAttachment(Root);
	Walls->SetUsingAbsoluteLocation(true);  // instances en coordonnees monde
	Walls->SetUsingAbsoluteRotation(true);
	Walls->SetUsingAbsoluteScale(true);
	Walls->SetMobility(EComponentMobility::Movable);
	Walls->SetCollisionProfileName(TEXT("BlockAll"));
	if (Cube.Succeeded()) Walls->SetStaticMesh(Cube.Object);

	FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(Root);
	FloorMesh->SetUsingAbsoluteLocation(true);
	FloorMesh->SetUsingAbsoluteRotation(true);
	FloorMesh->SetUsingAbsoluteScale(true);
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	FloorMesh->bUseComplexAsSimpleCollision = true;
	if (GlowMat.Succeeded()) FloorMesh->SetMaterial(1, GlowMat.Object);
}

void ACorridor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ACorridor::BeginPlay()
{
	Super::BeginPlay();

	// Normalement deja construit dans l'editeur ; sinon (ex : couloir cree par le code) on construit maintenant.
	const bool bNeedsFloor = Layout == EWallLayout::Corridor && bFloor;
	if (Walls->GetInstanceCount() == 0 || (bNeedsFloor && FloorMesh->GetNumSections() == 0))
	{
		Rebuild();
	}

	// Lignes lumineuses : materiau lumineux (couleur + opacite) cree en jeu.
	if (bEdgeLights && Layout == EWallLayout::Corridor && FloorMesh->GetNumSections() > 1)
	{
		EdgeMaterial = FloorMesh->CreateDynamicMaterialInstance(1);
		if (EdgeMaterial)
		{
			EdgeMaterial->SetVectorParameterValue(TEXT("Color"), EdgeLightColor * 3.f);
			EdgeMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.85f);
		}
	}
}

bool ACorridor::IsInOpening(const FVector& WorldPoint) const
{
	for (const FVector& Local : Openings)
	{
		const FVector Opening = GetActorTransform().TransformPosition(Local);
		if (FVector::Dist2D(Opening, WorldPoint) < OpeningWidth * 0.5f)
		{
			return true;
		}
	}
	return false;
}

void ACorridor::Rebuild()
{
	Walls->ClearInstances();
	FloorMesh->ClearAllMeshSections();
	Walls->SetMaterial(0, WallMaterial);

	const float Length = Path->GetSplineLength();
	if (Length < 1.f)
	{
		return;
	}
	const int32 NumSegments = FMath::Max(1, FMath::CeilToInt(Length / SegmentLength));

	// Points le long de la ligne, avec leur vecteur "droite" (pour decaler les murs de chaque cote).
	TArray<FVector> Centers;
	TArray<FVector> Rights;
	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float Distance = Length * i / NumSegments;
		Centers.Add(Path->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
		FVector Right = Path->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		Right.Z = 0.f;
		Rights.Add(Right.GetSafeNormal());
	}

	// --- Murs : des blocs entre deux points decales (pas de trou dans les courbes) ---
	auto AddWallLine = [&](float Offset)
	{
		for (int32 i = 0; i < NumSegments; ++i)
		{
			const FVector A = Centers[i] + Rights[i] * Offset;
			const FVector B = Centers[i + 1] + Rights[i + 1] * Offset;
			const FVector Mid = (A + B) * 0.5f;
			if (IsInOpening(Mid))
			{
				continue;
			}
			const FVector Dir = (B - A).GetSafeNormal2D();
			const float SegLength = FVector::Dist2D(A, B) + WallThickness * 0.5f; // leger recouvrement aux jointures
			const FTransform Instance(FRotator(0.f, Dir.Rotation().Yaw, 0.f),
				FVector(Mid.X, Mid.Y, Mid.Z + WallHeight * 0.5f),
				FVector(SegLength / 100.f, WallThickness / 100.f, WallHeight / 100.f));
			Walls->AddInstance(Instance, /*bWorldSpace*/ true);
		}
	};

	if (Layout == EWallLayout::Wall)
	{
		AddWallLine(0.f);
		return;
	}

	const float Half = Width * 0.5f + WallThickness * 0.5f;
	if (bLeftWall)  AddWallLine(-Half);
	if (bRightWall) AddWallLine(Half);

	// --- Sol : une bande exacte le long de la ligne (section 0), + 2 lignes lumineuses (section 1) ---
	if (bFloor)
	{
		TArray<FVector> Verts;
		TArray<int32> Tris;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		const float FloorHalf = Width * 0.5f + WallThickness;
		float V = 0.f;
		for (int32 i = 0; i <= NumSegments; ++i)
		{
			const FVector Up(0.f, 0.f, FloorLift);
			Verts.Add(Centers[i] - Rights[i] * FloorHalf + Up);
			Verts.Add(Centers[i] + Rights[i] * FloorHalf + Up);
			Normals.Append({ FVector::UpVector, FVector::UpVector });
			if (i > 0)
			{
				V += FVector::Dist(Centers[i - 1], Centers[i]) / 100.f;
			}
			UVs.Add(FVector2D(0.f, V));
			UVs.Add(FVector2D(Width / 100.f, V));
			if (i > 0)
			{
				const int32 B = (i - 1) * 2;
				Tris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
			}
		}
		FloorMesh->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, {}, {}, /*bCreateCollision*/ true);
		FloorMesh->SetMaterial(0, FloorMaterial);

		if (bEdgeLights)
		{
			TArray<FVector> LVerts;
			TArray<int32> LTris;
			for (const float Side : { -1.f, 1.f })
			{
				const float Inner = Width * 0.5f - EdgeLightInset - EdgeLightWidth;
				const float Outer = Width * 0.5f - EdgeLightInset;
				const int32 Start = LVerts.Num();
				for (int32 i = 0; i <= NumSegments; ++i)
				{
					const FVector Up(0.f, 0.f, FloorLift + 0.5f);
					LVerts.Add(Centers[i] + Rights[i] * Side * Inner + Up);
					LVerts.Add(Centers[i] + Rights[i] * Side * Outer + Up);
					if (i > 0)
					{
						const int32 B = Start + (i - 1) * 2;
						LTris.Append({ B, B + 2, B + 1, B + 1, B + 2, B + 3 });
						LTris.Append({ B, B + 1, B + 2, B + 1, B + 3, B + 2 }); // visible des deux cotes
					}
				}
			}
			FloorMesh->CreateMeshSection_LinearColor(1, LVerts, LTris, {}, {}, {}, {}, false);
		}
	}
}
