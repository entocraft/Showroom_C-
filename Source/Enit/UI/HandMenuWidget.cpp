#include "UI/HandMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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
	RestartButton = MakeButton(WidgetTree, TEXT("RestartButton"), FText::FromString(TEXT("Recommencer")), 32, Blue);
	QuitButton = MakeButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quitter")), 32, FLinearColor(0.25f, 0.25f, 0.3f, 1.f));
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
