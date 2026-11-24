#include "Interaction/ZoneGeometry.h"
#include "Algo/Reverse.h"

bool ZoneGeometry::IsInside(const TArray<FVector2D>& Polygon, const FVector2D& P)
{
	// Lancer de rayon : on compte les bords traverses a droite du point.
	bool bInside = false;
	const int32 N = Polygon.Num();
	for (int32 i = 0, j = N - 1; i < N; j = i++)
	{
		const FVector2D& A = Polygon[i];
		const FVector2D& B = Polygon[j];
		if ((A.Y > P.Y) != (B.Y > P.Y))
		{
			const double X = A.X + (P.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y);
			if (P.X < X)
			{
				bInside = !bInside;
			}
		}
	}
	return bInside;
}

float ZoneGeometry::SignedArea(const TArray<FVector2D>& Polygon)
{
	double Area = 0.0;
	const int32 N = Polygon.Num();
	for (int32 i = 0, j = N - 1; i < N; j = i++)
	{
		Area += (double)Polygon[j].X * Polygon[i].Y - (double)Polygon[i].X * Polygon[j].Y;
	}
	return (float)(Area * 0.5);
}

void ZoneGeometry::Clean(TArray<FVector2D>& Polygon)
{
	// Points trop proches
	for (int32 i = Polygon.Num() - 1; i >= 0 && Polygon.Num() > 3; --i)
	{
		const FVector2D& Prev = Polygon[(i + Polygon.Num() - 1) % Polygon.Num()];
		if (FVector2D::DistSquared(Prev, Polygon[i]) < 1.f)
		{
			Polygon.RemoveAt(i);
		}
	}
	// Points alignes (inutiles)
	for (int32 i = Polygon.Num() - 1; i >= 0 && Polygon.Num() > 3; --i)
	{
		const FVector2D& A = Polygon[(i + Polygon.Num() - 1) % Polygon.Num()];
		const FVector2D& B = Polygon[i];
		const FVector2D& C = Polygon[(i + 1) % Polygon.Num()];
		if (FMath::Abs(FVector2D::CrossProduct(B - A, C - B)) < 0.5f)
		{
			Polygon.RemoveAt(i);
		}
	}
	if (SignedArea(Polygon) < 0.f)
	{
		Algo::Reverse(Polygon);
	}
}

namespace
{
	bool IsPointInTriangle(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		const float D1 = FVector2D::CrossProduct(B - A, P - A);
		const float D2 = FVector2D::CrossProduct(C - B, P - B);
		const float D3 = FVector2D::CrossProduct(A - C, P - C);
		return D1 >= 0.f && D2 >= 0.f && D3 >= 0.f;
	}
}

TArray<int32> ZoneGeometry::Triangulate(const TArray<FVector2D>& Polygon)
{
	// "Ear clipping" : on retire une a une les "oreilles" (triangles convexes sans autre point dedans).
	// Le polygone doit etre dans le sens trigonometrique (voir Clean).
	TArray<int32> Result;
	TArray<int32> Remaining;
	for (int32 i = 0; i < Polygon.Num(); ++i)
	{
		Remaining.Add(i);
	}

	int32 Guard = Polygon.Num() * Polygon.Num() + 10;
	while (Remaining.Num() > 3 && Guard-- > 0)
	{
		bool bFound = false;
		for (int32 i = 0; i < Remaining.Num(); ++i)
		{
			const int32 IA = Remaining[(i + Remaining.Num() - 1) % Remaining.Num()];
			const int32 IB = Remaining[i];
			const int32 IC = Remaining[(i + 1) % Remaining.Num()];
			const FVector2D& A = Polygon[IA];
			const FVector2D& B = Polygon[IB];
			const FVector2D& C = Polygon[IC];
			if (FVector2D::CrossProduct(B - A, C - B) <= 0.f)
			{
				continue; // angle rentrant : pas une oreille
			}
			bool bEmpty = true;
			for (const int32 Other : Remaining)
			{
				if (Other != IA && Other != IB && Other != IC && IsPointInTriangle(Polygon[Other], A, B, C))
				{
					bEmpty = false;
					break;
				}
			}
			if (bEmpty)
			{
				Result.Append({ IA, IB, IC });
				Remaining.RemoveAt(i);
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			break; // polygone croise : on s'arrete (le reste n'est pas couvert)
		}
	}
	if (Remaining.Num() == 3)
	{
		Result.Append({ Remaining[0], Remaining[1], Remaining[2] });
	}
	return Result;
}
