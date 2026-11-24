#include "UI/ExperienceEntryWidget.h"
#include "UI/ShowroomStyle.h"
#include "Interaction/DetectionZone.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

void UExperienceEntryWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  SizeBox "Tile" (taille fixe de la vignette)
	//   └ Button "EntryButton" (toute la vignette est cliquable)
	//       └ VerticalBox
	//           ├ Thumbnail (photo, coins arrondis)
	//           └ HorizontalBox : TitleText (+ SubtitleText cache) | fleche
	USizeBox* Tile = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Tile"));
	Tile->SetWidthOverride(TileSize.X);
	Tile->SetHeightOverride(TileSize.Y);
	WidgetTree->RootWidget = Tile;

	EntryButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("EntryButton"), Surface(), SurfaceHover(), RadiusCard, FMargin(8.f));
	Tile->SetContent(EntryButton);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	EntryButton->SetContent(Column);

	USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ImageBox"));
	ImageBox->SetHeightOverride(ImageHeight);
	Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Thumbnail"));
	Thumbnail->SetBrush(RoundedImageBrush(nullptr, RadiusSmall, FVector2D(TileSize.X, ImageHeight)));
	ImageBox->SetContent(Thumbnail);
	Column->AddChildToVerticalBox(ImageBox);

	UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Label"));
	if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(Label))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetPadding(FMargin(8.f, 6.f, 6.f, 0.f));
	}

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	if (UHorizontalBoxSlot* TextsSlot = Label->AddChildToHorizontalBox(Texts))
	{
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
	}
	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Expérience")), 20, TextMain(), TEXT("Bold"), true);
	TitleText->SetClipping(EWidgetClipping::ClipToBounds);
	Texts->AddChildToVerticalBox(TitleText);
	SubtitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("SubtitleText"), FText::GetEmpty(), 18, TextMuted(), TEXT("Regular"), false);
	SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
	Texts->AddChildToVerticalBox(SubtitleText);

	UTextBlock* Chevron = ShowroomStyle::MakeText(WidgetTree, TEXT("ChevronText"), FText::FromString(TEXT("›")), 40, TextMuted(), TEXT("Bold"), false);
	if (UHorizontalBoxSlot* ChevronSlot = Label->AddChildToHorizontalBox(Chevron))
	{
		ChevronSlot->SetVerticalAlignment(VAlign_Center);
		ChevronSlot->SetPadding(FMargin(8.f, 0.f, 4.f, 4.f));
	}
}

void UExperienceEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &UExperienceEntryWidget::HandleClicked);
		EntryButton->OnClicked.AddDynamic(this, &UExperienceEntryWidget::HandleClicked);
	}
	Refresh();
}

void UExperienceEntryWidget::SetZone(ADetectionZone* InZone)
{
	Zone = InZone;
	Refresh();
}

void UExperienceEntryWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	// Vignette de l'experience guidee par le GPS : fond bleu.
	ShowroomStyle::SetButtonColors(EntryButton,
		bSelected ? ShowroomStyle::AccentDeep() : ShowroomStyle::Surface(),
		bSelected ? ShowroomStyle::Accent() : ShowroomStyle::SurfaceHover(),
		ShowroomStyle::RadiusCard);
}

void UExperienceEntryWidget::HandleClicked()
{
	OnSelected.Broadcast(Zone.Get());
}

void UExperienceEntryWidget::Refresh()
{
	const ADetectionZone* Z = Zone.Get();
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
	}
	if (Thumbnail)
	{
		// Pas de photo : fond uni (la vignette garde sa forme).
		const FVector2D Size = Thumbnail->GetBrush().ImageSize;
		Thumbnail->SetBrush(ShowroomStyle::RoundedImageBrush(Z->Info.Photo, ShowroomStyle::RadiusSmall, Size));
	}
	SetSelected(bIsSelected);
}
