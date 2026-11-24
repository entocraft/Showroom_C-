#include "Interaction/InfoMenu.h"
#include "UI/InfoPanelWidget.h"
#include "Interaction/DetectionZone.h"
#include "Core/ShowroomSettings.h"
#include "Core/ShowroomAudio.h"
#include "UI/FloatingScreenGrabBar.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AInfoMenu::AInfoMenu()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // apres le mouvement de la tete : pas de tremblement

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	RootComponent = Widget;
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetWidgetClass(UInfoPanelWidget::StaticClass());
	Widget->SetDrawSize(FVector2D(1000.f, 600.f));
	Widget->SetPivot(FVector2D(0.5f, 0.5f));
	Widget->SetTwoSided(true);
	Widget->SetRelativeScale3D(FVector(0.05f)); // 1000 px -> 50 cm (ecrase par le reglage InfoPanelScale)
	Widget->SetCollisionProfileName(TEXT("UI"));
	// Le laser des manettes trace sur le canal "3DWidget" (comme dans l'ancien projet).
	Widget->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	// Barre de prise sous la fiche (dans l'espace du widget : 1 unite = 1 pixel).
	GrabBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("GrabBar"));
	GrabBar->SetupAttachment(Widget);
	GrabBar->SetWidgetSpace(EWidgetSpace::World);
	GrabBar->SetWidgetClass(UFloatingScreenGrabBar::StaticClass());
	GrabBar->SetDrawSize(FVector2D(240.f, 28.f));
	GrabBar->SetPivot(FVector2D(0.5f, 0.5f));
	GrabBar->SetTwoSided(true);
	GrabBar->SetRelativeLocation(FVector(0.f, 0.f, -340.f)); // fiche de 600 px de haut : juste en dessous
	GrabBar->SetCollisionProfileName(TEXT("UI"));
	GrabBar->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AInfoMenu::BeginPlay()
{
	// Le Widget Blueprint de la fiche (Project Settings > Showroom > Interfaces), avant la creation du widget.
	Widget->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->InfoPanelWidget, UInfoPanelWidget::StaticClass()));
	GrabBar->SetWidgetClass(UShowroomWidget::ResolveClass(UShowroomSettings::Get()->GrabBarWidget, UFloatingScreenGrabBar::StaticClass()));
	Super::BeginPlay();

	if (UShowroomWidget* Bar = Cast<UShowroomWidget>(GrabBar->GetUserWidgetObject()))
	{
		Bar->SetOwnerActor(this); // la barre previent la fiche quand on l'attrape
	}

	if (bFollowPlayer)
	{
		// Le panneau se deplace librement dans le monde (il n'est plus lie a la zone).
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorScale3D(FVector(UShowroomSettings::Get()->InfoPanelScale));
	}

	if (UInfoPanelWidget* Panel = GetPanel())
	{
		Panel->OnActionClicked.AddDynamic(this, &AInfoMenu::HandleActionClicked);
		Panel->OnPreviousClicked.AddDynamic(this, &AInfoMenu::HandlePreviousClicked);
		Panel->OnNextClicked.AddDynamic(this, &AInfoMenu::HandleNextClicked);
	}

	SetInfo(Info);
	ShowMenu(bVisibleAtStart);
}

void AInfoMenu::SetActionButton(bool bVisible, const FText& Label)
{
	if (UInfoPanelWidget* Panel = GetPanel())
	{
		Panel->SetActionButton(bVisible, Label);
	}
}

void AInfoMenu::BeginDrag()
{
	Drag.Begin(this, GrabBar, GetActorLocation());
}

void AInfoMenu::EndDrag()
{
	Drag.End();
}

void AInfoMenu::SetStepNav(bool bVisible, const FText& StepLabel, bool bCanPrevious, bool bCanNext)
{
	if (UInfoPanelWidget* Panel = GetPanel())
	{
		Panel->SetStepNav(bVisible, StepLabel, bCanPrevious, bCanNext);
	}
}

void AInfoMenu::HandlePreviousClicked()
{
	ShowroomAudio::Play2D(this, UShowroomSettings::Get()->SoundUIClick, UShowroomSettings::Get()->EffectsVolume);
	if (ADetectionZone* Zone = Cast<ADetectionZone>(ActiveZone))
	{
		Zone->PreviousStep();
	}
}

void AInfoMenu::HandleNextClicked()
{
	ShowroomAudio::Play2D(this, UShowroomSettings::Get()->SoundUIClick, UShowroomSettings::Get()->EffectsVolume);
	if (ADetectionZone* Zone = Cast<ADetectionZone>(ActiveZone))
	{
		Zone->NextStep();
	}
}

void AInfoMenu::HandleActionClicked()
{
	ShowroomAudio::Play2D(this, UShowroomSettings::Get()->SoundUIClick, UShowroomSettings::Get()->EffectsVolume);
	if (ADetectionZone* Zone = Cast<ADetectionZone>(ActiveZone))
	{
		Zone->ToggleExperience();
	}
}

UInfoPanelWidget* AInfoMenu::GetPanel() const
{
	return Widget ? Cast<UInfoPanelWidget>(Widget->GetUserWidgetObject()) : nullptr;
}

void AInfoMenu::SetInfo(const FShowroomInfo& NewInfo)
{
	Info = NewInfo;
	if (UInfoPanelWidget* Panel = GetPanel())
	{
		Panel->SetInfo(Info);
	}
}

void AInfoMenu::ShowMenu(bool bShow)
{
	const bool bWasVisible = bMenuVisible;
	bMenuVisible = bShow;
	if (bShow && !bWasVisible)
	{
		bAnchored = false; // la fiche revient devant le joueur a chaque ouverture
	}
	if (!bShow)
	{
		Drag.End();
	}

	// En apparaissant, le panneau se place directement devant le joueur (pas de "vol" a travers la piece).
	if (bShow && !bWasVisible && bFollowPlayer)
	{
		FVector Location;
		FQuat Rotation;
		if (ComputeTargetTransform(Location, Rotation))
		{
			SetActorLocationAndRotation(Location, Rotation);
		}
	}

	SetActorHiddenInGame(!bShow);
	SetActorEnableCollision(bShow); // sinon le laser touche un panneau invisible
	SetActorTickEnabled(bShow);
}

FQuat AInfoMenu::FaceTowards(const FVector& From, const FVector& Eyes)
{
	// La face du widget est son axe +X : on le pointe vers les yeux (ce qui incline aussi le panneau
	// vers le haut quand il est plus bas que la tete). Roulis nul pour qu'il reste bien droit.
	FRotator Rotation = (Eyes - From).Rotation();
	Rotation.Roll = 0.f;
	return Rotation.Quaternion();
}

bool AInfoMenu::ComputeTargetTransform(FVector& OutLocation, FQuat& OutRotation) const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return false;
	}

	const UShowroomSettings* Settings = UShowroomSettings::Get();
	// La camera VR = la tete du joueur.
	const UCameraComponent* Head = Player->FindComponentByClass<UCameraComponent>();
	const FVector Eyes = Head ? Head->GetComponentLocation() : Player->GetPawnViewLocation();
	const FRotator View = Head ? Head->GetComponentRotation() : Player->GetActorRotation();

	// Seulement la direction horizontale du regard : regarder en haut ou en bas ne deplace pas le panneau.
	const FVector Forward = FRotator(0.f, View.Yaw, 0.f).Vector();

	OutLocation = Eyes + Forward * Settings->InfoPanelDistance - FVector(0.f, 0.f, Settings->InfoPanelHeightBelowEyes);
	OutRotation = FaceTowards(OutLocation, Eyes);
	return true;
}

void AInfoMenu::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	// --- Deplacement par le joueur (barre de prise) ---
	if (Drag.IsActive())
	{
		if (Drag.Update(this, DeltaSeconds))
		{
			SetActorRotation(FaceTowards(GetActorLocation(), Player->GetPawnViewLocation()));
			return;
		}
		bAnchored = true; // relachee : la fiche reste ou le joueur l'a posee
	}

	// --- Fiche posee : elle reste en place, tournee vers le joueur, tant qu'il ne s'eloigne pas ---
	if (bAnchored)
	{
		const FVector Eyes = Player->GetPawnViewLocation();
		if (FVector::Dist2D(Eyes, GetActorLocation()) > UShowroomSettings::Get()->InfoPanelReleaseDistance)
		{
			bAnchored = false; // trop loin : elle revient devant le joueur (lissage ci-dessous)
		}
		else
		{
			SetActorRotation(FaceTowards(GetActorLocation(), Eyes));
			return;
		}
	}

	if (!bFollowPlayer)
	{
		// Panneau fixe : il se tourne simplement vers le joueur.
		SetActorRotation(FaceTowards(GetActorLocation(), Player->GetPawnViewLocation()));
		return;
	}

	FVector TargetLocation;
	FQuat TargetRotation;
	if (!ComputeTargetTransform(TargetLocation, TargetRotation))
	{
		return;
	}

	// Lissage exponentiel : le panneau rattrape sa cible en douceur, independamment des FPS.
	// FollowSpeed petit = plus de retard ; grand = plus "colle".
	// Si le joueur s'est teleporte (grand saut), le panneau se replace directement devant lui.
	const bool bTooFar = FVector::DistSquared(GetActorLocation(), TargetLocation) > FMath::Square(150.f);
	const float Alpha = bTooFar ? 1.f : 1.f - FMath::Exp(-UShowroomSettings::Get()->InfoPanelFollowSpeed * DeltaSeconds);
	const FVector NewLocation = FMath::Lerp(GetActorLocation(), TargetLocation, Alpha);

	// L'orientation vise les yeux depuis la position LISSEE : le panneau reste toujours lisible,
	// meme pendant qu'il rattrape le joueur.
	SetActorLocationAndRotation(NewLocation, FaceTowards(NewLocation, Player->GetPawnViewLocation()));
}

#if WITH_EDITOR
void AInfoMenu::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	SetInfo(Info);
}
#endif
