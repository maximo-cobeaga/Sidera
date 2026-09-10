#include "Planet/Collision/AstraeonPlanetCollisionRing.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"

bool FAstraeonPlanetCollisionRing::Select(const FAstraeonPlanetDefinition& Planet, uint8 Lod, const FVector& Direction,
	double CapRadiusCm, TArray<FAstraeonPlanetPatchAddress>& Out)
{
	Out.Reset();
	FVector Unit;
	if (!Planet.IsValid() || !FMath::IsFinite(CapRadiusCm) || CapRadiusCm < 0.0 || CapRadiusCm > Planet.RadiusCm
		|| !FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction, Unit)) return false;
	FAstraeonPlanetPatchAddress Center;
	if (!FAstraeonPlanetPatchAddress::TryFromDirection(Planet.BodyId, Unit, Lod, Center)) return false;

	// Concentric sample circles far closer than a patch is wide, so no patch fits between them.
	// The outer circle is four times denser: what a patch corner can still hide between two of
	// its samples is a sliver a few centimetres deep, well inside the keep margin.
	const double PatchSpanCm = 0.5 * PI * Planet.RadiusCm / double(int64(1) << Lod);
	const double StepCm = FMath::Max(1.0, FMath::Min(PatchSpanCm * 0.125, CapRadiusCm * 0.25));
	const FVector Reference = FMath::Abs(Unit.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0);
	const FVector A = FVector::CrossProduct(Reference, Unit).GetSafeNormal();
	const FVector B = FVector::CrossProduct(Unit, A);
	TSet<FAstraeonPlanetPatchAddress> Found;
	Found.Add(Center);
	for (double Ring = StepCm; Ring < CapRadiusCm + StepCm; Ring += StepCm)
	{
		const bool bOuter = Ring >= CapRadiusCm;
		const double Arc = FMath::Min(Ring, CapRadiusCm) / Planet.RadiusCm;
		const int32 Samples = FMath::Max(8, FMath::CeilToInt32(2.0 * PI * FMath::Min(Ring, CapRadiusCm) / StepCm * (bOuter ? 4.0 : 1.0)));
		for (int32 I = 0; I < Samples; ++I)
		{
			const double Angle = 2.0 * PI * I / Samples;
			const FVector Tangent = A * FMath::Cos(Angle) + B * FMath::Sin(Angle);
			FAstraeonPlanetPatchAddress Address;
			if (FAstraeonPlanetPatchAddress::TryFromDirection(Planet.BodyId, Unit * FMath::Cos(Arc) + Tangent * FMath::Sin(Arc), Lod, Address))
				Found.Add(Address);
		}
	}
	Found.Remove(Center);
	Out = Found.Array();
	Out.Sort(FAstraeonPlanetLODManager::Less);
	Out.Insert(Center, 0);
	return true;
}
