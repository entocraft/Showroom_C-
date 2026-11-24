#include "UI/HandMenu.h"
#include "UI/HandMenuWidget.h"
#include "Core/ShowroomSettings.h"
#include "Components/WidgetComponent.h"

AHandMenu::AHandMenu()
{
	PrimaryActorTick.bCanEverTick = false;

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetWidgetClass(UHandMenuWidget::StaticClass());
	Widget->SetDrawSize(FVector2D(400.f, 320.f));
	Widget->SetPivot(FVector2D(0.5f, 0.f));
	Widget->SetTwoSided(true);

	// Le panneau est petit (~16 cm de large), un peu au-dessus de la manette, tourne vers le joueur.
	Widget->SetRelativeScale3D(FVector(0.04f));

	// Le laser des manettes trace sur le canal "3DWidget".
	Widget->SetCollisionProfileName(TEXT("UI"));
	Widget->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AHandMenu::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	Widget->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->HandMenuWidget, UHandMenuWidget::StaticClass()));
}
