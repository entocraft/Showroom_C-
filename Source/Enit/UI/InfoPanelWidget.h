#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "Engine/Texture2D.h"
#include "InfoPanelWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class UButton;
class UPanelWidget;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInfoPanelActionSignature);

/** Un auteur (chercheur) d'une experience, affiche sur sa fiche. */
USTRUCT(BlueprintType)
struct ENIT_API FShowroomAuthor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auteur")
	FText Name = FText::FromString(TEXT("Prénom Nom"));

	/** Son domaine / metier, affiche en bleu (ex : "Aerodynamique"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auteur")
	FText Role;

	/** Une ou deux phrases sur son role dans l'experience. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auteur", meta = (MultiLine = true))
	FText Bio;

	/** Photo carree (optionnelle : sinon ses initiales). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auteur")
	TObjectPtr<UTexture2D> Photo = nullptr;
};

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

	/** Photo principale : vignette de l'ecran d'accueil et image de la fiche (optionnelle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche")
	TObjectPtr<UTexture2D> Photo = nullptr;

	// --- Fiche detaillee (ecran d'accueil facon Disney+) : tout est optionnel, vide = cache ---

	/** Petite pastille au-dessus du titre (ex : "EXPERIENCE N°2"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	FText Tag;

	/** Domaine (ex : "Aeronautique"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	FText Domain;

	/** Duree (ex : "~ 15 min"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	FText Duration;

	/** Ou se trouve l'experience (ex : "Zone 3", "Galerie"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	FText Location;

	/** Images supplementaires de la fiche (defilent apres Photo avec les fleches). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	TArray<TObjectPtr<UTexture2D>> Gallery;

	/** Les chercheurs a l'origine de l'experience (4 au maximum sont affiches). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee", meta = (TitleProperty = "Name"))
	TArray<FShowroomAuthor> Authors;

	/** Mise en avant dans le grand carrousel de l'ecran d'accueil. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fiche detaillee")
	bool bFeatured = true;

	/** Toutes les images de la fiche : Photo puis Gallery (sans les vides). */
	TArray<UTexture2D*> GetImages() const
	{
		TArray<UTexture2D*> Images;
		if (Photo) Images.Add(Photo);
		for (UTexture2D* Image : Gallery) { if (Image) Images.AddUnique(Image); }
		return Images;
	}
};

/**
 * Remplace UI_InfoPanel ("Carte Perso") : la fiche info des zones (style de la maquette Figma de Hugo,
 * en paysage : photo a gauche, textes et boutons a droite).
 *
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_InfoPanel.
 * Widgets remplis par le code (garder leur nom) : TitleText, SubtitleText, DescriptionText, PhotoImage,
 * ActionButton / ActionButtonText, StepNav, PreviousButton, NextButton, StepText, TagBox / TagText, MetaText.
 */
UCLASS()
class ENIT_API UInfoPanelWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	UInfoPanelWidget(const FObjectInitializer& ObjectInitializer);

	/** Met a jour le texte et la photo du panneau. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetInfo(const FShowroomInfo& NewInfo);

	UFUNCTION(BlueprintPure, Category = "Fiche")
	const FShowroomInfo& GetInfo() const { return Info; }

	/** Progression des etapes (barre de segments) : Current = 0..Count-1. Count = 0 : barre cachee. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetStepProgress(int32 Current, int32 Count);

	/** L'action est "Terminer" (icone carre) plutot que "Lancer" (icone lecture, qui respire pour attirer l'oeil). */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetActionIsStop(bool bStop);

	virtual void PlayIntro() override;

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
	FVector2D PhotoSize = FVector2D(360.f, 548.f);

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

	/** Barre de progression des etapes (segments ajoutes par le code). */
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> StepProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActionButtonIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ContentColumn;

	/** Pastille au-dessus du titre (Info > Tag) ; TagBox est cache si le tag est vide. */
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TagBox;

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TagText;

	/** "Domaine  •  Duree  •  Lieu". */
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MetaText;

	UFUNCTION() void HandleActionClicked() { OnActionClicked.Broadcast(); }
	UFUNCTION() void HandlePreviousClicked() { OnPreviousClicked.Broadcast(); }
	UFUNCTION() void HandleNextClicked() { OnNextClicked.Broadcast(); }


	bool bActionVisible = false;
	FText ActionLabel;

	bool bNavVisible = false;
	bool bActionIsStop = false;
	int32 ProgressCurrent = -1;
	int32 ProgressCount = 0;
	bool bCanPrevious = false;
	bool bCanNext = false;
	FText StepLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fiche")
	FShowroomInfo Info;
};
