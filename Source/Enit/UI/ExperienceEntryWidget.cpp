#include "UI/ExperienceEntryWidget.h"
#include "UI/ShowroomStyle.h"
#include "Interaction/DetectionZone.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

void UExperienceEntryWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  SizeBox "Tile" (taille fixe de la vignette)
	//   └ Button "EntryButton" (toute la vignette est cliquable, se souleve au survol)
	//       └ VerticalBox
	//           ├ Overlay "Visual" : Thumbnail (photo ou couleur), TileIcon, NumberText (badge), GpsBadge
	//           └ HorizontalBox : TitleText / SubtitleText | fleche
	USizeBox* Tile = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Tile"));
	Tile->SetWidthOverride(TileSize.X);
	Tile->SetHeightOverride(TileSize.Y);
	WidgetTree->RootWidget = Tile;

	EntryButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("EntryButton"), Surface(), SurfaceHover(), RadiusCard, FMargin(8.f));
	Tile->SetContent(EntryButton);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	EntryButton->SetContent(Column);
	if (UButtonSlot* ColumnSlot = Cast<UButtonSlot>(Column->Slot))
	{
		// Toutes les vignettes alignees en haut, quelle que soit la longueur du titre.
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ImageBox"));
	ImageBox->SetHeightOverride(ImageHeight);
	Column->AddChildToVerticalBox(ImageBox);
	UOverlay* Visual = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Visual"));
	ImageBox->SetContent(Visual);

	Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Thumbnail"));
	Thumbnail->SetBrush(RoundedImageBrush(nullptr, RadiusSmall, FVector2D(TileSize.X, ImageHeight)));
	if (UOverlaySlot* ThumbSlot = Visual->AddChildToOverlay(Thumbnail))
	{
		ThumbSlot->SetHorizontalAlignment(HAlign_Fill);
		ThumbSlot->SetVerticalAlignment(VAlign_Fill);
	}

	TileIcon = MakeIcon(WidgetTree, TEXT("TileIcon"), TEXT("cube"), 92.f, FLinearColor(1.f, 1.f, 1.f, 0.85f));
	if (UOverlaySlot* IconSlot = Visual->AddChildToOverlay(TileIcon))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* NumberBadge = MakeBox(WidgetTree, TEXT("NumberBadge"), Shade(), 999.f, FMargin(12.f, 3.f));
	NumberText = ShowroomStyle::MakeText(WidgetTree, TEXT("NumberText"), FText::FromString(TEXT("01")), 19, TextMain(), TEXT("Bold"), false);
	NumberBadge->SetContent(NumberText);
	if (UOverlaySlot* NumberSlot = Visual->AddChildToOverlay(NumberBadge))
	{
		NumberSlot->SetHorizontalAlignment(HAlign_Left);
		NumberSlot->SetVerticalAlignment(VAlign_Top);
		NumberSlot->SetPadding(FMargin(10.f));
	}

	UBorder* Gps = MakeBox(WidgetTree, TEXT("GpsBadge"), ShowroomStyle::Accent(), 999.f, FMargin(10.f, 4.f, 14.f, 4.f));
	UTextBlock* GpsLabel = nullptr;
	Gps->SetContent(MakeIconLabel(WidgetTree, TEXT("GpsBadgeLabel"), TEXT("pin"), FText::FromString(TEXT("GPS")), 18, TextMain(), TEXT("Bold"), GpsLabel, 6.f));
	Gps->SetVisibility(ESlateVisibility::Collapsed);
	GpsBadge = Gps;
	if (UOverlaySlot* GpsSlot = Visual->AddChildToOverlay(Gps))
	{
		GpsSlot->SetHorizontalAlignment(HAlign_Right);
		GpsSlot->SetVerticalAlignment(VAlign_Top);
		GpsSlot->SetPadding(FMargin(10.f));
	}

	UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Label"));
	if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetPadding(FMargin(10.f, 8.f, 6.f, 2.f));
	}

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	if (UHorizontalBoxSlot* TextsSlot = Label->AddChildToHorizontalBox(Texts))
	{
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Top);
	}
	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Expérience")), 25, TextMain(), TEXT("Bold"), true);
	TitleText->SetClipping(EWidgetClipping::ClipToBounds);
	Texts->AddChildToVerticalBox(TitleText);
	SubtitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("SubtitleText"), FText::GetEmpty(), 18, TextMuted(), TEXT("Regular"), false);
	SubtitleText->SetClipping(EWidgetClipping::ClipToBounds);
	SubtitleText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis); // "Visite guidée · 9 ét…" plutot que coupe net
	if (UVerticalBoxSlot* SubSlot = Texts->AddChildToVerticalBox(SubtitleText))
	{
		SubSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	if (UHorizontalBoxSlot* ChevronSlot = Label->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("ChevronIcon"), TEXT("chevron_right"), 30.f, TextMuted())))
	{
		ChevronSlot->SetVerticalAlignment(VAlign_Center);
		ChevronSlot->SetPadding(FMargin(8.f, 0.f, 2.f, 0.f));
	}
}

void UExperienceEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &UExperienceEntryWidget::HandleClicked);
		EntryButton->OnClicked.AddDynamic(this, &UExperienceEntryWidget::HandleClicked);
		AnimateHover(EntryButton, EntryButton, 1.05f, -10.f); // la vignette se souleve au survol
	}
	if (TileIcon)
	{
		AnimateHover(TileIcon, EntryButton, 1.15f, 0.f); // ... et son icone grossit un peu
	}
	Refresh();
}

void UExperienceEntryWidget::SetZone(ADetectionZone* InZone)
{
	Zone = InZone;
	Refresh();
}

void UExperienceEntryWidget::SetAccent(const FLinearColor& Color, int32 InNumber)
{
	Accent = Color;
	Number = InNumber;
	Refresh();
}

void UExperienceEntryWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	// Vignette de l'experience guidee par le GPS : fond bleu + badge "GPS".
	ShowroomStyle::SetButtonColors(EntryButton,
		bSelected ? ShowroomStyle::AccentDeep() : ShowroomStyle::Surface(),
		bSelected ? ShowroomStyle::Accent() : ShowroomStyle::SurfaceHover(),
		ShowroomStyle::RadiusCard);
	if (GpsBadge)
	{
		GpsBadge->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bSelected)
		{
			AnimatePulse(GpsBadge, 1.08f, 1.f);
		}
		else
		{
			StopPulse(GpsBadge);
		}
	}
}

void UExperienceEntryWidget::HandleClicked()
{
	OnSelected.Broadcast(Zone.Get());
}

void UExperienceEntryWidget::Refresh()
{
	const ADetectionZone* Z = Zone.Get();
	if (NumberText)
	{
		NumberText->SetText(FText::FromString(FString::Printf(TEXT("%02d"), FMath::Max(Number, 1))));
		if (UWidget* Badge = NumberText->GetParent())
		{
			Badge->SetVisibility(Number > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	if (!Z)
	{
		return;
	}
	if (TitleText)
	{
		TitleText->SetText(Z->Info.Title);
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(Z->Info.Subtitle);
		SubtitleText->SetVisibility(Z->Info.Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	UTexture2D* Photo = Z->Info.Photo;
	if (Thumbnail)
	{
		// Pas de photo : la couleur de l'experience + une grande icone.
		const FVector2D Size = Thumbnail->GetBrush().ImageSize;
		FSlateBrush Brush = ShowroomStyle::RoundedImageBrush(Photo, ShowroomStyle::RadiusSmall, Size);
		if (!Photo)
		{
			Brush.TintColor = FSlateColor(Accent);
		}
		Thumbnail->SetBrush(Brush);
	}
	if (TileIcon)
	{
		TileIcon->SetVisibility(Photo ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	SetSelected(bIsSelected);
}
