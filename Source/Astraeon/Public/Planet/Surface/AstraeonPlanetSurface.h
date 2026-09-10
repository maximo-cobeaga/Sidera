#pragma once

#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"

// Consulta pura de la superficie de referencia. La entrada geometrica es una direccion
// planetaria global, no una UV local: dos caras que comparten borde reciben el mismo valor.
struct ASTRAEON_API FAstraeonPlanetSurface
{
	static constexpr int32 GeneratorVersion = 2;
	static constexpr double MaxReliefCm = 180.0;
	// Invalid definition/direction or unsupported version returns NaN, never flat ground.
	static double SampleRadialHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
	static FVector SampleRadialNormal(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
};
