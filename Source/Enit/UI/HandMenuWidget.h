#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "HandMenuWidget.generated.h"

class UButton;

/**
 * Remplace WBP_Menu : le petit menu qui s'ouvre dans la main (bouton Menu / B).
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_HandMenu
 * (garder les noms RestartButton et QuitButton).
 */
UCLASS()
class ENIT_API UHandMenuWidget : public UShowroomWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void OnRestartClicked();
	UFUNCTION() void OnQuitClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;
};
