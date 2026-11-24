#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "ShowroomHandAnimInstance.generated.h"

class UAnimSequence;
class UMirrorDataTable;

/** Les donnees que le thread d'animation lit (copiees depuis l'AnimInstance a chaque frame). */
USTRUCT()
struct FShowroomHandAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FShowroomHandAnimProxy() = default;
	explicit FShowroomHandAnimProxy(UAnimInstance* InInstance) : FAnimInstanceProxy(InInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	const UAnimSequence* Idle = nullptr;
	const UAnimSequence* Grasp = nullptr;
	const UAnimSequence* IndexCurl = nullptr;
	const UAnimSequence* Point = nullptr;
	const UAnimSequence* ThumbUp = nullptr;
	const UMirrorDataTable* MirrorTable = nullptr;

	float AlphaGrasp = 0.f;
	float AlphaIndexCurl = 0.f;
	float AlphaPoint = 0.f;
	float AlphaThumbUp = 0.f;
	bool bMirror = false;
};

/**
 * Remplace l'Animation Blueprint ABP_MannequinsXR : anime les doigts en C++.
 *
 *   Pose de base = Idle, melangee avec Grasp (poing ferme) selon PoseAlphaGrasp.
 *   Os de l'index ("index_*") : + IndexCurl puis + Point.
 *   Os du pouce  ("thumb_*")  : + ThumbUp.
 *   Main gauche : la pose (faite pour la main droite) est inversee via la Mirror Data Table.
 *
 * Le pawn ecrit PoseAlpha* (0..1) a chaque frame.
 */
UCLASS(Transient)
class ENIT_API UShowroomHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UShowroomHandAnimInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poses") float PoseAlphaGrasp = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poses") float PoseAlphaIndexCurl = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poses") float PoseAlphaPoint = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poses") float PoseAlphaThumbUp = 0.f;

	/** Inverser gauche/droite (pour la main gauche). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poses") bool bMirror = false;

	// Les animations utilisees (assets du dossier XRMannequins/Animations).
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UAnimSequence> GraspAnim;
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UAnimSequence> IndexCurlAnim;
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UAnimSequence> PointAnim;
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UAnimSequence> ThumbUpAnim;
	UPROPERTY(EditAnywhere, Category = "Assets") TObjectPtr<UMirrorDataTable> MirrorTable;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FShowroomHandAnimProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }

	friend struct FShowroomHandAnimProxy;
};
