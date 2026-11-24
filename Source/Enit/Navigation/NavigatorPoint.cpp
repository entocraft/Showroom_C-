#include "Navigation/NavigatorPoint.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ANavigatorPoint::ANavigatorPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Petit cylindre au sol pour reperer l'etape.
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.02f));
	Marker->SetCanEverAffectNavigation(false);
	Marker->SetHiddenInGame(true); // visible dans l'editeur, cache en jeu tant que ce n'est pas l'etape active
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Marker->SetStaticMesh(Cylinder.Object);
	}
}

void ANavigatorPoint::SetActiveTarget(bool bActive)
{
	Marker->SetHiddenInGame(!(bShowMarkerInGame && bActive));
}
