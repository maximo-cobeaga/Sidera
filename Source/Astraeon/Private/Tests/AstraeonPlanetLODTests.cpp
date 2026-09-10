#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
#include "Planet/Surface/AstraeonPlanetSurface.h"
#include "HAL/PlatformTime.h"

namespace AstraeonLODTests
{
	FAstraeonPlanetDefinition Planet(double Radius)
	{
		FAstraeonPlanetDefinition P;
		P.BodyId = TEXT("lod_lab"); P.RadiusCm = Radius; P.MassKg = 1.e16; P.SurfaceGravityMS2 = 9.81; P.BodySeed = 4242;
		return P;
	}
	TArray<FAstraeonPlanetPatchAddress> Roots()
	{
		TArray<FAstraeonPlanetPatchAddress> Result;
		for (uint8 F = 0; F < 6; ++F) { FAstraeonPlanetPatchAddress A; A.BodyId = TEXT("lod_lab"); A.Face = EAstraeonPlanetFace(F); Result.Add(A); }
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLODNeighborsTest, "Astraeon.Planet.LOD.NeighborsAcrossAllFaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonLODNeighborsTest::RunTest(const FString& Parameters)
{
	for (uint8 Level : {uint8(0), uint8(1), uint8(4), uint8(13), uint8(24)})
	for (uint8 F = 0; F < 6; ++F)
	for (int32 X : {0, (1 << Level) / 2, (1 << Level) - 1})
	for (int32 Y : {0, (1 << Level) / 2, (1 << Level) - 1})
	for (uint8 E = 0; E < 4; ++E)
	{
		FAstraeonPlanetPatchAddress A; A.BodyId = TEXT("lod_lab"); A.Face = EAstraeonPlanetFace(F); A.Lod = Level; A.X = X; A.Y = Y;
		FAstraeonPlanetPatchAddress Neighbor;
		if (!TestTrue(TEXT("Every edge has a neighbour"), FAstraeonPlanetLODManager::SameLevelNeighbor(A, EAstraeonPatchEdge(E), Neighbor))) return false;
		TestFalse(TEXT("Neighbour is not self"), Neighbor == A);
		TestEqual(TEXT("Level preserved across face seam"), Neighbor.Lod, A.Lod);
		bool Reciprocal = false;
		for (uint8 Back = 0; Back < 4; ++Back)
		{
			FAstraeonPlanetPatchAddress Found;
			FAstraeonPlanetLODManager::SameLevelNeighbor(Neighbor, EAstraeonPatchEdge(Back), Found);
			Reciprocal |= Found == A;
		}
		TestTrue(TEXT("Edge adjacency is reciprocal including corners"), Reciprocal);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLODSelectionTest, "Astraeon.Planet.LOD.BalancedDeterministicEngineeringTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonLODSelectionTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonLODTests;
	int32 MaxFine = 0, MinFine = MAX_int32;
	for (double Radius : {1000000.0, 50000000.0, 250000000.0})
	{
		const auto P = Planet(Radius);
		for (const FVector Dir : {FVector(0,0,1), FVector(1,1,0).GetSafeNormal(), FVector(-1,-1,-1).GetSafeNormal()})
		{
			FAstraeonPlanetLODView V; V.ObserverBodyCm = Dir * (Radius + 1000.0);
			FAstraeonPlanetLODSelection Selected, Again;
			const double Start = FPlatformTime::Seconds();
			if (!TestTrue(TEXT("Selection succeeds"), FAstraeonPlanetLODManager::Select(P,V,{},Selected))) return false;
			TestTrue(TEXT("Complete balanced cover at poles, edges and corners"), FAstraeonPlanetLODManager::ValidateCover(Selected.Leaves));
			TestTrue(TEXT("Bounded active leaf count"), Selected.Leaves.Num() <= 384);
			if (!TestTrue(TEXT("Repeat selection succeeds"), FAstraeonPlanetLODManager::Select(P,V,{},Again))) return false;
			TestTrue(TEXT("Selection and ordering deterministic"), Selected.Leaves == Again.Leaves);
			int32 Fine = 0; uint8 Finest = 0;
			for (const auto& A : Selected.Leaves) { Finest = FMath::Max(Finest,A.Lod); if (A.Lod == Selected.FinestAllowedLod) ++Fine; }
			TestEqual(TEXT("Near observer reaches physical resolution at every radius"), Finest, Selected.FinestAllowedLod);
			MinFine = FMath::Min(MinFine,Fine); MaxFine = FMath::Max(MaxFine,Fine);
			AddInfo(FString::Printf(TEXT("LOD radius_cm=%.0f leaves=%d fine=%d level=%d budget_limited=%d two_selections_ms=%.2f"),
				Radius,Selected.Leaves.Num(),Fine,Finest,Selected.bBudgetLimited,(FPlatformTime::Seconds()-Start)*1000.0));
		}
	}
	// The budget, not the planet radius, bounds active finest leaves. A 108-leaf case
	// is expected at the closest engineering view; it is still well below the 384 cap.
	TestTrue(TEXT("High detail stays bounded instead of scaling with radius"), MinFine > 0 && MaxFine <= 128);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLODCoverTest, "Astraeon.Planet.LOD.RejectsHolesOverlapsAndUnbalancedTrees",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonLODCoverTest::RunTest(const FString& Parameters)
{
	using namespace AstraeonLODTests;
	auto Leaves = Roots();
	TestTrue(TEXT("Six roots cover sphere"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	Leaves.RemoveAt(0); TestFalse(TEXT("Hole is rejected"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	Leaves = Roots(); const auto Root = Leaves[0]; Leaves.Add(Root);
	TestFalse(TEXT("Duplicate is rejected"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	Leaves = Roots(); FAstraeonPlanetPatchAddress Child; Root.TryChild(0,Child); Leaves.Add(Child);
	TestFalse(TEXT("Parent-child overlap is rejected"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	Leaves = Roots(); Leaves.Remove(Root);
	for (uint8 Q = 0; Q < 4; ++Q) { Root.TryChild(Q,Child); Leaves.Add(Child); }
	TestTrue(TEXT("Single level split is balanced"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	Root.TryChild(0,Child); Leaves.Remove(Child);
	for (uint8 Q = 0; Q < 4; ++Q) { FAstraeonPlanetPatchAddress Grandchild; Child.TryChild(Q,Grandchild); Leaves.Add(Grandchild); }
	TestFalse(TEXT("Two-level delta at another cube face is rejected"), FAstraeonPlanetLODManager::ValidateCover(Leaves));
	TestTrue(TEXT("The same unbalanced set is still a partition, as mid-relay screens are"), FAstraeonPlanetLODManager::ValidatePartition(Leaves));
	auto Holed = Leaves; Holed.Pop();
	TestFalse(TEXT("Partition rejects a hole"), FAstraeonPlanetLODManager::ValidatePartition(Holed));
	auto Overlapped = Leaves; Overlapped.Add(Child);
	TestFalse(TEXT("Partition rejects overlap"), FAstraeonPlanetLODManager::ValidatePartition(Overlapped));
	TestEqual(TEXT("Six roots have delta zero"), FAstraeonPlanetLODManager::MaxNeighborLodDelta(Roots()), 0);
	TestEqual(TEXT("Unbalanced partition reports delta two"), FAstraeonPlanetLODManager::MaxNeighborLodDelta(Leaves), 2);
	TestEqual(TEXT("Delta of a non-partition is an error"), FAstraeonPlanetLODManager::MaxNeighborLodDelta(Holed), -1);
	FAstraeonPlanetLODView V; V.ObserverBodyCm = FVector(1,0,0) * 50001000.0;
	FAstraeonPlanetLODSettings S; S.MaxPatches = 6;
	FAstraeonPlanetLODSelection Out;
	TestTrue(TEXT("Minimal budget still gives complete sphere"), FAstraeonPlanetLODManager::Select(Planet(50000000.0),V,S,Out));
	TestTrue(TEXT("Budget pressure is explicit"), Out.bBudgetLimited && Out.Leaves.Num() == 6);
	S.MaxPatches = 5;
	TestFalse(TEXT("Impossible budget rejected"), FAstraeonPlanetLODManager::Select(Planet(50000000.0),V,S,Out));
	TestTrue(TEXT("Failed selection clears result"), Out.Leaves.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLODFastSelectTest, "Astraeon.Planet.LOD.FastSelectionMatchesReference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonLODFastSelectTest::RunTest(const FString& Parameters)
{
	// TL_12 measured the first selector at 19 ms per call. The fast one must be a pure speedup:
	// same leaves, same budget flag, same finest level, for every view tried here.
	using namespace AstraeonLODTests;
	double FastSeconds = 0, ReferenceSeconds = 0;
	int32 Cases = 0, BudgetLimited = 0;
	for (double Radius : {20000.0, 5000000.0, 50000000.0, 250000000.0})
	for (int32 X = -1; X <= 1; ++X)
	for (int32 Y = -1; Y <= 1; ++Y)
	for (int32 Z = -1; Z <= 1; ++Z)
	for (double Altitude : {2000.0, 100000.0, 10000000.0})
	for (int32 Budget : {384, 60})
	{
		if (!X && !Y && !Z) continue;
		// Off the exact cardinal: an irregular offset avoids only testing symmetric ties.
		const FVector Dir = (FVector(X, Y, Z) + FVector(0.137, -0.071, 0.029) * (X + 2 * Y + 3 * Z)).GetSafeNormal();
		if (Dir.IsNearlyZero()) continue;
		const auto P = Planet(Radius);
		FAstraeonPlanetLODView V; V.ObserverBodyCm = Dir * (Radius + Altitude);
		V.VelocityBodyCmS = FVector::CrossProduct(Dir, FVector(0.3, 0.2, 1)).GetSafeNormal() * 5000.0;
		FAstraeonPlanetLODSettings S; S.MaxPatches = Budget;
		FAstraeonPlanetLODSelection Fast, Reference;
		double Start = FPlatformTime::Seconds();
		const bool bFast = FAstraeonPlanetLODManager::Select(P, V, S, Fast);
		FastSeconds += FPlatformTime::Seconds() - Start;
		Start = FPlatformTime::Seconds();
		const bool bReference = FAstraeonPlanetLODManager::SelectReference(P, V, S, Reference);
		ReferenceSeconds += FPlatformTime::Seconds() - Start;
		++Cases; BudgetLimited += Reference.bBudgetLimited;
		if (!TestEqual(TEXT("Same success"), bFast, bReference)
			|| !TestTrue(FString::Printf(TEXT("Same leaves (radius %.0f, dir %s, altitude %.0f, budget %d)"), Radius, *Dir.ToString(), Altitude, Budget),
				Fast.Leaves == Reference.Leaves)
			|| !TestEqual(TEXT("Same budget flag"), Fast.bBudgetLimited, Reference.bBudgetLimited)
			|| !TestEqual(TEXT("Same finest level"), Fast.FinestAllowedLod, Reference.FinestAllowedLod)) return false;
	}
	TestTrue(TEXT("Budget pressure was exercised"), BudgetLimited > 0 && BudgetLimited < Cases);
	TestTrue(TEXT("Fast path is faster"), FastSeconds < ReferenceSeconds);
	AddInfo(FString::Printf(TEXT("FastSelect cases=%d budget_limited=%d fast_ms_per_call=%.3f reference_ms_per_call=%.3f"),
		Cases, BudgetLimited, 1000.0 * FastSeconds / Cases, 1000.0 * ReferenceSeconds / Cases));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstraeonLODSkirtTest, "Astraeon.Planet.LOD.SkirtsCoverCoarseNeighbourSeams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAstraeonLODSkirtTest::RunTest(const FString& Parameters)
{
	// Where a fine patch meets a coarser one, the coarse edge is a straight chord between two
	// shared samples and the fine edge has extra samples in between. The crack between them is
	// hidden only if the skirt hanging from the upper edge reaches below the lower one: a fine
	// vertex above the chord needs the fine skirt, one below it needs the coarse skirt.
	using namespace AstraeonLODTests;
	const FAstraeonPlanetLODSettings Settings;
	const int32 Q = Settings.Quads;
	constexpr int32 CellsPerLine = 1024;
	for (double Radius : {20000.0, 5000000.0, 50000000.0})
	{
		const auto P = Planet(Radius);
		const auto Position = [&P](EAstraeonPlanetFace Face, double U, double V)
		{
			const FVector Dir = FAstraeonPlanetCoordinates::FaceUvToDirection(Face, FVector2D(U, V));
			return Dir * (P.RadiusCm + FAstraeonPlanetSurface::SampleRadialHeightCm(P, Dir));
		};
		const uint8 Finest = FAstraeonPlanetLODManager::FinestAllowedLod(P, Settings);
		for (int32 Delta = 1; Delta <= 2; ++Delta)
		{
			double WorstRatio = 0, WorstAbove = 0, WorstBelow = 0;
			for (int32 Fine = Delta; Fine <= Finest; ++Fine)
			{
				FAstraeonPlanetPatchAddress FineA; FineA.Lod = uint8(Fine);
				FAstraeonPlanetPatchAddress CoarseA; CoarseA.Lod = uint8(Fine - Delta);
				const double FineSkirt = FAstraeonPlanetLODManager::SkirtDepthCm(P, FineA, Q);
				const double CoarseSkirt = FAstraeonPlanetLODManager::SkirtDepthCm(P, CoarseA, Q);
				const int64 Cells = (int64(1) << (Fine - Delta)) * Q;
				const int64 Stride = FMath::Max<int64>(1, Cells / CellsPerLine);
				double Above = 0, Below = 0;
				for (uint8 F = 0; F < 6; ++F)
				for (const int64 Column : {int64(0), Cells / 2}) // A cube-face seam and an interior patch boundary.
				for (int64 J = 0; J < Cells; J += Stride)
				{
					const EAstraeonPlanetFace Face = EAstraeonPlanetFace(F);
					const double U = -1.0 + 2.0 * double(Column) / double(Cells);
					const double V0 = -1.0 + 2.0 * double(J) / double(Cells), V1 = -1.0 + 2.0 * double(J + 1) / double(Cells);
					const FVector C0 = Position(Face, U, V0), D = Position(Face, U, V1) - C0;
					for (int32 K = 1; K < (1 << Delta); ++K)
					{
						const FVector FineVertex = Position(Face, U, FMath::Lerp(V0, V1, double(K) / (1 << Delta)));
						const FVector Ray = FineVertex.GetSafeNormal();
						// Chord point on the same ray: the offset perpendicular to the ray vanishes.
						const FVector C0Perp = C0 - (C0 | Ray) * Ray, DPerp = D - (D | Ray) * Ray;
						const double S = -(C0Perp | DPerp) / FMath::Max(DPerp.SizeSquared(), UE_DOUBLE_SMALL_NUMBER);
						const double Gap = FineVertex.Size() - ((C0 + S * D) | Ray);
						Above = FMath::Max(Above, Gap); Below = FMath::Max(Below, -Gap);
					}
				}
				WorstAbove = FMath::Max(WorstAbove, Above); WorstBelow = FMath::Max(WorstBelow, Below);
				WorstRatio = FMath::Max(WorstRatio, FMath::Max(Above / FineSkirt, Below / CoarseSkirt));
				// Delta 2 only exists mid-relay, but it is on screen then: measured 0.855 of the
				// skirt at Target on 2026-09-10, so the delta-1 sizing covers it as well.
				TestTrue(FString::Printf(TEXT("Fine skirt hides fine-above-chord crack (radius %.0f, lod %d, delta %d)"), Radius, Fine, Delta), Above <= FineSkirt);
				TestTrue(FString::Printf(TEXT("Coarse skirt hides fine-below-chord crack (radius %.0f, lod %d, delta %d)"), Radius, Fine, Delta), Below <= CoarseSkirt);
			}
			AddInfo(FString::Printf(TEXT("SkirtSeams radius_cm=%.0f delta=%d finest=%d worst_above_cm=%.2f worst_below_cm=%.2f worst_gap_over_skirt=%.3f"),
				Radius, Delta, Finest, WorstAbove, WorstBelow, WorstRatio));
		}
	}
	return true;
}
#endif
