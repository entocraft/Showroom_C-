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

	/** Liste "Nom (Type)" des widgets d'un Widget Blueprint (verification par script). */
	UFUNCTION(BlueprintCallable, Category = "Showroom|Editeur")
	static TArray<FString> DescribeWidgetBlueprint(UObject* WidgetBlueprint);
};
