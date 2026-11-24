#include "UI/FloatingScreenGrabBar.h"
#include "UI/ScreenDrag.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"

void UFloatingScreenGrabBar::BuildDefaultLayout()
{
	// Une "pilule" gris clair, comme sous les fenetres du Meta Quest.
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
	Size->SetWidthOverride(240.f);
	Size->SetHeightOverride(28.f);
	WidgetTree->RootWidget = Size;

	GrabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GrabButton"));
	FButtonStyle Style = GrabButton->GetStyle();
	Style.SetNormal(MakeRoundedBrush(FLinearColor(0.75f, 0.75f, 0.8f, 0.55f), 14.f));
	Style.SetHovered(MakeRoundedBrush(FLinearColor(1.f, 1.f, 1.f, 0.9f), 14.f));
	Style.SetPressed(MakeRoundedBrush(FLinearColor(0.55f, 0.75f, 1.f, 1.f), 14.f));
	GrabButton->SetStyle(Style);
	Size->SetContent(GrabButton);
}

void UFloatingScreenGrabBar::NativeConstruct()
{
	Super::NativeConstruct();
	if (GrabButton)
	{
		GrabButton->OnPressed.RemoveDynamic(this, &UFloatingScreenGrabBar::HandlePressed);
		GrabButton->OnPressed.AddDynamic(this, &UFloatingScreenGrabBar::HandlePressed);
		GrabButton->OnReleased.RemoveDynamic(this, &UFloatingScreenGrabBar::HandleReleased);
		GrabButton->OnReleased.AddDynamic(this, &UFloatingScreenGrabBar::HandleReleased);
	}
}

void UFloatingScreenGrabBar::HandlePressed()
{
	if (IShowroomDraggable* Draggable = Cast<IShowroomDraggable>(GetOwnerActor()))
	{
		Draggable->BeginDrag();
	}
}

void UFloatingScreenGrabBar::HandleReleased()
{
	if (IShowroomDraggable* Draggable = Cast<IShowroomDraggable>(GetOwnerActor()))
	{
		Draggable->EndDrag();
	}
}
