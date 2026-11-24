#include "UI/ExperienceSelectorWidget.h"
#include "UI/ExperienceEntryWidget.h"
#include "UI/ExperienceSheetWidget.h"
#include "UI/ShowroomStyle.h"
#include "Interaction/DetectionZone.h"
#include "Navigation/NavigatorSystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"

namespace
{
	constexpr float SidebarWidth = 250.f;
	constexpr float HeroHeight = 400.f;
	constexpr float PanelGap = 16.f;

	/** Onglet du menu lateral : un bouton plat avec son texte (nomme <Name>Text). */
	UButton* MakeTab(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Label)
	{
		using namespace ShowroomStyle;
		UButton* Tab = MakeButton(Tree, Name, FLinearColor::Transparent, SurfaceHover(), RadiusSmall, FMargin(12.f, 14.f));
		UTextBlock* Text = MakeText(Tree, *(FString(Name) + TEXT("Text")), FText::FromString(Label), 19, TextMain(), TEXT("Bold"), false);
		Tab->SetContent(Text);
		return Tab;
	}

	/** Bouton rond avec une fleche. */
	UButton* MakeArrow(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Symbol, int32 Size, const FLinearColor& Normal)
	{
		using namespace ShowroomStyle;
		UButton* Button = MakeButton(Tree, Name, Normal, AccentDeep(), 999.f, FMargin(Size * 0.38f, 0.f, Size * 0.38f, Size * 0.12f));
		Button->SetContent(MakeText(Tree, *(FString(Name) + TEXT("Text")), FText::FromString(Symbol), Size, TextMain(), TEXT("Bold"), false));
		return Button;
	}

	/** Page de texte simple : titre + encadre. */
	UVerticalBox* MakeTextPage(UWidgetTree* Tree, const TCHAR* PageName, const TCHAR* TitleName, const TCHAR* BodyName,
		const FText& Title, const FText& Body, UTextBlock*& OutTitle, UTextBlock*& OutBody)
	{
		using namespace ShowroomStyle;
		UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), PageName);
		OutTitle = MakeText(Tree, TitleName, Title, 44, TextMain(), TEXT("Bold"));
		if (UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(OutTitle))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
		}
		UBorder* Box = MakeBox(Tree, *(FString(PageName) + TEXT("Box")), SurfaceDeep(), RadiusCard, FMargin(28.f, 24.f));
		Box->SetVerticalAlignment(VAlign_Top);
		OutBody = MakeText(Tree, BodyName, Body, 26, TextMain());
		Box->SetContent(OutBody);
		if (UVerticalBoxSlot* BoxSlot = Page->AddChildToVerticalBox(Box))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return Page;
	}
}

UExperienceSelectorWidget::UExperienceSelectorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EntryClass = TSoftClassPtr<UExperienceEntryWidget>(FSoftObjectPath(TEXT("/Game/Showroom/UI/WBP_ExperienceEntry.WBP_ExperienceEntry_C")));
	SheetClass = TSoftClassPtr<UExperienceSheetWidget>(FSoftObjectPath(TEXT("/Game/Showroom/UI/WBP_ExperienceSheet.WBP_ExperienceSheet_C")));
	LogoTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Showroom/UI/Textures/T_UI_LogoENIT.T_UI_LogoENIT")));
}

// ============================================================================
// Mise en page par defaut (sert aussi a fabriquer WBP_ExperienceSelector)
// ============================================================================

void UExperienceSelectorWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  HorizontalBox "Root"
	//   ├ SizeBox > Border "Sidebar" > VerticalBox : logo, onglets, (vide), Parametres
	//   └ Border "ContentPanel" > VerticalBox
	//       ├ WidgetSwitcher "PageSwitcher"
	//       │   0 HomePage    : carrousel + vignettes
	//       │   1 SheetSlot   : la fiche (WBP_ExperienceSheet, ajoutee par le code)
	//       │   2 ProjectPage, 3 HelpPage, 4 SettingsPage
	//       └ HorizontalBox "StatusBar" : StatusText | StopButton
	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// ---------------- Menu lateral ----------------
	USizeBox* SidebarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarBox"));
	SidebarBox->SetWidthOverride(SidebarWidth);
	if (UHorizontalBoxSlot* SidebarSlot = Root->AddChildToHorizontalBox(SidebarBox))
	{
		SidebarSlot->SetPadding(FMargin(0.f, 0.f, PanelGap, 0.f));
	}
	UBorder* Sidebar = MakeBox(WidgetTree, TEXT("Sidebar"), ShowroomStyle::Sidebar(), RadiusPanel, FMargin(16.f, 20.f));
	SidebarBox->SetContent(Sidebar);
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Menu"));
	Sidebar->SetContent(Menu);

	// Logo : carte blanche (image T_UI_LogoENIT, sinon "ENIT" ecrit)
	UBorder* LogoCard = MakeBox(WidgetTree, TEXT("LogoCard"), FLinearColor::White, RadiusSmall, FMargin(12.f, 10.f));
	LogoCard->SetHorizontalAlignment(HAlign_Center);
	if (UVerticalBoxSlot* LogoSlot = Menu->AddChildToVerticalBox(LogoCard))
	{
		LogoSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	}
	UOverlay* LogoStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LogoStack"));
	LogoCard->SetContent(LogoStack);
	UVerticalBox* Fallback = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LogoFallback"));
	LogoFallback = Fallback;
	UTextBlock* LogoWord = ShowroomStyle::MakeText(WidgetTree, TEXT("LogoWord"), FText::FromString(TEXT("ENIT")), 48, Hex(TEXT("1B1F3B")), TEXT("Bold"), false);
	LogoWord->SetJustification(ETextJustify::Center);
	Fallback->AddChildToVerticalBox(LogoWord);
	UTextBlock* LogoLine = ShowroomStyle::MakeText(WidgetTree, TEXT("LogoLine"), FText::FromString(TEXT("École Nationale d'Ingénieurs de Tarbes")), 13, Hex(TEXT("1B1F3B")));
	LogoLine->SetJustification(ETextJustify::Center);
	Fallback->AddChildToVerticalBox(LogoLine);
	LogoStack->AddChildToOverlay(Fallback);
	LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoImage"));
	LogoImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* LogoImageSlot = LogoStack->AddChildToOverlay(LogoImage))
	{
		LogoImageSlot->SetHorizontalAlignment(HAlign_Center);
		LogoImageSlot->SetVerticalAlignment(VAlign_Center);
	}

	TabExperiences = MakeTab(WidgetTree, TEXT("TabExperiences"), TEXT("EXPÉRIENCES"));
	TabProject = MakeTab(WidgetTree, TEXT("TabProject"), TEXT("NOTRE PROJET"));
	TabHelp = MakeTab(WidgetTree, TEXT("TabHelp"), TEXT("BESOIN D'AIDE ?"));
	for (UButton* Tab : { TabExperiences.Get(), TabProject.Get(), TabHelp.Get() })
	{
		if (UVerticalBoxSlot* TabSlot = Menu->AddChildToVerticalBox(Tab))
		{
			TabSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}
	}
	USpacer* MenuSpace = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("MenuSpace"));
	if (UVerticalBoxSlot* SpaceSlot = Menu->AddChildToVerticalBox(MenuSpace))
	{
		SpaceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	TabSettings = MakeTab(WidgetTree, TEXT("TabSettings"), TEXT("PARAMÈTRES"));
	Menu->AddChildToVerticalBox(TabSettings);

	// ---------------- Panneau principal ----------------
	UBorder* ContentPanel = MakeBox(WidgetTree, TEXT("ContentPanel"), ShowroomStyle::Background(), RadiusPanel, FMargin(28.f, 26.f));
	if (UHorizontalBoxSlot* ContentSlot = Root->AddChildToHorizontalBox(ContentPanel))
	{
		ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	ContentPanel->SetContent(Content);

	PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("PageSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = Content->AddChildToVerticalBox(PageSwitcher))
	{
		SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ---- 0 : accueil ----
	UVerticalBox* Home = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HomePage"));
	PageSwitcher->AddChild(Home);

	USizeBox* HeroBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HeroBox"));
	HeroBox->SetHeightOverride(HeroHeight);
	Home->AddChildToVerticalBox(HeroBox);
	UOverlay* Hero = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Hero"));
	HeroBox->SetContent(Hero);

	HeroButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("HeroButton"), FLinearColor::Transparent, FLinearColor(1.f, 1.f, 1.f, 0.04f), RadiusCard, FMargin(0.f));
	HeroImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HeroImage"));
	HeroImage->SetBrush(RoundedImageBrush(nullptr, RadiusCard, FVector2D(846.f, HeroHeight)));
	HeroButton->SetContent(HeroImage);
	if (UOverlaySlot* HeroButtonSlot = Hero->AddChildToOverlay(HeroButton))
	{
		HeroButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		HeroButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* Caption = MakeBox(WidgetTree, TEXT("HeroCaption"), Shade(), RadiusCard, FMargin(24.f, 14.f, 28.f, 16.f));
	Caption->SetVisibility(ESlateVisibility::HitTestInvisible); // le clic passe au carrousel
	UVerticalBox* CaptionTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeroCaptionTexts"));
	Caption->SetContent(CaptionTexts);
	HeroTitle = ShowroomStyle::MakeText(WidgetTree, TEXT("HeroTitle"), FText::FromString(TEXT("EXPÉRIENCE")), 40, TextMain(), TEXT("Bold"), false);
	CaptionTexts->AddChildToVerticalBox(HeroTitle);
	HeroSubtitle = ShowroomStyle::MakeText(WidgetTree, TEXT("HeroSubtitle"), FText::FromString(TEXT("Sous-titre")), 22, TextMuted(), TEXT("Regular"), false);
	CaptionTexts->AddChildToVerticalBox(HeroSubtitle);
	if (UOverlaySlot* CaptionSlot = Hero->AddChildToOverlay(Caption))
	{
		CaptionSlot->SetHorizontalAlignment(HAlign_Left);
		CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
		CaptionSlot->SetPadding(FMargin(96.f, 0.f, 0.f, 22.f));
	}

	HeroPrev = MakeArrow(WidgetTree, TEXT("HeroPrev"), TEXT("‹"), 60, Shade());
	if (UOverlaySlot* PrevSlot = Hero->AddChildToOverlay(HeroPrev))
	{
		PrevSlot->SetHorizontalAlignment(HAlign_Left);
		PrevSlot->SetVerticalAlignment(VAlign_Center);
		PrevSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	HeroNext = MakeArrow(WidgetTree, TEXT("HeroNext"), TEXT("›"), 60, Shade());
	if (UOverlaySlot* NextSlot = Hero->AddChildToOverlay(HeroNext))
	{
		NextSlot->SetHorizontalAlignment(HAlign_Right);
		NextSlot->SetVerticalAlignment(VAlign_Center);
		NextSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	UHorizontalBox* Dots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeroDots"));
	Dots->SetVisibility(ESlateVisibility::HitTestInvisible);
	HeroDots = Dots;
	if (UOverlaySlot* DotsSlot = Hero->AddChildToOverlay(Dots))
	{
		DotsSlot->SetHorizontalAlignment(HAlign_Right);
		DotsSlot->SetVerticalAlignment(VAlign_Bottom);
		DotsSlot->SetPadding(FMargin(0.f, 0.f, 28.f, 26.f));
	}

	// Titre de la liste + fleches de page
	UHorizontalBox* RowHeader = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowHeader"));
	if (UVerticalBoxSlot* HeaderSlot = Home->AddChildToVerticalBox(RowHeader))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 14.f));
	}
	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), Title, 28, TextMain(), TEXT("Bold"), false);
	if (UHorizontalBoxSlot* TitleSlot = RowHeader->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	RowPageText = ShowroomStyle::MakeText(WidgetTree, TEXT("RowPageText"), FText::FromString(TEXT("1 / 1")), 20, TextDim(), TEXT("Bold"), false);
	if (UHorizontalBoxSlot* PageSlot = RowHeader->AddChildToHorizontalBox(RowPageText))
	{
		PageSlot->SetVerticalAlignment(VAlign_Center);
		PageSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}
	RowPrev = MakeArrow(WidgetTree, TEXT("RowPrev"), TEXT("‹"), 40, Surface());
	if (UHorizontalBoxSlot* RowPrevSlot = RowHeader->AddChildToHorizontalBox(RowPrev))
	{
		RowPrevSlot->SetVerticalAlignment(VAlign_Center);
		RowPrevSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	RowNext = MakeArrow(WidgetTree, TEXT("RowNext"), TEXT("›"), 40, Surface());
	if (UHorizontalBoxSlot* RowNextSlot = RowHeader->AddChildToHorizontalBox(RowNext))
	{
		RowNextSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* List = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EntryList"));
	EntryList = List;
	Home->AddChildToVerticalBox(List);

	// ---- 1 : fiche ----
	UBorder* SheetHolder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SheetSlot"));
	SheetHolder->SetBrushColor(FLinearColor::Transparent);
	SheetHolder->SetPadding(FMargin(0.f));
	SheetHolder->SetHorizontalAlignment(HAlign_Fill);
	SheetHolder->SetVerticalAlignment(VAlign_Fill);
	SheetSlot = SheetHolder;
	PageSwitcher->AddChild(SheetHolder);

	// ---- 2, 3, 4 : pages de texte ----
	UTextBlock* TitleOut = nullptr;
	UTextBlock* BodyOut = nullptr;
	PageSwitcher->AddChild(MakeTextPage(WidgetTree, TEXT("ProjectPage"), TEXT("ProjectTitle"), TEXT("ProjectBody"), ProjectTitleText, ProjectBodyText, TitleOut, BodyOut));
	ProjectTitle = TitleOut;
	ProjectBody = BodyOut;
	PageSwitcher->AddChild(MakeTextPage(WidgetTree, TEXT("HelpPage"), TEXT("HelpTitle"), TEXT("HelpBody"), HelpTitleText, HelpBodyText, TitleOut, BodyOut));
	HelpTitle = TitleOut;
	HelpBody = BodyOut;
	PageSwitcher->AddChild(MakeTextPage(WidgetTree, TEXT("SettingsPage"), TEXT("SettingsTitle"), TEXT("SettingsBody"), SettingsTitleText, SettingsBodyText, TitleOut, BodyOut));
	SettingsTitle = TitleOut;
	SettingsBody = BodyOut;

	// ---------------- Barre d'etat du GPS ----------------
	UHorizontalBox* StatusBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusBar"));
	if (UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusBar))
	{
		StatusSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	StatusText = ShowroomStyle::MakeText(WidgetTree, TEXT("StatusText"), IdleStatus, 22, AccentText(), TEXT("Regular"));
	if (UHorizontalBoxSlot* TextSlot = StatusBar->AddChildToHorizontalBox(StatusText))
	{
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
	StopButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("StopButton"), Surface(), SurfaceHover(), 999.f, FMargin(24.f, 10.f));
	StopButton->SetContent(ShowroomStyle::MakeText(WidgetTree, TEXT("StopButtonText"), FText::FromString(TEXT("Arrêter le guidage")), 22, TextMain(), TEXT("Bold"), false));
	if (UHorizontalBoxSlot* StopSlot = StatusBar->AddChildToHorizontalBox(StopButton))
	{
		StopSlot->SetVerticalAlignment(VAlign_Center);
	}
}

// ============================================================================
// Vie du widget
// ============================================================================

void UExperienceSelectorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StopButton)     StopButton->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleStopClicked);
	if (TabExperiences) TabExperiences->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleTabExperiences);
	if (TabProject)     TabProject->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleTabProject);
	if (TabHelp)        TabHelp->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleTabHelp);
	if (TabSettings)    TabSettings->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleTabSettings);
	if (HeroButton)     HeroButton->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleHeroClicked);
	if (HeroPrev)       HeroPrev->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleHeroPrev);
	if (HeroNext)       HeroNext->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleHeroNext);
	if (RowPrev)        RowPrev->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleRowPrev);
	if (RowNext)        RowNext->OnClicked.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleRowNext);

	// Textes des pages (modifiables dans les Class Defaults du WBP, sans toucher a la mise en page)
	if (TitleText)     TitleText->SetText(Title);
	if (ProjectTitle)  ProjectTitle->SetText(ProjectTitleText);
	if (ProjectBody)   ProjectBody->SetText(ProjectBodyText);
	if (HelpTitle)     HelpTitle->SetText(HelpTitleText);
	if (HelpBody)      HelpBody->SetText(HelpBodyText);
	if (SettingsTitle) SettingsTitle->SetText(SettingsTitleText);
	if (SettingsBody)  SettingsBody->SetText(SettingsBodyText);

	// Logo
	if (LogoImage)
	{
		if (UTexture2D* Logo = LogoTexture.LoadSynchronous())
		{
			LogoImage->SetBrushFromTexture(Logo, true);
			LogoImage->SetDesiredSizeOverride(FVector2D(190.f, 190.f * Logo->GetSurfaceHeight() / FMath::Max(1.f, Logo->GetSurfaceWidth())));
			LogoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (LogoFallback) LogoFallback->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	EnsureSheet();

	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Nav->OnGuideArrived.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleGuideArrived);
		Nav->OnGuideStopped.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleGuideStopped);
		Selected = Cast<ADetectionZone>(Nav->GetGuideTarget());
	}
	RefreshList();
	ShowPage(CurrentPage);
}

void UExperienceSelectorWidget::NativeDestruct()
{
	if (ANavigatorSystem* Nav = Navigator.Get())
	{
		Nav->OnGuideArrived.RemoveDynamic(this, &UExperienceSelectorWidget::HandleGuideArrived);
		Nav->OnGuideStopped.RemoveDynamic(this, &UExperienceSelectorWidget::HandleGuideStopped);
	}
	Super::NativeDestruct();
}

void UExperienceSelectorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Carrousel automatique (seulement sur la page d'accueil).
	if (CarouselInterval > 0.f && Featured.Num() > 1 && CurrentPage == EShowroomHubPage::Experiences)
	{
		CarouselTimer += InDeltaTime;
		if (CarouselTimer >= CarouselInterval)
		{
			ShowFeatured(HeroIndex + 1);
		}
	}
}

void UExperienceSelectorWidget::EnsureSheet()
{
	if (Sheet || !SheetSlot)
	{
		return;
	}
	TSubclassOf<UExperienceSheetWidget> Class = SheetClass.LoadSynchronous();
	if (!Class)
	{
		Class = UExperienceSheetWidget::StaticClass();
	}
	Sheet = CreateWidget<UExperienceSheetWidget>(this, Class);
	if (Sheet)
	{
		SheetSlot->ClearChildren();
		SheetSlot->AddChild(Sheet);
		Sheet->OnBack.AddUObject(this, &UExperienceSelectorWidget::HandleSheetBack);
		Sheet->OnGpsClicked.AddUObject(this, &UExperienceSelectorWidget::HandleSheetGps);
	}
}

ANavigatorSystem* UExperienceSelectorWidget::GetNavigator()
{
	if (!Navigator.IsValid() && GetWorld() && GetWorld()->IsGameWorld())
	{
		Navigator = ANavigatorSystem::FindOrSpawn(this);
	}
	return Navigator.Get();
}

// ============================================================================
// Liste des experiences
// ============================================================================

void UExperienceSelectorWidget::RefreshList()
{
	if (!GetWorld())
	{
		return;
	}

	// Toutes les zones d'experience a montrer, dans l'ordre choisi.
	TArray<ADetectionZone*> Zones;
	for (TActorIterator<ADetectionZone> It(GetWorld()); It; ++It)
	{
		if (It->bHasExperience && It->bShowInSelector)
		{
			Zones.Add(*It);
		}
	}
	Zones.Sort([](const ADetectionZone& A, const ADetectionZone& B)
	{
		return A.SelectorOrder != B.SelectorOrder ? A.SelectorOrder < B.SelectorOrder
			: A.Info.Title.ToString() < B.Info.Title.ToString();
	});

	// Carrousel : les experiences mises en avant (toutes si aucune ne l'est).
	Featured.Reset();
	for (ADetectionZone* Zone : Zones)
	{
		if (Zone->Info.bFeatured)
		{
			Featured.Add(Zone);
		}
	}
	if (Featured.Num() == 0)
	{
		for (ADetectionZone* Zone : Zones)
		{
			Featured.Add(Zone);
		}
	}
	HeroIndex = Featured.Num() > 0 ? HeroIndex % Featured.Num() : 0;

	// Vignettes
	Entries.Reset();
	RowItems.Reset();
	if (EntryList)
	{
		EntryList->ClearChildren();

		TSubclassOf<UExperienceEntryWidget> Class = EntryClass.LoadSynchronous();
		if (!Class)
		{
			Class = UExperienceEntryWidget::StaticClass();
		}
		for (ADetectionZone* Zone : Zones)
		{
			UExperienceEntryWidget* Entry = CreateWidget<UExperienceEntryWidget>(this, Class);
			if (!Entry)
			{
				continue;
			}
			Entry->SetZone(Zone);
			Entry->OnSelected.AddUObject(this, &UExperienceSelectorWidget::HandleEntrySelected);
			Entries.Add(Entry);
			RowItems.Add(Entry);
		}

		if (bShowComingSoonTile)
		{
			// Vignette "Prochainement" (non cliquable), meme taille qu'une vignette.
			using namespace ShowroomStyle;
			const FVector2D Size = GetDefault<UExperienceEntryWidget>()->TileSize;
			USizeBox* Tile = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Tile->SetWidthOverride(Size.X);
			Tile->SetHeightOverride(Size.Y);
			UBorder* Box = MakeBox(WidgetTree, NAME_None, Hex(TEXT("141A2C")), RadiusCard, FMargin(8.f));
			Box->SetBrush(RoundedOutlinedBrush(Hex(TEXT("141A2C")), RadiusCard, Outline(), 2.f));
			Box->SetHorizontalAlignment(HAlign_Center);
			Box->SetVerticalAlignment(VAlign_Center);
			Tile->SetContent(Box);
			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Box->SetContent(Column);
			UTextBlock* Plus = ShowroomStyle::MakeText(WidgetTree, NAME_None, FText::FromString(TEXT("+")), 64, TextDim(), TEXT("Light"), false);
			Plus->SetJustification(ETextJustify::Center);
			Column->AddChildToVerticalBox(Plus);
			UTextBlock* Soon = ShowroomStyle::MakeText(WidgetTree, NAME_None, ComingSoonLabel, 20, TextDim(), TEXT("Regular"), false);
			Soon->SetJustification(ETextJustify::Center);
			Column->AddChildToVerticalBox(Soon);
			RowItems.Add(Tile);
		}

		for (int32 i = 0; i < RowItems.Num(); ++i)
		{
			UPanelSlot* ItemSlot = EntryList->AddChild(RowItems[i]);
			if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(ItemSlot))
			{
				BoxSlot->SetPadding(FMargin(0.f, 0.f, i + 1 < RowItems.Num() ? 20.f : 0.f, 0.f));
				BoxSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (Zones.Num() == 0)
	{
		SetStatus(FText::FromString(TEXT("Aucune expérience dans ce niveau.")));
	}
	else if (const ADetectionZone* Zone = Selected.Get())
	{
		SetStatus(FText::Format(GuidingStatus, Zone->Info.Title));
	}
	else
	{
		SetStatus(IdleStatus);
	}

	UpdateHero();
	UpdateRow();
	UpdateSelection();
}

void UExperienceSelectorWidget::UpdateRow()
{
	const int32 PerPage = FMath::Max(1, TilesPerPage);
	const int32 PageCount = FMath::Max(1, FMath::DivideAndRoundUp(RowItems.Num(), PerPage));
	RowPage = FMath::Clamp(RowPage, 0, PageCount - 1);

	for (int32 i = 0; i < RowItems.Num(); ++i)
	{
		if (UWidget* Item = RowItems[i])
		{
			const bool bOnPage = i / PerPage == RowPage;
			Item->SetVisibility(bOnPage ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	const ESlateVisibility Arrows = PageCount > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (RowPrev) { RowPrev->SetVisibility(Arrows); RowPrev->SetIsEnabled(RowPage > 0); }
	if (RowNext) { RowNext->SetVisibility(Arrows); RowNext->SetIsEnabled(RowPage < PageCount - 1); }
	if (RowPageText)
	{
		RowPageText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), RowPage + 1, PageCount)));
		RowPageText->SetVisibility(PageCount > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UExperienceSelectorWidget::HandleRowPrev()
{
	--RowPage;
	UpdateRow();
}

void UExperienceSelectorWidget::HandleRowNext()
{
	++RowPage;
	UpdateRow();
}

// ============================================================================
// Carrousel
// ============================================================================

void UExperienceSelectorWidget::ShowFeatured(int32 Index)
{
	const int32 Count = Featured.Num();
	HeroIndex = Count > 0 ? (Index % Count + Count) % Count : 0;
	CarouselTimer = 0.f;
	UpdateHero();
}

void UExperienceSelectorWidget::UpdateHero()
{
	const ADetectionZone* Zone = Featured.IsValidIndex(HeroIndex) ? Featured[HeroIndex].Get() : nullptr;

	if (HeroImage)
	{
		const FVector2D Size = HeroImage->GetBrush().ImageSize;
		HeroImage->SetBrush(ShowroomStyle::RoundedImageBrush(Zone ? Zone->Info.Photo.Get() : nullptr, ShowroomStyle::RadiusCard, Size));
	}
	if (HeroTitle)    HeroTitle->SetText(Zone ? Zone->Info.Title.ToUpper() : FText::FromString(TEXT("Aucune expérience")));
	if (HeroSubtitle) HeroSubtitle->SetText(Zone ? Zone->Info.Subtitle : FText::GetEmpty());
	if (HeroButton)   HeroButton->SetIsEnabled(Zone != nullptr);

	const ESlateVisibility Arrows = Featured.Num() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (HeroPrev) HeroPrev->SetVisibility(Arrows);
	if (HeroNext) HeroNext->SetVisibility(Arrows);

	// Points : un par experience, le point actif est plus long et bleu.
	if (HeroDots)
	{
		HeroDots->ClearChildren();
		if (Featured.Num() > 1)
		{
			for (int32 i = 0; i < Featured.Num(); ++i)
			{
				const bool bActive = i == HeroIndex;
				USizeBox* DotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				DotBox->SetWidthOverride(bActive ? 30.f : 12.f);
				DotBox->SetHeightOverride(12.f);
				UBorder* Dot = ShowroomStyle::MakeBox(WidgetTree, NAME_None,
					bActive ? ShowroomStyle::AccentText() : FLinearColor(1.f, 1.f, 1.f, 0.4f), 6.f, FMargin(0.f));
				DotBox->SetContent(Dot);
				if (UHorizontalBoxSlot* DotSlot = Cast<UHorizontalBoxSlot>(HeroDots->AddChild(DotBox)))
				{
					DotSlot->SetPadding(FMargin(4.f, 0.f));
					DotSlot->SetVerticalAlignment(VAlign_Center);
				}
			}
		}
	}
}

void UExperienceSelectorWidget::HandleHeroPrev()
{
	ShowFeatured(HeroIndex - 1);
}

void UExperienceSelectorWidget::HandleHeroNext()
{
	ShowFeatured(HeroIndex + 1);
}

void UExperienceSelectorWidget::HandleHeroClicked()
{
	if (Featured.IsValidIndex(HeroIndex))
	{
		OpenSheet(Featured[HeroIndex].Get());
	}
}

// ============================================================================
// Pages et fiche
// ============================================================================

void UExperienceSelectorWidget::ShowPage(EShowroomHubPage Page)
{
	// Pas de fiche sans experience choisie.
	if (Page == EShowroomHubPage::Sheet && (!Sheet || !Sheet->GetZone()))
	{
		Page = EShowroomHubPage::Experiences;
	}
	CurrentPage = Page;
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(static_cast<int32>(Page));
	}
	CarouselTimer = 0.f;
	UpdateTabs();
}

void UExperienceSelectorWidget::UpdateTabs()
{
	// La fiche fait partie de l'onglet EXPERIENCES.
	const EShowroomHubPage Tab = CurrentPage == EShowroomHubPage::Sheet ? EShowroomHubPage::Experiences : CurrentPage;
	auto Style = [Tab](UButton* Button, EShowroomHubPage Page)
	{
		const bool bActive = Tab == Page;
		ShowroomStyle::SetButtonColors(Button,
			bActive ? ShowroomStyle::AccentDeep() : FLinearColor::Transparent,
			bActive ? ShowroomStyle::Accent() : ShowroomStyle::SurfaceHover(),
			ShowroomStyle::RadiusSmall);
	};
	Style(TabExperiences, EShowroomHubPage::Experiences);
	Style(TabProject, EShowroomHubPage::Project);
	Style(TabHelp, EShowroomHubPage::Help);
	Style(TabSettings, EShowroomHubPage::Settings);
}

void UExperienceSelectorWidget::HandleTabExperiences() { ShowPage(EShowroomHubPage::Experiences); }
void UExperienceSelectorWidget::HandleTabProject()     { ShowPage(EShowroomHubPage::Project); }
void UExperienceSelectorWidget::HandleTabHelp()        { ShowPage(EShowroomHubPage::Help); }
void UExperienceSelectorWidget::HandleTabSettings()    { ShowPage(EShowroomHubPage::Settings); }

void UExperienceSelectorWidget::OpenSheet(ADetectionZone* Zone)
{
	EnsureSheet();
	if (!Zone || !Sheet)
	{
		return;
	}
	Sheet->SetZone(Zone);
	ShowPage(EShowroomHubPage::Sheet);
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleEntrySelected(ADetectionZone* Zone)
{
	OpenSheet(Zone);
}

void UExperienceSelectorWidget::HandleSheetBack()
{
	ShowPage(EShowroomHubPage::Experiences);
}

void UExperienceSelectorWidget::HandleSheetGps(ADetectionZone* Zone)
{
	// Deuxieme clic sur le GPS de la meme experience = arreter le guidage.
	if (Zone && Zone == Selected.Get())
	{
		StopGuiding();
	}
	else
	{
		SelectExperience(Zone);
	}
}

// ============================================================================
// GPS
// ============================================================================

void UExperienceSelectorWidget::SelectExperience(ADetectionZone* Zone)
{
	if (!Zone)
	{
		return;
	}
	if (Zone->IsPlayerInside())
	{
		SetStatus(ArrivedStatus);
		UpdateSelection();
		return;
	}
	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Selected = Zone;
		Nav->GuideTo(Zone);
		SetStatus(FText::Format(GuidingStatus, Zone->Info.Title));
		UpdateSelection();
	}
}

void UExperienceSelectorWidget::StopGuiding()
{
	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Nav->StopGuideTo();
	}
	Selected = nullptr;
	SetStatus(IdleStatus);
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleStopClicked()
{
	StopGuiding();
}

void UExperienceSelectorWidget::HandleGuideArrived(AActor* /*Target*/)
{
	Selected = nullptr;
	SetStatus(ArrivedStatus);
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleGuideStopped(AActor* /*Target*/)
{
	// Une autre destination a pu etre choisie ailleurs : on se cale sur le GPS.
	if (const ANavigatorSystem* Nav = Navigator.Get())
	{
		Selected = Cast<ADetectionZone>(Nav->GetGuideTarget());
	}
	UpdateSelection();
}

void UExperienceSelectorWidget::UpdateSelection()
{
	for (UExperienceEntryWidget* Entry : Entries)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->GetZone() && Entry->GetZone() == Selected.Get());
		}
	}
	if (StopButton)
	{
		StopButton->SetVisibility(Selected.IsValid() ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (Sheet)
	{
		if (ADetectionZone* Zone = Sheet->GetZone())
		{
			Sheet->SetGpsState(Zone->IsPlayerInside() ? EExperienceGpsState::Arrived
				: Zone == Selected.Get() ? EExperienceGpsState::Guiding
				: EExperienceGpsState::Idle);
		}
	}
}

void UExperienceSelectorWidget::SetStatus(const FText& Text)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
	}
}
