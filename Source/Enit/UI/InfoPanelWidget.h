#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "InfoPanelWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class UButton;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInfoPanelActionSignature);

/** Contenu d'une fiche info (ce qu'on voit sur le panneau). */
USTRUCT(BlueprintType)
struct ENIT_API FShowroomInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	FText Title = FText::FromString(TEXT("J. Doe"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	FText Subtitle = FText::FromString(TEXT("Sous titre"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche", meta = (MultiLine = true))
	FText Description = FText::FromString(TEXT("Lorem ipsum"));

	/** Photo affichee a gauche (optionnelle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	TObjectPtr<UTexture2D> Photo = nullptr;
};

/**
 * Remplace UI_InfoPanel ("Carte Perso") : la fiche info des zones.
 *
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_InfoPanel.
 * Widgets remplis par le code (garder leur nom) : TitleText, SubtitleText, DescriptionText, PhotoImage,
 * ActionButton / ActionButtonText, StepNav, PreviousButton, NextButton, StepText.
 */
UCLASS()
class ENIT_API UInfoPanelWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	/** Met a jour le texte et la photo du panneau. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetInfo(const FShowroomInfo& NewInfo);

	UFUNCTION(BlueprintPure, Category = "Fiche")
	const FShowroomInfo& GetInfo() const { return Info; }

	/** Affiche / cache le bouton d'action en bas de la fiche (ex : "Lancer l'experience") et change son texte. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetActionButton(bool bVisible, const FText& Label);

	/** Appele quand on clique sur le bouton d'action. */
	UPROPERTY(BlueprintAssignable, Category = "Fiche")
	FInfoPanelActionSignature OnActionClicked;

	/** Ligne "< Precedent | Etape 2 / 4 | Suivant >" affichee pendant une experience. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetStepNav(bool bVisible, const FText& InStepLabel, bool bInCanPrevious, bool bInCanNext);

	UPROPERTY(BlueprintAssignable, Category = "Fiche")
	FInfoPanelActionSignature OnPreviousClicked;

	UPROPERTY(BlueprintAssignable, Category = "Fiche")
	FInfoPanelActionSignature OnNextClicked;

	// --- Apparence du panneau par defaut (modifiable dans un Blueprint enfant) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor SubtitleColor = FLinearColor(0.6f, 0.75f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 TitleFontSize = 48;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 SubtitleFontSize = 28;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 DescriptionFontSize = 22;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FVector2D PhotoSize = FVector2D(256.f, 256.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor ButtonColor = FLinearColor(0.1f, 0.45f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	int32 ButtonFontSize = 30;

	/** Couleur du bouton Precedent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	FLinearColor NavButtonColor = FLinearColor(0.25f, 0.25f, 0.3f, 1.f);

protected:
	virtual void NativeConstruct() override;

	/** Mise en page par defaut (sert aussi a fabriquer WBP_InfoPanel). */
	virtual void BuildDefaultLayout() override;
	void Refresh();

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PhotoImage;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActionButton;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StepNav;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StepText;

	UFUNCTION() void HandleActionClicked() { OnActionClicked.Broadcast(); }
	UFUNCTION() void HandlePreviousClicked() { OnPreviousClicked.Broadcast(); }
	UFUNCTION() void HandleNextClicked() { OnNextClicked.Broadcast(); }


	bool bActionVisible = false;
	FText ActionLabel;

	bool bNavVisible = false;
	bool bCanPrevious = false;
	bool bCanNext = false;
	FText StepLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fiche")
	FShowroomInfo Info;
};
