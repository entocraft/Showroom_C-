#pragma once

#include "CoreMinimal.h"

/** Petits calculs de geometrie pour les zones dessinees (polygones au sol). */
namespace ZoneGeometry
{
	/** Le point P est-il a l'interieur du polygone ? */
	ENIT_API bool IsInside(const TArray<FVector2D>& Polygon, const FVector2D& P);

	/** Aire signee (> 0 = sens trigonometrique). */
	ENIT_API float SignedArea(const TArray<FVector2D>& Polygon);

	/** Decoupe un polygone simple (non croise) en triangles. Renvoie des indices (3 par triangle). */
	ENIT_API TArray<int32> Triangulate(const TArray<FVector2D>& Polygon);

	/** Enleve les points en double / alignes, et oriente le polygone dans le sens trigonometrique. */
	ENIT_API void Clean(TArray<FVector2D>& Polygon);
}
