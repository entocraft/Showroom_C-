#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "SimpleScreenWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

/**
 * Modele d'ecran flottant (titre + texte + bouton Fermer).
 * Pour faire ton propre ecran : DUPLIQUER Content/Showroom/UI/WBP_EcranSimple, le modifier dans l'editeur UMG
 * (textes, images, videos, boutons...), puis le choisir dans "Content" d'un Ecran flottant.
 * Un bouton nomme CloseButton ferme l'ecran automatiquement.
 * TitleText, BodyText et MainImage peuvent etre remplis par l'Ecran flottant (Title / Body / Image)
 * sans toucher au Widget Blueprint : pratique pour les ecrans de detail des experiences.
 */
UCLASS()
class ENIT_API USimpleScreenWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	virtual void SetScreenContent(const FText& Title, const FText& Body, UTexture2D* Image) override;

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandleCloseClicked();

	UPROPERTY(BlueprintReadOnly, Category = "Ecran", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(BlueprintReadOnly, Category = "Ecran", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Ecran", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(BlueprintReadOnly, Category = "Ecran", meta = (BindWidgetOptional))
	TObjectPtr<UImage> MainImage;

private:
	void ApplyContent();

	// Contenu donne par l'ecran (applique aussi a la construction du widget).
	FText PendingTitle;
	FText PendingBody;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PendingImage;
	bool bHasContent = false;
};
