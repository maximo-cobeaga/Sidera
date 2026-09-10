#pragma once

#include "CoreMinimal.h"
#include "Planet/Patches/AstraeonPlanetPatchAddress.h"
#include <atomic>

struct ASTRAEON_API FAstraeonPlanetPatchBuildOptions
{
	int32 Quads = 32; // 33 x 33 surface vertices by default.
	double SkirtDepthCm = 100.0; // Explicit visual coverage; never part of collision.
	bool IsValid() const;
};

enum class EAstraeonPatchBuildStatus : uint8 { Success, InvalidInput, Cancelled };

struct ASTRAEON_API FAstraeonPlanetPatchBuildResult
{
	FAstraeonPlanetPatchAddress Address;
	uint64 BuildRevision = 0;
	uint64 PatchSeed = 0;
	int32 Quads = 0;
	int32 SurfaceVertexCount = 0;
	int32 SurfaceIndexCount = 0;
	FVector OriginBodyCm = FVector::ZeroVector; // double; subtract before render/physics float conversion.
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Indices;
	FBox LocalBounds = FBox(ForceInit);
	bool IsValid() const;
};

// No UObjects, world or global RNG. Safe on workers with copied inputs. A failure clears
// Out entirely, so neither partial geometry nor a previous success can be committed.
struct ASTRAEON_API FAstraeonPlanetPatchMesh
{
	static EAstraeonPatchBuildStatus Build(const FAstraeonPlanetDefinition& Planet,
		const FAstraeonPlanetPatchAddress& Address, uint64 Revision,
		const FAstraeonPlanetPatchBuildOptions& Options, FAstraeonPlanetPatchBuildResult& Out,
		const std::atomic<bool>* Cancelled = nullptr);
};
