#include "Interaction/DetectionZone.h"
#include "Interaction/InfoMenu.h"
#include "Interaction/ZoneGeometry.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "Experience/ExperienceSpot.h"
#include "Experience/ExperienceGuide.h"
#include "Experience/ExperienceHighlight.h"
#include "Experience/ExperiencePing.h"
#include "UI/FloatingScreen.h"
#include "Core/ShowroomAudio.h"
#include "Components/AudioComponent.h"
#include "Core/ShowroomSettings.h"
#include "Player/ShowroomPawn.h"
#include "Enit.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName DarknessOpacityParam(TEXT("Opacity"));
	const FName DarknessColorParam(TEXT("Color"));

	/** Une seule experience a la fois : pendant qu'elle tourne, les autres zones n'ouvrent pas leur fiche. */
	TWeakObjectPtr<ADetectionZone> GRunningExperience;

	FVector GetHeadLocation(const APawn* Player)
	{
		const UCameraComponent* Head = Player->FindComponentByClass<UCameraComponent>();
		return Head ? Head->GetComponentLocation() : Player->GetPawnViewLocation();
	}

	constexpr float PresenceInterval = 0.1f;   // verification de presence du joueur (s)
	constexpr float OutlineWidth = 4.f;        // largeur de la ligne au sol (cm)
	constexpr float OutlineIdleOpacity = 0.35f;
	constexpr float OutlineActiveOpacity = 0.9f;

	UProceduralMeshComponent* MakeWorldMesh(AActor* Owner, const TCHAR* Name, USceneComponent* Parent)
	{
		UProceduralMeshComponent* Mesh = Owner->CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		// Les sommets sont calcules en coordonnees MONDE.
		Mesh->SetUsingAbsoluteLocation(true);
		Mesh->SetUsingAbsoluteRotation(true);
		Mesh->SetUsingAbsoluteScale(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->bUseAsyncCooking = true;
		return Mesh;
	}
}

ADetectionZone::ADetectionZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // ne tourne que pendant l'experience et les fondus

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;
	Box->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	// La presence du joueur est verifiee par le code (boite OU forme dessinee) : pas besoin de collision.
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetGenerateOverlapEvents(false);
	Box->ShapeColor = FColor(0, 200, 255);
	Box->SetLineThickness(2.f);

	// Comme BP_DetectionZone : la fiche fait partie de la zone.
	Menu = CreateDefaultSubobject<UChildActorComponent>(TEXT("Menu"));
	Menu->SetupAttachment(Box);
	Menu->SetChildActorClass(AInfoMenu::StaticClass());
	Menu->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	// Sphere sombre de l'experience (cachee par defaut).
	DarknessShell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DarknessShell"));
	DarknessShell->SetupAttachment(Box);
	DarknessShell->SetUsingAbsoluteScale(true);
	DarknessShell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DarknessShell->SetCastShadow(false);
	DarknessShell->SetCanEverAffectNavigation(false);
	DarknessShell->SetHiddenInGame(true);
	DarknessShell->SetVisibility(false);
	DarknessShell->SetTranslucentSortPriority(-10); // dessinee AVANT les effets lumineux qu'elle contient
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) DarknessShell->SetStaticMesh(Sphere.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DarkMat(TEXT("/Game/Showroom/M_ExperienceDarkness.M_ExperienceDarkness"));
	if (DarkMat.Succeeded()) DarknessShell->SetMaterial(0, DarkMat.Object);

	// Contour dessine : un carre de 3 x 3 m pose au sol pour commencer.
	Outline = CreateDefaultSubobject<USplineComponent>(TEXT("Outline"));
	Outline->SetupAttachment(Box);
	Outline->SetClosedLoop(true, false);
	Outline->ClearSplinePoints(false);
	for (const FVector& Corner : { FVector(150, 150, -100), FVector(-150, 150, -100), FVector(-150, -150, -100), FVector(150, -150, -100) })
	{
		Outline->AddSplinePoint(Corner, ESplineCoordinateSpace::Local, false);
	}
	for (int32 i = 0; i < 4; ++i)
	{
		Outline->SetSplinePointType(i, ESplinePointType::Linear, false);
	}
	Outline->UpdateSpline();
	Outline->SetHiddenInGame(true);
	Outline->EditorUnselectedSplineSegmentColor = FLinearColor(0.f, 0.8f, 1.f);

	Curtain = MakeWorldMesh(this, TEXT("Curtain"), Box);
	Curtain->SetTranslucentSortPriority(-10);
	Curtain->SetVisibility(false);
	OutlineMesh = MakeWorldMesh(this, TEXT("OutlineMesh"), Box);
	OutlineMesh->SetTranslucentSortPriority(5);
	if (DarkMat.Succeeded())
	{
		Curtain->SetMaterial(0, DarkMat.Object);
		OutlineMesh->SetMaterial(0, DarkMat.Object);
	}
}

AInfoMenu* ADetectionZone::GetMenu() const
{
	if (LinkedMenu)
	{
		return LinkedMenu;
	}
	return Menu ? Cast<AInfoMenu>(Menu->GetChildActor()) : nullptr;
}

void ADetectionZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Un panneau externe est utilise : la fiche interne ne sert a rien.
	const TSubclassOf<AActor> WantedClass = LinkedMenu ? nullptr : AInfoMenu::StaticClass();
	if (Menu->GetChildActorClass() != WantedClass)
	{
		Menu->SetChildActorClass(WantedClass);
	}
	PushInfoToMenu();
	RebuildShape();
	FitDarknessShell();
}

void ADetectionZone::PushInfoToMenu() const
{
	if (AInfoMenu* Target = GetMenu())
	{
		Target->SetInfo(Info);
	}
}

void ADetectionZone::RebuildShape()
{
	// 1) Le contour au sol (monde)
	Polygon.Reset();
	if (Shape == EZoneShape::Drawn && Outline->GetNumberOfSplinePoints() >= 3)
	{
		const int32 NumPoints = Outline->GetNumberOfSplinePoints();
		FloorZ = TNumericLimits<float>::Max();
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const FVector Point = Outline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			FloorZ = FMath::Min(FloorZ, (float)Point.Z);
			Polygon.Add(FVector2D(Point));
			// Segment courbe : on ajoute des points intermediaires.
			if (Outline->GetSplinePointType(i) != ESplinePointType::Linear)
			{
				const float From = Outline->GetDistanceAlongSplineAtSplinePoint(i);
				const float To = (i + 1 < NumPoints) ? Outline->GetDistanceAlongSplineAtSplinePoint(i + 1) : Outline->GetSplineLength();
				for (int32 k = 1; k < 8; ++k)
				{
					Polygon.Add(FVector2D(Outline->GetLocationAtDistanceAlongSpline(FMath::Lerp(From, To, k / 8.f), ESplineCoordinateSpace::World)));
				}
			}
		}
		ZoneHeight = DrawnHeight;
	}
	else
	{
		const FTransform& T = Box->GetComponentTransform();
		const FVector E = Box->GetUnscaledBoxExtent();
		for (const FVector& Corner : { FVector(E.X, E.Y, -E.Z), FVector(-E.X, E.Y, -E.Z), FVector(-E.X, -E.Y, -E.Z), FVector(E.X, -E.Y, -E.Z) })
		{
			Polygon.Add(FVector2D(T.TransformPosition(Corner)));
		}
		FloorZ = T.TransformPosition(FVector(0.f, 0.f, -E.Z)).Z;
		ZoneHeight = Box->GetScaledBoxExtent().Z * 2.f;
	}
	ZoneGeometry::Clean(Polygon);

	Box->SetVisibility(Shape == EZoneShape::Box);
	Outline->SetVisibility(Shape == EZoneShape::Drawn);

	// 2) La ligne lumineuse au sol (un ruban le long du contour)
	OutlineMesh->ClearAllMeshSections();
	if (bShowOutline && Polygon.Num() >= 3)
	{
		TArray<FVector> Verts;
		TArray<int32> Tris;
		const float Z = FloorZ + 1.5f;
		for (int32 i = 0; i < Polygon.Num(); ++i)
		{
			const FVector2D A = Polygon[i];
			const FVector2D B = Polygon[(i + 1) % Polygon.Num()];
			const FVector2D Side = FVector2D(-(B - A).Y, (B - A).X).GetSafeNormal() * (OutlineWidth * 0.5f);
			const int32 Base = Verts.Num();
			Verts.Add(FVector(A - Side, Z));
			Verts.Add(FVector(A + Side, Z));
			Verts.Add(FVector(B + Side, Z));
			Verts.Add(FVector(B - Side, Z));
			Tris.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
		}
		OutlineMesh->CreateMeshSection_LinearColor(0, Verts, Tris, {}, {}, {}, {}, false);
	}

	// 3) Les murs sombres (zone dessinee) : un mur le long du contour + un plafond.
	Curtain->ClearAllMeshSections();
	if (Shape == EZoneShape::Drawn && Polygon.Num() >= 3)
	{
		TArray<FVector> Verts;
		TArray<int32> Tris;
		const float Bottom = FloorZ - 20.f;
		const float Top = FloorZ + ZoneHeight;
		for (int32 i = 0; i < Polygon.Num(); ++i)
		{
			const FVector2D A = Polygon[i];
			const FVector2D B = Polygon[(i + 1) % Polygon.Num()];
			const int32 Base = Verts.Num();
			Verts.Add(FVector(A, Bottom));
			Verts.Add(FVector(B, Bottom));
			Verts.Add(FVector(B, Top));
			Verts.Add(FVector(A, Top));
			Tris.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
		}
		const int32 CapBase = Verts.Num();
		for (const FVector2D& P : Polygon)
		{
			Verts.Add(FVector(P, Top));
		}
		for (const int32 Index : ZoneGeometry::Triangulate(Polygon))
		{
			Tris.Add(CapBase + Index);
		}
		Curtain->CreateMeshSection_LinearColor(0, Verts, Tris, {}, {}, {}, {}, false);
	}
}

bool ADetectionZone::IsPointInside(const FVector& Point) const
{
	if (Polygon.Num() < 3 || Point.Z < FloorZ - 50.f || Point.Z > FloorZ + ZoneHeight + 50.f)
	{
		return false;
	}
	return ZoneGeometry::IsInside(Polygon, FVector2D(Point));
}

void ADetectionZone::SetDarknessVisible(bool bVisible)
{
	const bool bDrawn = Shape == EZoneShape::Drawn;
	DarknessShell->SetHiddenInGame(!(bVisible && !bDrawn));
	DarknessShell->SetVisibility(bVisible && !bDrawn);
	Curtain->SetVisibility(bVisible && bDrawn);
}

void ADetectionZone::CheckPresence()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	APawn* Local = (Player && Player->IsLocallyControlled()) ? const_cast<APawn*>(Player) : nullptr;
	const bool bInside = Local && IsPointInside(GetHeadLocation(Local));
	if (bInside != bPlayerInside)
	{
		bPlayerInside = bInside;
		if (bInside)
		{
			HandlePlayerEnter(Local);
		}
		else
		{
			HandlePlayerExit(Local);
		}
	}
}

void ADetectionZone::FitDarknessShell()
{
	// Une sphere centree sur la zone qui englobe la boite, les zones lumineuses et les objets de l'experience.
	const FVector Center = Box->GetComponentLocation();
	const FVector Extent = Box->GetScaledBoxExtent();
	float Radius = FMath::Max(Extent.Size2D(), Extent.Z);

	for (const FExperienceStep& Step : Steps)
	{
		for (const AExperienceSpot* Spot : Step.Spots)
		{
			if (Spot)
			{
				Radius = FMath::Max(Radius, FVector::Dist(Center, Spot->GetActorLocation()) + Spot->GetBoundingRadius());
			}
		}
		for (const FExperienceHighlightInfo& Highlight : Step.Highlights)
		{
			if (Highlight.Actor)
			{
				FVector Origin, BoundsExtent;
				Highlight.Actor->GetActorBounds(false, Origin, BoundsExtent);
				Radius = FMath::Max(Radius, FVector::Dist(Center, Origin) + BoundsExtent.Size());
			}
		}
		for (const AFloatingScreen* Screen : Step.Screens)
		{
			if (Screen)
			{
				FVector Origin, BoundsExtent;
				Screen->GetActorBounds(false, Origin, BoundsExtent);
				Radius = FMath::Max(Radius, FVector::Dist(Center, Origin) + BoundsExtent.Size());
			}
		}
	}

	Radius += UShowroomSettings::Get()->ExperienceDarknessMargin;
	ExperienceRadius = Radius;
	DarknessShell->SetWorldLocation(Center);
	DarknessShell->SetWorldScale3D(FVector(Radius * 2.f / 100.f)); // la sphere du moteur fait 100 cm de diametre
}

void ADetectionZone::BeginPlay()
{
	Super::BeginPlay();

	PushInfoToMenu();
	RebuildShape();
	FitDarknessShell();

	const UShowroomSettings* Settings = UShowroomSettings::Get();
	if (UMaterialInterface* Mat = DarknessShell->GetMaterial(0))
	{
		DarknessMaterial = DarknessShell->CreateDynamicMaterialInstance(0, Mat);
		DarknessMaterial->SetVectorParameterValue(DarknessColorParam, Settings->ExperienceDarknessColor);
		DarknessMaterial->SetScalarParameterValue(DarknessOpacityParam, 0.f);
		CurtainMaterial = Curtain->CreateDynamicMaterialInstance(0, Mat);
		CurtainMaterial->SetVectorParameterValue(DarknessColorParam, Settings->ExperienceDarknessColor);
		CurtainMaterial->SetScalarParameterValue(DarknessOpacityParam, 0.f);
		OutlineMaterial = OutlineMesh->CreateDynamicMaterialInstance(0, Mat);
		OutlineMaterial->SetVectorParameterValue(DarknessColorParam, Settings->ZoneOutlineColor * 3.f);
		OutlineMaterial->SetScalarParameterValue(DarknessOpacityParam, OutlineIdleOpacity);
		OutlineAlpha = OutlineIdleOpacity;
	}
	else
	{
		UE_LOG(LogShowroom, Warning, TEXT("DetectionZone : materiau /Game/Showroom/M_ExperienceDarkness introuvable, l'assombrissement ne fonctionnera pas."));
	}

	InitExperienceSpots();
	GetWorldTimerManager().SetTimerForNextTick(this, &ADetectionZone::HideStepScreens);

	GetWorldTimerManager().SetTimer(PresenceTimer, this, &ADetectionZone::CheckPresence, PresenceInterval, true,
		FMath::FRand() * PresenceInterval); // les zones ne verifient pas toutes en meme temps
}

bool ADetectionZone::IsAnyExperienceRunning()
{
	return GRunningExperience.IsValid();
}

void ADetectionZone::EndPlay(const EEndPlayReason::Type Reason)
{
	bExperienceRunning = false;
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (Guide)
	{
		Guide->Destroy();
		Guide = nullptr;
	}
	for (AExperienceHighlight* Highlight : ActiveHighlights)
	{
		if (IsValid(Highlight))
		{
			Highlight->Destroy();
		}
	}
	ActiveHighlights.Reset();
	if (GRunningExperience.Get() == this)
	{
		GRunningExperience.Reset();
	}
	Super::EndPlay(Reason);
}

void ADetectionZone::UpdateMenuButton() const
{
	if (AInfoMenu* Target = GetMenu())
	{
		Target->SetActionButton(bHasExperience, bExperienceRunning ? StopButtonText : StartButtonText);
	}
}

void ADetectionZone::HandlePlayerEnter(APawn* Player)
{
	// Pendant l'experience d'une AUTRE zone, on n'ouvre pas notre fiche (sinon deux fiches devant le joueur).
	const ADetectionZone* Running = GRunningExperience.Get();
	if (Running && Running != this)
	{
		return;
	}

	if (AInfoMenu* Target = GetMenu())
	{
		// Un panneau partage affiche la fiche de la DERNIERE zone dans laquelle on est entre.
		Target->ActiveZone = this;
		UpdateMenuForStep();
		UpdateMenuButton();
		Target->ShowMenu(true);
	}
	OnPlayerEnter.Broadcast(Player);
}

void ADetectionZone::HandlePlayerExit(APawn* Player)
{
	// Pendant l'experience, le joueur peut sortir de la boite pour rejoindre les zones lumineuses :
	// l'experience ne s'arrete que s'il s'eloigne vraiment (voir Tick).
	if (!bExperienceRunning)
	{
		HideMenuIfOutside();
	}
	OnPlayerExit.Broadcast(Player);
}

void ADetectionZone::HideMenuIfOutside()
{
	AInfoMenu* Target = GetMenu();
	// On ne cache le panneau que si c'est bien NOTRE fiche qui est affichee
	// (sinon on cacherait celle d'une zone voisine dans laquelle le joueur vient d'entrer).
	if (!bPlayerInside && Target && bHideOnExit && Target->ActiveZone == this)
	{
		Target->ShowMenu(false);
		Target->ActiveZone = nullptr;
	}
}

// ============================================================================
// Experience
// ============================================================================

FVector ADetectionZone::GetStartPointOnFloor() const
{
	const FVector Start3D = GetActorTransform().TransformPosition(StartPoint);

	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowroomZoneFloor), false, this);
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Query.AddIgnoredActor(Player);
	}
	const FVector Start = FVector(Start3D.X, Start3D.Y, FMath::Max((float)Start3D.Z, FloorZ) + 150.f);
	const FVector End = FVector(Start3D.X, Start3D.Y, FloorZ - 500.f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query))
	{
		return Hit.ImpactPoint;
	}
	return FVector(Start3D.X, Start3D.Y, FloorZ); // sol de la zone
}

void ADetectionZone::ToggleExperience()
{
	if (bExperienceRunning)
	{
		StopExperience();
	}
	else
	{
		StartExperience();
	}
}

void ADetectionZone::StartExperience()
{
	if (bExperienceRunning)
	{
		return;
	}
	if (GRunningExperience.IsValid() && GRunningExperience.Get() != this)
	{
		GRunningExperience->StopExperience();
	}
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	bExperienceRunning = true;
	GRunningExperience = this;

	// 1) Le joueur au centre de la zone
	if (AShowroomPawn* ShowroomPlayer = Cast<AShowroomPawn>(Player))
	{
		ShowroomPlayer->TeleportPlayerTo(GetStartPointOnFloor());
	}
	else
	{
		Player->SetActorLocation(GetStartPointOnFloor());
	}

	// 2) Tout ce qui est hors de l'experience s'assombrit (fondu dans Tick)
	RebuildShape();
	FitDarknessShell();
	SetDarknessVisible(true);
	OutsideTime = 0.f;
	SetActorTickEnabled(true);

	UpdateMenuButton();
	OnExperienceStarted.Broadcast(Player);

	// Son de debut et fond sonore
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	ShowroomAudio::Play2D(this, Settings->SoundExperienceStart, Settings->EffectsVolume);
	if (AmbienceAudio)
	{
		AmbienceAudio->Stop();
	}
	AmbienceAudio = ShowroomAudio::Spawn2D(this, Settings->SoundAmbience.LoadSynchronous(), Settings->AmbienceVolume, 2.f);

	// 3) Premiere etape
	if (Steps.Num() > 0)
	{
		GoToStep(0);
	}
	else
	{
		UpdateMenuForStep();
	}
}

void ADetectionZone::StopExperience()
{
	if (!bExperienceRunning)
	{
		return;
	}
	bExperienceRunning = false;
	if (GRunningExperience.Get() == this)
	{
		GRunningExperience.Reset();
	}

	// Tout s'eteint : zones lumineuses et mises en valeur.
	CurrentStep = -1;
	ApplyStep(-1);

	// Sons : fin, narrateur coupe, fond sonore en fondu.
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	GetWorldTimerManager().ClearTimer(AdvanceTimer);
	StopNarration();
	ShowroomAudio::Play2D(this, Settings->SoundExperienceEnd, Settings->EffectsVolume);
	if (AmbienceAudio)
	{
		AmbienceAudio->FadeOut(1.5f, 0.f);
		AmbienceAudio = nullptr;
	}
	UpdateGuide();
	SetActorTickEnabled(true); // fondu de retour

	UpdateMenuButton();
	UpdateMenuForStep();
	OnExperienceStopped.Broadcast(UGameplayStatics::GetPlayerPawn(this, 0));

	HideMenuIfOutside();
}

void ADetectionZone::NextStep()
{
	if (!bExperienceRunning)
	{
		return;
	}
	if (CurrentStep + 1 < Steps.Num())
	{
		GoToStep(CurrentStep + 1);
	}
	else if (bEndAfterLastStep)
	{
		StopExperience();
	}
}

void ADetectionZone::PreviousStep()
{
	if (bExperienceRunning && CurrentStep > 0)
	{
		GoToStep(CurrentStep - 1);
	}
}

void ADetectionZone::GoToStep(int32 StepIndex)
{
	if (!bExperienceRunning || Steps.Num() == 0)
	{
		return;
	}
	StepIndex = FMath::Clamp(StepIndex, 0, Steps.Num() - 1);
	const bool bFirstStep = CurrentStep < 0;
	CurrentStep = StepIndex;
	StepTime = 0.f;
	GetWorldTimerManager().ClearTimer(AdvanceTimer);

	ApplyStep(StepIndex);

	const FExperienceStep& Step = Steps[StepIndex];
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	if (!bFirstStep)
	{
		ShowroomAudio::Play2D(this, Settings->SoundStepChange, Settings->EffectsVolume);
	}

	// Le narrateur parle apres un court instant (le temps que les objets commencent a apparaitre).
	StopNarration();
	if (Settings->bNarrationEnabled && Step.Narration)
	{
		GetWorldTimerManager().SetTimer(NarrationTimer,
			FTimerDelegate::CreateUObject(this, &ADetectionZone::PlayNarration, StepIndex),
			FMath::Max(Settings->NarrationDelay, 0.01f), false);
	}
	UpdateGuide();
	if (Step.bTeleportToSpot && Step.Spots.Num() > 0 && Step.Spots[0])
	{
		if (AShowroomPawn* Player = Cast<AShowroomPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Player->TeleportPlayerTo(Step.Spots[0]->GetActorLocation());
		}
	}
	if (Step.Sound)
	{
		UGameplayStatics::PlaySound2D(this, Step.Sound);
	}

	UpdateMenuForStep();
	SetActorTickEnabled(true);
	OnStepChanged.Broadcast(StepIndex);
}

void ADetectionZone::UpdateMenuForStep() const
{
	AInfoMenu* Target = GetMenu();
	if (!Target || Target->ActiveZone != this)
	{
		return;
	}

	FShowroomInfo Shown = Info;
	if (bExperienceRunning && Steps.IsValidIndex(CurrentStep))
	{
		const FExperienceStep& Step = Steps[CurrentStep];
		if (!Step.Title.IsEmpty())       Shown.Title = Step.Title;
		if (!Step.Description.IsEmpty()) Shown.Description = Step.Description;

		const FText Label = FText::Format(FText::FromString(TEXT("Étape {0} / {1}")),
			FText::AsNumber(CurrentStep + 1), FText::AsNumber(Steps.Num()));
		Target->SetStepNav(true, Label, CurrentStep > 0, CurrentStep + 1 < Steps.Num() || bEndAfterLastStep);
	}
	else
	{
		Target->SetStepNav(false, FText::GetEmpty(), false, false);
	}
	Target->SetInfo(Shown);
}

void ADetectionZone::HandleSpotReached(AExperienceSpot* Spot)
{
	if (bExperienceRunning && Steps.IsValidIndex(CurrentStep))
	{
		const FExperienceStep& Step = Steps[CurrentStep];
		if (Step.Spots.Contains(Spot))
		{
			const UShowroomSettings* Settings = UShowroomSettings::Get();
			ShowroomAudio::Play2D(this, Settings->SoundSpotReached, Settings->EffectsVolume);
			UpdateGuide();
			if (Step.Advance == EExperienceAdvance::ReachSpot)
			{
				NextStep();
			}
		}
	}
}

// ============================================================================
// Acteurs et zones lumineuses des etapes
// ============================================================================

void ADetectionZone::InitExperienceSpots()
{
	AllSpots.Reset();
	AllScreens.Reset();
	for (const FExperienceStep& Step : Steps)
	{
		for (AFloatingScreen* Screen : Step.Screens)
		{
			if (Screen)
			{
				AllScreens.AddUnique(Screen);
			}
		}
		for (AExperienceSpot* Spot : Step.Spots)
		{
			if (Spot && !AllSpots.Contains(Spot))
			{
				AllSpots.Add(Spot);
				Spot->OnPlayerReached.AddUObject(this, &ADetectionZone::HandleSpotReached);
			}
		}
	}
}

void ADetectionZone::HideStepScreens()
{
	if (bExperienceRunning)
	{
		return;
	}
	for (const TWeakObjectPtr<AFloatingScreen>& Screen : AllScreens)
	{
		if (Screen.IsValid())
		{
			Screen->HideScreen();
		}
	}
}

void ADetectionZone::ClearHighlights()
{
	for (AExperienceHighlight* Highlight : ActiveHighlights)
	{
		if (IsValid(Highlight))
		{
			Highlight->Dismiss();
		}
	}
	ActiveHighlights.Reset();
}

void ADetectionZone::ApplyStep(int32 StepIndex)
{
	// 1) Objets mis en valeur : ceux de l'etape en cours, les uns apres les autres.
	ClearHighlights();
	if (Steps.IsValidIndex(StepIndex))
	{
		const FExperienceStep& Step = Steps[StepIndex];
		// On laisse les mises en valeur precedentes s'eteindre avant d'allumer les nouvelles.
		float Delay = 0.3f;
		for (const FExperienceHighlightInfo& Highlight : Step.Highlights)
		{
			if (AExperienceHighlight* Spawned = AExperienceHighlight::Spawn(GetWorld(), Highlight, Delay))
			{
				ActiveHighlights.Add(Spawned);
				Delay += Step.DelayBetweenHighlights;
			}
		}
	}

	// 2) Ecrans de detail : ceux de l'etape en cours apparaissent, les autres disparaissent.
	for (const TWeakObjectPtr<AFloatingScreen>& Screen : AllScreens)
	{
		if (AFloatingScreen* S = Screen.Get())
		{
			const bool bWanted = Steps.IsValidIndex(StepIndex) && Steps[StepIndex].Screens.Contains(S);
			if (bWanted && !S->IsScreenVisible())
			{
				S->ShowScreen();
			}
			else if (!bWanted && S->IsScreenVisible())
			{
				S->HideScreen();
			}
		}
	}

	// 3) Zones lumineuses : etape en cours = allumee, etapes passees = faible, le reste = eteint.
	TMap<AExperienceSpot*, EExperienceSpotState> SpotStates;
	for (const TWeakObjectPtr<AExperienceSpot>& Spot : AllSpots)
	{
		if (Spot.IsValid()) SpotStates.Add(Spot.Get(), EExperienceSpotState::Off);
	}
	for (int32 i = 0; i < StepIndex && Steps.IsValidIndex(i); ++i)
	{
		for (AExperienceSpot* Spot : Steps[i].Spots) { if (Spot) SpotStates.Add(Spot, EExperienceSpotState::Done); }
	}
	if (Steps.IsValidIndex(StepIndex))
	{
		for (AExperienceSpot* Spot : Steps[StepIndex].Spots) { if (Spot) SpotStates.Add(Spot, EExperienceSpotState::Active); }
	}
	for (const TPair<AExperienceSpot*, EExperienceSpotState>& Pair : SpotStates)
	{
		Pair.Key->SetState(Pair.Value);
	}
}

void ADetectionZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UShowroomSettings* Settings = UShowroomSettings::Get();

	// --- Deroulement de l'experience ---
	if (bExperienceRunning)
	{
		StepTime += DeltaSeconds;
		if (Steps.IsValidIndex(CurrentStep))
		{
			const FExperienceStep& Step = Steps[CurrentStep];
			if (Step.Advance == EExperienceAdvance::Timer && StepTime >= Step.Duration)
			{
				StepTime = -FLT_MAX; // une seule fois (ex : derniere etape sans fin automatique)
				NextStep();
			}
		}

		// Le joueur s'est eloigne de l'experience (marche / teleportation) : on arrete.
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (Shape == EZoneShape::Drawn)
			{
				// Zone dessinee : sortir de la forme plus d'une seconde (traverser un mur sombre).
				OutsideTime = bPlayerInside ? 0.f : OutsideTime + DeltaSeconds;
				if (OutsideTime > 1.f)
				{
					StopExperience();
				}
			}
			else if (FVector::Dist2D(GetHeadLocation(Player), Box->GetComponentLocation()) > ExperienceRadius)
			{
				StopExperience();
			}
		}
	}

	// --- Assombrissement ---
	const float Target = bExperienceRunning ? Settings->ExperienceDarkness : 0.f;
	DarknessAlpha = FMath::FInterpConstantTo(DarknessAlpha, Target, DeltaSeconds, 1.f / FMath::Max(Settings->ExperienceFadeTime, 0.01f));
	if (DarknessMaterial)
	{
		DarknessMaterial->SetScalarParameterValue(DarknessOpacityParam, DarknessAlpha);
	}
	if (CurtainMaterial)
	{
		CurtainMaterial->SetScalarParameterValue(DarknessOpacityParam, DarknessAlpha);
	}

	// Le contour au sol brille davantage pendant l'experience.
	const float OutlineTarget = bExperienceRunning ? OutlineActiveOpacity : OutlineIdleOpacity;
	OutlineAlpha = FMath::FInterpConstantTo(OutlineAlpha, OutlineTarget, DeltaSeconds, 1.f);
	if (OutlineMaterial)
	{
		OutlineMaterial->SetScalarParameterValue(DarknessOpacityParam, OutlineAlpha);
	}

	const bool bFading = !FMath::IsNearlyEqual(DarknessAlpha, Target) || !FMath::IsNearlyEqual(OutlineAlpha, OutlineTarget);
	if (!bFading && !bExperienceRunning)
	{
		SetDarknessVisible(false);
	}

	if (!bExperienceRunning && !bFading)
	{
		SetActorTickEnabled(false);
	}
}

// ============================================================================
// Narrateur et guide
// ============================================================================

void ADetectionZone::PlayNarration(int32 StepIndex)
{
	if (!bExperienceRunning || StepIndex != CurrentStep || !Steps.IsValidIndex(StepIndex))
	{
		return;
	}
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	NarrationAudio = ShowroomAudio::Spawn2D(this, Steps[StepIndex].Narration, Settings->NarrationVolume);
	if (NarrationAudio)
	{
		NarrationAudio->OnAudioFinished.AddDynamic(this, &ADetectionZone::HandleNarrationFinished);
		// Le fond sonore baisse pendant que le narrateur parle.
		if (AmbienceAudio && Settings->AmbienceVolume > 0.f)
		{
			AmbienceAudio->AdjustVolume(0.5f, Settings->AmbienceVolumeDuringNarration / Settings->AmbienceVolume);
		}
	}
}

void ADetectionZone::StopNarration()
{
	GetWorldTimerManager().ClearTimer(NarrationTimer);
	if (NarrationAudio)
	{
		// On se desabonne AVANT d'arreter : sinon l'arret compterait comme "narration terminee".
		NarrationAudio->OnAudioFinished.RemoveAll(this);
		NarrationAudio->FadeOut(0.3f, 0.f);
		NarrationAudio = nullptr;
	}
	if (AmbienceAudio)
	{
		AmbienceAudio->AdjustVolume(0.5f, 1.f);
	}
}

void ADetectionZone::HandleNarrationFinished()
{
	NarrationAudio = nullptr;
	if (AmbienceAudio)
	{
		AmbienceAudio->AdjustVolume(1.f, 1.f);
	}

	if (bExperienceRunning && Steps.IsValidIndex(CurrentStep) && Steps[CurrentStep].Advance == EExperienceAdvance::AfterNarration)
	{
		const int32 StepWhenFinished = CurrentStep;
		GetWorldTimerManager().SetTimer(AdvanceTimer, FTimerDelegate::CreateWeakLambda(this, [this, StepWhenFinished]()
		{
			if (bExperienceRunning && CurrentStep == StepWhenFinished)
			{
				NextStep();
			}
		}), FMath::Max(UShowroomSettings::Get()->NarrationAdvanceDelay, 0.01f), false);
	}
}

void ADetectionZone::UpdateGuide()
{
	// La zone a rejoindre : la premiere zone lumineuse de l'etape ou le joueur n'est pas encore.
	// Sinon, la fleche montre le premier objet mis en valeur (s'il est hors du regard).
	AExperienceSpot* Wanted = nullptr;
	if (bExperienceRunning && Steps.IsValidIndex(CurrentStep))
	{
		for (AExperienceSpot* Spot : Steps[CurrentStep].Spots)
		{
			if (Spot && !Spot->IsPlayerInside())
			{
				Wanted = Spot;
				break;
			}
		}
	}

	AActor* LookAt = nullptr;
	if (!Wanted && bExperienceRunning && Steps.IsValidIndex(CurrentStep))
	{
		for (const FExperienceHighlightInfo& Highlight : Steps[CurrentStep].Highlights)
		{
			if (Highlight.Actor)
			{
				LookAt = Highlight.Actor;
				break;
			}
		}
		if (!LookAt && Steps[CurrentStep].Screens.Num() > 0)
		{
			LookAt = Steps[CurrentStep].Screens[0]; // la fleche montre l'ecran de detail s'il est hors du regard
		}
	}

	if ((Wanted || LookAt) && !Guide)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Guide = GetWorld()->SpawnActor<AExperienceGuide>(GetActorLocation(), FRotator::ZeroRotator, Params);
	}
	if (Guide)
	{
		if (Wanted)
		{
			Guide->SetTarget(Wanted);
		}
		else
		{
			Guide->SetLookTarget(LookAt, UShowroomSettings::Get()->HighlightColor);
		}
	}
}
