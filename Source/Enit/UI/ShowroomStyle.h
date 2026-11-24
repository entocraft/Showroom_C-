#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UWidgetTree;
class UBorder;
class UButton;
class UTextBlock;
class UTexture2D;

/**
 * CHARTE GRAPHIQUE des interfaces (maquette Figma "PTUT - Showroom" de Hugo).
 *
 * Bleu nuit + accent bleu, coins arrondis. Toutes les couleurs des mises en page par defaut viennent d'ici :
 * changer une couleur ici change toutes les interfaces (apres recompilation et regeneration des WBP,
 * voir Scripts/RebuildUI.py). Dans l'editeur UMG, chaque WBP reste modifiable a la main.
 */
namespace ShowroomStyle
{
	/** Couleur depuis un code hexa "RRGGBB" ou "RRGGBBAA" (sRGB, comme dans Figma). */
	inline FLinearColor Hex(const TCHAR* Code)
	{
		return FLinearColor(FColor::FromHex(Code));
	}

	// --- Palette ---
	inline FLinearColor Background()   { return Hex(TEXT("101125F2")); } // fond des panneaux
	inline FLinearColor Sidebar()      { return Hex(TEXT("0C0E1FF2")); } // menu lateral
	inline FLinearColor Surface()      { return Hex(TEXT("181D30")); }   // cartes, vignettes
	inline FLinearColor SurfaceHover() { return Hex(TEXT("25304D")); }   // carte survolee
	inline FLinearColor SurfaceDeep()  { return Hex(TEXT("09182D")); }   // encadre de description
	inline FLinearColor Outline()      { return Hex(TEXT("2C3345")); }   // bordures discretes
	inline FLinearColor Accent()       { return Hex(TEXT("2E5FA3")); }   // bouton principal
	inline FLinearColor AccentHover()  { return Hex(TEXT("3D78C9")); }
	inline FLinearColor AccentDeep()   { return Hex(TEXT("1A3A70")); }   // onglet / carte selectionnes
	inline FLinearColor AccentText()   { return Hex(TEXT("7FB2FF")); }   // petits titres bleus (metiers...)
	inline FLinearColor Tag()          { return Hex(TEXT("3E5A85")); }   // pastille "EXPERIENCE N°2"
	inline FLinearColor TextMain()         { return FLinearColor::White; }
	inline FLinearColor TextMuted()    { return Hex(TEXT("A9B6CE")); }   // sous-titres
	inline FLinearColor TextDim()      { return Hex(TEXT("6E7C98")); }   // textes tres secondaires
	inline FLinearColor Shade()        { return Hex(TEXT("050A16C8")); } // voile sombre sur les images

	// --- Arrondis (pixels du widget) ---
	constexpr float RadiusPanel = 28.f;
	constexpr float RadiusCard = 16.f;
	constexpr float RadiusSmall = 10.f;

	// --- Outils ---

	/** Pinceau "boite arrondie" uni. */
	ENIT_API FSlateBrush RoundedBrush(const FLinearColor& Color, float Radius);

	/** Pinceau "boite arrondie" avec une bordure fine. */
	ENIT_API FSlateBrush RoundedOutlinedBrush(const FLinearColor& Color, float Radius, const FLinearColor& OutlineColor, float OutlineWidth);

	/** Pinceau image a coins arrondis (Texture peut etre nul : simple fond uni). */
	ENIT_API FSlateBrush RoundedImageBrush(UTexture2D* Texture, float Radius, const FVector2D& Size);

	/** Police par defaut (Roboto du moteur) : Style = "Regular", "Bold", "Light"... */
	ENIT_API FSlateFontInfo Font(int32 Size, FName Style = TEXT("Regular"));

	/** Texte stylise. */
	ENIT_API UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size, const FLinearColor& Color,
		FName FontStyle = TEXT("Regular"), bool bWrap = true);

	/** Encadre arrondi (Border). */
	ENIT_API UBorder* MakeBox(UWidgetTree* Tree, FName Name, const FLinearColor& Color, float Radius, const FMargin& Padding);

	/** Bouton arrondi SANS contenu (a remplir), avec ses etats normal / survol / appui. */
	ENIT_API UButton* MakeButton(UWidgetTree* Tree, FName Name, const FLinearColor& Normal, const FLinearColor& Hovered,
		float Radius, const FMargin& Padding);

	/** Change les couleurs d'un bouton deja construit (ex : onglet selectionne). */
	ENIT_API void SetButtonColors(UButton* Button, const FLinearColor& Normal, const FLinearColor& Hovered, float Radius);
}
