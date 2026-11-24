#include "UI/ExperienceSheetWidget.h"
#include "UI/AuthorCardWidget.h"
#include "UI/ShowroomStyle.h"
#include "Interaction/DetectionZone.h"
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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr float GalleryHeight = 300.f;

	/** Petite pastille arrondie avec un texte (domaine, duree, lieu...). */
	UBorder* MakePill(UWidgetTree* Tree, const TCHAR* BoxName, const TCHAR* TextName, const FLinearColor& Color,
		int32 FontSize, const FLinearColor& TextColor, UTextBlock*& OutText, const TCHAR* IconName = nullptr)
	{
		UBorder* Box = ShowroomStyle::MakeBox(Tree, BoxName, Color, 999.f, FMargin(IconName ? 12.f : 16.f, 6.f, 16.f, 6.f));
		if (IconName)
		{
			UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(FString(BoxName) + TEXT("Row")));
			if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(ShowroomStyle::MakeIcon(Tree, *(FString(BoxName) + TEXT("Icon")), IconName, FontSize * 1.1f, ShowroomStyle::AccentText())))
			{
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			}
			OutText = ShowroomStyle::MakeText(Tree, TextName, FText::FromString(TEXT("…")), FontSize, TextColor, TEXT("Bold"), false);
			if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutText))
			{
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
			Box->SetContent(Row);
			return Box;
		}
		OutText = ShowroomStyle::MakeText(Tree, TextName, FText::FromString(TEXT("…")), FontSize, TextColor, TEXT("Bold"), false);
		Box->SetContent(OutText);
		return Box;
	}

	/** Bouton rond avec un symbole (fleches de la galerie). */
	UButton* MakeRoundButton(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* IconName)
	{
		UButton* Button = ShowroomStyle::MakeButton(Tree, Name, ShowroomStyle::Shade(), ShowroomStyle::AccentDeep(), 999.f, FMargin(16.f));
		Button->SetContent(ShowroomStyle::MakeIcon(Tree, *(FString(Name) + TEXT("Icon")), IconName, 36.f, ShowroomStyle::TextMain()));
		return Button;
	}

	void SetTextOrHide(UTextBlock* Text, UWidget* Container, const FText& Value)
	{
		if (Text)
		{
			Text->SetText(Value);
		}
		if (UWidget* Target = Container ? Container : Text)
		{
			Target->SetVisibility(Value.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}
}

UExperienceSheetWidget::UExperienceSheetWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AuthorCardClass = TSoftClassPtr<UAuthorCardWidget>(FSoftObjectPath(TEXT("/Game/Showroom/UI/WBP_AuthorCard.WBP_AuthorCard_C")));
}

void UExperienceSheetWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  HorizontalBox "Columns"
	//   ├ VerticalBox "LeftColumn"
	//   │   ├ Overlay "Gallery" : GalleryImage, BackButton, GalleryCounter, GalleryPrev, GalleryNext
	//   │   ├ TagBox > TagText
	//   │   ├ TitleText, SubtitleText
	//   │   ├ HorizontalBox "MetaRow" : DomainBox, DurationBox, LocationBox
	//   │   └ Border "DescriptionBox" > DescriptionText
	//   └ VerticalBox "RightColumn"
	//       ├ AuthorsTitle
	//       ├ VerticalBox "AuthorList" (cartes ajoutees par le code)
	//       └ GpsButton : icone | GpsTitle / GpsSubtitle | fleche
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Columns"));
	WidgetTree->RootWidget = Columns;

	// ---------------- Colonne gauche ----------------
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumn"));
	LeftColumn = Left;
	if (UHorizontalBoxSlot* LeftSlot = Columns->AddChildToHorizontalBox(Left))
	{
		FSlateChildSize LeftSize(ESlateSizeRule::Fill);
		LeftSize.Value = 1.35f; // la colonne image + description est la plus large
		LeftSlot->SetSize(LeftSize);
		LeftSlot->SetPadding(FMargin(0.f, 0.f, 28.f, 0.f));
	}

	USizeBox* GalleryBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GalleryBox"));
	GalleryBox->SetHeightOverride(GalleryHeight);
	Left->AddChildToVerticalBox(GalleryBox);
	UOverlay* Gallery = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Gallery"));
	GalleryBox->SetContent(Gallery);

	GalleryImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("GalleryImage"));
	GalleryImage->SetBrush(RoundedImageBrush(nullptr, RadiusCard, FVector2D(470.f, GalleryHeight)));
	if (UOverlaySlot* ImageSlot = Gallery->AddChildToOverlay(GalleryImage))
	{
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}
	GalleryArt = MakeIcon(WidgetTree, TEXT("GalleryArt"), TEXT("cube"), 150.f, FLinearColor(1.f, 1.f, 1.f, 0.35f));
	if (UOverlaySlot* ArtSlot = Gallery->AddChildToOverlay(GalleryArt))
	{
		ArtSlot->SetHorizontalAlignment(HAlign_Center);
		ArtSlot->SetVerticalAlignment(VAlign_Center);
	}

	BackButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("BackButton"), Shade(), AccentDeep(), 999.f, FMargin(16.f, 8.f, 24.f, 8.f));
	UTextBlock* BackText = nullptr;
	BackButton->SetContent(MakeIconLabel(WidgetTree, TEXT("BackButtonLabel"), TEXT("arrow_left"), FText::FromString(TEXT("Retour")), 26, TextMain(), TEXT("Bold"), BackText, 10.f));
	if (UOverlaySlot* BackSlot = Gallery->AddChildToOverlay(BackButton))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Left);
		BackSlot->SetVerticalAlignment(VAlign_Top);
		BackSlot->SetPadding(FMargin(14.f));
	}

	UBorder* CounterBox = MakeBox(WidgetTree, TEXT("GalleryCounterBox"), Shade(), 999.f, FMargin(14.f, 4.f));
	GalleryCounter = ShowroomStyle::MakeText(WidgetTree, TEXT("GalleryCounter"), FText::FromString(TEXT("1 / 1")), 22, TextMain(), TEXT("Bold"), false);
	CounterBox->SetContent(GalleryCounter);
	if (UOverlaySlot* CounterSlot = Gallery->AddChildToOverlay(CounterBox))
	{
		CounterSlot->SetHorizontalAlignment(HAlign_Right);
		CounterSlot->SetVerticalAlignment(VAlign_Top);
		CounterSlot->SetPadding(FMargin(14.f));
	}

	GalleryPrev = MakeRoundButton(WidgetTree, TEXT("GalleryPrev"), TEXT("chevron_left"));
	if (UOverlaySlot* PrevSlot = Gallery->AddChildToOverlay(GalleryPrev))
	{
		PrevSlot->SetHorizontalAlignment(HAlign_Left);
		PrevSlot->SetVerticalAlignment(VAlign_Center);
		PrevSlot->SetPadding(FMargin(14.f, 40.f, 0.f, 0.f));
	}
	GalleryNext = MakeRoundButton(WidgetTree, TEXT("GalleryNext"), TEXT("chevron_right"));
	if (UOverlaySlot* NextSlot = Gallery->AddChildToOverlay(GalleryNext))
	{
		NextSlot->SetHorizontalAlignment(HAlign_Right);
		NextSlot->SetVerticalAlignment(VAlign_Center);
		NextSlot->SetPadding(FMargin(0.f, 40.f, 14.f, 0.f));
	}

	UHorizontalBox* TagRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TagRow"));
	if (UVerticalBoxSlot* TagSlot = Left->AddChildToVerticalBox(TagRow))
	{
		TagSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 4.f));
	}
	UTextBlock* Tag = nullptr;
	TagBox = MakePill(WidgetTree, TEXT("TagBox"), TEXT("TagText"), ShowroomStyle::Tag(), 18, TextMain(), Tag, TEXT("sparkle"));
	TagText = Tag;
	TagRow->AddChildToHorizontalBox(TagBox);

	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("TITRE")), 44, TextMain(), TEXT("Bold"));
	Left->AddChildToVerticalBox(TitleText);
	SubtitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("SubtitleText"), FText::FromString(TEXT("Sous-titre")), 27, TextMuted());
	Left->AddChildToVerticalBox(SubtitleText);

	UHorizontalBox* Meta = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MetaRow"));
	if (UVerticalBoxSlot* MetaSlot = Left->AddChildToVerticalBox(Meta))
	{
		MetaSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 14.f));
	}
	UTextBlock* Domain = nullptr;
	UTextBlock* Duration = nullptr;
	UTextBlock* Location = nullptr;
	DomainBox = MakePill(WidgetTree, TEXT("DomainBox"), TEXT("DomainText"), Surface(), 20, TextMain(), Domain, TEXT("tag"));
	DurationBox = MakePill(WidgetTree, TEXT("DurationBox"), TEXT("DurationText"), Surface(), 20, TextMain(), Duration, TEXT("clock"));
	LocationBox = MakePill(WidgetTree, TEXT("LocationBox"), TEXT("LocationText"), Surface(), 20, TextMain(), Location, TEXT("pin"));
	DomainText = Domain;
	DurationText = Duration;
	LocationText = Location;
	for (UWidget* Pill : { DomainBox.Get(), DurationBox.Get(), LocationBox.Get() })
	{
		if (UHorizontalBoxSlot* PillSlot = Meta->AddChildToHorizontalBox(Pill))
		{
			PillSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}
	}

	UBorder* DescriptionBox = MakeBox(WidgetTree, TEXT("DescriptionBox"), SurfaceDeep(), RadiusCard, FMargin(20.f, 16.f));
	DescriptionText = ShowroomStyle::MakeText(WidgetTree, TEXT("DescriptionText"), FText::FromString(TEXT("Description de l'expérience.")), 25, TextMain());
	DescriptionBox->SetContent(DescriptionText);
	DescriptionBox->SetVerticalAlignment(VAlign_Top);
	if (UVerticalBoxSlot* DescSlot = Left->AddChildToVerticalBox(DescriptionBox))
	{
		DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ---------------- Colonne droite ----------------
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightColumn"));
	RightColumn = Right;
	if (UHorizontalBoxSlot* RightSlot = Columns->AddChildToHorizontalBox(Right))
	{
		RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* AuthorsTitleText = nullptr;
	UHorizontalBox* AuthorsHeader = MakeIconLabel(WidgetTree, TEXT("AuthorsHeader"), TEXT("user"), FText::FromString(TEXT("LES AUTEURS DE L'EXPÉRIENCE")), 21, AccentText(), TEXT("Bold"), AuthorsTitleText, 10.f);
	AuthorsTitle = AuthorsTitleText;
	if (UVerticalBoxSlot* AuthorsTitleSlot = Right->AddChildToVerticalBox(AuthorsHeader))
	{
		AuthorsTitleSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 12.f));
	}

	UVerticalBox* Authors = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AuthorList"));
	AuthorList = Authors;
	if (UVerticalBoxSlot* AuthorsSlot = Right->AddChildToVerticalBox(Authors))
	{
		AuthorsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	GpsButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("GpsButton"), Accent(), AccentHover(), RadiusCard, FMargin(18.f, 14.f));
	if (UVerticalBoxSlot* GpsSlot = Right->AddChildToVerticalBox(GpsButton))
	{
		GpsSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	UHorizontalBox* GpsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GpsRow"));
	GpsButton->SetContent(GpsRow);

	UBorder* GpsIcon = MakeBox(WidgetTree, TEXT("GpsIcon"), FLinearColor(1.f, 1.f, 1.f, 0.16f), 999.f, FMargin(14.f));
	GpsIconImage = MakeIcon(WidgetTree, TEXT("GpsIconImage"), TEXT("pin"), 40.f, TextMain());
	GpsIcon->SetContent(GpsIconImage);
	if (UHorizontalBoxSlot* IconSlot = GpsRow->AddChildToHorizontalBox(GpsIcon))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}
	UVerticalBox* GpsTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GpsTexts"));
	if (UHorizontalBoxSlot* GpsTextsSlot = GpsRow->AddChildToHorizontalBox(GpsTexts))
	{
		GpsTextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GpsTextsSlot->SetVerticalAlignment(VAlign_Center);
	}
	GpsTitle = ShowroomStyle::MakeText(WidgetTree, TEXT("GpsTitle"), GpsIdleTitle, 19, FLinearColor(1.f, 1.f, 1.f, 0.75f), TEXT("Bold"));
	GpsTexts->AddChildToVerticalBox(GpsTitle);
	GpsSubtitle = ShowroomStyle::MakeText(WidgetTree, TEXT("GpsSubtitle"), GpsIdleSubtitle, 29, TextMain(), TEXT("Bold"));
	GpsTexts->AddChildToVerticalBox(GpsSubtitle);
	if (UHorizontalBoxSlot* ChevronSlot = GpsRow->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("GpsChevron"), TEXT("chevron_right"), 34.f, TextMain())))
	{
		ChevronSlot->SetVerticalAlignment(VAlign_Center);
		ChevronSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 4.f));
	}
}

void UExperienceSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackButton)  BackButton->OnClicked.AddUniqueDynamic(this, &UExperienceSheetWidget::HandleBackClicked);
	if (GpsButton)   GpsButton->OnClicked.AddUniqueDynamic(this, &UExperienceSheetWidget::HandleGpsClicked);
	if (GalleryPrev) GalleryPrev->OnClicked.AddUniqueDynamic(this, &UExperienceSheetWidget::HandlePrevImage);
	if (GalleryNext) GalleryNext->OnClicked.AddUniqueDynamic(this, &UExperienceSheetWidget::HandleNextImage);
	AnimateHover(GpsButton, GpsButton, 1.03f, -4.f);
	AnimateHover(BackButton, BackButton, 1.06f, 0.f);
	AnimateHover(GalleryPrev, GalleryPrev, 1.12f, 0.f);
	AnimateHover(GalleryNext, GalleryNext, 1.12f, 0.f);
	Refresh();
}

void UExperienceSheetWidget::SetZone(ADetectionZone* InZone)
{
	if (Zone.Get() != InZone)
	{
		ImageIndex = 0;
	}
	Zone = InZone;
	Refresh();
	// La fiche arrive : les deux colonnes glissent en place.
	AnimateIn(LeftColumn, 0.f, FVector2D(-40.f, 0.f), 0.35f);
	AnimateIn(RightColumn, 0.08f, FVector2D(40.f, 0.f), 0.35f);
}

void UExperienceSheetWidget::SetGpsState(EExperienceGpsState State)
{
	GpsState = State;
	RefreshGps();
}

void UExperienceSheetWidget::HandleBackClicked()
{
	OnBack.Broadcast();
}

void UExperienceSheetWidget::HandleGpsClicked()
{
	OnGpsClicked.Broadcast(Zone.Get());
}

void UExperienceSheetWidget::HandlePrevImage()
{
	ShowImage(ImageIndex - 1);
}

void UExperienceSheetWidget::HandleNextImage()
{
	ShowImage(ImageIndex + 1);
}

void UExperienceSheetWidget::ShowImage(int32 Index)
{
	const ADetectionZone* Z = Zone.Get();
	const int32 Count = Z ? Z->Info.GetImages().Num() : 0;
	ImageIndex = Count > 0 ? (Index % Count + Count) % Count : 0; // fait le tour dans les deux sens
	RefreshGallery();
}

void UExperienceSheetWidget::Refresh()
{
	const ADetectionZone* Z = Zone.Get();
	if (!Z)
	{
		return;
	}
	const FShowroomInfo& Info = Z->Info;

	SetTextOrHide(TagText, TagBox, Info.Tag.ToUpper());
	SetTextOrHide(TitleText, nullptr, Info.Title.ToUpper());
	SetTextOrHide(SubtitleText, nullptr, Info.Subtitle);
	SetTextOrHide(DomainText, DomainBox, Info.Domain);
	SetTextOrHide(DurationText, DurationBox, Info.Duration);
	SetTextOrHide(LocationText, LocationBox, Info.Location);
	if (DescriptionText)
	{
		DescriptionText->SetText(Info.Description);
	}

	// Les auteurs : une carte par auteur (Widget Blueprint WBP_AuthorCard).
	if (AuthorList)
	{
		AuthorList->ClearChildren();
		TSubclassOf<UAuthorCardWidget> CardClass = AuthorCardClass.LoadSynchronous();
		if (!CardClass)
		{
			CardClass = UAuthorCardWidget::StaticClass();
		}
		const int32 Count = FMath::Min(Info.Authors.Num(), MaxAuthors);
		for (int32 i = 0; i < Count; ++i)
		{
			UAuthorCardWidget* Card = CreateWidget<UAuthorCardWidget>(this, CardClass);
			if (!Card)
			{
				continue;
			}
			Card->SetAuthor(Info.Authors[i]);
			if (UVerticalBoxSlot* CardSlot = Cast<UVerticalBoxSlot>(AuthorList->AddChild(Card)))
			{
				CardSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			}
		}
		if (Count == 0)
		{
			AuthorList->AddChild(ShowroomStyle::MakeText(WidgetTree, NAME_None, NoAuthors, 23, ShowroomStyle::TextDim()));
		}
	}

	RefreshGallery();
	RefreshGps();
}

void UExperienceSheetWidget::RefreshGallery()
{
	const ADetectionZone* Z = Zone.Get();
	const TArray<UTexture2D*> Images = Z ? Z->Info.GetImages() : TArray<UTexture2D*>();
	const int32 Count = Images.Num();

	if (GalleryImage)
	{
		UTexture2D* Current = Count > 0 ? Images[FMath::Clamp(ImageIndex, 0, Count - 1)] : nullptr;
		const FVector2D Size = GalleryImage->GetBrush().ImageSize;
		GalleryImage->SetBrush(ShowroomStyle::RoundedImageBrush(Current, ShowroomStyle::RadiusCard, Size));
		AnimateIn(GalleryImage, 0.f, FVector2D::ZeroVector, 0.3f); // fondu entre deux images
	}
	if (GalleryArt)
	{
		GalleryArt->SetVisibility(Count > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	// Fleches et compteur seulement s'il y a plusieurs images.
	const ESlateVisibility Multi = Count > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (GalleryPrev) GalleryPrev->SetVisibility(Multi);
	if (GalleryNext) GalleryNext->SetVisibility(Multi);
	if (GalleryCounter)
	{
		GalleryCounter->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), ImageIndex + 1, FMath::Max(Count, 1))));
		if (UWidget* CounterBox = GalleryCounter->GetParent())
		{
			CounterBox->SetVisibility(Count > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UExperienceSheetWidget::RefreshGps()
{
	FText Title = GpsIdleTitle;
	FText Subtitle = GpsIdleSubtitle;
	FLinearColor Normal = ShowroomStyle::Accent();
	FLinearColor Hovered = ShowroomStyle::AccentHover();
	switch (GpsState)
	{
	case EExperienceGpsState::Guiding:
		Title = GpsGuidingTitle;
		Subtitle = GpsGuidingSubtitle;
		Normal = ShowroomStyle::AccentDeep();
		Hovered = ShowroomStyle::Accent();
		break;
	case EExperienceGpsState::Arrived:
		Title = GpsArrivedTitle;
		Subtitle = GpsArrivedSubtitle;
		Normal = ShowroomStyle::Surface();
		Hovered = ShowroomStyle::SurfaceHover();
		break;
	default:
		break;
	}
	if (GpsTitle) GpsTitle->SetText(Title);
	if (GpsSubtitle) GpsSubtitle->SetText(Subtitle);
	if (GpsIconImage)
	{
		ShowroomStyle::SetIcon(GpsIconImage, GpsState == EExperienceGpsState::Arrived ? TEXT("check") : TEXT("pin"));
	}
	// Le bouton invite au clic (pret) ou montre que le GPS travaille (en cours).
	if (GpsState == EExperienceGpsState::Arrived)
	{
		StopPulse(GpsIconImage);
	}
	else
	{
		AnimatePulse(GpsIconImage, GpsState == EExperienceGpsState::Guiding ? 1.22f : 1.12f, GpsState == EExperienceGpsState::Guiding ? 1.2f : 0.7f);
	}
	ShowroomStyle::SetButtonColors(GpsButton, Normal, Hovered, ShowroomStyle::RadiusCard);
}
