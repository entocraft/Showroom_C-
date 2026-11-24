#include "UI/SimpleScreenWidget.h"
#include "UI/FloatingScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void USimpleScreenWidget::BuildDefaultLayout()
{
	//  Border arrondi
	//   └ VerticalBox
	//       ├ TitleText
	//       ├ HorizontalBox : MainImage (cachee s'il n'y a pas d'image) + ScrollBox > BodyText
	//       └ CloseButton
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Root->SetBrush(MakeRoundedBrush(FLinearColor(0.03f, 0.03f, 0.045f, 0.93f), 40.f));
	Root->SetPadding(FMargin(56.f, 44.f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Root->SetContent(Column);

	TitleText = MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Titre de l'\u00E9cran")), 56, FLinearColor::White);
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));
	if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
	{
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ImageBox"));
	ImageBox->SetWidthOverride(420.f);
	ImageBox->SetMaxDesiredHeight(420.f);
	MainImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainImage"));
	ImageBox->SetContent(MainImage);
	ImageBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UHorizontalBoxSlot* ImageSlot = Row->AddChildToHorizontalBox(ImageBox))
	{
		ImageSlot->SetPadding(FMargin(0.f, 0.f, 32.f, 0.f));
		ImageSlot->SetVerticalAlignment(VAlign_Top);
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BodyScroll"));
	if (UHorizontalBoxSlot* ScrollSlot = Row->AddChildToHorizontalBox(Scroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	BodyText = MakeText(WidgetTree, TEXT("BodyText"),
		FText::FromString(TEXT("Modifie cet \u00E9cran dans l'\u00E9diteur UMG : textes, images, boutons...\nDuplique-le pour cr\u00E9er d'autres \u00E9crans.")),
		30, FLinearColor(0.85f, 0.88f, 0.95f, 1.f));
	Scroll->AddChild(BodyText);

	CloseButton = MakeButton(WidgetTree, TEXT("CloseButton"), FText::FromString(TEXT("Fermer")), 30, FLinearColor(0.1f, 0.45f, 0.9f, 1.f));
	if (UVerticalBoxSlot* CloseSlot = Column->AddChildToVerticalBox(CloseButton))
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
		CloseSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
	}
}

void USimpleScreenWidget::SetScreenContent(const FText& Title, const FText& Body, UTexture2D* Image)
{
	PendingTitle = Title;
	PendingBody = Body;
	PendingImage = Image;
	bHasContent = true;
	ApplyContent();
}

void USimpleScreenWidget::ApplyContent()
{
	if (!bHasContent)
	{
		return;
	}
	const FText& Title = PendingTitle;
	const FText& Body = PendingBody;
	UTexture2D* Image = PendingImage;
	if (TitleText && !Title.IsEmpty())
	{
		TitleText->SetText(Title);
	}
	if (BodyText && !Body.IsEmpty())
	{
		BodyText->SetText(Body);
	}
	if (MainImage)
	{
		UWidget* Frame = MainImage->GetParent() && MainImage->GetParent()->IsA<USizeBox>() ? static_cast<UWidget*>(MainImage->GetParent()) : static_cast<UWidget*>(MainImage);
		if (Image)
		{
			MainImage->SetBrushFromTexture(Image, /*bMatchSize*/ false);
			Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Frame->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void USimpleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &USimpleScreenWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &USimpleScreenWidget::HandleCloseClicked);
	}
	ApplyContent();
}

void USimpleScreenWidget::HandleCloseClicked()
{
	if (AFloatingScreen* Screen = Cast<AFloatingScreen>(GetOwnerActor()))
	{
		Screen->HideScreen();
	}
}
