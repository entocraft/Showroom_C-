#include "UI/ScreenDrag.h"
#include "Player/ShowroomPawn.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float DragFollowSpeed = 14.f; // lissage pendant le deplacement
}

bool FScreenDrag::Begin(const UObject* WorldContext, const UWidgetComponent* Handle, const FVector& ActorLocation)
{
	const AShowroomPawn* Pawn = Cast<AShowroomPawn>(UGameplayStatics::GetPlayerPawn(WorldContext, 0));
	if (!Pawn || !Handle)
	{
		return false;
	}
	// Quelle main vise la poignee ?
	for (const bool bLeft : { false, true })
	{
		UWidgetInteractionComponent* Candidate = Pawn->GetWidgetInteraction(bLeft);
		if (Candidate && Candidate->GetHoveredWidgetComponent() == Handle)
		{
			Hand = Candidate;
			Distance = FVector::Dist(Candidate->GetComponentLocation(), Handle->GetComponentLocation());
			HandleOffset = ActorLocation - Handle->GetComponentLocation();
			return true;
		}
	}
	return false;
}

bool FScreenDrag::Update(AActor* Actor, float DeltaSeconds)
{
	UWidgetInteractionComponent* Interaction = Hand.Get();
	const AShowroomPawn* Pawn = Actor ? Cast<AShowroomPawn>(UGameplayStatics::GetPlayerPawn(Actor, 0)) : nullptr;
	if (!Interaction || !Pawn || !Pawn->IsMenuClickHeld(Interaction))
	{
		End(); // gachette relachee (meme si le laser a quitte la poignee)
		return false;
	}
	// La poignee suit le bout du laser.
	const FVector HandleTarget = Interaction->GetComponentLocation() + Interaction->GetForwardVector() * Distance;
	const float Lerp = 1.f - FMath::Exp(-DragFollowSpeed * DeltaSeconds);
	Actor->SetActorLocation(FMath::Lerp(Actor->GetActorLocation(), HandleTarget + HandleOffset, Lerp));
	return true;
}
