#pragma once

#include "CoreMinimal.h"
#include "Planet/AstraeonPlanetDefinition.h"
#include "WorldGen/AstraeonTerrainTraversal.h"

// A region described in its own flat coordinates (the Region A plan: centre, radius, Itaca,
// start, goals, clearings and mountain keep-outs, in cm). Phase 3 authors regions on the sphere;
// until then this is how the existing plan is placed on a planet and validated there.
struct ASTRAEON_API FAstraeonPlanetRegionPlan
{
	FVector2D CenterCm = FVector2D::ZeroVector;
	double RadiusCm = 0.0;
	FVector2D ItacaOriginCm = FVector2D::ZeroVector;
	FVector2D StartCm = FVector2D::ZeroVector;
	TArray<FAstraeonTraversalGoal> Goals;
	TArray<FVector2D> FlatSpotsCm;
	TArray<FVector2D> MountainKeepOutCm;

	// From the flat materializer: the same goals, clearings and keep-outs the flat build uses.
	static FAstraeonPlanetRegionPlan FromFlatRegion(int32 ContentSeed, const FVector& ItacaOriginCm);
};

// The plan placed on a planet: region centre at `Anchor`, flat +X/+Y along a tangent basis, and
// a flat point maps to the direction at the same arc distance (exponential map).
struct ASTRAEON_API FAstraeonPlanetRegionSurface
{
	FAstraeonPlanetDefinition Planet;
	FAstraeonPlanetRegionPlan Plan;
	FVector Anchor = FVector(0, 0, 1);
	FVector East = FVector(1, 0, 0);
	FVector North = FVector(0, 1, 0);
	TArray<FVector> FlatSpots;
	TArray<FVector> MountainKeepOut;
	FVector ItacaDirection = FVector(0, 0, 1);

	static FAstraeonPlanetRegionSurface Place(const FAstraeonPlanetDefinition& Planet, const FVector& Anchor, const FAstraeonPlanetRegionPlan& Plan);
	FVector ToDirection(const FVector2D& PlanCm) const;
	// Inverse of ToDirection: where a direction falls on the plan. Exact on the region.
	FVector2D ToPlan(const FVector& Direction) const;
	// Takes the plan's axes (+X, +Y, up) to the tangent frame at `Direction`: the plan's axes at
	// the anchor, carried along the great circle. A rigid thing authored on the plan keeps its
	// heading on the sphere.
	FQuat PlanRotationAt(const FVector& Direction) const;
	// The same rule as the flat field: Itaca's rigid footprint is a plateau, the ground is cleared
	// around flat spots, and mountains rise everywhere except near a playable point.
	double HeightCm(const FVector2D& PlanCm) const;
};

struct ASTRAEON_API FAstraeonPlanetRegionResolution
{
	FVector Anchor = FVector(0, 0, 1);
	int32 AttemptsUsed = 0;
	bool bUsedFallback = false;
	TArray<FName> RejectedGoals;
	FAstraeonTraversalReport Report;
};

// Walkability of a region on the sphere, at the resolution of the mesh the player stands on.
// The planet's relief is global and cannot be re-seeded for one region, so what is resolved is
// WHERE the region sits: the first of a few deterministic anchors that is traversable, else a
// documented safe variant.
struct ASTRAEON_API FAstraeonPlanetTraversal
{
	static constexpr int32 MaxAnchorAttempts = 8;
	// Distance between two vertices of the finest patch grid at `Direction`.
	static double MeshSpacingCm(const FAstraeonPlanetDefinition& Planet, const FVector& Direction);
	static FVector CandidateAnchor(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed, int32 Attempt);
	// The safe variant: the first candidate whose whole region has no mountain layer. The ground
	// layer always respects the step limit (`Terrain.Relief`), so it is traversable by
	// construction, and `Terrain.Connectivity` checks it anyway.
	static bool FindSafeAnchor(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed, double RadiusCm, FVector& OutAnchor);
	static FAstraeonTraversalReport Evaluate(const FAstraeonPlanetRegionSurface& Surface);
	static FAstraeonPlanetRegionResolution ResolveRegion(const FAstraeonPlanetDefinition& Planet, int32 ContentSeed,
		const FAstraeonPlanetRegionPlan& Plan);
};
