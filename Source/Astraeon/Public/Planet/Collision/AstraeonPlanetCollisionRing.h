#pragma once

#include "CoreMinimal.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"

// Which finest-level patches must carry collision around a point of the surface. Pure data:
// the runtime decides when to build them; tests check the cover without a world.
//
// Collision is built from the same builder as the rendered finest patches, minus skirts, so
// what the player stands on is what the player sees wherever the finest level is on screen.
struct ASTRAEON_API FAstraeonPlanetCollisionRing
{
	// Must have collision: generous for a sprint of several seconds, so a worker has built a
	// patch long before the player can reach it. The Phase 1 near patch covered ~37 m.
	static constexpr double RadiusCm = 4000.0;
	// Stays until this far: walking along a patch boundary must not churn its collision.
	static constexpr double KeepRadiusCm = 8000.0;

	// Every patch at `Lod` that intersects the cap of `CapRadiusCm` around `Direction`, sorted.
	// The first entry is always the patch that contains `Direction` itself.
	static bool Select(const FAstraeonPlanetDefinition& Planet, uint8 Lod, const FVector& Direction,
		double CapRadiusCm, TArray<FAstraeonPlanetPatchAddress>& Out);
};
