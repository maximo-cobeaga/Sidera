#pragma once

#include "CoreMinimal.h"
#include "WorldGen/AstraeonTerrainField.h"

// Superficie authored de la Cuenca de la Primera Señal. No acepta seed: la seed de contenido
// no puede desplazar geografía, corredores ni POIs narrativos.
class ASTRAEON_API FAstraeonRegionASurface
{
public:
	static FAstraeonTerrainSurfaceSample Sample(const FVector2D& PointCm);
};
