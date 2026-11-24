#include "Player/ShowroomPawn.h"
#include "Core/ShowroomSettings.h"
#include "Interaction/ShowroomGrabComponent.h"
#include "Input/ShowroomInput.h"
#include "Player/ShowroomHandAnimInstance.h"
#include "UI/HandMenu.h"
#include "Enit.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

// ============================================================================
// Petits utilitaires
// ============================================================================

namespace
{
	FShowroomHandPose& PoseFor(FShowroomHandPose& Left, FShowroomHandPose& Right, bool bLeft) { return bLeft ? Left : Right; }
}

// ============================================================================
// Construction
// ============================================================================

AShowroomPawn::AShowroomPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false); // la position est envoyee via ReplicatedTracking
	bAlwaysRelevant = true;      // petit showroom : tout le monde voit tout le monde
	AutoPossessAI = EAutoPossessAI::Disabled;

	// --- Origine VR + camera -------------------------------------------------
	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	RootComponent = VROrigin;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);
	Camera->bLockToHmd = true;

	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->SetupAttachment(VROrigin);
	Body->InitCapsuleSize(20.f, 88.f);
	Body->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Body->SetCollisionObjectType(ECC_Pawn);
	Body->SetGenerateOverlapEvents(true);
	Body->SetCanEverAffectNavigation(false);

	RemoteHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RemoteHead"));
	RemoteHead->SetupAttachment(Camera);
	RemoteHead->SetOwnerNoSee(true); // on ne voit pas sa propre tete
	RemoteHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RemoteHead->SetRelativeScale3D(FVector(0.22f, 0.18f, 0.25f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) RemoteHead->SetStaticMesh(SphereMesh.Object);

	// --- Manettes ------------------------------------------------------------
	auto MakeController = [this](const TCHAR* Name, const TCHAR* Source)
	{
		UMotionControllerComponent* MC = CreateDefaultSubobject<UMotionControllerComponent>(Name);
		MC->SetupAttachment(VROrigin);
		MC->MotionSource = FName(Source);
		return MC;
	};
	MotionControllerLeftGrip  = MakeController(TEXT("MotionControllerLeftGrip"),  TEXT("LeftGrip"));
	MotionControllerRightGrip = MakeController(TEXT("MotionControllerRightGrip"), TEXT("RightGrip"));
	MotionControllerLeftAim   = MakeController(TEXT("MotionControllerLeftAim"),   TEXT("LeftAim"));
	MotionControllerRightAim  = MakeController(TEXT("MotionControllerRightAim"),  TEXT("RightAim"));

	// --- Mains (memes assets que le template XR) -----------------------------
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandLeftMesh(TEXT("/Game/XRMannequins/Meshes/SKM_MannyXR_left.SKM_MannyXR_left"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandRightMesh(TEXT("/Game/XRMannequins/Meshes/SKM_MannyXR_right.SKM_MannyXR_right"));

	auto MakeHand = [&](const TCHAR* Name, UMotionControllerComponent* Parent, USkeletalMesh* Mesh)
	{
		USkeletalMeshComponent* Hand = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Hand->SetupAttachment(Parent);
		Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Mesh) Hand->SetSkeletalMeshAsset(Mesh);
		Hand->SetAnimInstanceClass(UShowroomHandAnimInstance::StaticClass()); // doigts animes en C++
		return Hand;
	};
	HandLeft  = MakeHand(TEXT("HandLeft"),  MotionControllerLeftGrip,  HandLeftMesh.Object);
	HandRight = MakeHand(TEXT("HandRight"), MotionControllerRightGrip, HandRightMesh.Object);

	// --- Lasers pour cliquer sur les menus -------------------------------------
	auto MakeInteraction = [this](const TCHAR* Name, UMotionControllerComponent* Parent, int32 PointerIndex)
	{
		UWidgetInteractionComponent* WI = CreateDefaultSubobject<UWidgetInteractionComponent>(Name);
		WI->SetupAttachment(Parent);
		WI->PointerIndex = PointerIndex;
		WI->InteractionDistance = 1000.f;
		WI->InteractionSource = EWidgetInteractionSource::World;
		WI->bShowDebug = false;
		WI->TraceChannel = ECC_GameTraceChannel1; // canal "3DWidget" du projet (bloque par les menus)
		return WI;
	};
	WidgetInteractionLeft  = MakeInteraction(TEXT("WidgetInteractionLeft"),  MotionControllerLeftAim,  0);
	WidgetInteractionRight = MakeInteraction(TEXT("WidgetInteractionRight"), MotionControllerRightAim, 1);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto MakeLaser = [&](const TCHAR* Name)
	{
		UStaticMeshComponent* Laser = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Laser->SetupAttachment(VROrigin);
		Laser->SetUsingAbsoluteLocation(true);
		Laser->SetUsingAbsoluteRotation(true);
		Laser->SetUsingAbsoluteScale(true);
		Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Laser->SetCastShadow(false);
		Laser->SetVisibility(false);
		if (CylinderMesh.Succeeded()) Laser->SetStaticMesh(CylinderMesh.Object);
		return Laser;
	};
	LaserLeft  = MakeLaser(TEXT("LaserLeft"));
	LaserRight = MakeLaser(TEXT("LaserRight"));

	// --- Arc de teleportation (effet Niagara du template) --------------------
	TeleportTrace = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportTrace"));
	TeleportTrace->SetupAttachment(VROrigin);
	TeleportTrace->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TraceFX(TEXT("/Game/XRFramework/VFX/NS_TeleportTrace.NS_TeleportTrace"));
	if (TraceFX.Succeeded()) TeleportTrace->SetAsset(TraceFX.Object);

	// Marqueur au sol de la destination (anneau du template), place en coordonnees monde.
	TeleportRing = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportRing"));
	TeleportRing->SetupAttachment(VROrigin);
	TeleportRing->SetUsingAbsoluteLocation(true);
	TeleportRing->SetUsingAbsoluteRotation(true);
	TeleportRing->SetAutoActivate(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RingFX(TEXT("/Game/XRFramework/VFX/NS_TeleportRing.NS_TeleportRing"));
	if (RingFX.Succeeded()) TeleportRing->SetAsset(RingFX.Object);
}

void AShowroomPawn::BeginPlay()
{
	Super::BeginPlay();

	const UShowroomSettings* Settings = UShowroomSettings::Get();
	HandLeft->SetRelativeTransform(Settings->HandOffsetLeft);
	HandRight->SetRelativeTransform(Settings->HandOffsetRight);
	Body->SetCapsuleRadius(Settings->PlayerCollisionRadius); // meme rayon que pour les murs

	ReplicatedTracking.OriginLocation = GetActorLocation();
	ReplicatedTracking.OriginYaw = GetActorRotation().Yaw;

	UpdateLocalOnlyComponents();
}

void AShowroomPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	// En reseau, le controleur arrive souvent APRES BeginPlay : on refait la config ici.
	UpdateLocalOnlyComponents();
}

void AShowroomPawn::UpdateLocalOnlyComponents()
{
	const bool bLocal = IsLocallyControlled();
	if (bLocal)
	{
		// Le sol reel = le sol du jeu.
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
		// Ecran du PC : affiche ce que voit le joueur (remplace l'ancien Blueprint VRSpectator).
		UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(ESpectatorScreenMode::SingleEyeCroppedToFill);
	}

	// Les lasers des autres joueurs ne doivent pas cliquer sur NOS menus.
	for (UWidgetInteractionComponent* WI : { WidgetInteractionLeft.Get(), WidgetInteractionRight.Get() })
	{
		WI->SetActive(bLocal);
		WI->SetComponentTickEnabled(bLocal);
	}
}

FVector AShowroomPawn::GetPawnViewLocation() const
{
	return Camera ? Camera->GetComponentLocation() : Super::GetPawnViewLocation();
}

// ============================================================================
// Input
// ============================================================================

void AShowroomPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogShowroom, Error, TEXT("ShowroomPawn : Enhanced Input n'est pas active."));
		return;
	}

	// 1) Les controles partages (crees au lancement du jeu, voir Input/ShowroomInput.cpp)
	Controls = UShowroomInput::Get();

	// 2) Les active pour ce joueur (normalement deja fait automatiquement : simple securite)
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(Controls->Context, /*Priority*/ 0);
		}
	}

	// 3) Branche chaque action sur une fonction
	const UShowroomInput* C = Controls;

	// Deplacement
	EIC->BindAction(C->Walk,     ETriggerEvent::Triggered, this, &AShowroomPawn::OnWalk);
	EIC->BindAction(C->Turn,     ETriggerEvent::Triggered, this, &AShowroomPawn::OnTurn);
	EIC->BindAction(C->Turn,     ETriggerEvent::Completed, this, &AShowroomPawn::OnTurnReleased);
	EIC->BindAction(C->Teleport, ETriggerEvent::Triggered, this, &AShowroomPawn::OnTeleportHeld);
	EIC->BindAction(C->Teleport, ETriggerEvent::Completed, this, &AShowroomPawn::OnTeleportReleased);
	EIC->BindAction(C->Teleport, ETriggerEvent::Canceled,  this, &AShowroomPawn::EndTeleportTrace);

	// Attraper : appui = prendre, relache = lacher
	EIC->BindAction(C->GrabLeft,  ETriggerEvent::Started,   this, &AShowroomPawn::OnGrabLeft);
	EIC->BindAction(C->GrabLeft,  ETriggerEvent::Completed, this, &AShowroomPawn::OnReleaseLeft);
	EIC->BindAction(C->GrabRight, ETriggerEvent::Started,   this, &AShowroomPawn::OnGrabRight);
	EIC->BindAction(C->GrabRight, ETriggerEvent::Completed, this, &AShowroomPawn::OnReleaseRight);

	// Menus
	EIC->BindAction(C->MenuToggleLeft,  ETriggerEvent::Started,   this, &AShowroomPawn::OnMenuToggleLeft);
	EIC->BindAction(C->MenuToggleRight, ETriggerEvent::Started,   this, &AShowroomPawn::OnMenuToggleRight);
	EIC->BindAction(C->MenuClickLeft,   ETriggerEvent::Started,   this, &AShowroomPawn::OnMenuPressLeft);
	EIC->BindAction(C->MenuClickLeft,   ETriggerEvent::Completed, this, &AShowroomPawn::OnMenuReleaseLeft);
	EIC->BindAction(C->MenuClickRight,  ETriggerEvent::Started,   this, &AShowroomPawn::OnMenuPressRight);
	EIC->BindAction(C->MenuClickRight,  ETriggerEvent::Completed, this, &AShowroomPawn::OnMenuReleaseRight);

	// Doigts : 0 = Grasp (grip), 1 = IndexCurl (gachette) ; + doigts poses (touch)
	for (const bool bLeft : { true, false })
	{
		UInputAction* Grasp = bLeft ? C->GraspLeft : C->GraspRight;
		UInputAction* Curl  = bLeft ? C->IndexCurlLeft : C->IndexCurlRight;
		UInputAction* Index = bLeft ? C->IndexTouchLeft : C->IndexTouchRight;
		UInputAction* Thumb = bLeft ? C->ThumbTouchLeft : C->ThumbTouchRight;
		for (const ETriggerEvent Event : { ETriggerEvent::Triggered, ETriggerEvent::Completed })
		{
			EIC->BindAction(Grasp, Event, this, &AShowroomPawn::OnHandAxis, bLeft, 0);
			EIC->BindAction(Curl,  Event, this, &AShowroomPawn::OnHandAxis, bLeft, 1);
			EIC->BindAction(Index, Event, this, &AShowroomPawn::OnHandTouch, bLeft, false);
			EIC->BindAction(Thumb, Event, this, &AShowroomPawn::OnHandTouch, bLeft, true);
		}
	}

	// Sans doigt sur la gachette ni pouce sur un bouton : main qui pointe + pouce leve.
	UpdatePointAndThumb(true);
	UpdatePointAndThumb(false);
}

// ============================================================================
// Marche et rotation
// ============================================================================

void AShowroomPawn::OnWalk(const FInputActionValue& Value)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	const FVector2D Stick = Value.Get<FVector2D>();
	if (Stick.IsNearlyZero())
	{
		return;
	}

	// Direction : regard ou manette gauche, mise a plat.
	const USceneComponent* Ref = Settings->bWalkFollowsHead ? static_cast<USceneComponent*>(Camera) : static_cast<USceneComponent*>(MotionControllerLeftAim);
	const FVector Forward = FVector(Ref->GetForwardVector().X, Ref->GetForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right   = FVector(Ref->GetRightVector().X,   Ref->GetRightVector().Y,   0.f).GetSafeNormal();
	FVector Delta = (Forward * Stick.Y + Right * Stick.X) * Settings->WalkSpeed * GetWorld()->GetDeltaSeconds();

	// Collision avec les murs : on balaie une sphere a hauteur de hanche.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowroomWalk), false, this);
	TArray<AActor*> Attached;
	GetAttachedActors(Attached, true, true); // les objets tenus en main
	Query.AddIgnoredActors(Attached);

	const FVector Head = Camera->GetComponentLocation();
	const FVector Feet(Head.X, Head.Y, GetActorLocation().Z);
	const FVector Up(0.f, 0.f, Settings->MaxStepHeight + Settings->PlayerCollisionRadius);
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Settings->PlayerCollisionRadius);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Feet + Up, Feet + Up + Delta, FQuat::Identity, ECC_Pawn, Sphere, Query))
	{
		// On glisse le long du mur.
		const FVector WallNormal = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.f).GetSafeNormal();
		const FVector Slide = FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal);
		Delta = Delta * Hit.Time;
		if (!GetWorld()->SweepSingleByChannel(Hit, Feet + Up + Delta, Feet + Up + Delta + Slide, FQuat::Identity, ECC_Pawn, Sphere, Query))
		{
			Delta += Slide;
		}
	}

	// Suivre le sol (escaliers, rampes). Pas de sol = on ne bouge pas.
	const FVector NewFeet = Feet + Delta;
	if (GetWorld()->LineTraceSingleByChannel(Hit, NewFeet + FVector(0, 0, Settings->MaxStepHeight), NewFeet - FVector(0, 0, 200.f), ECC_Visibility, Query))
	{
		Delta.Z = Hit.ImpactPoint.Z - Feet.Z;
		AddActorWorldOffset(Delta);
	}
}

void AShowroomPawn::OnTurn(const FInputActionValue& Value)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	const float X = Value.Get<FVector2D>().X; // marche aussi si l'action est en Axis1D

	if (Settings->TurnMode == EShowroomTurnMode::Smooth)
	{
		// On memorise juste le stick : la rotation est appliquee (et lissee) dans Tick.
		TurnInput = FMath::Abs(X) > 0.1f ? X : 0.f;
		return;
	}

	// Par crans : un cran par poussee du stick.
	if (FMath::Abs(X) < Settings->TurnDeadZone)
	{
		bTurnReady = true;
	}
	else if (bTurnReady)
	{
		bTurnReady = false;
		TurnBy(FMath::Sign(X) * Settings->SnapTurnDegrees);
	}
}

void AShowroomPawn::OnTurnReleased()
{
	bTurnReady = true;
	TurnInput = 0.f;
}

void AShowroomPawn::UpdateSmoothTurn(float DeltaSeconds)
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();
	if (Settings->TurnMode != EShowroomTurnMode::Smooth)
	{
		CurrentTurnRate = 0.f;
		return;
	}

	// Courbe douce : petits mouvements du stick = rotation lente et precise.
	const float Target = FMath::Sign(TurnInput) * FMath::Square(TurnInput) * Settings->SmoothTurnSpeed;

	// Acceleration / deceleration progressives : pas de demarrage ni d'arret brusque.
	CurrentTurnRate = FMath::FInterpTo(CurrentTurnRate, Target, DeltaSeconds, Settings->SmoothTurnAcceleration);
	if (FMath::Abs(CurrentTurnRate) > 0.5f)
	{
		TurnBy(CurrentTurnRate * DeltaSeconds);
	}
	else if (FMath::IsNearlyZero(Target))
	{
		CurrentTurnRate = 0.f;
	}
}

void AShowroomPawn::TurnBy(float YawDegrees)
{
	// On tourne autour de la tete, pas autour du centre de la piece.
	const FVector Pivot = Camera->GetComponentLocation();
	const FRotator Delta(0.f, YawDegrees, 0.f);
	const FVector NewLocation = Pivot + Delta.RotateVector(GetActorLocation() - Pivot);
	SetActorLocationAndRotation(NewLocation, GetActorRotation() + Delta);
}

void AShowroomPawn::TeleportPlayerTo(const FVector& Location)
{
	FVector HeadOffset = Camera->GetComponentLocation() - GetActorLocation();
	HeadOffset.Z = 0.f;
	SetActorLocation(Location - HeadOffset, false, nullptr, ETeleportType::TeleportPhysics);
}

// ============================================================================
// Teleportation
// ============================================================================

void AShowroomPawn::OnTeleportHeld(const FInputActionValue& /*Value*/)
{
	if (!UShowroomSettings::Get()->bTeleportEnabled)
	{
		return;
	}
	// La gachette droite sert aussi a cliquer : si on vise un menu, on ne lance pas la teleportation.
	if (!bTeleportTraceActive && WidgetInteractionRight->IsOverHitTestVisibleWidget())
	{
		return;
	}
	UpdateTeleportTrace();
}

void AShowroomPawn::OnTeleportReleased()
{
	if (bTeleportTraceActive && bValidTeleportLocation)
	{
		TeleportPlayerTo(ProjectedTeleportLocation);
	}
	EndTeleportTrace();
}

void AShowroomPawn::UpdateTeleportTrace()
{
	const UShowroomSettings* Settings = UShowroomSettings::Get();

	if (!bTeleportTraceActive)
	{
		bTeleportTraceActive = true;
		TeleportTrace->SetVisibility(true);
		TeleportTrace->Activate(true);
	}

	// 1) Arc parabolique depuis la manette droite
	FPredictProjectilePathParams Params(
		Settings->TeleportProjectileRadius,
		MotionControllerRightAim->GetComponentLocation(),
		MotionControllerRightAim->GetForwardVector() * Settings->TeleportLaunchSpeed,
		/*MaxSimTime*/ 2.f,
		ECC_WorldStatic,
		this);
	Params.SimFrequency = 15.f;

	FPredictProjectilePathResult Result;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	TArray<FVector> Points;
	Points.Reserve(Result.PathData.Num());
	for (const FPredictProjectilePathPointData& P : Result.PathData)
	{
		Points.Add(P.Location);
	}
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TeleportTrace, TEXT("User.PointArray"), Points);

	// 2) Le point touche est-il sur le NavMesh ?
	bValidTeleportLocation = false;
	if (bHit)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, Settings->TeleportNavMeshExtent))
			{
				bValidTeleportLocation = true;
				ProjectedTeleportLocation = NavLocation.Location;

				// Le NavMesh flotte un peu au-dessus du sol : on recolle au vrai sol.
				FHitResult Floor;
				FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowroomTeleportFloor), false, this);
				if (GetWorld()->LineTraceSingleByChannel(Floor, ProjectedTeleportLocation + FVector(0, 0, 50), ProjectedTeleportLocation - FVector(0, 0, 100), ECC_Visibility, Query))
				{
					ProjectedTeleportLocation = Floor.ImpactPoint;
				}
			}
		}
	}

	// Anneau au sol sur la destination
	if (bValidTeleportLocation)
	{
		TeleportRing->SetWorldLocation(ProjectedTeleportLocation);
		if (!TeleportRing->IsActive())
		{
			TeleportRing->SetVisibility(true);
			TeleportRing->Activate(true);
		}
	}
	else if (TeleportRing->IsActive())
	{
		TeleportRing->DeactivateImmediate();
		TeleportRing->SetVisibility(false);
	}
}

void AShowroomPawn::EndTeleportTrace()
{
	bTeleportTraceActive = false;
	bValidTeleportLocation = false;
	// Coupe net les effets : Deactivate() seul laisserait les particules deja emises
	// (l'arc resterait affiche apres la teleportation).
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TeleportTrace, TEXT("User.PointArray"), TArray<FVector>());
	TeleportTrace->DeactivateImmediate();
	TeleportTrace->SetVisibility(false);
	TeleportRing->DeactivateImmediate();
	TeleportRing->SetVisibility(false);
}

// ============================================================================
// Attraper (execute sur le serveur)
// ============================================================================

void AShowroomPawn::RequestGrab(bool bLeft)
{
	Server_Grab(bLeft); // sur le serveur (ou en solo) ca s'execute directement
}

void AShowroomPawn::RequestRelease(bool bLeft)
{
	Server_Release(bLeft);
}

UShowroomGrabComponent* AShowroomPawn::FindNearestGrabComponent(const UMotionControllerComponent* Hand) const
{
	const float Radius = UShowroomSettings::Get()->GrabRadius;
	const FVector HandLocation = Hand->GetComponentLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowroomGrab), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, HandLocation, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(Radius), Query);

	UShowroomGrabComponent* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	TSet<AActor*> Visited;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Visited.Contains(Actor)) continue;
		Visited.Add(Actor);

		TArray<UShowroomGrabComponent*> GrabComponents;
		Actor->GetComponents(GrabComponents);
		for (UShowroomGrabComponent* Grab : GrabComponents)
		{
			const bool bHeldByMe = (Grab == HeldLeft || Grab == HeldRight);
			if (Grab->IsHeld() && !bHeldByMe) continue; // tenu par un autre joueur

			const float DistSq = FVector::DistSquared(HandLocation, Grab->GetComponentLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Grab;
			}
		}
	}
	return Best;
}

void AShowroomPawn::Server_Grab_Implementation(bool bLeft)
{
	UMotionControllerComponent* Hand = bLeft ? MotionControllerLeftGrip : MotionControllerRightGrip;
	TObjectPtr<UShowroomGrabComponent>& Held = bLeft ? HeldLeft : HeldRight;
	TObjectPtr<UShowroomGrabComponent>& Other = bLeft ? HeldRight : HeldLeft;

	UShowroomGrabComponent* Grab = FindNearestGrabComponent(Hand);
	if (!Grab || !Grab->TryGrab(Hand))
	{
		return;
	}

	if (Other == Grab) Other = nullptr; // passe d'une main a l'autre
	Held = Grab;

	if (UPrimitiveComponent* Prim = Grab->GetGrabbedPrimitive())
	{
		Multicast_SetPhysics(Prim, false);
	}
}

void AShowroomPawn::Server_Release_Implementation(bool bLeft)
{
	TObjectPtr<UShowroomGrabComponent>& Held = bLeft ? HeldLeft : HeldRight;
	if (!Held)
	{
		return;
	}

	if (Held->GetHeldByHand() == (bLeft ? MotionControllerLeftGrip : MotionControllerRightGrip))
	{
		Held->TryRelease();
		if (UPrimitiveComponent* Prim = Held->GetGrabbedPrimitive())
		{
			Multicast_SetPhysics(Prim, Prim->IsSimulatingPhysics());
		}
	}
	Held = nullptr;
}

void AShowroomPawn::Multicast_SetPhysics_Implementation(UPrimitiveComponent* Component, bool bSimulate)
{
	if (Component && !HasAuthority())
	{
		Component->SetSimulatePhysics(bSimulate);
	}
}

// ============================================================================
// Menus
// ============================================================================

void AShowroomPawn::ToggleHandMenu(bool bLeft)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (HandMenu)
	{
		const bool bSameHand = (bHandMenuOnLeft == bLeft);
		HandMenu->Destroy();
		HandMenu = nullptr;
		if (bSameHand) return; // simple fermeture
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	HandMenu = GetWorld()->SpawnActor<AHandMenu>(AHandMenu::StaticClass(), GetActorTransform(), Params);
	if (HandMenu)
	{
		// Sur la manette (grip), un peu au-dessus, face au joueur.
		HandMenu->AttachToComponent(bLeft ? MotionControllerLeftGrip : MotionControllerRightGrip, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		HandMenu->SetActorRelativeLocation(FVector(5.f, 0.f, 8.f));
		HandMenu->SetActorRelativeRotation(FRotator(30.f, 180.f, 0.f));
		bHandMenuOnLeft = bLeft;
	}
}

void AShowroomPawn::OnMenuPressLeft()
{
	if (!bMenuPressedLeft) { bMenuPressedLeft = true; WidgetInteractionLeft->PressPointerKey(EKeys::LeftMouseButton); }
}
void AShowroomPawn::OnMenuReleaseLeft()
{
	if (bMenuPressedLeft) { bMenuPressedLeft = false; WidgetInteractionLeft->ReleasePointerKey(EKeys::LeftMouseButton); }
}
void AShowroomPawn::OnMenuPressRight()
{
	if (!bMenuPressedRight) { bMenuPressedRight = true; WidgetInteractionRight->PressPointerKey(EKeys::LeftMouseButton); }
}
void AShowroomPawn::OnMenuReleaseRight()
{
	if (bMenuPressedRight) { bMenuPressedRight = false; WidgetInteractionRight->ReleasePointerKey(EKeys::LeftMouseButton); }
}

void AShowroomPawn::UpdateLaser(UWidgetInteractionComponent* Interaction, UStaticMeshComponent* Laser) const
{
	const bool bPointing = Interaction->IsActive() && Interaction->IsOverHitTestVisibleWidget();
	Laser->SetVisibility(bPointing);
	if (!bPointing)
	{
		return;
	}

	const FVector Start = Interaction->GetComponentLocation();
	const FVector End = Interaction->GetLastHitResult().ImpactPoint;
	const FVector Dir = End - Start;
	const float Length = Dir.Size();

	// Le cylindre du moteur fait 100 cm de haut, centre sur son milieu, axe Z.
	Laser->SetWorldLocationAndRotation((Start + End) * 0.5f, FRotationMatrix::MakeFromZ(Dir).Rotator());
	Laser->SetWorldScale3D(FVector(0.005f, 0.005f, Length / 100.f));
}

// ============================================================================
// Animation des doigts
// ============================================================================

void AShowroomPawn::OnHandAxis(const FInputActionInstance& Instance, bool bLeft, int32 PoseIndex)
{
	const float Value = (Instance.GetTriggerEvent() == ETriggerEvent::Completed) ? 0.f : Instance.GetValue().Get<float>();
	FShowroomHandPose& Pose = PoseFor(LeftPose, RightPose, bLeft);
	if (PoseIndex == 0) Pose.Grasp = Value;
	else                Pose.IndexCurl = Value;
}

void AShowroomPawn::OnHandTouch(const FInputActionInstance& Instance, bool bLeft, bool bThumb)
{
	const bool bTouching = Instance.GetTriggerEvent() != ETriggerEvent::Completed && Instance.GetValue().Get<bool>();
	bool* Touch = bThumb ? bThumbTouch : bIndexTouch;
	Touch[bLeft ? 0 : 1] = bTouching;
	UpdatePointAndThumb(bLeft);
}

void AShowroomPawn::UpdatePointAndThumb(bool bLeft)
{
	// Doigt leve de la gachette = on pointe ; pouce leve des boutons = pouce en l'air.
	const int32 Side = bLeft ? 0 : 1;
	FShowroomHandPose& Pose = PoseFor(LeftPose, RightPose, bLeft);
	Pose.Point   = bIndexTouch[Side] ? 0.f : 1.f;
	Pose.ThumbUp = bThumbTouch[Side] ? 0.f : 1.f;
}

void AShowroomPawn::ApplyHandPose(USkeletalMeshComponent* Hand, const FShowroomHandPose& Pose, bool bMirror) const
{
	UShowroomHandAnimInstance* Anim = Hand ? Cast<UShowroomHandAnimInstance>(Hand->GetAnimInstance()) : nullptr;
	if (!Anim) return;
	Anim->PoseAlphaGrasp     = Pose.Grasp;
	Anim->PoseAlphaIndexCurl = Pose.IndexCurl;
	Anim->PoseAlphaPoint     = Pose.Point;
	Anim->PoseAlphaThumbUp   = Pose.ThumbUp;
	Anim->bMirror            = bMirror;
}

// ============================================================================
// Reseau : tete + mains
// ============================================================================

void AShowroomPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Le proprietaire connait deja sa propre position : on ne la lui renvoie pas.
	DOREPLIFETIME_CONDITION(AShowroomPawn, ReplicatedTracking, COND_SkipOwner);
}

FShowroomTrackingData AShowroomPawn::CaptureTracking() const
{
	FShowroomTrackingData Data;
	Data.OriginLocation = GetActorLocation();
	Data.OriginYaw      = GetActorRotation().Yaw;
	Data.HeadLocation   = Camera->GetRelativeLocation();
	Data.HeadRotation   = Camera->GetRelativeRotation();
	Data.LeftLocation   = MotionControllerLeftGrip->GetRelativeLocation();
	Data.LeftRotation   = MotionControllerLeftGrip->GetRelativeRotation();
	Data.RightLocation  = MotionControllerRightGrip->GetRelativeLocation();
	Data.RightRotation  = MotionControllerRightGrip->GetRelativeRotation();
	Data.LeftPose       = LeftPose;
	Data.RightPose      = RightPose;
	return Data;
}

void AShowroomPawn::Server_UpdateTracking_Implementation(const FShowroomTrackingData& Data)
{
	ReplicatedTracking = Data;
}

void AShowroomPawn::ApplyRemoteTracking(float DeltaSeconds)
{
	const FShowroomTrackingData& T = ReplicatedTracking;
	const float Alpha = 1.f - FMath::Exp(-UShowroomSettings::Get()->RemoteSmoothing * DeltaSeconds);

	auto Blend = [Alpha](USceneComponent* Comp, const FVector& Loc, const FRotator& Rot)
	{
		const FVector NewLoc = FMath::Lerp(Comp->GetRelativeLocation(), Loc, Alpha);
		const FQuat NewRot = FQuat::Slerp(Comp->GetRelativeRotation().Quaternion(), Rot.Quaternion(), Alpha);
		Comp->SetRelativeLocationAndRotation(NewLoc, NewRot);
	};

	// Grand saut (teleportation) : on se place directement au lieu de "glisser" a travers la piece.
	const bool bTeleported = FVector::DistSquared(GetActorLocation(), FVector(T.OriginLocation)) > FMath::Square(200.f);
	const float OriginAlpha = bTeleported ? 1.f : Alpha;
	const FVector NewOrigin = FMath::Lerp(GetActorLocation(), FVector(T.OriginLocation), OriginAlpha);
	const FQuat NewYaw = FQuat::Slerp(GetActorQuat(), FRotator(0.f, T.OriginYaw, 0.f).Quaternion(), OriginAlpha);
	SetActorLocationAndRotation(NewOrigin, NewYaw);

	Blend(Camera, T.HeadLocation, T.HeadRotation);
	Blend(MotionControllerLeftGrip, T.LeftLocation, T.LeftRotation);
	Blend(MotionControllerRightGrip, T.RightLocation, T.RightRotation);
	// Les lasers des autres joueurs suivent leurs mains.
	MotionControllerLeftAim->SetRelativeTransform(MotionControllerLeftGrip->GetRelativeTransform());
	MotionControllerRightAim->SetRelativeTransform(MotionControllerRightGrip->GetRelativeTransform());
}

// ============================================================================
// Tick
// ============================================================================

void AShowroomPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UShowroomSettings* Settings = UShowroomSettings::Get();

	if (IsLocallyControlled())
	{
		UpdateSmoothTurn(DeltaSeconds);
		UpdateLaser(WidgetInteractionLeft, LaserLeft);
		UpdateLaser(WidgetInteractionRight, LaserRight);

		// Envoi de la position au serveur, X fois par seconde.
		TrackingSendAccumulator += DeltaSeconds;
		if (TrackingSendAccumulator >= 1.f / Settings->TrackingSendRate)
		{
			TrackingSendAccumulator = 0.f;
			const FShowroomTrackingData Data = CaptureTracking();
			if (HasAuthority())
			{
				ReplicatedTracking = Data;
			}
			else
			{
				Server_UpdateTracking(Data);
			}
		}

		ApplyHandPose(HandLeft, LeftPose, Settings->bMirrorLeftHandAnimation);
		ApplyHandPose(HandRight, RightPose, false);
	}
	else
	{
		ApplyRemoteTracking(DeltaSeconds);
		ApplyHandPose(HandLeft, ReplicatedTracking.LeftPose, Settings->bMirrorLeftHandAnimation);
		ApplyHandPose(HandRight, ReplicatedTracking.RightPose, false);
	}

	// Le "corps" suit la tete (pour les DetectionZone).
	const FVector Head = Camera->GetComponentLocation();
	Body->SetWorldLocation(FVector(Head.X, Head.Y, GetActorLocation().Z + Body->GetScaledCapsuleHalfHeight()));
}
