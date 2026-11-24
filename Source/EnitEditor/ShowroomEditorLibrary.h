#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShowroomEditorLibrary.generated.h"

class UUserWidget;

/** Outils d'editeur du showroom (utilises par les scripts Python du dossier Scripts). */
UCLASS()
class UShowroomEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Cree un Widget Blueprint (enfant de ParentClass, une classe ShowroomWidget) dont l'arbre de widgets
	 * est la mise en page par defaut de cette classe : on peut ensuite tout modifier dans l'editeur UMG.
	 * Si l'asset existe deja, il n'est PAS modifie (sauf bOverwrite = true : sa mise en page est refaite).
	 */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Editeur")
	static UObject* CreateWidgetBlueprintFromLayout(const FString& PackagePath, const FString& AssetName,
		TSubclassOf<UUserWidget> ParentClass, bool bOverwrite = false);

	/**
	 * APERCU : dessine une interface dans une image PNG (sans lancer le jeu), avec les vraies donnees du niveau
	 * ouvert dans l'editeur (zones d'experience). Utilise par Scripts/RebuildUI.py.
	 *  - Page : pour l'ecran d'accueil (ExperienceSelectorWidget), la page a afficher
	 *    (0 Experiences, 1 fiche de la 1re experience, 2 Notre projet, 3 Aide, 4 Parametres) ;
	 *    pour la fiche des zones (InfoPanelWidget), la 1re zone d'experience est affichee.
	 */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Editeur")
	static bool RenderWidgetPreview(TSubclassOf<UUserWidget> WidgetClass, FVector2D Size, const FString& FilePath, int32 Page = 0);

	/** Liste "Nom (Type)" des widgets d'un Widget Blueprint (verification par script). */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Editeur")
	static TArray<FString> DescribeWidgetBlueprint(UObject* WidgetBlueprint);
};
