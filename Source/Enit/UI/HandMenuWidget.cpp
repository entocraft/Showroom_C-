#include "UI/HandMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "UI/ShowroomStyle.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UHandMenuWidget::BuildDefaultLayout()
{
	//  Border arrondi
	//   └ VerticalBox : titre, Recommencer, Quitter
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Root->SetBrush(MakeRoundedBrush(FLinearColor(0.03f, 0.03f, 0.04f, 0.92f), 28.f));
	Root->SetPadding(FMargin(24.f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Root->SetContent(Column);

	Column->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Menu")), 40, FLinearColor::White, ETextJustify::Center));

	const FLinearColor Blue(0.1f, 0.45f, 0.9f, 1.f);
	// Boutons arrondis : une icone + un libelle
	auto MakeMenuButton = [this](const TCHAR* Name, const TCHAR* Icon, const TCHAR* Label, const FLinearColor& Color)
	{
		UButton* Button = ShowroomStyle::MakeButton(WidgetTree, Name, Color, Color * 1.35f, 18.f, FMargin(24.f, 12.f));
		UTextBlock* Text = nullptr;
		Button->SetContent(ShowroomStyle::MakeIconLabel(WidgetTree, *(FString(Name) + TEXT("Label")), Icon, FText::FromString(Label), 32, FLinearColor::White, TEXT("Bold"), Text, 12.f));
		return Button;
	};
	RestartButton = MakeMenuButton(TEXT("RestartButton"), TEXT("refresh"), TEXT("Recommencer"), FLinearColor(0.1f, 0.45f, 0.9f, 1.f));
	QuitButton = MakeMenuButton(TEXT("QuitButton"), TEXT("power"), TEXT("Quitter"), FLinearColor(0.25f, 0.25f, 0.3f, 1.f));
	for (UButton* Button : { RestartButton.Get(), QuitButton.Get() })
	{
		if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
		}
	}
}

void UHandMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RestartButton)
	{
		RestartButton->OnClicked.RemoveDynamic(this, &UHandMenuWidget::OnRestartClicked);
		RestartButton->OnClicked.AddDynamic(this, &UHandMenuWidget::OnRestartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UHandMenuWidget::OnQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UHandMenuWidget::OnQuitClicked);
	}
	AnimateHover(RestartButton, RestartButton, 1.05f, 0.f);
	AnimateHover(QuitButton, QuitButton, 1.05f, 0.f);
}

void UHandMenuWidget::OnRestartClicked()
{
	// Recharge le niveau actuel.
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void UHandMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}
