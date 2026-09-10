#pragma once

#include "CoreMinimal.h"

// Convencion canonica de cube-sphere.
//
// Las seis caras usan UV en [-1, 1]. La direccion superficial es siempre la fuente de
// verdad; face/UV es una representacion para indexar patches. La conversion inversa
// normaliza el punto sobre el cubo, por lo que dos caras comparten exactamente la misma
// direccion en sus bordes (salvo el error numerico de la normalizacion).
enum class EAstraeonPlanetFace : uint8
{
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ
};

struct ASTRAEON_API FAstraeonPlanetFaceUv
{
	EAstraeonPlanetFace Face = EAstraeonPlanetFace::PositiveZ;
	FVector2D Uv = FVector2D::ZeroVector;
	bool bIsValid = false;
};

struct ASTRAEON_API FAstraeonPlanetCoordinates
{
	static constexpr double UvMin = -1.0;
	static constexpr double UvMax = 1.0;
	// Invalid addresses return NaN; never silently clamp to a different location.
	static FVector FaceUvToCube(EAstraeonPlanetFace Face, const FVector2D& Uv);
	static bool TryNormalizeDirection(const FVector& Direction, FVector& OutUnit);

	// Convierte UV de una cara a una direccion unitaria sobre la esfera.
	static FVector FaceUvToDirection(EAstraeonPlanetFace Face, const FVector2D& Uv);

	// Elige la cara dominante y devuelve UV dentro de [-1,1]. La direccion se normaliza
	// primero para que la identidad no dependa de la longitud de la entrada.
	static FAstraeonPlanetFaceUv DirectionToFaceUv(const FVector& Direction);

	static bool IsValidUv(const FVector2D& Uv, double Tolerance = 1.0e-9);
};
