#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "ExperienceSelectorWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UPanelWidget;
class UWidget;
class UWidgetSwitcher;
class UTexture2D;
class UExperienceEntryWidget;
class UExperienceSheetWidget;
class ADetectionZone;
class ANavigatorSystem;

/** Les pages de l'ecran d'accueil (onglets du menu lateral + la fiche). */
UENUM(BlueprintType)
enum class EShowroomHubPage : uint8
{
	Experiences = 0 UMETA(DisplayName = "Experiences"),
	Sheet       = 1 UMETA(DisplayName = "Fiche d'une experience"),
	Project     = 2 UMETA(DisplayName = "Notre projet"),
	Help        = 3 UMETA(DisplayName = "Besoin d'aide"),
	Settings    = 4 UMETA(DisplayName = "Parametres"),
};

/**
 * ECRAN D'ACCUEIL facon Disney+ (maquette Figma de Hugo), pose devant le point d'apparition.
 *
 *  - menu lateral : logo ENIT, EXPERIENCES, NOTRE PROJET, BESOIN D'AIDE ?, PARAMETRES ;
 *  - EXPERIENCES : grand carrousel des experiences mises en avant (defile tout seul + fleches),
 *    puis toutes les experiences en vignettes (par pages de 3, fleches) ;
 *  - un clic sur une vignette ouvre sa FICHE (WBP_ExperienceSheet) : images, auteurs, bouton "Lancer le GPS" ;
 *  - le GPS guide le joueur jusqu'a la zone (trait au sol), comme avant.
 *
 * Les experiences = les zones d'experience du niveau (Show In Selector), leur contenu = leur fiche Info.
 *
 * A mettre comme Content d'un Ecran flottant (resolution conseillee 1200 x 900).
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_ExperienceSelector. Garder les noms :
 * TabExperiences, TabProject, TabHelp, TabSettings, LogoImage, LogoFallback, PageSwitcher, SheetSlot,
 * HeroButton, HeroImage, HeroTitle, HeroSubtitle, HeroPrev, HeroNext, HeroDots, TitleText, RowPrev, RowNext,
 * RowPageText, EntryList, StatusText, StopButton, ProjectTitle, ProjectBody, HelpTitle, HelpBody, SettingsTitle, SettingsBody.
 */
UCLASS()
class ENIT_API UExperienceSelectorWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	// ---------------- Classes des sous-interfaces ----------------

	/** Le Widget Blueprint utilise pour chaque vignette. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	TSoftClassPtr<UExperienceEntryWidget> EntryClass;

	/** Le Widget Blueprint de la fiche d'une experience. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	TSoftClassPtr<UExperienceSheetWidget> SheetClass;

	/** Logo affiche en haut du menu (vide ou introuvable = logo ecrit "ENIT"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	TSoftObjectPtr<UTexture2D> LogoTexture;

	// ---------------- Comportement ----------------

	/** Vignettes visibles a la fois (les fleches passent a la page suivante). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur", meta = (ClampMin = "1"))
	int32 TilesPerPage = 3;

	/** Temps entre deux experiences du carrousel (s). 0 = pas de defilement automatique. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur", meta = (ClampMin = "0"))
	float CarouselInterval = 7.f;

	/** Ajoute une vignette "Prochainement" a la fin de la liste. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selecteur")
	bool bShowComingSoonTile = true;

	// ---------------- Textes ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText Title = FText::FromString(TEXT("Toutes les expériences"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText IdleStatus = FText::FromString(TEXT("Visez une expérience et appuyez sur la gâchette."));

	/** {0} = nom de l'experience. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText GuidingStatus = FText::FromString(TEXT("Suivez le trait au sol jusqu'à « {0} »."));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText ArrivedStatus = FText::FromString(TEXT("Vous êtes arrivé : lancez l'expérience depuis la fiche."));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText ComingSoonLabel = FText::FromString(TEXT("Prochainement"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText ProjectTitleText = FText::FromString(TEXT("Notre projet"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes", meta = (MultiLine = true))
	FText ProjectBodyText = FText::FromString(TEXT(
		"Ce musée virtuel présente les expériences menées par les chercheurs de l'ENIT "
		"(École Nationale d'Ingénieurs de Tarbes) en mécanique et en robotique.\n\n"
		"Ici, on ne lit pas l'expérience : on la réalise. Choisissez une expérience, laissez le GPS "
		"vous guider jusqu'à elle, puis manipulez les objets vous-même.\n\n"
		"Projet tutoré (PTUT) du BUT MMI de l'IUT de Tarbes."));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText HelpTitleText = FText::FromString(TEXT("Besoin d'aide ?"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes", meta = (MultiLine = true))
	FText HelpBodyText = FText::FromString(TEXT(
		"•  Viser une interface + gâchette : cliquer\n"
		"•  Joystick gauche : marcher\n"
		"•  Joystick droit : tourner\n"
		"•  Gâchette droite (sans viser d'interface) : se téléporter\n"
		"•  Bouton de côté (grip) : attraper un objet\n"
		"•  Bouton Y ou B : ouvrir le menu de la main\n"
		"•  Viser la barre sous un écran + gâchette maintenue : déplacer l'écran"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes")
	FText SettingsTitleText = FText::FromString(TEXT("Paramètres"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textes", meta = (MultiLine = true))
	FText SettingsBodyText = FText::FromString(TEXT(
		"Bientôt disponible : volume, mode de déplacement, rotation par crans ou continue."));

	// ---------------- Actions ----------------

	/** Reconstruit la liste (si des zones ont ete ajoutees en jeu). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void RefreshList();

	/** Guide le joueur vers une experience (bouton "Lancer le GPS"). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void SelectExperience(ADetectionZone* Zone);

	/** Arrete le guidage. */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void StopGuiding();

	/** Ouvre la fiche d'une experience. */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void OpenSheet(ADetectionZone* Zone);

	/** Change de page (onglets du menu lateral). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void ShowPage(EShowroomHubPage Page);

	UFUNCTION(BlueprintPure, Category = "Selecteur")
	EShowroomHubPage GetPage() const { return CurrentPage; }

	/** Experience du carrousel (index dans les experiences mises en avant). */
	UFUNCTION(BlueprintCallable, Category = "Selecteur")
	void ShowFeatured(int32 Index);

protected:
	UExperienceSelectorWidget(const FObjectInitializer& ObjectInitializer);

	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void HandleStopClicked();
	UFUNCTION() void HandleGuideArrived(AActor* Target);
	UFUNCTION() void HandleGuideStopped(AActor* Target);
	UFUNCTION() void HandleTabExperiences();
	UFUNCTION() void HandleTabProject();
	UFUNCTION() void HandleTabHelp();
	UFUNCTION() void HandleTabSettings();
	UFUNCTION() void HandleHeroClicked();
	UFUNCTION() void HandleHeroPrev();
	UFUNCTION() void HandleHeroNext();
	UFUNCTION() void HandleRowPrev();
	UFUNCTION() void HandleRowNext();

	// --- Menu lateral ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> TabExperiences;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> TabProject;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> TabHelp;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> TabSettings;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UImage> LogoImage;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UWidget> LogoFallback;

	// --- Pages ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UWidgetSwitcher> PageSwitcher;
	/** La fiche (WBP_ExperienceSheet) est placee dans ce conteneur. */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> SheetSlot;

	// --- Carrousel ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> HeroButton;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UImage> HeroImage;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeroTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeroSubtitle;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> HeroPrev;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> HeroNext;
	/** Les petits points sous le carrousel (remplis par le code). */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> HeroDots;

	// --- Liste des experiences ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> RowPrev;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> RowNext;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RowPageText;
	/** Le conteneur ou les vignettes sont ajoutees (Horizontal Box, Wrap Box...). */
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> EntryList;

	// --- Barre d'etat du GPS ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UButton> StopButton;

	// --- Pages de texte ---
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ProjectTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ProjectBody;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HelpTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HelpBody;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SettingsTitle;
	UPROPERTY(BlueprintReadOnly, Category = "Selecteur", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SettingsBody;

private:
	void HandleEntrySelected(ADetectionZone* Zone);
	void HandleSheetGps(ADetectionZone* Zone);
	void HandleSheetBack();
	void UpdateSelection();
	void UpdateTabs();
	void UpdateRow();
	void UpdateHero();
	void SetStatus(const FText& Text);
	void EnsureSheet();
	ANavigatorSystem* GetNavigator();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UExperienceEntryWidget>> Entries;

	/** Vignettes + "Prochainement", dans l'ordre d'affichage. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> RowItems;

	UPROPERTY(Transient)
	TObjectPtr<UExperienceSheetWidget> Sheet;

	TArray<TWeakObjectPtr<ADetectionZone>> Featured;
	TWeakObjectPtr<ANavigatorSystem> Navigator;
	TWeakObjectPtr<ADetectionZone> Selected;

	EShowroomHubPage CurrentPage = EShowroomHubPage::Experiences;
	int32 HeroIndex = 0;
	int32 RowPage = 0;
	float CarouselTimer = 0.f;
};
