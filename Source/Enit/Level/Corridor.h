#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Corridor.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EWallLayout : uint8
{
	/** Un sol + un mur de chaque cote, le long de la ligne. */
	Corridor UMETA(DisplayName = "Couloir (sol + 2 murs)"),
	/** Un seul mur, pose SUR la ligne (pour fermer une piece, separer deux espaces...). */
	Wall     UMETA(DisplayName = "Mur (sur la ligne)"),
};

/**
 * COULOIR / MURS modulaires, dessines avec une ligne (spline).
 *
 * A glisser dans le niveau ("Couloir / Murs"), puis selectionner le composant "Path" et deplacer ses points ;
 * Alt + glisser un point en cree un nouveau. Points "Curve" = couloir arrondi, "Linear" = angles droits.
 *   - Layout = Couloir : sol + 2 murs (largeur Width), avec des lignes lumineuses au bord du sol.
 *   - Layout = Mur     : un mur sur la ligne (ex : les murs d'une alcove).
 *   - Openings : des passages dans les murs (losanges deplacables dans le niveau), largeur OpeningWidth.
 * Les murs bloquent le joueur et le GPS (NavMesh).
 */
UCLASS(meta = (DisplayName = "Couloir / Murs"))
class ENIT_API ACorridor : public AActor
{
	GENERATED_BODY()

public:
	ACorridor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir")
	EWallLayout Layout = EWallLayout::Corridor;

	/** Largeur du passage (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (ClampMin = "50", EditCondition = "Layout == EWallLayout::Corridor", EditConditionHides))
	float Width = 320.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (ClampMin = "10"))
	float WallHeight = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (ClampMin = "2"))
	float WallThickness = 15.f;

	/** Mur a gauche / a droite (en suivant la ligne dans le sens des points). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (EditCondition = "Layout == EWallLayout::Corridor", EditConditionHides))
	bool bLeftWall = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (EditCondition = "Layout == EWallLayout::Corridor", EditConditionHides))
	bool bRightWall = true;

	/** Sol du couloir (une bande posee 1 cm au-dessus du sol existant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (EditCondition = "Layout == EWallLayout::Corridor", EditConditionHides))
	bool bFloor = true;

	/** Lignes lumineuses le long des bords du sol (visibles en jeu). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (EditCondition = "Layout == EWallLayout::Corridor", EditConditionHides))
	bool bEdgeLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (EditCondition = "bEdgeLights"))
	FLinearColor EdgeLightColor = FLinearColor(0.3f, 0.6f, 1.f, 1.f);

	/** Passages dans les murs : chaque losange retire le mur sur OpeningWidth autour de lui. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (MakeEditWidget = true))
	TArray<FVector> Openings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (ClampMin = "50"))
	float OpeningWidth = 300.f;

	/** Longueur des morceaux de mur (plus petit = courbes plus lisses). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Couloir", meta = (ClampMin = "10"), AdvancedDisplay)
	float SegmentLength = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	TObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apparence")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	/** Reconstruit le sol et les murs (fait automatiquement quand on modifie la ligne ou un reglage). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Couloir")
	void Rebuild();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<USplineComponent> Path;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	TObjectPtr<UProceduralMeshComponent> FloorMesh;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	bool IsInOpening(const FVector& WorldPoint) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EdgeMaterial;
};
