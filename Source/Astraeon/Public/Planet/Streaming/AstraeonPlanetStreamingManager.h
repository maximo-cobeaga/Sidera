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

// Seam between the patch scheduler and whoever builds meshes. Tests substitute a
// deterministic queue; production uses the worker-backed streaming manager below.
class ASTRAEON_API IAstraeonPlanetPatchBuildQueue
{
public:
	virtual ~IAstraeonPlanetPatchBuildQueue() = default;
	virtual EAstraeonPatchRequestStatus Request(const FAstraeonPlanetDefinition& Planet,
		const FAstraeonPlanetPatchAddress& Address, const FAstraeonPlanetPatchBuildOptions& Options,
		uint64& OutRevision) = 0;
	virtual bool TryTakeCompleted(FAstraeonPlanetPatchCompletion& Out) = 0;
	// Check immediately before committing: a result can become obsolete after collection.
	virtual bool IsCurrent(const FAstraeonPlanetPatchAddress& Address, uint64 Revision) const = 0;
	virtual void Release(const FAstraeonPlanetPatchAddress& Address) = 0;
};

// Game-thread owner, data-only workers. Bounded outstanding tasks AND tracked addresses.
// AtCapacity is backpressure: the scheduler retries after draining completions.
// Tracked = requested and not yet released. The patch manager releases each address as soon
// as it collects its result, so this registry holds pending work, not the committed set.
// No UObject is captured by a job.
class ASTRAEON_API FAstraeonPlanetStreamingManager : public IAstraeonPlanetPatchBuildQueue
{
public:
	FAstraeonPlanetStreamingManager() = default;
	FAstraeonPlanetStreamingManager(const FAstraeonPlanetStreamingManager&) = delete;
	FAstraeonPlanetStreamingManager& operator=(const FAstraeonPlanetStreamingManager&) = delete;
	static constexpr int32 MaxOutstandingBuilds = 2;
	static constexpr int32 MaxTrackedPatches = 64;
	virtual ~FAstraeonPlanetStreamingManager() override;
	virtual EAstraeonPatchRequestStatus Request(const FAstraeonPlanetDefinition& Planet,
		const FAstraeonPlanetPatchAddress& Address, const FAstraeonPlanetPatchBuildOptions& Options,
		uint64& OutRevision) override;
	virtual bool TryTakeCompleted(FAstraeonPlanetPatchCompletion& Out) override;
	virtual bool IsCurrent(const FAstraeonPlanetPatchAddress& Address, uint64 Revision) const override;
	virtual void Release(const FAstraeonPlanetPatchAddress& Address) override;
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
