#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Coordinates/AstraeonPlanetCoordinates.h"
#include <limits>

namespace AstraeonPlanetSurfaceLocal
{
	// Version 2 replaces the unshipped discontinuous hash-per-direction prototype.
	// Quintic interpolation on a 3D integer lattice is C2 across cell boundaries.
	constexpr double WavelengthCm = 10000.0;
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
}

double FAstraeonPlanetSurface::SampleRadialHeightCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction)
{
	FVector Unit;
	if (!Planet.IsValid() || Planet.GeneratorVersion != GeneratorVersion
		|| !FAstraeonPlanetCoordinates::TryNormalizeDirection(Direction, Unit))
		return std::numeric_limits<double>::quiet_NaN();
	const FVector P = Unit * (Planet.RadiusCm / AstraeonPlanetSurfaceLocal::WavelengthCm);
	return MaxReliefCm * (0.75 * AstraeonPlanetSurfaceLocal::Noise(Planet.BodySeed, P, 0x5445525241494eULL)
		+ 0.25 * AstraeonPlanetSurfaceLocal::Noise(Planet.BodySeed, P*2.0, 0x44455441494cULL));
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
