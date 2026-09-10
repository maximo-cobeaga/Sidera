#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Planet/Patches/AstraeonPlanetPatchMesh.h"
#include "Planet/Streaming/AstraeonPlanetStreamingManager.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "Planet/Surface/AstraeonCubeSphereMesh.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include <limits>

namespace AstraeonPatchTests
{
	FAstraeonPlanetDefinition Planet(double Radius = 50000000.0)
	{
		FAstraeonPlanetDefinition P;
		P.BodyId = TEXT("test_body"); P.WorldSeed = 13579; P.BodySeed = 4242;
		P.RadiusCm = Radius; P.MassKg = 1.e16; P.SurfaceGravityMS2 = 9.81;
		return P;
	}
	FAstraeonPlanetPatchAddress Address(uint8 Lod = 3, int32 X = 2, int32 Y = 5)
	{
		FAstraeonPlanetPatchAddress A;
		A.BodyId = TEXT("test_body"); A.Face = EAstraeonPlanetFace::PositiveZ;
		A.Lod = Lod; A.X = X; A.Y = Y;
		return A;
	}
	bool Build(const FAstraeonPlanetDefinition& P, const FAstraeonPlanetPatchAddress& A,
		FAstraeonPlanetPatchBuildResult& Out, double SkirtDepth = 100.0)
	{
		FAstraeonPlanetPatchBuildOptions Options; Options.SkirtDepthCm = SkirtDepth;
		return FAstraeonPlanetPatchMesh::Build(P, A, 1, Options, Out) == EAstraeonPatchBuildStatus::Success;
	}
	bool Drain(FAstraeonPlanetStreamingManager& Manager, TArray<FAstraeonPlanetPatchCompletion>& Out)
	{
		const double Deadline = FPlatformTime::Seconds() + 10.0;
		while (Manager.GetOutstandingBuildCount() > 0 && FPlatformTime::Seconds() < Deadline)
		{
			FAstraeonPlanetPatchCompletion Completed;
			if (Manager.TryTakeCompleted(Completed)) Out.Add(MoveTemp(Completed));
			else FPlatformProcess::Sleep(0.001f);
		}
		return Manager.GetOutstandingBuildCount() == 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchAddressTest, "Astraeon.Planet.Patches.AddressHierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchAddressTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	const auto A = Address();
	for (uint8 Q = 0; Q < 4; ++Q)
	{
		FAstraeonPlanetPatchAddress Child, Parent;
		TestTrue(TEXT("Child exists"), A.TryChild(Q, Child));
		TestTrue(TEXT("Parent exists"), Child.TryParent(Parent));
		TestTrue(TEXT("Round trip preserves address"), Parent == A);
		FVector2D Min, Max; Child.TryUvBounds(Min, Max);
		FAstraeonPlanetPatchAddress Found;
		TestTrue(TEXT("Direction indexes child"), FAstraeonPlanetPatchAddress::TryFromDirection(A.BodyId,
			FAstraeonPlanetCoordinates::FaceUvToDirection(Child.Face, (Min + Max) * 0.5), Child.Lod, Found));
		TestTrue(TEXT("Direction resolves original child"), Found == Child);
	}
	FAstraeonPlanetPatchAddress Out;
	TestFalse(TEXT("Root has no parent"), Address(0, 0, 0).TryParent(Out));
	TestFalse(TEXT("Invalid quadrant rejected"), A.TryChild(4, Out));
	TestFalse(TEXT("Terminal level cannot subdivide"), Address(24, 0, 0).TryChild(0, Out));
	TestFalse(TEXT("Overflow level rejected before shifting"), Address(255, 0, 0).IsValid());
	TestFalse(TEXT("Negative coordinate rejected"), Address(3, -1, 0).IsValid());
	TestFalse(TEXT("Coordinate outside level rejected"), Address(3, 8, 0).IsValid());
	auto Bad = A; Bad.Face = EAstraeonPlanetFace(6);
	TestFalse(TEXT("Seventh face rejected"), Bad.IsValid());
	Bad = A; Bad.BodyId = NAME_None; TestFalse(TEXT("Missing body rejected"), Bad.IsValid());
	Bad.BodyId = TEXT("body with spaces"); TestFalse(TEXT("Ambiguous identity rejected"), Bad.IsValid());
	for (int32 Face = 0; Face < 6; ++Face)
	for (const FVector2D Uv : {FVector2D(-1,-1), FVector2D(1,1), FVector2D(0,0)})
	{
		TestTrue(TEXT("Faces and corners resolve at maximum level"), FAstraeonPlanetPatchAddress::TryFromDirection(A.BodyId,
			FAstraeonPlanetCoordinates::FaceUvToDirection(EAstraeonPlanetFace(Face), Uv), 24, Out));
		TestTrue(TEXT("Boundary never produces out of range index"), Out.IsValid());
	}
	TestFalse(TEXT("Zero direction rejected"), FAstraeonPlanetPatchAddress::TryFromDirection(A.BodyId, FVector::ZeroVector, 3, Out));
	TestFalse(TEXT("Failed lookup clears old address"), Out.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchHashTest, "Astraeon.Planet.Patches.StableHashGoldenVectors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchHashTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	const uint8 Hello[] = {'h','e','l','l','o'};
	TestEqual(TEXT("Published FNV-1a hello vector"), FAstraeonStableHash64::Bytes(MakeArrayView(Hello)), 0xa430d84680aabd0bULL);
	const auto A = Address(); const auto P = Planet();
	uint64 Seed = 0;
	TestTrue(TEXT("Seed derives"), A.TryDeriveSeed(P, EAstraeonGenerationChannel::Terrain, Seed));
	// Independently calculated from format bytes with integer FNV-1a, not UE's hash.
	TestEqual(TEXT("Frozen patch seed format 1"), Seed, 0x15f6d44ed0533175ULL);
	TSet<uint64> Seeds; Seeds.Add(Seed);
	for (int32 Field = 0; Field < 9; ++Field)
	{
		auto ChangedA = A; auto ChangedP = P; auto Channel = EAstraeonGenerationChannel::Terrain;
		switch (Field)
		{
		case 0: ++ChangedP.WorldSeed; break;
		case 1: ++ChangedP.BodySeed; break;
		case 2: Channel = EAstraeonGenerationChannel::Entities; break;
		case 3: ChangedA.Face = EAstraeonPlanetFace::NegativeZ; break;
		case 4: ++ChangedA.Lod; break;
		case 5: ++ChangedA.X; break;
		case 6: ++ChangedA.Y; break;
		case 7: ++ChangedP.GeneratorVersion; break;
		case 8: ChangedP.BodyId = ChangedA.BodyId = TEXT("second_body"); break;
		}
		uint64 Other;
		TestTrue(TEXT("Changed input derives"), ChangedA.TryDeriveSeed(ChangedP, Channel, Other));
		Seeds.Add(Other);
	}
	TestEqual(TEXT("Each seed input separates its domain"), Seeds.Num(), 10);
	auto UpperA = A; auto UpperP = P;
	UpperA.BodyId = UpperP.BodyId = TEXT("TEST_BODY");
	uint64 Other;
	TestTrue(TEXT("Case-insensitive body identity derives"), UpperA.TryDeriveSeed(UpperP, EAstraeonGenerationChannel::Terrain, Other));
	TestEqual(TEXT("FName display casing never changes a seed"), Other, Seed);
	UpperP.BodyId = TEXT("wrong_body");
	TestFalse(TEXT("Body mismatch rejected"), A.TryDeriveSeed(UpperP, EAstraeonGenerationChannel::Terrain, Other));
	TestFalse(TEXT("Unknown channel rejected"), A.TryDeriveSeed(P, EAstraeonGenerationChannel(99), Other));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchMeshTest, "Astraeon.Planet.Patches.MeshBudgetAndPrecision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchMeshTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	for (double Radius : {1000000.0, 50000000.0, 250000000.0})
	for (int32 Face = 0; Face < 6; ++Face)
	{
		const auto P = Planet(Radius); auto A = Address(18, 100000, 180000); A.Face = EAstraeonPlanetFace(Face);
		FAstraeonPlanetPatchBuildResult D;
		if (!TestTrue(TEXT("Local patch builds at engineering tier"), Build(P, A, D))) return false;
		TestTrue(TEXT("Generated result validates"), D.IsValid());
		TestEqual(TEXT("Fixed surface budget independent of radius"), D.SurfaceVertexCount, 1089);
		TestEqual(TEXT("Bounded skirt vertex budget"), D.Vertices.Num(), 1221);
		TestEqual(TEXT("Bounded skirt index budget"), D.Indices.Num(), 6912);
		for (int32 I = 0; I < D.SurfaceVertexCount; ++I)
		{
			const FVector Reconstructed = D.OriginBodyCm + FVector(FVector3f(D.Vertices[I]));
			TestTrue(TEXT("Float local render conversion loses less than 0.01 cm at Stress"),
				Reconstructed.Equals(D.OriginBodyCm + D.Vertices[I], 0.01));
		}
		for (int32 I = 0; I < D.SurfaceIndexCount; I += 3)
		{
			const FVector V = D.Vertices[D.Indices[I]], B = D.Vertices[D.Indices[I+1]], C = D.Vertices[D.Indices[I+2]];
			TestTrue(TEXT("Unreal winding is outward at every orientation"), FVector::DotProduct(FVector::CrossProduct(B-V,C-V), V+D.OriginBodyCm) < 0.0);
		}
		for (int32 I = D.SurfaceVertexCount; I < D.Vertices.Num(); ++I)
		{
			const FVector Bottom = D.Vertices[I] + D.OriginBodyCm;
			const FVector Dir = Bottom.GetSafeNormal();
			TestTrue(TEXT("Skirt extrudes toward body centre, never global down"),
				FMath::IsNearlyEqual(Bottom.Size(), P.RadiusCm + FAstraeonPlanetSurface::SampleRadialHeightCm(P, Dir) - 100.0, 0.001));
		}
		FAstraeonPlanetPatchBuildResult Again;
		if (!TestTrue(TEXT("Patch regenerates independently"), Build(P, A, Again))) return false;
		TestTrue(TEXT("Return reproduces complete geometry"), D.Vertices == Again.Vertices && D.Normals == Again.Normals && D.Indices == Again.Indices);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchSeamTest, "Astraeon.Planet.Patches.SharedEdgesAndLodSamples",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchSeamTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	const auto P = Planet(250000000.0);
	for (int32 Face = 0; Face < 6; ++Face)
	{
		auto A = Address(12, 1300, 1700); A.Face = EAstraeonPlanetFace(Face);
		auto B = A; ++B.X;
		FAstraeonPlanetPatchBuildResult DA, DB;
		if (!Build(P, A, DA) || !Build(P, B, DB)) { AddError(TEXT("Adjacent build failed")); return false; }
		for (int32 Y = 0; Y <= 32; ++Y)
		{
			const int32 I = Y * 33 + 32, J = Y * 33;
			TestTrue(TEXT("Same-LOD edge positions agree"), (DA.Vertices[I]+DA.OriginBodyCm).Equals(DB.Vertices[J]+DB.OriginBodyCm, 1.e-6));
			TestTrue(TEXT("Same-LOD normals agree"), DA.Normals[I].Equals(DB.Normals[J], 1.e-10));
		}
		for (uint8 Quadrant = 0; Quadrant < 4; ++Quadrant)
		{
			FAstraeonPlanetPatchAddress Child; A.TryChild(Quadrant, Child);
			if (!Build(P, Child, DB)) { AddError(TEXT("Child build failed")); return false; }
			for (int32 Y = 0; Y <= 16; ++Y)
			for (int32 X = 0; X <= 16; ++X)
			{
				const int32 I = (Y + (Quadrant >> 1) * 16) * 33 + X + (Quadrant & 1) * 16;
				const int32 J = (Y * 2) * 33 + X * 2;
				TestTrue(TEXT("Coarse/fine shared samples agree without patch-seeded height"),
					(DA.Vertices[I]+DA.OriginBodyCm).Equals(DB.Vertices[J]+DB.OriginBodyCm, 1.e-6));
				TestTrue(TEXT("Coarse/fine normals agree"), DA.Normals[I].Equals(DB.Normals[J], 1.e-10));
			}
		}
	}
	// All 24 directed face edges, including their two corners; search the matching
	// geometric edge on the other five faces, independent of a neighbour table.
	TArray<FAstraeonPlanetPatchBuildResult> Faces;
	for (int32 Face = 0; Face < 6; ++Face)
	{
		auto A = Address(0, 0, 0); A.Face = EAstraeonPlanetFace(Face);
		FAstraeonPlanetPatchBuildResult D;
		if (!Build(P, A, D)) { AddError(TEXT("Face build failed")); return false; }
		Faces.Add(MoveTemp(D));
	}
	for (int32 F = 0; F < 6; ++F)
	for (int32 Y = 0; Y <= 32; ++Y)
	for (int32 X = 0; X <= 32; ++X)
	{
		if (X != 0 && X != 32 && Y != 0 && Y != 32) continue;
		const int32 I = Y * 33 + X;
		const FVector V = Faces[F].Vertices[I] + Faces[F].OriginBodyCm;
		bool Matched = false;
		for (int32 G = 0; G < 6 && !Matched; ++G)
		{
			if (F == G) continue;
			for (int32 J = 0; J < Faces[G].SurfaceVertexCount; ++J)
			{
				if (!V.Equals(Faces[G].Vertices[J] + Faces[G].OriginBodyCm, 1.e-6)) continue;
				TestTrue(TEXT("Cross-face normals agree"), Faces[F].Normals[I].Equals(Faces[G].Normals[J], 1.e-10));
				Matched = true; break;
			}
		}
		TestTrue(TEXT("Every face boundary sample has a matching neighbour"), Matched);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchInvalidTest, "Astraeon.Planet.Patches.InvalidAndCancelledBuilds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchInvalidTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	for (int32 Case = 0; Case < 8; ++Case)
	{
		auto P = Planet(); auto A = Address(); FAstraeonPlanetPatchBuildOptions O;
		uint64 Revision = 1;
		switch (Case)
		{
		case 0: Revision = 0; break;
		case 1: O.Quads = 31; break;
		case 2: O.Quads = 1024; break;
		case 3: O.SkirtDepthCm = -1; break;
		case 4: O.SkirtDepthCm = std::numeric_limits<double>::quiet_NaN(); break;
		case 5: P.GeneratorVersion = 999; break;
		case 6: A.BodyId = TEXT("wrong_body"); break;
		case 7: O.SkirtDepthCm = P.RadiusCm; break;
		}
		FAstraeonPlanetPatchBuildResult Result; Result.Vertices.Add(FVector(1));
		TestTrue(TEXT("Invalid build rejected"), FAstraeonPlanetPatchMesh::Build(P, A, Revision, O, Result) == EAstraeonPatchBuildStatus::InvalidInput);
		TestTrue(TEXT("Failure clears old geometry"), Result.Vertices.IsEmpty() && Result.BuildRevision == 0);
	}
	std::atomic<bool> Cancelled(true);
	FAstraeonPlanetPatchBuildResult D;
	TestTrue(TEXT("Pre-cancelled request produces no mesh"), FAstraeonPlanetPatchMesh::Build(Planet(), Address(), 1, {}, D, &Cancelled) == EAstraeonPatchBuildStatus::Cancelled);
	TestFalse(TEXT("Empty result cannot commit"), D.IsValid());
	if (!Build(Planet(), Address(), D)) return false;
	D.Indices[0] = D.SurfaceVertexCount;
	TestFalse(TEXT("Collision index cannot refer to skirt"), D.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchWorkersTest, "Astraeon.Planet.Streaming.RevisionsCancellationAndBackpressure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchWorkersTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	FAstraeonPlanetStreamingManager Manager;
	const auto P = Planet(); const auto A = Address(); const auto B = Address(3, 3, 5);
	FAstraeonPlanetPatchBuildOptions Options;
	uint64 OldRevision, NewRevision, RefusedRevision;
	TestTrue(TEXT("First request accepted"), Manager.Request(P, A, Options, OldRevision) == EAstraeonPatchRequestStatus::Accepted);
	TestTrue(TEXT("Replacement accepted"), Manager.Request(P, A, Options, NewRevision) == EAstraeonPatchRequestStatus::Accepted);
	TestTrue(TEXT("Replacement revision increases"), NewRevision > OldRevision);
	TestFalse(TEXT("Old result cannot commit"), Manager.IsCurrent(A, OldRevision));
	TestTrue(TEXT("Outstanding work has a hard bound"), Manager.Request(P, B, Options, RefusedRevision) == EAstraeonPatchRequestStatus::AtCapacity);
	TestEqual(TEXT("Refusal issues no revision"), RefusedRevision, uint64(0));
	TestTrue(TEXT("Refusal preserves accepted revision"), Manager.IsCurrent(A, NewRevision));
	TArray<FAstraeonPlanetPatchCompletion> Completed;
	if (!TestTrue(TEXT("Workers finish in bounded time"), Drain(Manager, Completed))) return false;
	if (!TestEqual(TEXT("Only latest result is delivered"), Completed.Num(), 1)) return false;
	TestTrue(TEXT("Latest worker succeeds"), Completed[0].Status == EAstraeonPatchBuildStatus::Success);
	FAstraeonPlanetPatchBuildResult Direct;
	if (!Build(P, A, Direct)) return false;
	TestTrue(TEXT("Worker matches synchronous geometry exactly"), Direct.Vertices == Completed[0].Mesh.Vertices && Direct.Normals == Completed[0].Mesh.Normals);
	Manager.Release(A);
	TestFalse(TEXT("Collected result becomes stale when unloaded"), Manager.IsCurrent(A, NewRevision));
	TestTrue(TEXT("Unload/reload accepted"), Manager.Request(P, A, Options, RefusedRevision) == EAstraeonPatchRequestStatus::Accepted);
	TestTrue(TEXT("Unload never reuses revision (ABA)"), RefusedRevision > NewRevision);
	Manager.Release(A); Completed.Reset();
	TestTrue(TEXT("Cancelled jobs drain"), Drain(Manager, Completed));
	TestTrue(TEXT("Released patch never arrives"), Completed.IsEmpty());
	TestEqual(TEXT("Released address does not leak registry state"), Manager.GetTrackedPatchCount(), 0);
	Options.Quads = 31;
	TestTrue(TEXT("Bad worker input rejected before enqueue"), Manager.Request(P, A, Options, RefusedRevision) == EAstraeonPatchRequestStatus::InvalidInput);
	TestEqual(TEXT("Bad input creates no task"), Manager.GetOutstandingBuildCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchOrderTest, "Astraeon.Planet.Streaming.SubmissionOrderDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchOrderTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonPatchTests;
	const auto P = Planet(); const auto A = Address(); const auto B = Address(3, 3, 5);
	TMap<FAstraeonPlanetPatchAddress, FAstraeonPlanetPatchBuildResult> First;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		FAstraeonPlanetStreamingManager Manager;
		uint64 Revision;
		TestTrue(TEXT("Request one accepted"), Manager.Request(P, Pass == 0 ? A : B, {}, Revision) == EAstraeonPatchRequestStatus::Accepted);
		TestTrue(TEXT("Request two accepted"), Manager.Request(P, Pass == 0 ? B : A, {}, Revision) == EAstraeonPatchRequestStatus::Accepted);
		TArray<FAstraeonPlanetPatchCompletion> Completed;
		if (!TestTrue(TEXT("Both workers finish"), Drain(Manager, Completed))) return false;
		if (!TestEqual(TEXT("Both addresses delivered"), Completed.Num(), 2)) return false;
		for (auto& Item : Completed)
		{
			TestTrue(TEXT("Independent task succeeds"), Item.Status == EAstraeonPatchBuildStatus::Success);
			if (Pass == 0) First.Add(Item.Address, MoveTemp(Item.Mesh));
			else
			{
				const auto* Prior = First.Find(Item.Address);
				if (!TestNotNull(TEXT("Address survives ordering"), Prior)) return false;
				TestEqual(TEXT("Seed independent of scheduling"), Item.Mesh.PatchSeed, Prior->PatchSeed);
				TestTrue(TEXT("Geometry independent of submission and collection ordering"), Item.Mesh.Vertices == Prior->Vertices && Item.Mesh.Indices == Prior->Indices);
			}
			Manager.Release(Item.Address);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonPatchCollisionBridgeTest, "Astraeon.Planet.Patches.CollisionBridgeMatchesFinestPatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonPatchCollisionBridgeTest::RunTest(const FString& Parameters)
{
	// P2.3 keeps whole-face collision until P2.4. It is only honest if a face built at
	// Quads << FinestLod is, triangle by triangle and in the same winding, the finest patches.
	using namespace AstraeonPatchTests;
	const auto P = Planet(20000.0);
	const FAstraeonPlanetLODSettings Settings;
	const uint8 Finest = FAstraeonPlanetLODManager::FinestAllowedLod(P, Settings);
	const int32 Grid = Settings.Quads << Finest;
	if (!TestTrue(TEXT("Lab radius fits the builder"), Finest > 0 && Grid <= 128)) return false;
	const int32 Q = Settings.Quads, Count = 1 << Finest;
	for (int32 Face = 0; Face < 6; ++Face)
	{
		FAstraeonCubeSphereMesh Whole;
		if (!TestTrue(TEXT("Collision face builds"), FAstraeonCubeSphereMesh::BuildFace(P, EAstraeonPlanetFace(Face), Grid, Whole))) return false;
		for (int32 PY = 0; PY < Count; ++PY)
		for (int32 PX = 0; PX < Count; ++PX)
		{
			auto A = Address(Finest, PX, PY); A.BodyId = P.BodyId; A.Face = EAstraeonPlanetFace(Face);
			FAstraeonPlanetPatchBuildResult Patch;
			if (!Build(P, A, Patch)) { AddError(TEXT("Finest patch build failed")); return false; }
			int32 Mismatches = 0;
			for (int32 Y = 0; Y < Q; ++Y)
			for (int32 X = 0; X < Q; ++X)
			{
				const int32 PatchCell = (Y * Q + X) * 6, FaceCell = ((PY * Q + Y) * Grid + PX * Q + X) * 6;
				for (int32 K = 0; K < 6; ++K)
				{
					const FVector Rendered = Patch.Vertices[Patch.Indices[PatchCell + K]] + Patch.OriginBodyCm;
					const FVector Collided = Whole.Vertices[Whole.Indices[FaceCell + K]] + Whole.OriginBodyCm;
					Mismatches += !Rendered.Equals(Collided, 1.e-6);
				}
			}
			TestEqual(TEXT("Every collision triangle is a rendered triangle, same corner order"), Mismatches, 0);
		}
	}
	AddInfo(FString::Printf(TEXT("CollisionBridge radius_cm=%.0f finest_lod=%d grid=%d"), P.RadiusCm, Finest, Grid));
	return true;
}

#endif
