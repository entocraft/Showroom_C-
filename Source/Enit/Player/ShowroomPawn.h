#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShowroomPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UMotionControllerComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UWidgetInteractionComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UShowroomInput;
class AHandMenu;
class UShowroomGrabComponent;
class UPrimitiveComponent;
struct FInputActionValue;
struct FInputActionInstance;

/** Poses de doigts d'une main (0 = relache, 1 = applique). */
USTRUCT(BlueprintType)
struct FShowroomHandPose
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float Grasp = 0.f;
	UPROPERTY(BlueprintReadOnly) float IndexCurl = 0.f;
	UPROPERTY(BlueprintReadOnly) float Point = 0.f;
	UPROPERTY(BlueprintReadOnly) float ThumbUp = 0.f;
};

/** Ce que chaque joueur envoie au serveur (et que le serveur renvoie aux autres joueurs). */
USTRUCT()
struct FShowroomTrackingData
{
	GENERATED_BODY()

	// Origine VR (le "sol" du joueur) en coordonnees monde
	UPROPERTY() FVector_NetQuantize10 OriginLocation = FVector::ZeroVector;
	UPROPERTY() float OriginYaw = 0.f;

	// Tete et mains, relatives a l'origine VR
	UPROPERTY() FVector_NetQuantize10 HeadLocation = FVector::ZeroVector;
	UPROPERTY() FRotator HeadRotation = FRotator::ZeroRotator;
	UPROPERTY() FVector_NetQuantize10 LeftLocation = FVector::ZeroVector;
	UPROPERTY() FRotator LeftRotation = FRotator::ZeroRotator;
	UPROPERTY() FVector_NetQuantize10 RightLocation = FVector::ZeroVector;
	UPROPERTY() FRotator RightRotation = FRotator::ZeroRotator;

	UPROPERTY() FShowroomHandPose LeftPose;
	UPROPERTY() FShowroomHandPose RightPose;
};

/**
 * Remplace BP_XRPawn : le joueur VR.
 *
 *  - Marche au joystick (IA_Walk), rotation par crans ou continue (IA_Turn)
 *  - Teleportation (IA_Move)
 *  - Attraper / lacher (IA_Grab_*), cote serveur pour le multijoueur
 *  - Laser pour cliquer sur les menus (IA_Menu_Interact_*), menu de main (IA_Menu_Toggle_*)
 *  - Animation des doigts (UShowroomHandAnimInstance)
 *  - Replication reseau de la tete et des mains
 *
 * Les valeurs (vitesses, angles...) sont dans Project Settings > Game > Showroom.
 * Les boutons des manettes sont definis dans Input/ShowroomInput.cpp.
 */
UCLASS()
class ENIT_API AShowroomPawn : public APawn
{
	GENERATED_BODY()

public:
	AShowroomPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual FVector GetPawnViewLocation() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ------------------------------------------------------------------
	// Composants
	// ------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UCameraComponent> Camera;

	/** Petit "corps" invisible qui suit la tete : sert a declencher les DetectionZone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UCapsuleComponent> Body;

	/** Tete visible par les AUTRES joueurs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> RemoteHead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UMotionControllerComponent> MotionControllerLeftGrip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UMotionControllerComponent> MotionControllerRightGrip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UMotionControllerComponent> MotionControllerLeftAim;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UMotionControllerComponent> MotionControllerRightAim;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<USkeletalMeshComponent> HandLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<USkeletalMeshComponent> HandRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetInteractionComponent> WidgetInteractionLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetInteractionComponent> WidgetInteractionRight;

	/** Rayons laser visibles quand on pointe un menu. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> LaserLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> LaserRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UNiagaraComponent> TeleportTrace;

	/** Marqueur au sol de la destination de teleportation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UNiagaraComponent> TeleportRing;

	/** Les controles (actions + touches), crees en C++ : voir Input/ShowroomInput.cpp. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UShowroomInput> Controls;

	// ------------------------------------------------------------------
	// Fonctions utiles depuis un Blueprint
	// ------------------------------------------------------------------

	/** Deplace le joueur pour que sa TETE arrive au-dessus de Location (Location = point au sol). */
	UFUNCTION(BlueprintCallable, Category = "Showroom")
	void TeleportPlayerTo(const FVector& Location);

	/** Le laser d'interface d'une main (pour les ecrans flottants...). */
	UWidgetInteractionComponent* GetWidgetInteraction(bool bLeft) const { return bLeft ? WidgetInteractionLeft : WidgetInteractionRight; }

	/** La gachette "clic" de la main de ce laser est-elle maintenue ? */
	bool IsMenuClickHeld(const UWidgetInteractionComponent* Interaction) const
	{
		return Interaction == WidgetInteractionLeft ? bMenuPressedLeft : Interaction == WidgetInteractionRight ? bMenuPressedRight : false;
	}

	/** Tourne le joueur autour de sa tete. */
	UFUNCTION(BlueprintCallable, Category = "Showroom")
	void TurnBy(float YawDegrees);

	UFUNCTION(BlueprintPure, Category = "Showroom")
	const FShowroomHandPose& GetHandPose(bool bLeft) const { return bLeft ? LeftPose : RightPose; }

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	void UpdateLocalOnlyComponents();

	// --- Input ---
	void OnWalk(const FInputActionValue& Value);
	void OnTurn(const FInputActionValue& Value);
	void OnTurnReleased();
	void UpdateSmoothTurn(float DeltaSeconds);
	void OnTeleportHeld(const FInputActionValue& Value);
	void OnTeleportReleased();
	void OnGrabLeft()         { RequestGrab(true); }
	void OnReleaseLeft()      { RequestRelease(true); }
	void OnGrabRight()        { RequestGrab(false); }
	void OnReleaseRight()     { RequestRelease(false); }
	void OnMenuToggleLeft()   { ToggleHandMenu(true); }
	void OnMenuToggleRight()  { ToggleHandMenu(false); }
	void OnMenuPressLeft();
	void OnMenuReleaseLeft();
	void OnMenuPressRight();
	void OnMenuReleaseRight();
	void OnHandAxis(const FInputActionInstance& Instance, bool bLeft, int32 PoseIndex);
	void OnHandTouch(const FInputActionInstance& Instance, bool bLeft, bool bThumb);
	void UpdatePointAndThumb(bool bLeft);

	// --- Teleportation ---
	void UpdateTeleportTrace();
	void EndTeleportTrace();

	// --- Attraper ---
	void RequestGrab(bool bLeft);
	void RequestRelease(bool bLeft);
	UShowroomGrabComponent* FindNearestGrabComponent(const UMotionControllerComponent* Hand) const;

	UFUNCTION(Server, Reliable)
	void Server_Grab(bool bLeft);

	UFUNCTION(Server, Reliable)
	void Server_Release(bool bLeft);

	/** Active / desactive la physique de l'objet chez tous les joueurs. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPhysics(UPrimitiveComponent* Component, bool bSimulate);

	// --- Menu ---
	void ToggleHandMenu(bool bLeft);
	void UpdateLaser(UWidgetInteractionComponent* Interaction, UStaticMeshComponent* Laser) const;

	// --- Animation des mains ---
	void ApplyHandPose(USkeletalMeshComponent* Hand, const FShowroomHandPose& Pose, bool bMirror) const;

	// --- Reseau ---
	FShowroomTrackingData CaptureTracking() const;
	void ApplyRemoteTracking(float DeltaSeconds);

	UFUNCTION(Server, Unreliable)
	void Server_UpdateTracking(const FShowroomTrackingData& Data);

	UPROPERTY(Replicated)
	FShowroomTrackingData ReplicatedTracking;

	// --- Etat ---
	UPROPERTY(Transient) TObjectPtr<UShowroomGrabComponent> HeldLeft;
	UPROPERTY(Transient) TObjectPtr<UShowroomGrabComponent> HeldRight;
	UPROPERTY(Transient) TObjectPtr<AHandMenu> HandMenu;

	FShowroomHandPose LeftPose;
	FShowroomHandPose RightPose;
	bool bIndexTouch[2] = { false, false }; // [0] = gauche, [1] = droite
	bool bThumbTouch[2] = { false, false };

	bool bTeleportTraceActive = false;
	bool bValidTeleportLocation = false;
	FVector ProjectedTeleportLocation = FVector::ZeroVector;

	float TurnInput = 0.f;        // valeur du stick de rotation (-1..1)
	float CurrentTurnRate = 0.f;  // vitesse de rotation actuelle (deg/s), lissee
	bool bTurnReady = true;       // pour la rotation par crans : il faut relacher le stick entre deux crans
	bool bHandMenuOnLeft = false;
	bool bMenuPressedLeft = false;
	bool bMenuPressedRight = false;
	float TrackingSendAccumulator = 0.f;
};
