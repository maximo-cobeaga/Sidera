#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"
#include <limits>

namespace AstraeonPlanetSurfaceLocal
{
	// Version 2 replaced the unshipped discontinuous hash-per-direction prototype.
	// Quintic interpolation on a 3D integer lattice is C2 across cell boundaries.
	//
	// Longitudes de onda portadas del dominio plano sin cambiarlas: describen que es una
	// loma y que es una montana en metros, no en fraccion de planeta.
	constexpr double GroundCoarseCellCm = 46000.0;
	constexpr double GroundFineCellCm = 19000.0;
	constexpr double MountainCellCm = 72000.0;
	// El umbral alto vuelve escasas las montanas: son excepciones en el paisaje.
	constexpr double MountainThreshold = 0.62;

	constexpr uint64 ChannelGroundCoarse = 0x5445525241494eULL;
	constexpr uint64 ChannelGroundFine = 0x44455441494cULL;
	constexpr uint64 ChannelMountain = 0x4d4f554e5441494eULL;

	double Fade(double T) { return T*T*T*(T*(T*6.0-15.0)+10.0); }
	uint64 Mix(uint64 V)
	{
		V = (V ^ (V >> 30)) * 0xbf58476d1ce4e5b9ULL;
		V = (V ^ (V >> 27)) * 0x94d049bb133111ebULL;
		return V ^ (V >> 31);
	}
	double Corner(int32 Seed, int64 X, int64 Y, int64 Z, uint64 Channel)
	{
		uint64 H = Mix(uint64(uint32(Seed)) ^ (uint64(FAstraeonPlanetSurface::GeneratorVersion) << 32) ^ Channel);
		H = Mix(H ^ uint64(X)); H = Mix(H ^ uint64(Y)); H = Mix(H ^ uint64(Z));
		return double(H >> 11) * (1.0 / 9007199254740991.0) * 2.0 - 1.0;
	}
	double Noise(int32 Seed, const FVector& P, uint64 Channel)
	{
		const int64 X = FMath::FloorToInt64(P.X), Y = FMath::FloorToInt64(P.Y), Z = FMath::FloorToInt64(P.Z);
		const double U = Fade(P.X-X), V = Fade(P.Y-Y), W = Fade(P.Z-Z);
		double Plane[2];
		for (int32 K=0; K<2; ++K)
		{
			Plane[K] = FMath::Lerp(
				FMath::Lerp(Corner(Seed,X,Y,Z+K,Channel), Corner(Seed,X+1,Y,Z+K,Channel), U),
				FMath::Lerp(Corner(Seed,X,Y+1,Z+K,Channel), Corner(Seed,X+1,Y+1,Z+K,Channel), U), V);
		}
		return FMath::Lerp(Plane[0], Plane[1], W);
	}
	// El ruido vive en [-1,1] y las dos capas razonan en [0,1], igual que el dominio plano.
	double Unit01(double N) { return FMath::Clamp(0.5*(N+1.0), 0.0, 1.0); }

	double NaN() { return std::numeric_limits<double>::quiet_NaN(); }

	// Devuelve false si la definicion, la version o la direccion no sirven.
	bool Prepare(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, FVector& OutUnit)
	{
		return Planet.IsValid()
			&& Planet.GeneratorVersion == FAstraeonPlanetSurface::GeneratorVersion
			&& FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction, OutUnit);
	}
}

bool FAstraeonPlanetSurface::HasMountainLayer(const FAstraeonPlanetDefinition& Planet)
{
	using namespace AstraeonPlanetSurfaceLocal;
	return Planet.IsValid()
		&& 2.0 * PI * Planet.RadiusCm >= MinMountainCellsAround * MountainCellCm;
}

double FAstraeonPlanetSurface::SampleGroundHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	using namespace AstraeonPlanetSurfaceLocal;
	FVector Unit;
	if (!Prepare(Planet, Direction, Unit)) return NaN();

	const double Coarse = Unit01(Noise(Planet.BodySeed,
		Unit * (Planet.RadiusCm / GroundCoarseCellCm), ChannelGroundCoarse));
	const double Fine = Unit01(Noise(Planet.BodySeed + 7919,
		Unit * (Planet.RadiusCm / GroundFineCellCm), ChannelGroundFine));

	// La octava fina solo matiza: sin esto el suelo queda ondulado y sin lectura.
	const double Combined = FMath::Clamp(Coarse * 0.85 + Fine * 0.15, 0.0, 1.0);

	// El exponente se mantiene bajo porque tambien multiplica la pendiente: subirlo vuelve
	// a generar escalones infranqueables entre muestras vecinas.
	return FMath::Pow(Combined, 1.2) * GroundMaxHeightCm;
}

double FAstraeonPlanetSurface::SampleMountainHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	using namespace AstraeonPlanetSurfaceLocal;
	FVector Unit;
	if (!Prepare(Planet, Direction, Unit)) return NaN();
	if (!HasMountainLayer(Planet)) return 0.0;

	const double Mask = Unit01(Noise(Planet.BodySeed + 31337,
		Unit * (Planet.RadiusCm / MountainCellCm), ChannelMountain));
	if (Mask <= MountainThreshold) return 0.0;

	// Normalizar sobre el umbral y elevar al cuadrado da faldas que nacen suaves desde el
	// suelo y solo se empinan cerca de la cima: se lee como montana y no como meseta.
	const double Above = (Mask - MountainThreshold) / (1.0 - MountainThreshold);
	return Above * Above * MountainMaxHeightCm;
}

double FAstraeonPlanetSurface::SampleRadialHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	const double Ground = SampleGroundHeightCm(Planet, Direction);
	if (!FMath::IsFinite(Ground)) return Ground;
	return Ground + SampleMountainHeightCm(Planet, Direction);
}

double FAstraeonPlanetSurface::ArcDistanceCm(const FAstraeonPlanetDefinition& Planet, const FVector& A, const FVector& B)
{
	FVector UnitA, UnitB;
	if (!FAstraeonPlanetCoordinates::TryNormalizeDirection(A, UnitA)
		|| !FAstraeonPlanetCoordinates::TryNormalizeDirection(B, UnitB))
		return AstraeonPlanetSurfaceLocal::NaN();
	return Planet.RadiusCm * FMath::Acos(FMath::Clamp(FVector::DotProduct(UnitA, UnitB), -1.0, 1.0));
}

bool FAstraeonPlanetSurface::IsWithinAnySpot(const FAstraeonPlanetDefinition& Planet, const FVector& Direction,
	const TArray<FVector>& SpotDirections, double RadiusCm)
{
	for (const FVector& Spot : SpotDirections)
	{
		const double DistanceCm = ArcDistanceCm(Planet, Direction, Spot);
		if (FMath::IsFinite(DistanceCm) && DistanceCm < RadiusCm) return true;
	}
	return false;
}

double FAstraeonPlanetSurface::SampleClearedGroundHeightCm(const FAstraeonPlanetDefinition& Planet,
	const FVector& Direction, const TArray<FVector>& FlatSpotDirections)
{
	double HeightCm = SampleGroundHeightCm(Planet, Direction);
	if (!FMath::IsFinite(HeightCm)) return HeightCm;

	for (const FVector& Spot : FlatSpotDirections)
	{
		const double DistanceCm = ArcDistanceCm(Planet, Direction, Spot);
		if (!FMath::IsFinite(DistanceCm) || DistanceCm >= FlatSpotRadiusCm) continue;

		const double PlateauCm = SampleGroundHeightCm(Planet, Spot);
		if (!FMath::IsFinite(PlateauCm)) continue;
		// Escalar en vez de recortar convierte el borde del claro en una rampa; recortar
		// dejaba el claro en el fondo de un acantilado del alto del terreno vecino.
		const double Blend = FMath::SmoothStep(0.0, FlatSpotRadiusCm, DistanceCm);
		HeightCm = FMath::Lerp(PlateauCm, HeightCm, Blend);
	}
	return HeightCm;
}

FVector FAstraeonPlanetSurface::SampleRadialNormal(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	FVector Unit;
	if (!FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction, Unit)
		|| !FMath::IsFinite(SampleRadialHeightCm(Planet, Direction)))
		return FVector(std::numeric_limits<double>::quiet_NaN());
	// Global axes are only basis candidates; local up is always radial.
	const FVector Reference = FMath::Abs(Unit.Z) < 0.9 ? FVector(0,0,1) : FVector(1,0,0);
	const FVector A = FVector::CrossProduct(Reference, Unit).GetSafeNormal();
	const FVector B = FVector::CrossProduct(Unit, A).GetSafeNormal();
	const double Angle = 1.0 / Planet.RadiusCm; // one centimetre physical difference
	const double Radius = Planet.RadiusCm + SampleRadialHeightCm(Planet, Unit);
	const double DA = (SampleRadialHeightCm(Planet,(Unit+A*Angle).GetSafeNormal())
		- SampleRadialHeightCm(Planet,(Unit-A*Angle).GetSafeNormal())) / (2.0*Angle*Radius);
	const double DB = (SampleRadialHeightCm(Planet,(Unit+B*Angle).GetSafeNormal())
		- SampleRadialHeightCm(Planet,(Unit-B*Angle).GetSafeNormal())) / (2.0*Angle*Radius);
	return (Unit-A*DA-B*DB).GetSafeNormal();
}
