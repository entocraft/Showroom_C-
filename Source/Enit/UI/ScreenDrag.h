#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ScreenDrag.generated.h"

class UWidgetComponent;
class UWidgetInteractionComponent;

/** Un acteur qu'on peut deplacer avec une barre de prise (ecran flottant, fiche d'experience...). */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UShowroomDraggable : public UInterface
{
	GENERATED_BODY()
};

class ENIT_API IShowroomDraggable
{
	GENERATED_BODY()

public:
	virtual void BeginDrag() = 0;
	virtual void EndDrag() = 0;
};

/**
 * Deplacement d'un acteur au bout du laser d'une manette, tant que la gachette est maintenue.
 * Utilise par les ecrans flottants et la fiche d'experience.
 */
struct ENIT_API FScreenDrag
{
	/** Commence si un laser vise la poignee (Handle). Renvoie true si le deplacement a commence. */
	bool Begin(const UObject* WorldContext, const UWidgetComponent* Handle, const FVector& ActorLocation);

	/**
	 * Deplace l'acteur pour que la poignee suive le bout du laser.
	 * Renvoie false quand la gachette est relachee (le deplacement est fini).
	 */
	bool Update(AActor* Actor, float DeltaSeconds);

	void End() { Hand = nullptr; }
	bool IsActive() const { return Hand.IsValid(); }

private:
	TWeakObjectPtr<UWidgetInteractionComponent> Hand;
	float Distance = 0.f;
	FVector HandleOffset = FVector::ZeroVector; // acteur - poignee (en monde)
};
