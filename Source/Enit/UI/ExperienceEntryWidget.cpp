#include "UI/ExperienceEntryWidget.h"
#include "Interaction/DetectionZone.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"

void UExperienceEntryWidget::BuildDefaultLayout()
{
	//  Button arrondi (toute la ligne est cliquable)
	//   └ HorizontalBox
	//       ├ SizeBox > Image (miniature)
	//       └ VerticalBox : titre, sous-titre
	EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EntryButton"));
	FButtonStyle Style = EntryButton->GetStyle();
	Style.SetNormal(MakeRoundedBrush(NormalColor, 22.f));
	Style.SetHovered(MakeRoundedBrush(NormalColor * 1.8f, 22.f));
	Style.SetPressed(MakeRoundedBrush(SelectedColor, 22.f));
	Style.SetNormalPadding(FMargin(18.f, 14.f));
	Style.SetPressedPadding(FMargin(18.f, 15.f, 18.f, 13.f));
	EntryButton->SetStyle(Style);
	WidgetTree->RootWidget = EntryButton;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	EntryButton->SetContent(Row);

	USizeBox* ThumbBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ThumbnailBox"));
	ThumbBox->SetWidthOverride(120.f);
	ThumbBox->SetHeightOverride(90.f);
	Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Thumbnail"));
	ThumbBox->SetContent(Thumbnail);
	if (UHorizontalBoxSlot* ThumbSlot = Row->AddChildToHorizontalBox(ThumbBox))
	{
		ThumbSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
		ThumbSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	if (UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts))
	{
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
	}
	TitleText = MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Expérience")), 34, FLinearColor::White);
	Texts->AddChildToVerticalBox(TitleText);
	SubtitleText = MakeText(WidgetTree, TEXT("SubtitleText"), FText::FromString(TEXT("Sous-titre")), 22, FLinearColor(0.7f, 0.78f, 0.9f, 1.f));
	Texts->AddChildToVerticalBox(SubtitleText);
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
	if (EntryButton)
	{
		// Teinte du bouton : bleu quand l'experience est choisie.
		EntryButton->SetBackgroundColor(bSelected ? SelectedTint : FLinearColor::White);
	}
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
	if (TitleText)    TitleText->SetText(Z->Info.Title);
	if (SubtitleText) SubtitleText->SetText(Z->Info.Subtitle);
	if (Thumbnail)
	{
		// Pas de photo : on cache la miniature (et son cadre s'il y en a un).
		UWidget* Frame = Cast<USizeBox>(Thumbnail->GetParent()) ? static_cast<UWidget*>(Thumbnail->GetParent()) : static_cast<UWidget*>(Thumbnail);
		if (Z->Info.Photo)
		{
			Thumbnail->SetBrushFromTexture(Z->Info.Photo);
			Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Frame->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	SetSelected(bIsSelected);
}
