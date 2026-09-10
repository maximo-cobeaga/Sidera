#pragma once

#include "CoreMinimal.h"

// Lo que una region authored le hace al relieve de su cuerpo: la meseta rigida bajo Itaca, los
// claros del suelo y las exclusiones de montana. Viaja con la definicion del planeta, asi la malla,
// la colision, el validador de transito y cualquier consulta de altura leen la misma funcion
// (`FAstraeonPlanetSurface::SampleRadialHeightCm`). Es solo geometria: las alturas salen siempre
// del ruido del cuerpo.
struct ASTRAEON_API FAstraeonPlanetRegionRelief
{
	// Itaca's frame: the hull's +X and +Y on the tangent plane at its origin. They are the region
	// plan's axes carried there, the same frame the hull is placed with.
	FVector ItacaDirection = FVector(0, 0, 1);
	FVector ItacaAxisX = FVector(1, 0, 0);
	FVector ItacaAxisY = FVector(0, 1, 0);
	TArray<FVector> FlatSpots;
	TArray<FVector> MountainKeepOut;
	// Outside this cone nothing is edited: the height is the body's own, bit for bit.
	FVector Center = FVector(0, 0, 1);
	double InfluenceCosine = 1.0;

	// Where a unit direction falls on Itaca's footprint, in cm along the hull's axes.
	FVector2D ToItacaLocal(double PlanetRadiusCm, const FVector& Unit) const;
	// Closes the influence cone around `Center` once Itaca and the spots are set.
	void FinishInfluence(double PlanetRadiusCm);
};
