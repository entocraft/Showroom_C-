#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceLabelWidget.generated.h"

class UTextBlock;

/**
 * Etiquette flottante au-dessus d'un objet mis en valeur (nom + petit texte).
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_ExperienceLabel
 * (garder les noms LabelText et DetailsText).
 */
UCLASS()
class ENIT_API UExperienceLabelWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	void SetTexts(const FText& InLabel, const FText& InDetails);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor AccentColor = FLinearColor(1.f, 0.75f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 LabelFontSize = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 DetailsFontSize = 26;

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Etiquette", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Etiquette", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailsText;

private:
	void Refresh();

	FText Label;
	FText Details;
};
