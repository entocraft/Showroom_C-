#include "UI/HandMenuWidget.h"
#include "UI/ShowroomStyle.h"
#include "Core/ShowroomSessions.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	using namespace ShowroomStyle;

	/** Gros bouton de menu : icone + libelle, aligne a gauche. */
	UButton* MakeMenuButton(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* IconName, const TCHAR* Label, const FLinearColor& Color, const FLinearColor& Hover)
	{
		UButton* Button = ShowroomStyle::MakeButton(Tree, Name, Color, Hover, RadiusCard, FMargin(22.f, 16.f));
		UTextBlock* Text = nullptr;
		UHorizontalBox* Row = MakeIconLabel(Tree, *(FString(Name) + TEXT("Label")), IconName, FText::FromString(Label), 30, TextMain(), TEXT("Bold"), Text, 16.f);
		Button->SetContent(Row);
		if (UButtonSlot* RowSlot = Cast<UButtonSlot>(Row->Slot))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Left);
		}
		return Button;
	}

	/** Titre de page : icone dans une pastille + texte. */
	UHorizontalBox* MakeHeader(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* IconName, const TCHAR* Title)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
		UBorder* Badge = MakeBox(Tree, NAME_None, AccentDeep(), 999.f, FMargin(12.f));
		Badge->SetContent(MakeIcon(Tree, NAME_None, IconName, 34.f, AccentText()));
		if (UHorizontalBoxSlot* BadgeSlot = Row->AddChildToHorizontalBox(Badge))
		{
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
			BadgeSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		}
		if (UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(ShowroomStyle::MakeText(Tree, NAME_None, FText::FromString(Title), 40, TextMain(), TEXT("Bold"), false)))
		{
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Row;
	}

	void AddSpaced(UVerticalBox* Column, UWidget* Widget, float Top)
	{
		if (UVerticalBoxSlot* WidgetSlot = Column->AddChildToVerticalBox(Widget))
		{
			WidgetSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		}
	}
}

void UHandMenuWidget::BuildDefaultLayout()
{
	//  Border arrondi
	//   └ WidgetSwitcher "Pages"
	//       0 Menu     : titre, etat multijoueur, Sessions, Recommencer, Quitter
	//       1 Sessions : retour + titre, etat, Heberger / Rechercher, liste des sessions, Quitter la session
	UBorder* Root = MakeBox(WidgetTree, TEXT("Background"), Background(), RadiusPanel, FMargin(26.f));
	WidgetTree->RootWidget = Root;
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	Root->SetContent(Pages);

	// ---- Page Menu ----
	UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainPage"));
	Pages->AddChild(Main);
	Main->AddChildToVerticalBox(MakeHeader(WidgetTree, TEXT("MainHeader"), TEXT("grid"), TEXT("Menu")));

	UBorder* Chip = MakeBox(WidgetTree, TEXT("SessionChip"), SurfaceDeep(), RadiusCard, FMargin(16.f, 12.f));
	UTextBlock* Summary = nullptr;
	UHorizontalBox* ChipRow = MakeIconLabel(WidgetTree, TEXT("SessionChipRow"), TEXT("user"), FText::FromString(TEXT("Hors ligne")), 22, TextMuted(), TEXT("Regular"), Summary, 12.f);
	Chip->SetContent(ChipRow);
	SessionSummary = Summary;
	SessionSummary->SetAutoWrapText(true);
	if (UHorizontalBoxSlot* SummarySlot = Cast<UHorizontalBoxSlot>(SessionSummary->Slot))
	{
		SummarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	AddSpaced(Main, Chip, 18.f);

	SessionsButton = MakeMenuButton(WidgetTree, TEXT("SessionsButton"), TEXT("user"), TEXT("Sessions multijoueur"), Accent(), AccentHover());
	RestartButton = MakeMenuButton(WidgetTree, TEXT("RestartButton"), TEXT("refresh"), TEXT("Recommencer"), Surface(), SurfaceHover());
	QuitButton = MakeMenuButton(WidgetTree, TEXT("QuitButton"), TEXT("power"), TEXT("Quitter"), Surface(), SurfaceHover());
	AddSpaced(Main, SessionsButton, 22.f);
	AddSpaced(Main, RestartButton, 12.f);
	AddSpaced(Main, QuitButton, 12.f);

	// ---- Page Sessions ----
	UVerticalBox* SessionsPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SessionsPage"));
	Pages->AddChild(SessionsPage);

	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SessionsTop"));
	BackButton = ShowroomStyle::MakeButton(WidgetTree, TEXT("BackButton"), Surface(), SurfaceHover(), 999.f, FMargin(14.f));
	BackButton->SetContent(MakeIcon(WidgetTree, TEXT("BackButtonIcon"), TEXT("arrow_left"), 30.f, TextMain()));
	if (UHorizontalBoxSlot* BackSlot = Top->AddChildToHorizontalBox(BackButton))
	{
		BackSlot->SetVerticalAlignment(VAlign_Center);
		BackSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}
	if (UHorizontalBoxSlot* HeaderSlot = Top->AddChildToHorizontalBox(MakeHeader(WidgetTree, TEXT("SessionsHeader"), TEXT("user"), TEXT("Sessions"))))
	{
		HeaderSlot->SetVerticalAlignment(VAlign_Center);
	}
	SessionsPage->AddChildToVerticalBox(Top);

	SessionStatus = ShowroomStyle::MakeText(WidgetTree, TEXT("SessionStatus"), FText::FromString(TEXT("Hors ligne")), 22, TextMuted(), TEXT("Regular"), true);
	AddSpaced(SessionsPage, SessionStatus, 16.f);

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SessionActions"));
	HostButton = MakeMenuButton(WidgetTree, TEXT("HostButton"), TEXT("home"), TEXT("Héberger"), Accent(), AccentHover());
	SearchButton = MakeMenuButton(WidgetTree, TEXT("SearchButton"), TEXT("refresh"), TEXT("Rechercher"), Surface(), SurfaceHover());
	for (UButton* Button : { HostButton.Get(), SearchButton.Get() })
	{
		if (UHorizontalBoxSlot* ActionSlot = Actions->AddChildToHorizontalBox(Button))
		{
			ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ActionSlot->SetPadding(FMargin(Button == HostButton ? 0.f : 10.f, 0.f, 0.f, 0.f));
		}
	}
	AddSpaced(SessionsPage, Actions, 16.f);

	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SessionList"));
	SessionList = List;
	if (UVerticalBoxSlot* ListSlot = SessionsPage->AddChildToVerticalBox(List))
	{
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	LeaveButton = MakeMenuButton(WidgetTree, TEXT("LeaveButton"), TEXT("close"), TEXT("Quitter la session"), Hex(TEXT("7A2E35")), Hex(TEXT("9C3A43")));
	AddSpaced(SessionsPage, LeaveButton, 12.f);
}

// ============================================================================

UShowroomSessions* UHandMenuWidget::GetSessions() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UShowroomSessions>() : nullptr;
}

void UHandMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)  { RestartButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnRestartClicked); AnimateHover(RestartButton, RestartButton, 1.04f, 0.f); }
	if (QuitButton)     { QuitButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnQuitClicked); AnimateHover(QuitButton, QuitButton, 1.04f, 0.f); }
	if (SessionsButton) { SessionsButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnSessionsClicked); AnimateHover(SessionsButton, SessionsButton, 1.04f, 0.f); }
	if (BackButton)     { BackButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnBackClicked); AnimateHover(BackButton, BackButton, 1.1f, 0.f); }
	if (HostButton)     { HostButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnHostClicked); AnimateHover(HostButton, HostButton, 1.04f, 0.f); }
	if (SearchButton)   { SearchButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnSearchClicked); AnimateHover(SearchButton, SearchButton, 1.04f, 0.f); }
	if (LeaveButton)    { LeaveButton->OnClicked.AddUniqueDynamic(this, &UHandMenuWidget::OnLeaveClicked); AnimateHover(LeaveButton, LeaveButton, 1.04f, 0.f); }

	if (UShowroomSessions* Sessions = GetSessions())
	{
		ChangedHandle = Sessions->OnChanged.AddUObject(this, &UHandMenuWidget::RefreshSessions);
	}
	ShowPage(0);
	RefreshSessions();
	PlayIntro();
}

void UHandMenuWidget::NativeDestruct()
{
	if (UShowroomSessions* Sessions = GetSessions())
	{
		Sessions->OnChanged.Remove(ChangedHandle);
	}
	Super::NativeDestruct();
}

void UHandMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Le nombre de joueurs change sans prevenir : on rafraichit l'etat de temps en temps.
	RefreshTimer += InDeltaTime;
	if (RefreshTimer > 1.f)
	{
		RefreshTimer = 0.f;
		const UShowroomSessions* Sessions = GetSessions();
		const FText Status = Sessions ? Sessions->GetStatusText() : FText::FromString(TEXT("Multijoueur indisponible."));
		if (SessionSummary) SessionSummary->SetText(Status);
		if (SessionStatus)  SessionStatus->SetText(Status);
	}

	// Une session de la liste a ete cliquee ?
	for (const TPair<TWeakObjectPtr<UButton>, int32>& Pair : JoinButtons)
	{
		if (const UButton* Button = Pair.Key.Get(); Button && Button->IsPressed())
		{
			JoinFound(Pair.Value);
			break;
		}
	}
}

void UHandMenuWidget::ShowPage(int32 Index)
{
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Index);
		AnimateIn(Pages->GetActiveWidget(), 0.f, FVector2D(Index == 0 ? -30.f : 30.f, 0.f), 0.25f);
	}
}

void UHandMenuWidget::RefreshSessions()
{
	UShowroomSessions* Sessions = GetSessions();
	const FText Status = Sessions ? Sessions->GetStatusText() : FText::FromString(TEXT("Multijoueur indisponible."));
	if (SessionSummary) SessionSummary->SetText(Status);
	if (SessionStatus)  SessionStatus->SetText(Status);

	const bool bInSession = Sessions && Sessions->IsInSession();
	const bool bBusy = !Sessions || Sessions->IsBusy();
	const UWorld* World = GetWorld();
	const bool bClient = World && World->GetNetMode() == NM_Client;

	if (LeaveButton)   LeaveButton->SetVisibility(bInSession ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (HostButton)    HostButton->SetIsEnabled(!bBusy && !(Sessions && Sessions->IsHosting()));
	if (SearchButton)  SearchButton->SetIsEnabled(!bBusy);
	// Un joueur connecte a quelqu'un ne relance pas le niveau (seul l'hote peut).
	if (RestartButton) RestartButton->SetVisibility(bClient ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	// La liste des sessions trouvees : un bouton par session.
	if (SessionList && WidgetTree)
	{
		SessionList->ClearChildren();
		JoinButtons.Reset();
		if (Sessions)
		{
			for (const FShowroomSessionInfo& Info : Sessions->GetFoundSessions())
			{
				UButton* Button = ShowroomStyle::MakeButton(WidgetTree, NAME_None, ShowroomStyle::Surface(), ShowroomStyle::SurfaceHover(), ShowroomStyle::RadiusCard, FMargin(18.f, 12.f));
				UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(ShowroomStyle::MakeIcon(WidgetTree, NAME_None, TEXT("user"), 34.f, ShowroomStyle::AccentText())))
				{
					IconSlot->SetVerticalAlignment(VAlign_Center);
					IconSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
				}
				UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
				Texts->AddChildToVerticalBox(ShowroomStyle::MakeText(WidgetTree, NAME_None, FText::FromString(Info.Name), 24, ShowroomStyle::TextMain(), TEXT("Bold"), false));
				Texts->AddChildToVerticalBox(ShowroomStyle::MakeText(WidgetTree, NAME_None,
					FText::FromString(FString::Printf(TEXT("%d / %d joueurs · %d ms"), Info.Players, Info.MaxPlayers, Info.PingMs)), 19, ShowroomStyle::TextMuted(), TEXT("Regular"), false));
				if (UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts))
				{
					TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
				if (UHorizontalBoxSlot* ChevronSlot = Row->AddChildToHorizontalBox(ShowroomStyle::MakeIcon(WidgetTree, NAME_None, TEXT("chevron_right"), 28.f, ShowroomStyle::TextMuted())))
				{
					ChevronSlot->SetVerticalAlignment(VAlign_Center);
				}
				Button->SetContent(Row);
				Button->SetIsEnabled(!bBusy);
				if (UVerticalBoxSlot* ItemSlot = Cast<UVerticalBoxSlot>(SessionList->AddChild(Button)))
				{
					ItemSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
				}
				AnimateHover(Button, Button, 1.03f, 0.f);
				AnimateIn(Button, 0.05f * JoinButtons.Num(), FVector2D(0.f, 16.f), 0.25f);
				JoinButtons.Add(Button, Info.Index);
			}
		}
	}
}

void UHandMenuWidget::JoinFound(int32 Index)
{
	JoinButtons.Reset(); // un seul clic
	if (UShowroomSessions* Sessions = GetSessions())
	{
		Sessions->JoinSession(Index);
	}
}

void UHandMenuWidget::OnSessionsClicked() { ShowPage(1); RefreshSessions(); }
void UHandMenuWidget::OnBackClicked()     { ShowPage(0); }

void UHandMenuWidget::OnHostClicked()
{
	if (UShowroomSessions* Sessions = GetSessions())
	{
		Sessions->HostSession();
	}
}

void UHandMenuWidget::OnSearchClicked()
{
	if (UShowroomSessions* Sessions = GetSessions())
	{
		Sessions->FindSessions();
	}
}

void UHandMenuWidget::OnLeaveClicked()
{
	if (UShowroomSessions* Sessions = GetSessions())
	{
		Sessions->LeaveSession();
	}
}

void UHandMenuWidget::OnRestartClicked()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FString Level = UGameplayStatics::GetCurrentLevelName(this, true);
	if (World->GetNetMode() == NM_ListenServer)
	{
		World->ServerTravel(Level + TEXT("?listen")); // l'hote relance : tout le monde suit
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*Level));
	}
}

void UHandMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}
