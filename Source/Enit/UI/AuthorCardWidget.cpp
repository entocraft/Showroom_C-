#include "UI/AuthorCardWidget.h"
#include "UI/ShowroomStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	/** "Juliette Martin" -> "JM". */
	FText MakeInitials(const FText& Name)
	{
		TArray<FString> Words;
		Name.ToString().ParseIntoArrayWS(Words);
		FString Initials;
		for (const FString& Word : Words)
		{
			if (!Word.IsEmpty() && Initials.Len() < 2)
			{
				Initials.AppendChar(FChar::ToUpper(Word[0]));
			}
		}
		return FText::FromString(Initials);
	}
}

void UAuthorCardWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  Border "Card"
	//   └ HorizontalBox
	//       ├ SizeBox > Overlay : fond + InitialsText, PhotoImage
	//       └ VerticalBox : NameText, RoleText, BioText
	UBorder* Card = MakeBox(WidgetTree, TEXT("Card"), Surface(), RadiusCard, FMargin(14.f));
	WidgetTree->RootWidget = Card;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Card->SetContent(Row);

	USizeBox* PhotoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PhotoBox"));
	PhotoBox->SetWidthOverride(PhotoSize);
	PhotoBox->SetHeightOverride(PhotoSize);
	if (UHorizontalBoxSlot* PhotoSlot = Row->AddChildToHorizontalBox(PhotoBox))
	{
		PhotoSlot->SetVerticalAlignment(VAlign_Top);
		PhotoSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}
	UOverlay* PhotoStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PhotoStack"));
	PhotoBox->SetContent(PhotoStack);

	UBorder* InitialsBack = MakeBox(WidgetTree, TEXT("InitialsBack"), AccentDeep(), RadiusSmall, FMargin(0.f));
	InitialsBack->SetHorizontalAlignment(HAlign_Center);
	InitialsBack->SetVerticalAlignment(VAlign_Center);
	InitialsText = ShowroomStyle::MakeText(WidgetTree, TEXT("InitialsText"), FText::FromString(TEXT("PN")), 34, TextMain(), TEXT("Bold"), false);
	InitialsBack->SetContent(InitialsText);
	if (UOverlaySlot* BackSlot = PhotoStack->AddChildToOverlay(InitialsBack))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	PhotoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PhotoImage"));
	if (UOverlaySlot* ImageSlot = PhotoStack->AddChildToOverlay(PhotoImage))
	{
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	if (UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts))
	{
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
	}
	NameText = ShowroomStyle::MakeText(WidgetTree, TEXT("NameText"), FText::FromString(TEXT("Prénom Nom")), 26, TextMain(), TEXT("Bold"));
	Texts->AddChildToVerticalBox(NameText);
	RoleText = ShowroomStyle::MakeText(WidgetTree, TEXT("RoleText"), FText::FromString(TEXT("MÉTIER")), 19, AccentText(), TEXT("Bold"));
	if (UVerticalBoxSlot* RoleSlot = Texts->AddChildToVerticalBox(RoleText))
	{
		RoleSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
	}
	BioText = ShowroomStyle::MakeText(WidgetTree, TEXT("BioText"), FText::FromString(TEXT("Son rôle dans l'expérience.")), 21, TextMuted());
	Texts->AddChildToVerticalBox(BioText);
}

void UAuthorCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UAuthorCardWidget::SetAuthor(const FShowroomAuthor& InAuthor)
{
	Author = InAuthor;
	Refresh();
}

void UAuthorCardWidget::Refresh()
{
	auto SetOrHide = [](UTextBlock* Block, const FText& Value)
	{
		if (Block)
		{
			Block->SetText(Value);
			Block->SetVisibility(Value.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	};
	SetOrHide(NameText, Author.Name);
	SetOrHide(RoleText, Author.Role.ToUpper());
	SetOrHide(BioText, Author.Bio);

	if (InitialsText)
	{
		InitialsText->SetText(MakeInitials(Author.Name));
	}
	if (PhotoImage)
	{
		if (Author.Photo)
		{
			PhotoImage->SetBrush(ShowroomStyle::RoundedImageBrush(Author.Photo, ShowroomStyle::RadiusSmall, FVector2D(PhotoSize)));
			PhotoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PhotoImage->SetVisibility(ESlateVisibility::Collapsed); // on voit les initiales
		}
	}
}
