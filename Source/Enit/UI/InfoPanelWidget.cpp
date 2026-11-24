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
#include "Components/PanelWidget.h"
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
	TitleFontSize = 46;
	SubtitleFontSize = 27;
	DescriptionFontSize = 27;
	ButtonFontSize = 29;
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
	AnimateHover(ActionButton, ActionButton, 1.03f, -4.f);
	AnimateHover(NextButton, NextButton, 1.08f, 0.f);
	AnimateHover(PreviousButton, PreviousButton, 1.08f, 0.f);
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
	const bool bChanged = !Info.Title.EqualTo(NewInfo.Title) || !Info.Description.EqualTo(NewInfo.Description);
	Info = NewInfo;
	Refresh();
	if (bChanged && IsConstructed())
	{
		AnimateIn(ContentColumn, 0.f, FVector2D(30.f, 0.f), 0.3f); // nouvelle etape : le texte glisse
	}
}

void UInfoPanelWidget::PlayIntro()
{
	Super::PlayIntro();
	AnimateIn(ActionButton, 0.15f, FVector2D(0.f, 20.f), 0.3f);
}

void UInfoPanelWidget::SetActionIsStop(bool bStop)
{
	bActionIsStop = bStop;
	Refresh();
}

void UInfoPanelWidget::SetStepProgress(int32 Current, int32 Count)
{
	ProgressCurrent = Current;
	ProgressCount = Count;
	if (!StepProgress || !WidgetTree)
	{
		return;
	}
	StepProgress->ClearChildren();
	StepProgress->SetVisibility(Count > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (int32 i = 0; i < Count; ++i)
	{
		// Segment : fait (bleu clair), en cours (blanc, plus large), a venir (sombre).
		USizeBox* SegBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SegBox->SetHeightOverride(10.f);
		const FLinearColor Color = i < Current ? ShowroomStyle::AccentText() : i == Current ? ShowroomStyle::TextMain() : ShowroomStyle::Outline();
		SegBox->SetContent(ShowroomStyle::MakeBox(WidgetTree, NAME_None, Color, 5.f, FMargin(0.f)));
		if (UHorizontalBoxSlot* SegSlot = Cast<UHorizontalBoxSlot>(StepProgress->AddChild(SegBox)))
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = i == Current ? 2.f : 1.f;
			SegSlot->SetSize(Size);
			SegSlot->SetPadding(FMargin(i > 0 ? 4.f : 0.f, 0.f, 0.f, 0.f));
			SegSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
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
	if (ActionButtonIcon)
	{
		ShowroomStyle::SetIcon(ActionButtonIcon, bActionIsStop ? TEXT("stop") : TEXT("play"));
	}
	// "Lancer l'experience" respire doucement pour attirer l'oeil ; pas "Terminer".
	if (bActionVisible && !bActionIsStop)
	{
		AnimatePulse(ActionButton, 1.03f, 0.6f);
	}
	else
	{
		StopPulse(ActionButton);
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
	ContentColumn = Column;
	if (UHorizontalBoxSlot* ColumnSlot = Row->AddChildToHorizontalBox(Column))
	{
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* TagRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TagRow"));
	if (UVerticalBoxSlot* TagSlot = Column->AddChildToVerticalBox(TagRow))
	{
		TagSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	UBorder* Tag = MakeBox(WidgetTree, TEXT("TagBox"), ShowroomStyle::Tag(), 999.f, FMargin(12.f, 5.f, 16.f, 5.f));
	UTextBlock* TagLabel = nullptr;
	Tag->SetContent(MakeIconLabel(WidgetTree, TEXT("TagLabel"), TEXT("sparkle"), FText::FromString(TEXT("EXPÉRIENCE")), 19, TextColor, TEXT("Bold"), TagLabel, 8.f));
	TagText = TagLabel;
	TagBox = Tag;
	TagRow->AddChildToHorizontalBox(Tag);

	TitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("TITRE")), TitleFontSize, TextColor, TEXT("Bold"));
	Column->AddChildToVerticalBox(TitleText);
	SubtitleText = ShowroomStyle::MakeText(WidgetTree, TEXT("SubtitleText"), FText::FromString(TEXT("Sous-titre")), SubtitleFontSize, SubtitleColor);
	Column->AddChildToVerticalBox(SubtitleText);
	MetaText = ShowroomStyle::MakeText(WidgetTree, TEXT("MetaText"), FText::FromString(TEXT("Domaine  •  Durée  •  Lieu")), 21, AccentText(), TEXT("Bold"));
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
	UVerticalBox* NavBlock = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StepNav"));
	StepNav = NavBlock;
	UHorizontalBox* Progress = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StepProgress"));
	StepProgress = Progress;
	if (UVerticalBoxSlot* ProgressSlot = NavBlock->AddChildToVerticalBox(Progress))
	{
		ProgressSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}
	UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StepNavRow"));
	NavBlock->AddChildToVerticalBox(Nav);
	PreviousButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("PreviousButton"), NavButtonColor, SurfaceHover(), 999.f, FMargin(16.f));
	PreviousButton->SetContent(MakeIcon(WidgetTree, TEXT("PreviousButtonIcon"), TEXT("chevron_left"), 34.f, TextColor));
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
	NextButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("NextButton"), ButtonColor, AccentHover(), 999.f, FMargin(22.f, 12.f, 18.f, 12.f));
	{
		UHorizontalBox* NextRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NextButtonRow"));
		if (UHorizontalBoxSlot* NextTextSlot = NextRow->AddChildToHorizontalBox(ShowroomStyle::MakeText(WidgetTree, TEXT("NextButtonText"), FText::FromString(TEXT("Suivant")), ButtonFontSize - 3, TextColor, TEXT("Bold"), false)))
		{
			NextTextSlot->SetVerticalAlignment(VAlign_Center);
			NextTextSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}
		if (UHorizontalBoxSlot* NextIconSlot = NextRow->AddChildToHorizontalBox(MakeIcon(WidgetTree, TEXT("NextButtonIcon"), TEXT("chevron_right"), 28.f, TextColor)))
		{
			NextIconSlot->SetVerticalAlignment(VAlign_Center);
		}
		NextButton->SetContent(NextRow);
	}
	if (UHorizontalBoxSlot* NextSlot = Nav->AddChildToHorizontalBox(NextButton))
	{
		NextSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* NavSlot = Column->AddChildToVerticalBox(NavBlock))
	{
		NavSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// Bouton d'action : toute la largeur, comme "Lancer le GPS" de la maquette
	ActionButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("ActionButton"), ButtonColor, AccentHover(), RadiusCard, FMargin(20.f, 14.f));
	{
		UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
		ActionButtonIcon = MakeIcon(WidgetTree, TEXT("ActionButtonIcon"), TEXT("play"), 32.f, TextColor);
		if (UHorizontalBoxSlot* ActionIconSlot = ActionRow->AddChildToHorizontalBox(ActionButtonIcon))
		{
			ActionIconSlot->SetVerticalAlignment(VAlign_Center);
			ActionIconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}
		ActionButtonText = ShowroomStyle::MakeText(WidgetTree, TEXT("ActionButtonText"), FText::FromString(TEXT("Lancer l'expérience")), ButtonFontSize, TextColor, TEXT("Bold"), false);
		if (UHorizontalBoxSlot* ActionTextSlot = ActionRow->AddChildToHorizontalBox(ActionButtonText))
		{
			ActionTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		ActionButton->SetContent(ActionRow);
		ActionButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(ActionButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill); // contenu centre par le bouton
		ButtonSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}
}
