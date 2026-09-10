#pragma once

#include "CoreMinimal.h"

// Hechos reproducibles de un cuerpo. No es Data Asset en las Fases 1-2: la estructura aun
// esta evolucionando y debe poder probarse sin cargar contenido binario. La representacion
// visual y el estado mutable se mantienen fuera de esta definicion.
struct ASTRAEON_API FAstraeonPlanetDefinition
{
	FName BodyId = NAME_None;
	double RadiusCm = 0.0;
	double MassKg = 0.0;
	double SurfaceGravityMS2 = 0.0;
	double SeaLevelAltitudeCm = 0.0;
	int32 WorldSeed = 0;
	int32 BodySeed = 0;
	int32 GeneratorVersion = 2;

	bool IsValid(FString* OutReason = nullptr) const;
};
