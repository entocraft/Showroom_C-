#include "Navigation/NavigatorSystem.h"
#include "Navigation/NavigatorPoint.h"
#include "Core/ShowroomSettings.h"
#include "Interaction/DetectionZone.h"
#include "Enit.h"
#include "Components/LineBatchComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ANavigatorSystem::ANavigatorSystem()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Composant qui dessine des lignes (fonctionne aussi dans le jeu package, contrairement a DrawDebugLine).
	PathLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("PathLines"));
	PathLines->SetupAttachment(RootComponent);
}

void ANavigatorSystem::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoCollectPoints && Points.Num() == 0)
	{
		for (TActorIterator<ANavigatorPoint> It(GetWorld()); It; ++It)
		{
			Points.Add(*It);
		}
		Points.Sort([](const ANavigatorPoint& A, const ANavigatorPoint& B) { return A.Order < B.Order; });
	}
	Points.RemoveAll([](const TObjectPtr<ANavigatorPoint>& P) { return P == nullptr; });

	if (Points.Num() == 0)
	{
		UE_LOG(LogShowroom, Warning, TEXT("NavigatorSystem : aucun NavigatorPoint dans le niveau."));
		return;
	}

	if (bStartOnBeginPlay)
	{
		StartGuidance();
	}
}

ANavigatorPoint* ANavigatorSystem::GetCurrentPoint() const
{
	return Points.IsValidIndex(CurrentIndex) ? Points[CurrentIndex].Get() : nullptr;
}

ANavigatorSystem* ANavigatorSystem::FindOrSpawn(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ANavigatorSystem> It(World); It; ++It)
	{
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ANavigatorSystem>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void ANavigatorSystem::GuideTo(AActor* Target)
{
	if (!Target)
	{
		StopGuideTo();
		return;
	}
	AActor* Previous = GuideTarget.Get();
	if (!Previous)
	{
		// Le parcours d'etapes passe en pause (son marqueur s'eteint).
		bRouteRunning = GetWorldTimerManager().IsTimerActive(RecalcTimer);
		if (ANavigatorPoint* Current = GetCurrentPoint())
		{
			Current->SetActiveTarget(false);
		}
	}
	else if (Previous != Target)
	{
		OnGuideStopped.Broadcast(Previous);
	}
	GuideTarget = Target;
	LastPos = FVector(TNumericLimits<float>::Max()); // recalcul immediat
	const float Interval = UShowroomSettings::Get()->NavigatorRefreshInterval;
	GetWorldTimerManager().SetTimer(RecalcTimer, this, &ANavigatorSystem::RecalcPath, Interval, /*bLoop*/ true, /*FirstDelay*/ 0.f);
}

void ANavigatorSystem::StopGuideTo()
{
	AActor* Previous = GuideTarget.Get();
	if (!GuideTarget.IsValid() && !Previous)
	{
		return;
	}
	GuideTarget = nullptr;
	ClearPath();
	if (bRouteRunning && Points.Num() > 0)
	{
		StartGuidance(); // le parcours d'etapes reprend
	}
	else
	{
		GetWorldTimerManager().ClearTimer(RecalcTimer);
	}
	OnGuideStopped.Broadcast(Previous);
}

void ANavigatorSystem::UpdateGuideTo(const FVector& PlayerPos)
{
	AActor* Target = GuideTarget.Get();
	const ADetectionZone* Zone = Cast<ADetectionZone>(Target);

	// Arrive ?
	const FVector Goal = Zone ? Zone->GetStartPointOnFloor() : Target->GetActorLocation();
	const bool bArrived = Zone ? Zone->IsPlayerInside()
		: FVector::Dist2D(PlayerPos, Goal) < UShowroomSettings::Get()->NavigatorReachDistance;
	if (bArrived)
	{
		OnGuideArrived.Broadcast(Target);
		StopGuideTo();
		return;
	}

	if (FVector::Dist(PlayerPos, LastPos) < UShowroomSettings::Get()->NavigatorMinMoveToRecalc)
	{
		return;
	}
	LastPos = PlayerPos;

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(this, PlayerPos, Goal);
	if (Path && Path->IsValid() && Path->PathPoints.Num() >= 2)
	{
		DrawPath(Path->PathPoints);
	}
	else
	{
		DrawPath({ PlayerPos, Goal }); // pas de NavMesh : ligne droite
	}
}

void ANavigatorSystem::StartGuidance()
{
	SetCurrentIndex(CurrentIndex);
	const float Interval = UShowroomSettings::Get()->NavigatorRefreshInterval;
	GetWorldTimerManager().SetTimer(RecalcTimer, this, &ANavigatorSystem::RecalcPath, Interval, /*bLoop*/ true, /*FirstDelay*/ 0.f);
}

void ANavigatorSystem::StopGuidance()
{
	GetWorldTimerManager().ClearTimer(RecalcTimer);
	if (ANavigatorPoint* Current = GetCurrentPoint())
	{
		Current->SetActiveTarget(false);
	}
	ClearPath();
}

void ANavigatorSystem::SetCurrentIndex(int32 NewIndex)
{
	if (ANavigatorPoint* Old = GetCurrentPoint())
	{
		Old->SetActiveTarget(false);
	}
	CurrentIndex = FMath::Clamp(NewIndex, 0, FMath::Max(Points.Num() - 1, 0));
	if (ANavigatorPoint* New = GetCurrentPoint())
	{
		New->SetActiveTarget(true);
	}
	LastPos = FVector(TNumericLimits<float>::Max()); // force le recalcul
}

void ANavigatorSystem::NextPoint()
{
	const bool bLast = CurrentIndex >= Points.Num() - 1;
	if (!bLast)
	{
		SetCurrentIndex(CurrentIndex + 1);
	}
	else if (UShowroomSettings::Get()->bNavigatorLoop)
	{
		SetCurrentIndex(0);
	}
	else
	{
		StopGuidance();
		OnRouteFinished.Broadcast();
	}
}

void ANavigatorSystem::RecalcPath()
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	// Destination choisie (ex : une experience) : prioritaire sur le parcours d'etapes.
	if (GuideTarget.IsValid())
	{
		if (ADetectionZone::IsAnyExperienceRunning())
		{
			ClearPath();
			LastPos = FVector(FLT_MAX);
			return;
		}
		FVector Feet = Player->GetPawnViewLocation();
		Feet.Z = Player->GetActorLocation().Z;
		UpdateGuideTo(Feet);
		return;
	}

	ANavigatorPoint* Target = GetCurrentPoint();
	if (!Target)
	{
		return;
	}

	// Pendant une experience, le GPS se met en pause (l'experience a son propre guidage).
	if (ADetectionZone::IsAnyExperienceRunning())
	{
		ClearPath();
		LastPos = FVector(FLT_MAX);
		return;
	}

	// Position des pieds du joueur = tete projetee au sol de l'origine VR.
	FVector PlayerPos = Player->GetPawnViewLocation();
	PlayerPos.Z = Player->GetActorLocation().Z;

	// Etape atteinte ?
	if (FVector::Dist2D(PlayerPos, Target->GetActorLocation()) < Settings->NavigatorReachDistance)
	{
		OnPointReached.Broadcast(Target, CurrentIndex);
		NextPoint(); // peut declencher OnRouteFinished : toujours APRES OnPointReached
		return;
	}

	// Le joueur n'a presque pas bouge : inutile de recalculer.
	if (FVector::Dist(PlayerPos, LastPos) < Settings->NavigatorMinMoveToRecalc)
	{
		return;
	}
	LastPos = PlayerPos;

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(this, PlayerPos, Target->GetActorLocation());
	if (Path && Path->IsValid() && Path->PathPoints.Num() >= 2)
	{
		DrawPath(Path->PathPoints);
	}
	else
	{
		// Pas de NavMesh : on trace une ligne droite.
		DrawPath({ PlayerPos, Target->GetActorLocation() });
	}
}

void ANavigatorSystem::DrawPath(const TArray<FVector>& PathPoints)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	const FVector Up(0.f, 0.f, Settings->NavigatorLineHeight);

	ClearPath();
	for (int32 i = 0; i + 1 < PathPoints.Num(); ++i)
	{
		PathLines->DrawLine(PathPoints[i] + Up, PathPoints[i + 1] + Up, Settings->NavigatorLineColor,
			SDPG_World, Settings->NavigatorLineThickness, /*LifeTime*/ 0.f);
	}
}

void ANavigatorSystem::ClearPath()
{
	if (PathLines)
	{
		PathLines->Flush();
	}
}
