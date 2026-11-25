#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "HandMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class UWidgetSwitcher;
class UShowroomSessions;

/**
 * MENU DE LA MAIN (bouton Menu / Y / B) : deux pages.
 *   - Menu     : etat du multijoueur, Sessions, Recommencer, Quitter.
 *   - Sessions : Heberger, Rechercher, la liste des sessions du reseau local (viser + gachette pour rejoindre),
 *                Quitter la session.
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_HandMenu. Garder les noms :
 * Pages, SessionSummary, SessionsButton, RestartButton, QuitButton, BackButton, SessionStatus,
 * HostButton, SearchButton, SessionList, LeaveButton.
 */
UCLASS()
class ENIT_API UHandMenuWidget : public UShowroomWidget
{
	GENERATED_BODY()

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void OnRestartClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSessionsClicked();
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnHostClicked();
	UFUNCTION() void OnSearchClicked();
	UFUNCTION() void OnLeaveClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SessionSummary;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> SessionsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> RestartButton;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> BackButton;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SessionStatus;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> HostButton;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> SearchButton;
	/** Les sessions trouvees sont ajoutees ici (un bouton par session). */
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> SessionList;
	UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional)) TObjectPtr<UButton> LeaveButton;

private:
	UShowroomSessions* GetSessions() const;
	void RefreshSessions();
	void ShowPage(int32 Index);
	void JoinFound(int32 Index);

	/** Boutons des sessions trouvees -> index de la session. */
	TMap<TWeakObjectPtr<UButton>, int32> JoinButtons;
	FDelegateHandle ChangedHandle;
	float RefreshTimer = 0.f;
};
