#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class ADetectionZone;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnExperienceEntrySelected, ADetectionZone*);

/**
 * Une ligne du selecteur d'experiences (photo + titre + sous-titre), cliquable.
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_ExperienceEntry
 * (garder les noms EntryButton, TitleText, SubtitleText, Thumbnail).
 */
UCLASS()
class ENIT_API UExperienceEntryWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	void SetZone(ADetectionZone* InZone);

	UFUNCTION(BlueprintPure, Category = "Selecteur")
	ADetectionZone* GetZone() const { return Zone.Get(); }

	/** Met la ligne en surbrillance (experience choisie). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void SetSelected(bool bSelected);

	FOnExperienceEntrySelected OnSelected;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor NormalColor = FLinearColor(0.12f, 0.12f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor SelectedColor = FLinearColor(0.1f, 0.45f, 0.9f, 1.f);

	/** Teinte appliquee au bouton quand l'experience est choisie. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor SelectedTint = FLinearColor(0.7f, 1.6f, 4.f, 1.f);

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandleClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UImage> Thumbnail;

private:
	void Refresh();

	TWeakObjectPtr<ADetectionZone> Zone;
	bool bIsSelected = false;
};
