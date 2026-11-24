#include "UI/ExperienceSelectorWidget.h"
#include "UI/ExperienceEntryWidget.h"
#include "Interaction/DetectionZone.h"
#include "Navigation/NavigatorSystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"

UExperienceSelectorWidget::UExperienceSelectorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EntryClass = TSoftClassPtr<UExperienceEntryWidget>(FSoftObjectPath(TEXT("/Game/Showroom/UI/WBP_ExperienceEntry.WBP_ExperienceEntry_C")));
}

void UExperienceSelectorWidget::BuildDefaultLayout()
{
	//  Border arrondi (fond)
	//   └ VerticalBox
	//       ├ TitleText
	//       ├ ScrollBox "EntryList"  (les lignes sont ajoutees ici)
	//       ├ StatusText
	//       └ StopButton
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Root->SetBrush(MakeRoundedBrush(FLinearColor(0.03f, 0.03f, 0.045f, 0.93f), 40.f));
	Root->SetPadding(FMargin(48.f, 40.f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Root->SetContent(Column);

	TitleText = MakeText(WidgetTree, TEXT("TitleText"), Title, 54, FLinearColor::White);
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
	}

	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("EntryList"));
	List->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	EntryList = List;
	if (UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(List))
	{
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	StatusText = MakeText(WidgetTree, TEXT("StatusText"), IdleStatus, 28, FLinearColor(0.7f, 0.8f, 1.f, 1.f), ETextJustify::Center);
	if (UVerticalBoxSlot* StatusSlot = Column->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 12.f));
	}

	StopButton = MakeButton(WidgetTree, TEXT("StopButton"), FText::FromString(TEXT("Arrêter le guidage")), 28, FLinearColor(0.25f, 0.25f, 0.3f, 1.f));
	if (UVerticalBoxSlot* StopSlot = Column->AddChildToVerticalBox(StopButton))
	{
		StopSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UExperienceSelectorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (StopButton)
	{
		StopButton->OnClicked.RemoveDynamic(this, &UExperienceSelectorWidget::HandleStopClicked);
		StopButton->OnClicked.AddDynamic(this, &UExperienceSelectorWidget::HandleStopClicked);
	}
	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Nav->OnGuideArrived.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleGuideArrived);
		Nav->OnGuideStopped.AddUniqueDynamic(this, &UExperienceSelectorWidget::HandleGuideStopped);
		Selected = Cast<ADetectionZone>(Nav->GetGuideTarget());
	}
	RefreshList();
}

void UExperienceSelectorWidget::NativeDestruct()
{
	if (ANavigatorSystem* Nav = Navigator.Get())
	{
		Nav->OnGuideArrived.RemoveDynamic(this, &UExperienceSelectorWidget::HandleGuideArrived);
		Nav->OnGuideStopped.RemoveDynamic(this, &UExperienceSelectorWidget::HandleGuideStopped);
	}
	Super::NativeDestruct();
}

ANavigatorSystem* UExperienceSelectorWidget::GetNavigator()
{
	if (!Navigator.IsValid() && GetWorld() && GetWorld()->IsGameWorld())
	{
		Navigator = ANavigatorSystem::FindOrSpawn(this);
	}
	return Navigator.Get();
}

void UExperienceSelectorWidget::RefreshList()
{
	if (!EntryList || !GetWorld())
	{
		return;
	}
	EntryList->ClearChildren();
	Entries.Reset();

	// Toutes les zones d'experience a montrer, dans l'ordre choisi.
	TArray<ADetectionZone*> Zones;
	for (TActorIterator<ADetectionZone> It(GetWorld()); It; ++It)
	{
		if (It->bHasExperience && It->bShowInSelector)
		{
			Zones.Add(*It);
		}
	}
	Zones.Sort([](const ADetectionZone& A, const ADetectionZone& B)
	{
		return A.SelectorOrder != B.SelectorOrder ? A.SelectorOrder < B.SelectorOrder
			: A.Info.Title.ToString() < B.Info.Title.ToString();
	});

	TSubclassOf<UExperienceEntryWidget> Class = EntryClass.LoadSynchronous();
	if (!Class)
	{
		Class = UExperienceEntryWidget::StaticClass();
	}

	for (ADetectionZone* Zone : Zones)
	{
		UExperienceEntryWidget* Entry = CreateWidget<UExperienceEntryWidget>(this, Class);
		if (!Entry)
		{
			continue;
		}
		Entry->SetZone(Zone);
		Entry->OnSelected.AddUObject(this, &UExperienceSelectorWidget::HandleEntrySelected);
		UPanelSlot* EntrySlot = EntryList->AddChild(Entry);
		if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(EntrySlot))
		{
			ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		}
		else if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(EntrySlot))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		}
		Entries.Add(Entry);
	}

	if (Zones.Num() == 0)
	{
		SetStatus(FText::FromString(TEXT("Aucune expérience dans ce niveau.")));
	}
	else if (const ADetectionZone* Zone = Selected.Get())
	{
		SetStatus(FText::Format(GuidingStatus, Zone->Info.Title));
	}
	else
	{
		SetStatus(IdleStatus);
	}
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleEntrySelected(ADetectionZone* Zone)
{
	SelectExperience(Zone);
}

void UExperienceSelectorWidget::SelectExperience(ADetectionZone* Zone)
{
	if (!Zone)
	{
		return;
	}
	if (Zone->IsPlayerInside())
	{
		SetStatus(ArrivedStatus);
		return;
	}
	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Selected = Zone;
		Nav->GuideTo(Zone);
		SetStatus(FText::Format(GuidingStatus, Zone->Info.Title));
		UpdateSelection();
	}
}

void UExperienceSelectorWidget::StopGuiding()
{
	if (ANavigatorSystem* Nav = GetNavigator())
	{
		Nav->StopGuideTo();
	}
	Selected = nullptr;
	SetStatus(IdleStatus);
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleStopClicked()
{
	StopGuiding();
}

void UExperienceSelectorWidget::HandleGuideArrived(AActor* /*Target*/)
{
	Selected = nullptr;
	SetStatus(ArrivedStatus);
	UpdateSelection();
}

void UExperienceSelectorWidget::HandleGuideStopped(AActor* Target)
{
	// Une autre destination a pu etre choisie ailleurs : on se cale sur le GPS.
	if (const ANavigatorSystem* Nav = Navigator.Get())
	{
		Selected = Cast<ADetectionZone>(Nav->GetGuideTarget());
	}
	UpdateSelection();
}

void UExperienceSelectorWidget::UpdateSelection()
{
	for (UExperienceEntryWidget* Entry : Entries)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->GetZone() && Entry->GetZone() == Selected.Get());
		}
	}
	if (StopButton)
	{
		StopButton->SetVisibility(Selected.IsValid() ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UExperienceSelectorWidget::SetStatus(const FText& Text)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
	}
}
