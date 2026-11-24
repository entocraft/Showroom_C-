#include "UI/InfoPanelWidget.h"
#include "UI/ShowroomStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

UInfoPanelWidget::UInfoPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Charte de la maquette Figma (voir UI/ShowroomStyle.h).
	BackgroundColor = ShowroomStyle::Background();
	TextColor = ShowroomStyle::TextMain();
	SubtitleColor = ShowroomStyle::TextMuted();
	ButtonColor = ShowroomStyle::Accent();
	NavButtonColor = ShowroomStyle::Surface();
	TitleFontSize = 42;
	SubtitleFontSize = 22;
	DescriptionFontSize = 21;
	ButtonFontSize = 26;
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
	if (TitleText)       TitleText->SetText(Info.Title.ToUpper());
	if (SubtitleText)    SubtitleText->SetText(Info.Subtitle);
	if (DescriptionText) DescriptionText->SetText(Info.Description);

	// Pastille et ligne "Domaine • Duree • Lieu" : cachees si vides.
	if (TagText)
	{
		TagText->SetText(Info.Tag.ToUpper());
	}
	if (UWidget* TagTarget = TagBox ? TagBox.Get() : static_cast<UWidget*>(TagText.Get()))
	{
		TagTarget->SetVisibility(Info.Tag.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (MetaText)
	{
		TArray<FString> Parts;
		for (const FText* Part : { &Info.Domain, &Info.Duration, &Info.Location })
		{
			if (!Part->IsEmpty())
			{
				Parts.Add(Part->ToString());
			}
		}
		MetaText->SetText(FText::FromString(FString::Join(Parts, TEXT("   •   "))));
		MetaText->SetVisibility(Parts.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

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
		// Sans photo, on cache aussi son cadre (le texte prend toute la largeur).
		UWidget* Frame = Cast<USizeBox>(PhotoImage->GetParent()) ? static_cast<UWidget*>(PhotoImage->GetParent()) : static_cast<UWidget*>(PhotoImage.Get());
		if (Info.Photo)
		{
			PhotoImage->SetBrush(ShowroomStyle::RoundedImageBrush(Info.Photo, ShowroomStyle::RadiusCard, PhotoSize));
			Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Frame->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UInfoPanelWidget::BuildDefaultLayout()
{
	using namespace ShowroomStyle;

	//  Border "Background"
	//   └ HorizontalBox
	//       ├ SizeBox "PhotoBox" > PhotoImage (coins arrondis)
	//       └ VerticalBox "Texts"
	//           ├ TagBox > TagText
	//           ├ TitleText, SubtitleText, MetaText
	//           ├ Border "DescriptionBox" > DescriptionText
	//           ├ StepNav : [< Precedent]  Etape 2 / 4  [Suivant >]
	//           └ ActionButton ("Lancer l'experience")
	UBorder* Root = MakeBox(WidgetTree, TEXT("Background"), BackgroundColor, RadiusPanel, FMargin(26.f));
	WidgetTree->RootWidget = Root;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Root->SetContent(Row);

	USizeBox* PhotoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PhotoBox"));
	PhotoBox->SetWidthOverride(PhotoSize.X);
	PhotoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PhotoImage"));
	PhotoImage->SetBrush(RoundedImageBrush(nullptr, RadiusCard, PhotoSize));
	PhotoBox->SetContent(PhotoImage);
	if (UHorizontalBoxSlot* PhotoSlot = Row->AddChildToHorizontalBox(PhotoBox))
	{
		PhotoSlot->SetPadding(FMargin(0.f, 0.f, 26.f, 0.f));
		PhotoSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	if (UHorizontalBoxSlot* ColumnSlot = Row->AddChildToHorizontalBox(Column))
	{
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* TagRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TagRow"));
	if (UVerticalBoxSlot* TagSlot = Column->AddChildToVerticalBox(TagRow))
	{
		TagSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	UBorder* Tag = MakeBox(WidgetTree, TEXT("TagBox"), ShowroomStyle::Tag(), 999.f, FMargin(14.f, 5.f));
	TagText = ShowroomStyle::MakeText(WidgetTree, TEXT("TagText"), FText::FromString(TEXT("EXPÉRIENCE")), 16, TextColor, TEXT("Bold"), false);
	Tag->SetContent(TagText);
	TagBox = Tag;
	TagRow->AddChildToHorizontalBox(Tag);

	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("TITRE")), TitleFontSize, TextColor, TEXT("Bold"));
	Column->AddChildToVerticalBox(TitleText);
	SubtitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("SubtitleText"), FText::FromString(TEXT("Sous-titre")), SubtitleFontSize, SubtitleColor);
	Column->AddChildToVerticalBox(SubtitleText);
	MetaText = ShowroomStyle::MakeText(WidgetTree, TEXT("MetaText"), FText::FromString(TEXT("Domaine  •  Durée  •  Lieu")), 18, AccentText(), TEXT("Bold"));
	if (UVerticalBoxSlot* MetaSlot = Column->AddChildToVerticalBox(MetaText))
	{
		MetaSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	UBorder* DescriptionBox = MakeBox(WidgetTree, TEXT("DescriptionBox"), SurfaceDeep(), RadiusCard, FMargin(20.f, 16.f));
	DescriptionBox->SetVerticalAlignment(VAlign_Top);
	DescriptionText = ShowroomStyle::MakeText(WidgetTree, TEXT("DescriptionText"), FText::FromString(TEXT("Description")), DescriptionFontSize, TextColor);
	DescriptionBox->SetContent(DescriptionText);
	if (UVerticalBoxSlot* DescSlot = Column->AddChildToVerticalBox(DescriptionBox))
	{
		DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DescSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	// Navigation des etapes (cachee hors experience)
	UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StepNav"));
	StepNav = Nav;
	PreviousButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("PreviousButton"), NavButtonColor, SurfaceHover(), 999.f, FMargin(22.f, 10.f));
	PreviousButton->SetContent(ShowroomStyle::MakeText(WidgetTree, TEXT("PreviousButtonText"), FText::FromString(TEXT("‹  Précédent")), ButtonFontSize - 4, TextColor, TEXT("Bold"), false));
	if (UHorizontalBoxSlot* PrevSlot = Nav->AddChildToHorizontalBox(PreviousButton))
	{
		PrevSlot->SetVerticalAlignment(VAlign_Center);
	}
	StepText = ShowroomStyle::MakeText(WidgetTree, TEXT("StepText"), FText::FromString(TEXT("Étape 1 / 4")), ButtonFontSize - 4, SubtitleColor, TEXT("Bold"), false);
	StepText->SetJustification(ETextJustify::Center);
	if (UHorizontalBoxSlot* StepSlot = Nav->AddChildToHorizontalBox(StepText))
	{
		StepSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StepSlot->SetVerticalAlignment(VAlign_Center);
	}
	NextButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("NextButton"), ButtonColor, AccentHover(), 999.f, FMargin(22.f, 10.f));
	NextButton->SetContent(ShowroomStyle::MakeText(WidgetTree, TEXT("NextButtonText"), FText::FromString(TEXT("Suivant  ›")), ButtonFontSize - 4, TextColor, TEXT("Bold"), false));
	if (UHorizontalBoxSlot* NextSlot = Nav->AddChildToHorizontalBox(NextButton))
	{
		NextSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* NavSlot = Column->AddChildToVerticalBox(Nav))
	{
		NavSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// Bouton d'action : toute la largeur, comme "Lancer le GPS" de la maquette
	ActionButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("ActionButton"), ButtonColor, AccentHover(), RadiusCard, FMargin(20.f, 14.f));
	ActionButtonText = ShowroomStyle::MakeText(WidgetTree, TEXT("ActionButtonText"), FText::FromString(TEXT("Lancer l'expérience")), ButtonFontSize, TextColor, TEXT("Bold"), false);
	ActionButtonText->SetJustification(ETextJustify::Center);
	ActionButton->SetContent(ActionButtonText);
	if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(ActionButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
}
