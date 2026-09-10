#pragma once

#include "CoreMinimal.h"

struct FAstraeonPlanetRegionRelief;

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
	// Sigue a `FAstraeonPlanetSurface::GeneratorVersion`, que no se puede incluir aqui sin
	// ciclo. La 3 separo suelo caminable y montanas; la 2 era una sola capa de +-180 cm.
	int32 GeneratorVersion = 3;
	// El lugar fijo de una region authored tambien es un hecho del cuerpo: su meseta y sus
	// exclusiones forman parte del relieve que todos consultan. Inmutable y compartido, asi la
	// copia que recibe cada worker cuesta un contador.
	TSharedPtr<const FAstraeonPlanetRegionRelief> RegionRelief;

	bool IsValid(FString* OutReason = nullptr) const;
};
