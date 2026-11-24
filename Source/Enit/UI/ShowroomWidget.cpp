#include "UI/ShowroomWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/ShowroomAudio.h"
#include "Core/ShowroomSettings.h"

namespace
{
	/** Courbe douce (rapide au debut, ralentit a la fin). */
	float EaseOutCubic(float T)
	{
		const float U = 1.f - FMath::Clamp(T, 0.f, 1.f);
		return 1.f - U * U * U;
	}
}

void UShowroomWidget::PlayIntro()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		AnimateIn(WidgetTree->RootWidget, 0.f, FVector2D(0.f, 24.f), 0.3f);
	}
}

void UShowroomWidget::AnimateHover(UWidget* Target, UButton* Button, float Scale, float Lift)
{
	if (!Target || !Button)
	{
		return;
	}
	HoverAnims.RemoveAll([Target](const FHoverAnim& A) { return A.Target.Get() == Target; });
	FHoverAnim& Anim = HoverAnims.AddDefaulted_GetRef();
	Anim.Target = Target;
	Anim.Button = Button;
	Anim.Scale = Scale;
	Anim.Lift = Lift;
	Target->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

void UShowroomWidget::AnimateIn(UWidget* Target, float Delay, FVector2D Offset, float Duration)
{
	if (!Target)
	{
		return;
	}
	InAnims.RemoveAll([Target](const FInAnim& A) { return A.Target.Get() == Target; });
	FInAnim& Anim = InAnims.AddDefaulted_GetRef();
	Anim.Target = Target;
	Anim.Offset = Offset;
	Anim.Delay = Delay;
	Anim.Duration = FMath::Max(Duration, 0.01f);
	// Invisible tout de suite (sinon on le verrait une image avant l'animation).
	Target->SetRenderOpacity(0.f);
	Target->SetRenderTranslation(Offset);
}

void UShowroomWidget::AnimatePulse(UWidget* Target, float MaxScale, float Speed)
{
	if (!Target)
	{
		return;
	}
	for (const FPulseAnim& A : PulseAnims)
	{
		if (A.Target.Get() == Target)
		{
			return; // deja en cours : on ne recommence pas le cycle
		}
	}
	FPulseAnim& Anim = PulseAnims.AddDefaulted_GetRef();
	Anim.Target = Target;
	Anim.MaxScale = MaxScale;
	Anim.Speed = Speed;
	Target->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

void UShowroomWidget::StopPulse(UWidget* Target)
{
	PulseAnims.RemoveAll([Target](const FPulseAnim& A) { return A.Target.Get() == Target; });
	if (Target)
	{
		Target->SetRenderScale(FVector2D(1.f, 1.f));
	}
}

void UShowroomWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HoverAnims.Num() == 0 && InAnims.Num() == 0 && PulseAnims.Num() == 0)
	{
		return;
	}

	// On additionne les effets de toutes les animations d'un meme widget.
	struct FState { FVector2D Translation = FVector2D::ZeroVector; float Scale = 1.f; float Opacity = 1.f; };
	TMap<UWidget*, FState> States;

	const UShowroomSettings* Settings = UShowroomSettings::Get();
	for (FHoverAnim& Anim : HoverAnims)
	{
		UWidget* Target = Anim.Target.Get();
		const UButton* Button = Anim.Button.Get();
		if (!Target || !Button)
		{
			continue;
		}
		const bool bHovered = Button->IsHovered() && Button->GetIsEnabled();
		if (bHovered && !Anim.bHovered)
		{
			ShowroomAudio::Play2D(this, Settings->SoundUIHover, Settings->EffectsVolume * 0.6f); // petit "tic" au survol
		}
		Anim.bHovered = bHovered;
		Anim.Alpha = FMath::FInterpTo(Anim.Alpha, bHovered ? 1.f : 0.f, InDeltaTime, 14.f);
		FState& State = States.FindOrAdd(Target);
		State.Scale *= FMath::Lerp(1.f, Anim.Scale, Anim.Alpha);
		State.Translation.Y += Anim.Lift * Anim.Alpha;
	}

	for (int32 i = InAnims.Num() - 1; i >= 0; --i)
	{
		FInAnim& Anim = InAnims[i];
		UWidget* Target = Anim.Target.Get();
		if (!Target)
		{
			InAnims.RemoveAt(i);
			continue;
		}
		Anim.Time += InDeltaTime;
		const float T = EaseOutCubic((Anim.Time - Anim.Delay) / Anim.Duration);
		FState& State = States.FindOrAdd(Target);
		State.Opacity *= T;
		State.Translation += Anim.Offset * (1.f - T);
		if (Anim.Time >= Anim.Delay + Anim.Duration)
		{
			InAnims.RemoveAt(i);
		}
	}

	for (FPulseAnim& Anim : PulseAnims)
	{
		if (UWidget* Target = Anim.Target.Get())
		{
			Anim.Time += InDeltaTime;
			const float Wave = 0.5f - 0.5f * FMath::Cos(Anim.Time * UE_TWO_PI * Anim.Speed);
			States.FindOrAdd(Target).Scale *= FMath::Lerp(1.f, Anim.MaxScale, Wave);
		}
	}

	for (const TPair<UWidget*, FState>& Pair : States)
	{
		Pair.Key->SetRenderTranslation(Pair.Value.Translation);
		Pair.Key->SetRenderScale(FVector2D(Pair.Value.Scale, Pair.Value.Scale));
		Pair.Key->SetRenderOpacity(Pair.Value.Opacity);
	}
	HoverAnims.RemoveAll([](const FHoverAnim& A) { return !A.Target.IsValid(); });
	PulseAnims.RemoveAll([](const FPulseAnim& A) { return !A.Target.IsValid(); });
}

TSharedRef<SWidget> UShowroomWidget::RebuildWidget()
{
	// Widget Blueprint vide (ou classe C++ utilisee directement) -> mise en page par defaut.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	return Super::RebuildWidget();
}

TSubclassOf<UUserWidget> UShowroomWidget::ResolveClass(const TSoftClassPtr<UUserWidget>& Wanted, TSubclassOf<UUserWidget> Fallback)
{
	if (!Wanted.IsNull())
	{
		UClass* Loaded = Wanted.LoadSynchronous();
		if (Loaded && (!Fallback || Loaded->IsChildOf(Fallback)))
		{
			return Loaded;
		}
	}
	return Fallback;
}

void UShowroomWidget::BuildLayoutInto(UWidgetTree* Tree)
{
	if (!Tree)
	{
		return;
	}
	// On construit dans l'arbre fourni (celui du Widget Blueprint) au lieu du notre.
	UWidgetTree* Own = WidgetTree;
	WidgetTree = Tree;
	BuildDefaultLayout();
	WidgetTree = Own;
}

FSlateBrush UShowroomWidget::MakeRoundedBrush(const FLinearColor& Color, float Radius)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Color);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius));
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	return Brush;
}

UTextBlock* UShowroomWidget::MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size, const FLinearColor& Color, ETextJustify::Type Justify)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = Size;
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetJustification(Justify);
	Block->SetAutoWrapText(true);
	Block->SetText(Text);
	return Block;
}

UButton* UShowroomWidget::MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize, const FLinearColor& Color, const FLinearColor& TextColor, float Radius)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

	// Trois etats arrondis : normal, survole (plus clair), appuye (plus sombre).
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(MakeRoundedBrush(Color, Radius));
	Style.SetHovered(MakeRoundedBrush(Color * 1.35f, Radius));
	Style.SetPressed(MakeRoundedBrush(Color * 0.7f, Radius));
	Style.SetNormalPadding(FMargin(24.f, 10.f));
	Style.SetPressedPadding(FMargin(24.f, 11.f, 24.f, 9.f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);

	UTextBlock* Text = MakeText(Tree, FName(*(Name.ToString() + TEXT("Text"))), Label, FontSize, TextColor, ETextJustify::Center);
	Button->SetContent(Text);
	return Button;
}
