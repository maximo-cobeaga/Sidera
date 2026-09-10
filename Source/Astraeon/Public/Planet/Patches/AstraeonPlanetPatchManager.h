#pragma once

#include "CoreMinimal.h"
#include "Planet/Streaming/AstraeonPlanetStreamingManager.h"

// Where committed patch meshes live. The production backend is chosen by ADR after measuring;
// the scheduler only depends on this seam.
class ASTRAEON_API IAstraeonPlanetPatchMeshBackend
{
public:
	virtual ~IAstraeonPlanetPatchMeshBackend() = default;
	// Uploads hidden: the manager reveals a patch only when its whole relay group is ready.
	virtual bool CommitHidden(const FAstraeonPlanetPatchBuildResult& Mesh) = 0;
	virtual void SetVisible(const FAstraeonPlanetPatchAddress& Address, bool bVisible) = 0;
	virtual void Remove(const FAstraeonPlanetPatchAddress& Address) = 0;
};

struct ASTRAEON_API FAstraeonPlanetPatchManagerStats
{
	int32 Requests = 0;
	int32 Commits = 0;
	int32 StaleDrops = 0;
	int32 Failures = 0;
	int32 Relays = 0;
	int32 Removals = 0;
	int32 Cancels = 0;
};

// Game-thread scheduler between the LOD selection, the build queue and the mesh backend.
//
// Invariant: once anything is visible, the visible set is a partition of the sphere after
// every call. A split shows the incoming leaves only when all of them are committed, and a
// merge hides the outgoing leaves only when their parent is committed; until then the old
// patches stay on screen. The LOD delta may briefly exceed one during a relay.
//
// A failed build is never replaced with flat terrain: the patch it would have replaced stays.
// Destruction does not touch the queue or backend; call Reset first while both are alive.
class ASTRAEON_API FAstraeonPlanetPatchManager
{
public:
	using FAddress = FAstraeonPlanetPatchAddress;
	FAstraeonPlanetPatchManager(IAstraeonPlanetPatchBuildQueue& InQueue, IAstraeonPlanetPatchMeshBackend& InBackend)
		: Queue(InQueue), Backend(InBackend) {}
	FAstraeonPlanetPatchManager(const FAstraeonPlanetPatchManager&) = delete;
	FAstraeonPlanetPatchManager& operator=(const FAstraeonPlanetPatchManager&) = delete;

	// Leaves must be a balanced cover. A different planet or grid resets everything first.
	// The observer only orders requests (nearest first); it never changes the selection.
	bool SetTarget(const FAstraeonPlanetDefinition& Planet, const TArray<FAddress>& Leaves, int32 Quads,
		const FVector& ObserverBodyCm);
	// MaxCommits bounds backend uploads per call: completions beyond it wait in the queue.
	void Update(int32 MaxCommits);
	void Reset();

	TArray<FAddress> GetVisible() const;
	const TArray<FAddress>& GetTarget() const { return Target; }
	bool IsSettled() const;
	int32 GetPendingCount() const;
	int32 GetCommittedCount() const;
	const FAstraeonPlanetPatchManagerStats& GetStats() const { return Stats; }

private:
	struct FPatch
	{
		uint64 PendingRevision = 0;
		bool bCommitted = false;
		bool bFailed = false;
	};
	void Relay();
	void Collect();
	void RequestMissing();
	void Show(const FAddress& Address);
	void Hide(const FAddress& Address);

	IAstraeonPlanetPatchBuildQueue& Queue;
	IAstraeonPlanetPatchMeshBackend& Backend;
	FAstraeonPlanetDefinition Planet;
	int32 Quads = 0;
	TArray<FAddress> Target;       // Sorted with FAstraeonPlanetLODManager::Less.
	TArray<FAddress> RequestOrder; // Target, nearest to the observer first.
	TSet<FAddress> TargetSet;
	TSet<FAddress> Visible;
	TMap<FAddress, FPatch> Patches;
	FAstraeonPlanetPatchManagerStats Stats;
	bool bDirty = false;
};
