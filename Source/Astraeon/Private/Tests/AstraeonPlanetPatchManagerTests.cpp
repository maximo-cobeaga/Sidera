#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Patches/AstraeonPlanetPatchManager.h"

namespace AstraeonPatchManagerTests
{
	using FAddress = FAstraeonPlanetPatchAddress;
	constexpr int32 Quads = 4; // Cheap meshes; Lab radius then reaches LOD 4.

	FAstraeonPlanetDefinition Planet()
	{
		FAstraeonPlanetDefinition P;
		P.BodyId = TEXT("patch_lab"); P.RadiusCm = 20000.0; P.MassKg = 1.e16; P.SurfaceGravityMS2 = 9.81;
		P.WorldSeed = 13579; P.BodySeed = 4242;
		return P;
	}
	TArray<FAddress> Roots()
	{
		TArray<FAddress> Result;
		for (uint8 F = 0; F < 6; ++F) { FAddress A; A.BodyId = TEXT("patch_lab"); A.Face = EAstraeonPlanetFace(F); Result.Add(A); }
		return Result;
	}
	FAddress Child(const FAddress& Parent, uint8 Quadrant) { FAddress C; Parent.TryChild(Quadrant, C); return C; }
	TArray<FAddress> SplitFirstRoot()
	{
		auto Leaves = Roots();
		const FAddress Root = Leaves[0];
		Leaves.RemoveAt(0);
		for (uint8 Q = 0; Q < 4; ++Q) Leaves.Add(Child(Root, Q));
		return Leaves;
	}
	FString Name(const FAddress& A)
	{
		return FString::Printf(TEXT("%d/%d/%d/%d"), int32(A.Face), int32(A.Lod), A.X, A.Y);
	}

	// Deterministic FIFO. Like the worker queue, a released job keeps its slot until drained.
	class FFakeQueue final : public IAstraeonPlanetPatchBuildQueue
	{
	public:
		int32 Capacity = 4;
		bool bDeliverReleased = false; // Misbehave: hand out results the queue should drop.
		TSet<FAddress> Failing;
		TArray<FAstraeonPlanetPatchCompletion> Injected;
		TMap<FAddress, uint64> Current;

		virtual EAstraeonPatchRequestStatus Request(const FAstraeonPlanetDefinition& P, const FAddress& A,
			const FAstraeonPlanetPatchBuildOptions& Options, uint64& OutRevision) override
		{
			OutRevision = 0;
			if (!Options.IsValid() || !P.IsValid()) return EAstraeonPatchRequestStatus::InvalidInput;
			if (Jobs.Num() >= Capacity) return EAstraeonPatchRequestStatus::AtCapacity;
			OutRevision = ++Last;
			Current.Add(A, OutRevision);
			Jobs.Add({P, A, Options, OutRevision});
			return EAstraeonPatchRequestStatus::Accepted;
		}
		virtual bool TryTakeCompleted(FAstraeonPlanetPatchCompletion& Out) override
		{
			Out = {};
			if (!Injected.IsEmpty()) { Out = Injected[0]; Injected.RemoveAt(0); return true; }
			while (!Jobs.IsEmpty())
			{
				const FJob Job = Jobs[0];
				Jobs.RemoveAt(0);
				if (!bDeliverReleased && !IsCurrent(Job.Address, Job.Revision)) continue;
				Out.Address = Job.Address; Out.BuildRevision = Job.Revision;
				Out.Status = Failing.Contains(Job.Address) ? EAstraeonPatchBuildStatus::InvalidInput
					: FAstraeonPlanetPatchMesh::Build(Job.Planet, Job.Address, Job.Revision, Job.Options, Out.Mesh);
				return true;
			}
			return false;
		}
		virtual bool IsCurrent(const FAddress& A, uint64 Revision) const override
		{
			const uint64* Found = Current.Find(A);
			return Found && *Found == Revision;
		}
		virtual void Release(const FAddress& A) override { Current.Remove(A); }
		int32 Outstanding() const { return Jobs.Num(); }
	private:
		struct FJob { FAstraeonPlanetDefinition Planet; FAddress Address; FAstraeonPlanetPatchBuildOptions Options; uint64 Revision; };
		TArray<FJob> Jobs;
		uint64 Last = 0;
	};

	class FFakeBackend final : public IAstraeonPlanetPatchMeshBackend
	{
	public:
		TMap<FAddress, bool> Meshes; // Committed patches and whether they are on screen.
		TArray<FString> Log;
		int32 Violations = 0;
		// Every mesh ever received, by address: a patch that comes back must come back the same.
		TMap<FAddress, uint64> FirstHash;
		int32 Returns = 0;
		int32 ReturnMismatches = 0;

		static uint64 Hash(const FAstraeonPlanetPatchBuildResult& Mesh)
		{
			uint64 H = FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Mesh.Vertices.GetData()), Mesh.Vertices.Num() * sizeof(FVector)));
			H = FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Mesh.Normals.GetData()), Mesh.Normals.Num() * sizeof(FVector)), H);
			H = FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Mesh.Indices.GetData()), Mesh.Indices.Num() * sizeof(int32)), H);
			return FAstraeonStableHash64::Bytes(TConstArrayView<uint8>(reinterpret_cast<const uint8*>(&Mesh.OriginBodyCm), sizeof(FVector)), H);
		}

		virtual bool CommitHidden(const FAstraeonPlanetPatchBuildResult& Mesh) override
		{
			const uint64 H = Hash(Mesh);
			if (const uint64* Seen = FirstHash.Find(Mesh.Address)) { ++Returns; ReturnMismatches += *Seen != H; }
			else FirstHash.Add(Mesh.Address, H);
			if (!Mesh.IsValid() || Meshes.Contains(Mesh.Address)) ++Violations;
			Meshes.Add(Mesh.Address, false);
			Log.Add(TEXT("commit ") + Name(Mesh.Address));
			return true;
		}
		virtual void SetVisible(const FAddress& A, bool bVisible) override
		{
			if (!Meshes.Contains(A)) { ++Violations; return; }
			Meshes[A] = bVisible;
			Log.Add((bVisible ? TEXT("show ") : TEXT("hide ")) + Name(A));
		}
		virtual void Remove(const FAddress& A) override
		{
			if (Meshes.Remove(A) == 0) ++Violations;
			Log.Add(TEXT("remove ") + Name(A));
		}
		TArray<FAddress> OnScreen() const
		{
			TArray<FAddress> Result;
			for (const auto& Item : Meshes) if (Item.Value) Result.Add(Item.Key);
			Result.Sort(FAstraeonPlanetLODManager::Less);
			return Result;
		}
	};

	// The one property a player sees: once the planet appears, it never has a hole or overlap.
	bool CheckScreen(FAutomationTestBase& Test, const FAstraeonPlanetPatchManager& Manager, const FFakeBackend& Backend,
		const TCHAR* Stage)
	{
		const auto Visible = Manager.GetVisible();
		if (!Test.TestTrue(FString::Printf(TEXT("%s: backend shows what the manager believes"), Stage), Visible == Backend.OnScreen())) return false;
		if (Visible.IsEmpty()) return true;
		FString Reason;
		const bool bPartition = FAstraeonPlanetLODManager::ValidatePartition(Visible, &Reason);
		return Test.TestTrue(FString::Printf(TEXT("%s: visible set is a partition (%s)"), Stage, *Reason), bPartition);
	}

	FVector Observer(const FVector& Direction) { return Direction.GetSafeNormal() * (Planet().RadiusCm + 1000.0); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchManagerRelayTest, "Astraeon.Planet.PatchManager.HoleFreeSplitAndMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchManagerRelayTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchManagerTests;
	FFakeQueue Queue; Queue.Capacity = 1;
	FFakeBackend Backend;
	FAstraeonPlanetPatchManager Manager(Queue, Backend);
	const auto P = Planet();
	const FVector Eye = Observer(FVector(1, 0, 0));
	const FAddress Root = Roots()[0];

	if (!TestTrue(TEXT("Roots accepted"), Manager.SetTarget(P, Roots(), Quads, Eye))) return false;
	// Capacity 1: each call commits one root and requests the next, so five are ready after six calls.
	for (int32 I = 0; I < 6; ++I)
	{
		Manager.Update(1);
		TestTrue(TEXT("First cover appears whole or not at all"), Manager.GetVisible().IsEmpty());
	}
	Manager.Update(1);
	if (!TestTrue(TEXT("Six roots settle"), Manager.IsSettled())) return false;
	if (!CheckScreen(*this, Manager, Backend, TEXT("roots"))) return false;

	// Split: the parent stays until all four children are ready, then one relay swaps them.
	TestTrue(TEXT("Split accepted"), Manager.SetTarget(P, SplitFirstRoot(), Quads, Eye));
	for (int32 I = 0; I < 5; ++I)
	{
		Manager.Update(1);
		if (!CheckScreen(*this, Manager, Backend, TEXT("split"))) return false;
		const bool bLast = I == 4;
		TestEqual(TEXT("Parent visible until the last child commits"), Backend.OnScreen().Contains(Root), !bLast);
		TestEqual(TEXT("No child visible before its siblings"), Backend.OnScreen().Contains(Child(Root, 0)), bLast);
	}
	TestTrue(TEXT("Split settles"), Manager.IsSettled());
	TestFalse(TEXT("Outgoing parent is released from the backend"), Backend.Meshes.Contains(Root));

	// Merge: the children stay until the parent is rebuilt.
	TestTrue(TEXT("Merge accepted"), Manager.SetTarget(P, Roots(), Quads, Eye));
	Manager.Update(1);
	if (!CheckScreen(*this, Manager, Backend, TEXT("merge request"))) return false;
	TestTrue(TEXT("Children stay while the parent builds"), Backend.OnScreen().Contains(Child(Root, 3)));
	Manager.Update(1);
	if (!CheckScreen(*this, Manager, Backend, TEXT("merge"))) return false;
	TestTrue(TEXT("Merge settles in one relay"), Manager.IsSettled());
	TestEqual(TEXT("Children removed after merge"), Backend.Meshes.Num(), 6);

	// Retarget mid-relay: half-built children are cancelled or removed, the parent never blinks.
	Queue.Capacity = 2;
	TestTrue(TEXT("Split again"), Manager.SetTarget(P, SplitFirstRoot(), Quads, Eye));
	Manager.Update(1);
	const int32 CancelsBefore = Manager.GetStats().Cancels;
	TestTrue(TEXT("Back to roots"), Manager.SetTarget(P, Roots(), Quads, Eye));
	Manager.Update(1);
	if (!CheckScreen(*this, Manager, Backend, TEXT("retarget"))) return false;
	TestTrue(TEXT("Root never left the screen"), Backend.OnScreen().Contains(Root));
	TestTrue(TEXT("Pending child cancelled"), Manager.GetStats().Cancels > CancelsBefore);
	TestTrue(TEXT("Settled on the old cover"), Manager.IsSettled());
	TestEqual(TEXT("Committed hidden child removed"), Backend.Meshes.Num(), 6);
	TestEqual(TEXT("No pending work leaks"), Manager.GetPendingCount(), 0);

	Manager.Reset();
	TestTrue(TEXT("Reset empties the backend"), Backend.Meshes.IsEmpty());
	TestTrue(TEXT("Reset releases the queue registry"), Queue.Current.IsEmpty());
	TestEqual(TEXT("Backend never saw an invalid operation"), Backend.Violations, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchManagerRoundTripTest, "Astraeon.Planet.PatchManager.RoundTripRegeneratesSamePatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchManagerRoundTripTest::RunTest(const FString& Parameters)
{
	// Phase 2 gate: going away and coming back regenerates the same patch. Out to the far side,
	// back again, twice, on the real worker pool: every patch built again is byte-identical.
	using namespace AstraeonPatchManagerTests;
	const auto P = Planet();
	FAstraeonPlanetLODSettings Settings; Settings.Quads = Quads;
	FAstraeonPlanetStreamingManager Workers;
	FFakeBackend Backend;
	FAstraeonPlanetPatchManager Manager(Workers, Backend);
	const double Deadline = FPlatformTime::Seconds() + 30.0;
	for (const FVector Stop : {FVector(1, 0.2, 0.1), FVector(-1, -0.3, 0.2), FVector(1, 0.2, 0.1), FVector(-1, -0.3, 0.2), FVector(1, 0.2, 0.1)})
	{
		FAstraeonPlanetLODView View; View.ObserverBodyCm = Observer(Stop);
		FAstraeonPlanetLODSelection Selection;
		FAstraeonPlanetLODManager::Select(P, View, Settings, Selection);
		Manager.SetTarget(P, Selection.Leaves, Quads, View.ObserverBodyCm);
		while (!Manager.IsSettled() && FPlatformTime::Seconds() < Deadline) { Manager.Update(4); FPlatformProcess::Sleep(0.0005f); }
		if (!TestTrue(TEXT("Each stop settles"), Manager.IsSettled())) return false;
	}
	TestTrue(TEXT("Patches were unloaded and built again"), Backend.Returns > 10 && Manager.GetStats().Removals > 10);
	TestEqual(TEXT("Every rebuilt patch is identical to its first build"), Backend.ReturnMismatches, 0);
	AddInfo(FString::Printf(TEXT("PatchRoundTrip returns=%d mismatches=%d removals=%d"), Backend.Returns, Backend.ReturnMismatches, Manager.GetStats().Removals));
	Manager.Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchManagerRejectTest, "Astraeon.Planet.PatchManager.RejectsStaleFailedAndInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchManagerRejectTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchManagerTests;
	const auto P = Planet();
	const FVector Eye = Observer(FVector(1, 0, 0));
	const FAddress Root = Roots()[0];

	{
		FFakeQueue Queue; Queue.Capacity = 6;
		FFakeBackend Backend;
		FAstraeonPlanetPatchManager Manager(Queue, Backend);
		auto Unbalanced = SplitFirstRoot();
		const FAddress Grown = Child(Root, 0);
		Unbalanced.Remove(Grown);
		for (uint8 Q = 0; Q < 4; ++Q) Unbalanced.Add(Child(Grown, Q));
		TestFalse(TEXT("Unbalanced target rejected"), Manager.SetTarget(P, Unbalanced, Quads, Eye));
		TestFalse(TEXT("Non power-of-two grid rejected"), Manager.SetTarget(P, Roots(), 5, Eye));
		auto Foreign = P; Foreign.BodyId = TEXT("other_body");
		TestFalse(TEXT("Leaves of another body rejected"), Manager.SetTarget(Foreign, Roots(), Quads, Eye));
		TestTrue(TEXT("Rejections leave no target"), Manager.GetTarget().IsEmpty());

		// Commit budget: six results are ready, one upload per call.
		Manager.SetTarget(P, Roots(), Quads, Eye);
		Manager.Update(1);
		TestEqual(TEXT("Six requests fill the queue"), Queue.Outstanding(), 6);
		Manager.Update(1);
		TestEqual(TEXT("One commit per call under budget 1"), Manager.GetStats().Commits, 1);

		// A forged result with an unknown revision never reaches the backend.
		FAstraeonPlanetPatchCompletion Forged;
		Forged.Address = Roots()[5]; Forged.BuildRevision = 999; Forged.Status = EAstraeonPatchBuildStatus::Success;
		FAstraeonPlanetPatchMesh::Build(P, Forged.Address, 999, {}, Forged.Mesh);
		Queue.Injected.Add(Forged);
		const int32 CommitsBefore = Manager.GetStats().Commits;
		Manager.Update(1);
		TestEqual(TEXT("Stale revision dropped"), Manager.GetStats().StaleDrops, 1);
		TestEqual(TEXT("Dropping a stale result does not consume the budget"), Manager.GetStats().Commits, CommitsBefore + 1);

		// The queue forgets a job but still delivers it: dropped, then asked for again.
		Queue.Release(Roots()[4]);
		Queue.bDeliverReleased = true;
		for (int32 I = 0; I < 12 && !Manager.IsSettled(); ++I) Manager.Update(6);
		TestTrue(TEXT("Forgotten job is re-requested and the cover settles"), Manager.IsSettled());
		TestEqual(TEXT("Seven requests: six roots plus the forgotten one"), Manager.GetStats().Requests, 7);
		TestEqual(TEXT("No invalid backend operation"), Backend.Violations, 0);
		Manager.Reset();
	}
	{
		// A queue that hands out released work: the manager, not the queue, is the last guard.
		FFakeQueue Queue; Queue.Capacity = 8; Queue.bDeliverReleased = true;
		FFakeBackend Backend;
		FAstraeonPlanetPatchManager Manager(Queue, Backend);
		Manager.SetTarget(P, Roots(), Quads, Eye);
		for (int32 I = 0; I < 3 && !Manager.IsSettled(); ++I) Manager.Update(8);
		Manager.SetTarget(P, SplitFirstRoot(), Quads, Eye);
		Manager.Update(0); // Requests the children without taking any result.
		Manager.SetTarget(P, Roots(), Quads, Eye);
		Manager.Update(0); // Releases them, still queued.
		Manager.Update(8);
		TestEqual(TEXT("Released children are never committed"), Backend.Meshes.Num(), 6);
		TestEqual(TEXT("Each released child counted as stale"), Manager.GetStats().StaleDrops, 4);
		if (!CheckScreen(*this, Manager, Backend, TEXT("released"))) return false;
		Manager.Reset();
	}
	{
		// A failed child: never flat terrain, never a hole. The parent simply stays.
		FFakeQueue Queue; Queue.Capacity = 8;
		Queue.Failing.Add(Child(Root, 2));
		FFakeBackend Backend;
		FAstraeonPlanetPatchManager Manager(Queue, Backend);
		Manager.SetTarget(P, Roots(), Quads, Eye);
		for (int32 I = 0; I < 3 && !Manager.IsSettled(); ++I) Manager.Update(8);
		Manager.SetTarget(P, SplitFirstRoot(), Quads, Eye);
		for (int32 I = 0; I < 10; ++I)
		{
			Manager.Update(8);
			if (!CheckScreen(*this, Manager, Backend, TEXT("failed child"))) return false;
		}
		TestEqual(TEXT("Failure counted once, not retried every frame"), Manager.GetStats().Failures, 1);
		TestTrue(TEXT("Parent stays on screen"), Backend.OnScreen().Contains(Root));
		TestFalse(TEXT("Split never settles"), Manager.IsSettled());
		Manager.SetTarget(P, Roots(), Quads, Eye);
		Manager.Update(8);
		TestTrue(TEXT("Leaving the failed area recovers"), Manager.IsSettled());
		TestEqual(TEXT("No invalid backend operation"), Backend.Violations, 0);
		Manager.Reset();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchManagerRouteTest, "Astraeon.Planet.PatchManager.DeterministicRouteWithWorkers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchManagerRouteTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchManagerTests;
	const auto P = Planet();
	FAstraeonPlanetLODSettings Settings; Settings.Quads = Quads;
	// Face centre, across the +X/+Y seam, into the +X/+Y/+Z corner and back over the seam.
	TArray<FVector> Route;
	const FVector Stops[] = {FVector(1, 0.1, 0.05), FVector(1, 1, 0.05), FVector(1, 1, 1), FVector(0.2, 1, 0.3)};
	for (int32 Leg = 0; Leg + 1 < UE_ARRAY_COUNT(Stops); ++Leg)
		for (int32 Step = 0; Step < 12; ++Step)
			Route.Add(FMath::Lerp(Stops[Leg].GetSafeNormal(), Stops[Leg + 1].GetSafeNormal(), Step / 12.0));
	Route.Add(Stops[UE_ARRAY_COUNT(Stops) - 1]);

	// Two runs with the deterministic queue. Two updates per step: the route outruns the
	// builds on purpose, so relays happen against a moving target.
	TArray<FString> FirstLog;
	int32 MaxVisible = 0;
	for (int32 Run = 0; Run < 2; ++Run)
	{
		FFakeQueue Queue; Queue.Capacity = 2;
		FFakeBackend Backend;
		FAstraeonPlanetPatchManager Manager(Queue, Backend);
		for (int32 Index = 0; Index < Route.Num(); ++Index)
		{
			FAstraeonPlanetLODView View; View.ObserverBodyCm = Observer(Route[Index]);
			FAstraeonPlanetLODSelection Selection;
			if (!TestTrue(TEXT("Selection succeeds"), FAstraeonPlanetLODManager::Select(P, View, Settings, Selection))) return false;
			if (!TestTrue(TEXT("Target accepted"), Manager.SetTarget(P, Selection.Leaves, Quads, View.ObserverBodyCm))) return false;
			// Settle the start so the whole route runs with a planet on screen to protect.
			for (int32 I = 0; Index == 0 && I < 2000 && !Manager.IsSettled(); ++I) Manager.Update(2);
			for (int32 I = 0; I < 2; ++I)
			{
				Manager.Update(2);
				if (!CheckScreen(*this, Manager, Backend, TEXT("route"))) return false;
				MaxVisible = FMath::Max(MaxVisible, Manager.GetVisible().Num());
			}
		}
		for (int32 I = 0; I < 2000 && !Manager.IsSettled(); ++I) Manager.Update(2);
		if (!TestTrue(TEXT("Route settles once the observer stops"), Manager.IsSettled())) return false;
		FString Reason;
		TestTrue(TEXT("Settled screen is a balanced cover"), FAstraeonPlanetLODManager::ValidateCover(Manager.GetVisible(), &Reason));
		TestEqual(TEXT("No invalid backend operation"), Backend.Violations, 0);
		TestEqual(TEXT("Only the target stays committed"), Manager.GetCommittedCount(), Manager.GetTarget().Num());
		if (Run == 0) FirstLog = Backend.Log;
		else TestTrue(TEXT("Same route and completion order give the same backend operations"), Backend.Log == FirstLog);
		Manager.Reset();
		AddInfo(FString::Printf(TEXT("PatchRoute run=%d ops=%d requests=%d commits=%d relays=%d cancels=%d removals=%d max_visible=%d"),
			Run, Backend.Log.Num(), Manager.GetStats().Requests, Manager.GetStats().Commits, Manager.GetStats().Relays,
			Manager.GetStats().Cancels, Manager.GetStats().Removals, MaxVisible));
	}

	// Same route on the real worker pool: completion order is not controlled, the invariant is.
	FAstraeonPlanetStreamingManager Workers;
	FFakeBackend Backend;
	FAstraeonPlanetPatchManager Manager(Workers, Backend);
	const double Deadline = FPlatformTime::Seconds() + 30.0;
	for (int32 Index = 0; Index < Route.Num(); Index += 4)
	{
		FAstraeonPlanetLODView View; View.ObserverBodyCm = Observer(Route[Index]);
		FAstraeonPlanetLODSelection Selection;
		FAstraeonPlanetLODManager::Select(P, View, Settings, Selection);
		Manager.SetTarget(P, Selection.Leaves, Quads, View.ObserverBodyCm);
		while (!Manager.IsSettled() && FPlatformTime::Seconds() < Deadline)
		{
			Manager.Update(2);
			if (!CheckScreen(*this, Manager, Backend, TEXT("workers"))) return false;
			TestTrue(TEXT("Streaming registry holds only pending work"),
				Workers.GetTrackedPatchCount() <= FAstraeonPlanetStreamingManager::MaxOutstandingBuilds);
			FPlatformProcess::Sleep(0.0005f);
		}
		if (!TestTrue(TEXT("Workers settle every stop in bounded time"), Manager.IsSettled())) return false;
	}
	Manager.Reset();
	TestEqual(TEXT("Reset releases every worker address"), Workers.GetTrackedPatchCount(), 0);
	TestTrue(TEXT("Reset empties the backend"), Backend.Meshes.IsEmpty());
	TestEqual(TEXT("No invalid backend operation with real workers"), Backend.Violations, 0);
	return true;
}

#endif
