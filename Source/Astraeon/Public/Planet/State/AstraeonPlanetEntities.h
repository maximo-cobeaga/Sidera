#pragma once

#include "CoreMinimal.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"

// One entity placed by the planet's definition: same seed, same entity, same place, always.
struct ASTRAEON_API FAstraeonPlanetEntitySpawn
{
	FName EntityId;
	FAstraeonPlanetPatchAddress Cell;
	FVector Direction = FVector::ZeroVector;
};

// Deterministic placement of planetary entities (creatures today) in cells of a FIXED level,
// ~1 km wide. The cell is identity, not representation: unlike a rendered patch it does not
// change with the view, so an entity keeps its id however far the player goes and returns.
//
// The seed comes from the cell through the stable hash, channel `Entities`: nothing depends
// on the order in which cells are loaded, or on whether they were loaded before.
struct ASTRAEON_API FAstraeonPlanetEntities
{
	static constexpr double CellSpanCm = 100000.0;
	// No creature is placed where the mountain layer rises: it could not stand there.
	static constexpr double MaxSpawnMountainCm = 1.0;

	static uint8 CellLod(const FAstraeonPlanetDefinition& Planet);
	static bool CellAt(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, FAstraeonPlanetPatchAddress& Out);
	// Cells intersecting the cap of `RadiusCm` around `Direction`, the containing cell first.
	static bool CellsNear(const FAstraeonPlanetDefinition& Planet, const FVector& Direction, double RadiusCm,
		TArray<FAstraeonPlanetPatchAddress>& Out);
	static bool CreaturesInCell(const FAstraeonPlanetDefinition& Planet, const FAstraeonPlanetPatchAddress& Cell,
		TArray<FAstraeonPlanetEntitySpawn>& Out);
	// Stable across sessions and builds: body, kind, cell and index, in ASCII.
	static FName MakeEntityId(const FAstraeonPlanetPatchAddress& Cell, int32 Index);
};
