#include "UI/FloatingScreen.h"
#include "UI/FloatingScreenGrabBar.h"
#include "UI/ShowroomWidget.h"
#include "Player/ShowroomPawn.h"
#include "Core/ShowroomSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float GrabBarGap = 6.f;       // espace entre l'ecran et la barre (cm)
	constexpr float ScreenFadeSpeed = 4.f;        // 1/s

	void SetupWidgetComponent(UWidgetComponent* Widget)
	{
		Widget->SetWidgetSpace(EWidgetSpace::World);
		Widget->SetPivot(FVector2D(0.5f, 0.5f));
		Widget->SetTwoSided(false);
		Widget->SetBlendMode(EWidgetBlendMode::Transparent);
		Widget->SetCollisionProfileName(TEXT("UI"));
		// Le laser des manettes trace sur le canal "3DWidget".
		Widget->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	}
}

AFloatingScreen::AFloatingScreen()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // apres le mouvement des mains

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Root);
	SetupWidgetComponent(Screen);

	GrabBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("GrabBar"));
	GrabBar->SetupAttachment(Root);
	SetupWidgetComponent(GrabBar);
	GrabBar->SetWidgetClass(UFloatingScreenGrabBar::StaticClass());
	GrabBar->SetDrawSize(FVector2D(240.f, 28.f));

	ApplyLayout();
}

void AFloatingScreen::ApplyLayout()
{
	const float Scale = WorldWidth / FMath::Max(Resolution.X, 1.f);
	Screen->SetDrawSize(Resolution);
	Screen->SetRelativeScale3D(FVector(Scale));
	if (Curvature > 0.5f)
	{
		Screen->SetGeometryMode(EWidgetGeometryMode::Cylinder);
		Screen->SetCylinderArcAngle(Curvature);
	}
	else
	{
		Screen->SetGeometryMode(EWidgetGeometryMode::Plane);
	}
	if (Content)
	{
		Screen->SetWidgetClass(Content);
	}

	// La barre sous l'ecran (meme echelle, pour garder les memes proportions).
	GrabBar->SetRelativeScale3D(FVector(Scale));
	GrabBar->SetRelativeLocation(FVector(0.f, 0.f, -(Resolution.Y * Scale * 0.5f + GrabBarGap)));
	GrabBar->SetVisibility(bMovable);
	GrabBar->SetCollisionEnabled(bMovable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AFloatingScreen::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLayout(); // apercu dans l'editeur
	PushTexts();
}

void AFloatingScreen::BeginPlay()
{
	ApplyLayout(); // la classe du contenu doit etre connue AVANT que les widgets soient crees
	GrabBar->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->GrabBarWidget, UFloatingScreenGrabBar::StaticClass()));
	Super::BeginPlay();

	// Les widgets savent qui les affiche (pour "Get Owner Actor" dans les Widget Blueprints).
	if (UShowroomWidget* Widget = Cast<UShowroomWidget>(Screen->GetUserWidgetObject()))
	{
		Widget->SetOwnerActor(this);
	}
	if (UShowroomWidget* Bar = Cast<UShowroomWidget>(GrabBar->GetUserWidgetObject()))
	{
		Bar->SetOwnerActor(this);
	}

	PushTexts();
	if (bFacePlayerAtStart)
	{
		FacePlayer();
	}
	bWantVisible = bVisibleAtStart;
	Alpha = 0.f;
	SetActorHiddenInGame(!bWantVisible);
}

UUserWidget* AFloatingScreen::GetContentWidget() const
{
	return Screen ? Screen->GetUserWidgetObject() : nullptr;
}

void AFloatingScreen::ShowScreen()
{
	if (!bWantVisible)
	{
		if (UShowroomWidget* Widget = Cast<UShowroomWidget>(GetContentWidget()))
		{
			Widget->PlayIntro(); // le contenu apparait en glissant
		}
	}
	bWantVisible = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
}

void AFloatingScreen::HideScreen()
{
	bWantVisible = false;
	EndDrag();
	SetActorEnableCollision(false); // plus cliquable pendant qu'il s'efface
	SetActorTickEnabled(true);
}

FVector AFloatingScreen::GetPlayerEyes() const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return GetActorLocation() - GetActorForwardVector() * 100.f;
	}
	const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
	return Camera ? Camera->GetComponentLocation() : Player->GetPawnViewLocation();
}

void AFloatingScreen::FacePlayer()
{
	// La face du widget est son axe +X : on la tourne vers les yeux, en restant bien droit.
	const FVector ToEyes = GetPlayerEyes() - GetActorLocation();
	SetActorRotation(FRotator(0.f, ToEyes.Rotation().Yaw, 0.f));
}

void AFloatingScreen::BeginDrag()
{
	if (bMovable && Drag.Begin(this, GrabBar, GetActorLocation()))
	{
		SetActorTickEnabled(true);
	}
}

void AFloatingScreen::EndDrag()
{
	Drag.End();
}

void AFloatingScreen::PushTexts() const
{
	if (UShowroomWidget* Widget = Cast<UShowroomWidget>(GetContentWidget()))
	{
		Widget->SetScreenContent(Title, Body, Image);
	}
}

void AFloatingScreen::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// --- Deplacement avec la barre : la barre suit le laser, l'ecran se tourne vers le joueur ---
	if (Drag.IsActive() && Drag.Update(this, DeltaSeconds))
	{
		FacePlayer();
	}

	// --- Apparition / disparition ---
	const float Wanted = bWantVisible ? 1.f : 0.f;
	Alpha = FMath::FInterpConstantTo(Alpha, Wanted, DeltaSeconds, ScreenFadeSpeed);
	const FLinearColor Tint(1.f, 1.f, 1.f, Alpha);
	Screen->SetTintColorAndOpacity(Tint);
	GrabBar->SetTintColorAndOpacity(Tint);

	if (!bWantVisible && Alpha <= 0.f)
	{
		SetActorHiddenInGame(true);
	}
	if (Alpha == Wanted && !Drag.IsActive())
	{
		SetActorTickEnabled(false);
	}
}
