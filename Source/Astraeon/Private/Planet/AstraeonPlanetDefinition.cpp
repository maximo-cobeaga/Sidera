#include "Planet/AstraeonPlanetDefinition.h"

bool FAstraeonPlanetDefinition::IsValid(FString* OutReason) const
{
	const auto Fail = [OutReason](const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	};

	if (BodyId.IsNone()) return Fail(TEXT("BodyId no puede ser None"));
	if (!FMath::IsFinite(RadiusCm) || RadiusCm < 1000.0 || RadiusCm > 250000000.0)
		return Fail(TEXT("RadiusCm fuera del dominio de ingenieria [10 m, 2500 km]"));
	if (!FMath::IsFinite(MassKg) || MassKg <= 0.0) return Fail(TEXT("MassKg debe ser positivo"));
	if (!FMath::IsFinite(SurfaceGravityMS2) || SurfaceGravityMS2 <= 0.0) return Fail(TEXT("SurfaceGravityMS2 debe ser positivo"));
	if (!FMath::IsFinite(SeaLevelAltitudeCm) || SeaLevelAltitudeCm <= -RadiusCm) return Fail(TEXT("SeaLevelAltitudeCm fuera de rango"));
	if (GeneratorVersion <= 0) return Fail(TEXT("GeneratorVersion debe ser positivo"));

	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}
