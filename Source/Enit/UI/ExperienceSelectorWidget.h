#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceSelectorWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class UExperienceEntryWidget;
class ADetectionZone;
class ANavigatorSystem;

/**
 * Ecran "Choisir une experience" : la liste de toutes les zones d'experience du niveau.
 * Un clic sur une experience -> le trait GPS au sol guide le joueur jusqu'a elle.
 *
 * A mettre comme Content d'un Ecran flottant. Mise en page modifiable sans code :
 * Content/Showroom/UI/WBP_ExperienceSelector (garder les noms TitleText, EntryList, StatusText, StopButton).
 * L'apparence de chaque ligne : WBP_ExperienceEntry (choisi dans "Entry Class").
 */
UCLASS()
class ENIT_API UExperienceSelectorWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	/** Le Widget Blueprint utilise pour chaque ligne de la liste. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	TSoftClassPtr<UExperienceEntryWidget> EntryClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	FText Title = FText::FromString(TEXT("Choisissez une expérience"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	FText IdleStatus = FText::FromString(TEXT("Visez une expérience et appuyez sur la gâchette."));

	/** {0} = nom de l'experience. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	FText GuidingStatus = FText::FromString(TEXT("Suivez le trait au sol jusqu'à « {0} »."));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	FText ArrivedStatus = FText::FromString(TEXT("Vous êtes arrivé : lancez l'expérience depuis la fiche."));

	/** Reconstruit la liste (si des zones ont ete ajoutees en jeu). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void RefreshList();

	/** Guide le joueur vers une experience (comme un clic sur sa ligne). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void SelectExperience(ADetectionZone* Zone);

	/** Arrete le guidage. */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void StopGuiding();

protected:
	UExperienceSelectorWidget(const FObjectInitializer& ObjectInitializer);

	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION() void HandleStopClicked();
	UFUNCTION() void HandleGuideArrived(AActor* Target);
	UFUNCTION() void HandleGuideStopped(AActor* Target);

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Le conteneur ou les lignes sont ajoutees (Vertical Box, Scroll Box, Wrap Box...). */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EntryList;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UButton> StopButton;

private:
	void HandleEntrySelected(ADetectionZone* Zone);
	void UpdateSelection();
	void SetStatus(const FText& Text);
	ANavigatorSystem* GetNavigator();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UExperienceEntryWidget>> Entries;

	TWeakObjectPtr<ANavigatorSystem> Navigator;
	TWeakObjectPtr<ADetectionZone> Selected;
};
