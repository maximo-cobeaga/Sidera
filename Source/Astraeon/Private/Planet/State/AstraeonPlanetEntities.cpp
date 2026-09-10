#include "Planet/State/AstraeonPlanetEntities.h"
#include "Planet/Collision/AstraeonPlanetCollisionRing.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"

namespace
{
	// SplitMix64 over the stable cell seed: an explicit stream, never the global RNG.
	uint64 Next(uint64& State)
	{
		uint64 Z = (State += 0x9e3779b97f4a7c15ULL);
		Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebULL;
		return Z ^ (Z >> 31);
	}
	double Unit(uint64& State) { return double(Next(State) >> 11) * (1.0 / 9007199254740992.0); }
}

uint8 FAstraeonPlanetEntities::CellLod(const FAstraeonPlanetDefinition& Planet)
{
	uint8 Lod = 0;
	while (Lod < FAstraeonPlanetPatchAddress::MaxLod && 0.5 * PI * Planet.RadiusCm / double(int64(1) << Lod) > CellSpanCm) ++Lod;
	return Lod;
}

bool FAstraeonPlanetEntities::CellAt(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, FAstraeonPlanetPatchAddress& Out)
{
	return Planet.IsValid() && FAstraeonPlanetPatchAddress::TryFromDirection(Planet.BodyId, Direction, CellLod(Planet), Out);
}

bool FAstraeonPlanetEntities::CellsNear(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, double RadiusCm,
	TArray<FAstraeonPlanetPatchAddress>& Out)
{
	// On a 200 m bench the whole planet is closer than the radius: the cap is capped by it.
	return FAstraeonPlanetCollisionRing::Select(Planet, CellLod(Planet), Direction, FMath::Min(RadiusCm, Planet.RadiusCm), Out);
}

FName FAstraeonPlanetEntities::MakeEntityId(const FAstraeonPlanetPatchAddress& Cell, int32 Index)
{
	return FName(*FString::Printf(TEXT("creature.%s.%d.%d.%d.%d.%d"), *Cell.BodyId.ToString().ToLower(),
		int32(Cell.Face), int32(Cell.Lod), Cell.X, Cell.Y, Index));
}

bool FAstraeonPlanetEntities::CreaturesInCell(const FAstraeonPlanetDefinition& Planet, const FAstraeonPlanetPatchAddress& Cell,
	TArray<FAstraeonPlanetEntitySpawn>& Out)
{
	Out.Reset();
	uint64 Seed = 0;
	FVector2D Min, Max;
	if (!Planet.IsValid() || Cell.Lod != CellLod(Planet) || Cell.BodyId != Planet.BodyId
		|| !Cell.TryDeriveSeed(Planet, EAstraeonGenerationChannel::Entities, Seed) || !Cell.TryUvBounds(Min, Max)) return false;
	// Most cells hold one animal, some two, some none: a grazer herd reads as sparse life.
	const double Roll = Unit(Seed);
	const int32 Count = Roll < 0.55 ? 1 : Roll < 0.75 ? 2 : 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// The index keeps the id even when a candidate is rejected: ids never shift.
		const FVector2D Uv(FMath::Lerp(Min.X, Max.X, 0.1 + 0.8 * Unit(Seed)), FMath::Lerp(Min.Y, Max.Y, 0.1 + 0.8 * Unit(Seed)));
		const FVector Direction = FAstraeonPlanetCoordinates::FaceUvToDirection(Cell.Face, Uv);
		if (FAstraeonPlanetSurface::SampleMountainHeightCm(Planet, Direction) > MaxSpawnMountainCm) continue;
		Out.Add({MakeEntityId(Cell, Index), Cell, Direction});
	}
	return true;
}
