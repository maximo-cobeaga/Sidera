#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"
#include <limits>

FVector FAstraeonPlanetCoordinates::FaceUvToCube(EAstraeonPlanetFace Face, const FVector2D& Uv)
{
	if (!IsValidUv(Uv, 0.0)) return FVector(std::numeric_limits<double>::quiet_NaN());
	const double U = Uv.X;
	const double V = Uv.Y;
	FVector Cube;

	switch (Face)
	{
	case EAstraeonPlanetFace::PositiveX: Cube = FVector(1.0, U, V); break;
	case EAstraeonPlanetFace::NegativeX: Cube = FVector(-1.0, -U, V); break;
	case EAstraeonPlanetFace::PositiveY: Cube = FVector(-U, 1.0, V); break;
	case EAstraeonPlanetFace::NegativeY: Cube = FVector(U, -1.0, V); break;
	case EAstraeonPlanetFace::PositiveZ: Cube = FVector(U, V, 1.0); break;
	case EAstraeonPlanetFace::NegativeZ: Cube = FVector(U, -V, -1.0); break;
	default: return FVector(std::numeric_limits<double>::quiet_NaN());
	}

	return Cube;
}

bool FAstraeonPlanetCoordinates::TryNormalizeDirection(const FVector& Direction, FVector& OutUnit)
{
	if (Direction.ContainsNaN()) return false;
	const double Scale = FMath::Max3(FMath::Abs(Direction.X), FMath::Abs(Direction.Y), FMath::Abs(Direction.Z));
	if (Scale <= 0.0) return false;
	OutUnit = (Direction / Scale).GetSafeNormal();
	return !OutUnit.ContainsNaN() && !OutUnit.IsNearlyZero();
}

FVector FAstraeonPlanetCoordinates::FaceUvToDirection(EAstraeonPlanetFace Face, const FVector2D& Uv)
{
	const FVector Cube = FaceUvToCube(Face, Uv);
	return Cube.ContainsNaN() ? Cube : Cube.GetSafeNormal();
}

FAstraeonPlanetFaceUv FAstraeonPlanetCoordinates::DirectionToFaceUv(const FVector& Direction)
{
	FVector Unit;
	if (!TryNormalizeDirection(Direction, Unit))
	{
		return {};
	}

	const double Ax = FMath::Abs(Unit.X);
	const double Ay = FMath::Abs(Unit.Y);
	const double Az = FMath::Abs(Unit.Z);
	FAstraeonPlanetFaceUv Result;
	if (Ax >= Ay && Ax >= Az)
	{
		if (Unit.X >= 0.0)
		{
			Result.Face = EAstraeonPlanetFace::PositiveX;
			Result.Uv = FVector2D(Unit.Y / Ax, Unit.Z / Ax);
		}
		else
		{
			Result.Face = EAstraeonPlanetFace::NegativeX;
			Result.Uv = FVector2D(-Unit.Y / Ax, Unit.Z / Ax);
		}
	}
	else if (Ay >= Az)
	{
		if (Unit.Y >= 0.0)
		{
			Result.Face = EAstraeonPlanetFace::PositiveY;
			Result.Uv = FVector2D(-Unit.X / Ay, Unit.Z / Ay);
		}
		else
		{
			Result.Face = EAstraeonPlanetFace::NegativeY;
			Result.Uv = FVector2D(Unit.X / Ay, Unit.Z / Ay);
		}
	}
	else if (Unit.Z >= 0.0)
	{
		Result.Face = EAstraeonPlanetFace::PositiveZ;
		Result.Uv = FVector2D(Unit.X / Az, Unit.Y / Az);
	}
	else
	{
		Result.Face = EAstraeonPlanetFace::NegativeZ;
		Result.Uv = FVector2D(Unit.X / Az, -Unit.Y / Az);
	}

	Result.bIsValid = true;
	return Result;
}

bool FAstraeonPlanetCoordinates::IsValidUv(const FVector2D& Uv, double Tolerance)
{
	return FMath::IsFinite(Uv.X) && FMath::IsFinite(Uv.Y) && FMath::IsFinite(Tolerance) && Tolerance >= 0.0
		&& Uv.X >= UvMin - Tolerance && Uv.X <= UvMax + Tolerance
		&& Uv.Y >= UvMin - Tolerance && Uv.Y <= UvMax + Tolerance;
}
