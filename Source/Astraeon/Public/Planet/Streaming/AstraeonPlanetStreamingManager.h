#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Planet/Patches/AstraeonPlanetPatchMesh.h"

enum class EAstraeonPatchRequestStatus : uint8 { Accepted, AtCapacity, InvalidInput, RevisionExhausted };

struct ASTRAEON_API FAstraeonPlanetPatchCompletion
{
	FAstraeonPlanetPatchAddress Address;
	uint64 BuildRevision = 0;
	EAstraeonPatchBuildStatus Status = EAstraeonPatchBuildStatus::InvalidInput;
	FAstraeonPlanetPatchBuildResult Mesh;
};

// Game-thread owner, data-only workers. Bounded outstanding tasks AND tracked addresses.
// AtCapacity is backpressure: the future LOD scheduler retries after draining completions.
// Release is mandatory when an address leaves the active set. No UObject is captured by a job.
class ASTRAEON_API FAstraeonPlanetStreamingManager
{
public:
	FAstraeonPlanetStreamingManager() = default;
	FAstraeonPlanetStreamingManager(const FAstraeonPlanetStreamingManager&) = delete;
	FAstraeonPlanetStreamingManager& operator=(const FAstraeonPlanetStreamingManager&) = delete;
	static constexpr int32 MaxOutstandingBuilds = 2;
	static constexpr int32 MaxTrackedPatches = 64;
	~FAstraeonPlanetStreamingManager();
	EAstraeonPatchRequestStatus Request(const FAstraeonPlanetDefinition& Planet,
		const FAstraeonPlanetPatchAddress& Address, const FAstraeonPlanetPatchBuildOptions& Options,
		uint64& OutRevision);
	bool TryTakeCompleted(FAstraeonPlanetPatchCompletion& Out);
	// Check immediately before committing: a result can become obsolete after collection.
	bool IsCurrent(const FAstraeonPlanetPatchAddress& Address, uint64 Revision) const;
	void Release(const FAstraeonPlanetPatchAddress& Address);
	int32 GetOutstandingBuildCount() const { return Jobs.Num(); }
	int32 GetTrackedPatchCount() const { return CurrentRevisions.Num(); }
private:
	struct FJob
	{
		FAstraeonPlanetPatchAddress Address;
		TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> Cancelled;
		TFuture<FAstraeonPlanetPatchCompletion> Future;
	};
	TArray<FJob> Jobs;
	TMap<FAstraeonPlanetPatchAddress, uint64> CurrentRevisions;
	uint64 LastRevision = 0; // Never reset on Release: prevents the unload/reload ABA bug.
};
