#pragma once
#include "CoreMinimal.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"

enum class EAstraeonPatchEdge : uint8 { Left, Right, Bottom, Top };

struct ASTRAEON_API FAstraeonPlanetLODView
{
	FVector ObserverBodyCm = FVector::ZeroVector;
	FVector VelocityBodyCmS = FVector::ZeroVector;
	double VerticalFovDegrees = 60.0;
	int32 ViewHeightPixels = 1080;
};

struct ASTRAEON_API FAstraeonPlanetLODSettings
{
	static constexpr int32 MaxAllowedPatches = 512;
	int32 MaxPatches = 384;
	int32 Quads = 32;
	double MinCellSpanCm = 700.0;
	double MaxErrorPixels = 4.0;
	double PredictionSeconds = 0.5;
};

struct ASTRAEON_API FAstraeonPlanetLODSelection
{
	TArray<FAstraeonPlanetPatchAddress> Leaves;
	bool bBudgetLimited = false;
	uint8 FinestAllowedLod = 0;
};

// Pure, deterministic quadtree selection. The geometric error is a conservative sphere
// sagitta plus a documented relief heuristic, not a claim of a certified heightfield error.
struct ASTRAEON_API FAstraeonPlanetLODManager
{
	static bool Select(const FAstraeonPlanetDefinition& Planet, const FAstraeonPlanetLODView& View,
		const FAstraeonPlanetLODSettings& Settings, FAstraeonPlanetLODSelection& Out);
	// First level whose cell is no wider than MinCellSpanCm. Depends on the radius, not the view.
	static uint8 FinestAllowedLod(const FAstraeonPlanetDefinition& Planet, const FAstraeonPlanetLODSettings& Settings);
	static bool SameLevelNeighbor(const FAstraeonPlanetPatchAddress& Address, EAstraeonPatchEdge Edge,
		FAstraeonPlanetPatchAddress& Out);
	// Partition: every face fully covered, no overlap. Cover: partition plus LOD delta <= 1.
	// A visible set mid-relay only guarantees the partition.
	static bool ValidatePartition(const TArray<FAstraeonPlanetPatchAddress>& Leaves, FString* Reason = nullptr);
	static bool ValidateCover(const TArray<FAstraeonPlanetPatchAddress>& Leaves, FString* Reason = nullptr);
	static bool Less(const FAstraeonPlanetPatchAddress& A, const FAstraeonPlanetPatchAddress& B);
	static double SkirtDepthCm(const FAstraeonPlanetDefinition& Planet,
		const FAstraeonPlanetPatchAddress& Address, int32 Quads);
};
