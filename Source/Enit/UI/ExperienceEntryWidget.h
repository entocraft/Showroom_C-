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
 * Une VIGNETTE de l'ecran d'accueil (maquette Figma) : la photo de l'experience et, dessous,
 * une barre avec son titre et une fleche. Toute la vignette est cliquable (ouvre la fiche).
 *
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

	/** Met la vignette en surbrillance (experience vers laquelle le GPS guide). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void SetSelected(bool bSelected);

	FOnExperienceEntrySelected OnSelected;

	/** Taille de la vignette (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FVector2D TileSize = FVector2D(268.f, 236.f);

	/** Hauteur de la photo dans la vignette (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	float ImageHeight = 136.f;

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandleClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Sous-titre (cache dans la vignette par defaut ; utile si on agrandit la vignette). */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UImage> Thumbnail;

private:
	void Refresh();

	TWeakObjectPtr<ADetectionZone> Zone;
	bool bIsSelected = false;
};
