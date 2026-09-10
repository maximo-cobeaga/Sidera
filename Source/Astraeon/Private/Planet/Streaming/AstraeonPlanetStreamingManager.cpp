#include "Planet/Streaming/AstraeonPlanetStreamingManager.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Async/Async.h"

FAstraeonPlanetStreamingManager::~FAstraeonPlanetStreamingManager()
{
	// Tasks own input copies and their cancellation token. Destruction never waits for a
	// worker on the game thread, and late results cannot reach a destroyed manager.
	for (auto& Job : Jobs) Job.Cancelled->store(true, std::memory_order_relaxed);
}

EAstraeonPatchRequestStatus FAstraeonPlanetStreamingManager::Request(const FAstraeonPlanetDefinition& Planet,
	const FAstraeonPlanetPatchAddress& Address, const FAstraeonPlanetPatchBuildOptions& Options, uint64& OutRevision)
{
	check(IsInGameThread());
	OutRevision = 0;
	uint64 Seed;
	if (!Options.IsValid() || !Planet.IsValid() || Options.SkirtDepthCm >= Planet.RadiusCm
		|| Planet.GeneratorVersion != FAstraeonPlanetSurface::GeneratorVersion
		|| !Address.TryDeriveSeed(Planet, EAstraeonGenerationChannel::Terrain, Seed)) return EAstraeonPatchRequestStatus::InvalidInput;
	if (Jobs.Num() >= MaxOutstandingBuilds ||
		(!CurrentRevisions.Contains(Address) && CurrentRevisions.Num() >= MaxTrackedPatches)) return EAstraeonPatchRequestStatus::AtCapacity;
	if (LastRevision == MAX_uint64) return EAstraeonPatchRequestStatus::RevisionExhausted;

	Release(Address);
	OutRevision = ++LastRevision;
	CurrentRevisions.Add(Address, OutRevision);
	FJob Job;
	Job.Address = Address;
	Job.Cancelled = MakeShared<std::atomic<bool>, ESPMode::ThreadSafe>(false);
	const auto Cancelled = Job.Cancelled;
	const uint64 Revision = OutRevision;
	Job.Future = Async(EAsyncExecution::ThreadPool, [Planet, Address, Options, Cancelled, Revision]()
	{
		check(!IsInGameThread());
		FAstraeonPlanetPatchCompletion Result;
		Result.Address = Address; Result.BuildRevision = Revision;
		Result.Status = FAstraeonPlanetPatchMesh::Build(Planet, Address, Revision, Options, Result.Mesh, Cancelled.Get());
		return Result;
	});
	Jobs.Add(MoveTemp(Job));
	return EAstraeonPatchRequestStatus::Accepted;
}

bool FAstraeonPlanetStreamingManager::TryTakeCompleted(FAstraeonPlanetPatchCompletion& Out)
{
	check(IsInGameThread());
	Out = {};
	for (int32 I = Jobs.Num() - 1; I >= 0; --I)
	{
		if (!Jobs[I].Future.IsReady()) continue;
		auto Completed = Jobs[I].Future.Get();
		Jobs.RemoveAtSwap(I);
		if (!IsCurrent(Completed.Address, Completed.BuildRevision)) continue;
		Out = MoveTemp(Completed);
		return true; // Failures are delivered explicitly, never replaced with flat terrain.
	}
	return false;
}

bool FAstraeonPlanetStreamingManager::IsCurrent(const FAstraeonPlanetPatchAddress& Address, uint64 Revision) const
{
	check(IsInGameThread());
	const uint64* Current = CurrentRevisions.Find(Address);
	return Current && *Current == Revision;
}

void FAstraeonPlanetStreamingManager::Release(const FAstraeonPlanetPatchAddress& Address)
{
	check(IsInGameThread());
	CurrentRevisions.Remove(Address);
	for (auto& Job : Jobs)
		if (Job.Address == Address) Job.Cancelled->store(true, std::memory_order_relaxed);
}
