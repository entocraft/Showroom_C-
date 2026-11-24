#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigatorSystem.generated.h"

class ANavigatorPoint;
class ULineBatchComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNavigatorPointReached, ANavigatorPoint*, Point, int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNavigatorRouteFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNavigatorGuideSignature, AActor*, Target);

/**
 * Remplace BP_NavigatorSystem (le "GPS").
 *
 * Dessine au sol le chemin (calcule sur le NavMesh) entre le joueur et l'etape en cours.
 * Quand le joueur arrive a l'etape, on passe automatiquement a la suivante.
 *
 * Il peut aussi guider vers UNE destination precise (GuideTo) : par exemple la zone d'experience
 * choisie sur un ecran "Choisir une experience". Cette destination passe avant le parcours d'etapes.
 *
 * Il faut un NavMeshBoundsVolume dans le niveau.
 * Les reglages (couleur, epaisseur, distance...) sont dans Project Settings > Game > Showroom.
 */
UCLASS()
class ENIT_API ANavigatorSystem : public AActor
{
	GENERATED_BODY()

public:
	ANavigatorSystem();

	/** Les etapes dans l'ordre. Si vide (et bAutoCollectPoints), tous les NavigatorPoint du niveau tries par "Order". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	TArray<TObjectPtr<ANavigatorPoint>> Points;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	bool bAutoCollectPoints = true;

	/** Le GPS demarre tout seul au lancement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GPS")
	bool bStartOnBeginPlay = true;

	UPROPERTY(BlueprintAssignable, Category = "GPS")
	FNavigatorPointReached OnPointReached;

	UPROPERTY(BlueprintAssignable, Category = "GPS")
	FNavigatorRouteFinished OnRouteFinished;

	UFUNCTION(BlueprintCallable, Category = "GPS")
	void StartGuidance();

	UFUNCTION(BlueprintCallable, Category = "GPS")
	void StopGuidance();

	/** Aller directement a une etape. */
	UFUNCTION(BlueprintCallable, Category = "GPS")
	void SetCurrentIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "GPS")
	void NextPoint();

	UFUNCTION(BlueprintPure, Category = "GPS")
	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintPure, Category = "GPS")
	ANavigatorPoint* GetCurrentPoint() const;

	/**
	 * Guide le joueur vers un acteur (ex : une zone d'experience). Le trait au sol mene a lui ;
	 * pour une zone, il est "atteint" quand le joueur entre dedans.
	 */
	UFUNCTION(BlueprintCallable, Category = "GPS")
	void GuideTo(AActor* Target);

	/** Arrete le guidage vers la destination choisie (le parcours d'etapes reprend s'il tournait). */
	UFUNCTION(BlueprintCallable, Category = "GPS")
	void StopGuideTo();

	UFUNCTION(BlueprintPure, Category = "GPS")
	AActor* GetGuideTarget() const { return GuideTarget.Get(); }

	/** La destination choisie est atteinte. */
	UPROPERTY(BlueprintAssignable, Category = "GPS")
	FNavigatorGuideSignature OnGuideArrived;

	/** Le guidage vers une destination a ete arrete (arrivee, annulation, autre destination). */
	UPROPERTY(BlueprintAssignable, Category = "GPS")
	FNavigatorGuideSignature OnGuideStopped;

	/** Le GPS du niveau (en cree un s'il n'y en a pas). */
	static ANavigatorSystem* FindOrSpawn(const UObject* WorldContext);

protected:
	virtual void BeginPlay() override;

	/** Appele en boucle par un timer : recalcule et redessine le chemin. */
	void RecalcPath();
	void DrawPath(const TArray<FVector>& PathPoints);
	void ClearPath();

	UPROPERTY(VisibleAnywhere, Category = "Composants")
	TObjectPtr<ULineBatchComponent> PathLines;

private:
	/** Guide vers la destination choisie (appele par RecalcPath). */
	void UpdateGuideTo(const FVector& PlayerPos);

	TWeakObjectPtr<AActor> GuideTarget;
	bool bRouteRunning = false;   // le parcours d'etapes tournait-il avant GuideTo ?

	int32 CurrentIndex = 0;
	FVector LastPos = FVector(TNumericLimits<float>::Max());
	FTimerHandle RecalcTimer;
};
