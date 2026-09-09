#include "WorldGen/AstraeonRegionASurface.h"

namespace AstraeonRegionA
{
	constexpr float HalfExtentCm = 25000.0f;
	constexpr float CorridorHalfWidthCm = 150.0f;
	const TArray<FVector2D> Direct = { {0,0}, {7000,-3500}, {14500,7000}, {18500,10500}, {22000,14500} };
	const TArray<FVector2D> Safe = { {0,0}, {7000,-3500}, {19000,-8500}, {20500,2000}, {22000,14500} };

	float DistanceToSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Delta = B - A;
		const float LengthSquared = Delta.SizeSquared();
		const float T = LengthSquared > 0.0f ? FMath::Clamp(FVector2D::DotProduct(Point - A, Delta) / LengthSquared, 0.0f, 1.0f) : 0.0f;
		return FVector2D::Distance(Point, A + Delta * T);
	}

	float CorridorDistance(const FVector2D& Point, const TArray<FVector2D>& Route)
	{
		float Result = TNumericLimits<float>::Max();
		for (int32 Index = 1; Index < Route.Num(); ++Index) Result = FMath::Min(Result, DistanceToSegment(Point, Route[Index - 1], Route[Index]));
		return Result;
	}

	float Height(const FVector2D& Point)
	{
		// Cuenca suave (0-2.4 m) y cresta noreste fija. La señal queda elevada y visible.
		const float Basin = 120.0f + 0.00008f * (Point.X * Point.X + Point.Y * Point.Y);
		const FVector2D RidgeCenter(18500.0f, 12000.0f);
		const float Ridge = 1150.0f * FMath::Exp(-FVector2D::DistSquared(Point, RidgeCenter) / FMath::Square(6500.0f));
		float Result = Basin + Ridge;
		const float Corridor = FMath::Min(CorridorDistance(Point, Direct), CorridorDistance(Point, Safe));
		// Una franja de 3 m plana y una transición exterior de 6 m mantienen el paso legible.
		if (Corridor < CorridorHalfWidthCm + 600.0f)
		{
			const float Blend = FMath::SmoothStep(CorridorHalfWidthCm, CorridorHalfWidthCm + 600.0f, Corridor);
			// El corredor sube de forma controlada hacia la señal: no aplana la cresta ni
			// convierte el landmark final en una extensión indistinguible de la landing.
			const float CorridorHeight = 120.0f + FMath::Clamp(Point.X, 0.0f, 22000.0f) * 0.02f;
			Result = FMath::Lerp(CorridorHeight, Result, Blend);
		}
		return Result;
	}
}

FAstraeonTerrainSurfaceSample FAstraeonRegionASurface::Sample(const FVector2D& PointCm)
{
	FAstraeonTerrainSurfaceSample Result;
	if (FMath::Abs(PointCm.X) > AstraeonRegionA::HalfExtentCm || FMath::Abs(PointCm.Y) > AstraeonRegionA::HalfExtentCm) return Result;
	Result.HeightCm = AstraeonRegionA::Height(PointCm);
	constexpr float OffsetCm = 100.0f;
	const float Left = AstraeonRegionA::Height(PointCm - FVector2D(OffsetCm, 0.0f));
	const float Right = AstraeonRegionA::Height(PointCm + FVector2D(OffsetCm, 0.0f));
	const float Down = AstraeonRegionA::Height(PointCm - FVector2D(0.0f, OffsetCm));
	const float Up = AstraeonRegionA::Height(PointCm + FVector2D(0.0f, OffsetCm));
	Result.Normal = FVector(-(Right - Left) / (2.0f * OffsetCm), -(Up - Down) / (2.0f * OffsetCm), 1.0f).GetSafeNormal();
	Result.bIsValid = true;
	return Result;
}
