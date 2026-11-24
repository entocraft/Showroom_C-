#include "UI/ShowroomWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

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
