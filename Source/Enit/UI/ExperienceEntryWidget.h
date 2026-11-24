#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UWidget;
class ADetectionZone;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnExperienceEntrySelected, ADetectionZone*);

/**
 * Une VIGNETTE de l'ecran d'accueil : la photo de l'experience (ou, sans photo, sa couleur et une grande
 * icone), son numero, son titre et son sous-titre. Toute la vignette est cliquable (ouvre la fiche) ;
 * elle se souleve quand le laser la survole. Un badge "GPS" apparait quand le GPS guide vers elle.
 *
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_ExperienceEntry
 * (garder les noms EntryButton, TitleText, SubtitleText, Thumbnail, TileIcon, NumberText, GpsBadge).
 */
UCLASS()
class ENIT_API UExperienceEntryWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	void SetZone(ADetectionZone* InZone);

	/** Couleur de la vignette sans photo et numero affiche (01, 02...). */
	void SetAccent(const FLinearColor& Color, int32 Number);

	UFUNCTION(BlueprintPure, Category = "Selecteur")
	ADetectionZone* GetZone() const { return Zone.Get(); }

	/** Met la vignette en surbrillance (experience vers laquelle le GPS guide). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void SetSelected(bool bSelected);

	FOnExperienceEntrySelected OnSelected;

	/** Taille de la vignette (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FVector2D TileSize = FVector2D(290.f, 320.f);

	/** Hauteur de la photo dans la vignette (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	float ImageHeight = 150.f;

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

	/** Grande icone posee sur la couleur quand il n'y a pas de photo. */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UImage> TileIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NumberText;

	/** Badge "GPS" (visible quand le GPS guide vers cette experience). */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GpsBadge;

private:
	void Refresh();

	TWeakObjectPtr<ADetectionZone> Zone;
	FLinearColor Accent = FLinearColor(0.18f, 0.37f, 0.64f);
	int32 Number = 0;
	bool bIsSelected = false;
};
