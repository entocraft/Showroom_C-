#include "Interaction/GrabbableActor.h"
#include "Interaction/ShowroomGrabComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AGrabbableActor::AGrabbableActor()
{
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetRelativeScale3D(FVector(0.1f));

	// Meme apparence que l'ancien BP_Grabbable_SmallCube.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CubeMat(TEXT("/Game/XRFramework/Materials/MI_VRColorway.MI_VRColorway"));
	if (CubeMat.Succeeded())
	{
		Mesh->SetMaterial(0, CubeMat.Object);
	}

	Grab = CreateDefaultSubobject<UShowroomGrabComponent>(TEXT("Grab"));
	Grab->SetupAttachment(Mesh);
	Grab->GrabType = EShowroomGrabType::Free;
}
