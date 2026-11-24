#include "UI/ShowroomStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

namespace ShowroomStyle
{
	FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Color);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius));
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}

	FSlateBrush RoundedOutlinedBrush(const FLinearColor& Color, float Radius, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush = RoundedBrush(Color, Radius);
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.Width = OutlineWidth;
		return Brush;
	}

	FSlateBrush RoundedImageBrush(UTexture2D* Texture, float Radius, const FVector2D& Size)
	{
		FSlateBrush Brush = RoundedBrush(Texture ? FLinearColor::White : Surface(), Radius);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;

		// Remplissage facon "cover" : on recadre l'image au centre au lieu de la deformer.
		if (Texture && Size.X > 0.f && Size.Y > 0.f)
		{
			const float TexW = Texture->GetSurfaceWidth();
			const float TexH = Texture->GetSurfaceHeight();
			if (TexW > 0.f && TexH > 0.f)
			{
				const float BoxAspect = Size.X / Size.Y;
				const float TexAspect = TexW / TexH;
				FVector2f Min(0.f, 0.f), Max(1.f, 1.f);
				if (TexAspect > BoxAspect)
				{
					const float Keep = BoxAspect / TexAspect; // part de la largeur gardee
					Min.X = 0.5f - Keep * 0.5f;
					Max.X = 0.5f + Keep * 0.5f;
				}
				else
				{
					const float Keep = TexAspect / BoxAspect; // part de la hauteur gardee
					Min.Y = 0.5f - Keep * 0.5f;
					Max.Y = 0.5f + Keep * 0.5f;
				}
				Brush.SetUVRegion(FBox2f(Min, Max));
			}
		}
		return Brush;
	}

	FSlateFontInfo Font(int32 Size, FName Style)
	{
		// La police Roboto du moteur (celle des TextBlock par defaut).
		static const UObject* Roboto = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		FSlateFontInfo Info(Roboto, Size, Style);
		return Info;
	}

	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size, const FLinearColor& Color, FName FontStyle, bool bWrap)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetFont(Font(Size, FontStyle));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetAutoWrapText(bWrap);
		Block->SetText(Text);
		return Block;
	}

	UBorder* MakeBox(UWidgetTree* Tree, FName Name, const FLinearColor& Color, float Radius, const FMargin& Padding)
	{
		UBorder* Box = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Box->SetBrush(RoundedBrush(Color, Radius));
		Box->SetPadding(Padding);
		return Box;
	}

	UButton* MakeButton(UWidgetTree* Tree, FName Name, const FLinearColor& Normal, const FLinearColor& Hovered, float Radius, const FMargin& Padding)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormalPadding(Padding);
		// Petit enfoncement visuel a l'appui (le contenu descend d'1 px).
		Style.SetPressedPadding(FMargin(Padding.Left, Padding.Top + 1.f, Padding.Right, Padding.Bottom - 1.f));
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
		SetButtonColors(Button, Normal, Hovered, Radius);
		return Button;
	}

	void SetButtonColors(UButton* Button, const FLinearColor& Normal, const FLinearColor& Hovered, float Radius)
	{
		if (!Button)
		{
			return;
		}
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(RoundedBrush(Normal, Radius));
		// Survol : couleur plus claire + liseré clair, bien visible au laser.
		Style.SetHovered(RoundedOutlinedBrush(Hovered, Radius, FLinearColor(1.f, 1.f, 1.f, 0.55f), 2.f));
		Style.SetPressed(RoundedBrush(Normal * 0.75f, Radius));
		Style.SetDisabled(RoundedBrush(Normal * FLinearColor(1.f, 1.f, 1.f, 0.4f), Radius));
		Button->SetStyle(Style);
	}

	FLinearColor ExperienceColor(int32 Index)
	{
		static const TCHAR* Palette[] = { TEXT("2E5FA3"), TEXT("7B4FC9"), TEXT("1F8A8A"), TEXT("C9783A"), TEXT("B8457A"), TEXT("4F8A3A") };
		return Hex(Palette[((Index % 6) + 6) % 6]);
	}

	UTexture2D* Icon(const TCHAR* Name)
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Cache;
		const FString Key(Name);
		if (const TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Key))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
		const FString Path = FString::Printf(TEXT("/Game/Showroom/UI/Icons/T_Icon_%s.T_Icon_%s"), Name, Name);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		Cache.Add(Key, Texture);
		return Texture;
	}

	UImage* MakeIcon(UWidgetTree* Tree, FName Name, const TCHAR* IconName, float Size, const FLinearColor& Color)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		FSlateBrush Brush;
		Brush.SetResourceObject(Icon(IconName));
		Brush.ImageSize = FVector2D(Size, Size);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Color);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	void SetIcon(UImage* Image, const TCHAR* IconName)
	{
		if (Image)
		{
			FSlateBrush Brush = Image->GetBrush();
			Brush.SetResourceObject(Icon(IconName));
			Image->SetBrush(Brush);
		}
	}

	UHorizontalBox* MakeIconLabel(UWidgetTree* Tree, FName Name, const TCHAR* IconName, const FText& Text,
		int32 FontSize, const FLinearColor& Color, FName FontStyle, UTextBlock*& OutText, float Gap)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
		const FString Base = Name.IsNone() ? FString() : Name.ToString();
		UImage* IconImage = MakeIcon(Tree, Base.IsEmpty() ? NAME_None : FName(*(Base + TEXT("Icon"))), IconName, FontSize * 1.15f, Color);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconImage))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, Gap, 0.f));
		}
		OutText = MakeText(Tree, Base.IsEmpty() ? NAME_None : FName(*(Base + TEXT("Text"))), Text, FontSize, Color, FontStyle, false);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutText))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Row;
	}
}
