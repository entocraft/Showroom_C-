#include "UI/ExperienceSelectorWidget.h"
#include "UI/ExperienceEntryWidget.h"
#include "UI/ExperienceSheetWidget.h"
#include "UI/ShowroomStyle.h"
#include "Interaction/DetectionZone.h"
#include "Navigation/NavigatorSystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
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
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"

namespace
{
	constexpr float SidebarWidth = 300.f;
	constexpr float HeroHeight = 320.f;
	constexpr float HeroWidth = 1550.f; // toute la largeur du panneau principal (ecran de 1920 px)
	constexpr float PanelGap = 16.f;

	/** Onglet du menu lateral : un bouton plat avec son texte (nomme <Name>Text). */
	UButton* MakeTab(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Label, const TCHAR* IconName)
	{
		using namespace ShowroomStyle;
		UButton* Tab = MakeButton(Tree, Name, FLinearColor::Transparent, SurfaceHover(), RadiusCard, FMargin(14.f, 16.f));
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(FString(Name) + TEXT("Row")));
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(MakeIcon(Tree, *(FString(Name) + TEXT("Icon")), IconName, 32.f, TextMain())))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
		}
		UTextBlock* Text = MakeText(Tree, *(FString(Name) + TEXT("Text")), FText::FromString(Label), 22, TextMain(), TEXT("Bold"), false);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		Tab->SetContent(Row);
		if (UButtonSlot* RowSlot = Cast<UButtonSlot>(Row->Slot))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Left); // icones alignees les unes sous les autres
		}
		return Tab;
	}

	/** Bouton rond avec une fleche. */
	UButton* MakeArrow(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* IconName, int32 Size, const FLinearColor& Normal)
	{
		using namespace ShowroomStyle;
		UButton* Button = MakeButton(Tree, Name, Normal, AccentDeep(), 999.f, FMargin(Size * 0.3f));
		Button->SetContent(MakeIcon(Tree, *(FString(Name) + TEXT("Icon")), IconName, Size * 0.7f, TextMain()));
		return Button;
	}

	/** Page de texte simple : titre + encadre. */
	/** Titre de page : icone dans une pastille bleue + texte. */
	UHorizontalBox* MakePageTitle(UWidgetTree* Tree, const TCHAR* TitleName, const TCHAR* IconName, const FText& Title, UTextBlock*& OutTitle)
	{
		using namespace ShowroomStyle;
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(FString(TitleName) + TEXT("Row")));
		UBorder* Badge = MakeBox(Tree, *(FString(TitleName) + TEXT("Badge")), AccentDeep(), 999.f, FMargin(16.f));
		Badge->SetContent(MakeIcon(Tree, NAME_None, IconName, 40.f, AccentText()));
		if (UHorizontalBoxSlot* BadgeSlot = Row->AddChildToHorizontalBox(Badge))
		{
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
			BadgeSlot->SetPadding(FMargin(0.f, 0.f, 22.f, 0.f));
		}
		OutTitle = MakeText(Tree, TitleName, Title, 52, TextMain(), TEXT("Bold"), false);
		if (UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(OutTitle))
		{
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Row;
	}

	UVerticalBox* MakeTextPage(UWidgetTree* Tree, const TCHAR* PageName, const TCHAR* TitleName, const TCHAR* BodyName,
		const FText& Title, const FText& Body, UTextBlock*& OutTitle, UTextBlock*& OutBody, const TCHAR* IconName)
	{
		using namespace ShowroomStyle;
		UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), PageName);
		if (UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(MakePageTitle(Tree, TitleName, IconName, Title, OutTitle)))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		}
		UBorder* Box = MakeBox(Tree, *(FString(PageName) + TEXT("Box")), SurfaceDeep(), RadiusCard, FMargin(28.f, 24.f));
		Box->SetVerticalAlignment(VAlign_Top);
		OutBody = MakeText(Tree, BodyName, Body, 31, TextMain());
		Box->SetContent(OutBody);
		if (UVerticalBoxSlot* BoxSlot = Page->AddChildToVerticalBox(Box))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return Page;
	}

	/** Une carte de la page d'aide : icone dans un rond + titre + explication. */
	UBorder* MakeHelpCard(UWidgetTree* Tree, const TCHAR* IconName, const TCHAR* Title, const TCHAR* Text)
	{
		using namespace ShowroomStyle;
		UBorder* Card = MakeBox(Tree, NAME_None, Surface(), RadiusCard, FMargin(22.f, 18.f));
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Card->SetContent(Row);
		UBorder* Badge = MakeBox(Tree, NAME_None, AccentDeep(), 999.f, FMargin(16.f));
		Badge->SetContent(MakeIcon(Tree, NAME_None, IconName, 40.f, AccentText()));
		if (UHorizontalBoxSlot* BadgeSlot = Row->AddChildToHorizontalBox(Badge))
		{
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
			BadgeSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
		}
		UVerticalBox* Texts = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts))
		{
			TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TextsSlot->SetVerticalAlignment(VAlign_Center);
		}
		Texts->AddChildToVerticalBox(MakeText(Tree, NAME_None, FText::FromString(Title), 27, TextMain(), TEXT("Bold"), false));
		Texts->AddChildToVerticalBox(MakeText(Tree, NAME_None, FText::FromString(Text), 22, TextMuted(), TEXT("Regular"), true));
		return Card;
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

	TabExperiences = MakeTab(WidgetTree, TEXT("TabExperiences"), TEXT("EXPÉRIENCES"), TEXT("grid"));
	TabProject = MakeTab(WidgetTree, TEXT("TabProject"), TEXT("NOTRE PROJET"), TEXT("info"));
	TabHelp = MakeTab(WidgetTree, TEXT("TabHelp"), TEXT("AIDE"), TEXT("help"));
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
	// Astuce (rassurante pour une premiere fois en VR)
	UBorder* Tip = MakeBox(WidgetTree, TEXT("TipCard"), SurfaceDeep(), RadiusCard, FMargin(16.f, 14.f));
	UHorizontalBox* TipRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TipRow"));
	Tip->SetContent(TipRow);
	if (UHorizontalBoxSlot* TipIconSlot = TipRow->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("TipIcon"), TEXT("pointer"), 34.f, AccentText())))
	{
		TipIconSlot->SetVerticalAlignment(VAlign_Center);
		TipIconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}
	TipLabel = ShowroomStyle::MakeText(WidgetTree, TEXT("TipLabel"), TipText, 19, TextMuted(), TEXT("Regular"), true);
	if (UHorizontalBoxSlot* TipTextSlot = TipRow->AddChildToHorizontalBox(TipLabel))
	{
		TipTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TipTextSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* TipSlot = Menu->AddChildToVerticalBox(Tip))
	{
		TipSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	TabSettings = MakeTab(WidgetTree, TEXT("TabSettings"), TEXT("PARAMÈTRES"), TEXT("gear"));
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

	UHorizontalBox* Welcome = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WelcomeRow"));
	if (UVerticalBoxSlot* WelcomeSlot = Home->AddChildToVerticalBox(Welcome))
	{
		WelcomeSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 20.f));
	}
	if (UHorizontalBoxSlot* SparkSlot = Welcome->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("WelcomeIcon"), TEXT("sparkle"), 56.f, AccentText())))
	{
		SparkSlot->SetVerticalAlignment(VAlign_Center);
		SparkSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
	}
	UVerticalBox* WelcomeTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WelcomeTexts"));
	Welcome->AddChildToHorizontalBox(WelcomeTexts);
	WelcomeTitle = ShowroomStyle::MakeText(WidgetTree, TEXT("WelcomeTitle"), WelcomeTitleText, 40, TextMain(), TEXT("Bold"), false);
	WelcomeTexts->AddChildToVerticalBox(WelcomeTitle);
	WelcomeSubtitle = ShowroomStyle::MakeText(WidgetTree, TEXT("WelcomeSubtitle"), WelcomeSubtitleText, 25, TextMuted(), TEXT("Regular"), false);
	WelcomeTexts->AddChildToVerticalBox(WelcomeSubtitle);

	USizeBox* HeroBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HeroBox"));
	HeroBox->SetHeightOverride(HeroHeight);
	Home->AddChildToVerticalBox(HeroBox);
	UOverlay* Hero = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Hero"));
	HeroBox->SetContent(Hero);

	HeroButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("HeroButton"), FLinearColor::Transparent, FLinearColor(1.f, 1.f, 1.f, 0.04f), RadiusCard, FMargin(0.f));
	HeroImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HeroImage"));
	HeroImage->SetBrush(RoundedImageBrush(nullptr, RadiusCard, FVector2D(HeroWidth, HeroHeight)));
	HeroButton->SetContent(HeroImage);
	if (UOverlaySlot* HeroButtonSlot = Hero->AddChildToOverlay(HeroButton))
	{
		HeroButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		HeroButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Grande icone en filigrane (experience sans photo)
	HeroArt = MakeIcon(WidgetTree, TEXT("HeroArt"), TEXT("cube"), 230.f, FLinearColor(1.f, 1.f, 1.f, 0.16f));
	if (UOverlaySlot* ArtSlot = Hero->AddChildToOverlay(HeroArt))
	{
		ArtSlot->SetHorizontalAlignment(HAlign_Right);
		ArtSlot->SetVerticalAlignment(VAlign_Center);
		ArtSlot->SetPadding(FMargin(0.f, 0.f, 150.f, 0.f));
	}
	// Degrade sombre en bas : le titre reste lisible meme sur une photo claire
	UImage* HeroShade = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HeroShade"));
	{
		FSlateBrush ShadeBrush = RoundedBrush(FLinearColor::White, RadiusCard);
		ShadeBrush.SetResourceObject(LoadObject<UTexture2D>(nullptr, TEXT("/Game/Showroom/UI/Icons/T_UI_ShadeBottom.T_UI_ShadeBottom"), nullptr, LOAD_NoWarn | LOAD_Quiet));
		HeroShade->SetBrush(ShadeBrush);
		HeroShade->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (UOverlaySlot* ShadeSlot = Hero->AddChildToOverlay(HeroShade))
	{
		ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
		ShadeSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* Caption = MakeBox(WidgetTree, TEXT("HeroCaption"), FLinearColor::Transparent, RadiusCard, FMargin(8.f, 0.f));
	HeroCaption = Caption;
	Caption->SetVisibility(ESlateVisibility::HitTestInvisible); // le clic passe au carrousel
	UVerticalBox* CaptionTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeroCaptionTexts"));
	Caption->SetContent(CaptionTexts);
	HeroTitle = ShowroomStyle::MakeText(WidgetTree, TEXT("HeroTitle"), FText::FromString(TEXT("EXPÉRIENCE")), 54, TextMain(), TEXT("Bold"), false);
	CaptionTexts->AddChildToVerticalBox(HeroTitle);
	HeroSubtitle = ShowroomStyle::MakeText(WidgetTree, TEXT("HeroSubtitle"), FText::FromString(TEXT("Sous-titre")), 28, TextMuted(), TEXT("Regular"), false);
	CaptionTexts->AddChildToVerticalBox(HeroSubtitle);
	// "Decouvrir l'experience" : invite a cliquer (le carrousel entier est cliquable)
	UBorder* Cta = MakeBox(WidgetTree, TEXT("HeroCta"), Accent(), 999.f, FMargin(20.f, 10.f, 26.f, 10.f));
	HeroCta = Cta;
	UTextBlock* CtaText = nullptr;
	Cta->SetContent(MakeIconLabel(WidgetTree, TEXT("HeroCtaLabel"), TEXT("play"), DiscoverText, 24, TextMain(), TEXT("Bold"), CtaText, 10.f));
	HeroCtaText = CtaText;
	UHorizontalBox* CtaRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeroCtaRow"));
	CtaRow->AddChildToHorizontalBox(Cta);
	if (UVerticalBoxSlot* CtaSlot = CaptionTexts->AddChildToVerticalBox(CtaRow))
	{
		CtaSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
	if (UOverlaySlot* CaptionSlot = Hero->AddChildToOverlay(Caption))
	{
		CaptionSlot->SetHorizontalAlignment(HAlign_Left);
		CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
		CaptionSlot->SetPadding(FMargin(100.f, 0.f, 0.f, 26.f));
	}

	HeroPrev = MakeArrow(WidgetTree, TEXT("HeroPrev"), TEXT("chevron_left"), 60, Shade());
	if (UOverlaySlot* PrevSlot = Hero->AddChildToOverlay(HeroPrev))
	{
		PrevSlot->SetHorizontalAlignment(HAlign_Left);
		PrevSlot->SetVerticalAlignment(VAlign_Center);
		PrevSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	HeroNext = MakeArrow(WidgetTree, TEXT("HeroNext"), TEXT("chevron_right"), 60, Shade());
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
		HeaderSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 16.f));
	}
	if (UHorizontalBoxSlot* ListIconSlot = RowHeader->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("ListIcon"), TEXT("grid"), 34.f, AccentText())))
	{
		ListIconSlot->SetVerticalAlignment(VAlign_Center);
		ListIconSlot->SetPadding(FMargin(4.f, 0.f, 14.f, 0.f));
	}
	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), Title, 34, TextMain(), TEXT("Bold"), false);
	if (UHorizontalBoxSlot* TitleSlot = RowHeader->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	RowPageText = ShowroomStyle::MakeText(WidgetTree, TEXT("RowPageText"), FText::FromString(TEXT("1 / 1")), 24, TextDim(), TEXT("Bold"), false);
	if (UHorizontalBoxSlot* PageSlot = RowHeader->AddChildToHorizontalBox(RowPageText))
	{
		PageSlot->SetVerticalAlignment(VAlign_Center);
		PageSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}
	RowPrev = MakeArrow(WidgetTree, TEXT("RowPrev"), TEXT("chevron_left"), 40, Surface());
	if (UHorizontalBoxSlot* RowPrevSlot = RowHeader->AddChildToHorizontalBox(RowPrev))
	{
		RowPrevSlot->SetVerticalAlignment(VAlign_Center);
		RowPrevSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	RowNext = MakeArrow(WidgetTree, TEXT("RowNext"), TEXT("chevron_right"), 40, Surface());
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
	PageSwitcher->AddChild(MakeTextPage(WidgetTree, TEXT("ProjectPage"), TEXT("ProjectTitle"), TEXT("ProjectBody"), ProjectTitleText, ProjectBodyText, TitleOut, BodyOut, TEXT("info")));
	ProjectTitle = TitleOut;
	ProjectBody = BodyOut;
	{
		// Aide : une carte illustree par geste
		UVerticalBox* HelpPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HelpPage"));
		if (UVerticalBoxSlot* HelpTitleSlot = HelpPage->AddChildToVerticalBox(MakePageTitle(WidgetTree, TEXT("HelpTitle"), TEXT("help"), HelpTitleText, TitleOut)))
		{
			HelpTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		}
		HelpTitle = TitleOut;
		HelpBody = ShowroomStyle::MakeText(WidgetTree, TEXT("HelpBody"), HelpBodyText, 26, TextMuted());
		if (UVerticalBoxSlot* HelpBodySlot = HelpPage->AddChildToVerticalBox(HelpBody))
		{
			HelpBodySlot->SetPadding(FMargin(4.f, 0.f, 0.f, 18.f));
		}
		UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("HelpCards"));
		Grid->SetSlotPadding(FMargin(8.f));
		struct FCard { const TCHAR* Icon; const TCHAR* Title; const TCHAR* Text; };
		const FCard Cards[] = {
			{ TEXT("pointer"), TEXT("Cliquer"), TEXT("Visez avec le laser, puis gâchette.") },
			{ TEXT("move"), TEXT("Marcher"), TEXT("Joystick gauche.") },
			{ TEXT("refresh"), TEXT("Tourner"), TEXT("Joystick droit.") },
			{ TEXT("pin"), TEXT("Se téléporter"), TEXT("Gâchette droite, sans viser d'écran.") },
			{ TEXT("hand"), TEXT("Attraper"), TEXT("Bouton de côté (grip) près d'un objet.") },
			{ TEXT("grid"), TEXT("Menu de la main"), TEXT("Bouton Y ou B.") },
			{ TEXT("steps"), TEXT("Déplacer un écran"), TEXT("Visez la barre dessous, gâchette maintenue.") },
			{ TEXT("play"), TEXT("Vivre une expérience"), TEXT("Suivez le GPS, puis « Lancer l'expérience ».") },
		};
		for (int32 i = 0; i < UE_ARRAY_COUNT(Cards); ++i)
		{
			if (UUniformGridSlot* CardSlot = Grid->AddChildToUniformGrid(MakeHelpCard(WidgetTree, Cards[i].Icon, Cards[i].Title, Cards[i].Text), i / 2, i % 2))
			{
				CardSlot->SetHorizontalAlignment(HAlign_Fill);
				CardSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		if (UVerticalBoxSlot* GridSlot = HelpPage->AddChildToVerticalBox(Grid))
		{
			GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		PageSwitcher->AddChild(HelpPage);
	}
	PageSwitcher->AddChild(MakeTextPage(WidgetTree, TEXT("SettingsPage"), TEXT("SettingsTitle"), TEXT("SettingsBody"), SettingsTitleText, SettingsBodyText, TitleOut, BodyOut, TEXT("gear")));
	SettingsTitle = TitleOut;
	SettingsBody = BodyOut;

	// ---------------- Barre d'etat du GPS ----------------
	UHorizontalBox* StatusBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusBar"));
	if (UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusBar))
	{
		StatusSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	UBorder* StatusPill = MakeBox(WidgetTree, TEXT("StatusPill"), SurfaceDeep(), 999.f, FMargin(18.f, 10.f, 26.f, 10.f));
	if (UHorizontalBoxSlot* PillSlot = StatusBar->AddChildToHorizontalBox(StatusPill))
	{
		PillSlot->SetVerticalAlignment(VAlign_Center);
	}
	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusRow"));
	StatusPill->SetContent(StatusRow);
	StatusIcon = MakeIcon(WidgetTree, TEXT("StatusIcon"), TEXT("pointer"), 32.f, AccentText());
	if (UHorizontalBoxSlot* StatusIconSlot = StatusRow->AddChildToHorizontalBox(StatusIcon))
	{
		StatusIconSlot->SetVerticalAlignment(VAlign_Center);
		StatusIconSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}
	StatusText = ShowroomStyle::MakeText(WidgetTree, TEXT("StatusText"), IdleStatus, 26, TextMain(), TEXT("Regular"), false);
	if (UHorizontalBoxSlot* TextSlot = StatusRow->AddChildToHorizontalBox(StatusText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* StatusSpace = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("StatusSpace"));
	if (UHorizontalBoxSlot* SpaceSlot2 = StatusBar->AddChildToHorizontalBox(StatusSpace))
	{
		SpaceSlot2->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	StopButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("StopButton"), Surface(), SurfaceHover(), 999.f, FMargin(22.f, 10.f, 26.f, 10.f));
	UTextBlock* StopText = nullptr;
	StopButton->SetContent(MakeIconLabel(WidgetTree, TEXT("StopButtonLabel"), TEXT("stop"), FText::FromString(TEXT("Arrêter le guidage")), 24, TextMain(), TEXT("Bold"), StopText, 10.f));
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
	if (WelcomeTitle)    WelcomeTitle->SetText(WelcomeTitleText);
	if (WelcomeSubtitle) WelcomeSubtitle->SetText(WelcomeSubtitleText);
	if (TipLabel)        TipLabel->SetText(TipText);
	if (HeroCtaText)     HeroCtaText->SetText(DiscoverText);
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

	// Survol : les boutons se soulevent legerement (+ petit son)
	for (UButton* Tab : { TabExperiences.Get(), TabProject.Get(), TabHelp.Get(), TabSettings.Get() })
	{
		AnimateHover(Tab, Tab, 1.03f, 0.f);
	}
	for (UButton* Arrow : { HeroPrev.Get(), HeroNext.Get(), RowPrev.Get(), RowNext.Get(), StopButton.Get() })
	{
		AnimateHover(Arrow, Arrow, 1.1f, 0.f);
	}
	if (HeroButton && HeroCta)
	{
		AnimateHover(HeroCta, HeroButton, 1.06f, -4.f);
	}

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
			Entry->SetAccent(ShowroomStyle::ExperienceColor(Entries.Num()), Entries.Num() + 1);
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
			UTextBlock* Soon = ShowroomStyle::MakeText(WidgetTree, NAME_None, ComingSoonLabel, 24, TextDim(), TEXT("Regular"), false);
			Soon->SetJustification(ETextJustify::Center);
			Column->AddChildToVerticalBox(Soon);
			RowItems.Add(Tile);
		}

		for (int32 i = 0; i < RowItems.Num(); ++i)
		{
			AnimateIn(RowItems[i], 0.15f + 0.07f * i, FVector2D(0.f, 40.f), 0.4f); // apparition en cascade
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

	UTexture2D* Photo = Zone ? Zone->Info.Photo.Get() : nullptr;
	if (HeroImage)
	{
		const FVector2D Size = HeroImage->GetBrush().ImageSize;
		FSlateBrush Brush = ShowroomStyle::RoundedImageBrush(Photo, ShowroomStyle::RadiusCard, Size);
		if (!Photo)
		{
			// Pas de photo : la couleur de l'experience (la meme que sa vignette)
			const int32 Order = Zone ? Entries.IndexOfByPredicate([Zone](const UExperienceEntryWidget* E) { return E && E->GetZone() == Zone; }) : 0;
			Brush.TintColor = FSlateColor(ShowroomStyle::ExperienceColor(FMath::Max(Order, 0)) * 0.75f);
		}
		HeroImage->SetBrush(Brush);
		AnimateIn(HeroImage, 0.f, FVector2D::ZeroVector, 0.45f); // fondu entre deux experiences
	}
	if (HeroArt)
	{
		HeroArt->SetVisibility(Photo ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (HeroCaption)
	{
		AnimateIn(HeroCaption, 0.08f, FVector2D(-40.f, 0.f), 0.45f);
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
		AnimateIn(PageSwitcher->GetActiveWidget(), 0.f, FVector2D(0.f, 24.f), 0.3f);
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
	if (StatusIcon)
	{
		ShowroomStyle::SetIcon(StatusIcon, TEXT("check"));
	}
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
	if (StatusIcon)
	{
		if (Selected.IsValid())
		{
			ShowroomStyle::SetIcon(StatusIcon, TEXT("pin"));
			AnimatePulse(StatusIcon, 1.25f, 1.1f); // le GPS est actif : l'icone "respire"
		}
		else
		{
			StopPulse(StatusIcon);
			ShowroomStyle::SetIcon(StatusIcon, TEXT("pointer"));
		}
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
