#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "FloatingScreenGrabBar.generated.h"

class UButton;

/**
 * La petite barre arrondie sous un ecran flottant : viser + gachette maintenue = deplacer l'ecran.
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_GrabBar (garder le nom GrabButton).
 */
UCLASS()
class ENIT_API UFloatingScreenGrabBar : public UShowroomWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandlePressed();
	UFUNCTION() void HandleReleased();

	UPROPERTY(BlueprintReadOnly, Category = "Barre", meta = (BindWidgetOptional))
	TObjectPtr<UButton> GrabButton;
};
