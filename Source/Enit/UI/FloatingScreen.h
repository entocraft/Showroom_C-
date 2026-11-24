#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/ScreenDrag.h"
#include "FloatingScreen.generated.h"

class UWidgetComponent;
class UWidgetInteractionComponent;
class UUserWidget;
class UTexture2D;

/**
 * ECRAN FLOTTANT facon Meta Quest.
 *
 * A glisser dans le niveau, puis choisir son contenu dans "Content" : N'IMPORTE QUEL Widget Blueprint
 * (cree et modifie dans l'editeur UMG, sans code). Exemples fournis dans Content/Showroom/UI.
 *
 *   - ecran legerement incurve (Curvature), coins arrondis (selon le Widget Blueprint),
 *   - barre de prise sous l'ecran : viser la barre + gachette maintenue = deplacer l'ecran,
 *   - apparition en fondu, se tourne vers le joueur au lancement si demande.
 *
 * Le Widget Blueprint peut appeler "Get Owner Actor" (s'il herite de ShowroomWidget) pour piloter l'ecran
 * (ex : Hide Screen).
 */
UCLASS(meta = (DisplayName = "Ecran flottant"))
class ENIT_API AFloatingScreen : public AActor, public IShowroomDraggable
{
	GENERATED_BODY()

public:
	AFloatingScreen();

	/** Le contenu de l'ecran : un Widget Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran")
	TSubclassOf<UUserWidget> Content;

	/** Resolution du widget (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran")
	FVector2D Resolution = FVector2D(1280.f, 800.f);

	/** Largeur de l'ecran dans le monde (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran", meta = (ClampMin = "10"))
	float WorldWidth = 120.f;

	/** Courbure de l'ecran (degres d'arc). 0 = plat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran", meta = (ClampMin = "0", ClampMax = "120"))
	float Curvature = 25.f;

	/** Barre de prise sous l'ecran pour le deplacer avec le laser. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran")
	bool bMovable = true;

	/** Au lancement, l'ecran se tourne vers le joueur (sinon il garde l'orientation choisie dans le niveau). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran")
	bool bFacePlayerAtStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecran")
	bool bVisibleAtStart = true;

	/**
	 * Textes de l'ecran, SANS modifier le Widget Blueprint : remplissent les widgets TitleText, BodyText et
	 * MainImage du contenu (ex : WBP_EcranDetail). Vide = garde le texte du Widget Blueprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contenu")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contenu", meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contenu")
	TObjectPtr<UTexture2D> Image;

	UFUNCTION(BlueprintCallable, Category = "Ecran")
	void ShowScreen();

	UFUNCTION(BlueprintCallable, Category = "Ecran")
	void HideScreen();

	UFUNCTION(BlueprintPure, Category = "Ecran")
	bool IsScreenVisible() const { return bWantVisible; }

	/** Le widget affiche (pour le piloter depuis un Blueprint ou le code). */
	UFUNCTION(BlueprintPure, Category = "Ecran")
	UUserWidget* GetContentWidget() const;

	/** Appele par la barre de prise. */
	virtual void BeginDrag() override;
	virtual void EndDrag() override;

	/** Pousse Title / Body / Image dans le widget. */
	void PushTexts() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> Screen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UWidgetComponent> GrabBar;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyLayout();
	void FacePlayer();
	FVector GetPlayerEyes() const;

	bool bWantVisible = true;
	float Alpha = 0.f;

	FScreenDrag Drag;
};
