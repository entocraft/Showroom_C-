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
 *
 * ANIMATIONS (sans timeline UMG, calculees chaque image) :
 *   - AnimateHover : l'element se souleve et grossit un peu quand le laser le survole (+ petit son),
 *   - AnimateIn    : apparition en fondu + glissement (avec un delai, pour des apparitions en cascade),
 *   - AnimatePulse : respiration douce (boutons a mettre en avant : GPS, Lancer l'experience...).
 * PlayIntro() rejoue l'apparition du widget (appele quand un ecran / une fiche s'affiche).
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

	/** Rejoue l'animation d'apparition (ecran / fiche qui s'affiche). */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Animation")
	virtual void PlayIntro();

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

	// --- Animations (utilisables depuis les Widget Blueprints) ---

	/** Survol : Target se souleve (Lift, px) et grossit (Scale) quand Button est survole. */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Animation")
	void AnimateHover(UWidget* Target, UButton* Button, float Scale = 1.04f, float Lift = -6.f);

	/** Apparition : fondu + glissement depuis Offset (px), apres Delay (s). */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Animation")
	void AnimateIn(UWidget* Target, float Delay = 0.f, FVector2D Offset = FVector2D(0.f, 28.f), float Duration = 0.35f);

	/** Respiration : grossit doucement entre 1 et MaxScale (Speed = cycles par seconde). */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Animation")
	void AnimatePulse(UWidget* Target, float MaxScale = 1.035f, float Speed = 0.8f);

	UFUNCTION(BlueprintCallable, Category = "Showroom|Animation")
	void StopPulse(UWidget* Target);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** La mise en page par defaut (utilisee si le Widget Blueprint est vide ou si on utilise la classe C++). */
	virtual void BuildDefaultLayout() {}

	TWeakObjectPtr<AActor> OwnerActor;

private:
	struct FHoverAnim { TWeakObjectPtr<UWidget> Target; TWeakObjectPtr<UButton> Button; float Scale = 1.f; float Lift = 0.f; float Alpha = 0.f; bool bHovered = false; };
	struct FInAnim { TWeakObjectPtr<UWidget> Target; FVector2D Offset; float Delay = 0.f; float Duration = 0.3f; float Time = 0.f; };
	struct FPulseAnim { TWeakObjectPtr<UWidget> Target; float MaxScale = 1.f; float Speed = 1.f; float Time = 0.f; };

	TArray<FHoverAnim> HoverAnims;
	TArray<FInAnim> InAnims;
	TArray<FPulseAnim> PulseAnims;
	bool bAnimating = false;   // des transformations ont ete appliquees (a remettre a zero a la fin)
};
