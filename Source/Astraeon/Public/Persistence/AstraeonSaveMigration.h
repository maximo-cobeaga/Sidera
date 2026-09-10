#pragma once

#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "Planet/State/AstraeonPlanetStateTypes.h"
#include "WorldGen/AstraeonRegionTypes.h"

class UAstraeonSaveGame;

// Where the flat region of saves v1/v2 lives on a planet. The plane is the tangent plane of a
// fixed anchor on a documented body, and a flat point maps to the direction at the same arc
// distance (exponential map): distances and bearings from the anchor are kept exactly, and the
// inverse recovers the flat point. Flat Z becomes altitude above the body's reference radius.
//
// This is how the flat build keeps loading its saves; the flat region's content is projected
// onto a real planet in Phase 3, not here.
struct ASTRAEON_API FAstraeonLegacyFlatProjection
{
	static FName BodyId();
	// Lab tier, 10 km. The map keeps arc distances from the anchor exactly at any radius.
	static constexpr double RadiusCm = 1000000.0;
	static FAstraeonPlanetDefinition Planet(int32 WorldSeed);
	static void ToPlanet(const FVector& FlatCm, FVector& OutDirection, double& OutAltitudeCm);
	static FVector ToFlat(const FVector& Direction, double AltitudeCm);
	// A flat heading carried to the tangent plane at `Direction`.
	static FVector HeadingToPlanet(const FVector& FlatForward, const FVector& Direction);
	// Flat nest clocks as planetary deltas at each nest's place, in stable id order.
	static void AppendNestDeltas(const FAstraeonRegionLayout& Layout, int32 ContentSeed,
		const TMap<FName, float>& NestClocks, TArray<FAstraeonPlanetDelta>& Out);
};

struct ASTRAEON_API FAstraeonSaveMigration
{
	static constexpr int32 CurrentVersion = 3;
	// Brings a save to the current format in place: v1 gets its legacy profile ids, v1/v2 get
	// planetary location and deltas projected from the flat fields. False for a future version.
	static bool Upgrade(UAstraeonSaveGame& Save);
};
