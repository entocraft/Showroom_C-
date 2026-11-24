#include "UI/ExperienceLabelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* MakeLabelText(UWidgetTree* Tree, FName Name, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(true);
		return Text;
	}
}

void UExperienceLabelWidget::BuildDefaultLayout()
{
	{
		//  Border (fond)
		//   └ VerticalBox
		//       ├ Nom de l'objet (couleur d'accent)
		//       └ Petit texte
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
		Root->SetBrush(MakeRoundedBrush(BackgroundColor, 24.f));
		Root->SetPadding(FMargin(28.f, 16.f));
		Root->SetHorizontalAlignment(HAlign_Fill);
		Root->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Root;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		Root->SetContent(Column);

		LabelText = MakeLabelText(WidgetTree, TEXT("LabelText"), LabelFontSize, AccentColor);
		Column->AddChildToVerticalBox(LabelText);

		DetailsText = MakeLabelText(WidgetTree, TEXT("DetailsText"), DetailsFontSize, FLinearColor::White);
		if (UVerticalBoxSlot* DetailsSlot = Column->AddChildToVerticalBox(DetailsText))
		{
			DetailsSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
	}
}

void UExperienceLabelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UExperienceLabelWidget::SetTexts(const FText& InLabel, const FText& InDetails)
{
	Label = InLabel;
	Details = InDetails;
	Refresh();
}

void UExperienceLabelWidget::Refresh()
{
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
	if (DetailsText)
	{
		DetailsText->SetText(Details);
		DetailsText->SetVisibility(Details.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}
