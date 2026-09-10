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
	// Pure data builder. Origin is subtracted in double before PMC converts local vertices.
	static bool BuildFace(const FAstraeonPlanetDefinition& Planet, EAstraeonPlanetFace Face,
		int32 Quads, FAstraeonCubeSphereMesh& Out);
};
