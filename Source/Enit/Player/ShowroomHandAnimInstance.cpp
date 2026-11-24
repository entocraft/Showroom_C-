#include "Player/ShowroomHandAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/MirrorDataTable.h"
#include "AnimationRuntime.h"
#include "UObject/ConstructorHelpers.h"

UShowroomHandAnimInstance::UShowroomHandAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/XRMannequins/Animations/A_MannequinsXR_Idle_Right.A_MannequinsXR_Idle_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Grasp(TEXT("/Game/XRMannequins/Animations/A_MannequinsXR_Grasp_Right.A_MannequinsXR_Grasp_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Curl(TEXT("/Game/XRMannequins/Animations/A_MannequinsXR_IndexCurl_Right.A_MannequinsXR_IndexCurl_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Point(TEXT("/Game/XRMannequins/Animations/A_MannequinsXR_Point_Right.A_MannequinsXR_Point_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Thumb(TEXT("/Game/XRMannequins/Animations/A_MannequinsXR_ThumbUp_Right.A_MannequinsXR_ThumbUp_Right"));
	static ConstructorHelpers::FObjectFinder<UMirrorDataTable> Mirror(TEXT("/Game/XRMannequins/Animations/MDT_MannequinsXR.MDT_MannequinsXR"));

	IdleAnim      = Idle.Object;
	GraspAnim     = Grasp.Object;
	IndexCurlAnim = Curl.Object;
	PointAnim     = Point.Object;
	ThumbUpAnim   = Thumb.Object;
	MirrorTable   = Mirror.Object;
}

void FShowroomHandAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	// Copie (thread du jeu) de ce dont le thread d'animation a besoin.
	const UShowroomHandAnimInstance* Hand = CastChecked<UShowroomHandAnimInstance>(InAnimInstance);
	Idle           = Hand->IdleAnim;
	Grasp          = Hand->GraspAnim;
	IndexCurl      = Hand->IndexCurlAnim;
	Point          = Hand->PointAnim;
	ThumbUp        = Hand->ThumbUpAnim;
	MirrorTable    = Hand->MirrorTable;
	AlphaGrasp     = FMath::Clamp(Hand->PoseAlphaGrasp, 0.f, 1.f);
	AlphaIndexCurl = FMath::Clamp(Hand->PoseAlphaIndexCurl, 0.f, 1.f);
	AlphaPoint     = FMath::Clamp(Hand->PoseAlphaPoint, 0.f, 1.f);
	AlphaThumbUp   = FMath::Clamp(Hand->PoseAlphaThumbUp, 0.f, 1.f);
	bMirror        = Hand->bMirror;
}

bool FShowroomHandAnimProxy::Evaluate(FPoseContext& Output)
{
	if (!Idle)
	{
		return false; // pose de reference
	}

	// Lit la premiere frame d'une animation (ce sont des poses fixes).
	auto ExtractPose = [](const UAnimSequence* Anim, FPoseContext& Into)
	{
		FAnimationPoseData PoseData(Into);
		Anim->GetAnimationPose(PoseData, FAnimExtractContext(0.0));
	};

	// Quels os appartiennent a l'index / au pouce ? (d'apres leur nom)
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& RefSkeleton = Bones.GetReferenceSkeleton();
	auto BoneStartsWith = [&](FCompactPoseBoneIndex Index, const TCHAR* Prefix)
	{
		const int32 MeshIndex = Bones.MakeMeshPoseIndex(Index).GetInt();
		return RefSkeleton.GetBoneName(MeshIndex).ToString().StartsWith(Prefix);
	};

	// Melange "Anim" dans Output avec le poids Alpha, seulement sur les os qui commencent par Prefix
	// (Prefix vide = tous les os).
	auto BlendIn = [&](const UAnimSequence* Anim, float Alpha, const TCHAR* Prefix)
	{
		if (!Anim || Alpha <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		FPoseContext Other(this);
		ExtractPose(Anim, Other);
		for (const FCompactPoseBoneIndex Index : Output.Pose.ForEachBoneIndex())
		{
			if (*Prefix == 0 || BoneStartsWith(Index, Prefix))
			{
				Output.Pose[Index].BlendWith(Other.Pose[Index], Alpha);
			}
		}
	};

	ExtractPose(Idle, Output);
	BlendIn(Grasp,     AlphaGrasp,     TEXT(""));
	BlendIn(IndexCurl, AlphaIndexCurl, TEXT("index_"));
	BlendIn(Point,     AlphaPoint,     TEXT("index_"));
	BlendIn(ThumbUp,   AlphaThumbUp,   TEXT("thumb_"));

	if (bMirror && MirrorTable)
	{
		FAnimationRuntime::MirrorPose(Output.Pose, *MirrorTable);
	}
	return true;
}
