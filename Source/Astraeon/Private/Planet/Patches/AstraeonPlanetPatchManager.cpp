#include "Planet/Patches/AstraeonPlanetPatchManager.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	using FAddress = FAstraeonPlanetPatchAddress;

	bool SameBody(const FAstraeonPlanetDefinition& A, const FAstraeonPlanetDefinition& B)
	{
		return A.BodyId == B.BodyId && A.RadiusCm == B.RadiusCm && A.MassKg == B.MassKg
			&& A.SurfaceGravityMS2 == B.SurfaceGravityMS2 && A.SeaLevelAltitudeCm == B.SeaLevelAltitudeCm
			&& A.WorldSeed == B.WorldSeed && A.BodySeed == B.BodySeed && A.GeneratorVersion == B.GeneratorVersion;
	}

	TArray<FAddress> Sorted(const TArray<FAddress>& Keys)
	{
		TArray<FAddress> Result = Keys;
		Result.Sort(FAstraeonPlanetLODManager::Less);
		return Result;
	}
}

bool FAstraeonPlanetPatchManager::SetTarget(const FAstraeonPlanetDefinition& InPlanet, const TArray<FAddress>& Leaves,
	int32 InQuads, const FVector& ObserverBodyCm)
{
	FAstraeonPlanetPatchBuildOptions Probe; Probe.Quads = InQuads;
	if (!InPlanet.IsValid() || !Probe.IsValid() || ObserverBodyCm.ContainsNaN()
		|| !FAstraeonPlanetLODManager::ValidateCover(Leaves) || Leaves[0].BodyId != InPlanet.BodyId) return false;
	if (Quads != InQuads || !SameBody(Planet, InPlanet)) Reset();
	Planet = InPlanet; Quads = InQuads;
	Target = Sorted(Leaves);
	TargetSet.Reset(); TargetSet.Append(Target);
	TArray<TPair<double, FAddress>> ByDistance;
	for (const auto& A : Target)
	{
		FVector2D Min, Max; A.TryUvBounds(Min, Max);
		const FVector Center = FAstraeonPlanetCoordinates::FaceUvToDirection(A.Face, (Min + Max) * 0.5) * Planet.RadiusCm;
		ByDistance.Emplace(FVector::DistSquared(Center, ObserverBodyCm), A);
	}
	ByDistance.Sort([](const auto& L, const auto& R)
	{
		return L.Key != R.Key ? L.Key < R.Key : FAstraeonPlanetLODManager::Less(L.Value, R.Value);
	});
	RequestOrder.Reset(ByDistance.Num());
	for (const auto& Item : ByDistance) RequestOrder.Add(Item.Value);
	bDirty = true;
	return true;
}

void FAstraeonPlanetPatchManager::Update(int32 MaxCommits)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Astraeon_PlanetPatches_Update);
	int32 Commits = 0;
	FAstraeonPlanetPatchCompletion Done;
	while (Commits < MaxCommits && Queue.TryTakeCompleted(Done))
	{
		FPatch* Patch = Patches.Find(Done.Address);
		// The queue filters revisions on collection; this is the commit-instant check its
		// contract asks for, and the guard against a queue that breaks it.
		if (!Patch || Patch->PendingRevision != Done.BuildRevision)
		{
			++Stats.StaleDrops;
			continue;
		}
		if (!Queue.IsCurrent(Done.Address, Done.BuildRevision))
		{
			Patch->PendingRevision = 0; // The queue forgot it: ask again rather than wait forever.
			++Stats.StaleDrops;
			continue;
		}
		Queue.Release(Done.Address);
		Patch->PendingRevision = 0;
		if (Done.Status != EAstraeonPatchBuildStatus::Success || !(Done.Mesh.Address == Done.Address)
			|| Done.Mesh.BuildRevision != Done.BuildRevision || !Backend.CommitHidden(Done.Mesh))
		{
			Patch->bFailed = true;
			++Stats.Failures;
			continue;
		}
		Patch->bCommitted = true;
		++Commits; ++Stats.Commits;
		bDirty = true;
	}
	if (bDirty)
	{
		Relay();
		Collect();
		bDirty = false;
	}
	RequestMissing();
}

void FAstraeonPlanetPatchManager::Relay()
{
	if (Target.IsEmpty()) return;
	const auto Committed = [this](const FAddress& A) { const FPatch* P = Patches.Find(A); return P && P->bCommitted; };
	if (Visible.IsEmpty())
	{
		// Nothing to keep on screen yet: the first cover appears whole or not at all.
		for (const auto& A : Target) if (!Committed(A)) return;
		for (const auto& A : Target) Show(A);
		++Stats.Relays;
		return;
	}
	// Both sets are partitions, so each difference is either an outgoing patch whose area the
	// target splits, or an incoming patch whose area the visible set still splits.
	TMap<FAddress, TArray<FAddress>> Splits;
	TMap<FAddress, TArray<FAddress>> Merges;
	for (const auto& A : Target)
	{
		if (Visible.Contains(A)) continue;
		for (FAddress P = A; P.TryParent(P);)
			if (Visible.Contains(P)) { Splits.FindOrAdd(P).Add(A); break; }
	}
	for (const auto& V : Sorted(Visible.Array()))
	{
		if (TargetSet.Contains(V)) continue;
		for (FAddress P = V; P.TryParent(P);)
			if (TargetSet.Contains(P)) { Merges.FindOrAdd(P).Add(V); break; }
	}
	TArray<FAddress> Keys;
	Splits.GetKeys(Keys);
	for (const auto& Outgoing : Sorted(Keys))
	{
		const auto& Incoming = Splits[Outgoing];
		if (!Incoming.ContainsByPredicate([&](const FAddress& A) { return !Committed(A); }))
		{
			// Show before hiding, the same rule as the collision relay.
			for (const auto& A : Incoming) Show(A);
			Hide(Outgoing);
			++Stats.Relays;
		}
	}
	Keys.Reset();
	Merges.GetKeys(Keys);
	for (const auto& Incoming : Sorted(Keys))
	{
		if (!Committed(Incoming)) continue;
		Show(Incoming);
		for (const auto& A : Merges[Incoming]) Hide(A);
		++Stats.Relays;
	}
}

void FAstraeonPlanetPatchManager::Collect()
{
	TArray<FAddress> Unused;
	for (const auto& Item : Patches)
		if (!TargetSet.Contains(Item.Key) && !Visible.Contains(Item.Key)) Unused.Add(Item.Key);
	for (const auto& A : Sorted(Unused))
	{
		const FPatch Patch = Patches.FindAndRemoveChecked(A);
		if (Patch.PendingRevision) { Queue.Release(A); ++Stats.Cancels; }
		if (Patch.bCommitted) { Backend.Remove(A); ++Stats.Removals; }
	}
}

void FAstraeonPlanetPatchManager::RequestMissing()
{
	for (const auto& A : RequestOrder)
	{
		FPatch& Patch = Patches.FindOrAdd(A);
		if (Patch.bCommitted || Patch.PendingRevision || Patch.bFailed) continue;
		FAstraeonPlanetPatchBuildOptions Options;
		Options.Quads = Quads;
		Options.SkirtDepthCm = FAstraeonPlanetLODManager::SkirtDepthCm(Planet, A, Quads);
		uint64 Revision = 0;
		const auto Status = Queue.Request(Planet, A, Options, Revision);
		if (Status == EAstraeonPatchRequestStatus::AtCapacity) break;
		if (Status != EAstraeonPatchRequestStatus::Accepted)
		{
			Patch.bFailed = true;
			++Stats.Failures;
			continue;
		}
		Patch.PendingRevision = Revision;
		++Stats.Requests;
	}
}

void FAstraeonPlanetPatchManager::Reset()
{
	TArray<FAddress> All;
	Patches.GetKeys(All);
	for (const auto& A : Sorted(All))
	{
		const FPatch& Patch = Patches[A];
		if (Patch.PendingRevision) { Queue.Release(A); ++Stats.Cancels; }
		if (Patch.bCommitted) { Backend.Remove(A); ++Stats.Removals; }
	}
	Patches.Reset(); Visible.Reset(); Target.Reset(); RequestOrder.Reset(); TargetSet.Reset();
	Planet = {}; Quads = 0; bDirty = false;
}

void FAstraeonPlanetPatchManager::Show(const FAddress& Address)
{
	Backend.SetVisible(Address, true);
	Visible.Add(Address);
}

void FAstraeonPlanetPatchManager::Hide(const FAddress& Address)
{
	Backend.SetVisible(Address, false);
	Visible.Remove(Address);
}

TArray<FAstraeonPlanetPatchAddress> FAstraeonPlanetPatchManager::GetVisible() const
{
	return Sorted(Visible.Array());
}

bool FAstraeonPlanetPatchManager::IsSettled() const
{
	if (Target.IsEmpty() || Visible.Num() != Target.Num()) return false;
	for (const auto& A : Target) if (!Visible.Contains(A)) return false;
	return true;
}

int32 FAstraeonPlanetPatchManager::GetPendingCount() const
{
	int32 Count = 0;
	for (const auto& Item : Patches) Count += Item.Value.PendingRevision != 0;
	return Count;
}

int32 FAstraeonPlanetPatchManager::GetCommittedCount() const
{
	int32 Count = 0;
	for (const auto& Item : Patches) Count += Item.Value.bCommitted;
	return Count;
}
