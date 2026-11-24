#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Experience/ExperienceTypes.h"
#include "ExperienceHighlight.generated.h"

class USpotLightComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UMaterialInstanceDynamic;

/**
 * Mise en valeur d'un objet du niveau pendant une etape d'experience (creee par la DetectionZone) :
 *   - un projecteur au-dessus de l'objet,
 *   - un halo lumineux au sol autour de lui (qui pulse doucement),
 *   - une etiquette flottante (nom + petit texte) tournee vers le joueur.
 * L'objet lui-meme n'est pas modifie. Suit l'objet s'il est deplace (ex : attrape).
 * Reglages : Project Settings > Game > Showroom > Mise en valeur.
 */
UCLASS(NotPlaceable)
class ENIT_API AExperienceHighlight : public AActor
{
	GENERATED_BODY()

public:
	AExperienceHighlight();

	/** Met en valeur Highlight.Actor apres Delay secondes. */
	static AExperienceHighlight* Spawn(UWorld* World, const FExperienceHighlightInfo& Highlight, float Delay);

	/** S'eteint en fondu puis se detruit. */
	void Dismiss();

	AActor* GetTarget() const { return Target.Get(); }

	/** Centre et taille actuels de l'objet mis en valeur. */
	bool GetTargetBounds(FVector& OutCenter, FVector& OutExtent) const;

protected:
	virtual void PreInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Begin();
	void Follow();
	void Apply(float Brightness);

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<USpotLightComponent> Light;

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<UStaticMeshComponent> Halo;

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<UWidgetComponent> Label;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HaloMaterial;

	TWeakObjectPtr<AActor> Target;
	FText LabelText;
	FText DetailsText;

	float Delay = 0.f;
	float Alpha = 0.f;
	float Time = 0.f;
	bool bStarted = false;
	bool bDismissed = false;
};
