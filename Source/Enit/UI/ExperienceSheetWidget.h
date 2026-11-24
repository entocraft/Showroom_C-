#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceSheetWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UPanelWidget;
class UWidget;
class ADetectionZone;
class UAuthorCardWidget;

/** Etat du bouton GPS de la fiche. */
UENUM(BlueprintType)
enum class EExperienceGpsState : uint8
{
	Idle      UMETA(DisplayName = "Pret"),
	Guiding   UMETA(DisplayName = "Guidage en cours"),
	Arrived   UMETA(DisplayName = "Deja dans la zone"),
};

DECLARE_MULTICAST_DELEGATE(FOnSheetBack);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSheetGps, ADetectionZone*);

/**
 * FICHE D'UNE EXPERIENCE sur l'ecran d'accueil (maquette Figma "Interface experience") :
 * galerie d'images, pastille, titre, sous-titre, domaine / duree / lieu, description,
 * les auteurs (chercheurs) et le bouton "Lancer le GPS".
 *
 * Tout le contenu vient de la fiche Info de la zone d'experience (Details > Zone > Info) :
 * aucun texte d'experience n'est ecrit dans le widget.
 *
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_ExperienceSheet. Garder les noms :
 * BackButton, GalleryImage, GalleryPrev, GalleryNext, GalleryCounter, TagBox, TagText, TitleText, SubtitleText,
 * DomainBox/DomainText, DurationBox/DurationText, LocationBox/LocationText, DescriptionText,
 * AuthorsTitle, AuthorList, GpsButton, GpsTitle, GpsSubtitle, GpsIconImage, GalleryArt, LeftColumn, RightColumn.
 */
UCLASS()
class ENIT_API UExperienceSheetWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	UExperienceSheetWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetZone(ADetectionZone* InZone);

	UFUNCTION(BlueprintPure, Category = "Fiche")
	ADetectionZone* GetZone() const { return Zone.Get(); }

	/** Change les textes du bouton GPS. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void SetGpsState(EExperienceGpsState State);

	/** Image suivante / precedente de la galerie. */
	UFUNCTION(BlueprintCallable, Category = "Fiche")
	void ShowImage(int32 Index);

	FOnSheetBack OnBack;
	FOnSheetGps OnGpsClicked;

	/** Widget Blueprint d'une carte auteur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fiche")
	TSoftClassPtr<UAuthorCardWidget> AuthorCardClass;

	/** Nombre maximum d'auteurs affiches (la place est limitee : pas de defilement au laser). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fiche", meta = (ClampMin = "1"))
	int32 MaxAuthors = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsIdleTitle = FText::FromString(TEXT("LANCER LE GPS"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsIdleSubtitle = FText::FromString(TEXT("Aller à la zone de l'expérience"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsGuidingTitle = FText::FromString(TEXT("GPS EN COURS"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsGuidingSubtitle = FText::FromString(TEXT("Suivez le trait au sol • cliquer pour arrêter"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsArrivedTitle = FText::FromString(TEXT("VOUS Y ÊTES"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GpsArrivedSubtitle = FText::FromString(TEXT("Lancez l'expérience depuis sa fiche"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText NoAuthors = FText::FromString(TEXT("Auteurs à venir."));

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleGpsClicked();
	UFUNCTION() void HandlePrevImage();
	UFUNCTION() void HandleNextImage();

	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UButton> BackButton;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UImage> GalleryImage;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UButton> GalleryPrev;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UButton> GalleryNext;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GalleryCounter;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> TagBox;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TagText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> DomainBox;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DomainText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> DurationBox;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DurationText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> LocationBox;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LocationText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AuthorsTitle;
	/** Le conteneur ou les cartes auteurs sont ajoutees (Vertical Box, Wrap Box...). */
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> AuthorList;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UButton> GpsButton;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GpsTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GpsSubtitle;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UImage> GpsIconImage;
	/** Grande icone de la galerie quand l'experience n'a pas d'image. */
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UImage> GalleryArt;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> LeftColumn;
	UPROPERTY(BlueprintReadOnly, Category = "Fiche", meta = (BindWidgetOptional)) TObjectPtr<UWidget> RightColumn;

private:
	void Refresh();
	void RefreshGallery();
	void RefreshGps();

	TWeakObjectPtr<ADetectionZone> Zone;
	int32 ImageIndex = 0;
	EExperienceGpsState GpsState = EExperienceGpsState::Idle;
};
