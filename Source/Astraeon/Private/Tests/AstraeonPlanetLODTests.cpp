#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Planet/LOD/AstraeonPlanetLODManager.h"
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
#endif
