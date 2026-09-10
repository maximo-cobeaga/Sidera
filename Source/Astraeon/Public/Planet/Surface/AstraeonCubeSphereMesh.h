#pragma once
#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"

struct ASTRAEON_API FAstraeonCubeSphereMesh
{
	FVector OriginBodyCm = FVector::ZeroVector;
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Indices;
	// Compatibility adapter: a whole face is a Lod-0 patch without skirts. Quads must
	// be a power of two in [4,128]. Origin is subtracted in double before PMC conversion.
	static bool BuildFace(const FAstraeonPlanetDefinition& Planet, EAstraeonPlanetFace Face,
		int32 Quads, FAstraeonCubeSphereMesh& Out);
};
