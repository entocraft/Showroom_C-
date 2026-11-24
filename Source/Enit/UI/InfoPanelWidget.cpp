#include "UI/InfoPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	void SetFontSize(UTextBlock* Text, int32 Size)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}
}

void UInfoPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ActionButton)
	{
		ActionButton->OnClicked.RemoveDynamic(this, &UInfoPanelWidget::HandleActionClicked);
		ActionButton->OnClicked.AddDynamic(this, &UInfoPanelWidget::HandleActionClicked);
	}
	if (PreviousButton)
	{
		PreviousButton->OnClicked.RemoveDynamic(this, &UInfoPanelWidget::HandlePreviousClicked);
		PreviousButton->OnClicked.AddDynamic(this, &UInfoPanelWidget::HandlePreviousClicked);
	}
	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(this, &UInfoPanelWidget::HandleNextClicked);
		NextButton->OnClicked.AddDynamic(this, &UInfoPanelWidget::HandleNextClicked);
	}
	Refresh();
}

void UInfoPanelWidget::SetActionButton(bool bVisible, const FText& Label)
{
	bActionVisible = bVisible;
	ActionLabel = Label;
	Refresh();
}

void UInfoPanelWidget::SetStepNav(bool bVisible, const FText& InStepLabel, bool bInCanPrevious, bool bInCanNext)
{
	bNavVisible = bVisible;
	StepLabel = InStepLabel;
	bCanPrevious = bInCanPrevious;
	bCanNext = bInCanNext;
	Refresh();
}

void UInfoPanelWidget::SetInfo(const FShowroomInfo& NewInfo)
{
	Info = NewInfo;
	Refresh();
}

void UInfoPanelWidget::Refresh()
{
	if (TitleText)       TitleText->SetText(Info.Title);
	if (SubtitleText)    SubtitleText->SetText(Info.Subtitle);
	if (DescriptionText) DescriptionText->SetText(Info.Description);

	if (ActionButton)
	{
		ActionButton->SetVisibility(bActionVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ActionButtonText)
	{
		ActionButtonText->SetText(ActionLabel);
	}

	if (StepNav)
	{
		StepNav->SetVisibility(bNavVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (StepText)
	{
		StepText->SetText(StepLabel);
	}
	// "Hidden" (et pas "Collapsed") : le texte "Etape x / y" reste bien au centre.
	if (PreviousButton)
	{
		PreviousButton->SetVisibility(bCanPrevious ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (NextButton)
	{
		NextButton->SetVisibility(bCanNext ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (PhotoImage)
	{
		if (Info.Photo)
		{
			PhotoImage->SetBrushFromTexture(Info.Photo);
			PhotoImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			PhotoImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UInfoPanelWidget::BuildDefaultLayout()
{
	//  Border (fond)
	//   └ VerticalBox
	//       ├ HorizontalBox
	//       │   ├ SizeBox > Image (photo)
	//       │   └ VerticalBox
	//       │       ├ Titre
	//       │       └ Sous titre
	//       ├ Spacer
	//       ├ Description
	//       ├ Navigation des etapes : [< Precedent]  Etape 2 / 4  [Suivant >]
	//       └ Bouton d'action ("Lancer l'experience")

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Root->SetBrush(MakeRoundedBrush(BackgroundColor, 32.f));
	Root->SetPadding(FMargin(32.f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Root->SetContent(Column);

	// En-tete : photo + titres
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	Column->AddChildToVerticalBox(Header);

	USizeBox* PhotoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PhotoBox"));
	PhotoBox->SetWidthOverride(PhotoSize.X);
	PhotoBox->SetHeightOverride(PhotoSize.Y);
	PhotoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PhotoImage"));
	PhotoBox->SetContent(PhotoImage);
	if (UHorizontalBoxSlot* PhotoSlot = Header->AddChildToHorizontalBox(PhotoBox))
	{
		PhotoSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
		PhotoSlot->SetVerticalAlignment(VAlign_Top);
	}

	UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Titles"));
	if (UHorizontalBoxSlot* TitlesSlot = Header->AddChildToHorizontalBox(Titles))
	{
		TitlesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitlesSlot->SetVerticalAlignment(VAlign_Center);
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	SetFontSize(TitleText, TitleFontSize);
	TitleText->SetColorAndOpacity(FSlateColor(TextColor));
	TitleText->SetAutoWrapText(true);
	Titles->AddChildToVerticalBox(TitleText);

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	SetFontSize(SubtitleText, SubtitleFontSize);
	SubtitleText->SetColorAndOpacity(FSlateColor(SubtitleColor));
	SubtitleText->SetAutoWrapText(true);
	Titles->AddChildToVerticalBox(SubtitleText);

	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Gap"));
	Gap->SetSize(FVector2D(1.f, 24.f));
	Column->AddChildToVerticalBox(Gap);

	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	SetFontSize(DescriptionText, DescriptionFontSize);
	DescriptionText->SetColorAndOpacity(FSlateColor(TextColor));
	DescriptionText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DescSlot = Column->AddChildToVerticalBox(DescriptionText))
	{
		DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// Navigation des etapes (cachee hors experience)
	UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StepNav"));
	StepNav = Nav;
	PreviousButton = MakeButton(WidgetTree, TEXT("PreviousButton"), FText::FromString(TEXT("< Pr\u00E9c\u00E9dent")), ButtonFontSize, NavButtonColor, TextColor);
	if (UHorizontalBoxSlot* PrevSlot = Nav->AddChildToHorizontalBox(PreviousButton))
	{
		PrevSlot->SetVerticalAlignment(VAlign_Center);
	}
	StepText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StepText"));
	SetFontSize(StepText, ButtonFontSize);
	StepText->SetColorAndOpacity(FSlateColor(SubtitleColor));
	StepText->SetJustification(ETextJustify::Center);
	if (UHorizontalBoxSlot* StepSlot = Nav->AddChildToHorizontalBox(StepText))
	{
		StepSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StepSlot->SetVerticalAlignment(VAlign_Center);
	}
	NextButton = MakeButton(WidgetTree, TEXT("NextButton"), FText::FromString(TEXT("Suivant >")), ButtonFontSize, ButtonColor, TextColor);
	if (UHorizontalBoxSlot* NextSlot = Nav->AddChildToHorizontalBox(NextButton))
	{
		NextSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* NavSlot = Column->AddChildToVerticalBox(Nav))
	{
		NavSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	// Bouton d'action, centre en bas
	ActionButton = MakeButton(WidgetTree, TEXT("ActionButton"), FText::GetEmpty(), ButtonFontSize, ButtonColor, TextColor);
	ActionButtonText = Cast<UTextBlock>(ActionButton->GetContent());
	if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(ActionButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
}
