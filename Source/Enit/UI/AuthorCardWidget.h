#pragma once

#include "CoreMinimal.h"
#include "UI/ShowroomWidget.h"
#include "UI/InfoPanelWidget.h"
#include "AuthorCardWidget.generated.h"

class UTextBlock;
class UImage;
class UWidget;

/**
 * Carte d'un auteur (chercheur) sur la fiche d'une experience : photo, nom, metier (bleu), petit texte.
 *
 * Mise en page modifiable sans code : Content/Showroom/UI/WBP_AuthorCard
 * (garder les noms PhotoImage, InitialsText, NameText, RoleText, BioText).
 */
UCLASS()
class ENIT_API UAuthorCardWidget : public UShowroomWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Auteur")
	void SetAuthor(const FShowroomAuthor& InAuthor);

	UFUNCTION(BlueprintPure, Category = "Auteur")
	const FShowroomAuthor& GetAuthor() const { return Author; }

	/** Taille de la photo (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	float PhotoSize = 96.f;

protected:
	virtual void BuildDefaultLayout() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Auteur", meta = (BindWidgetOptional))
	TObjectPtr<UImage> PhotoImage;

	/** Initiales affichees quand l'auteur n'a pas de photo. */
	UPROPERTY(BlueprintReadOnly, Category = "Auteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitialsText;

	UPROPERTY(BlueprintReadOnly, Category = "Auteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, Category = "Auteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoleText;

	UPROPERTY(BlueprintReadOnly, Category = "Auteur", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BioText;

private:
	void Refresh();

	UPROPERTY(Transient)
	FShowroomAuthor Author;
};
