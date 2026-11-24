#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "ShowroomWidget.generated.h"

class UWidgetTree;
class UTextBlock;
class UButton;
class UBorder;
class UTexture2D;

/**
 * Base de toutes les interfaces du showroom.
 *
 * Chaque interface a une MISE EN PAGE PAR DEFAUT construite en C++ (BuildDefaultLayout).
 * Pour la modifier SANS CODE : ouvrir son Widget Blueprint dans Content/Showroom/UI (WBP_...),
 * changer couleurs, textes, tailles, ajouter des images... dans l'editeur UMG.
 * Les widgets dont le nom est important (ex : TitleText, EntryList) sont remplis par le code :
 * on peut les deplacer / restyler, mais il faut garder leur nom.
 *
 * Les WBP sont (re)crees par Scripts/CreateWidgetBlueprints.py a partir de la mise en page par defaut.
 */
UCLASS(Abstract)
class ENIT_API UShowroomWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Construit la mise en page par defaut dans Tree (utilise par l'editeur pour fabriquer le Widget Blueprint). */
	void BuildLayoutInto(UWidgetTree* Tree);

	/** L'acteur qui affiche ce widget (ecran flottant, fiche...). */
	UFUNCTION(BlueprintPure, Category = "Showroom")
	AActor* GetOwnerActor() const { return OwnerActor.Get(); }

	void SetOwnerActor(AActor* InOwner) { OwnerActor = InOwner; }

	/** Textes / image donnes par l'ecran flottant (rien par defaut ; voir SimpleScreenWidget). */
	virtual void SetScreenContent(const FText& Title, const FText& Body, UTexture2D* Image) {}

	/** La classe voulue (Widget Blueprint des reglages) si elle existe et est compatible, sinon Fallback. */
	static TSubclassOf<UUserWidget> ResolveClass(const TSoftClassPtr<UUserWidget>& Wanted, TSubclassOf<UUserWidget> Fallback);

	// --- Outils de mise en page (aussi utiles pour des interfaces faites en C++) ---

	/** Pinceau "coins arrondis" facon Meta Quest. */
	static FSlateBrush MakeRoundedBrush(const FLinearColor& Color, float Radius);

	static UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justify = ETextJustify::Left);

	/** Bouton arrondi avec un texte (le texte est nomme <Name>Text). */
	static UButton* MakeButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize,
		const FLinearColor& Color, const FLinearColor& TextColor = FLinearColor::White, float Radius = 18.f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** La mise en page par defaut (utilisee si le Widget Blueprint est vide ou si on utilise la classe C++). */
	virtual void BuildDefaultLayout() {}

	TWeakObjectPtr<AActor> OwnerActor;
};
